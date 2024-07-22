/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "RenderFrontend.h"
#include <Engine/GameApplication/GameApplication.h>

#include <Engine/Core/Profiler.h>
#include <Engine/Core/Platform.h>

#include <Engine/World/Modules/Render/Components/CameraComponent.h>
#include <Engine/World/Modules/Render/Components/MeshComponent.h>
#include <Engine/World/Modules/Render/Components/TerrainComponent.h>
#include <Engine/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Engine/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Engine/World/Modules/Render/RenderInterface.h>
#include <Engine/World/Modules/Render/TerrainView.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_RenderView("r_RenderView"s, "1"s, CVAR_CHEAT);
ConsoleVar r_ResolutionScaleX("r_ResolutionScaleX"s, "1"s);
ConsoleVar r_ResolutionScaleY("r_ResolutionScaleY"s, "1"s);
ConsoleVar r_RenderLightPortals("r_RenderLightPortals"s, "1"s);
ConsoleVar r_VertexLight("r_VertexLight"s, "0"s);
ConsoleVar r_MotionBlur("r_MotionBlur"s, "1"s);
ConsoleVar r_RenderMeshes("r_RenderMeshes"s, "1"s, CVAR_CHEAT);
ConsoleVar r_RenderTerrain("r_RenderTerrain"s, "1"s, CVAR_CHEAT);

extern ConsoleVar r_HBAO;
extern ConsoleVar r_HBAODeinterleaved;

ConsoleVar com_DrawFrustumClusters("com_DrawFrustumClusters"s, "0"s, CVAR_CHEAT);

RenderFrontend::RenderFrontend()
{
    GameApplication::GetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(256, 256))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_PhotometricProfiles);

    m_PhotometricProfiles->SetDebugName("Photometric Profiles");
}

void RenderFrontend::Render(FrameLoop* frameLoop, Canvas* canvas)
{
    HK_PROFILER_EVENT("Render frontend");

    Vector<WorldRenderView*> const& renderViews = frameLoop->GetRenderViews();
    StreamedMemoryGPU* streamedMemory = frameLoop->GetStreamedMemoryGPU();

    m_FrameLoop = frameLoop;
    m_FrameNumber = frameLoop->SysFrameNumber();
    m_DebugDraw.Reset();

    m_Stat.FrontendTime = Core::SysMilliseconds();
    m_Stat.PolyCount = 0;
    m_Stat.ShadowMapPolyCount = 0;

    m_FrameData.FrameNumber = m_FrameNumber;

    m_FrameData.pCanvasDrawData = canvas->GetDrawData();

    if (m_FrameData.pCanvasDrawData->VertexCount > 0)
        m_FrameData.CanvasVertexData = streamedMemory->AllocateVertex(m_FrameData.pCanvasDrawData->VertexCount * sizeof(CanvasVertex), m_FrameData.pCanvasDrawData->Vertices);
    else
        m_FrameData.CanvasVertexData = 0;

    m_FrameData.Instances.Clear();
    m_FrameData.TranslucentInstances.Clear();
    m_FrameData.OutlineInstances.Clear();
    m_FrameData.ShadowInstances.Clear();
    m_FrameData.LightPortals.Clear();
    m_FrameData.DirectionalLights.Clear();
    m_FrameData.LightShadowmaps.Clear();
    m_FrameData.TerrainInstances.Clear();

    m_FrameData.NumViews = renderViews.Size();
    m_FrameData.RenderViews = (RenderViewData*)frameLoop->AllocFrameMem(sizeof(RenderViewData) * m_FrameData.NumViews);
    Core::ZeroMem(m_FrameData.RenderViews, sizeof(RenderViewData) * m_FrameData.NumViews);

    for (int i = 0; i < m_FrameData.NumViews; i++)
    {
        WorldRenderView* worldRenderView = renderViews[i];
        RenderViewData* view = &m_FrameData.RenderViews[i];

        RenderView(worldRenderView, view);
    }

    SortRenderInstances();

    if (m_DebugDraw.CommandsCount() > 0)
    {
        m_FrameData.DbgCmds = m_DebugDraw.GetCmds().ToPtr();
        m_FrameData.DbgVertexStreamOffset = streamedMemory->AllocateVertex(m_DebugDraw.GetVertices().Size() * sizeof(DebugVertex), m_DebugDraw.GetVertices().ToPtr());
        m_FrameData.DbgIndexStreamOffset = streamedMemory->AllocateIndex(m_DebugDraw.GetIndices().Size() * sizeof(unsigned short), m_DebugDraw.GetIndices().ToPtr());
    }
    else
    {
        m_FrameData.DbgCmds = nullptr;
        m_FrameData.DbgVertexStreamOffset = 0;
        m_FrameData.DbgIndexStreamOffset = 0;
    }

    m_Stat.FrontendTime = Core::SysMilliseconds() - m_Stat.FrontendTime;
}

void RenderFrontend::ClearRenderView(RenderViewData* view)
{
    Core::ZeroMem(view, sizeof(*view));
}

static constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    {0.5f, 0.0f, 0.0f, 0.0f},
    {0.0f, -0.5f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 0.5f, 0.0f, 1.0f});

void RenderFrontend::AddShadowmapCascades(DirectionalLightComponent const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* StreamedMemory, RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades)
{
    float cascadeSplits[MAX_CASCADE_SPLITS];
    int numSplits = light.m_MaxShadowCascades + 1;
    int numVisibleSplits;
    Float4x4 lightViewMatrix;
    Float3 worldspaceVerts[MAX_CASCADE_SPLITS][4];
    Float3 right, up;

    HK_ASSERT(light.m_MaxShadowCascades > 0 && light.m_MaxShadowCascades <= MAX_SHADOW_CASCADES);

    if (View->bPerspective)
    {
        float tanFovX = std::tan(View->ViewFovX * 0.5f);
        float tanFovY = std::tan(View->ViewFovY * 0.5f);
        right = View->ViewRightVec * tanFovX;
        up = View->ViewUpVec * tanFovY;
    }
    else
    {
        float orthoWidth = View->ViewOrthoMaxs.X - View->ViewOrthoMins.X;
        float orthoHeight = View->ViewOrthoMaxs.Y - View->ViewOrthoMins.Y;
        right = View->ViewRightVec * Math::Abs(orthoWidth * 0.5f);
        up = View->ViewUpVec * Math::Abs(orthoHeight * 0.5f);
    }

    const float shadowMaxDistance = light.m_ShadowMaxDistance;
    const float offset = light.m_ShadowCascadeOffset;
    const float a = (shadowMaxDistance - offset) / View->ViewZNear;
    const float b = (shadowMaxDistance - offset) - View->ViewZNear;
    const float lambda = light.m_ShadowCascadeSplitLambda;

    // Calc splits
    cascadeSplits[0] = View->ViewZNear;
    cascadeSplits[MAX_CASCADE_SPLITS - 1] = shadowMaxDistance;

    for (int splitIndex = 1; splitIndex < MAX_CASCADE_SPLITS - 1; splitIndex++)
    {
        const float factor = (float)splitIndex / (MAX_CASCADE_SPLITS - 1);
        const float logarithmic = View->ViewZNear * Math::Pow(a, factor);
        const float linear = View->ViewZNear + b * factor;
        const float dist = Math::Lerp(linear, logarithmic, lambda);
        cascadeSplits[splitIndex] = offset + dist;
    }

    float maxVisibleDist = Math::Max(View->MaxVisibleDistance, cascadeSplits[0]);

    // Calc worldspace verts
    for (numVisibleSplits = 0;
        numVisibleSplits < numSplits && (cascadeSplits[Math::Max(0, numVisibleSplits - 1)] <= maxVisibleDist);
        numVisibleSplits++)
    {
        Float3* pWorldSpaceVerts = worldspaceVerts[numVisibleSplits];

        float d = cascadeSplits[numVisibleSplits];

        // FIXME: variable distance can cause edge shimmering
        //d = d > maxVisibleDist ? maxVisibleDist : d;

        Float3 centerWorldspace = View->ViewPosition + View->ViewDir * d;

        Float3 c1 = right + up;
        Float3 c2 = right - up;

        if (View->bPerspective)
        {
            c1 *= d;
            c2 *= d;
        }

        pWorldSpaceVerts[0] = centerWorldspace - c1;
        pWorldSpaceVerts[1] = centerWorldspace - c2;
        pWorldSpaceVerts[2] = centerWorldspace + c1;
        pWorldSpaceVerts[3] = centerWorldspace + c2;
    }

    int numVisibleCascades = numVisibleSplits - 1;

    BvSphere cascadeSphere;

    Float3x3 basis = rotationMat.Transposed();
    lightViewMatrix[0] = Float4(basis[0], 0.0f);
    lightViewMatrix[1] = Float4(basis[1], 0.0f);
    lightViewMatrix[2] = Float4(basis[2], 0.0f);

    const float halfCascadeRes = light.m_ShadowCascadeResolution >> 1;
    const float oneOverHalfCascadeRes = 1.0f / halfCascadeRes;

    int firstCascade = View->NumShadowMapCascades;

    // Distance from cascade bounds to light source (near clip plane)
    // NOTE: We can calc actual light distance from scene geometry,
    // but now it just a magic number big enough to enclose most scenes = 1km.
    const float lightDistance = 1000.0f;

    Float4x4* lightViewProjectionMatrices = nullptr;
    if (numVisibleCascades > 0)
    {
        *ViewProjStreamHandle = StreamedMemory->AllocateConstant(numVisibleCascades * sizeof(Float4x4), nullptr);

        lightViewProjectionMatrices = (Float4x4*)StreamedMemory->Map(*ViewProjStreamHandle);
    }

    Float4x4::OrthoMatrixDesc orthoDesc = {};
    for (int i = 0; i < numVisibleCascades; i++)
    {
        // Calc cascade bounding sphere
        cascadeSphere.FromPointsAverage(worldspaceVerts[i], 8);

        // Set light position at cascade center
        lightViewMatrix[3] = Float4(basis * -cascadeSphere.Center, 1.0f);

        // Set ortho box
        Float3 cascadeMins = Float3(-cascadeSphere.Radius);
        Float3 cascadeMaxs = Float3(cascadeSphere.Radius);

        // Offset near clip distance
        cascadeMins[2] -= lightDistance;

        // Calc light view projection matrix
        orthoDesc.Mins = Float2(cascadeMins);
        orthoDesc.Maxs = Float2(cascadeMaxs);
        orthoDesc.ZNear = cascadeMins[2];
        orthoDesc.ZFar = cascadeMaxs[2];
        Float4x4 cascadeMatrix = Float4x4::GetOrthoMatrix(orthoDesc) * lightViewMatrix;

#if 0
        // Calc pixel fraction in texture space
        Float2 error = Float2( cascadeMatrix[3] );  // same cascadeMatrix * Float4(0,0,0,1)
        error.X = Math::Fract( error.X * halfCascadeRes ) * oneOverHalfCascadeRes;
        error.Y = Math::Fract( error.Y * halfCascadeRes ) * oneOverHalfCascadeRes;

        // Snap light projection to texel grid
        // Same cascadeMatrix = Float4x4::Translation( -error ) * cascadeMatrix;
        cascadeMatrix[3].X -= error.X;
        cascadeMatrix[3].Y -= error.Y;
#else
        // Snap light projection to texel grid
        cascadeMatrix[3].X -= Math::Fract(cascadeMatrix[3].X * halfCascadeRes) * oneOverHalfCascadeRes;
        cascadeMatrix[3].Y -= Math::Fract(cascadeMatrix[3].Y * halfCascadeRes) * oneOverHalfCascadeRes;
#endif

        int cascadeIndex = firstCascade + i;

        lightViewProjectionMatrices[i] = cascadeMatrix;
        View->ShadowMapMatrices[cascadeIndex] = ShadowMapBias * cascadeMatrix * View->ClipSpaceToWorldSpace;
    }

    View->NumShadowMapCascades += numVisibleCascades;

    *pFirstCascade = firstCascade;
    *pNumCascades = numVisibleCascades;
}

