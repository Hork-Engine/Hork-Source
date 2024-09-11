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

#include "DynamicBodyComponent.h"
#include <Hork/Runtime/World/Modules/Physics/PhysicsInterfaceImpl.h>

HK_NAMESPACE_BEGIN

void DynamicBodyComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    GameObject* owner = GetOwner();

    m_UserData = physics->CreateUserData();
    m_UserData->Initialize(this);

    m_CachedScale = owner->GetWorldScale();

    CreateCollisionSettings collisionSettings;
    collisionSettings.Object = GetOwner();
    collisionSettings.CenterOfMassOverride = CenterOfMassOverride;
    collisionSettings.ConvexOnly = true;

    if (physics->CreateCollision(collisionSettings, m_Shape, m_ScalingMode))
    {
        JPH::BodyCreationSettings settings;
        settings.SetShape(physics->CreateScaledShape(m_ScalingMode, m_Shape, m_CachedScale));
        settings.mPosition = ConvertVector(owner->GetWorldPosition());
        settings.mRotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());
        settings.mLinearVelocity = ConvertVector(LinearVelocity);
        settings.mAngularVelocity = ConvertVector(AngularVelocity);
        settings.mUserData = (size_t)m_UserData;
        settings.mObjectLayer = MakeObjectLayer(CollisionLayer, BroadphaseLayer::Dynamic);
        settings.mMotionType = m_IsKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
        settings.mIsSensor = false;
        //settings.mUseManifoldReduction = true;
        settings.mMotionQuality = UseCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
        settings.mAllowSleeping = AllowSleeping;
        settings.mFriction = Material.Friction;
        settings.mRestitution = Material.Restitution;
        settings.mLinearDamping = LinearDamping;
        settings.mAngularDamping = AngularDamping;
        settings.mMaxLinearVelocity = MaxLinearVelocity;
        settings.mMaxAngularVelocity = MaxAngularVelocity;
        settings.mGravityFactor = m_GravityFactor;
        settings.mEnhancedInternalEdgeRemoval = true;
    
        settings.mInertiaMultiplier = InertiaMultiplier;
        if (Mass > 0.0f)
        {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            settings.mMassPropertiesOverride.mMass = Mass;
        }
        else
        {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        }

        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        JPH::Body* body = bodyInterface.CreateBody(settings);
        m_BodyID = PhysBodyID(body->GetID().GetIndexAndSequenceNumber());

        physics->QueueToAdd(body, StartAsSleeping);
    }

    if (m_IsKinematic)
    {
        physics->m_KinematicBodies.Add(Handle32<DynamicBodyComponent>(GetHandle()));
    }

    if (m_IsDynamicScaling)
    {
        physics->m_DynamicScaling.Add(Handle32<DynamicBodyComponent>(GetHandle()));
    }

    if (!m_IsKinematic)
        owner->SetLockWorldPositionAndRotation(true);
}

void DynamicBodyComponent::EndPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();

    GameObject* owner = GetOwner();
    if (!m_IsKinematic)
        owner->SetLockWorldPositionAndRotation(false);

    if (m_IsKinematic)
    {
        auto index = physics->m_KinematicBodies.IndexOf(Handle32<DynamicBodyComponent>(GetHandle()));
        physics->m_KinematicBodies.RemoveUnsorted(index);
    }

    if (m_IsDynamicScaling)
    {
        auto index = physics->m_DynamicScaling.IndexOf(Handle32<DynamicBodyComponent>(GetHandle()));
        physics->m_DynamicScaling.RemoveUnsorted(index);
    }

    JPH::BodyID bodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        if (bodyInterface.IsAdded(bodyID))
            bodyInterface.RemoveBody(bodyID);

        bodyInterface.DestroyBody(bodyID);
        m_BodyID.ID = PhysBodyID::InvalidID;
    }

    if (m_Shape)
    {
        m_Shape->Release();
        m_Shape = nullptr;
    }

    physics->DeleteUserData(m_UserData);
    m_UserData = nullptr;
}

