#include "CharacterControllerComponent.h"
#include <Engine/World/Modules/Physics/PhysicsInterfaceImpl.h>
#include <Engine/World/Modules/Physics/CollisionModel.h>

HK_NAMESPACE_BEGIN

void CharacterControllerComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
	GameObject* owner = GetOwner();

    // Create capsule shapes for all stances
    //switch (ShapeType)
    //{
    //case CHARACTER_SHAPE_CAPSULE:
    //auto standing_shape= JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * HeightStanding, RadiusStanding)).Create().Get();
    //auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * HeightCrouching, RadiusCrouching)).Create().Get();
    //        break;

    //case CHARACTER_SHAPE_CYLINDER:
    auto standing_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * HeightStanding + RadiusStanding, RadiusStanding)).Create().Get();
    auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * HeightCrouching + RadiusCrouching, RadiusCrouching)).Create().Get();
    //    break;

    //case CHARACTER_SHAPE_BOX:
    //auto standing_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(RadiusStanding, 0.5f * HeightStanding + RadiusStanding, RadiusStanding))).Create().Get();
    //auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(RadiusCrouching, 0.5f * HeightCrouching + RadiusCrouching, RadiusCrouching))).Create().Get();
    //    break;
    //}
    // 
    //if (ObjectFilterID == ~0u)
    {
        // TOOD create ObjectFilterID
    }

    m_UserData = physics->CreateUserData();
    m_UserData->Initialize(this);

    auto position = ConvertVector(owner->GetWorldPosition());
    auto rotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());
    
    {
        JPH::BodyCreationSettings settings;
        settings.SetShape(standing_shape);
	    settings.mPosition = position;
	    settings.mRotation = rotation;
	    settings.mUserData = (size_t)m_UserData;
        settings.mObjectLayer = MakeObjectLayer(m_CollisionLayer, BroadphaseLayer::Character);
	    //settings.mCollisionGroup.SetGroupID(ObjectFilterID);
        //settings.mCollisionGroup.SetGroupFilter(physics->m_GroupFilter);
        settings.mMotionType = JPH::EMotionType::Kinematic;

        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        JPH::Body* body = bodyInterface.CreateBody(settings);
        m_BodyID = PhysBodyID(body->GetID().GetIndexAndSequenceNumber());

        physics->QueueToAdd(body, false);
    }

    {
        JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
        settings->mMaxSlopeAngle = MaxSlopeAngle;
        settings->mMaxStrength = MaxStrength;
        settings->mShape = standing_shape;
        settings->mCharacterPadding = CharacterPadding;
        settings->mPenetrationRecoverySpeed = PenetrationRecoverySpeed;
        settings->mPredictiveContactDistance = PredictiveContactDistance;
        settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -RadiusStanding); // Accept contacts that touch the lower sphere of the capsule

        m_pImpl = new CharacterControllerImpl(settings, position, rotation, &physics->m_PhysSystem);
        m_pImpl->m_StandingShape = standing_shape;
        m_pImpl->m_CrouchingShape = crouching_shape;
    }
}

void CharacterControllerComponent::EndPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();

    JPH::BodyID bodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        if (bodyInterface.IsAdded(bodyID))
            bodyInterface.RemoveBody(bodyID);

        bodyInterface.DestroyBody(bodyID);
        m_BodyID.ID = JPH::BodyID::cInvalidBodyID;
    }

    delete m_pImpl;
    m_pImpl = nullptr;

    physics->DeleteUserData(m_UserData);
    m_UserData = nullptr;
}

void CharacterControllerComponent::SetWorldPosition(Float3 const& position)
{
    if (m_pImpl)
        m_pImpl->SetPosition(ConvertVector(position));

    auto bodyID = JPH::BodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        bool isAdded = bodyInterface.IsAdded(bodyID);

        bodyInterface.SetPosition(JPH::BodyID(m_BodyID.ID), ConvertVector(position), isAdded ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }

    GetOwner()->SetWorldPosition(position);
}

void CharacterControllerComponent::SetWorldRotation(Quat const& rotation)
{
    if (m_pImpl)
        m_pImpl->SetRotation(ConvertQuaternion(rotation));

    auto bodyID = JPH::BodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        bool isAdded = bodyInterface.IsAdded(bodyID);

        bodyInterface.SetRotation(JPH::BodyID(m_BodyID.ID), ConvertQuaternion(rotation), isAdded ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }

    GetOwner()->SetWorldRotation(rotation);
}

void CharacterControllerComponent::SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation)
{
    if (m_pImpl)
    {
        m_pImpl->SetPosition(ConvertVector(position));
        m_pImpl->SetRotation(ConvertQuaternion(rotation));
    }

    auto bodyID = JPH::BodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        bool isAdded = bodyInterface.IsAdded(bodyID);

        bodyInterface.SetPositionAndRotation(JPH::BodyID(m_BodyID.ID), ConvertVector(position), ConvertQuaternion(rotation), isAdded ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }

    GetOwner()->SetWorldPositionAndRotation(position, rotation);
}

Float3 CharacterControllerComponent::GetWorldPosition() const
{
    return GetOwner()->GetWorldPosition();
}

Quat CharacterControllerComponent::GetWorldRotation() const
{
    return GetOwner()->GetWorldRotation();
}

void CharacterControllerComponent::SetLinearVelocity(Float3 const& velocity)
{
    if (m_pImpl)
        m_pImpl->SetLinearVelocity(ConvertVector(velocity));
}

Float3 CharacterControllerComponent::GetLinearVelocity()
{
    if (m_pImpl)
        return ConvertVector(m_pImpl->GetLinearVelocity());
    return Float3(0);
}

HK_NAMESPACE_END
