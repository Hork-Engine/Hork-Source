#include "World.h"

#include "Components/FinalTransformComponent.h"
#include "Components/ExperimentalComponents.h"

#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_InterpolateTransform("com_InterpolateTransform"s, "1"s);

World_ECS::World_ECS(ECS::WorldCreateInfo const& createInfo) :
    ECS::World(createInfo),
    m_PhysicsInterface(this)
{
    m_PhysicsSystem = CreateSystem<PhysicsSystem_ECS>(&m_GameEvents);
    m_CharacterControllerSystem = CreateSystem<CharacterControllerSystem>();
    m_NodeMotionSystem = CreateSystem<NodeMotionSystem>();
    m_AnimationSystem = CreateSystem<AnimationSystem>();
    m_TransformSystem = CreateSystem<TransformSystem>();
    m_CameraSystem = CreateSystem<CameraSystem>();
    m_RenderSystem = CreateSystem<RenderSystem>();
    m_TeleportSystem = CreateSystem<TeleportSystem>();
    m_OneFrameRemoveSystem = CreateSystem<OneFrameRemoveSystem>();
    m_SkinningSystem = CreateSystem<SkinningSystem_ECS>();
    m_LightingSystem = CreateSystem<LightingSystem_ECS>();
}

World_ECS::~World_ECS()
{
    GetCommandBuffer(0).DestroyEntities();

    ExecuteCommands();
}

void World_ECS::RunVariableTimeStepSystems(float timeStep)
{
    for (auto& system : m_GameplayVariableTimestepSystems)
        system->VariableTimestepUpdate(timeStep);
}

void World_ECS::RegisterGameplaySystem(GameplaySystemECS* gameplaySystem, GAMEPLAY_SYSTEM_EXECUTION execution)
{
    if (execution & GAMEPLAY_SYSTEM_VARIABLE_TIMESTEP)
        m_GameplayVariableTimestepSystems.EmplaceBack(gameplaySystem);

    if (execution & GAMEPLAY_SYSTEM_FIXED_TIMESTEP)
        m_GameplayFixedTimestepSystems.EmplaceBack(gameplaySystem);
}

void World_ECS::SetEventHandler(IEventHandler* eventHandler)
{
    m_EventHandler = eventHandler;
}

void World_ECS::Tick(float timeStep)
{
    if (bPaused)
    {
        m_Frame.RunningTime += timeStep;
        return;
    }

    //ExecuteCommands();

    const float fixedTimeStep = 1.0f / 60.0f;

    m_Frame.VariableTimeStep = timeStep;
    m_Frame.FixedTimeStep = fixedTimeStep;

    RunVariableTimeStepSystems(timeStep);

    m_Accumulator += timeStep;

    while (m_Accumulator >= fixedTimeStep)
    {
        m_Accumulator -= fixedTimeStep;

        m_Frame.PrevStateIndex = m_Frame.StateIndex;
        m_Frame.StateIndex = (m_Frame.StateIndex + 1) & 1;

        // FIXME: Call ExecuteCommands here?
        ExecuteCommands();

        m_OneFrameRemoveSystem->Update();

        m_TeleportSystem->Update(m_Frame);

        for (auto& system : m_GameplayFixedTimestepSystems)
            system->FixedTimestepUpdate(m_Frame);

        // Move / animate nodes
        m_NodeMotionSystem->Update(m_Frame);
        m_AnimationSystem->Update(m_Frame);

        // Recalc world transform
        m_TransformSystem->Update(m_Frame);

        // Update character controller
        m_CharacterControllerSystem->Update(m_Frame);

        // Update physics
        m_PhysicsSystem->Update(m_Frame);

        m_LightingSystem->UpdateBoundingBoxes(m_Frame);
        m_RenderSystem->UpdateBoundingBoxes(m_Frame);

        m_GameEvents.SwapReadWrite();
        if (m_EventHandler)
            m_EventHandler->ProcessEvents(m_GameEvents.GetEventsUnlocked());

        m_Frame.FixedFrameNum++;
        m_Frame.FixedTime = m_Frame.FixedFrameNum * fixedTimeStep;
    }

    m_Frame.Interpolate = m_Accumulator / fixedTimeStep;

    {
        using Query = ECS::Query<>
            ::Required<MeshComponent_ECS>
            ::ReadOnly<FinalTransformComponent>;

        for (Query::Iterator q(*this); q; q++)
        {
            MeshComponent_ECS* mesh = q.Get<MeshComponent_ECS>();
            FinalTransformComponent const* transform = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                mesh[i].TransformHistory = transform[i].ToMatrix();
            }
        }
    }
    {
        using Query = ECS::Query<>
            ::Required<ProceduralMeshComponent_ECS>
            ::ReadOnly<FinalTransformComponent>;

        for (Query::Iterator q(*this); q; q++)
        {
            ProceduralMeshComponent_ECS* mesh = q.Get<ProceduralMeshComponent_ECS>();
            FinalTransformComponent const* transform = q.Get<FinalTransformComponent>();

            for (int i = 0; i < q.Count(); i++)
            {
                mesh[i].TransformHistory = transform[i].ToMatrix();
            }
        }
    }

    if (com_InterpolateTransform)
    {
        m_TransformSystem->InterpolateTransformState(m_Frame);
    }
    else
    {
        m_TransformSystem->CopyTransformState(m_Frame);
    }

    m_LightingSystem->Update(m_Frame);

    m_SkinningSystem->Update(m_Frame);
    m_CameraSystem->Update();

    m_Frame.VariableTime += timeStep;
    m_Frame.FrameNum++;

    m_Frame.RunningTime += timeStep;

    //LOG("TIME: Variable {}\tFixed {}\tDiff {}\n", m_Frame.VariableTime, m_Frame.FixedTime, Math::Abs(m_Frame.VariableTime - m_Frame.FixedTime));
    //LOG("TIME: Diff {} ms\n", (m_Frame.VariableTime - m_Frame.FixedTime) * 1000);
}

void World_ECS::DrawDebug(DebugRenderer& renderer)
{
    for (auto& system : m_EngineSystems)
        system->DrawDebug(renderer);

    for (auto& system : m_GameplayVariableTimestepSystems)
        system->DrawDebug(renderer);

    for (auto& system : m_GameplayFixedTimestepSystems)
        system->DrawDebug(renderer);
}

void World_ECS::AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    m_RenderSystem->AddDirectionalLight(rd, frameData);
}

void World_ECS::AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    m_RenderSystem->AddDrawables(rd, frameData);
}

HK_NAMESPACE_END
