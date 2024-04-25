#include "RenderSystem.h"

#include <Engine/World/Common/GameFrame.h>

#include <Engine/World/Modules/Transform/Components/MovableTag.h>
#include <Engine/World/Modules/Transform/Components/WorldTransformComponent.h>
#include <Engine/World/Modules/Transform/Components/TransformHistoryComponent.h>
#include <Engine/World/Modules/Transform/Components/RenderTransformComponent.h>
#include <Engine/World/Modules/Terrain/TerrainView.h>

#include <Engine/World/Resources/ResourceManager.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Resources/Resource_Texture.h>

#include <Engine/Core/ConsoleVar.h>
#include <Engine/GameApplication/GameApplication.h>

#include "../Components/ExperimentalComponents.h"
#include "../Components/ShadowCastTag.h"

HK_NAMESPACE_BEGIN

ConsoleVar r_RenderMeshes("r_RenderMeshes"s, "1"s, CVAR_CHEAT);
ConsoleVar r_RenderTerrain("r_RenderTerrain"s, "1"s, CVAR_CHEAT);

ConsoleVar com_DrawMeshDebug("com_DrawMeshDebug"s, "0"s);
ConsoleVar com_DrawMeshBounds("com_DrawMeshBounds"s, "0"s);
ConsoleVar com_DrawTerrainMesh("com_DrawTerrainMesh"s, "0"s);

static constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    {0.5f, 0.0f, 0.0f, 0.0f},
    {0.0f, -0.5f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 0.5f, 0.0f, 1.0f});

RenderSystem::RenderSystem(ECS::World* world) :
    m_World(world)
{
    world->AddEventHandler<ECS::Event::OnComponentAdded<MeshComponent_ECS>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<MeshComponent_ECS>>(this);
}

RenderSystem::~RenderSystem()
{
    m_World->RemoveHandler(this);
}

void RenderSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<MeshComponent_ECS> const& event)
{
    auto& mesh = event.Component();

    auto view = world->GetEntityView(event.GetEntity());
    auto worldTransform = view.GetComponent<WorldTransformComponent>();

    if (worldTransform)
    {
        Float3x4 transformMatrix;
        Float3x3 worldRotation = worldTransform->Rotation[0].ToMatrix3x3();
        transformMatrix.Compose(worldTransform->Position[0], worldRotation, worldTransform->Scale[0]);

        mesh.m_WorldBoundingBox = mesh.BoundingBox.Transform(transformMatrix);
    }

    //mesh.m_PrimID = m_SpatialTree.AddPrimitive(mesh.m_WorldBoundingBox);
    //m_SpatialTree.AssignEntity(mesh.m_PrimID, event.GetEntity());
}

void RenderSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<MeshComponent_ECS> const& event)
{
    //auto& mesh = event.Component();

    //m_SpatialTree.RemovePrimitive(mesh.m_PrimID);
}

void RenderSystem::UpdateBoundingBoxes(GameFrame const& frame)
{
    {
        using Query = ECS::Query<>
            ::Required<MeshComponent_ECS>
            ::ReadOnly<WorldTransformComponent>
            ::ReadOnly<MovableTag>;

        for (Query::Iterator it(*m_World); it; it++)
        {
            MeshComponent_ECS* mesh = it.Get<MeshComponent_ECS>();
            WorldTransformComponent const* transform = it.Get<WorldTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                Float3x4 transformMatrix;
                Float3x3 worldRotation = transform[i].Rotation[frame.StateIndex].ToMatrix3x3();
                transformMatrix.Compose(transform[i].Position[frame.StateIndex], worldRotation, transform[i].Scale[frame.StateIndex]);

                if (mesh[i].Pose)
                    mesh[i].m_WorldBoundingBox = mesh[i].Pose->m_Bounds.Transform(transformMatrix);
                else
                    mesh[i].m_WorldBoundingBox = mesh[i].BoundingBox.Transform(transformMatrix);

                //m_SpatialTree.SetBounds(mesh[i].m_PrimID, mesh[i].m_WorldBoundingBox); // TODO
            }
        }
    }

    {
        using Query = ECS::Query<>
            ::Required<ProceduralMeshComponent_ECS>
            ::ReadOnly<WorldTransformComponent>
            ::ReadOnly<MovableTag>;

        for (Query::Iterator it(*m_World); it; it++)
        {
            ProceduralMeshComponent_ECS* mesh = it.Get<ProceduralMeshComponent_ECS>();
            WorldTransformComponent const* transform = it.Get<WorldTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                Float3x4 transformMatrix;
                Float3x3 worldRotation = transform[i].Rotation[frame.StateIndex].ToMatrix3x3();
                transformMatrix.Compose(transform[i].Position[frame.StateIndex], worldRotation, transform[i].Scale[frame.StateIndex]);

                mesh[i].m_WorldBoundingBox = mesh[i].BoundingBox.Transform(transformMatrix);

                //m_SpatialTree.SetBounds(mesh[i].m_PrimID, mesh[i].m_WorldBoundingBox); // TODO
            }
        }
    }
}