// Convert direction to rotation matrix. Direction should be normalized.
static Float3x3 DirectionToMatrix(Float3 const& direction)
{
    Float3 dir = -direction;

    if (dir.X * dir.X + dir.Z * dir.Z == 0.0f)
    {
        return Float3x3(1, 0, 0,
            0, 0, -dir.Y,
            dir.X, dir.Y, dir.Z);
    }
    else
    {
        Float3 xaxis = Math::Cross(Float3(0.0f, 1.0f, 0.0f), dir).Normalized();

        return Float3x3(xaxis,
            Math::Cross(dir, xaxis),
            dir);
    }
}

HK_FORCEINLINE Float3x3 FixupLightRotation(Quat const& rotation)
{
    return DirectionToMatrix(-rotation.ZAxis());
}

void RenderFrontend::AddDirectionalLightShadows(LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef)
{
    if (!m_RenderDef.View->NumShadowMapCascades)
        return;

    //if (!r_RenderMeshes)
    //    return;
#if 0
    alignas(16) BvAxisAlignedBoxSSE bounds[4];
    alignas(16) int32_t cullResult[4];

    BvFrustum frustum[MAX_SHADOW_CASCADES];

    Float4x4* lightViewProjectionMatrices = (Float4x4*)m_RenderDef.StreamedMemory->Map(lightDef->ViewProjStreamHandle);

    for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
        frustum[cascadeIndex].FromMatrix(lightViewProjectionMatrices[cascadeIndex]);

    auto& meshManager = m_World->GetComponentManager<MeshComponent>();


    struct Visitor
    {
        HK_FORCEINLINE void Visit(MeshComponent* batch, int batchSize)
        {
        }
    };

    Visitor visitor;
    meshManager.IterateComponentBatches(visitor);

    for (auto it = meshManager.GetComponents(); it.IsValid(); ++it)
    {
        MeshComponent& mesh = *it;

        if (!mesh.m_CastShadow)
            continue;

        auto* meshResource = GameApplication::GetResourceManager().TryGet(mesh.m_Resource);
        if (!meshResource)
            continue;

        auto* gameObject = mesh.GetOwner();

        // TODO: Исправить это временное решение
        mesh.m_LerpPosition = gameObject->GetWorldPosition();
        mesh.m_LerpRotation = gameObject->GetWorldRotation();
        mesh.m_LerpScale    = gameObject->GetWorldScale();

        int numChunks = q.Count() / 4;
        int residual = q.Count() - numChunks * 4;

        HK_ASSERT(residual < 4);

        for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
        {
            BvFrustum const& cascadeFrustum = frustum[cascadeIndex];

            int n = 0;

            for (int i = 0; i < numChunks; i++, n += 4)
            {
                bounds[0] = mesh[n].m_WorldBoundingBox;
                bounds[1] = mesh[n + 1].m_WorldBoundingBox;
                bounds[2] = mesh[n + 2].m_WorldBoundingBox;
                bounds[3] = mesh[n + 3].m_WorldBoundingBox;

                cullResult[0] = 0;
                cullResult[1] = 0;
                cullResult[2] = 0;
                cullResult[3] = 0;

                cascadeFrustum.CullBox_SSE(bounds, 4, cullResult);

                shadowCast[n].m_CascadeMask |= (cullResult[0] == 0) << cascadeIndex;
                shadowCast[n + 1].m_CascadeMask |= (cullResult[1] == 0) << cascadeIndex;
                shadowCast[n + 2].m_CascadeMask |= (cullResult[2] == 0) << cascadeIndex;
                shadowCast[n + 3].m_CascadeMask |= (cullResult[3] == 0) << cascadeIndex;
            }

            if (residual)
            {
                for (int i = 0; i < residual; i++)
                    bounds[i] = mesh[n + i].m_WorldBoundingBox;

                cullResult[0] = 0;
                cullResult[1] = 0;
                cullResult[2] = 0;
                cullResult[3] = 0;

                cascadeFrustum.CullBox_SSE(bounds, 4, cullResult);

                for (int i = 0; i < residual; i++)
                    shadowCast[n + i].m_CascadeMask |= (cullResult[i] == 0) << cascadeIndex;
            }
        }

        for (int i = 0; i < q.Count(); i++)
        {
            if (shadowCast[i].m_CascadeMask == 0)
                continue;

            AddMeshShadow(transform[i], mesh[i], mesh[i].Pose, shadowCast[i], shadowmap);

            shadowCast[i].m_CascadeMask = 0;
        }
    }
#endif
    AddMeshesShadow<StaticMeshComponent>(shadowmap);
    AddMeshesShadow<DynamicMeshComponent>(shadowmap);
}

template <typename MeshComponentType>
constexpr bool IsDynamicMesh();

template <>
constexpr bool IsDynamicMesh<StaticMeshComponent>()
{
    return false;
}

template <>
constexpr bool IsDynamicMesh<DynamicMeshComponent>()
{
    return true;
}

