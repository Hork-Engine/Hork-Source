#include "CharacterControllerSystem.h"
#include "../PhysicsModule.h"
#include "../CollisionModel.h"

#include <Engine/Core/ConsoleVar.h>

#include <Engine/World/World.h>
#include <Engine/World/Common/GameFrame.h>

#include <Engine/World/Common/DebugRenderer.h>
#include <Engine/World/Modules/Transform/Components/WorldTransformComponent.h>
#include <Engine/World/Modules/Transform/Components/TransformComponent.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCharacterController("com_DrawCharacterController"s, "0"s);

CharacterControllerSystem::CharacterControllerSystem(World* world) :
    m_World(world),
    m_PhysicsInterface(world->GetPhysicsInterface())
{
    m_World->AddEventHandler<ECS::Event::OnComponentAdded<CharacterControllerComponent>>(this);
    m_World->AddEventHandler<ECS::Event::OnComponentRemoved<CharacterControllerComponent>>(this);
}

CharacterControllerSystem::~CharacterControllerSystem()
{
    m_World->RemoveHandler(this);
}

void CharacterControllerSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<CharacterControllerComponent>& event)
{
}

void CharacterControllerSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<CharacterControllerComponent>& event)
{
    delete event.Component().m_pCharacter;
}

void CharacterControllerSystem::Update(GameFrame const& frame)
{
    m_FrameIndex = frame.StateIndex;

    // On pre physics update:
    if (!m_World->IsPaused())
        UpdateMovement(frame);
}

void CharacterControllerSystem::UpdateMovement(GameFrame const& frame)
{
    using Query = ECS::Query<>::Required<CharacterControllerComponent>::Required<TransformComponent>;

    float time_step = frame.FixedTimeStep;

    for (Query::Iterator it(*m_World); it; it++)
    {
        CharacterControllerComponent* character = it.Get<CharacterControllerComponent>();
        TransformComponent* transform = it.Get<TransformComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            UpdateCharacter(character[i], transform[i], time_step);
        }
    }
}

bool IsNearZero(Float3 const& vec, float inMaxDistSq = 1.0e-12f)
{
    return vec.LengthSqr() < inMaxDistSq;
}

void CharacterControllerSystem::UpdateCharacter(CharacterControllerComponent& character, TransformComponent& transform, float time_step)
{
    auto& temp_allocator = *PhysicsModule::Get().GetTempAllocator();

    auto gravity = m_PhysicsInterface.GetImpl().GetGravity();
    auto* phys_character = character.m_pCharacter;

    // Smooth the player input
    character.DesiredVelocity = 0.25f * character.MovementDirection * character.MoveSpeed + 0.75f * character.DesiredVelocity;

    // True if the player intended to move
    character.m_bAllowSliding = !IsNearZero(character.MovementDirection);

    // Determine new basic velocity
    JPH::Vec3 current_vertical_velocity = JPH::Vec3(0, phys_character->GetLinearVelocity().GetY(), 0);
    JPH::Vec3 ground_velocity = phys_character->GetGroundVelocity();
    JPH::Vec3 new_velocity;
    if (phys_character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
        && (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f)       // And not moving away from ground
    {
        // Assume velocity of ground when on ground
        new_velocity = ground_velocity;

        // Jump
        if (character.Jump)
            new_velocity += JPH::Vec3(0, character.JumpSpeed, 0);
    }
    else
        new_velocity = current_vertical_velocity;

    // Gravity
    new_velocity += gravity * time_step;

    // Player input
    new_velocity += ConvertVector(character.DesiredVelocity);

    // Update character velocity
    phys_character->SetLinearVelocity(new_velocity);

    // Stance switch
    //if (inSwitchStance)
    //    phys_character->SetShape(phys_character->GetShape() == mStandingShape? mCrouchingShape : mStandingShape, 1.5f * m_PhysicsInterface.GetPhysicsSettings().mPenetrationSlop, m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING), m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING), { }, *mTempAllocator);

    // Settings for our update function
    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    if (!character.EnableStickToFloor)
        update_settings.mStickToFloorStepDown = JPH::Vec3::sZero();
    if (!character.EnableWalkStairs)
        update_settings.mWalkStairsStepUp = JPH::Vec3::sZero();

    class BroadphaseLayerFilter final : public JPH::BroadPhaseLayerFilter
    {
    public:
        BroadphaseLayerFilter(uint32_t inCollisionMask) :
            m_CollisionMask(inCollisionMask)
        {}

        uint32_t m_CollisionMask;

        bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
        {
            return (HK_BIT(static_cast<uint8_t>(inLayer)) & m_CollisionMask) != 0;
        }
    };
    BroadphaseLayerFilter broadphase_filter(HK_BIT(BroadphaseLayer::MOVING) | HK_BIT(BroadphaseLayer::NON_MOVING) | HK_BIT(BroadphaseLayer::CHARACTER_PROXY));

    ObjectLayerFilter layer_filter(m_PhysicsInterface.GetCollisionFilter(), character.GetCollisionGroup());

    class BodyFilter final : public JPH::BodyFilter
    {
    public:
        //uint32_t m_ObjectFilterIDToIgnore = 0xFFFFFFFF - 1;

        //BodyFilter(uint32_t uiBodyFilterIdToIgnore = 0xFFFFFFFF - 1) :
        //    m_ObjectFilterIDToIgnore(uiBodyFilterIdToIgnore)
        //{
        //}

        //void ClearFilter()
        //{
        //    m_ObjectFilterIDToIgnore = 0xFFFFFFFF - 1;
        //}

        JPH::BodyID m_IgoreBodyID;

        bool ShouldCollideLocked(const JPH::Body& body) const override
        {
            return body.GetID() != m_IgoreBodyID;
        }
    };

    BodyFilter body_filter;
    body_filter.m_IgoreBodyID = character.GetBodyId();

    // Update the character position
    phys_character->ExtendedUpdate(time_step,
                                   gravity,
                                   update_settings,
                                   broadphase_filter, //m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                   layer_filter,      //m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING),
                                   body_filter,
                                   {},
                                   temp_allocator);

    m_PhysicsInterface.GetImpl().GetBodyInterface().MoveKinematic(character.GetBodyId(), phys_character->GetPosition(), phys_character->GetRotation(), time_step);

    transform.Position = ConvertVector(phys_character->GetPosition());
    transform.Rotation = ConvertQuaternion(phys_character->GetRotation());
}

void CharacterControllerSystem::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawCharacterController)
    {
        using Query = ECS::Query<>
            ::ReadOnly<CharacterControllerComponent>
            ::ReadOnly<WorldTransformComponent>;

        renderer.SetColor(Color4(1, 1, 0, 1));

        for (Query::Iterator it(*m_World); it; it++)
        {
            CharacterControllerComponent const* character = it.Get<CharacterControllerComponent>();
            WorldTransformComponent const* transform = it.Get<WorldTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                Float3x4 transform_matrix;
                transform_matrix.Compose(transform->Position[m_FrameIndex], transform->Rotation[m_FrameIndex].ToMatrix3x3());

                DrawShape(renderer, character[i].m_pCharacter->GetShape(), transform_matrix);
            }
        }
    }
}

HK_NAMESPACE_END