void DynamicBodyComponent::SetKinematic(bool isKinematic)
{
    if (m_IsKinematic == isKinematic)
        return;

    m_IsKinematic = isKinematic;

    if (!IsInitialized())
        return;

    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();

    if (m_IsKinematic)
    {
        physics->m_KinematicBodies.Add(Handle32<DynamicBodyComponent>(GetHandle()));
    }
    else
    {
        auto index = physics->m_KinematicBodies.IndexOf(Handle32<DynamicBodyComponent>(GetHandle()));
        physics->m_KinematicBodies.RemoveUnsorted(index);
    }

    auto bodyID = JPH::BodyID(m_BodyID.ID);

    if (bodyID.IsInvalid())
        return;

    {
        JPH::BodyLockWrite bodyLock(physics->m_PhysSystem.GetBodyLockInterface(), bodyID);
        if (bodyLock.Succeeded())
        {
            JPH::Body& body = bodyLock.GetBody();
            body.SetMotionType(m_IsKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
        }
    }

    if (!m_IsKinematic && physics->m_PhysSystem.GetBodyInterface().IsAdded(bodyID))
    {
        physics->m_PhysSystem.GetBodyInterface().ActivateBody(bodyID);
    }

    if (m_IsKinematic)
        GetOwner()->SetLockWorldPositionAndRotation(false);
    else
        GetOwner()->SetLockWorldPositionAndRotation(true);
}

void DynamicBodyComponent::SetDynamicScaling(bool isDynamicScaling)
{
    if (m_IsDynamicScaling == isDynamicScaling)
        return;

    m_IsDynamicScaling = isDynamicScaling;

    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();

    if (m_IsDynamicScaling)
    {
        physics->m_DynamicScaling.Add(Handle32<DynamicBodyComponent>(GetHandle()));
    }
    else
    {
        auto index = physics->m_DynamicScaling.IndexOf(Handle32<DynamicBodyComponent>(GetHandle()));
        physics->m_DynamicScaling.RemoveUnsorted(index);
    }
}

void DynamicBodyComponent::SetGravityFactor(float factor)
{
    if (m_GravityFactor == factor)
        return;

    m_GravityFactor = factor;

    auto bodyID = JPH::BodyID(m_BodyID.ID);

    if (bodyID.IsInvalid())
        return;

    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();

    JPH::BodyLockWrite bodyLock(physics->m_PhysSystem.GetBodyLockInterface(), bodyID);

    if (bodyLock.Succeeded())
    {
        bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_GravityFactor);

        if (physics->m_PhysSystem.GetBodyInterfaceNoLock().IsAdded(bodyID))
        {
            physics->m_PhysSystem.GetBodyInterfaceNoLock().ActivateBody(bodyID);
        }
    }
}

void DynamicBodyComponent::SetWorldPosition(Float3 const& position)
{
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

void DynamicBodyComponent::SetWorldRotation(Quat const& rotation)
{
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

void DynamicBodyComponent::SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation)
{
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

Float3 DynamicBodyComponent::GetWorldPosition() const
{
    return GetOwner()->GetWorldPosition();
}

Quat DynamicBodyComponent::GetWorldRotation() const
{
    return GetOwner()->GetWorldRotation();
}

void DynamicBodyComponent::MoveKinematic(Float3 const& destPosition, CoordinateSpace coordSpace)
{
    if (!IsKinematic())
        return;

    if (coordSpace == CoordinateSpace::World)
        GetOwner()->SetWorldPosition(destPosition);
    else
        GetOwner()->SetPosition(destPosition);
}

void DynamicBodyComponent::RotateKinematic(Quat const& destRotation, CoordinateSpace coordSpace)
{
    if (!IsKinematic())
        return;

    if (coordSpace == CoordinateSpace::World)
        GetOwner()->SetWorldRotation(destRotation);
    else
        GetOwner()->SetRotation(destRotation);
}

void DynamicBodyComponent::MoveAndRotateKinematic(Float3 const& destPosition, Quat const& destRotation, CoordinateSpace coordSpace)
{
    if (!IsKinematic())
        return;

    if (coordSpace == CoordinateSpace::World)
        GetOwner()->SetWorldPositionAndRotation(destPosition, destRotation);
    else
        GetOwner()->SetPositionAndRotation(destPosition, destRotation);
}

void DynamicBodyComponent::AddForce(Float3 const& force)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddForce, force);
}

