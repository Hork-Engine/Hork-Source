#pragma once

#include "Systems/EngineSystem.h"
#include "Systems/GameplaySystem.h"

#include "GameFrame.h"
#include "GameEvents.h"
#include "PhysicsInterface.h"

HK_NAMESPACE_BEGIN

class World : public ECS::World
{
public:
    World(ECS::WorldCreateInfo const& createInfo);
    ~World();

    bool bPaused = false;
    bool bTick = true;

    PhysicsInterface& GetPhysicsInterface() { return m_PhysicsInterface; }

    template <typename T>
    T* RegisterGameplaySystem(GAMEPLAY_SYSTEM_EXECUTION execution)
    {
        return static_cast<T*>(RegisterGameplaySystem(MakeRef<T>(this), execution));
    }

    GameplaySystemECS* RegisterGameplaySystem(GameplaySystemECS* gameplaySystem, GAMEPLAY_SYSTEM_EXECUTION execution);

    void SetEventHandler(IEventHandler* eventHandler);

    void Tick(float timeStep);

    void DrawDebug(DebugRenderer& renderer);

    void AddDirectionalLight(struct RenderFrontendDef& rd, struct RenderFrameData& frameData);
    void AddDrawables(struct RenderFrontendDef& rd, struct RenderFrameData& frameData);

    GameFrame const& GetFrame() const { return m_Frame; }

private:
    template <typename T, typename... Args>
    T* CreateSystem(Args&&... args)
    {
        m_EngineSystems.Add(MakeUnique<T>(this, std::forward<Args>(args)...));
        return static_cast<T*>(m_EngineSystems.Last().RawPtr());
    }

    float m_Accumulator = 0.0f;

    GameFrame m_Frame;
    GameEvents m_GameEvents;

    PhysicsInterface m_PhysicsInterface;

    TVector<TUniqueRef<EngineSystemECS>> m_EngineSystems;
    TVector<GameplaySystemECS*> m_GameplayVariableUpdateSystems;
    TVector<GameplaySystemECS*> m_GameplayFixedUpdateSystems;
    TVector<GameplaySystemECS*> m_GameplayPostPhysicsSystems;
    TVector<GameplaySystemECS*> m_GameplayLateUpdateSystems;
    TVector<TRef<GameplaySystemECS>> m_AllGameplaySystems;

    class PhysicsSystem* m_PhysicsSystem;
    class CharacterControllerSystem* m_CharacterControllerSystem;
    class BehaviorTreeSystem* m_BehaviorTreeSystem;
    class NodeMotionSystem* m_NodeMotionSystem;
    class TransformSystem* m_TransformSystem;
    class TransformHistorySystem* m_TransformHistorySystem;
    class TeleportSystem* m_TeleportSystem;
    class SkinningSystem_ECS* m_SkinningSystem;
    class LightingSystem* m_LightingSystem;
    class EntityDestroySystem* m_EntityDestroySystem;
    class RenderSystem* m_RenderSystem;
    TRef<IEventHandler> m_EventHandler;
};

HK_NAMESPACE_END
