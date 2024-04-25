#include "World.h"

#include <Engine/World/Modules/BehaviorTree/Systems/BehaviorTreeSystem.h>
#include <Engine/World/Modules/Transform/Systems/NodeMotionSystem.h>
#include <Engine/World/Modules/Transform/Systems/TransformSystem.h>
#include <Engine/World/Modules/Transform/Systems/TransformHistorySystem.h>
#include <Engine/World/Modules/Render/Systems/RenderSystem.h>
#include <Engine/World/Modules/Physics/Systems/PhysicsSystem.h>
#include <Engine/World/Modules/Physics/Systems/CharacterControllerSystem.h>
#include <Engine/World/Modules/Physics/Systems/TeleportSystem.h>
#include <Engine/World/Modules/Skeleton/Systems/SkinningSystem.h>
#include <Engine/World/Modules/Render/Systems/LightingSystem.h>
#include <Engine/World/Modules/Logic/Systems/EntityDestroySystem.h>

#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_InterpolateTransform("com_InterpolateTransform"s, "1"s);

World::World(ECS::WorldCreateInfo const& createInfo) :
    ECS::World(createInfo),
    m_PhysicsInterface(this)
{
    m_PhysicsSystem = CreateSystem<PhysicsSystem>(&m_GameEvents);
    m_CharacterControllerSystem = CreateSystem<CharacterControllerSystem>();
    m_BehaviorTreeSystem = CreateSystem<BehaviorTreeSystem>();
    m_NodeMotionSystem = CreateSystem<NodeMotionSystem>();
    m_TransformSystem = CreateSystem<TransformSystem>();
    m_TransformHistorySystem = CreateSystem<TransformHistorySystem>();
    m_RenderSystem = CreateSystem<RenderSystem>();
    m_TeleportSystem = CreateSystem<TeleportSystem>();
    m_SkinningSystem = CreateSystem<SkinningSystem_ECS>();
    m_LightingSystem = CreateSystem<LightingSystem>();
    m_EntityDestroySystem = CreateSystem<EntityDestroySystem>();
}

World::~World()
{
    GetCommandBuffer(0).DestroyEntities();

    ExecuteCommands();
}

GameplaySystemECS* World::RegisterGameplaySystem(GameplaySystemECS* gameplaySystem, GAMEPLAY_SYSTEM_EXECUTION execution)
{
    if (execution & GAMEPLAY_SYSTEM_VARIABLE_UPDATE)
        m_GameplayVariableUpdateSystems.Add(gameplaySystem);

    if (execution & GAMEPLAY_SYSTEM_FIXED_UPDATE)
        m_GameplayFixedUpdateSystems.Add(gameplaySystem);

    if (execution & GAMEPLAY_SYSTEM_POST_PHYSICS_UPDATE)
        m_GameplayPostPhysicsSystems.Add(gameplaySystem);

    if (execution & GAMEPLAY_SYSTEM_LATE_UPDATE)
        m_GameplayLateUpdateSystems.Add(gameplaySystem);

    m_AllGameplaySystems.EmplaceBack(gameplaySystem);

    return gameplaySystem;
}

void World::SetEventHandler(IEventHandler* eventHandler)
{
    m_EventHandler = eventHandler;
}

void World::AddRequest(WORLD_REQUEST request)
{
    m_Requests.Add(request);
}

