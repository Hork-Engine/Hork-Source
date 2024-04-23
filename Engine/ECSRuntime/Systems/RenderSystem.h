#pragma once

#include "EngineSystem.h"

#include "../Components/RenderTransformComponent.h"
#include "../Components/MeshComponent.h"
#include "../Components/TerrainComponent.h"
#include "../Components/ShadowCastComponent.h"
#include "../Components/DirectionalLightComponent.h"
#include "../Components/ExperimentalComponents.h"

#include "../Resources/ResourceManager.h"

HK_NAMESPACE_BEGIN

class FrameLoop;
struct GameFrame;

class RenderSystem : public EngineSystemECS
{
public:
    RenderSystem(ECS::World* world);
    ~RenderSystem();

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<MeshComponent_ECS> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<MeshComponent_ECS> const& event);

    void UpdateBoundingBoxes(GameFrame const& frame);

    void DrawDebug(DebugRenderer& renderer) override;

    void AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData);
    void AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData);

private:
    void AddMesh(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, MeshComponent_ECS const& mesh, Float3x4 const* transformHistory, SkeletonPose* pose, bool bMovable);
    void AddProceduralMesh(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, ProceduralMeshComponent_ECS const& mesh, Float3x4 const* transformHistory, bool bMovable);
    void AddTerrain(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, TerrainComponent_ECS const& terrainComponent, Float3x4 const* transformHistory, bool bMovable);
    void AddMeshShadow(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, MeshComponent_ECS const& mesh, SkeletonPose* pose, ShadowCastComponent const& shadow, LightShadowmap* shadowmap);
    void AddProceduralMeshShadow(RenderFrontendDef& rd, RenderFrameData& frameData, RenderTransformComponent const& transform, ProceduralMeshComponent_ECS const& mesh, ShadowCastComponent const& shadow, LightShadowmap* shadowmap);
    void AddShadowmapCascades(DirectionalLightComponent const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* StreamedMemory, RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades);
    void AddDirectionalLightShadows(RenderFrontendDef& rd, RenderFrameData& frameData, LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef);
    void SortShadowInstances(RenderFrameData& frameData, LightShadowmap const* shadowMap);

    MaterialFrameData* GetMaterialFrameData(MaterialInstance* materialInstance, FrameLoop* frameLoop, int frameNumber);

    ECS::World* m_World;
};

HK_NAMESPACE_END
