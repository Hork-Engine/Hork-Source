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

#include "WorldRenderer.h"
#include <Hork/Runtime/GameApplication/GameApplication.h>

#include <Hork/Core/Profiler.h>
#include <Hork/Core/Platform.h>
#include <Hork/Geometry/BV/BvIntersect.h>

#include <Hork/Runtime/World/Modules/Render/Components/CameraComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/TerrainComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>
#include <Hork/Runtime/World/Modules/Render/TerrainView.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_RenderView("r_RenderView"_s, "1"_s, CVAR_CHEAT);
ConsoleVar r_ResolutionScaleX("r_ResolutionScaleX"_s, "1"_s);
ConsoleVar r_ResolutionScaleY("r_ResolutionScaleY"_s, "1"_s);
ConsoleVar r_RenderLightPortals("r_RenderLightPortals"_s, "1"_s);
ConsoleVar r_VertexLight("r_VertexLight"_s, "0"_s);
ConsoleVar r_MotionBlur("r_MotionBlur"_s, "1"_s);
ConsoleVar r_RenderMeshes("r_RenderMeshes"_s, "1"_s, CVAR_CHEAT);
ConsoleVar r_RenderTerrain("r_RenderTerrain"_s, "1"_s, CVAR_CHEAT);

extern ConsoleVar r_HBAO;
extern ConsoleVar r_HBAODeinterleaved;

ConsoleVar com_DrawFrustumClusters("com_DrawFrustumClusters"_s, "0"_s, CVAR_CHEAT);

void WorldRenderer::AddRenderView(WorldRenderView* renderView)
{
    // TODO: Sort by render order. Render order get from renderView
    m_RenderViews.EmplaceBack(renderView);
}

void WorldRenderer::Render(FrameLoop* frameLoop)
{
    HK_PROFILER_EVENT("Render frontend");

    StreamedMemoryGPU* streamedMemory = frameLoop->GetStreamedMemoryGPU();

    m_FrameLoop = frameLoop;
    m_FrameNumber = frameLoop->SysFrameNumber();
    m_DebugDraw.Reset();

    m_Stat.FrontendTime = Core::SysMilliseconds();
    m_Stat.PolyCount = 0;
    m_Stat.ShadowMapPolyCount = 0;

    m_FrameData.FrameNumber = m_FrameNumber;

    m_FrameData.Instances.Clear();
    m_FrameData.TranslucentInstances.Clear();
    m_FrameData.OutlineInstances.Clear();
    m_FrameData.ShadowInstances.Clear();
    m_FrameData.LightPortals.Clear();
    m_FrameData.DirectionalLights.Clear();
    m_FrameData.LightShadowmaps.Clear();
    m_FrameData.TerrainInstances.Clear();

    m_FrameData.NumViews = m_RenderViews.Size();
    m_FrameData.RenderViews = (RenderViewData*)frameLoop->AllocFrameMem(sizeof(RenderViewData) * m_FrameData.NumViews);
    Core::ZeroMem(m_FrameData.RenderViews, sizeof(RenderViewData) * m_FrameData.NumViews);

    for (int i = 0; i < m_FrameData.NumViews; i++)
    {
        WorldRenderView* worldRenderView = m_RenderViews[i];
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

    m_RenderViews.Clear();
}

void WorldRenderer::ClearRenderView(RenderViewData* view)
{
    Core::ZeroMem(view, sizeof(*view));
}

static constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    {0.5f, 0.0f, 0.0f, 0.0f},
    {0.0f, -0.5f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 0.5f, 0.0f, 1.0f});

