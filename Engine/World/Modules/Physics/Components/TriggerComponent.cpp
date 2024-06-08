#include "TriggerComponent.h"
#include <Engine/World/Modules/Physics/PhysicsInterfaceImpl.h>
#include <Engine/World/Modules/Physics/CollisionModel.h>

HK_NAMESPACE_BEGIN

void TriggerComponent::BeginPlay()
{
    PhysicsInterfaceImpl* physics = GetWorld()->GetInterface<PhysicsInterface>().GetImpl();
    GameObject* owner = GetOwner();

    m_UserData = physics->CreateUserData();
    m_UserData->Initialize(this);

    JPH::BodyCreationSettings settings;
    settings.SetShape(m_CollisionModel->Instatiate(owner->GetWorldScale()));
    settings.mPosition = ConvertVector(owner->GetWorldPosition());
    settings.mRotation = ConvertQuaternion(owner->GetWorldRotation().Normalized());
    settings.mUserData = (size_t)m_UserData;
    settings.mObjectLayer = MakeObjectLayer(m_CollisionLayer, BroadphaseLayer::Trigger);
    //settings.mCollisionGroup.SetGroupID(ObjectFilterID);
    //settings.mCollisionGroup.SetGroupFilter(physics->m_GroupFilter);

    // Motion type устанавливаем Kinematic, чтобы не срабатывали "OnEndOverlap" на заснувших объектах
    settings.mMotionType = JPH::EMotionType::Kinematic;//owner->IsDynamic() ? JPH::EMotionType::Kinematic : JPH::EMotionType::Static;
    settings.mIsSensor = true;

    auto& bodyInterface = physics->m_PhysSystem.GetBodyInterface();

    JPH::Body* body = bodyInterface.CreateBody(settings);
    m_BodyID = PhysBodyID(body->GetID().GetIndexAndSequenceNumber());

    physics->QueueToAdd(body, false);

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

    physics->DeleteUserData(m_UserData);
    m_UserData = nullptr;
}

HK_NAMESPACE_END
