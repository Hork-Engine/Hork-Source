#pragma once

#include "Systems/GameplaySystem.h"
#include "Systems/NodeMotionSystem.h"
#include "Systems/AnimationSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/CharacterControllerSystem.h"
#include "Systems/TeleportSystem.h"
#include "Systems/OneFrameRemoveSystem.h"
#include "Systems/SkinningSystem.h"
#include "Systems/LightingSystem.h"
#include "GameFrame.h"
#include "GameEvents.h"
#include "Utils.h"

HK_NAMESPACE_BEGIN

class World_ECS : public ECS::World
{
public:
    World_ECS(ECS::WorldCreateInfo const& createInfo);
    ~World_ECS();

    bool bPaused = false;

    template <typename T>
    void RegisterGameplaySystem(GAMEPLAY_SYSTEM_EXECUTION execution)
    {
        RegisterGameplaySystem(MakeRef<T>(this), execution);
    }

    void RegisterGameplaySystem(GameplaySystemECS* gameplaySystem, GAMEPLAY_SYSTEM_EXECUTION execution);

    void SetEventHandler(IEventHandler* eventHandler);

    void Tick(float timeStep);

    void DrawDebug(DebugRenderer& renderer);

    void AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData);
    void AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData);

    GameFrame const& GetFrame() const { return m_Frame; }

private:
    void RunVariableTimeStepSystems(float timeStep);

    float m_Accumulator = 0.0f;

    GameFrame m_Frame;
    GameEvents m_GameEvents;

    PhysicsInterface m_PhysicsInterface;

    TVector<TRef<GameplaySystemECS>> m_GameplayVariableTimestepSystems;
    TVector<TRef<GameplaySystemECS>> m_GameplayFixedTimestepSystems;

    std::unique_ptr<PhysicsSystem_ECS> m_PhysicsSystem;
    std::unique_ptr<CharacterControllerSystem> m_CharacterControllerSystem;
    std::unique_ptr<NodeMotionSystem> m_NodeMotionSystem;
    std::unique_ptr<AnimationSystem> m_AnimationSystem;
    std::unique_ptr<TransformSystem> m_TransformSystem;
    std::unique_ptr<TeleportSystem> m_TeleportSystem;
    std::unique_ptr<OneFrameRemoveSystem> m_OneFrameRemoveSystem;
    std::unique_ptr<SkinningSystem_ECS> m_SkinningSystem;
    std::unique_ptr<CameraSystem> m_CameraSystem;
    std::unique_ptr<LightingSystem_ECS> m_LightingSystem;
    std::unique_ptr<RenderSystem> m_RenderSystem;
    TRef<IEventHandler> m_EventHandler;
};

HK_NAMESPACE_END
