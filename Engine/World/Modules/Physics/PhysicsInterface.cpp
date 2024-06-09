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

#include "PhysicsInterfaceImpl.h"
#include "CollisionModel.h"
#include "PhysicsModule.h"
#include "Components/StaticBodyComponent.h"
#include "Components/DynamicBodyComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/HeightFieldComponent.h"
#include "Components/TriggerComponent.h"
#include "Components/WaterVolumeComponent.h"

#include <Engine/World/DebugRenderer.h>

#include <Engine/Core/Logger.h>
#include <Engine/Core/ConsoleVar.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Geometry/OrientedBox.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCollisionModel("com_DrawCollisionModel"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCollisionShape("com_DrawCollisionShape"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawTriggers("com_DrawTriggers"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCenterOfMass("com_DrawCenterOfMass"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawWaterVolume("com_DrawWaterVolume"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCharacterController("com_DrawCharacterController"s, "0"s);

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

class BroadphaseBodyCollector : public JPH::CollideShapeBodyCollector
{
public:
    BroadphaseBodyCollector(Vector<PhysBodyID>& outResult) :
        m_Hits(outResult)
    {
        m_Hits.Clear();
    }

    void AddHit(const JPH::BodyID& inBodyID) override
    {
        m_Hits.Add(PhysBodyID(inBodyID.GetIndexAndSequenceNumber()));
    }

    Vector<PhysBodyID>& m_Hits;
};

class GroupFilter : public JPH::GroupFilter
{
public:
    virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
    {
        const uint64_t id = static_cast<uint64_t>(group1.GetGroupID()) << 32 | group2.GetGroupID();
        return !m_IgnoreCollisions.Contains(id);
    }

    HashSet<uint64_t> m_IgnoreCollisions;
};

class ObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
    ObjectLayerFilter(CollisionFilter const& collisionFilter, uint32_t collisionGroup) :
        m_CollisionFilter(collisionFilter),
        m_CollisionGroup(collisionGroup)
    {}

    bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return m_CollisionFilter.ShouldCollide(m_CollisionGroup, static_cast<uint32_t>(inLayer) & 0xff);
    }

private:
    CollisionFilter const& m_CollisionFilter;
    uint32_t m_CollisionGroup;
};

PhysicsInterfaceImpl::PhysicsInterfaceImpl() :
    m_ObjectVsObjectLayerFilter(m_CollisionFilter)
{}

BodyUserData* PhysicsInterfaceImpl::CreateUserData()
{
    return new (m_UserDataAllocator.Allocate()) BodyUserData;
}

void PhysicsInterfaceImpl::DeleteUserData(BodyUserData* userData)
{
    if (userData)
    {
        userData->~BodyUserData();
        m_UserDataAllocator.Deallocate(userData);
    }
}

void PhysicsInterfaceImpl::QueueToAdd(JPH::Body* body, bool startAsSleeping)
{
    if (!startAsSleeping)
        m_QueueToAdd[0].Add(body->GetID());
    else
        m_QueueToAdd[1].Add(body->GetID());
}

void CopyShapeCastResult(ShapeCastResult& out, JPH::ShapeCastResult const& in)
{
    out.BodyID = PhysBodyID(in.mBodyID2.GetIndexAndSequenceNumber());
    out.ContactPointOn1 = ConvertVector(in.mContactPointOn1);
    out.ContactPointOn2 = ConvertVector(in.mContactPointOn2);
    out.PenetrationAxis = ConvertVector(in.mPenetrationAxis);
    out.PenetrationDepth = in.mPenetrationDepth;
    out.Fraction = in.mFraction;
    out.IsBackFaceHit = in.mIsBackFaceHit;
}

void CopyShapeCastResult(Vector<ShapeCastResult>& out, JPH::Array<JPH::ShapeCastResult> const& in)
{
    out.Resize(in.size());
    for (int i = 0; i < in.size(); i++)
        CopyShapeCastResult(out[i], in[i]);
}

void CopyShapeCollideResult(ShapeCollideResult& out, JPH::CollideShapeResult const& in)
{
    out.BodyID = PhysBodyID(in.mBodyID2.GetIndexAndSequenceNumber());
    out.ContactPointOn1 = ConvertVector(in.mContactPointOn1);
    out.ContactPointOn2 = ConvertVector(in.mContactPointOn2);
    out.PenetrationAxis = ConvertVector(in.mPenetrationAxis);
    out.PenetrationDepth = in.mPenetrationDepth;
}

void CopyShapeCollideResult(Vector<ShapeCollideResult>& out, JPH::Array<JPH::CollideShapeResult> const& in)
{
    out.Resize(in.size());
    for (int i = 0; i < in.size(); i++)
        CopyShapeCollideResult(out[i], in[i]);
}

bool PhysicsInterfaceImpl::CastShapeClosest(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = true;

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysSystem.GetNarrowPhaseQuery().CastShape(inShapeCast, settings, inBaseOffset, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    if (collector.HadHit())
        CopyShapeCastResult(outResult, collector.mHit);

    return collector.HadHit();
}

bool PhysicsInterfaceImpl::CastShape(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::ShapeCastSettings settings;
    settings.mBackFaceModeTriangles = settings.mBackFaceModeConvex = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mReturnDeepestPoint = false;

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
    m_PhysSystem.GetNarrowPhaseQuery().CastShape(inShapeCast, settings, inBaseOffset, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCastResult(outResult, collector.mHits);
    }

    return collector.HadHit();
}

void BodyActivationListener::OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
{
    BodyUserData* userdata = reinterpret_cast<BodyUserData*>(inBodyUserData);
    if (userdata->TypeID == ComponentTypeRegistry::GetComponentTypeID<DynamicBodyComponent>())
    {
        MutexGuard lockGuard(m_Mutex);
        m_ActiveBodies.SortedInsert(userdata->Component);
    }
}

void BodyActivationListener::OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData)
{
    BodyUserData* userdata = reinterpret_cast<BodyUserData*>(inBodyUserData);
    if (userdata->TypeID == ComponentTypeRegistry::GetComponentTypeID<DynamicBodyComponent>())
    {
        MutexGuard lockGuard(m_Mutex);
        m_ActiveBodies.SortedErase(userdata->Component);
        m_JustDeactivated.SortedInsert(userdata->Component);
    }
}

