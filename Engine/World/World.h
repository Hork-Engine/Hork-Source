#pragma once

#include <Engine/World/Common/EngineSystem.h>
#include <Engine/World/Common/GameplaySystem.h>
#include <Engine/World/Common/GameFrame.h>
#include <Engine/World/Common/GameEvents.h>
#include <Engine/World/Modules/Physics/PhysicsInterface.h>

HK_NAMESPACE_BEGIN

enum WORLD_REQUEST
{
    WORLD_REQUEST_PAUSE,
    WORLD_REQUEST_UNPAUSE
};

class World : public ECS::World
{
public:
    World(ECS::WorldCreateInfo const& createInfo);
    ~World();

    bool bTick = true;

    PhysicsInterface& GetPhysicsInterface() { return m_PhysicsInterface; }

    template <typename T>
    T* RegisterGameplaySystem(GAMEPLAY_SYSTEM_EXECUTION execution)
    {
        return static_cast<T*>(RegisterGameplaySystem(MakeRef<T>(this), execution));
    }

    GameplaySystemECS* RegisterGameplaySystem(GameplaySystemECS* gameplaySystem, GAMEPLAY_SYSTEM_EXECUTION execution);

    void SetEventHandler(IEventHandler* eventHandler);

    void AddRequest(WORLD_REQUEST request);

    void Tick(float timeStep);

    void DrawDebug(DebugRenderer& renderer);

    GameFrame const& GetFrame() const { return m_Frame; }

    bool IsPaused() const { return m_bPaused; }

    // TODO: Remove this:
    void AddDirectionalLight(struct RenderFrontendDef& rd, struct RenderFrameData& frameData);
    void AddDrawables(struct RenderFrontendDef& rd, struct RenderFrameData& frameData);

private:
    template <typename T, typename... Args>
    T* CreateSystem(Args&&... args)
    {
        m_EngineSystems.Add(MakeUnique<T>(this, std::forward<Args>(args)...));
        return static_cast<T*>(m_EngineSystems.Last().RawPtr());
    }

    void ProcessRequests();

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

    TVector<WORLD_REQUEST> m_Requests;

    bool m_bPaused = false;
};

HK_NAMESPACE_END
