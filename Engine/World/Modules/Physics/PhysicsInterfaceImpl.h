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

#pragma once

#include "PhysicsInterface.h"
#include "CollisionFilter.h"
#include <Engine/World/Component.h>
#include <Engine/World/World.h>
#include <Engine/Core/Handle.h>
#include <Engine/Core/Allocators/PoolAllocator.h>
#include "JoltPhysics.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

HK_NAMESPACE_BEGIN

class BodyUserData
{
public:
    ComponentTypeID TypeID;
    ComponentHandle Component;

    ComponentExtendedHandle GetExtendedHandle() const
    {
        return {Component, TypeID};
    }

    template <typename ComponentType>
    void Initialize(ComponentType* component)
    {
        TypeID = ComponentTypeRegistry::GetComponentTypeID<ComponentType>();
        Component = component->GetHandle();
    }

    class Component* TryGetComponent(World* world)
    {
        ComponentManagerBase* componentManager = world->TryGetComponentManager(TypeID);
        if (!componentManager)
            return nullptr;
        return componentManager->GetComponent(Component);
    }

    template <typename ComponentType>
    ComponentType* TryGetComponent(World* world)
    {
        if (TypeID == ComponentTypeRegistry::GetComponentTypeID<ComponentType>())
            return world->GetComponentManager<ComponentType>().GetComponent(Handle32<ComponentType>(Component));
        return nullptr;
    }
};

/// BroadPhaseLayerInterface implementation
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return static_cast<JPH::uint>(BroadphaseLayer::Max);
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return JPH::BroadPhaseLayer(inLayer >> 8);
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        using namespace JPH;
        switch ((BroadPhaseLayer::Type)inLayer)
        {
        case BroadphaseLayer::Static: return "Static";
        case BroadphaseLayer::Dynamic: return "Dynamic";
        case BroadphaseLayer::Trigger: return "Trigger";
        case BroadphaseLayer::Character: return "Character";
        default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    uint32_t BroadphaseCollisionMask(JPH::BroadPhaseLayer broadphaseLayer) const
    {
        static constexpr Hk::Array<uint32_t, size_t(BroadphaseLayer::Max)> mask =
        {
            // Static
            HK_BIT(uint32_t(BroadphaseLayer::Dynamic)) | HK_BIT(uint32_t(BroadphaseLayer::Character)),

            // Dynamic
            HK_BIT(uint32_t(BroadphaseLayer::Static)) | HK_BIT(uint32_t(BroadphaseLayer::Dynamic)) | HK_BIT(uint32_t(BroadphaseLayer::Trigger)) | HK_BIT(uint32_t(BroadphaseLayer::Character)),

            // Trigger
            HK_BIT(uint32_t(BroadphaseLayer::Dynamic)) | HK_BIT(uint32_t(BroadphaseLayer::Character)),

            // Character
            HK_BIT(uint32_t(BroadphaseLayer::Character)) | HK_BIT(uint32_t(BroadphaseLayer::Trigger)) | HK_BIT(uint32_t(BroadphaseLayer::Static)) | HK_BIT(uint32_t(BroadphaseLayer::Dynamic)),
        };

        return mask[broadphaseLayer.operator unsigned char()];
    }

    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        uint32_t objectBroadphaseMask = HK_BIT(inLayer1 >> 8);
        uint32_t layerBroadphaseMask = BroadphaseCollisionMask(inLayer2);

        return (objectBroadphaseMask & layerBroadphaseMask) != 0;
    }
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
{
public:
    CollisionFilter const&  m_CollisionFilter;

    ObjectLayerPairFilterImpl(CollisionFilter const& collisionFilter) :
        m_CollisionFilter(collisionFilter)
    {}

    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        return m_CollisionFilter.ShouldCollide(static_cast<uint32_t>(inObject1) & 0xff, static_cast<uint32_t>(inObject2) & 0xff);
    }
};

class BodyActivationListener final : public JPH::BodyActivationListener
{
public:
    // FIXME: Use set?
    Mutex                   m_Mutex;
    Vector<uint32_t>       m_ActiveBodies;
    Vector<uint32_t>       m_JustDeactivated;

    virtual void			OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override;

    virtual void			OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override;
};

class ContactListener final : public JPH::ContactListener
{
public:
    World*                  m_World;

    JPH::Mutex              m_Mutex;

    struct TriggerContact
    {
        Handle32<class TriggerComponent>    Trigger;
        ComponentExtendedHandle             Target;
        uint32_t                            Count;
    };
    HashMap<uint64_t, TriggerContact> m_Triggers;

  