template <typename MeshComponentType>
void RenderFrontend::AddMeshes()
{
    PreRenderContext context;
    context.FrameNum = m_RenderDef.FrameNumber;
    context.Prev = m_World->GetTick().PrevStateIndex;
    context.Cur = m_World->GetTick().StateIndex;
    context.Frac = m_World->GetTick().Interpolate;

    auto& meshManager = m_World->GetComponentManager<MeshComponentType>();
    for (auto it = meshManager.GetComponents(); it.IsValid(); ++it)
    {
        MeshComponentType& mesh = *it;

        if (!mesh.IsInitialized())
            continue;

        mesh.PreRender(context);

        Float4x4 instanceMatrix = m_View->ViewProjection * mesh.GetRenderTransform();
        Float4x4 instanceMatrixP = m_View->ViewProjectionP * mesh.GetRenderTransformPrev();

        Float3x3 modelNormalToViewSpace = m_View->NormalToViewMatrix * mesh.GetRotationMatrix();

        if (auto* meshResource = GameApplication::GetResourceManager().TryGet(mesh.GetMesh()))
        {
            int surfaceCount = meshResource->GetSurfaceCount();
            for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
            {
                Material* materialInstance = mesh.GetMaterial(surfaceIndex);
                if (!materialInstance)
                    continue;

                MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->GetResource());
                if (!material)
                    continue;

                MaterialFrameData* materialInstanceFrameData = materialInstance->PreRender(m_FrameNumber);
                if (!materialInstanceFrameData)
                    continue;
            
                // Add render instance
                RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

                if (material->IsTranslucent())
                {
                    m_FrameData.TranslucentInstances.Add(instance);
                    m_View->TranslucentInstanceCount++;
                }
                else
                {
                    m_FrameData.Instances.Add(instance);
                    m_View->InstanceCount++;
                }

                if (mesh.HasOutline())
                {
                    m_FrameData.OutlineInstances.Add(instance);
                    m_View->OutlineInstanceCount++;
                }

                instance->Material = materialInstanceFrameData->Material;
                instance->MaterialInstance = materialInstanceFrameData;

                auto& surface = meshResource->GetSurfaces()[surfaceIndex];
                meshResource->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
                meshResource->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
                meshResource->GetSkinBufferBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

                //if (bHasLightmap)
                //{
                //    mesh->GetLightmapUVsGPU(&instance->LightmapUVChannel, &instance->LightmapUVOffset);
                //    instance->LightmapOffset = InComponent->LightmapOffset;
                //    instance->Lightmap = lighting->Lightmaps[InComponent->LightmapBlock];
                //}
                //else
                {
                    instance->LightmapUVChannel = nullptr;
                    instance->Lightmap = nullptr;
                }

                //if (InComponent->bHasVertexLight && !pose)
                //{
                //    VertexLight* vertexLight = level->GetVertexLight(InComponent->VertexLightChannel);
                //    if (vertexLight && vertexLight->GetVertexCount() == mesh->GetVertexCount())
                //    {
                //        vertexLight->GetVertexBufferGPU(&instance->VertexLightChannel, &instance->VertexLightOffset);
                //    }
                //}
                //else
                {
                    instance->VertexLightChannel = nullptr;
                }

                instance->Matrix = instanceMatrix;
                instance->MatrixP = instanceMatrixP;
                instance->ModelNormalToViewSpace = modelNormalToViewSpace;

                size_t skeletonOffset = 0;
                size_t skeletonOffsetMB = 0;
                size_t skeletonSize = 0;

                if constexpr (IsDynamicMesh<MeshComponentType>())
                {
                    if (SkeletonPose* pose = mesh.GetPose())
                    {
                        if (surface.SkinIndex != -1)
                        {
                            auto& buffer = pose->m_StreamBuffers[surface.SkinIndex];
                            skeletonOffset = buffer.Offset;
                            skeletonOffsetMB = buffer.OffsetP;
                            skeletonSize = buffer.Size;
                        }
                        else
                        {
                            alignas(16) Float4x4 transform;
                            Simd::StoreFloat4x4((pose->m_ModelMatrices[surface.JointIndex] * surface.InverseTransform).cols, transform);

                            Float3x4 transform3x4(transform.Transposed());
                        
                            instance->Matrix = instance->Matrix * transform3x4;

                            // TODO: calc previous transform for animated meshes
                            instance->MatrixP = instance->MatrixP * transform3x4;

                            instance->ModelNormalToViewSpace = instance->ModelNormalToViewSpace * transform3x4.DecomposeRotation();
                        }
                    }
                }

                instance->IndexCount = surface.IndexCount;
                instance->StartIndexLocation = surface.FirstIndex;
                instance->BaseVertexLocation = surface.BaseVertex; // + mesh.SurfaceBaseVertexOffset;
                instance->SkeletonOffset = skeletonOffset;
                instance->SkeletonOffsetMB = skeletonOffsetMB;
                instance->SkeletonSize = skeletonSize;

                instance->bPerObjectMotionBlur = IsDynamicMesh<MeshComponentType>();

                uint8_t priority = material->GetRenderingPriority();
                if constexpr (IsDynamicMesh<MeshComponentType>())
                {
                    priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
                }

                instance->GenerateSortKey(priority, (uint64_t)meshResource);

                m_RenderDef.PolyCount += instance->IndexCount / 3;
            }
        }

        ProceduralMesh* proceduralMesh = mesh.GetProceduralMesh();
        if (proceduralMesh && !proceduralMesh->IndexCache.IsEmpty())
        {
            Material* materialInstance = mesh.GetMaterial(0);
            if (!materialInstance)
                continue;

            MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->GetResource());
            if (!material)
                continue;

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRender(m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            // Add render instance
            RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

            if (material->IsTranslucent())
            {
                m_FrameData.TranslucentInstances.Add(instance);
                m_View->TranslucentInstanceCount++;
            }
            else
            {
                m_FrameData.Instances.Add(instance);
                m_View->InstanceCount++;
            }

            if (mesh.HasOutline())
            {
                m_FrameData.OutlineInstances.Add(instance);
                m_View->OutlineInstanceCount++;
            }

            instance->Material = materialInstanceFrameData->Material;
            instance->MaterialInstance = materialInstanceFrameData;

            proceduralMesh->PrepareStreams(&m_RenderDef);
            proceduralMesh->GetVertexBufferGPU(m_RenderDef.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
            proceduralMesh->GetIndexBufferGPU(m_RenderDef.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

            instance->WeightsBuffer = nullptr;
            instance->WeightsBufferOffset = 0;
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
            instance->VertexLightChannel = nullptr;
            instance->IndexCount = proceduralMesh->IndexCache.Size();
            instance->StartIndexLocation = 0;
            instance->BaseVertexLocation = 0;
            instance->SkeletonOffset = 0;
            instance->SkeletonOffsetMB = 0;
            instance->SkeletonSize = 0;
            instance->Matrix = instanceMatrix;
            instance->MatrixP = instanceMatrixP;
            instance->ModelNormalToViewSpace = modelNormalToViewSpace;

            instance->bPerObjectMotionBlur = IsDynamicMesh<MeshComponentType>();

            uint8_t priority = material->GetRenderingPriority();
            if constexpr (IsDynamicMesh<MeshComponentType>())
            {
                priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
            }

            instance->GenerateSortKey(priority, (uint64_t)proceduralMesh);

            m_RenderDef.PolyCount += instance->IndexCount / 3;
        }
    }
}

template <typename MeshComponentType>
void RenderFrontend::AddMeshesShadow(LightShadowmap* shadowMap)
{
    PreRenderContext context;
    context.FrameNum = m_RenderDef.FrameNumber;
    context.Prev = m_World->GetTick().PrevStateIndex;
    context.Cur = m_World->GetTick().StateIndex;
    context.Frac = m_World->GetTick().Interpolate;

    auto& meshManager = m_World->GetComponentManager<MeshComponentType>();
    for (auto it = meshManager.GetComponents(); it.IsValid(); ++it)
    {
        MeshComponentType& mesh = *it;

        if (!mesh.IsInitialized())
            continue;

        mesh.PreRender(context);

        Float3x4 const& instanceMatrix = mesh.GetRenderTransform();

        auto* meshResource = GameApplication::GetResourceManager().TryGet(mesh.GetMesh());
        if (meshResource)
        {
            int surfaceCount = meshResource->GetSurfaceCount();
            for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
            {
                Material* materialInstance = mesh.GetMaterial(surfaceIndex);
                if (!materialInstance)
                    continue;

                MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->GetResource());
                if (!material)
                    continue;

                if (!material->IsCastShadow())
                    continue;

                MaterialFrameData* materialInstanceFrameData = materialInstance->PreRender(m_FrameNumber);
                if (!materialInstanceFrameData)
                    continue;
            
                // Add render instance
                ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

                m_FrameData.ShadowInstances.Add(instance);

                instance->Material = materialInstanceFrameData->Material;
                instance->MaterialInstance = materialInstanceFrameData;

                meshResource->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
                meshResource->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
                meshResource->GetSkinBufferBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

                auto& surface = meshResource->GetSurfaces()[surfaceIndex];

                instance->WorldTransformMatrix = instanceMatrix;

                size_t skeletonOffset = 0;
                size_t skeletonSize = 0;

                if constexpr (IsDynamicMesh<MeshComponentType>())
                {
                    if (SkeletonPose* pose = mesh.GetPose())
                    {
                        if (surface.SkinIndex != -1)
                        {
                            auto& buffer = pose->m_StreamBuffers[surface.SkinIndex];
                            skeletonOffset = buffer.Offset;
                            skeletonSize = buffer.Size;
                        }
                        else
                        {
                            alignas(16) Float4x4 transform;
                            Simd::StoreFloat4x4((pose->m_ModelMatrices[surface.JointIndex] * surface.InverseTransform).cols, transform);
                            instance->WorldTransformMatrix = instance->WorldTransformMatrix * Float3x4(transform.Transposed());
                        }
                    }
                }

                instance->IndexCount = surface.IndexCount;
                instance->StartIndexLocation = surface.FirstIndex;
                instance->BaseVertexLocation = surface.BaseVertex; // + mesh.SurfaceBaseVertexOffset;
                instance->SkeletonOffset = skeletonOffset;
                instance->SkeletonSize = skeletonSize;
                instance->CascadeMask = 0xffff;//mesh.m_CascadeMask; // TODO

                uint8_t priority = material->GetRenderingPriority();

                instance->GenerateSortKey(priority, (uint64_t)meshResource);

                shadowMap->ShadowInstanceCount++;

                m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
            }
        }

        ProceduralMesh* proceduralMesh = mesh.GetProceduralMesh();
        if (proceduralMesh && !proceduralMesh->IndexCache.IsEmpty())
        {        
            Material* materialInstance = mesh.GetMaterial(0);
            if (!materialInstance)
                continue;

            MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->GetResource());
            if (!material)
                continue;

            if (!material->IsCastShadow())
                continue;

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRender(m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            // Add render instance
            ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

            m_FrameData.ShadowInstances.Add(instance);

            instance->Material = materialInstanceFrameData->Material;
            instance->MaterialInstance = materialInstanceFrameData;

            proceduralMesh->PrepareStreams(&m_RenderDef);
            proceduralMesh->GetVertexBufferGPU(m_RenderDef.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
            proceduralMesh->GetIndexBufferGPU(m_RenderDef.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

            instance->WeightsBuffer = nullptr;
            instance->WeightsBufferOffset = 0;
            instance->IndexCount = proceduralMesh->IndexCache.Size();
            instance->StartIndexLocation = 0;
            instance->BaseVertexLocation = 0;
            instance->SkeletonOffset = 0;
            instance->SkeletonSize = 0;
            instance->WorldTransformMatrix = instanceMatrix;
            instance->CascadeMask = 0xffff; //mesh.m_CascadeMask; // TODO

            uint8_t priority = material->GetRenderingPriority();

            instance->GenerateSortKey(priority, (uint64_t)proceduralMesh);

            shadowMap->ShadowInstanceCount++;

            m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }
    }
}

bool RenderFrontend::AddLightShadowmap(PunctualLightComponent* light, float radius)
{
    if (!light->IsCastShadow())
        return false;

    //Float4x4 const* cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();

    //Float4x4::PerspectiveMatrixDesc desc = {};
    //desc.AspectRatio = 1;
    //desc.FieldOfView = 90;
    //desc.ZNear = 0.1f;
    //desc.ZFar = 1000 /*radius*/;
    //Float4x4 projMat = Float4x4::GetPerspectiveMatrix(desc);

    //Float4x4 lightViewProjection;
    //Float4x4 lightViewMatrix;

    Float3 lightPos = light->m_RenderTransform.Position;

    int totalInstances = 0;

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        //Float3x3 basis = Float3x3(cubeFaceMatrices[faceIndex]);
        //Float3 origin = basis * (-lightPos);

        //lightViewMatrix[0] = Float4(basis[0], 0.0f);
        //lightViewMatrix[1] = Float4(basis[1], 0.0f);
        //lightViewMatrix[2] = Float4(basis[2], 0.0f);
        //lightViewMatrix[3] = Float4(origin, 1.0f);

        //lightViewProjection = projMat * lightViewMatrix;

        LightShadowmap* shadowMap = &m_FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount = 0;
        shadowMap->LightPosition = lightPos;

        // TODO: Add only visible objects
        AddMeshesShadow<StaticMeshComponent>(shadowMap);
        AddMeshesShadow<DynamicMeshComponent>(shadowMap);

        SortShadowInstances(shadowMap);

        totalInstances += shadowMap->ShadowInstanceCount;
    }

    if (totalInstances == 0)
    {
        m_FrameData.LightShadowmaps.Resize(m_FrameData.LightShadowmaps.Size() - 6);
        return false;
    }

    return true;
}

void RenderFrontend::RenderView(WorldRenderView* worldRenderView, RenderViewData* view)
{
    auto* world = worldRenderView->GetWorld();
    if (!world)
    {
        ClearRenderView(view);
        return;
    }

    m_World = world;
    m_View = view;

    auto& cameraManager = world->GetComponentManager<CameraComponent>();
    auto* camera = cameraManager.GetComponent(worldRenderView->GetCamera());

    if (!r_RenderView || !camera || !camera->IsInitialized())
    {
        ClearRenderView(view);
        return;
    }

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    uint32_t width = worldRenderView->GetWidth();
    uint32_t height = worldRenderView->GetHeight();

    view->FrameNumber = worldRenderView->m_FrameNum;    

    view->WidthP  = worldRenderView->m_ScaledWidth;
    view->HeightP = worldRenderView->m_ScaledHeight;
    view->Width   = worldRenderView->m_ScaledWidth = width * r_ResolutionScaleX.GetFloat();
    view->Height  = worldRenderView->m_ScaledHeight = height * r_ResolutionScaleY.GetFloat();
    view->WidthR  = width;
    view->HeightR = height;

    auto& tick = world->GetTick();

    // FIXME: float overflow
    view->GameRunningTimeSeconds = tick.RunningTime;
    view->GameplayTimeSeconds = tick.FrameTime;
    view->GameplayTimeStep = tick.IsPaused ? 0.0f : Math::Max(tick.FrameTimeStep, 0.0001f);

    Float3 cameraPosition = Math::Lerp (camera->GetPosition(tick.PrevStateIndex), camera->GetPosition(tick.StateIndex), tick.Interpolate);
    Quat   cameraRotation = Math::Slerp(camera->GetRotation(tick.PrevStateIndex), camera->GetRotation(tick.StateIndex), tick.Interpolate);

    //cameraPosition = cameraPosition + camera->OffsetPosition;
    //cameraRotation = cameraRotation * camera->OffsetRotation;

    Float3x3 billboardMatrix = cameraRotation.ToMatrix3x3();

    Float4x4 viewMatrix;
    {
        Float3x3 basis = billboardMatrix.Transposed();
        Float3 origin = basis * (-cameraPosition);

        viewMatrix[0] = Float4(basis[0], 0.0f);
        viewMatrix[1] = Float4(basis[1], 0.0f);
        viewMatrix[2] = Float4(basis[2], 0.0f);
        viewMatrix[3] = Float4(origin, 1.0f); 
    }

    float fovx, fovy;
    camera->GetEffectiveFov(fovx, fovy);

    view->ViewPosition       = cameraPosition;
    view->ViewRotation       = cameraRotation;
    view->ViewRightVec       = cameraRotation.XAxis();
    view->ViewUpVec          = cameraRotation.YAxis();
    view->ViewDir            = -cameraRotation.ZAxis();
    view->ViewMatrix         = viewMatrix;
    view->ProjectionMatrix   = camera->GetProjectionMatrix();
    view->ViewMatrixP        = worldRenderView->m_ViewMatrix;
    view->ProjectionMatrixP  = worldRenderView->m_ProjectionMatrix;
    view->ViewZNear          = camera->GetZNear();
    view->ViewZFar           = camera->GetZFar();
    view->ViewOrthoMins      = camera->GetOrthoMins();
    view->ViewOrthoMaxs      = camera->GetOrthoMaxs();
    view->ViewFovX           = fovx;
    view->ViewFovY           = fovy;
    view->bPerspective       = camera->IsPerspective();
    view->MaxVisibleDistance = camera->GetZFar(); // TODO: calculate the farthest point (use mesh bounding boxes)
    view->NormalToViewMatrix = Float3x3(view->ViewMatrix);

    view->InverseProjectionMatrix = camera->IsPerspective() ?
        view->ProjectionMatrix.PerspectiveProjectionInverseFast() :
        view->ProjectionMatrix.OrthoProjectionInverseFast();
    view->ClusterProjectionMatrix = camera->GetClusterProjectionMatrix();

    view->ClusterViewProjection = view->ClusterProjectionMatrix * view->ViewMatrix;
    view->ClusterViewProjectionInversed = view->ViewMatrix.ViewInverseFast() * view->ClusterProjectionMatrix.PerspectiveProjectionInverseFast();//view->ClusterViewProjection.Inversed();

    worldRenderView->m_ViewMatrix = view->ViewMatrix;
    worldRenderView->m_ProjectionMatrix = view->ProjectionMatrix;

    view->ViewProjection        = view->ProjectionMatrix * view->ViewMatrix;
    view->ViewProjectionP       = view->ProjectionMatrixP * view->ViewMatrixP;
    view->ViewSpaceToWorldSpace = view->ViewMatrix.ViewInverseFast();
    view->ClipSpaceToWorldSpace = view->ViewSpaceToWorldSpace * view->InverseProjectionMatrix;
    view->BackgroundColor       = Float3(worldRenderView->BackgroundColor.R, worldRenderView->BackgroundColor.G, worldRenderView->BackgroundColor.B);
    view->bClearBackground      = worldRenderView->bClearBackground;
    view->bWireframe            = worldRenderView->bWireframe;

    if (worldRenderView->Vignette)
    {
        view->VignetteColorIntensity = worldRenderView->Vignette->ColorIntensity;
        view->VignetteOuterRadiusSqr = worldRenderView->Vignette->OuterRadiusSqr;
        view->VignetteInnerRadiusSqr = worldRenderView->Vignette->InnerRadiusSqr;
    }
    else
    {
        view->VignetteColorIntensity.W = 0;
    }

    view->Exposure = camera->GetExposure();

    if (worldRenderView->ColorGrading)
    {
        ColorGradingParameters* params = worldRenderView->ColorGrading;

        TextureHandle lut = params->GetLUT();
        TextureResource* lutTexture = m_ResourceManager->TryGet(lut);

        view->ColorGradingLUT = lutTexture ? lutTexture->GetTextureGPU() : nullptr;
        view->CurrentColorGradingLUT = worldRenderView->GetCurrentColorGradingLUT();
        view->ColorGradingAdaptationSpeed = params->GetAdaptationSpeed();

        // Procedural color grading
        view->ColorGradingGrain = params->GetGrain();
        view->ColorGradingGamma = params->GetGamma();
        view->ColorGradingLift = params->GetLift();
        view->ColorGradingPresaturation = params->GetPresaturation();
        view->ColorGradingTemperatureScale = params->GetTemperatureScale();
        view->ColorGradingTemperatureStrength = params->GetTemperatureStrength();
        view->ColorGradingBrightnessNormalization = params->GetBrightnessNormalization();
    }
    else
    {
        view->ColorGradingLUT = nullptr;
        view->CurrentColorGradingLUT = nullptr;
        view->ColorGradingAdaptationSpeed = 0;
    }

    view->CurrentExposure = worldRenderView->GetCurrentExposure();

    // TODO: Do not initialize light&depth textures if screen space reflections disabled
    view->LightTexture = worldRenderView->AcquireLightTexture();
    view->DepthTexture = worldRenderView->AcquireDepthTexture();
    view->RenderTarget = worldRenderView->AcquireRenderTarget();

    if (r_HBAO && r_HBAODeinterleaved)
    {
        view->HBAOMaps = worldRenderView->AcquireHBAOMaps();
    }
    else
    {
        worldRenderView->ReleaseHBAOMaps();
        view->HBAOMaps = nullptr;
    }

    view->bAllowHBAO = worldRenderView->bAllowHBAO;
    view->bAllowMotionBlur = worldRenderView->bAllowMotionBlur && r_MotionBlur;
    view->AntialiasingType = worldRenderView->AntialiasingType;

    view->VTFeedback = &worldRenderView->m_VTFeedback;

    view->PhotometricProfiles = m_PhotometricProfiles;

    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;
    view->FirstInstance = m_FrameData.Instances.Size();
    view->InstanceCount = 0;
    view->FirstTranslucentInstance = m_FrameData.TranslucentInstances.Size();
    view->TranslucentInstanceCount = 0;
    view->FirstOutlineInstance = m_FrameData.OutlineInstances.Size();
    view->OutlineInstanceCount = 0;
    //view->FirstLightPortal = m_FrameData.LightPortals.Size();
    //view->LightPortalsCount = 0;
    //view->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
    //view->ShadowInstanceCount = 0;
    view->FirstDirectionalLight = m_FrameData.DirectionalLights.Size();
    view->NumDirectionalLights = 0;
    view->FirstDebugDrawCommand = 0;
    view->DebugDrawCommandCount = 0;

    size_t size = MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof(Float4x4);

    view->ShadowMapMatricesStreamHandle = streamedMemory->AllocateConstant(size, nullptr);
    view->ShadowMapMatrices = (Float4x4*)streamedMemory->Map(view->ShadowMapMatricesStreamHandle);

    size_t numFrustumClusters = MAX_FRUSTUM_CLUSTERS_X * MAX_FRUSTUM_CLUSTERS_Y * MAX_FRUSTUM_CLUSTERS_Z;

    view->ClusterLookupStreamHandle = streamedMemory->AllocateConstant(numFrustumClusters * sizeof(ClusterHeader), nullptr);
    view->ClusterLookup = (ClusterHeader*)streamedMemory->Map(view->ClusterLookupStreamHandle);

    view->FirstTerrainInstance = m_FrameData.TerrainInstances.Size();
    view->TerrainInstanceCount = 0;

    BvFrustum frustum;
    frustum.FromMatrix(view->ViewProjection, true);

    m_RenderDef.WorldRV = worldRenderView;
    m_RenderDef.FrameNumber = m_FrameNumber;
    m_RenderDef.View = view;
    m_RenderDef.Frustum = &frustum;
    m_RenderDef.VisibilityMask = worldRenderView->VisibilityMask;
    m_RenderDef.PolyCount = 0;
    m_RenderDef.ShadowMapPolyCount = 0;
    m_RenderDef.StreamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    // Update local frame number
    worldRenderView->m_FrameNum++;

    //QueryVisiblePrimitives(world);

    //EnvironmentMap* pEnvironmentMap = world->GetInterface<RenderInterface>().GetEnvironmentMap(); TODO

    view->WorldAmbient = world->GetInterface<RenderInterface>().GetAmbient();

    //if (pEnvironmentMap)
    //{
    //    view->GlobalIrradianceMap = pEnvironmentMap->GetIrradianceHandle();
    //    view->GlobalReflectionMap = pEnvironmentMap->GetReflectionHandle();
    //}
    //else
    {
#if 0// todo
        if (!m_DummyEnvironmentMap)
        {
            m_DummyEnvironmentMap = Resource::CreateDefault<EnvironmentMap>();
        }
        view->GlobalIrradianceMap = m_DummyEnvironmentMap->GetIrradianceHandle();
        view->GlobalReflectionMap = m_DummyEnvironmentMap->GetReflectionHandle();
#else
        view->GlobalIrradianceMap = 0;
        view->GlobalReflectionMap = 0;
#endif
    }

    // Generate debug draw commands
    if (worldRenderView->bDrawDebug)
    {
        m_DebugDraw.BeginRenderView(view, m_VisPass);

        world->DrawDebug(m_DebugDraw);

        if (com_DrawFrustumClusters)
        {
            m_LightVoxelizer.DrawVoxels(&m_DebugDraw);
        }
    }

    if (r_RenderMeshes)
    {
        AddMeshes<StaticMeshComponent>();
        AddMeshes<DynamicMeshComponent>();
    }

    if (r_RenderTerrain)
    {
        auto& terrainManager = world->GetComponentManager<TerrainComponent>();
        for (auto it = terrainManager.GetComponents(); it.IsValid(); ++it)
        {
            TerrainComponent& terrain = *it;

            auto* terrainResource = GameApplication::GetResourceManager().TryGet(terrain.m_Resource);
            if (!terrainResource)
                continue;

            auto* gameObject = terrain.GetOwner();

            Float3 worldPosition = gameObject->GetWorldPosition();

            // Terrain world rotation
            Float3x3 worldRotation = gameObject->GetWorldRotation().ToMatrix3x3();
            Float3x3 worldRotationInv = worldRotation.Transposed();

            // Terrain transform without scale
            //Float3x4 transformMatrix;
            //transformMatrix.Compose(transform.Position, worldRotation);

            // Terrain inversed transform
            //Float3x4 terrainWorldTransformInv = transformMatrix.Inversed();

            // Camera position in terrain space
            //Float3 localViewPosition = terrainWorldTransformInv * view->ViewPosition;
            Float3 localViewPosition = worldRotationInv * (view->ViewPosition - worldPosition);

            // Camera rotation in terrain space
            Float3x3 localRotation = worldRotationInv * view->ViewRotation.ToMatrix3x3();

            Float3x3 basis = localRotation.Transposed();
            Float3 origin = basis * (-localViewPosition);

            Float4x4 localViewMatrix;
            localViewMatrix[0] = Float4(basis[0], 0.0f);
            localViewMatrix[1] = Float4(basis[1], 0.0f);
            localViewMatrix[2] = Float4(basis[2], 0.0f);
            localViewMatrix[3] = Float4(origin, 1.0f);

            Float4x4 localMVP = view->ProjectionMatrix * localViewMatrix;

            BvFrustum localFrustum;
            localFrustum.FromMatrix(localMVP, true);

            // Update view
            auto terrainView = worldRenderView->GetTerrainView(terrain.m_Resource);

            terrainView->Update(localViewPosition, localFrustum);
            if (terrainView->GetIndirectBufferDrawCount() == 0)
            {
                // Everything was culled
                return;
            }

            // TODO: transform history
            //Float3x4 const& componentWorldTransform = transformMatrix;
            //Float3x4 const& componentWorldTransformP = transformHistory ? *transformHistory : transformMatrix;
            //Float4x4 instanceMatrix = view->ViewProjection * componentWorldTransform;
            //Float4x4 instanceMatrixP = view->ViewProjectionP * componentWorldTransformP;

            auto& frameLoop = GameApplication::GetFrameLoop();

            TerrainRenderInstance* instance = (TerrainRenderInstance*)frameLoop.AllocFrameMem(sizeof(TerrainRenderInstance));

            m_FrameData.TerrainInstances.Add(instance);

            instance->VertexBuffer = terrainView->GetVertexBufferGPU();
            instance->IndexBuffer = terrainView->GetIndexBufferGPU();
            instance->InstanceBufferStreamHandle = terrainView->GetInstanceBufferStreamHandle();
            instance->IndirectBufferStreamHandle = terrainView->GetIndirectBufferStreamHandle();
            instance->IndirectBufferDrawCount = terrainView->GetIndirectBufferDrawCount();
            instance->Clipmaps = terrainView->GetClipmapArray();
            instance->Normals = terrainView->GetNormalMapArray();
            instance->ViewPositionAndHeight.X = localViewPosition.X;
            instance->ViewPositionAndHeight.Y = localViewPosition.Y;
            instance->ViewPositionAndHeight.Z = localViewPosition.Z;
            instance->ViewPositionAndHeight.W = terrainView->GetViewHeight();
            instance->LocalViewProjection = localMVP;
            instance->ModelNormalToViewSpace = view->NormalToViewMatrix * worldRotation;
            instance->ClipMin = terrainResource->GetClipMin();
            instance->ClipMax = terrainResource->GetClipMax();

            view->TerrainInstanceCount++;
        }
    }

    // Add directional lights
    view->NumShadowMapCascades = 0;
    view->NumCascadedShadowMaps = 0;
    auto& dirLightManager = world->GetComponentManager<DirectionalLightComponent>();
    for (auto it = dirLightManager.GetComponents(); it.IsValid(); ++it)
    {
        DirectionalLightComponent& light = *it;

        if (view->NumDirectionalLights < MAX_DIRECTIONAL_LIGHTS)
        {
            DirectionalLightInstance* instance = (DirectionalLightInstance*)m_FrameLoop->AllocFrameMem(sizeof(DirectionalLightInstance));

            m_FrameData.DirectionalLights.Add(instance);

            Quat rotation = light.GetOwner()->GetWorldRotation(); // TODO: Interpolate?

            Float3x3 rotationMat = FixupLightRotation(rotation);

            if (light.m_CastShadow)
            {
                AddShadowmapCascades(light, rotationMat, m_FrameLoop->GetStreamedMemoryGPU(), view, &instance->ViewProjStreamHandle, &instance->FirstCascade, &instance->NumCascades);

                view->NumCascadedShadowMaps += instance->NumCascades > 0 ? 1 : 0; // Just statistics
            }
            else
            {
                instance->FirstCascade = 0;
                instance->NumCascades = 0;
            }

            light.UpdateEffectiveColor();

            instance->ColorAndAmbientIntensity = light.m_EffectiveColor;
            instance->Matrix = rotationMat;
            instance->MaxShadowCascades = light.GetMaxShadowCascades();
            instance->RenderMask = ~0; //light.RenderingGroup;
            instance->ShadowmapIndex = -1;
            instance->ShadowCascadeResolution = light.GetShadowCascadeResolution();

            view->NumDirectionalLights++;
        }
        else
        {
            LOG("MAX_DIRECTIONAL_LIGHTS hit\n");
            break;
        }
    }
    for (int lightIndex = 0; lightIndex < view->NumDirectionalLights; lightIndex++)
    {
        DirectionalLightInstance* lightDef = m_FrameData.DirectionalLights[view->FirstDirectionalLight + lightIndex];
        if (lightDef->NumCascades == 0)
            continue;

        lightDef->ShadowmapIndex = m_FrameData.LightShadowmaps.Size();

        LightShadowmap& shadowMap = m_FrameData.LightShadowmaps.Add();

        shadowMap.FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap.ShadowInstanceCount = 0;
        shadowMap.FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap.LightPortalsCount = 0;

        AddDirectionalLightShadows(&shadowMap, lightDef);
        SortShadowInstances(&shadowMap);
    }

    m_LightVoxelizer.Reset();

    auto& lightManager = m_World->GetComponentManager<PunctualLightComponent>();

    PreRenderContext context;
    context.FrameNum = m_RenderDef.FrameNumber;
    context.Prev = m_World->GetTick().PrevStateIndex;
    context.Cur = m_World->GetTick().StateIndex;
    context.Frac = m_World->GetTick().Interpolate;

    // Allocate lights
    view->NumPointLights = lightManager.GetComponentCount();//m_VisLights.Size();  // TODO: only visible light count!
    view->PointLightsStreamSize = sizeof(LightParameters) * view->NumPointLights;
    view->PointLightsStreamHandle = view->PointLightsStreamSize > 0 ? streamedMemory->AllocateConstant(view->PointLightsStreamSize, nullptr) : 0;
    view->PointLights = (LightParameters*)streamedMemory->Map(view->PointLightsStreamHandle);
    view->FirstOmnidirectionalShadowMap = m_FrameData.LightShadowmaps.Size();
    view->NumOmnidirectionalShadowMaps = 0;

    int maxOmnidirectionalShadowMaps = GameApplication::GetRenderBackend().MaxOmnidirectionalShadowMapsPerView();

    uint32_t index = 0;
    for (auto it = lightManager.GetComponents(); it.IsValid(); ++it)
    {
        PunctualLightComponent& light = *it;

        if (index >= MAX_LIGHTS)
        {
            LOG("MAX_LIGHTS hit\n");
            break;
        }

        if (!light.IsInitialized())
            continue;

        light.PreRender(context);
        light.PackLight(view->ViewMatrix, view->PointLights[index]);

        view->PointLights[index].ShadowmapIndex = -1;

        if (view->NumOmnidirectionalShadowMaps < maxOmnidirectionalShadowMaps)
        {
            if (AddLightShadowmap(&light, view->PointLights[index].Radius))
            {
                view->PointLights[index].ShadowmapIndex = view->NumOmnidirectionalShadowMaps;
                view->NumOmnidirectionalShadowMaps++;
            }
            else
                view->PointLights[index].ShadowmapIndex = -1;
        }
        else
        {
            LOG("maxOmnidirectionalShadowMaps hit\n");
        }

        //PhotometricProfile* profile = light->GetPhotometricProfile();
        //if (profile)
        //  profile->WritePhotometricData(m_PhotometricProfiles, m_FrameNumber);

        ItemInfo* info = m_LightVoxelizer.AllocItem();
        info->Type = ITEM_TYPE_LIGHT;
        info->ListIndex = index;

        BvAxisAlignedBox const& AABB = light.m_AABBWorldBounds;
        info->Mins = AABB.Mins;
        info->Maxs = AABB.Maxs;

        if (m_LightVoxelizer.IsSSE())
            info->ClipToBoxMatSSE = light.m_OBBTransformInverse * view->ClusterViewProjectionInversed;
        else
            info->ClipToBoxMat = light.m_OBBTransformInverse * view->ClusterViewProjectionInversed;

        index++;
    }
    view->NumPointLights = index;

    // Allocate probes
    view->NumProbes = 0;//m_VisEnvProbes.Size();
    view->ProbeStreamSize = sizeof(ProbeParameters) * view->NumProbes;
    view->ProbeStreamHandle = view->ProbeStreamSize > 0 ?
        streamedMemory->AllocateConstant(view->ProbeStreamSize, nullptr) :
        0;
    view->Probes = (ProbeParameters*)streamedMemory->Map(view->ProbeStreamHandle);

    // TODO
    //for (int i = 0; i < view->NumProbes; i++)
    //{
    //    envProbe = m_VisEnvProbes[i];

    //    envProbe->PackProbe(view->ViewMatrix, view->Probes[i]);

    //    ItemInfo* info = m_LightVoxelizer.AllocItem();
    //    info->Type = ITEM_TYPE_PROBE;
    //    info->ListIndex = i;

    //    BvAxisAlignedBox const& AABB = envProbe->GetWorldBounds();
    //    info->Mins = AABB.Mins;
    //    info->Maxs = AABB.Maxs;

    //    if (m_LightVoxelizer.IsSSE())
    //    {
    //        info->ClipToBoxMatSSE = envProbe->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
    //    }
    //    else
    //    {
    //        info->ClipToBoxMat = envProbe->GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
    //    }
    //}

    m_LightVoxelizer.Voxelize(m_FrameLoop->GetStreamedMemoryGPU(), view);

    m_Stat.PolyCount += m_RenderDef.PolyCount;
    m_Stat.ShadowMapPolyCount += m_RenderDef.ShadowMapPolyCount;

    if (worldRenderView->bDrawDebug)
    {
        //TODO
        //for (auto& it : worldRenderView->m_TerrainViews)
        //{
        //    it.second->DrawDebug(&m_DebugDraw);
        //}

        m_DebugDraw.EndRenderView();
    }
}

void RenderFrontend::SortRenderInstances()
{
    struct InstanceSortFunction
    {
        bool operator()(RenderInstance const* a, RenderInstance const* b) const
        {
            return a->SortKey < b->SortKey;
        }
    } instanceSortFunction;

    for (RenderViewData* view = m_FrameData.RenderViews; view < &m_FrameData.RenderViews[m_FrameData.NumViews]; view++)
    {
        std::sort(m_FrameData.Instances.Begin() + view->FirstInstance,
                  m_FrameData.Instances.Begin() + (view->FirstInstance + view->InstanceCount),
                  instanceSortFunction);

        std::sort(m_FrameData.TranslucentInstances.Begin() + view->FirstTranslucentInstance,
                  m_FrameData.TranslucentInstances.Begin() + (view->FirstTranslucentInstance + view->TranslucentInstanceCount),
                  instanceSortFunction);
    }
}

void RenderFrontend::SortShadowInstances(LightShadowmap const* shadowMap)
{
    struct ShadowInstanceSortFunction
    {
        bool operator()(ShadowRenderInstance const* a, ShadowRenderInstance const* b)
        {
            return a->SortKey < b->SortKey;
        }
    } shadowInstanceSortFunction;

    std::sort(m_FrameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
              m_FrameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
              shadowInstanceSortFunction);
}

void RenderFrontend::QueryVisiblePrimitives(World* world)
{
    VisibilityQuery query;

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &(*m_RenderDef.Frustum)[i];
    }
    query.ViewPosition = m_RenderDef.View->ViewPosition;
    query.ViewRightVec = m_RenderDef.View->ViewRightVec;
    query.ViewUpVec = m_RenderDef.View->ViewUpVec;
    query.VisibilityMask = m_RenderDef.VisibilityMask;
    query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS; // | VSD_QUERY_MASK_SHADOW_CAST;

    //world->QueryVisiblePrimitives(m_VisPrimitives, &m_VisPass, query);
}

