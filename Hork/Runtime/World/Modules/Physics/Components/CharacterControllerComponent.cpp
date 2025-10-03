/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "CharacterControllerComponent.h"
#include <Hork/Runtime/World/Modules/Physics/PhysicsInterfaceImpl.h>
#include <Hork/Runtime/World/Modules/Physics/PhysicsModule.h>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

HK_NAMESPACE_BEGIN

void CharacterControllerComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
	GameObject* owner = GetOwner();

    JPH::Ref<JPH::Shape> standingShape, crouchingShape;

    switch (ShapeType)
    {
        case CharacterShapeType::Box:
            standingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(RadiusStanding, 0.5f * HeightStanding + RadiusStanding, RadiusStanding))).Create().Get();
            crouchingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(RadiusCrouching, 0.5f * HeightCrouching + RadiusCrouching, RadiusCrouching))).Create().Get();
            break;

        case CharacterShapeType::Cylinder:
            standingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * HeightStanding + RadiusStanding, RadiusStanding)).Create().Get();
            crouchingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * HeightCrouching + RadiusCrouching, RadiusCrouching)).Create().Get();
            break;

        case CharacterShapeType::Capsule:
            standingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * HeightStanding, RadiusStanding)).Create().Get();
            crouchingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * HeightCrouching, RadiusCrouching)).Create().Get();
            break;
    }

    auto position = ConvertVector(owner->GetWorldPosition());
    auto rotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());

    JPH::CharacterVirtualSettings settings;
    settings.mMass = m_Mass;
    settings.mMaxSlopeAngle = Math::Radians(m_MaxSlopeAngle);
    settings.mMaxStrength = m_MaxStrength;
    settings.mShape = standingShape;
    settings.mCharacterPadding = CharacterPadding;
    settings.mPenetrationRecoverySpeed = m_PenetrationRecoverySpeed;
    settings.mPredictiveContactDistance = PredictiveContactDistance;
    settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -RadiusStanding); // Accept contacts that touch the lower sphere of the capsule
    settings.mEnhancedInternalEdgeRemoval = true;

    m_pImpl = new CharacterControllerImpl(&settings, position, rotation, &physics->m_PhysSystem);
    m_pImpl->SetListener(&physics->m_CharacterContactListener);
    m_pImpl->m_Component = Handle32<CharacterControllerComponent>(GetHandle());
    m_pImpl->m_StandingShape = standingShape;
    m_pImpl->m_CrouchingShape = crouchingShape;
    m_pImpl->m_CollisionLayer = m_CollisionLayer;

    m_pImpl->SetLinearVelocity(ConvertVector(m_LinearVelocity));

    m_pImpl->SetCharacterVsCharacterCollision(&physics->m_CharacterVsCharacterCollision);
    physics->m_CharacterVsCharacterCollision.Add(m_pImpl);
}

void CharacterControllerComponent::EndPlay()
{
    if (m_pImpl)
    {
        PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
        physics->m_CharacterVsCharacterCollision.Remove(m_pImpl);

        delete m_pImpl;
        m_pImpl = nullptr;
    }
}

void CharacterControllerComponent::SetCollisionLayer(uint8_t collisionLayer)
{
    m_CollisionLayer = collisionLayer;

    if (m_pImpl)
        m_pImpl->m_CollisionLayer = collisionLayer;
}

void CharacterControllerComponent::SetWorldPosition(Float3 const& position)
{
    if (m_pImpl)
        m_pImpl->SetPosition(ConvertVector(position));

    GetOwner()->SetWorldPosition(position);
}

void CharacterControllerComponent::SetWorldRotation(Quat const& rotation)
{
    if (m_pImpl)
        m_pImpl->SetRotation(ConvertQuaternion(rotation));

    GetOwner()->SetWorldRotation(rotation);
}

