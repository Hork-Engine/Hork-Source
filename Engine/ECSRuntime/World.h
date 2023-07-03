#pragma once

#include "Systems/EngineSystem.h"
#include "Systems/GameplaySystem.h"

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

    void AddDirectionalLight(struct RenderFrontendDef& rd, RenderFrameData& frameData);
    void AddDrawables(struct RenderFrontendDef& rd, RenderFrameData& frameData);

    GameFrame const& GetFrame() const { return m_Frame; }

private:
    template <typename T, typename... Args>
    T* CreateSystem(Args&&... args)
    {
        m_EngineSystems.Add(MakeUnique<T>(this, std::forward<Args>(args)...));
        return static_cast<T*>(m_EngineSystems.Last().GetObject());
    }

    void RunVariableTimeStepSystems(float timeStep);

    float m_Accumulator = 0.0f;

    GameFrame m_Frame;
    GameEvents m_GameEvents;

    PhysicsInterface m_PhysicsInterface;

    TVector<TUniqueRef<EngineSystemECS>> m_EngineSystems;
    TVector<TRef<GameplaySystemECS>> m_GameplayVariableTimestepSystems;
    TVector<TRef<GameplaySystemECS>> m_GameplayFixedTimestepSystems;
    TVector<TRef<GameplaySystemECS>> m_GameplayPostPhysicsSystems;

    class PhysicsSystem_ECS* m_PhysicsSystem;
    class CharacterControllerSystem* m_CharacterControllerSystem;
    class BehaviorTreeSystem* m_BehaviorTreeSystem;
    class NodeMotionSystem* m_NodeMotionSystem;
    class TransformSystem* m_TransformSystem;
    class TransformHistorySystem* m_TransformHistorySystem;
    class TeleportSystem* m_TeleportSystem;
    class OneFrameRemoveSystem* m_OneFrameRemoveSystem;
    class SkinningSystem_ECS* m_SkinningSystem;
    class CameraSystem* m_CameraSystem;
    class LightingSystem_ECS* m_LightingSystem;
    class RenderSystem* m_RenderSystem;
    TRef<IEventHandler> m_EventHandler;
};

HK_NAMESPACE_END