void RenderFrontend::QueryShadowCasters(World* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, Vector<PrimitiveDef*>& Primitives)
{
    VisibilityQuery query;
    BvFrustum frustum;

    frustum.FromMatrix(LightViewProjection, true);

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &frustum[i];
    }
    query.ViewPosition = LightPosition;
    query.ViewRightVec = LightBasis[0];
    query.ViewUpVec = LightBasis[1];
    query.VisibilityMask = m_RenderDef.VisibilityMask;
    query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_SHADOW_CAST;
#if 0
#    if 0
    Float3 clipBox[8] =
    {
        Float3(-1,-1, 0),
        Float3(-1,-1, 1),
        Float3(-1, 1, 0),
        Float3(-1, 1, 1),
        Float3( 1,-1, 0),
        Float3( 1,-1, 1),
        Float3( 1, 1, 0),
        Float3( 1, 1, 1)
    };

    Float4x4 inversed = LightViewProjection.Inversed();
    for ( int i = 0 ; i < 8 ; i++ ) {
        clipBox[i] = Float3( inversed * Float4( clipBox[i], 1.0f ) );
    }

    m_DebugDraw.SetDepthTest( false );
    m_DebugDraw.SetColor(Color4(1,1,0,1));
    m_DebugDraw.DrawLine( clipBox, 8 );
