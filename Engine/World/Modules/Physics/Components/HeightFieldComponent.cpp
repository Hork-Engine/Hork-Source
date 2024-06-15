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

#include "HeightFieldComponent.h"
#include <Engine/World/Modules/Physics/PhysicsInterfaceImpl.h>
#include <Engine/World/Modules/Physics/CollisionModel.h>

HK_NAMESPACE_BEGIN

void HeightFieldComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    GameObject* owner = GetOwner();

    m_UserData = physics->CreateUserData();
    m_UserData->Initialize(this);
    
    JPH::BodyCreationSettings settings;
    settings.SetShape(m_CollisionModel->Instatiate());
    settings.mPosition = ConvertVector(owner->GetWorldPosition());
    settings.mRotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());
    settings.mUserData = (size_t)m_UserData;
    settings.mObjectLayer = MakeObjectLayer(m_CollisionLayer, BroadphaseLayer::Static);
    //settings.mCollisionGroup.SetGroupID(ObjectFilterID);
    //settings.mCollisionGroup.SetGroupFilter(physics->m_GroupFilter);
    settings.mMotionType = JPH::EMotionType::Static;
    settings.mAllowDynamicOrKinematic = false;
    settings.mIsSensor = false;
    //settings.mFriction = Material.Friction;
    //settings.mRestitution = Material.Restitution;

    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    JPH::Body* body = bodyInterface.CreateBody(settings);
    m_BodyID = PhysBodyID(body->GetID().GetIndexAndSequenceNumber());

    physics->QueueToAdd(body, true);
}

void HeightFieldComponent::EndPlay()
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

    physics->DeleteUserData(m_UserData);
    m_UserData = nullptr;
}

HK_NAMESPACE_END