void WorldRenderer::AddShadowmapCascades(DirectionalLightComponent const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* streamedMemory, RenderViewData* view, size_t* viewProjStreamHandle, int* pFirstCascade, int* pNumCascades)
{
    float cascadeSplits[MAX_CASCADE_SPLITS];
    int numSplits = light.GetMaxShadowCascades() + 1;
    int numVisibleSplits;
    Float4x4 lightViewMatrix;
    Float3 worldspaceVerts[MAX_CASCADE_SPLITS][4];
    Float3 right, up;

    HK_ASSERT(light.GetMaxShadowCascades() > 0 && light.GetMaxShadowCascades() <= MAX_SHADOW_CASCADES);

    if (view->bPerspective)
    {
        float tanFovX = std::tan(view->ViewFovX * 0.5f);
        float tanFovY = std::tan(view->ViewFovY * 0.5f);
        right = view->ViewRightVec * tanFovX;
        up = view->ViewUpVec * tanFovY;
    }
    else
    {
        float orthoWidth = view->ViewOrthoMaxs.X - view->ViewOrthoMins.X;
        float orthoHeight = view->ViewOrthoMaxs.Y - view->ViewOrthoMins.Y;
        right = view->ViewRightVec * Math::Abs(orthoWidth * 0.5f);
        up = view->ViewUpVec * Math::Abs(orthoHeight * 0.5f);
    }

    const float shadowMaxDistance = light.GetShadowMaxDistance();
    const float offset = light.GetShadowCascadeOffset();
    const float a = (shadowMaxDistance - offset) / view->ViewZNear;
    const float b = (shadowMaxDistance - offset) - view->ViewZNear;
    const float lambda = light.GetShadowCascadeSplitLambda();

    // Calc splits
    cascadeSplits[0] = view->ViewZNear;
    cascadeSplits[MAX_CASCADE_SPLITS - 1] = shadowMaxDistance;

    for (int splitIndex = 1; splitIndex < MAX_CASCADE_SPLITS - 1; splitIndex++)
    {
        const float factor = (float)splitIndex / (MAX_CASCADE_SPLITS - 1);
        const float logarithmic = view->ViewZNear * Math::Pow(a, factor);
        const float linear = view->ViewZNear + b * factor;
        const float dist = Math::Lerp(linear, logarithmic, lambda);
        cascadeSplits[splitIndex] = offset + dist;
    }

    float maxVisibleDist = Math::Max(view->MaxVisibleDistance, cascadeSplits[0]);

    // Calc worldspace verts
    for (numVisibleSplits = 0;
        numVisibleSplits < numSplits && (cascadeSplits[Math::Max(0, numVisibleSplits - 1)] <= maxVisibleDist);
        numVisibleSplits++)
    {
        Float3* pWorldSpaceVerts = worldspaceVerts[numVisibleSplits];

        float d = cascadeSplits[numVisibleSplits];

        // FIXME: variable distance can cause edge shimmering
        //d = d > maxVisibleDist ? maxVisibleDist : d;

        Float3 centerWorldspace = view->ViewPosition + view->ViewDir * d;

        Float3 c1 = right + up;
        Float3 c2 = right - up;

        if (view->bPerspective)
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

    const float halfCascadeRes = light.GetShadowCascadeResolution() >> 1;
    const float oneOverHalfCascadeRes = 1.0f / halfCascadeRes;

    int firstCascade = view->NumShadowMapCascades;

    // Distance from cascade bounds to light source (near clip plane)
    // NOTE: We can calc actual light distance from scene geometry,
    // but now it just a magic number big enough to enclose most scenes = 1km.
    const float lightDistance = 1000.0f;

    Float4x4* lightViewProjectionMatrices = nullptr;
    if (numVisibleCascades > 0)
    {
        *viewProjStreamHandle = streamedMemory->AllocateConstant(numVisibleCascades * sizeof(Float4x4), nullptr);

        lightViewProjectionMatrices = (Float4x4*)streamedMemory->Map(*viewProjStreamHandle);
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
        Float4x4 cascadeMatrix = Float4x4::sGetOrthoMatrix(orthoDesc) * lightViewMatrix;

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
        view->ShadowMapMatrices[cascadeIndex] = ShadowMapBias * cascadeMatrix * view->ClipSpaceToWorldSpace;
    }

    view->NumShadowMapCascades += numVisibleCascades;

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

void WorldRenderer::AddDirectionalLightShadows(LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef)
{
    if (!m_Context.View->NumShadowMapCascades)
        return;

    //if (!r_RenderMeshes)
    //    return;
#if 0
    alignas(16) BvAxisAlignedBoxSSE bounds[4];
    alignas(16) int32_t cullResult[4];

    BvFrustum frustum[MAX_SHADOW_CASCADES];

    Float4x4* lightViewProjectionMatrices = (Float4x4*)m_Context.StreamedMemory->Map(lightDef->ViewProjStreamHandle);

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

        auto* meshResource = GameApplication::sGetResourceManager().TryGet(mesh.m_Resource);
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
    AddMeshesShadow<StaticMeshComponent, DirectionalLightComponent>(shadowmap);
    AddMeshesShadow<DynamicMeshComponent, DirectionalLightComponent>(shadowmap);
}

template <typename MeshComponentType>
constexpr bool IsDynamicMesh();

template <typename LightComponentType>
constexpr bool IsPunctualLight() { return false; }

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

template <>
constexpr bool IsPunctualLight<PunctualLightComponent>()
{
    return true;
}

template <typename MeshComponentType>
void WorldRenderer::AddMeshes()
{
    PreRenderContext context;
    context.FrameNum = m_Context.FrameNumber;
    context.Prev = m_World->GetTick().PrevStateIndex;
    context.Cur = m_World->GetTick().StateIndex;
    context.Frac = m_World->GetTick().Interpolate;

    auto& meshManager = m_World->GetComponentManager<MeshComponentType>();
    for (auto it = meshManager.GetComponents(); it.IsValid(); ++it)
    {
        MeshComponentType& mesh = *it;

        if (!mesh.IsInitialized())
            continue;

        if (!(m_Context.VisibilityMask & (1 << mesh.GetVisibilityLayer())))
            continue;

        mesh.PreRender(context);

        if (!m_Context.Frustum->IsBoxVisible(mesh.GetWorldBoundingBox()))
            continue;

        Float4x4 instanceMatrix = m_View->ViewProjection * mesh.GetRenderTransform();
        Float4x4 instanceMatrixP = m_View->ViewProjectionP * mesh.GetRenderTransformPrev();

        Float3x3 modelNormalToViewSpace = m_View->NormalToViewMatrix * mesh.GetRotationMatrix();

        if (auto* meshResource = GameApplication::sGetResourceManager().TryGet(mesh.GetMesh()))
        {
            int surfaceCount = meshResource->GetSurfaceCount();
            for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
            {
                Material* materialInstance = mesh.GetMaterial(surfaceIndex);
                if (!materialInstance)
                    continue;

                MaterialResource* material = GameApplication::sGetResourceManager().TryGet(materialInstance->GetResource());
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

                m_Context.PolyCount += instance->IndexCount / 3;
            }
        }

        ProceduralMesh* proceduralMesh = mesh.GetProceduralMesh();
        if (proceduralMesh && !proceduralMesh->IndexCache.IsEmpty())
        {
            Material* materialInstance = mesh.GetMaterial(0);
            if (!materialInstance)
                continue;

            MaterialResource* material = GameApplication::sGetResourceManager().TryGet(materialInstance->GetResource());
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

            proceduralMesh->PrepareStreams(m_Context);
            proceduralMesh->GetVertexBufferGPU(m_Context.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
            proceduralMesh->GetIndexBufferGPU(m_Context.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

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

            m_Context.PolyCount += instance->IndexCount / 3;
        }
    }
}

template <typename MeshComponentType, typename LightComponentType>
void WorldRenderer::AddMeshesShadow(LightShadowmap* shadowMap, BvAxisAlignedBox const& lightBounds)
{
    PreRenderContext context;
    context.FrameNum = m_Context.FrameNumber;
    context.Prev = m_World->GetTick().PrevStateIndex;
    context.Cur = m_World->GetTick().StateIndex;
    context.Frac = m_World->GetTick().Interpolate;

    auto& meshManager = m_World->GetComponentManager<MeshComponentType>();
    for (auto it = meshManager.GetComponents(); it.IsValid(); ++it)
    {
        MeshComponentType& mesh = *it;

        if (!mesh.IsInitialized())
            continue;

        if (!(m_Context.VisibilityMask & (1 << mesh.GetVisibilityLayer())))
            continue;

        mesh.PreRender(context);

        if constexpr (IsPunctualLight<LightComponentType>())
        {
            if (!BvBoxOverlapBox(mesh.GetWorldBoundingBox(), lightBounds))
                continue;
        }

        Float3x4 const& instanceMatrix = mesh.GetRenderTransform();

        auto* meshResource = GameApplication::sGetResourceManager().TryGet(mesh.GetMesh());
        if (meshResource)
        {
            int surfaceCount = meshResource->GetSurfaceCount();
            for (int surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex)
            {
                Material* materialInstance = mesh.GetMaterial(surfaceIndex);
                if (!materialInstance)
                    continue;

                MaterialResource* material = GameApplication::sGetResourceManager().TryGet(materialInstance->GetResource());
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

                m_Context.ShadowMapPolyCount += instance->IndexCount / 3;
            }
        }

        ProceduralMesh* proceduralMesh = mesh.GetProceduralMesh();
        if (proceduralMesh && !proceduralMesh->IndexCache.IsEmpty())
        {        
            Material* materialInstance = mesh.GetMaterial(0);
            if (!materialInstance)
                continue;

            MaterialResource* material = GameApplication::sGetResourceManager().TryGet(materialInstance->GetResource());
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

            proceduralMesh->PrepareStreams(m_Context);
            proceduralMesh->GetVertexBufferGPU(m_Context.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
            proceduralMesh->GetIndexBufferGPU(m_Context.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

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

            m_Context.ShadowMapPolyCount += instance->IndexCount / 3;
        }
    }
}

bool WorldRenderer::AddLightShadowmap(PunctualLightComponent* light, float radius)
{
    if (!light->IsCastShadow())
        return false;

    Float3 const& lightPos = light->GetRenderPosition();

    int totalInstances = 0;

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        LightShadowmap* shadowMap = &m_FrameData.LightShadowmaps.Add();

        shadowMap->FirstShadowInstance = m_FrameData.ShadowInstances.Size();
        shadowMap->ShadowInstanceCount = 0;
        shadowMap->FirstLightPortal = m_FrameData.LightPortals.Size();
        shadowMap->LightPortalsCount = 0;
        shadowMap->LightPosition = lightPos;
        
        // TODO: Add only visible objects
        AddMeshesShadow<StaticMeshComponent, PunctualLightComponent>(shadowMap, light->GetWorldBoundingBox());
        AddMeshesShadow<DynamicMeshComponent, PunctualLightComponent>(shadowMap, light->GetWorldBoundingBox());

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

void WorldRenderer::RenderView(WorldRenderView* worldRenderView, RenderViewData* view)
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

    auto* cullingCamera = cameraManager.GetComponent(worldRenderView->GetCullingCamera());
    if (!cullingCamera)
        cullingCamera = camera;

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
        TextureResource* lutTexture = GameApplication::sGetResourceManager().TryGet(lut);

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

    view->PhotometricProfiles = world->GetInterface<RenderInterface>().GetPhotometricPool().GetTexture().RawPtr();

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
    if (camera == cullingCamera)
    {
        frustum.FromMatrix(view->ViewProjection, true);
    }
    else
    {
        frustum = cullingCamera->GetFrustum();
    }

    m_Context.WorldRV = worldRenderView;
    m_Context.FrameNumber = m_FrameNumber;
    m_Context.View = view;
    m_Context.Frustum = &frustum;
    m_Context.VisibilityMask = (VISIBILITY_GROUP)camera->GetVisibilityMask();
    m_Context.PolyCount = 0;
    m_Context.ShadowMapPolyCount = 0;
    m_Context.StreamedMemory = m_FrameLoop->GetStreamedMemoryGPU();

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

            auto* terrainResource = GameApplication::sGetResourceManager().TryGet(terrain.GetResource());
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
            auto terrainView = worldRenderView->GetTerrainView(terrain.GetResource());

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

            auto& frameLoop = GameApplication::sGetFrameLoop();

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

            if (light.IsCastShadow())
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

            instance->ColorAndAmbientIntensity = light.GetEffectiveColor();
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
    context.FrameNum = m_Context.FrameNumber;
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

    int maxOmnidirectionalShadowMaps = GameApplication::sGetRenderBackend().MaxOmnidirectionalShadowMapsPerView();

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

        if (!m_Context.Frustum->IsBoxVisible(light.GetWorldBoundingBox())) // TODO: Check bounding sphere for point lights
            continue;

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

        ItemInfo* info = m_LightVoxelizer.AllocItem();
        info->Type = ITEM_TYPE_LIGHT;
        info->ListIndex = index;

        BvAxisAlignedBox const& AABB = light.GetWorldBoundingBox();
        info->Mins = AABB.Mins;
        info->Maxs = AABB.Maxs;

        if (m_LightVoxelizer.IsSSE())
            info->ClipToBoxMatSSE = light.GetOBBTransformInverse() * view->ClusterViewProjectionInversed;
        else
            info->ClipToBoxMat = light.GetOBBTransformInverse() * view->ClusterViewProjectionInversed;

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

    m_Stat.PolyCount += m_Context.PolyCount;
    m_Stat.ShadowMapPolyCount += m_Context.ShadowMapPolyCount;

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

void WorldRenderer::SortRenderInstances()
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

void WorldRenderer::SortShadowInstances(LightShadowmap const* shadowMap)
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

void WorldRenderer::QueryVisiblePrimitives(World* world)
{
    VisibilityQuery query;

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &(*m_Context.Frustum)[i];
    }
    query.ViewPosition = m_Context.View->ViewPosition;
    query.ViewRightVec = m_Context.View->ViewRightVec;
    query.ViewUpVec = m_Context.View->ViewUpVec;
    query.VisibilityMask = m_Context.VisibilityMask;
    query.QueryMask = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS; // | VSD_QUERY_MASK_SHADOW_CAST;

    //world->QueryVisiblePrimitives(m_VisPrimitives, &m_VisPass, query);
}

void WorldRenderer::QueryShadowCasters(World* world, Float4x4 const& lightViewProjection, Float3 const& lightPosition, Float3x3 const& lightBasis, Vector<PrimitiveDef*>& primitives)
{
    VisibilityQuery query;
    BvFrustum frustum;

    frustum.FromMatrix(lightViewProjection, true);

    for (int i = 0; i < 6; i++)
    {
        query.FrustumPlanes[i] = &frustum[i];
    }
    query.ViewPosition = lightPosition;
    query.ViewRightVec = lightBasis[0];
    query.ViewUpVec = lightBasis[1];
    query.VisibilityMask = m_Context.VisibilityMask;
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

    Float4x4 inversed = lightViewProjection.Inversed();
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
    Float3 origin = lightPosition;
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
    //world->QueryVisiblePrimitives(primitives, nullptr, query);
}


HK_NAMESPACE_END