void RenderSystem::AddShadowmapCascades(DirectionalLightComponent const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* StreamedMemory, RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades)
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
        Float4x4 cascadeMatrix = Float4x4::OrthoCC(Float2(cascadeMins), Float2(cascadeMaxs), cascadeMins[2], cascadeMaxs[2]) * lightViewMatrix;

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
Float3x3 DirectionToMatrix(Float3 const& direction)
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

void RenderSystem::SortShadowInstances(RenderFrameData& frameData, LightShadowmap const* shadowMap)
{
    struct ShadowInstanceSortFunction
    {
        bool operator()(ShadowRenderInstance const* a, ShadowRenderInstance const* b)
        {
            return a->SortKey < b->SortKey;
        }
    } shadowInstanceSortFunction;

    std::sort(frameData.ShadowInstances.Begin() + shadowMap->FirstShadowInstance,
              frameData.ShadowInstances.Begin() + (shadowMap->FirstShadowInstance + shadowMap->ShadowInstanceCount),
              shadowInstanceSortFunction);
}

void RenderSystem::AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    auto& frameLoop = GameApplication::GetFrameLoop();

    using Query = ECS::Query<>
        ::ReadOnly<DirectionalLightComponent>
        ::ReadOnly<RenderTransformComponent>;

    for (Query::Iterator it(*m_World); it; it++)
    {
        DirectionalLightComponent const* lights = it.Get<DirectionalLightComponent>();
        RenderTransformComponent const* transform = it.Get<RenderTransformComponent>();

        bool bCastShadow = it.HasComponent<ShadowCastTag>();

        for (int i = 0; i < it.Count(); i++)
        {
            if (rd.View->NumDirectionalLights < MAX_DIRECTIONAL_LIGHTS)
            {
                DirectionalLightInstance* instance = (DirectionalLightInstance*)frameLoop.AllocFrameMem(sizeof(DirectionalLightInstance));

                frameData.DirectionalLights.Add(instance);

                auto& light = lights[i];

                Float3x3 rotationMat = FixupLightRotation(transform[i].Rotation);

                if (bCastShadow)
                {
                    AddShadowmapCascades(light, rotationMat, frameLoop.GetStreamedMemoryGPU(), rd.View, &instance->ViewProjStreamHandle, &instance->FirstCascade, &instance->NumCascades);

                    rd.View->NumCascadedShadowMaps += instance->NumCascades > 0 ? 1 : 0; // Just statistics
                }
                else
                {
                    instance->FirstCascade = 0;
                    instance->NumCascades = 0;
                }

                instance->ColorAndAmbientIntensity = light.m_EffectiveColor;
                instance->Matrix = rotationMat;
                instance->MaxShadowCascades = light.GetMaxShadowCascades();
                instance->RenderMask = ~0; //light.RenderingGroup;
                instance->ShadowmapIndex = -1;
                instance->ShadowCascadeResolution = light.GetShadowCascadeResolution();

                rd.View->NumDirectionalLights++;
            }
            else
            {
                LOG("MAX_DIRECTIONAL_LIGHTS hit\n");
                break;
            }
        }
    }

    for (int lightIndex = 0; lightIndex < rd.View->NumDirectionalLights; lightIndex++)
    {
        DirectionalLightInstance* lightDef = frameData.DirectionalLights[rd.View->FirstDirectionalLight + lightIndex];
        if (lightDef->NumCascades == 0)
            continue;

        lightDef->ShadowmapIndex = frameData.LightShadowmaps.Size();

        LightShadowmap& shadowMap = frameData.LightShadowmaps.Add();

        shadowMap.FirstShadowInstance = frameData.ShadowInstances.Size();
        shadowMap.ShadowInstanceCount = 0;
        shadowMap.FirstLightPortal = frameData.LightPortals.Size();
        shadowMap.LightPortalsCount = 0;

        AddDirectionalLightShadows(rd, frameData, &shadowMap, lightDef);
        SortShadowInstances(frameData, &shadowMap);
    }
}

