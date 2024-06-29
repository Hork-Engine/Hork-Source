/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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
    //auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * HeightCrouching + RadiusCrouching, RadiusCrouching)).Create().Get();
    //    break;

    //case CHARACTER_SHAPE_BOX:
    //auto standing_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightStanding + RadiusStanding, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(RadiusStanding, 0.5f * HeightStanding + RadiusStanding, RadiusStanding))).Create().Get();
    //auto crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * HeightCrouching + RadiusCrouching, 0), JPH::Quat::sIdentity(), new JPH::BoxShape(JPH::Vec3(RadiusCrouching, 0.5f * HeightCrouching + RadiusCrouching, RadiusCrouching))).Create().Get();
    //    break;
    //}
    // 

    auto position = ConvertVector(owner->GetWorldPosition());
    auto rotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());

    JPH::CharacterVirtualSettings settings;
    settings.mMaxSlopeAngle = MaxSlopeAngle;
    settings.mMaxStrength = MaxStrength;
    settings.mShape = standing_shape;
    settings.mCharacterPadding = CharacterPadding;
    settings.mPenetrationRecoverySpeed = PenetrationRecoverySpeed;
    settings.mPredictiveContactDistance = PredictiveContactDistance;
    settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -RadiusStanding); // Accept contacts that touch the lower sphere of the capsule

    m_pImpl = new CharacterControllerImpl(&settings, position, rotation, &physics->m_PhysSystem);
    m_pImpl->SetListener(&physics->m_CharacterContactListener);
    m_pImpl->m_Component = GetHandle();
    //m_pImpl->m_StandingShape = standing_shape;
    //m_pImpl->m_CrouchingShape = crouching_shape;
}

void CharacterControllerComponent::EndPlay()
{
    delete m_pImpl;
    m_pImpl = nullptr;
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
