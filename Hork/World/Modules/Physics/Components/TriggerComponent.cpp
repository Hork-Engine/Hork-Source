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

#include "TriggerComponent.h"
#include <Hork/World/Modules/Physics/PhysicsInterfaceImpl.h>

HK_NAMESPACE_BEGIN

void TriggerComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    GameObject* owner = GetOwner();

    m_UserData = physics->CreateUserData();
    m_UserData->Initialize(this);

    CreateCollisionSettings collisionSettings;
    collisionSettings.Object = GetOwner();
    collisionSettings.ConvexOnly = false;

    ScalingMode scalingMode;
    if (physics->CreateCollision(collisionSettings, m_Shape, scalingMode))
    {
        JPH::BodyCreationSettings settings;
        settings.SetShape(physics->CreateScaledShape(scalingMode, m_Shape, owner->GetWorldScale()));
        settings.mPosition = ConvertVector(owner->GetWorldPosition());
        settings.mRotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());
        settings.mUserData = (size_t)m_UserData;
        settings.mObjectLayer = MakeObjectLayer(CollisionLayer, BroadphaseLayer::Trigger);

        // Motion type устанавливаем Kinematic, чтобы не срабатывали "OnEndOverlap" на заснувших объектах
        settings.mMotionType = JPH::EMotionType::Kinematic;//owner->IsDynamic() ? JPH::EMotionType::Kinematic : JPH::EMotionType::Static;
        settings.mIsSensor = true;

        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        JPH::Body* body = bodyInterface.CreateBody(settings);
        m_BodyID = PhysBodyID(body->GetID().GetIndexAndSequenceNumber());

        physics->QueueToAdd(body, false);
    }

    if (owner->IsDynamic())
    {
        physics->m_MovableTriggers.Add(Handle32<TriggerComponent>(GetHandle()));
    }
}

void TriggerComponent::EndPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    GameObject* owner = GetOwner();

    for (auto it = physics->m_ContactListener.m_Triggers.begin() ; it != physics->m_ContactListener.m_Triggers.end() ;)
    {
        if (it->second.Trigger.ToUInt32() == GetHandle().ToUInt32())
            it = physics->m_ContactListener.m_Triggers.Erase(it);
        else
            ++it;
    }

    if (owner->IsDynamic())
    {
        auto index = physics->m_MovableTriggers.IndexOf(Handle32<TriggerComponent>(GetHandle()));
        physics->m_MovableTriggers.RemoveUnsorted(index);
    }

    JPH::BodyID bodyID(m_BodyID.ID);
    if (!bodyID.IsInvalid())
    {
        auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

        if (bodyInterface.IsAdded(bodyID))
            bodyInterface.RemoveBody(bodyID);

        bodyInterface.DestroyBody(bodyID);
        m_BodyID.ID = JPH::BodyID::cInvalidBodyID;
    }

    if (m_Shape)
    {
        m_Shape->Release();
        m_Shape = nullptr;
    }

    physics->DeleteUserData(m_UserData);
    m_UserData = nullptr;
}

HK_NAMESPACE_END