void RenderSystem::AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    if (r_RenderMeshes)
    {
        {
            using Query = ECS::Query<>
                ::ReadOnly<MeshComponent_ECS>
                ::ReadOnly<RenderTransformComponent>;

            for (Query::Iterator q(*m_World); q; q++)
            {
                MeshComponent_ECS const* mesh = q.Get<MeshComponent_ECS>();
                RenderTransformComponent const* transform = q.Get<RenderTransformComponent>();
                TransformHistoryComponent const* history = q.TryGet<TransformHistoryComponent>();

                bool bMovable = q.HasComponent<MovableTag>();
                bool bHasTransformHistory = !!history;

                for (int i = 0; i < q.Count(); i++)
                {
                    AddMesh(rd, frameData, transform[i], mesh[i], bHasTransformHistory ? &history[i].TransformHistory : nullptr, mesh[i].Pose, bMovable);
                }
            }
        }

        {
            using Query = ECS::Query<>
                ::ReadOnly<ProceduralMeshComponent_ECS>
                ::ReadOnly<RenderTransformComponent>;

            for (Query::Iterator q(*m_World); q; q++)
            {
                ProceduralMeshComponent_ECS const* mesh = q.Get<ProceduralMeshComponent_ECS>();
                RenderTransformComponent const* transform = q.Get<RenderTransformComponent>();
                TransformHistoryComponent const* history = q.TryGet<TransformHistoryComponent>();

                bool bMovable = q.HasComponent<MovableTag>();
                bool bHasTransformHistory = !!history;

                for (int i = 0; i < q.Count(); i++)
                {
                    AddProceduralMesh(rd, frameData, transform[i], mesh[i], bHasTransformHistory ? &history[i].TransformHistory : nullptr, bMovable);
                }
            }
        }
    }


    if (r_RenderTerrain)
    {
        using Query = ECS::Query<>
            ::ReadOnly<TerrainComponent_ECS>
            ::ReadOnly<RenderTransformComponent>;

        for (Query::Iterator q(*m_World); q; q++)
        {
            TerrainComponent_ECS const* terrain = q.Get<TerrainComponent_ECS>();
            RenderTransformComponent const* transform = q.Get<RenderTransformComponent>();
            TransformHistoryComponent const* history = q.TryGet<TransformHistoryComponent>();

            bool bMovable = q.HasComponent<MovableTag>();
            bool bHasTransformHistory = !!history;

            for (int i = 0; i < q.Count(); i++)
            {
                AddTerrain(rd, frameData, transform[i], terrain[i], bHasTransformHistory ? &history[i].TransformHistory : nullptr, bMovable);
            }
        }
    }
}