JPH::ValidateResult	ContactListener::OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult)
{
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void ContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
{
    if (inBody1.IsSensor() || inBody2.IsSensor())
    {
        TriggerComponent* trigger = nullptr;
        ComponentExtendedHandle target;

        if (inBody1.IsSensor())
        {
            trigger = ((BodyUserData*)inBody1.GetUserData())->TryGetComponent<TriggerComponent>(m_World);
            target  = ((BodyUserData*)inBody2.GetUserData())->GetExtendedHandle();
        }
        else if (inBody2.IsSensor())
        {
            trigger = ((BodyUserData*)inBody2.GetUserData())->TryGetComponent<TriggerComponent>(m_World);
            target  = ((BodyUserData*)inBody1.GetUserData())->GetExtendedHandle();
        }

        if (trigger && target)
        {
            uint32_t body1ID = inBody1.GetID().GetIndexAndSequenceNumber();
            uint32_t body2ID = inBody2.GetID().GetIndexAndSequenceNumber();
            uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

            std::lock_guard lock(m_Mutex);
            TriggerContact& contact = m_Triggers[contactID];
            contact.Trigger = Handle32<TriggerComponent>(trigger->GetHandle());
            contact.Target  = target;
            ++contact.Count;
            if (contact.Count == 1)
            {
                auto& event = m_TriggerEvents.EmplaceBack();
                event.Type = TriggerEvent::OnBeginOverlap;
                event.Trigger = contact.Trigger;
                event.Target = contact.Target;
            }
        }

        return;
    }

    // TODO: Other contacts
}

void ContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
{
#if 0
    uint32_t body1ID = inBody1.GetID().GetIndexAndSequenceNumber();
    uint32_t body2ID = inBody2.GetID().GetIndexAndSequenceNumber();
    uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

    std::lock_guard lock(m_Mutex);

    auto it = m_Triggers.Find(contactID);
    if (it != m_Triggers.End())
    {
        TriggerContact& contact = it->second;

        auto& event = m_TriggerEvents.EmplaceBack();
        event.Type = TriggerEvent::OnUpdateOverlap;
        event.Trigger = contact.Trigger;
        event.Target = contact.Target;

        m_Triggers.Erase(it);

        return;
    }
#endif
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair)
{
    uint32_t body1ID = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
    uint32_t body2ID = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();
    uint64_t contactID = body1ID < body2ID ? (body1ID | (uint64_t(body2ID) << 32)) : (body2ID | (uint64_t(body1ID) << 32));

    std::lock_guard lock(m_Mutex);

    auto it = m_Triggers.Find(contactID);
    if (it != m_Triggers.End())
    {
        TriggerContact& contact = it->second;
        HK_ASSERT(contact.Count > 0);
        contact.Count--;
        if (contact.Count == 0)
        {
            auto& event = m_TriggerEvents.EmplaceBack();
            event.Type = TriggerEvent::OnEndOverlap;
            event.Trigger = contact.Trigger;
            event.Target = contact.Target;

            m_Triggers.Erase(it);
        }

        return;
    }
}


PhysicsInterface::PhysicsInterface() :
    m_pImpl(new PhysicsInterfaceImpl)
{}

void PhysicsInterface::Initialize()
{
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    const JPH::uint cMaxBodies = 65536;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const JPH::uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    const JPH::uint cMaxBodyPairs = 65536;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    const JPH::uint cMaxContactConstraints = 10240;

    // TODO: Move to game setup/config/resource
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Character, true);
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Default, true);
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Platform, true);
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::TriggerCharacter, true);
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Default,   CollisionLayer::Default, true);
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Platform,  CollisionLayer::Default, true);
    m_pImpl->m_CollisionFilter.SetShouldCollide(CollisionLayer::Water,     CollisionLayer::Default, true);

    // Now we can create the actual physics system.
    m_pImpl->m_PhysSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_pImpl->m_BroadPhaseLayerInterface, m_pImpl->m_ObjectVsBroadPhaseLayerFilter, m_pImpl->m_ObjectVsObjectLayerFilter);

    m_pImpl->m_GroupFilter = new GroupFilter;
    m_pImpl->m_GroupFilter->AddRef();

    m_pImpl->m_PhysSystem.SetBodyActivationListener(&m_pImpl->m_BodyActivationListener);
    m_pImpl->m_PhysSystem.SetContactListener(&m_pImpl->m_ContactListener);

    m_pImpl->m_ContactListener.m_World = GetWorld();

    {
        TickFunction f;
        f.Desc.Name.FromString("Update Physics");
        f.Desc.TickEvenWhenPaused = true;
        f.Group = TickGroup::PhysicsUpdate;
        f.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
        f.Delegate.Bind(this, &PhysicsInterface::Update);
        RegisterTickFunction(f);
    }

    {
        TickFunction f;
        f.Desc.Name.FromString("Update Physics Post Transform");
        f.Desc.TickEvenWhenPaused = true;
        f.Group = TickGroup::PostTransform;
        f.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
        f.Delegate.Bind(this, &PhysicsInterface::PostTransform);
        RegisterTickFunction(f);
    }

    RegisterDebugDrawFunction({this, &PhysicsInterface::DrawDebug});
}

void PhysicsInterface::Deinitialize()
{
    m_pImpl->m_GroupFilter->Release();
    m_pImpl->m_GroupFilter = nullptr;
}

void PhysicsInterface::Purge()
{
    m_pImpl->m_QueueToAdd[0].Clear();
    m_pImpl->m_QueueToAdd[1].Clear();
}

HK_FORCEINLINE bool IsNearZero(Float3 const& vec, float inMaxDistSq = 1.0e-12f)
{
    return vec.LengthSqr() < inMaxDistSq;
}

