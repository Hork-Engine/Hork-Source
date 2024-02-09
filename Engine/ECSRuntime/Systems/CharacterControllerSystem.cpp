#include "CharacterControllerSystem.h"
#include "../World.h"
#include "../GameFrame.h"
#include "../Components/WorldTransformComponent.h"
#include "../Components/FinalTransformComponent.h"
#include "../Components/TransformComponent.h"

#include <Engine/Runtime/DebugRenderer.h>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <Engine/Core/ConsoleVar.h>

#include <Engine/Runtime/PhysicsModule.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCharacterController("com_DrawCharacterController"s, "0"s);

class CharacterControllerImpl : public JPH::CharacterVirtual, public JPH::CharacterContactListener
{
public:
    JPH_OVERRIDE_NEW_DELETE

    ECS::EntityHandle m_EntityHandle;
    CharacterControllerSystem* m_System;

    CharacterControllerImpl(ECS::EntityHandle entityHandle, CharacterControllerSystem* system, const JPH::CharacterVirtualSettings* inSettings, JPH::Vec3Arg inPosition, JPH::QuatArg inRotation, JPH::PhysicsSystem* inPhysicsSystem) :
        JPH::CharacterVirtual(inSettings, inPosition, inRotation, inPhysicsSystem),
        m_EntityHandle(entityHandle),
        m_System(system)
    {
        SetListener(this);
    }

    void OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
    {
        m_System->OnContactAdded(m_EntityHandle, inCharacter, inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, ioSettings);
    }

    void OnContactSolve(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
    {
        m_System->OnContactSolve(m_EntityHandle, inCharacter, inBodyID2, inSubShapeID2, inContactPosition, inContactNormal, inContactVelocity, inContactMaterial, inCharacterVelocity, ioNewCharacterVelocity);
    }
};

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

    DestroyCharacters();
}

void CharacterControllerSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<CharacterControllerComponent>& event)
{
    m_PendingAddCharacters.Add(event.GetEntity());
}

void CharacterControllerSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<CharacterControllerComponent>& event)
{
    auto index = m_PendingAddCharacters.IndexOf(event.GetEntity());
    if (index != Core::NPOS)
        m_PendingAddCharacters.Remove(index);

    if (event.Component().m_pCharacter)
    {
        CharacterData ch;
        ch.pCharacter = event.Component().m_pCharacter;
        ch.BodyID = event.Component().m_BodyId;
        m_PendingRemoveCharacters.Add(ch);
    }
}