void DynamicBodyComponent::AddForceAtPosition(Float3 const& force, Float3 const& position)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddForceAtPosition, force, position);
}

void DynamicBodyComponent::AddTorque(Float3 const& torque)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddTorque, torque);
}

void DynamicBodyComponent::AddForceAndTorque(Float3 const& force, Float3 const& torque)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddForceAndTorque, force, torque);
}

void DynamicBodyComponent::AddImpulse(Float3 const& impulse)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddImpulse, impulse);
}

void DynamicBodyComponent::AddImpulseAtPosition(Float3 const& impulse, Float3 const& position)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddImpulseAtPosition, impulse, position);
}

void DynamicBodyComponent::AddAngularImpulse(Float3 const& angularImpulse)
{
    GetWorld()->GetInterface<PhysicsInterface>().GetImpl()->m_DynamicBodyMessageQueue.EmplaceBack(Handle32<DynamicBodyComponent>(GetHandle()), DynamicBodyMessage::AddAngularImpulse, angularImpulse);
}

float DynamicBodyComponent::GetMass() const
{
    if (m_IsKinematic || !m_BodyID.IsValid())
        return 0.0f;

    World* world = const_cast<World*>(GetWorld());

    auto& bodyLockInterface = world->GetInterface<PhysicsInterface>().GetImpl()->m_PhysSystem.GetBodyLockInterface();

    JPH::BodyLockRead bodyLock(bodyLockInterface, JPH::BodyID(m_BodyID.ID));

    const float inverseMass = bodyLock.GetBody().GetMotionProperties()->GetInverseMass();
    return inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
}

Float3 DynamicBodyComponent::GetCenterOfMassPosition() const
{
    World* world = const_cast<World*>(GetWorld());

    PhysicsInterfaceImpl* physics = world->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    return ConvertVector(bodyInterface.GetCenterOfMassPosition(JPH::BodyID(m_BodyID.ID)));
}

Float3 DynamicBodyComponent::GetLinearVelocity() const
{
    World* world = const_cast<World*>(GetWorld());

    PhysicsInterfaceImpl* physics = world->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    return ConvertVector(bodyInterface.GetLinearVelocity(JPH::BodyID(m_BodyID.ID)));
}

Float3 DynamicBodyComponent::GetAngularVelocity() const
{
    World* world = const_cast<World*>(GetWorld());

    PhysicsInterfaceImpl* physics = world->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    return ConvertVector(bodyInterface.GetAngularVelocity(JPH::BodyID(m_BodyID.ID)));
}

Float3 DynamicBodyComponent::GetVelocityAtPosition(Float3 const& position) const
{
    World* world = const_cast<World*>(GetWorld());

    PhysicsInterfaceImpl* physics = world->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    return ConvertVector(bodyInterface.GetPointVelocity(JPH::BodyID(m_BodyID.ID), ConvertVector(position)));
}

bool DynamicBodyComponent::IsSleeping() const
{
    World* world = const_cast<World*>(GetWorld());

    PhysicsInterfaceImpl* physics = world->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    return !bodyInterface.IsActive(JPH::BodyID(m_BodyID.ID));
}

void DynamicBodyComponent::GatherGeometry(Vector<Float3>& vertices, Vector<uint32_t>& indices)
{
    JPH::BodyID bodyID(m_BodyID.ID);
    if (bodyID.IsInvalid())
        return;

    if (!m_Shape)
        return;

    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    JPH::Vec3 position;
    JPH::Quat rotation;
    bodyInterface.GetPositionAndRotation(bodyID, position, rotation);

    Float3x4 transformMatrix;
    transformMatrix.Compose(ConvertVector(position), ConvertQuaternion(rotation).ToMatrix3x3());

    auto firstVert = vertices.Size();
    PhysicsInterfaceImpl::sGatherShapeGeometry(bodyInterface.GetShape(bodyID), vertices, indices);

    if (firstVert != vertices.Size())
        TransformVertices(&vertices[firstVert], vertices.Size() - firstVert, transformMatrix);
}

HK_NAMESPACE_END