void PhysicsInterface::Update()
{
    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();
    auto& dynamicBodyManager = GetWorld()->GetComponentManager<DynamicBodyComponent>();
    auto& triggerManager = GetWorld()->GetComponentManager<TriggerComponent>();

    for (int i = 0; i < 2; ++i)
    {
        if (!m_pImpl->m_QueueToAdd[i].IsEmpty())
        {
            auto addState = bodyInterface.AddBodiesPrepare(m_pImpl->m_QueueToAdd[i].ToPtr(), m_pImpl->m_QueueToAdd[i].Size());
            bodyInterface.AddBodiesFinalize(m_pImpl->m_QueueToAdd[i].ToPtr(), m_pImpl->m_QueueToAdd[i].Size(), addState, JPH::EActivation(i));

            m_pImpl->m_QueueToAdd[i].Clear();
        }
    }

    if (GetWorld()->GetTick().IsPaused)
        return;

    // Update character controller
    {
        float timeStep = GetWorld()->GetTick().FixedTimeStep;

        auto& characterControllerManager = GetWorld()->GetComponentManager<CharacterControllerComponent>();

        struct Visitor
        {
            float m_TimeStep;
            JPH::Vec3 m_Gravity;
            CollisionFilter const& m_CollisionFilter;
            JPH::BodyInterface& m_BodyInterface;

            Visitor(float timeStep, JPH::Vec3 const& gravity, CollisionFilter const& collisionFilter, JPH::BodyInterface& bodyInterface) :
                m_TimeStep(timeStep),
                m_Gravity(gravity),
                m_CollisionFilter(collisionFilter),
                m_BodyInterface(bodyInterface)
            {}

            HK_FORCEINLINE void Visit(CharacterControllerComponent& character)
            {
                GameObject* owner = character.GetOwner();

                auto& temp_allocator = *PhysicsModule::Get().GetTempAllocator();

                auto* phys_character = character.m_pImpl;

                // Smooth the player input
                character.DesiredVelocity = 0.25f * character.MovementDirection * character.MoveSpeed + 0.75f * character.DesiredVelocity;

                // True if the player intended to move
                phys_character->m_bAllowSliding = !IsNearZero(character.MovementDirection);

                // Determine new basic velocity
                JPH::Vec3 current_vertical_velocity = JPH::Vec3(0, phys_character->GetLinearVelocity().GetY(), 0);
                JPH::Vec3 ground_velocity = phys_character->GetGroundVelocity();
                JPH::Vec3 new_velocity;
                if (phys_character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround // If on ground
                    && (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f)       // And not moving away from ground
                {
                    // Assume velocity of ground when on ground
                    new_velocity = ground_velocity;

                    // Jump
                    if (character.Jump)
                        new_velocity += JPH::Vec3(0, character.JumpSpeed, 0);
                }
                else
                    new_velocity = current_vertical_velocity;

                // Gravity
                new_velocity += m_Gravity * m_TimeStep;

                // Player input
                new_velocity += ConvertVector(character.DesiredVelocity);

                // Update character velocity
                phys_character->SetLinearVelocity(new_velocity);

                // Stance switch
                //if (inSwitchStance)
                //    phys_character->SetShape(phys_character->GetShape() == mStandingShape? mCrouchingShape : mStandingShape, 1.5f * m_PhysicsInterface.GetPhysicsSettings().mPenetrationSlop, m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING), m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING), { }, *mTempAllocator);

                // Settings for our update function
                JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
                if (!character.EnableStickToFloor)
                    update_settings.mStickToFloorStepDown = JPH::Vec3::sZero();
                if (!character.EnableWalkStairs)
                    update_settings.mWalkStairsStepUp = JPH::Vec3::sZero();
                else
                    update_settings.mWalkStairsStepUp = JPH::Vec3(0,0.5f,0);

                class BroadphaseLayerFilter final : public JPH::BroadPhaseLayerFilter
                {
                public:
                    BroadphaseLayerFilter(uint32_t inCollisionMask) :
                        m_CollisionMask(inCollisionMask)
                    {}

                    uint32_t m_CollisionMask;

                    bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
                    {
                        return (HK_BIT(static_cast<uint8_t>(inLayer)) & m_CollisionMask) != 0;
                    }
                };
                BroadphaseLayerFilter broadphase_filter(
                    HK_BIT(uint32_t(BroadphaseLayer::Static)) |
                    HK_BIT(uint32_t(BroadphaseLayer::Dynamic)) |
                    HK_BIT(uint32_t(BroadphaseLayer::Character)));

                ObjectLayerFilter layer_filter(m_CollisionFilter, character.m_CollisionLayer);

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

                BodyFilter body_filter;
                body_filter.m_IgoreBodyID = JPH::BodyID(character.m_BodyID.ID);

                // Update the character position
                phys_character->ExtendedUpdate(m_TimeStep,
                    m_Gravity,
                    update_settings,
                    broadphase_filter, //m_PhysicsInterface.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                    layer_filter,      //m_PhysicsInterface.GetDefaultLayerFilter(Layers::MOVING),
                    body_filter,
                    {},
                    temp_allocator);

                m_BodyInterface.MoveKinematic(body_filter.m_IgoreBodyID, phys_character->GetPosition(), phys_character->GetRotation(), m_TimeStep);

                owner->SetWorldPositionAndRotation(ConvertVector(phys_character->GetPosition()), ConvertQuaternion(phys_character->GetRotation()));
            }
        };

        Visitor visitor(timeStep, m_pImpl->m_PhysSystem.GetGravity(), m_pImpl->m_CollisionFilter, bodyInterface);
        characterControllerManager.IterateComponents(visitor);
    }

    // Update dynmaic scaling
    // TODO: Update scale with lower framerate to save performance
    {
        for (auto& dynamicBody : m_pImpl->m_DynamicScaling)
        {
            DynamicBodyComponent* component = dynamicBodyManager.GetComponent(dynamicBody);

            auto* owner = component->GetOwner();

            Float3 const& scale = owner->GetWorldScale();
            if (scale != component->m_CachedScale)
            {
                component->m_CachedScale = scale;

                if (CollisionModel* model = component->m_CollisionModel)
                {
                    const bool bUpdateMassProperties = false;
                    bodyInterface.SetShape(JPH::BodyID(component->m_BodyID.ID), model->Instatiate(scale), bUpdateMassProperties, JPH::EActivation::Activate);
                }
            }
        }
    }

    // Move triggers bodies
    {
        for (auto& trigger : m_pImpl->m_MovableTriggers)
        {
            TriggerComponent* component = triggerManager.GetComponent(trigger);

            auto* owner = component->GetOwner();

            owner->UpdateWorldTransform();

            JPH::Vec3 position = ConvertVector(owner->GetWorldPosition());
            JPH::Quat rotation = ConvertQuaternion(owner->GetWorldRotation()).Normalized();

            bodyInterface.SetPositionAndRotation(JPH::BodyID(component->m_BodyID.ID), position, rotation, JPH::EActivation::Activate);
        }
    }

    // Move kinematic bodies
    {
        float timeStep = GetWorld()->GetTick().FixedTimeStep;

        for (auto& kinematicBody : m_pImpl->m_KinematicBodies)
        {
            DynamicBodyComponent* component = dynamicBodyManager.GetComponent(kinematicBody);

            auto* owner = component->GetOwner();

            owner->UpdateWorldTransform();

            JPH::Vec3 position = ConvertVector(owner->GetWorldPosition());
            JPH::Quat rotation = ConvertQuaternion(owner->GetWorldRotation()).Normalized();

            bodyInterface.MoveKinematic(JPH::BodyID(component->m_BodyID.ID), position, rotation, timeStep);
        }
    }

    // Update dynamic bodies
    {
        for (auto& msg : m_pImpl->m_DynamicBodyMessageQueue)
        {
            DynamicBodyComponent* component = dynamicBodyManager.GetComponent(msg.m_Component);
            if (!component)
                continue;

            if (component->IsKinematic())
                continue;

            auto bodyID = JPH::BodyID(component->m_BodyID.ID);
            if (bodyID.IsInvalid())
                continue;

            switch (msg.m_MsgType)
            {
            case DynamicBodyMessage::AddForce:
                bodyInterface.AddForce(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            case DynamicBodyMessage::AddForceAtPosition:
                bodyInterface.AddForce(bodyID, ConvertVector(msg.m_Data[0]), ConvertVector(msg.m_Data[1]));
                break;
            case DynamicBodyMessage::AddTorque:
                bodyInterface.AddTorque(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            case DynamicBodyMessage::AddForceAndTorque:
                bodyInterface.AddForceAndTorque(bodyID, ConvertVector(msg.m_Data[0]), ConvertVector(msg.m_Data[1]));
                break;
            case DynamicBodyMessage::AddImpulse:
                bodyInterface.AddImpulse(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            case DynamicBodyMessage::AddImpulseAtPosition:
                bodyInterface.AddImpulse(bodyID, ConvertVector(msg.m_Data[0]), ConvertVector(msg.m_Data[1]));
                break;
            case DynamicBodyMessage::AddAngularImpulse:
                bodyInterface.AddAngularImpulse(bodyID, ConvertVector(msg.m_Data[0]));
                break;
            }
        }
        m_pImpl->m_DynamicBodyMessageQueue.Clear();
    }

    // Update water bodies
    {
        float timeStep = GetWorld()->GetTick().FixedTimeStep;

        // Broadphase results, will apply buoyancy to any body that intersects with the water volume
        class Collector : public JPH::CollideShapeBodyCollector
        {
        public:
            Collector(JPH::PhysicsSystem& inSystem, JPH::Vec3Arg inSurfaceNormal, float inDeltaTime) :
                mSystem(inSystem), mSurfacePosition(JPH::RVec3::sZero()), mSurfaceNormal(inSurfaceNormal), mDeltaTime(inDeltaTime) {}

            void SetSurfacePosition(Float3 const& inSurfacePosition)
            {
                mSurfacePosition = ConvertVector(inSurfacePosition);
            }

            void AddHit(const JPH::BodyID& inBodyID) override
            {
                JPH::BodyLockWrite lock(mSystem.GetBodyLockInterface(), inBodyID);
                JPH::Body& body = lock.GetBody();
                if (body.IsActive() && body.GetMotionType() == JPH::EMotionType::Dynamic)
                    body.ApplyBuoyancyImpulse(mSurfacePosition, mSurfaceNormal, 1.1f, 0.3f, 0.05f, JPH::Vec3::sZero(), mSystem.GetGravity(), mDeltaTime);

                //if (body.GetMotionType() != JPH::EMotionType::Dynamic)
                //    LOG("Motion type {}\n", body.GetMotionType() == JPH::EMotionType::Static ? "Static" : "Kinematic");
            }

        private:
            JPH::PhysicsSystem& mSystem;
            JPH::RVec3 mSurfacePosition;
            JPH::Vec3 mSurfaceNormal;
            float mDeltaTime;
        };

        Collector collector(m_pImpl->m_PhysSystem, JPH::Vec3::sAxisY(), timeStep);

        auto& waterVolumeManager = GetWorld()->GetComponentManager<WaterVolumeComponent>();

        struct Visitor
        {
            JPH::BroadPhaseQuery const& m_BroadPhaseQuery;
            CollisionFilter const& m_CollisionFilter;
            Collector& m_Collector;

            Visitor(JPH::BroadPhaseQuery const& broadPhaseQuery, CollisionFilter const& collisionFilter, Collector& collector) :
                m_BroadPhaseQuery(broadPhaseQuery),
                m_CollisionFilter(collisionFilter),
                m_Collector(collector)
            {}

            HK_FORCEINLINE void Visit(WaterVolumeComponent& waterVolume)
            {
                if (waterVolume.HalfExtents.X <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Y <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Z <= std::numeric_limits<float>::epsilon())
                    return;

                GameObject* owner = waterVolume.GetOwner();

                Float3 worldPosition = owner->GetWorldPosition();
                Float3 scaledExtents = waterVolume.HalfExtents * owner->GetWorldScale();

                Float3 mins = worldPosition - scaledExtents;
                Float3 maxs = worldPosition + scaledExtents;

                JPH::AABox waterBox(ConvertVector(mins), ConvertVector(maxs));

                Float3 surfacePos = worldPosition;
                surfacePos.Y = maxs.Y;

                m_Collector.SetSurfacePosition(surfacePos);

                ObjectLayerFilter layer_filter(m_CollisionFilter, waterVolume.m_CollisionLayer);

                m_BroadPhaseQuery.CollideAABox(waterBox, m_Collector, JPH::SpecifiedBroadPhaseLayerFilter(JPH::BroadPhaseLayer(ToUnderlying(BroadphaseLayer::Dynamic))), layer_filter);
            }
        };

        JPH::BroadPhaseQuery const& broadphaseQuery = m_pImpl->m_PhysSystem.GetBroadPhaseQuery();

        Visitor visitor(broadphaseQuery, m_pImpl->m_CollisionFilter, collector);
        waterVolumeManager.IterateComponents(visitor);
    }

    // Simulation step
    {
        const int numCollisionSteps = 1;
        auto& physicsModule = PhysicsModule::Get();

        m_pImpl->m_PhysSystem.Update(GetWorld()->GetTick().FixedTimeStep, numCollisionSteps, physicsModule.GetTempAllocator(), physicsModule.GetJobSystemThreadPool());
    }

    // Capture active bodies transform
    {
        for (uint32_t handle : m_pImpl->m_BodyActivationListener.m_ActiveBodies)
        {
            DynamicBodyComponent* body = dynamicBodyManager.GetComponent(Handle32<DynamicBodyComponent>(handle));

            if (body && !body->IsKinematic())
            {
                JPH::Vec3 position;
                JPH::Quat rotation;

                bodyInterface.GetPositionAndRotation(JPH::BodyID(body->m_BodyID.ID), position, rotation);

                body->GetOwner()->SetWorldPositionAndRotation(ConvertVector(position), ConvertQuaternion(rotation));
            }
        }

        for (uint32_t handle : m_pImpl->m_BodyActivationListener.m_JustDeactivated)
        {
            DynamicBodyComponent* body = dynamicBodyManager.GetComponent(Handle32<DynamicBodyComponent>(handle));

            if (body && !body->IsKinematic())
            {
                JPH::Vec3 position;
                JPH::Quat rotation;

                bodyInterface.GetPositionAndRotation(JPH::BodyID(body->m_BodyID.ID), position, rotation);

                body->GetOwner()->SetWorldPositionAndRotation(ConvertVector(position), ConvertQuaternion(rotation));
            }
        }

        //LOG("Active {} from {}, just deactivated {}, kinematic {}\n", m_pImpl->m_BodyActivationListener.m_ActiveBodies.Size(), dynamicBodyManager.GetComponentCount(), m_pImpl->m_BodyActivationListener.m_JustDeactivated.Size(), m_pImpl->m_KinematicBodies.Size());
        m_pImpl->m_BodyActivationListener.m_JustDeactivated.Clear();
    }
}

void PhysicsInterface::PostTransform()
{
    for (auto& event : m_pImpl->m_ContactListener.m_TriggerEvents)
    {
        TriggerComponent* trigger = GetWorld()->GetComponent(event.Trigger);

        switch (event.Type)
        {
            case ContactListener::TriggerEvent::OnBeginOverlap:
            {
                if (auto componentManager = GetWorld()->TryGetComponentManager(event.Target.TypeID))
                {
                    if (auto component = componentManager->GetComponent(event.Target.Handle))
                    {
                        BodyComponent* body = static_cast<BodyComponent*>(component);
                        World::DispatchEvent<Event_OnBeginOverlap>(trigger->GetOwner(), body);
                    }
                }
                break;
            }
            case ContactListener::TriggerEvent::OnEndOverlap:
            {
                if (auto componentManager = GetWorld()->TryGetComponentManager(event.Target.TypeID))
                {
                    if (auto component = componentManager->GetComponent(event.Target.Handle))
                    {
                        BodyComponent* body = static_cast<BodyComponent*>(component);
                        World::DispatchEvent<Event_OnEndOverlap>(trigger->GetOwner(), body);
                    }
                }
                break;
            }
        }
    }
    m_pImpl->m_ContactListener.m_TriggerEvents.Clear();
}

template <typename T>
void PhysicsInterface::DrawRigidBody(DebugRenderer& renderer, PhysBodyID bodyID, T* rigidBody)
{
    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

    GameObject* owner = rigidBody->GetOwner();

    m_DebugDrawVertices.Clear();
    m_DebugDrawIndices.Clear();

    rigidBody->m_CollisionModel->GatherGeometry(m_DebugDrawVertices, m_DebugDrawIndices);

    JPH::Vec3 position;
    JPH::Quat rotation;
    bodyInterface.GetPositionAndRotation(JPH::BodyID(bodyID.ID), position, rotation);

    Float3x4 transform_matrix;
    transform_matrix.Compose(ConvertVector(position),
        ConvertQuaternion(rotation).ToMatrix3x3(),
        rigidBody->m_CollisionModel->GetValidScale(owner->GetWorldScale())); // TODO: Use m_CachedScale

    renderer.PushTransform(transform_matrix);
    renderer.DrawTriangleSoupWireframe(m_DebugDrawVertices, m_DebugDrawIndices);
    renderer.PopTransform();
}

void PhysicsInterface::DrawHeightField(DebugRenderer& renderer, PhysBodyID bodyID, HeightFieldComponent* heightfield)
{
    auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

    m_DebugDrawVertices.Clear();
    m_DebugDrawIndices.Clear();

    JPH::Vec3 position;
    JPH::Quat rotation;
    bodyInterface.GetPositionAndRotation(JPH::BodyID(bodyID.ID), position, rotation);

    Float3x4 transform_matrix;
    transform_matrix.Compose(ConvertVector(position), ConvertQuaternion(rotation).ToMatrix3x3());

    Float3x4 transform_matrix_inv = transform_matrix.Inversed();
    Float3 local_view_position = transform_matrix_inv * renderer.GetRenderView()->ViewPosition;

    BvAxisAlignedBox local_bounds(local_view_position - 5, local_view_position + 5);

    local_bounds.Mins.Y = -FLT_MAX;
    local_bounds.Maxs.Y = FLT_MAX;

    heightfield->m_CollisionModel->GatherGeometry(local_bounds, m_DebugDrawVertices, m_DebugDrawIndices);

    renderer.PushTransform(transform_matrix);
    renderer.DrawTriangleSoupWireframe(m_DebugDrawVertices, m_DebugDrawIndices);
    renderer.PopTransform();
}

void PhysicsInterface::DrawDebug(DebugRenderer& renderer)
{
    const float visibilityHalfSize = 100;// 5;

    if (com_DrawCollisionModel)
    {
        ShapeOverlapFilter filter;
        filter.BroadphaseLayers
            .AddLayer(BroadphaseLayer::Static)
            .AddLayer(BroadphaseLayer::Dynamic)
            .AddLayer(BroadphaseLayer::Trigger);

        OverlapBox(renderer.GetRenderView()->ViewPosition, Float3(visibilityHalfSize), Quat::Identity(), m_BodyQueryResult, filter);

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        renderer.SetColor(Color4::White());
        renderer.SetDepthTest(false);
        for (PhysBodyID bodyID : m_BodyQueryResult)
        {
            BodyUserData* userData = reinterpret_cast<BodyUserData*>(bodyInterface.GetUserData(JPH::BodyID(bodyID.ID)));

            if (auto staticBody = userData->TryGetComponent<StaticBodyComponent>(GetWorld()))
            {
                DrawRigidBody(renderer, bodyID, staticBody);
            }
            else if (auto dynamicBody = userData->TryGetComponent<DynamicBodyComponent>(GetWorld()))
            {
                DrawRigidBody(renderer, bodyID, dynamicBody);
            }
            else if (auto heightfield = userData->TryGetComponent<HeightFieldComponent>(GetWorld()))
            {
                DrawHeightField(renderer, bodyID, heightfield);
            }
        }
    }

    if (com_DrawCollisionShape)
    {
        ShapeOverlapFilter filter;
        filter.BroadphaseLayers
            .AddLayer(BroadphaseLayer::Static)
            .AddLayer(BroadphaseLayer::Dynamic)
            .AddLayer(BroadphaseLayer::Trigger);

        OverlapBox(renderer.GetRenderView()->ViewPosition, Float3(visibilityHalfSize), Quat::Identity(), m_BodyQueryResult, filter);

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        renderer.SetDepthTest(false);
        for (PhysBodyID bodyID : m_BodyQueryResult)
        {
            JPH::BodyID joltBodyID(bodyID.ID);
            BodyUserData* userData = reinterpret_cast<BodyUserData*>(bodyInterface.GetUserData(joltBodyID));

            CollisionModel* model;
            Float3 scale;

            if (auto staticBody = userData->TryGetComponent<StaticBodyComponent>(GetWorld()))
            {
                model = staticBody->m_CollisionModel;
                scale = Float3(1);
            }
            else if (auto dynamicBody = userData->TryGetComponent<DynamicBodyComponent>(GetWorld()))
            {
                model = dynamicBody->m_CollisionModel;
                scale = dynamicBody->m_CachedScale;
            }
            else
                continue;

            auto motionType = bodyInterface.GetMotionType(joltBodyID);
            switch (motionType)
            {
            case JPH::EMotionType::Static:
                renderer.SetColor({0.6f, 0.6f, 0.6f, 1});
                break;

            case JPH::EMotionType::Kinematic:
                renderer.SetColor({0, 1, 1, 1});
                break;

            case JPH::EMotionType::Dynamic:
                renderer.SetColor(bodyInterface.IsActive(joltBodyID) ? Color4(1, 0, 1, 1) : Color4(1, 1, 1, 1));
                break;
            }

            JPH::Vec3 position;
            JPH::Quat rotation;
            bodyInterface.GetPositionAndRotation(joltBodyID, position, rotation);

            Float3x3 rotation_matrix = ConvertQuaternion(rotation).ToMatrix3x3();

            if (model)
            {
                Float3x4 transform_matrix;
                transform_matrix.Compose(ConvertVector(position),
                                         rotation_matrix,
                                         model->GetValidScale(scale));

                model->DrawDebug(renderer, transform_matrix);
            }

            if (motionType == JPH::EMotionType::Dynamic)
            {
                renderer.SetColor({1, 1, 1, 1});
                renderer.DrawAxis(ConvertVector(position), rotation_matrix[0], rotation_matrix[1], rotation_matrix[2], Float3(0.25f));
            }
        }
    }

    if (com_DrawWaterVolume)
    {
        auto& waterVolumeManager = GetWorld()->GetComponentManager<WaterVolumeComponent>();

        renderer.SetDepthTest(true);
        renderer.SetColor({0, 0, 1, 0.5f});

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;

            Visitor(DebugRenderer& debugRenderer) :
                m_DebugRenderer(debugRenderer)
            {}

            HK_FORCEINLINE void Visit(WaterVolumeComponent& waterVolume)
            {
                if (waterVolume.HalfExtents.X <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Y <= std::numeric_limits<float>::epsilon() ||
                    waterVolume.HalfExtents.Z <= std::numeric_limits<float>::epsilon())
                    return;

                GameObject* owner = waterVolume.GetOwner();

                Float3 worldPosition = owner->GetWorldPosition();
                Float3 scaledExtents = waterVolume.HalfExtents * owner->GetWorldScale();

                m_DebugRenderer.DrawBoxFilled(worldPosition, scaledExtents, true);
            }
        };

        Visitor visitor(renderer);
        waterVolumeManager.IterateComponents(visitor);
    }

    if (com_DrawTriggers)
    {
        auto& triggerManager = GetWorld()->GetComponentManager<TriggerComponent>();

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;
            Vector<Float3>& m_DebugDrawVertices;
            Vector<unsigned int>& m_DebugDrawIndices;
            JPH::BodyInterface& m_BodyInterface;

            Visitor(DebugRenderer& debugRenderer, Vector<Float3>& debugDrawVertices, Vector<unsigned int>& debugDrawIndices, JPH::BodyInterface& bodyInterface) :
                m_DebugRenderer(debugRenderer), m_DebugDrawVertices(debugDrawVertices), m_DebugDrawIndices(debugDrawIndices), m_BodyInterface(bodyInterface)
            {}

            HK_FORCEINLINE void Visit(TriggerComponent& body)
            {
                auto* owner = body.GetOwner();
                auto* model = body.m_CollisionModel.RawPtr();

                if (model)
                {
                    m_DebugDrawVertices.Clear();
                    m_DebugDrawIndices.Clear();

                    model->GatherGeometry(m_DebugDrawVertices, m_DebugDrawIndices);

                    JPH::Vec3 position;
                    JPH::Quat rotation;
                    m_BodyInterface.GetPositionAndRotation(JPH::BodyID(body.m_BodyID.ID), position, rotation);

                    Float3x4 transform;
                    transform.Compose(ConvertVector(position), ConvertQuaternion(rotation).ToMatrix3x3(), model->GetValidScale(owner->GetWorldScale())); // TODO: Use cached scale

                    m_DebugRenderer.PushTransform(transform);
                    m_DebugRenderer.DrawTriangleSoup(m_DebugDrawVertices, m_DebugDrawIndices);
                    m_DebugRenderer.PopTransform();
                }
            }
        };

        renderer.SetDepthTest(true);
        renderer.SetColor({0, 1, 0, 0.5f});

        Visitor visitor(renderer, m_DebugDrawVertices, m_DebugDrawIndices, bodyInterface);
        triggerManager.IterateComponents(visitor);
    }

    if (com_DrawCenterOfMass)
    {
        auto& dynamicBodyManager = GetWorld()->GetComponentManager<DynamicBodyComponent>();

        auto& bodyInterface = m_pImpl->m_PhysSystem.GetBodyInterface();

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;
            JPH::BodyInterface& m_BodyInterface;

            Visitor(DebugRenderer& debugRenderer, JPH::BodyInterface& bodyInterface) :
                m_DebugRenderer(debugRenderer), m_BodyInterface(bodyInterface)
            {}

            HK_FORCEINLINE void Visit(DynamicBodyComponent& body)
            {
                if (!body.IsKinematic())
                {
                    auto* owner = body.GetOwner();
                    auto* model = body.m_CollisionModel.RawPtr();

                    Float3x4 transform;
                    transform.Compose(owner->GetWorldPosition(),
                        owner->GetWorldRotation().ToMatrix3x3(),
                        model->GetValidScale(owner->GetWorldScale())); // TODO: Use m_CachedScale

                    Float3 center_of_mass_pos = transform * model->GetCenterOfMass();
                    Float3 center_of_mass_pos2 = ConvertVector(m_BodyInterface.GetCenterOfMassPosition(JPH::BodyID(body.m_BodyID.ID)));

                    m_DebugRenderer.SetColor({1, 1, 1, 1});
                    m_DebugRenderer.DrawBoxFilled(center_of_mass_pos, Float3(0.05f));

                    m_DebugRenderer.SetColor({1, 0, 0, 1});
                    m_DebugRenderer.DrawBoxFilled(center_of_mass_pos2, Float3(0.05f));
                }
            }
        };

        renderer.SetDepthTest(false);

        Visitor visitor(renderer, bodyInterface);
        dynamicBodyManager.IterateComponents(visitor);
    }

    if (com_DrawCharacterController)
    {
        auto& characterControllerManager = GetWorld()->GetComponentManager<CharacterControllerComponent>();

        struct Visitor
        {
            DebugRenderer& m_DebugRenderer;

            Visitor(DebugRenderer& debugRenderer) :
                m_DebugRenderer(debugRenderer)
            {}

            HK_FORCEINLINE void Visit(CharacterControllerComponent& character)
            {
                if (character.IsInitialized())
                {
                    Float3x4 transform_matrix;
                    transform_matrix.Compose(ConvertVector(character.m_pImpl->GetPosition()), ConvertQuaternion(character.m_pImpl->GetRotation()).ToMatrix3x3());

                    DrawShape(m_DebugRenderer, character.m_pImpl->GetShape(), transform_matrix);
                }
            }
        };

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 1, 0, 1));

        Visitor visitor(renderer);
        characterControllerManager.IterateComponents(visitor);
    }
}