void CharacterControllerComponent::SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation)
{
    if (m_pImpl)
    {
        m_pImpl->SetPosition(ConvertVector(position));
        m_pImpl->SetRotation(ConvertQuaternion(rotation));
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

void CharacterControllerComponent::SetMass(float mass)
{
    m_Mass = mass;
    if (m_pImpl)
        m_pImpl->SetMass(mass);
}

float CharacterControllerComponent::GetMass() const
{
    return m_Mass;
}

void CharacterControllerComponent::SetMaxStrength(float maxStrength)
{
    m_MaxStrength = maxStrength;
    if (m_pImpl)
        m_pImpl->SetMaxStrength(maxStrength);
}

float CharacterControllerComponent::GetMaxStrength() const
{
    return m_MaxStrength;
}

void CharacterControllerComponent::SetMaxSlopeAngle(float maxSlopeAngle)
{
    m_MaxSlopeAngle = maxSlopeAngle;
    if (m_pImpl)
        m_pImpl->SetMaxSlopeAngle(Math::Radians(maxSlopeAngle));
}

float CharacterControllerComponent::GetMaxSlopeAngle() const
{
    return m_MaxSlopeAngle;
}

void CharacterControllerComponent::SetPenetrationRecoverySpeed(float speed)
{
    m_PenetrationRecoverySpeed = speed;
    if (m_pImpl)
        m_pImpl->SetPenetrationRecoverySpeed(speed);
}

float CharacterControllerComponent::GetPenetrationRecoverySpeed() const
{
    return m_PenetrationRecoverySpeed;
}

void CharacterControllerComponent::SetLinearVelocity(Float3 const& velocity)
{
    m_LinearVelocity = velocity;
    if (m_pImpl)
        m_pImpl->SetLinearVelocity(ConvertVector(velocity));
}

Float3 CharacterControllerComponent::GetLinearVelocity() const
{
    if (m_pImpl)
        return ConvertVector(m_pImpl->GetLinearVelocity());
    return Float3(0);
}

bool CharacterControllerComponent::IsSlopeTooSteep(Float3 const& normal) const
{
    if (m_pImpl)
        return m_pImpl->IsSlopeTooSteep(ConvertVector(normal));
    return false;
}

Float3 CharacterControllerComponent::GetGroundPosition() const
{
    if (m_pImpl)
        return ConvertVector(m_pImpl->GetGroundPosition());
    return Float3(0);
}

Float3 CharacterControllerComponent::GetGroundNormal() const
{
    if (m_pImpl)
        return ConvertVector(m_pImpl->GetGroundNormal());
    return Float3(0);
}

Float3 CharacterControllerComponent::GetGroundVelocity() const
{
    if (m_pImpl)
        return ConvertVector(m_pImpl->GetGroundVelocity());
    return Float3(0);
}

BodyComponent* CharacterControllerComponent::TryGetGroundBody()
{
    if (m_pImpl)
    {
        BodyUserData* userData = reinterpret_cast<BodyUserData*>(m_pImpl->GetGroundUserData());
        if (userData)
        {
            if (Component* component = userData->TryGetComponent(GetWorld()))
                return static_cast<BodyComponent*>(component);
        }
    }

    return nullptr;
}

bool CharacterControllerComponent::IsOnGround() const
{
    if (m_pImpl)
        return m_pImpl->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
    return false;
}

bool CharacterControllerComponent::IsOnSteepGround() const
{
    if (m_pImpl)
        return m_pImpl->GetGroundState() == JPH::CharacterBase::EGroundState::OnSteepGround;
    return false;
}

bool CharacterControllerComponent::ShouldFall() const
{
    if (m_pImpl)
        return m_pImpl->GetGroundState() == JPH::CharacterBase::EGroundState::NotSupported;
    return false;
}

bool CharacterControllerComponent::IsInAir() const
{
    if (m_pImpl)
        return m_pImpl->GetGroundState() == JPH::CharacterBase::EGroundState::InAir;
    return false;
}

void CharacterControllerComponent::UpdateGroundVelocity()
{
    if (m_pImpl)
        m_pImpl->UpdateGroundVelocity();
}

bool CharacterControllerComponent::UpdateStance(CharacterStance stance, float maxPenetrationDepth)
{
    // TODO: Add custom stances

    if (!m_pImpl)
        return false;

    CharacterControllerImpl::BroadphaseLayerFilter broadphaseFilter(
        HK_BIT(uint32_t(BroadphaseLayer::Static)) |
        HK_BIT(uint32_t(BroadphaseLayer::Dynamic)) |
        HK_BIT(uint32_t(BroadphaseLayer::Trigger)) |
        HK_BIT(uint32_t(BroadphaseLayer::Character)));

    ObjectLayerFilter layerFilter(GetWorld()->GetInterface<PhysicsInterface>().GetCollisionFilter(), m_CollisionLayer);
    CharacterControllerImpl::BodyFilter bodyFilter;
    CharacterControllerImpl::ShapeFilter shapeFilter;

    return m_pImpl->SetShape(stance == CharacterStance::Standing ? m_pImpl->m_StandingShape : m_pImpl->m_CrouchingShape,
                             maxPenetrationDepth, broadphaseFilter, layerFilter, bodyFilter, shapeFilter, *PhysicsModule::sGet().GetTempAllocator());
}

HK_NAMESPACE_END