void RenderSystem::AddDirectionalLightShadows(RenderFrontendDef& rd, RenderFrameData& frameData, LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef)
{
    if (!rd.View->NumShadowMapCascades)
        return;

    if (!r_RenderMeshes)
        return;

    alignas(16) BvAxisAlignedBoxSSE bounds[4];
    alignas(16) int32_t cullResult[4];

    BvFrustum frustum[MAX_SHADOW_CASCADES];

    Float4x4* lightViewProjectionMatrices = (Float4x4*)rd.StreamedMemory->Map(lightDef->ViewProjStreamHandle);

    for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
        frustum[cascadeIndex].FromMatrix(lightViewProjectionMatrices[cascadeIndex]);

    {
        using Query = ECS::Query<>
            ::ReadOnly<MeshComponent_ECS>
            ::ReadOnly<RenderTransformComponent>
            ::Required<ShadowCastComponent>;

        for (Query::Iterator q(*m_World); q; q++)
        {
            MeshComponent_ECS const* mesh = q.Get<MeshComponent_ECS>();
            RenderTransformComponent const* transform = q.Get<RenderTransformComponent>();
            ShadowCastComponent* shadowCast = q.Get<ShadowCastComponent>();

            int numChunks = q.Count() / 4;
            int residual = q.Count() - numChunks * 4;

            HK_ASSERT(residual < 4);

            for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
            {
                BvFrustum const& cascadeFrustum = frustum[cascadeIndex];

                int n = 0;

                for (int i = 0; i < numChunks; i++, n += 4)
                {
                    bounds[0] = mesh[n    ].m_WorldBoundingBox;
                    bounds[1] = mesh[n + 1].m_WorldBoundingBox;
                    bounds[2] = mesh[n + 2].m_WorldBoundingBox;
                    bounds[3] = mesh[n + 3].m_WorldBoundingBox;

                    cullResult[0] = 0;
                    cullResult[1] = 0;
                    cullResult[2] = 0;
                    cullResult[3] = 0;

                    cascadeFrustum.CullBox_SSE(bounds, 4, cullResult);

                    shadowCast[n    ].m_CascadeMask |= (cullResult[0] == 0) << cascadeIndex;
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

                AddMeshShadow(rd, frameData, transform[i], mesh[i], mesh[i].Pose, shadowCast[i], shadowmap);

                shadowCast[i].m_CascadeMask = 0;
            }
        }
    }

    {
        using Query = ECS::Query<>
            ::ReadOnly<ProceduralMeshComponent_ECS>
            ::ReadOnly<RenderTransformComponent>
            ::Required<ShadowCastComponent>;

        for (Query::Iterator q(*m_World); q; q++)
        {
            ProceduralMeshComponent_ECS const* mesh = q.Get<ProceduralMeshComponent_ECS>();
            RenderTransformComponent const* transform = q.Get<RenderTransformComponent>();
            ShadowCastComponent* shadowCast = q.Get<ShadowCastComponent>();

            int numChunks = q.Count() / 4;
            int residual = q.Count() - numChunks * 4;

            HK_ASSERT(residual < 4);

            for (int cascadeIndex = 0; cascadeIndex < lightDef->NumCascades; cascadeIndex++)
            {
                BvFrustum const& cascadeFrustum = frustum[cascadeIndex];

                int n = 0;

                for (int i = 0; i < numChunks; i++, n += 4)
                {
                    bounds[0] = mesh[n    ].m_WorldBoundingBox;
                    bounds[1] = mesh[n + 1].m_WorldBoundingBox;
                    bounds[2] = mesh[n + 2].m_WorldBoundingBox;
                    bounds[3] = mesh[n + 3].m_WorldBoundingBox;

                    cullResult[0] = 0;
                    cullResult[1] = 0;
                    cullResult[2] = 0;
                    cullResult[3] = 0;

                    cascadeFrustum.CullBox_SSE(bounds, 4, cullResult);

                    shadowCast[n    ].m_CascadeMask |= (cullResult[0] == 0) << cascadeIndex;
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

                AddProceduralMeshShadow(rd, frameData, transform[i], mesh[i], shadowCast[i], shadowmap);

                shadowCast[i].m_CascadeMask = 0;
            }
        }
    }
}

void RenderSystem::AddMesh(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, MeshComponent_ECS const& mesh, Float3x4 const* transformHistory, SkeletonPose* pose, bool bMovable)
{
    Float3x4 transformMatrix;
    Float3x3 worldRotation = transform.Rotation.ToMatrix3x3();
    transformMatrix.Compose(transform.Position, worldRotation, transform.Scale);

    auto& frameLoop = GameApplication::GetFrameLoop();

    Float3x4 const& componentWorldTransform = transformMatrix;   //InComponent->GetRenderTransformMatrix(rd.FrameNumber);
    Float3x4 const& componentWorldTransformP = transformHistory ? *transformHistory : transformMatrix; //InComponent->GetRenderTransformMatrix(rd.FrameNumber + 1);

    Float4x4 instanceMatrix = rd.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = rd.View->ViewProjectionP * componentWorldTransformP;

    size_t skeletonOffset = 0;
    size_t skeletonOffsetMB = 0;
    size_t skeletonSize = 0;

    if (pose)
    {
        if (!pose->IsValid())
            return;

        skeletonOffset = pose->m_SkeletonOffset;
        skeletonOffsetMB = pose->m_SkeletonOffsetMB;
        skeletonSize = pose->m_SkeletonSize;
    }

    if (pose)
        bMovable = true; // Skinned meshes are always dynamic

    //Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    //Level* level = InComponent->GetLevel();
    //LevelLighting* lighting = level->Lighting;

    //IndexedMesh* mesh = InComponent->GetMesh();
    //IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    //bool bHasLightmap = (lighting &&
    //                     InComponent->bHasLightmap &&
    //                     InComponent->LightmapBlock < lighting->Lightmaps.Size() &&
    //                     !r_VertexLight &&
    //                     mesh->HasLightmapUVs() && !pose);

    //auto& meshRenderViews = InComponent->GetRenderViews();

    MeshResource* meshResource = GameApplication::GetResourceManager().TryGet(mesh.Mesh);

    if (!meshResource)
        return;

    for (int layerNum = 0; layerNum < mesh.NumLayers; layerNum++)
    {
        //if (!layers[layerNum].IsEnabled())
        //    continue;

        if (mesh.SubmeshIndex >= meshResource->m_Subparts.Size())
        {
            LOG("Invalid mesh subpart index\n");
            continue;
        }

        auto& subpart = meshResource->m_Subparts[mesh.SubmeshIndex];

        MaterialInstance* materialInstance = mesh.Materials[layerNum];
        if (!materialInstance)
            continue;

        MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->m_Material);
        if (!material)
            continue;

        MaterialFrameData* materialInstanceFrameData = GetMaterialFrameData(materialInstance, &frameLoop, rd.FrameNumber);
        if (!materialInstanceFrameData)
            continue;

        // Add render instance
        RenderInstance* instance = (RenderInstance*)frameLoop.AllocFrameMem(sizeof(RenderInstance));

        if (material->m_pCompiledMaterial->bTranslucent)
        {
            frameData.TranslucentInstances.Add(instance);
            rd.View->TranslucentInstanceCount++;
        }
        else
        {
            frameData.Instances.Add(instance);
            rd.View->InstanceCount++;
        }

        if (mesh.bOutline)
        {
            frameData.OutlineInstances.Add(instance);
            rd.View->OutlineInstanceCount++;
        }

        instance->Material = materialInstanceFrameData->Material;
        instance->MaterialInstance = materialInstanceFrameData;

        meshResource->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
        meshResource->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
        meshResource->GetWeightsBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

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

        instance->IndexCount = subpart.IndexCount;
        instance->StartIndexLocation = subpart.FirstIndex;
        instance->BaseVertexLocation = subpart.BaseVertex; // + InComponent->SubpartBaseVertexOffset;
        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonOffsetMB = skeletonOffsetMB;
        instance->SkeletonSize = skeletonSize;
        instance->Matrix = instanceMatrix;
        instance->MatrixP = instanceMatrixP;
        instance->ModelNormalToViewSpace = rd.View->NormalToViewMatrix * worldRotation;

        uint8_t priority = material->m_pCompiledMaterial->RenderingPriority;
        if (bMovable)
        {
            priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
        }

        instance->GenerateSortKey(priority, (uint64_t)meshResource);

        rd.PolyCount += instance->IndexCount / 3;
    }
}

void RenderSystem::AddProceduralMesh(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, ProceduralMeshComponent_ECS const& mesh, Float3x4 const* transformHistory, bool bMovable)
{
    Float3x4 transformMatrix;
    Float3x3 worldRotation = transform.Rotation.ToMatrix3x3();
    transformMatrix.Compose(transform.Position, worldRotation, transform.Scale);

    auto& frameLoop = GameApplication::GetFrameLoop();

    Float3x4 const& componentWorldTransform = transformMatrix;        //InComponent->GetRenderTransformMatrix(rd.FrameNumber);
    Float3x4 const& componentWorldTransformP = transformHistory ? *transformHistory : transformMatrix; //InComponent->GetRenderTransformMatrix(rd.FrameNumber + 1);

    Float4x4 instanceMatrix = rd.View->ViewProjection * componentWorldTransform;
    Float4x4 instanceMatrixP = rd.View->ViewProjectionP * componentWorldTransformP;

    //Float3x3 worldRotation = InComponent->GetWorldRotation().ToMatrix3x3();

    //Level* level = InComponent->GetLevel();
    //LevelLighting* lighting = level->Lighting;

    //IndexedMesh* mesh = InComponent->GetMesh();
    //IndexedMeshSubpartArray const& subparts = mesh->GetSubparts();

    //bool bHasLightmap = (lighting &&
    //                     InComponent->bHasLightmap &&
    //                     InComponent->LightmapBlock < lighting->Lightmaps.Size() &&
    //                     !r_VertexLight &&
    //                     mesh->HasLightmapUVs() && !pose);

    //auto& meshRenderViews = InComponent->GetRenderViews();

    ProceduralMesh_ECS* proceduralMesh = mesh.Mesh;

    if (!proceduralMesh)
        return;

    if (proceduralMesh->IndexCache.IsEmpty())
    {
        return;
    }

    for (int layerNum = 0; layerNum < mesh.NumLayers; layerNum++)
    {
        //if (!layers[layerNum].IsEnabled())
        //    continue;

        MaterialInstance* materialInstance = mesh.Materials[layerNum];
        if (!materialInstance)
            continue;

        MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->m_Material);
        if (!material)
            continue;

        MaterialFrameData* materialInstanceFrameData = GetMaterialFrameData(materialInstance, &frameLoop, rd.FrameNumber);
        if (!materialInstanceFrameData)
            continue;

        // Add render instance
        RenderInstance* instance = (RenderInstance*)frameLoop.AllocFrameMem(sizeof(RenderInstance));

        if (material->m_pCompiledMaterial->bTranslucent)
        {
            frameData.TranslucentInstances.Add(instance);
            rd.View->TranslucentInstanceCount++;
        }
        else
        {
            frameData.Instances.Add(instance);
            rd.View->InstanceCount++;
        }

        if (mesh.bOutline)
        {
            frameData.OutlineInstances.Add(instance);
            rd.View->OutlineInstanceCount++;
        }

        instance->Material = materialInstanceFrameData->Material;
        instance->MaterialInstance = materialInstanceFrameData;

        proceduralMesh->PrepareStreams(&rd);
        proceduralMesh->GetVertexBufferGPU(rd.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
        proceduralMesh->GetIndexBufferGPU(rd.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

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
        instance->ModelNormalToViewSpace = rd.View->NormalToViewMatrix * worldRotation;

        uint8_t priority = material->m_pCompiledMaterial->RenderingPriority;
        if (bMovable)
        {
            priority |= RENDERING_GEOMETRY_PRIORITY_DYNAMIC;
        }

        instance->GenerateSortKey(priority, (uint64_t)proceduralMesh);

        rd.PolyCount += instance->IndexCount / 3;
    }
}

void RenderSystem::AddTerrain(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, TerrainComponent_ECS const& terrainComponent, Float3x4 const* transformHistory, bool bMovable)
{
    TerrainResource* terrainResource = GameApplication::GetResourceManager().TryGet(terrainComponent.Resource);

    if (!terrainResource)
        return;

    // Terrain world rotation
    Float3x3 worldRotation = transform.Rotation.ToMatrix3x3();
    Float3x3 worldRotationInv = worldRotation.Transposed();

    // Terrain transform without scale
    //Float3x4 transformMatrix;
    //transformMatrix.Compose(transform.Position, worldRotation);

    // Terrain inversed transform
    //Float3x4 terrainWorldTransformInv = transformMatrix.Inversed();

    // Camera position in terrain space
    //Float3 localViewPosition = terrainWorldTransformInv * rd.View->ViewPosition;
    Float3 localViewPosition = worldRotationInv * (rd.View->ViewPosition - transform.Position);

    // Camera rotation in terrain space
    Float3x3 localRotation = worldRotationInv * rd.View->ViewRotation.ToMatrix3x3();

    Float3x3 basis = localRotation.Transposed();
    Float3 origin = basis * (-localViewPosition);

    Float4x4 localViewMatrix;
    localViewMatrix[0] = Float4(basis[0], 0.0f);
    localViewMatrix[1] = Float4(basis[1], 0.0f);
    localViewMatrix[2] = Float4(basis[2], 0.0f);
    localViewMatrix[3] = Float4(origin, 1.0f);

    Float4x4 localMVP = rd.View->ProjectionMatrix * localViewMatrix;

    BvFrustum localFrustum;
    localFrustum.FromMatrix(localMVP, true);

    // Update view
    auto terrainView = rd.WorldRV->GetTerrainView(terrainComponent.Resource);

    terrainView->Update(localViewPosition, localFrustum);
    if (terrainView->GetIndirectBufferDrawCount() == 0)
    {
        // Everything was culled
        return;
    }

    // TODO: transform history
    //Float3x4 const& componentWorldTransform = transformMatrix;
    //Float3x4 const& componentWorldTransformP = transformHistory ? *transformHistory : transformMatrix;
    //Float4x4 instanceMatrix = rd.View->ViewProjection * componentWorldTransform;
    //Float4x4 instanceMatrixP = rd.View->ViewProjectionP * componentWorldTransformP;

    auto& frameLoop = GameApplication::GetFrameLoop();

    TerrainRenderInstance* instance = (TerrainRenderInstance*)frameLoop.AllocFrameMem(sizeof(TerrainRenderInstance));

    frameData.TerrainInstances.Add(instance);

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
    instance->ModelNormalToViewSpace = rd.View->NormalToViewMatrix * worldRotation;
    instance->ClipMin = terrainResource->GetClipMin();
    instance->ClipMax = terrainResource->GetClipMax();

    rd.View->TerrainInstanceCount++;
}

void RenderSystem::AddMeshShadow(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, MeshComponent_ECS const& mesh, SkeletonPose* pose, ShadowCastComponent const& shadow, LightShadowmap* shadowmap)
{
    auto& frameLoop = GameApplication::GetFrameLoop();

    Float3x4 transformMatrix = transform.ToMatrix();

    Float3x4 const& instanceMatrix = transformMatrix;

    MeshResource* meshResource = GameApplication::GetResourceManager().TryGet(mesh.Mesh);

    if (!meshResource)
        return;

    size_t skeletonOffset = 0;
    size_t skeletonSize = 0;

    if (pose)
    {
        skeletonOffset = pose->m_SkeletonOffset;
        skeletonSize = pose->m_SkeletonSize;
    }

    for (int layerNum = 0; layerNum < mesh.NumLayers; layerNum++)
    {
        //if (!layers[layerNum].IsEnabled())
        //    continue;

        if (mesh.SubmeshIndex >= meshResource->m_Subparts.Size())
        {
            LOG("Invalid mesh subpart index\n");
            continue;
        }

        auto& subpart = meshResource->m_Subparts[mesh.SubmeshIndex];

        MaterialInstance* materialInstance = mesh.Materials[layerNum];
        if (!materialInstance)
            continue;

        MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->m_Material);
        if (!material)
            continue;

        // Prevent rendering of instances with disabled shadow casting
        if (material->m_pCompiledMaterial->bNoCastShadow)
        {
            continue;
        }

        MaterialFrameData* materialInstanceFrameData = GetMaterialFrameData(materialInstance, &frameLoop, rd.FrameNumber);
        if (!materialInstanceFrameData)
            continue;

        // Add render instance
        ShadowRenderInstance* instance = (ShadowRenderInstance*)frameLoop.AllocFrameMem(sizeof(ShadowRenderInstance));

        frameData.ShadowInstances.Add(instance);

        instance->Material = materialInstanceFrameData->Material;
        instance->MaterialInstance = materialInstanceFrameData;

        meshResource->GetVertexBufferGPU(&instance->VertexBuffer, &instance->VertexBufferOffset);
        meshResource->GetIndexBufferGPU(&instance->IndexBuffer, &instance->IndexBufferOffset);
        meshResource->GetWeightsBufferGPU(&instance->WeightsBuffer, &instance->WeightsBufferOffset);

        instance->IndexCount = subpart.IndexCount;
        instance->StartIndexLocation = subpart.FirstIndex;
        instance->BaseVertexLocation = subpart.BaseVertex; // + InComponent->SubpartBaseVertexOffset;
        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonSize = skeletonSize;
        instance->WorldTransformMatrix = instanceMatrix;
        instance->CascadeMask = shadow.m_CascadeMask;

        uint8_t priority = material->m_pCompiledMaterial->RenderingPriority;

        instance->GenerateSortKey(priority, (uint64_t)meshResource);

        shadowmap->ShadowInstanceCount++;

        rd.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

void RenderSystem::AddProceduralMeshShadow(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, ProceduralMeshComponent_ECS const& mesh, ShadowCastComponent const& shadow, LightShadowmap* shadowmap)
{
    auto& frameLoop = GameApplication::GetFrameLoop();

    Float3x4 transformMatrix = transform.ToMatrix();

    Float3x4 const& instanceMatrix = transformMatrix;

    ProceduralMesh_ECS* proceduralMesh = mesh.Mesh;

    if (!proceduralMesh)
        return;

    if (proceduralMesh->IndexCache.IsEmpty())
    {
        return;
    }

    for (int layerNum = 0; layerNum < mesh.NumLayers; layerNum++)
    {
        //if (!layers[layerNum].IsEnabled())
        //    continue;

        MaterialInstance* materialInstance = mesh.Materials[layerNum];
        if (!materialInstance)
            continue;

        MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->m_Material);
        if (!material)
            continue;

        // Prevent rendering of instances with disabled shadow casting
        if (material->m_pCompiledMaterial->bNoCastShadow)
        {
            continue;
        }

        MaterialFrameData* materialInstanceFrameData = GetMaterialFrameData(materialInstance, &frameLoop, rd.FrameNumber);
        if (!materialInstanceFrameData)
            continue;

        // Add render instance
        ShadowRenderInstance* instance = (ShadowRenderInstance*)frameLoop.AllocFrameMem(sizeof(ShadowRenderInstance));

        frameData.ShadowInstances.Add(instance);

        instance->Material = materialInstanceFrameData->Material;
        instance->MaterialInstance = materialInstanceFrameData;

        proceduralMesh->PrepareStreams(&rd);
        proceduralMesh->GetVertexBufferGPU(rd.StreamedMemory, &instance->VertexBuffer, &instance->VertexBufferOffset);
        proceduralMesh->GetIndexBufferGPU(rd.StreamedMemory, &instance->IndexBuffer, &instance->IndexBufferOffset);

        instance->WeightsBuffer = nullptr;
        instance->WeightsBufferOffset = 0;
        instance->IndexCount = proceduralMesh->IndexCache.Size();
        instance->StartIndexLocation = 0;
        instance->BaseVertexLocation = 0;
        instance->SkeletonOffset = 0;
        instance->SkeletonSize = 0;
        instance->WorldTransformMatrix = instanceMatrix;
        instance->CascadeMask = shadow.m_CascadeMask;

        uint8_t priority = material->m_pCompiledMaterial->RenderingPriority;

        instance->GenerateSortKey(priority, (uint64_t)proceduralMesh);

        shadowmap->ShadowInstanceCount++;

        rd.ShadowMapPolyCount += instance->IndexCount / 3;
    }
}

MaterialFrameData* RenderSystem::GetMaterialFrameData(MaterialInstance* materialInstance, FrameLoop* frameLoop, int frameNumber)
{
    if (materialInstance->m_VisFrame == frameNumber)
    {
        return materialInstance->m_FrameData;
    }

    MaterialResource* material = GameApplication::GetResourceManager().TryGet(materialInstance->m_Material);
    if (!material)
        return nullptr;

    MaterialFrameData* frameData = (MaterialFrameData*)frameLoop->AllocFrameMem(sizeof(MaterialFrameData));

    materialInstance->m_VisFrame = frameNumber;
    materialInstance->m_FrameData = frameData;

    frameData->Material    = material->m_GpuMaterial;
    frameData->NumTextures = material->m_pCompiledMaterial->Samplers.Size();

    HK_ASSERT(frameData->NumTextures <= MAX_MATERIAL_TEXTURES);

    for (int i = 0, count = frameData->NumTextures; i < count; ++i)
    {
        TextureHandle texHandle = materialInstance->m_Textures[i];

        TextureResource* texture = GameApplication::GetResourceManager().TryGet(texHandle);
        if (!texture)
        {
            materialInstance->m_FrameData = nullptr;
            return nullptr;
        }

        frameData->Textures[i] = texture->GetTextureGPU();
    }

    frameData->NumUniformVectors = material->m_pCompiledMaterial->NumUniformVectors;
    Core::Memcpy(frameData->UniformVectors, materialInstance->m_Constants, sizeof(Float4) * frameData->NumUniformVectors);

    return frameData;
}

void RenderSystem::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawMeshDebug)
    {
        using Query = ECS::Query<>
            ::ReadOnly<MeshComponent_ECS>
            ::ReadOnly<RenderTransformComponent>;

        Float3x4 matrix;
        for (Query::Iterator q(*m_World); q; q++)
        {
            MeshComponent_ECS const* mesh = q.Get<MeshComponent_ECS>();
            RenderTransformComponent const* transform = q.Get<RenderTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                if (MeshResource* meshRes = GameApplication::GetResourceManager().TryGet(mesh[i].Mesh))
                {
                    Float3x3 worldRotation = transform[i].Rotation.ToMatrix3x3();
                    matrix.Compose(transform[i].Position, worldRotation, transform[i].Scale);

                    renderer.PushTransform(matrix);
                    meshRes->DrawDebug(renderer);
                    meshRes->DrawDebugSubpart(renderer, mesh[i].SubmeshIndex);
                    renderer.PopTransform();
                }
            }
        }
    }

    if (com_DrawMeshBounds)
    {
        renderer.SetDepthTest(false);
        
        {
            using Query = ECS::Query<>
                ::ReadOnly<MeshComponent_ECS>;

            for (Query::Iterator q(*m_World); q; q++)
            {
                MeshComponent_ECS const* mesh = q.Get<MeshComponent_ECS>();

                for (int i = 0; i < q.Count(); i++)
                {
                    renderer.SetColor(mesh[i].Pose ? Color4(0.5f, 0.5f, 1, 1) : Color4(1, 1, 1, 1));
                    renderer.DrawAABB(mesh[i].m_WorldBoundingBox);
                }
            }
        }

        {
            using Query = ECS::Query<>
                ::ReadOnly<ProceduralMeshComponent_ECS>;

            renderer.SetColor(Color4(0.5f, 1, 0.5f, 1));

            for (Query::Iterator q(*m_World); q; q++)
            {
                ProceduralMeshComponent_ECS const* mesh = q.Get<ProceduralMeshComponent_ECS>();

                for (int i = 0; i < q.Count(); i++)
                    renderer.DrawAABB(mesh[i].m_WorldBoundingBox);
            }
        }
    }

    if (com_DrawTerrainMesh)
    {
        using Query = ECS::Query<>
            ::ReadOnly<TerrainComponent_ECS>
            ::ReadOnly<RenderTransformComponent>;

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(0, 0, 1, 0.5f));

        TVector<Float3> vertices;
        TVector<unsigned int> indices;

        for (Query::Iterator q(*m_World); q; q++)
        {
            TerrainComponent_ECS const* terrains = q.Get<TerrainComponent_ECS>();
            RenderTransformComponent const* transforms = q.Get<RenderTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                if (TerrainResource* resource = GameApplication::GetResourceManager().TryGet(terrains[i].Resource))
                {
                    Float3x4 transform_matrix;
                    transform_matrix.Compose(transforms[i].Position, transforms[i].Rotation.ToMatrix3x3());

                    Float3x4 transform_matrix_inv = transform_matrix.Inversed();
                    Float3 local_view_position = transform_matrix_inv * renderer.GetRenderView()->ViewPosition;

                    BvAxisAlignedBox local_bounds(local_view_position - 4, local_view_position + 4);

                    local_bounds.Mins.Y = -FLT_MAX;
                    local_bounds.Maxs.Y = FLT_MAX;

                    vertices.Clear();
                    indices.Clear();
                    resource->GatherGeometry(local_bounds, vertices, indices);

                    renderer.PushTransform(transform_matrix);
                    renderer.DrawTriangleSoupWireframe(vertices, indices);
                    renderer.PopTransform();
                }
            }
        }
    }
}

HK_NAMESPACE_END