namespace
{

JPH::RVec3 CalcBaseOffset(JPH::Vec3 const& pos, JPH::Vec3 const& direction)
{
    // Define a base offset that is halfway the probe to test getting the collision results relative to some offset.
    // Note that this is not necessarily the best choice for a base offset, but we want something that's not zero
    // and not the start of the collision test either to ensure that we'll see errors in the algorithm.
    return pos + 0.5f * direction;
}

} // namespace

bool PhysicsInterface::CastRayClosest(Float3 const& inRayStart, Float3 const& inRayDir, RayCastResult& outResult, RayCastFilter const& inFilter)
{
    JPH::RRayCast raycast;
    raycast.mOrigin = ConvertVector(inRayStart);
    raycast.mDirection = ConvertVector(inRayDir);

    JPH::RayCastResult hit;
    if (inFilter.IgonreBackFaces)
    {
        JPH::RayCastSettings settings;

        // How backfacing triangles should be treated
        //settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
        settings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;

        // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
        settings.mTreatConvexAsSolid = true;

        JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
        m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CastRay(raycast, settings, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

        if (!collector.HadHit())
            return false;

        hit = collector.mHit;
    }
    else
    {
        if (!m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CastRay(raycast, hit, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())))
            return false;
    }

    outResult.BodyID = PhysBodyID(hit.mBodyID.GetIndexAndSequenceNumber());
    outResult.Fraction = hit.mFraction;

    if (inFilter.CalcSurfcaceNormal)
    {
        JPH::BodyLockRead lock(m_pImpl->m_PhysSystem.GetBodyLockInterface(), hit.mBodyID);
        JPH::Body const& body = lock.GetBody();

        auto normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, raycast.GetPointOnRay(outResult.Fraction));
        outResult.Normal = ConvertVector(normal);
    }

    return true;
}