#    else
    Float3 vectorTR;
    Float3 vectorTL;
    Float3 vectorBR;
    Float3 vectorBL;
    Float3 origin = LightPosition;
    Float3 v[4];
    Float3 faces[4][3];
    float lightRadius = 4;
    float rayLength = lightRadius / Math::Cos( Math::_PI/4.0f );

    frustum.CornerVector_TR( vectorTR );
    frustum.CornerVector_TL( vectorTL );
    frustum.CornerVector_BR( vectorBR );
    frustum.CornerVector_BL( vectorBL );

    v[0] = origin + vectorTR * rayLength;
    v[1] = origin + vectorBR * rayLength;
    v[2] = origin + vectorBL * rayLength;
    v[3] = origin + vectorTL * rayLength;

    // top
    faces[0][0] = origin;
    faces[0][1] = v[0];
    faces[0][2] = v[3];

    // left
    faces[1][0] = origin;
    faces[1][1] = v[3];
    faces[1][2] = v[2];

    // bottom
    faces[2][0] = origin;
    faces[2][1] = v[2];
    faces[2][2] = v[1];

    // right
    faces[3][0] = origin;
    faces[3][1] = v[1];
    faces[3][2] = v[0];

    m_DebugDraw.SetDepthTest( true );

    m_DebugDraw.SetColor( Color4( 0, 1, 1, 1 ) );
    m_DebugDraw.DrawLine( origin, v[0] );
    m_DebugDraw.DrawLine( origin, v[3] );
    m_DebugDraw.DrawLine( origin, v[1] );
    m_DebugDraw.DrawLine( origin, v[2] );
    m_DebugDraw.DrawLine( v, 4, true );

    m_DebugDraw.SetColor( Color4( 1, 1, 1, 0.3f ) );
    m_DebugDraw.DrawTriangles( &faces[0][0], 4, sizeof( Float3 ), false );
    m_DebugDraw.DrawConvexPoly( v, 4, false );
