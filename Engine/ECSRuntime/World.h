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

    PhysicsInterface& GetPhysicsInterface() { return m_PhysicsInterface; }

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
    template <typename T, typename... Args>
    T* CreateSystem(Args&&... args)
    {
        m_EngineSystems.Add(std::make_unique<T>(this, std::forward<Args>(args)...));
        return static_cast<T*>(m_EngineSystems.Last().get());
    }

    void RunVariableTimeStepSystems(float timeStep);

    float m_Accumulator = 0.0f;

    GameFrame m_Frame;
    GameEvents m_GameEvents;

    PhysicsInterface m_PhysicsInterface;

    TVector<std::unique_ptr<EngineSystemECS>> m_EngineSystems;
    TVector<TRef<GameplaySystemECS>> m_GameplayVariableTimestepSystems;
    TVector<TRef<GameplaySystemECS>> m_GameplayFixedTimestepSystems;

    PhysicsSystem_ECS* m_PhysicsSystem;
    CharacterControllerSystem* m_CharacterControllerSystem;
    NodeMotionSystem* m_NodeMotionSystem;
    AnimationSystem* m_AnimationSystem;
    TransformSystem* m_TransformSystem;
    TeleportSystem* m_TeleportSystem;
    OneFrameRemoveSystem* m_OneFrameRemoveSystem;
    SkinningSystem_ECS* m_SkinningSystem;
    CameraSystem* m_CameraSystem;
    LightingSystem_ECS* m_LightingSystem;
    RenderSystem* m_RenderSystem;
    TRef<IEventHandler> m_EventHandler;
};

HK_NAMESPACE_END