void CharacterControllerSystem::OnContactAdded(ECS::EntityHandle entityHandle, const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
{
    LOG("CharacterControllerSystem::OnContactAdded {}\n", inBodyID2.GetIndexAndSequenceNumber());

    ECS::EntityView view = m_World->GetEntityView(entityHandle);

    CharacterControllerComponent* component = view.GetComponent<CharacterControllerComponent>();
    if (component)
    {
#if 0
        // Dynamic boxes on the ramp go through all permutations
        JPH::Array<JPH::BodyID>::const_iterator i = std::find(mRampBlocks.begin(), mRampBlocks.end(), inBodyID2);
        if (i != mRampBlocks.end())
        {
            size_t index = i - mRampBlocks.begin();
            ioSettings.mCanPushCharacter = (index & 1) != 0;
            ioSettings.mCanReceiveImpulses = (index & 2) != 0;
        }
#endif

        // If we encounter an object that can push us, enable sliding
        if (ioSettings.mCanPushCharacter && m_PhysicsInterface.GetImpl().GetBodyInterface().GetMotionType(inBodyID2) != JPH::EMotionType::Static)
            component->m_bAllowSliding = true;
    }


}

void CharacterControllerSystem::OnContactSolve(ECS::EntityHandle entityHandle, const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
{
    ECS::EntityView view = m_World->GetEntityView(entityHandle);

    CharacterControllerComponent* component = view.GetComponent<CharacterControllerComponent>();
    if (component)
    {
        // Don't allow the player to slide down static not-too-steep surfaces when not actively moving and when not on a moving platform
        if (!component->m_bAllowSliding && inContactVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
            ioNewCharacterVelocity = JPH::Vec3::sZero();
    }
}

static float sMaxSlopeAngle = Math::Radians(45.0f);
static float sMaxStrength = 100.0f;
static float sCharacterPadding = 0.02f;
static float sPenetrationRecoverySpeed = 1.0f;
static float sPredictiveContactDistance = 0.1f;
static bool sEnableWalkStairs = true;
static bool sEnableStickToFloor = true;

void CharacterControllerSystem::DestroyCharacters()
{
    auto& bodyInterface = m_PhysicsInterface.GetImpl().GetBodyInterface();

    for (auto& ch : m_PendingRemoveCharacters)
    {
        bodyInterface.RemoveBody(ch.BodyID);
        bodyInterface.DestroyBody(ch.BodyID);
        delete ch.pCharacter;
    }
    m_PendingRemoveCharacters.Clear();
}

void CharacterControllerSystem::Update(GameFrame const& frame)
{
    DestroyCharacters();

    for (auto entity : m_PendingAddCharacters)
    {
        ECS::EntityView view = m_World->GetEntityView(entity);

        CharacterControllerComponent* component = view.GetComponent<CharacterControllerComponent>();
        if (component)
        {
            // Create capsule shapes for all stances
            //switch (sShapeType)
            //{
                //case EType::Capsule:
            //component->m_StandingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * component->cCharacterHeightStanding + component->cCharacterRadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * component->cCharacterHeightStanding, component->cCharacterRadiusStanding)).Create().Get();
            //component->m_CrouchingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * component->cCharacterHeightCrouching + component->cCharacterRadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * component->cCharacterHeightCrouching, component->cCharacterRadiusCrouching)).Create().Get();
            //        break;

                //case EType::Cylinder:
            component->m_StandingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * component->cCharacterHeightStanding + component->cCharacterRadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * component->cCharacterHeightStanding + component->cCharacterRadiusStanding, component->cCharacterRadiusStanding)).Create().Get();
            component->m_CrouchingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * component->cCharacterHeightCrouching + component->cCharacterRadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * component->cCharacterHeightCrouching + component->cCharacterRadiusCrouching, component->cCharacterRadiusCrouching)).Create().Get();
                //    break;

                //case EType::Box:
                //    mStandingShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * cCharacterHeightStanding + cCharacterRadiusStanding, 0), JPH::Quat::sIdentity(), new BoxShape(Vec3(cCharacterRadiusStanding, 0.5f * cCharacterHeightStanding + cCharacterRadiusStanding, cCharacterRadiusStanding))).Create().Get();
                //    mCrouchingShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * cCharacterHeightCrouching + cCharacterRadiusCrouching, 0), JPH::Quat::sIdentity(), new BoxShape(Vec3(cCharacterRadiusCrouching, 0.5f * cCharacterHeightCrouching + cCharacterRadiusCrouching, cCharacterRadiusCrouching))).Create().Get();
                //    break;
            //}



            JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
            settings->mMaxSlopeAngle = sMaxSlopeAngle;
            settings->mMaxStrength = sMaxStrength;
            settings->mShape = component->m_StandingShape;
            settings->mCharacterPadding = sCharacterPadding;
            settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
            settings->mPredictiveContactDistance = sPredictiveContactDistance;
            settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -component->cCharacterRadiusStanding); // Accept contacts that touch the lower sphere of the capsule

            WorldTransformComponent* worldTransform = view.GetComponent<WorldTransformComponent>();

            JPH::CharacterVirtual* character = new CharacterControllerImpl(entity,
                                                                           this,
                                                                           settings,
                                                                           worldTransform ? ConvertVector(worldTransform->Position[frame.StateIndex]) : JPH::Vec3::sZero(),
                                                                           worldTransform ? ConvertQuaternion(worldTransform->Rotation[frame.StateIndex]) : JPH::Quat::sIdentity(),
                                                                           &m_PhysicsInterface.GetImpl());
            character->SetListener(this);

            JPH::ObjectLayer layer = MakeObjectLayer(component->m_CollisionGroup, BroadphaseLayer::CHARACTER_PROXY);

            {
                JPH::BodyCreationSettings body_settings(component->m_StandingShape,
                                                        character->GetPosition(),
                                                        character->GetRotation(),
                                                        JPH::EMotionType::Kinematic,
                                                        layer);
                body_settings.mUserData = entity;

                //body_settings.mCollisionGroup.SetGroupID(groupId);
                //body_settings.mCollisionGroup.SetGroupFilter(GetGroupFilter());

                component->m_BodyId = m_PhysicsInterface.GetImpl().GetBodyInterface().CreateAndAddBody(body_settings, JPH::EActivation::Activate);
            }

            component->m_pCharacter = character;
        }
    }
    m_PendingAddCharacters.Clear();

    // On pre physics update:
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
            desiredVelocity = 0.25f * movementDir * characterController[i].cCharacterSpeed + 0.75f * desiredVelocity;

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
                    new_velocity += JPH::Vec3(0, characterController[i].cJumpSpeed, 0);
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
            if (!sEnableStickToFloor)
                update_settings.mStickToFloorStepDown = JPH::Vec3::sZero();
            if (!sEnableWalkStairs)
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

            ObjectLayerFilter layerFilter(m_PhysicsInterface.GetCollisionFilter(), characterController[i].m_CollisionGroup);

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
            bodyFilter.m_IgoreBodyID = characterController[i].m_BodyId;

            // Update the character position
            character->ExtendedUpdate(timeStep,
                                      gravity,
                                      update_settings,
                                      broadphaseFilter,//m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                      layerFilter,//m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING),
                                      bodyFilter,
                                      {},
                                      tempAllocator);

            //m_PhysicsInterface.GetBodyInterface().MoveKinematic(characterController[i].m_BodyId,
            //                                                 character->GetPosition(), character->GetRotation(),
            //                                                 frame.FixedTimeStep);

            //m_PhysicsInterface.GetImpl().GetBodyInterface().SetPositionAndRotation(characterController[i].m_BodyId, character->GetPosition(), character->GetRotation(), JPH::EActivation::Activate);

            m_PhysicsInterface.GetImpl().GetBodyInterface().MoveKinematic(characterController[i].m_BodyId, character->GetPosition(), character->GetRotation(), timeStep);

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
            ::ReadOnly<FinalTransformComponent>;

        renderer.SetColor(Color4(1, 1, 0, 1));

        for (Query::Iterator it(*m_World); it; it++)
        {
            CharacterControllerComponent const* characterController = it.Get<CharacterControllerComponent>();
            FinalTransformComponent const* transform = it.Get<FinalTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                float r = characterController[i].cCharacterRadiusStanding;
                float h = characterController[i].cCharacterHeightStanding;

                renderer.DrawCapsule(transform[i].Position + Float3(0, h * 0.5f + r, 0), transform[i].Rotation.ToMatrix3x3(), r, h, 1);
            }
        }
    }
}

HK_NAMESPACE_END
