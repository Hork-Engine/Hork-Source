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

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

HK_NAMESPACE_BEGIN

HK_INLINE JPH::Vec3 ConvertVector(Float3 const& v)
{
    return JPH::Vec3(v.X, v.Y, v.Z);
}

HK_INLINE JPH::Vec4 ConvertVector(Float4 const& v)
{
    return JPH::Vec4(v.X, v.Y, v.Z, v.W);
}

HK_INLINE JPH::Quat ConvertQuaternion(Quat const& q)
{
    return JPH::Quat(q.X, q.Y, q.Z, q.W);
}

HK_INLINE Float3 ConvertVector(JPH::Vec3 const& v)
{
    return Float3(v.GetX(), v.GetY(), v.GetZ());
}

HK_INLINE Float4 ConvertVector(JPH::Vec4 const& v)
{
    return Float4(v.GetX(), v.GetY(), v.GetZ(), v.GetW());
}

HK_INLINE Quat ConvertQuaternion(JPH::Quat const& q)
{
    return Quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

HK_INLINE Float4x4 ConvertMatrix(JPH::Mat44 const& m)
{
    return Float4x4(ConvertVector(m.GetColumn4(0)),
        ConvertVector(m.GetColumn4(1)),
        ConvertVector(m.GetColumn4(2)),
        ConvertVector(m.GetColumn4(3)));
}

HK_INLINE JPH::Mat44 ConvertMatrix(Float4x4 const& m)
{
    return JPH::Mat44(ConvertVector(m.Col0),
        ConvertVector(m.Col1),
        ConvertVector(m.Col2),
        ConvertVector(m.Col3));
}

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
        TypeID = ComponentRTTR::TypeID<ComponentType>;
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
        if (TypeID == ComponentRTTR::TypeID<ComponentType>)
            return world->GetComponent<ComponentType>(Handle32<ComponentType>(Component));
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

class ObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
    ObjectLayerFilter(CollisionFilter const& collisionFilter, uint32_t collisionLayer) :
        m_CollisionFilter(collisionFilter),
        m_CollisionLayer(collisionLayer)
    {}

    bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return m_CollisionFilter.ShouldCollide(m_CollisionLayer, static_cast<uint32_t>(inLayer) & 0xff);
    }

private:
    CollisionFilter const& m_CollisionFilter;
    uint32_t m_CollisionLayer;
};

class TriggerComponent;

struct TriggerEvent
{
    enum EventType
    {
        OnBeginOverlap,
        OnEndOverlap
    };

    EventType                   Type;
    Handle32<TriggerComponent>  Trigger;
    ComponentExtendedHandle     Target;
};

struct ContactEvent
{
    enum EventType
    {
        OnBeginContact,
        OnUpdateContact,
        OnEndContact
    };

    EventType                   Type;
    ComponentExtendedHandle     Self;
    ComponentExtendedHandle     Other;
    Float3                      Normal;
    float                       Depth;
    uint32_t                    FirstPoint;
    uint32_t                    NumPoints;
};

class BodyActivationListener final : public JPH::BodyActivationListener
{
public:
    // FIXME: Use set?
    Mutex                   m_Mutex;
    Vector<uint32_t>        m_ActiveBodies;
    Vector<uint32_t>        m_JustDeactivated;

    virtual void			OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override;

    virtual void			OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override;
};

class ContactListener final : public JPH::ContactListener
{
public:
    using ContactID = uint64_t;

    struct TriggerContact
    {
        Handle32<TriggerComponent>  Trigger;
        ComponentExtendedHandle     Target;
        uint32_t                    Count;
    };
    using TriggerContacts = HashMap<ContactID, TriggerContact>;

    struct BodyContact
    {
        ComponentExtendedHandle     Body1;
        ComponentExtendedHandle     Body2;
        bool                        Body1DispatchEvent;
        bool                        Body2DispatchEvent;
    };
    using BodyContacts = HashMap<ContactID, BodyContact>;