void World::Tick(float timeStep)
{
    ProcessRequests();

    const float fixedTimeStep = 1.0f / 60.0f;

    m_Frame.VariableTimeStep = timeStep;
    m_Frame.FixedTimeStep = fixedTimeStep;

    for (auto& system : m_GameplayVariableUpdateSystems)
    {
        if (!m_bPaused || system->bTickEvenWhenPaused)
            system->VariableUpdate(timeStep);
    }

    m_Accumulator += timeStep;

    while (m_Accumulator >= fixedTimeStep)
    {
        m_Accumulator -= fixedTimeStep;

        m_Frame.PrevStateIndex = m_Frame.StateIndex;
        m_Frame.StateIndex = (m_Frame.StateIndex + 1) & 1;

        // FIXME: Call ExecuteCommands here?
        ExecuteCommands();

        if (!m_bPaused)
            m_EntityDestroySystem->Update();

        if (!m_bPaused)
            m_TeleportSystem->Update(m_Frame);

        for (auto& system : m_GameplayFixedUpdateSystems)
        {
            if (!m_bPaused || system->bTickEvenWhenPaused)
                system->FixedUpdate(m_Frame);
        }

        if (!m_bPaused)
            m_BehaviorTreeSystem->Update(m_Frame);

        // Move / animate nodes
        if (!m_bPaused)
            m_NodeMotionSystem->Update(m_Frame);

        // Update skeleton poses and sockets
        m_SkinningSystem->UpdatePoses(m_Frame);

        // Copy socket transform to transform component
        m_SkinningSystem->UpdateSockets();

        // Recalc world transform
        m_TransformSystem->Update(m_Frame);

        m_PhysicsSystem->AddAndRemoveBodies(m_Frame);

        // Update character controller
        m_CharacterControllerSystem->Update(m_Frame);

        // Update physics
        m_PhysicsSystem->Update(m_Frame);

        for (auto& system : m_GameplayPostPhysicsSystems)
        {
            if (!m_bPaused || system->bTickEvenWhenPaused)
                system->PostPhysicsUpdate(m_Frame);
        }

        m_LightingSystem->UpdateBoundingBoxes(m_Frame);
        m_RenderSystem->UpdateBoundingBoxes(m_Frame);

        m_GameEvents.SwapReadWrite();
        if (m_EventHandler)
            m_EventHandler->ProcessEvents(m_GameEvents.GetEventsUnlocked());

        if (!m_bPaused)
        {
            m_Frame.FixedFrameNum++;
            m_Frame.FixedTime = m_Frame.FixedFrameNum * fixedTimeStep;
        }
    }

    m_Frame.Interpolate = m_Accumulator / fixedTimeStep;

    m_TransformHistorySystem->Update(m_Frame);

    if (com_InterpolateTransform)
    {
        m_TransformSystem->InterpolateTransformState(m_Frame);
    }
    else
    {
        m_TransformSystem->CopyTransformState(m_Frame);
    }

    m_LightingSystem->Update(m_Frame);

    // TODO:
    //m_SkinningSystem->InterpolatePoses(m_Frame);
    m_SkinningSystem->UpdateSkins();

    for (auto& system : m_GameplayLateUpdateSystems)
    {
        if (!m_bPaused || system->bTickEvenWhenPaused)
            system->LateUpdate(timeStep);
    }

    if (!m_bPaused)
        m_Frame.VariableTime += timeStep;
    m_Frame.FrameNum++;

    m_Frame.RunningTime += timeStep;

    //LOG("TIME: Variable {}\tFixed {}\tDiff {}\n", m_Frame.VariableTime, m_Frame.FixedTime, Math::Abs(m_Frame.VariableTime - m_Frame.FixedTime));
    //LOG("TIME: Diff {} ms\n", (m_Frame.VariableTime - m_Frame.FixedTime) * 1000);
}

void World::ProcessRequests()
{
    for (WORLD_REQUEST request : m_Requests)
    {
        switch (request)
        {
            case WORLD_REQUEST_PAUSE:
                m_bPaused = true;
                break;
            case WORLD_REQUEST_UNPAUSE:
                m_bPaused = false;
                break;
        }
    }
    m_Requests.Clear();
}

void World::DrawDebug(DebugRenderer& renderer)
{
    for (auto& system : m_EngineSystems)
        system->DrawDebug(renderer);

    for (auto& system : m_AllGameplaySystems)
        system->DrawDebug(renderer);
}

void World::AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    m_RenderSystem->AddDirectionalLight(rd, frameData);
}

void World::AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    m_RenderSystem->AddDrawables(rd, frameData);
}

HK_NAMESPACE_END