bool PhysicsInterface::CastRay(Float3 const& inRayStart, Float3 const& inRayDir, Vector<RayCastResult>& outResult, RayCastFilter const& inFilter)
{
    JPH::RRayCast raycast;
    raycast.mOrigin = ConvertVector(inRayStart);
    raycast.mDirection = ConvertVector(inRayDir);

    JPH::RayCastSettings settings;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    // If convex shapes should be treated as solid. When true, a ray starting inside a convex shape will generate a hit at fraction 0.
    settings.mTreatConvexAsSolid = true;

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CastRay(raycast, settings, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        // Order hits on closest first
        if (inFilter.SortByDistance)
            collector.Sort();

        outResult.Reserve(collector.mHits.size());
        for (auto& hit : collector.mHits)
        {
            RayCastResult& r = outResult.Add();
            r.BodyID = PhysBodyID(hit.mBodyID.GetIndexAndSequenceNumber());
            r.Fraction = hit.mFraction;

            if (inFilter.CalcSurfcaceNormal)
            {
                JPH::BodyLockRead lock(m_pImpl->m_PhysSystem.GetBodyLockInterface(), hit.mBodyID);
                JPH::Body const& body = lock.GetBody();

                auto normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, raycast.GetPointOnRay(hit.mFraction));
                r.Normal = ConvertVector(normal);
            }
        }
    }

    return collector.HadHit();
}