    struct TriggerEvent
    {
        enum EventType
        {
            OnBeginOverlap,
            OnUpdateOverlap,
            OnEndOverlap
        };

        EventType                           Type;
        Handle32<class TriggerComponent>    Trigger;
        ComponentExtendedHandle             Target;
    };

    Vector<TriggerEvent> m_TriggerEvents;

    virtual JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override;

    virtual void			OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;

    virtual void			OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;

    virtual void			OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;
};

class DynamicBodyComponent;
struct DynamicBodyMessage
{
    enum MessageType
    {
        AddForce,
        AddForceAtPosition,
        AddTorque,
        AddForceAndTorque,
        AddImpulse,
        AddImpulseAtPosition,
        AddAngularImpulse
    };

    DynamicBodyMessage(Handle32<DynamicBodyComponent> component, MessageType msgType, Float3 const& v0, Float3 const& v1) :
        m_Component(component),
        m_MsgType(msgType)
    {
        m_Data[0] = v0;
        m_Data[1] = v1;
    }

    DynamicBodyMessage(Handle32<DynamicBodyComponent> component, MessageType msgType, Float3 const& v0) :
        m_Component(component),
        m_MsgType(msgType)
    {
        m_Data[0] = v0;
    }

    Handle32<DynamicBodyComponent>      m_Component;
    MessageType                         m_MsgType;
    Float3                              m_Data[2];
};

class PhysicsInterfaceImpl final
{
public:
                                        PhysicsInterfaceImpl();

    BodyUserData*                       CreateUserData();
    void                                DeleteUserData(BodyUserData* userData);

    void                                QueueToAdd(JPH::Body* body, bool startAsSleeping);

    bool                                CastShapeClosest(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, ShapeCastResult& outResult, ShapeCastFilter const& inFilter);
    bool                                CastShape(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter);

    JPH::PhysicsSystem                  m_PhysSystem;

    BodyActivationListener              m_BodyActivationListener;
    ContactListener                     m_ContactListener;

    Vector<Handle32<class DynamicBodyComponent>> m_KinematicBodies;
    Vector<Handle32<class DynamicBodyComponent>> m_DynamicScaling;
    Vector<Handle32<class TriggerComponent>>     m_MovableTriggers;

    Vector<DynamicBodyMessage>         m_DynamicBodyMessageQueue;

    CollisionFilter                     m_CollisionFilter;

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    BPLayerInterfaceImpl                m_BroadPhaseLayerInterface;

    // Class that filters object vs broadphase layers
    ObjectVsBroadPhaseLayerFilterImpl   m_ObjectVsBroadPhaseLayerFilter;
    // Class that filters object vs object layers
    ObjectLayerPairFilterImpl           m_ObjectVsObjectLayerFilter;

    JPH::GroupFilter*                   m_GroupFilter;

    Vector<JPH::BodyID>                m_QueueToAdd[2];

    PoolAllocator<BodyUserData>        m_UserDataAllocator;
};

HK_FORCEINLINE JPH::ObjectLayer MakeObjectLayer(uint32_t group, BroadphaseLayer broadphase)
{
    return (uint32_t(broadphase) << 8) | (group & 0xff);
}

class CharacterControllerImpl : public JPH::CharacterVirtual, public JPH::CharacterContactListener
{
public:
    JPH_OVERRIDE_NEW_DELETE

    JPH::RefConst<JPH::Shape>   m_StandingShape;
    JPH::RefConst<JPH::Shape>   m_CrouchingShape;
    bool                        m_bAllowSliding = false;

    CharacterControllerImpl(const JPH::CharacterVirtualSettings* inSettings, JPH::Vec3Arg inPosition, JPH::QuatArg inRotation, JPH::PhysicsSystem* inSystem) :
        JPH::CharacterVirtual(inSettings, inPosition, inRotation, inSystem)
    {
        SetListener(this);
    }

    // Called whenever the character collides with a body. Returns true if the contact can push the character.
    void OnContactAdded(const JPH::CharacterVirtual*, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
    {
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
            if (ioSettings.mCanPushCharacter && mSystem->GetBodyInterface().GetMotionType(inBodyID2) != JPH::EMotionType::Static)
                m_bAllowSliding = true;
        }
    }

    // Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
    void OnContactSolve(const JPH::CharacterVirtual*, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
    {
        // Don't allow the player to slide down static not-too-steep surfaces when not actively moving and when not on a moving platform
        if (!m_bAllowSliding && inContactVelocity.IsNearZero() && !IsSlopeTooSteep(inContactNormal))
            ioNewCharacterVelocity = JPH::Vec3::sZero();
    }
};

HK_NAMESPACE_END
