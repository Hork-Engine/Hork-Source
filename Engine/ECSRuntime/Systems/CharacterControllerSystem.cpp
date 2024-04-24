#include "CharacterControllerSystem.h"
#include "../World.h"
#include "../GameFrame.h"
#include "../CollisionModel.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/TransformComponent.h"

#include <Engine/Runtime/PhysicsModule.h>
#include <Engine/Runtime/DebugRenderer.h>
#include <Engine/Core/ConsoleVar.h>

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

bool IsNearZero(Float3 const& vec, float inMaxDistSq = 1.0e-12f)
{
    return vec.LengthSqr() < inMaxDistSq;
}

void CharacterControllerSystem::UpdateMovement(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::Required<CharacterControllerComponent>
        ::Required<TransformComponent>;

    float timeStep = frame.FixedTimeStep;
    auto gravity = m_PhysicsInterface.GetImpl().GetGravity();

    auto& tempAllocator = *PhysicsModule::Get().GetTempAllocator();

    for (Query::Iterator it(*m_World); it; it++)
    {
        CharacterControllerComponent* characterController = it.Get<CharacterControllerComponent>();
        TransformComponent* transform = it.Get<TransformComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            auto const& movementDir = characterController[i].MovementDirection;
            auto& desiredVelocity = characterController[i].DesiredVelocity;
            auto* character = characterController[i].m_pCharacter;

            // Smooth the player input
            desiredVelocity = 0.25f * movementDir * characterController[i].MoveSpeed + 0.75f * desiredVelocity;

            // True if the player intended to move
            characterController[i].m_bAllowSliding = !IsNearZero(movementDir);

            // Determine new basic velocity
            JPH::Vec3 current_vertical_velocity = JPH::Vec3(0, character->GetLinearVelocity().GetY(), 0);
            JPH::Vec3 ground_velocity = character->GetGroundVelocity();
            JPH::Vec3 new_velocity;
            if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
                && (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f)  // And not moving away from ground
            {
                // Assume velocity of ground when on ground
                new_velocity = ground_velocity;

                // Jump
                if (characterController[i].Jump)
                    new_velocity += JPH::Vec3(0, characterController[i].JumpSpeed, 0);
            }
            else
                new_velocity = current_vertical_velocity;

            // Gravity
            new_velocity += gravity * timeStep;

            // Player input
            new_velocity += ConvertVector(desiredVelocity);

            // Update character velocity
            character->SetLinearVelocity(new_velocity);

            // Stance switch
            //if (inSwitchStance)
            //    character->SetShape(character->GetShape() == mStandingShape? mCrouchingShape : mStandingShape, 1.5f * m_PhysicsInterface.GetPhysicsSettings().mPenetrationSlop, m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING), m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING), { }, *mTempAllocator);

            // Remember old position
            //JPH::Vec3 old_position = character->GetPosition();

            // Settings for our update function
            JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
            if (!characterController[i].EnableStickToFloor)
                update_settings.mStickToFloorStepDown = JPH::Vec3::sZero();
            if (!characterController[i].EnableWalkStairs)
                update_settings.mWalkStairsStepUp = JPH::Vec3::sZero();

            class BroadphaseLayerFilter final : public JPH::BroadPhaseLayerFilter
            {
            public:
                BroadphaseLayerFilter(uint32_t collisionMask) :
                    m_CollisionMask(collisionMask)
                {}

                uint32_t m_CollisionMask;

                bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
                {
                    return (HK_BIT(static_cast<uint8_t>(inLayer)) & m_CollisionMask) != 0;
                }
            };
            BroadphaseLayerFilter broadphaseFilter(HK_BIT(BroadphaseLayer::MOVING) | HK_BIT(BroadphaseLayer::NON_MOVING) | HK_BIT(BroadphaseLayer::CHARACTER_PROXY));

            ObjectLayerFilter layerFilter(m_PhysicsInterface.GetCollisionFilter(), characterController[i].GetCollisionGroup());

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

            BodyFilter bodyFilter;
            bodyFilter.m_IgoreBodyID = characterController[i].GetBodyId();

            // Update the character position
            character->ExtendedUpdate(timeStep,
                                      gravity,
                                      update_settings,
                                      broadphaseFilter,//m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                      layerFilter,//m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING),
                                      bodyFilter,
                                      {},
                                      tempAllocator);

            //m_PhysicsInterface.GetBodyInterface().MoveKinematic(characterController[i].GetBodyId(),
            //                                                 character->GetPosition(), character->GetRotation(),
            //                                                 frame.FixedTimeStep);

            //m_PhysicsInterface.GetImpl().GetBodyInterface().SetPositionAndRotation(characterController[i].GetBodyId(), character->GetPosition(), character->GetRotation(), JPH::EActivation::Activate);

            m_PhysicsInterface.GetImpl().GetBodyInterface().MoveKinematic(characterController[i].GetBodyId(), character->GetPosition(), character->GetRotation(), timeStep);

            transform[i].Position = ConvertVector(character->GetPosition());
            transform[i].Rotation = ConvertQuaternion(character->GetRotation());
        }
    }
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
            CharacterControllerComponent const* characterController = it.Get<CharacterControllerComponent>();
            WorldTransformComponent const* transform = it.Get<WorldTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                Float3x4 transform_matrix;
                transform_matrix.Compose(transform->Position[m_FrameIndex], transform->Rotation[m_FrameIndex].ToMatrix3x3());

                DrawShape(renderer, characterController[i].m_StandingShape.GetPtr(), transform_matrix);
            }
        }
    }
}

HK_NAMESPACE_END