#    endif
#endif
    //InWorld->QueryVisiblePrimitives(Primitives, nullptr, query);
}

#if 0
void RenderFrontend::AddStaticMesh(MeshComponent* InComponent)
{
    if (!r_RenderMeshes)
    {
        return;
    }

    InComponent->PreRenderUpdate(&m_RenderDef);

    Float3x4 const& componentWorldTransform = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber);
    Float3x4 const& componentWorldTransformP = InComponent->GetRenderTransformMatrix(m_RenderDef.FrameNumber + 1);

    // TODO: optimize: parallel, sse, check if transformable
    Float4x4 instanceMatrix = m_RenderDef.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = m_RenderDef.View->ViewProjectionP * componentWorldTransformP;

    Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    Level* level = InComponent->GetLevel();
    LevelLighting* lighting = level->Lighting;

    IndexedMesh* mesh = InComponent->GetMesh();
    IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    bool bHasLightmap = (lighting &&
                         InComponent->bHasLightmap &&
                         InComponent->LightmapBlock < lighting->Lightmaps.Size() &&
                         !r_VertexLight &&
                         mesh->HasLightmapUVs());

    auto& meshRenderViews = InComponent->GetRenderViews();

    for (auto& meshRender : meshRenderViews)
    {
        if (!meshRender->IsEnabled())
            continue;

        for (int subpartIndex = 0, count = subparts.Size(); subpartIndex < count; subpartIndex++)
        {
            IndexedMeshSubpart* subpart = subparts[subpartIndex];

            MaterialInstance* materialInstance = meshRender->GetMaterial(subpartIndex);
            HK_ASSERT(materialInstance);

            MaterialFrameData* materialInstanceFrameData = materialInstance->PreRenderUpdate(m_FrameLoop, m_FrameNumber);
            if (!materialInstanceFrameData)
                continue;

            Material* material = materialInstance->GetMaterial();

            // Add render instance
            RenderInstance* instance = (RenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(RenderInstance));

            if (material->IsTranslucent())
            {
                m_FrameData.TranslucentInstances.Add(instance);
                m_RenderDef.View->TranslucentInstanceCount++;
            }
            else
            {
                m_FrameData.Instances.Add(instance);
                m_RenderDef.View->InstanceCount++;
            }

            if (InComponent->bOutline)
            {
                m_FrameData.OutlineInstances.Add(instance);
                m_RenderDef.View->OutlineInstanceCount++;
            }

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;

            mesh->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
            mesh->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
            mesh->GetSkinBufferBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

            if (bHasLightmap)
            {
                mesh->GetLightmapUVsGPU(&instance->LightmapUVChannel, &instance->LightmapUVOffset);
                instance->LightmapOffset = InComponent->LightmapOffset;
                instance->Lightmap = lighting->Lightmaps[InComponent->LightmapBlock];
            }
            else
            {
                instance->LightmapUVChannel = nullptr;
                instance->Lightmap = nullptr;
            }

            if (InComponent->bHasVertexLight)
            {
                VertexLight* vertexLight = level->GetVertexLight(InComponent->VertexLightChannel);
                if (vertexLight && vertexLight->GetVertexCount() == mesh->GetVertexCount())
                {
                    vertexLight->GetVertexBufferGPU(&instance->VertexLightChannel, &instance->VertexLightOffset);
                }
            }
            else
            {
                instance->VertexLightChannel = nullptr;
            }

            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex() + InComponent->SubpartBaseVertexOffset;
            instance->SkeletonOffset = 0;
            instance->SkeletonOffsetMB = 0;
            instance->SkeletonSize = 0;
            instance->Matrix = instanceMatrix;
            instance->MatrixP = instanceMatrixP;
            instance->ModelNormalToViewSpace = m_RenderDef.View->NormalToViewMatrix * worldRotation;

            uint8_t priority = material->GetRenderingPriority();
            if (InComponent->GetMotionBehavior() != MB_STATIC)
            {
                priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
            }

            instance->GenerateSortKey(priority, (uint64_t)mesh);

            m_RenderDef.PolyCount += instance->IndexCount / 3;
        }
    }
}
#endif