    World*                  m_World;
    TriggerContacts         m_Triggers;
    BodyContacts            m_BodyContacts;
    Vector<TriggerEvent>*   m_pTriggerEvents;
    Vector<ContactEvent>*   m_pContactEvents;
    Vector<ContactPoint>*   m_pContactPoints;
    JPH::Mutex              m_Mutex;

    virtual JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override;

    virtual void			OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;

    virtual void			OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;

    virtual void			OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;

    void                    AddContactEvents(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings, bool isPersisted);
};

class CharacterContactListener final : public JPH::CharacterContactListener
{
public:
    using ContactID = uint64_t;

    struct TriggerContact
    {
        Handle32<TriggerComponent>      Trigger;
        uint32_t                        FrameIndex;
    };
    using TriggerContacts = HashMap<ContactID, TriggerContact>;

    struct BodyContact
    {
        ComponentExtendedHandle         Body;
        uint32_t                        FrameIndex;
    };
    using BodyContacts = HashMap<ContactID, BodyContact>;

    World*                              m_World;
    JPH::PhysicsSystem*                 m_PhysSystem;
    TriggerContacts                     m_Triggers;
    BodyContacts                        m_BodyContacts;
    Vector<TriggerEvent>*               m_pTriggerEvents;
    Vector<ContactEvent>*               m_pContactEvents;
    Vector<ContactPoint>*               m_pContactPoints;
    Vector<ContactID>                   m_UpdateOverlap;
    Vector<ContactID>                   m_UpdateContact;

    // Called whenever the character collides with a body. Returns true if the contact can push the character.
    void                                OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override;

    // Same as OnContactAdded but when colliding with a CharacterVirtual
    void                                OnCharacterContactAdded(const JPH::CharacterVirtual *inCharacter, const JPH::CharacterVirtual *inOtherCharacter, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings &ioSettings) override;

    // Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
    void                                OnContactSolve(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity) override;

    // Same as OnContactSolve but when colliding with a CharacterVirtual
    void                                OnCharacterContactSolve(const JPH::CharacterVirtual *inCharacter, const JPH::CharacterVirtual *inOtherCharacter, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial *inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3 &ioNewCharacterVelocity) override;
};

class CharacterVsCharacterCollision : public JPH::CharacterVsCharacterCollision
{
public:
    // Add a character to the list of characters to check collision against.
    void                                Add(JPH::CharacterVirtual *inCharacter);

    // Remove a character from the list of characters to check collision against.
    void                                Remove(const JPH::CharacterVirtual *inCharacter);

    virtual void                        CollideCharacter(const JPH::CharacterVirtual *inCharacter, JPH::RMat44Arg inCenterOfMassTransform, const JPH::CollideShapeSettings &inCollideShapeSettings, JPH::RVec3Arg inBaseOffset, JPH::CollideShapeCollector &ioCollector) const override;
    virtual void                        CastCharacter(const JPH::CharacterVirtual *inCharacter, JPH::RMat44Arg inCenterOfMassTransform, JPH::Vec3Arg inDirection, const JPH::ShapeCastSettings &inShapeCastSettings, JPH::RVec3Arg inBaseOffset, JPH::CastShapeCollector &ioCollector) const override;

    CollisionFilter*                    m_pCollisionFilter;
    JPH::Array<JPH::CharacterVirtual*>  m_Characters;
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

class MeshCollisionDataInternal
{
public:
    JPH::Ref<JPH::Shape>    m_Shape;
};

struct CreateCollisionSettings
{
    GameObject* Object = nullptr;
    Float3      CenterOfMassOverride{std::numeric_limits<float>::quiet_NaN()};
    bool        ConvexOnly = false;
};

class PhysicsInterfaceImpl final
{
public:
                                        PhysicsInterfaceImpl();

    BodyUserData*                       CreateUserData();
    void                                DeleteUserData(BodyUserData* userData);

    void                                QueueToAdd(JPH::Body* body, bool startAsSleeping);

