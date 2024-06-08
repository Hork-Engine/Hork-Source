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