#if 0
void RenderFrontend::AddDirectionalShadowmapInstances(World* InWorld)
{
    if (!m_RenderDef.View->NumShadowMapCascades)
    {
        return;
    }

    StreamedMemoryGPU* streamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

    // Create shadow instances

    m_ShadowCasters.Clear();
    m_ShadowBoxes.Clear();

    LightingSystem& lightingSystem = InWorld->LightingSystem;

    for (TListIterator<Drawable> component(lightingSystem.ShadowCasters); component; component++)
    {
        if ((component->GetVisibilityGroup() & m_RenderDef.VisibilityMask) == 0)
        {
            continue;
        }
        //component->CascadeMask = 0;

        m_ShadowCasters.Add(*component);
        m_ShadowBoxes.Add(component->GetWorldBounds());
    }

    if (m_ShadowBoxes.IsEmpty())
        return;

    m_ShadowBoxes.Resize(Align(m_ShadowBoxes.Size(), 4));

    m_ShadowCasterCullResult.ResizeInvalidate(m_ShadowBoxes.Size() / 4);

    BvFrustum frustum;

    for (int lightIndex = 0; lightIndex < m_RenderDef.View->NumDirectionalLights; lightIndex++)
    {
        int lightOffset = m_RenderDef.View->FirstDirectionalLight + lightIndex;

        DirectionalLightInstance* lightDef = m_FrameData.DirectionalLights[lightOffset];

        if (lightDef->NumCascades == 0)
        {
            continue;
        }

        lightDef->ShadowmapIndex = m_FrameData.LightShadowmaps.Size();

        LightShadowmap* shadowMap = &m_FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount = 0;

        Float4x4* lightViewProjectionMatrices = (Float4x4*)streamedMemory->Map(lightDef->ViewProjStreamHandle);

        // Perform culling for each cascade
        // TODO: Do it parallel (jobs)
        for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
        {
            frustum.FromMatrix(lightViewProjectionMatrices[cascadeIndex]);

            m_ShadowCasterCullResult.ZeroMem();

            frustum.CullBox_SSE(m_ShadowBoxes.ToPtr(), m_ShadowCasters.Size(), &m_ShadowCasterCullResult[0].Result[0]);
            //frustum.CullBox_Generic( m_ShadowBoxes.ToPtr(), m_ShadowCasters.Size(), m_ShadowCasterCullResult.ToPtr() );

            for (int n = 0, n2 = 0; n < m_ShadowCasters.Size(); n += 4, n2++)
            {
                for (int t = 0; t < 4 && n + t < m_ShadowCasters.Size(); t++)
                    m_ShadowCasters[n + t]->CascadeMask |= (m_ShadowCasterCullResult[n2].Result[t] == 0) << cascadeIndex;
            }
        }

        worldECS->AddShadows(m_RenderDef, m_FrameData, shadowMap);

        for (int n = 0; n < m_ShadowCasters.Size(); n++)
        {
            Drawable* component = m_ShadowCasters[n];

            if (component->CascadeMask == 0)
            {
                continue;
            }

            //switch (component->GetDrawableType())
            //{
            //    case DRAWABLE_STATIC_MESH:
            //        AddShadowmap_StaticMesh(shadowMap, static_cast<MeshComponent*>(component));
            //        break;
            //    default:
            //        break;
            //}

            // Clear cascade mask for next light source
            component->CascadeMask = 0;
        }

        // Add static shadow casters
        for (Level* level : InWorld->GetArrayOfLevels())
        {
            LevelLighting* lighting = level->Lighting;

            if (!lighting)
                continue;

            // TODO: Perform culling for each shadow cascade, set CascadeMask

            if (!lighting->GetShadowCasterIndexCount())
            {
                continue;
            }

            // Add render instance
            ShadowRenderInstance* instance = (ShadowRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(ShadowRenderInstance));

            m_FrameData.ShadowInstances.Add(instance);

            instance->Material = nullptr;
            instance->MaterialInstance = nullptr;
            instance->VertexBuffer = lighting->GetShadowCasterVB();
            instance->VertexBufferOffset = 0;
            instance->IndexBuffer = lighting->GetShadowCasterIB();
            instance->IndexBufferOffset = 0;
            instance->WeightsBuffer = nullptr;
            instance->WeightsBufferOffset = 0;
            instance->IndexCount = lighting->GetShadowCasterIndexCount();
            instance->StartIndexLocation = 0;
            instance->BaseVertexLocation = 0;
            instance->SkeletonOffset = 0;
            instance->SkeletonSize = 0;
            instance->WorldTransformMatrix.SetIdentity();
            instance->CascadeMask = 0xffff; // TODO: Calculate!!!
            instance->SortKey = 0;

            shadowMap->ShadowInstanceCount++;

            m_RenderDef.ShadowMapPolyCount += instance->IndexCount / 3;
        }

        SortShadowInstances(shadowMap);

        if (r_RenderLightPortals)
        {
            // Add light portals
            for (Level* level : InWorld->GetArrayOfLevels())
            {
                LevelLighting* lighting = level->Lighting;

                if (!lighting)
                    continue;

                Vector<LightPortalDef> const& lightPortals = lighting->GetLightPortals();

                if (lightPortals.IsEmpty())
                {
                    continue;
                }

                for (LightPortalDef const& lightPortal : lightPortals)
                {

                    // TODO: Perform culling for each light portal
                    // NOTE: We can precompute visible geometry for static light and meshes from every light portal

                    LightPortalRenderInstance* instance = (LightPortalRenderInstance*)m_FrameLoop->AllocFrameMem(sizeof(LightPortalRenderInstance));

                    m_FrameData.LightPortals.Add(instance);

                    instance->VertexBuffer = lighting->GetLightPortalsVB();
                    instance->VertexBufferOffset = 0;
                    instance->IndexBuffer = lighting->GetLightPortalsIB();
                    instance->IndexBufferOffset = 0;
                    instance->IndexCount = lightPortal.NumIndices;
                    instance->StartIndexLocation = lightPortal.FirstIndex;
                    instance->BaseVertexLocation = 0;

                    shadowMap->LightPortalsCount++;

                    //m_RenderDef.LightPortalPolyCount += instance->IndexCount / 3;
                }
            }
        }
    }
}
#endif

HK_NAMESPACE_END