bool PhysicsInterface::CastBoxClosest(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastBox(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastBoxMinMaxClosest(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return m_pImpl->CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos), direction);

    return m_pImpl->CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastSphereClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return m_pImpl->CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastSphere(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(pos) /* * rotation*/, direction);

    return m_pImpl->CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCapsuleClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCapsule(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCylinderClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShapeClosest(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

bool PhysicsInterface::CastCylinder(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inRayStart);
    JPH::Vec3 direction = ConvertVector(inRayDir);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(rotation, pos), direction);

    return m_pImpl->CastShape(shape_cast, CalcBaseOffset(pos, direction), outResult, inFilter);
}

void PhysicsInterface::OverlapBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    if (inRotation == Quat::Identity())
    {
        m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideAABox(JPH::AABox(ConvertVector(inPosition - inHalfExtent), ConvertVector(inPosition + inHalfExtent)),
            collector,
            BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
    }
    else
    {
        JPH::OrientedBox oriented_box;
        oriented_box.mOrientation.SetTranslation(ConvertVector(inPosition));
        oriented_box.mOrientation.SetRotation(ConvertMatrix(inRotation.ToMatrix4x4()));
        oriented_box.mHalfExtents = ConvertVector(inHalfExtent);

        m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideOrientedBox(oriented_box, collector, BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
    }
}

void PhysicsInterface::OverlapBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideAABox(JPH::AABox(ConvertVector(inMins), ConvertVector(inMaxs)),
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
}

void PhysicsInterface::OverlapSphere(Float3 const& inPosition, float inRadius, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollideSphere(ConvertVector(inPosition),
        inRadius,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));
}

void PhysicsInterface::OverlapPoint(Float3 const& inPosition, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter)
{
    BroadphaseBodyCollector collector(outResult);
    m_pImpl->m_PhysSystem.GetBroadPhaseQuery().CollidePoint(ConvertVector(inPosition),
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
    /* TODO: objectLayerFilter */);
}

bool PhysicsInterface::CheckBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckSphere(Float3 const& inPosition, float inRadius, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    return collector.HadHit();
}

bool PhysicsInterface::CheckPoint(Float3 const& inPosition, BroadphaseLayerMask inBroadphaseLayers)
{
    JPH::AnyHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(inPosition), collector, BroadphaseLayerFilter(inBroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);
    return collector.HadHit();
}

void PhysicsInterface::CollideBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector(inHalfExtent));

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::BoxShape shape(ConvertVector((inMaxs - inMins) * 0.5f));

    JPH::Vec3 pos = ConvertVector((inMins + inMaxs) * 0.5f);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = JPH::RVec3::sZero();

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideSphere(Float3 const& inPosition, float inRadius, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::SphereShape shape(inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sTranslation(pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CapsuleShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollideCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter)
{
    JPH::CylinderShape shape(inHalfHeight, inRadius);

    JPH::Vec3 pos = ConvertVector(inPosition);
    JPH::Quat rotation = ConvertQuaternion(inRotation);

    JPH::CollideShapeSettings settings;

    // When > 0 contacts in the vicinity of the query shape can be found. All nearest contacts that are not further away than this distance will be found (uint: meter)
    settings.mMaxSeparationDistance = 0.0f;

    // How backfacing triangles should be treated
    settings.mBackFaceMode = inFilter.IgonreBackFaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // All hit results will be returned relative to this offset, can be zero to get results in world position,
    // but when you 're testing far from the origin you get better precision by picking a position that' s closer e.g.inCenterOfMassTransform.GetTranslation()
    // since floats are most accurate near the origin
    JPH::RVec3 base_offset = pos;

    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollideShape(&shape,
        JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(rotation, pos),
        settings,
        base_offset,
        collector,
        BroadphaseLayerFilter(inFilter.BroadphaseLayers.Get()));

    outResult.Clear();
    if (collector.HadHit())
    {
        if (inFilter.SortByDistance)
            collector.Sort();

        CopyShapeCollideResult(outResult, collector.mHits);
    }
}

void PhysicsInterface::CollidePoint(Float3 const& inPosition, Vector<PhysBodyID>& outResult, BroadphaseLayerMask inBroadphaseLayers)
{
    JPH::AllHitCollisionCollector<JPH::CollidePointCollector> collector;
    m_pImpl->m_PhysSystem.GetNarrowPhaseQuery().CollidePoint(ConvertVector(inPosition), collector, BroadphaseLayerFilter(inBroadphaseLayers.Get())
    /* TODO: objectLayerFilter, bodyFilter, shapeFilter*/);

    outResult.Clear();
    if (collector.HadHit())
    {
        outResult.Reserve(collector.mHits.size());
        for (JPH::CollidePointResult const& hit : collector.mHits)
            outResult.Add(PhysBodyID(hit.mBodyID.GetIndexAndSequenceNumber()));
    }
}

void PhysicsInterface::SetGravity(Float3 const inGravity)
{
    return m_pImpl->m_PhysSystem.SetGravity(ConvertVector(inGravity));
}

Float3 PhysicsInterface::GetGravity() const
{
    return ConvertVector(m_pImpl->m_PhysSystem.GetGravity());
}

HK_NAMESPACE_END
