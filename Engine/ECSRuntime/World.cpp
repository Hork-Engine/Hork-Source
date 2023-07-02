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
    m_PlayerControlSystem = std::make_unique<PlayerControlSystem>(this);
    m_PhysicsSystem = std::make_unique<PhysicsSystem_ECS>(this, m_PhysicsInterface, & m_GameEvents);
    m_CharacterControllerSystem = std::make_unique<CharacterControllerSystem>(this, m_PhysicsInterface);
    m_NodeMotionSystem = std::make_unique<NodeMotionSystem>(this);
    m_AnimationSystem = std::make_unique<AnimationSystem>(this);
    m_TransformSystem = std::make_unique<TransformSystem>(this);
    m_CameraSystem = std::make_unique<CameraSystem>(this);
    m_RenderSystem = std::make_unique<RenderSystem>(this);
    m_TeleportSystem = std::make_unique<TeleportSystem>(this, m_PhysicsInterface);
    m_SpawnSystem = std::make_unique<SpawnSystem>(this);
    m_OneFrameRemoveSystem = std::make_unique<OneFrameRemoveSystem>(this);
    m_SkinningSystem = std::make_unique<SkinningSystem_ECS>(this);
    m_LightingSystem = std::make_unique<LightingSystem_ECS>(this);

    m_EventHandler = std::make_unique<EventHandler>(this, m_PhysicsInterface);
}

World_ECS::~World_ECS()
{
    GetCommandBuffer(0).DestroyEntities();

    ExecuteCommands();
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

    m_PlayerControlSystem->Update(timeStep);

    m_Accumulator += timeStep;

    while (m_Accumulator >= fixedTimeStep)
    {
        m_Accumulator -= fixedTimeStep;

        m_Frame.PrevStateIndex = m_Frame.StateIndex;
        m_Frame.StateIndex = (m_Frame.StateIndex + 1) & 1;

        // FIXME: Call ExecuteCommands here?
        ExecuteCommands();

        m_OneFrameRemoveSystem->Update();

        m_SpawnSystem->Update(m_Frame);

        m_TeleportSystem->Update(m_Frame);

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
    m_PhysicsSystem->DrawDebug(renderer);
    m_CharacterControllerSystem->DrawDebug(renderer);
    m_SkinningSystem->DrawDebug(renderer);
    m_LightingSystem->DrawDebug(renderer);
    m_RenderSystem->DrawDebug(renderer);
}

void World_ECS::AddDirectionalLight(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    m_RenderSystem->AddDirectionalLight(rd, frameData);
}

void World_ECS::AddDrawables(RenderFrontendDef& rd, RenderFrameData& frameData)
{
    m_RenderSystem->AddDrawables(rd, frameData);
}

#if 0
#    define REGISTER_VISITOR(Type)                                                                    \
        m_ComponentView[ECS::Component<Type>::Id] = [this](ECS::EntityHandle handle, uint8_t* data) { \
            Visit(handle, *(Type*)data);                                                              \
        }
class EntityBrowser
{
public:
    EntityBrowser(ECS::World& world, ECS::EntityHandle handle)
    {
        std::unordered_map<ECS::ComponentTypeId, std::function<void(ECS::EntityHandle, uint8_t*)>> m_ComponentView;

        REGISTER_VISITOR(TestComponent);

        for (ECS::World::ComponentIterator it(world, handle); it; it++)
        {
            uint8_t* data = it.GetData();
            ECS::ComponentTypeId componentTID = it.GetTypeId();

            m_ComponentView[componentTID](handle, data);
        }
    }
    void Visit(ECS::EntityHandle handle, TestComponent& c)
    {
    }
};

#endif


HK_NAMESPACE_END
