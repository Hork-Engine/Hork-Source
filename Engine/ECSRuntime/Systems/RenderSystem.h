#pragma once

#include <Engine/ECS/ECS.h>

#include "../Components/FinalTransformComponent.h"
#include "../Components/ExperimentalComponents.h"

#include "../Resources/ResourceManager.h"

HK_NAMESPACE_BEGIN

class FrameLoop;
struct GameFrame;

class RenderSystem
{
public:
    RenderSystem(ECS::World* world);
    ~RenderSystem();

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<MeshComponent_ECS> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<MeshComponent_ECS> const& event);

    void UpdateBoundingBoxes(GameFrame const& frame);

    void DrawDebug(DebugRenderer& renderer);

    void AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData);
    void AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData);

private:
    void AddMesh(RenderFrontendDef& rd, RenderFrameData& frameData, FinalTransformComponent const& transform, MeshComponent_ECS const& mesh, SkeletonPose* pose, bool bMovable);
    void AddProceduralMesh(RenderFrontendDef& rd, RenderFrameData& frameData, FinalTransformComponent const& transform, ProceduralMeshComponent_ECS const& mesh, bool bMovable);
    void AddMeshShadow(RenderFrontendDef& rd, RenderFrameData& frameData, FinalTransformComponent const& transform, MeshComponent_ECS const& mesh, SkeletonPose* pose, ShadowCastComponent const& shadow, LightShadowmap* shadowmap);
    void AddProceduralMeshShadow(RenderFrontendDef& rd, RenderFrameData& frameData, FinalTransformComponent const& transform, ProceduralMeshComponent_ECS const& mesh, ShadowCastComponent const& shadow, LightShadowmap* shadowmap);
    void AddShadowmapCascades(DirectionalLightComponent_ECS const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* StreamedMemory, RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades);
    void AddDirectionalLightShadows(RenderFrontendDef& rd, RenderFrameData& frameData, LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef);
    void SortShadowInstances(RenderFrameData& frameData, LightShadowmap const* shadowMap);

    MaterialFrameData* GetMaterialFrameData(MaterialInstance* materialInstance, FrameLoop* frameLoop, int frameNumber);

    ECS::World* m_World;
};

HK_NAMESPACE_END