    bool                                CreateCollision(CreateCollisionSettings const& settings, JPH::Shape*& outShape, ScalingMode& outScalingMode);
    HK_NODISCARD JPH::Shape*            CreateScaledShape(ScalingMode scalingMode, JPH::Shape* sourceShape, Float3 const& scale);

    static void                         sGatherShapeGeometry(JPH::Shape const* shape, Vector<Float3>& vertices, Vector<uint32_t>& indices);

    bool                                CastShapeClosest(JPH::RShapeCast const& inShapeCast, ShapeCastResult& outResult, ShapeCastFilter const& inFilter);
    bool                                CastShape(JPH::RShapeCast const& inShapeCast, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter);

    JPH::PhysicsSystem                  m_PhysSystem;

    BodyActivationListener              m_BodyActivationListener;
    ContactListener                     m_ContactListener;
    CharacterContactListener            m_CharacterContactListener;
    CharacterVsCharacterCollision       m_CharacterVsCharacterCollision;

    Vector<TriggerEvent>                m_TriggerEvents;
    Vector<ContactEvent>                m_ContactEvents;
    Vector<ContactPoint>                m_ContactPoints;

    Vector<Handle32<class DynamicBodyComponent>> m_KinematicBodies;
    Vector<Handle32<class DynamicBodyComponent>> m_DynamicScaling;
    Vector<Handle32<class TriggerComponent>>     m_MovableTriggers;

    Vector<DynamicBodyMessage>          m_DynamicBodyMessageQueue;

    CollisionFilter                     m_CollisionFilter;

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    BPLayerInterfaceImpl                m_BroadPhaseLayerInterface;

    // Class that filters object vs broadphase layers
    ObjectVsBroadPhaseLayerFilterImpl   m_ObjectVsBroadPhaseLayerFilter;
    // Class that filters object vs object layers
    ObjectLayerPairFilterImpl           m_ObjectVsObjectLayerFilter;

    JPH::GroupFilter*                   m_GroupFilter;

    Vector<JPH::BodyID>                 m_QueueToAdd[2];

    PoolAllocator<BodyUserData>         m_UserDataAllocator;

    struct ShapeTransform
    {
        JPH::Vec3 Position;
        JPH::Quat Rotation;

        ShapeTransform() = default;
        ShapeTransform(JPH::Vec3Arg position, JPH::QuatArg rotation) :
            Position(position), Rotation(rotation)
        {}
    };
    Vector<JPH::Shape*>                 m_TempShapes;
    Vector<ShapeTransform>              m_TempShapeTransform;
    JPH::StaticCompoundShapeSettings    m_TempCompoundShapeSettings;
};

class CharacterControllerComponent;

class CharacterControllerImpl : public JPH::CharacterVirtual
{
public:
    JPH_OVERRIDE_NEW_DELETE

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

    class BodyFilter final : public JPH::BodyFilter
    {
    public:
        //JPH::BodyID m_IgoreBodyID;

        bool ShouldCollideLocked(const JPH::Body& body) const override
        {
            return true;//body.GetID() != m_IgoreBodyID;
        }
    };

    using ShapeFilter = JPH::ShapeFilter;

    Handle32<CharacterControllerComponent> m_Component;
    JPH::RefConst<JPH::Shape>   m_StandingShape;
    JPH::RefConst<JPH::Shape>   m_CrouchingShape;
    uint8_t                     m_CollisionLayer = 0;

    CharacterControllerImpl(const JPH::CharacterVirtualSettings* inSettings, JPH::Vec3Arg inPosition, JPH::QuatArg inRotation, JPH::PhysicsSystem* inSystem);
};

HK_FORCEINLINE JPH::ObjectLayer MakeObjectLayer(uint32_t group, BroadphaseLayer broadphase)
{
    return (uint32_t(broadphase) << 8) | (group & 0xff);
}

HK_INLINE void TransformVertices(Float3* inoutVertices, uint32_t inVertexCount, Float3x4 const& inTransform)
{
    for (uint32_t i = 0; i < inVertexCount; i++)
    {
        inoutVertices[i] = inTransform * inoutVertices[i];
    }
}

HK_NAMESPACE_END
