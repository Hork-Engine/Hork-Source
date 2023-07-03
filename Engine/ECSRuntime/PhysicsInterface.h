#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Geometry/Quat.h>

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

HK_NAMESPACE_BEGIN

class CollisionFilter
{
public:
    CollisionFilter();

    //void SetGroupName(uint32_t group, StringView name);

    //uint32_t FindGroup(StringView name) const;

    void Clear();

    void SetShouldCollide(uint32_t group1, uint32_t group2, bool shouldCollide);

    bool ShouldCollide(uint32_t group1, uint32_t group2) const;

private:
    TArray<uint32_t, 32> m_CollisionMask;
    //TArray<char[32], 32> m_GroupName;
};

HK_NAMESPACE_END


/// Layer that objects can be in, determines which other objects it can collide with
namespace CollisionGroup
{
static constexpr JPH::uint8 DEFAULT   = 0;
static constexpr JPH::uint8 CHARACTER = 1;
static constexpr JPH::uint8 PLATFORM  = 2;
static constexpr JPH::uint8 TRIGGER_CHARACTER = 3;
static constexpr JPH::uint8 WATER = 4;

//static constexpr JPH::uint8 CHARACTER = 1;
//static constexpr JPH::uint8 NON_MOVING = 2;
//static constexpr JPH::uint8 MOVING = 3;
//static constexpr JPH::uint8 SENSOR = 4; // Sensors only collide with MOVING objects
}; // namespace CollisionGroup

/// Broadphase layers
namespace BroadphaseLayer
{
// Static non-movable objects
static constexpr JPH::uint8 NON_MOVING(0);

// Dynamic/Kinematic movable object
static constexpr JPH::uint8 MOVING(1);

// Triggers
static constexpr JPH::uint8 SENSOR(2);

// Character proxy is only to collide with triggers
static constexpr JPH::uint8 CHARACTER_PROXY(3);

static constexpr JPH::uint NUM_LAYERS(4);

}; // namespace BroadphaseLayer


/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    Hk::CollisionFilter const& m_CollisionFilter;

    ObjectLayerPairFilterImpl(Hk::CollisionFilter const& collisionFilter) :
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
    ObjectLayerFilter(Hk::CollisionFilter const& collisionFilter, uint32_t collisionGroup) :
        m_CollisionFilter(collisionFilter),
        m_CollisionGroup(collisionGroup)
    {}

    bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return m_CollisionFilter.ShouldCollide(m_CollisionGroup, static_cast<uint32_t>(inLayer) & 0xff);
    }

private:
    Hk::CollisionFilter const& m_CollisionFilter;
    uint32_t m_CollisionGroup;
};

class SpecifiedObjectLayerFilter : public JPH::ObjectLayerFilter
{
public:
    explicit SpecifiedObjectLayerFilter(uint32_t collisionGroup) :
        m_CollisionGroup(collisionGroup)
    {}

    bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        uint32_t group = static_cast<uint32_t>(inLayer) & 0xff;

        return m_CollisionGroup == group;
    }

private:
    uint32_t m_CollisionGroup;
};

class BodyFilter final : public JPH::BodyFilter
{
public:
    uint32_t m_ObjectFilterIDToIgnore = 0xFFFFFFFF - 1;

    BodyFilter(uint32_t uiBodyFilterIdToIgnore = 0xFFFFFFFF - 1) :
        m_ObjectFilterIDToIgnore(uiBodyFilterIdToIgnore)
    {
    }

    void ClearFilter()
    {
        m_ObjectFilterIDToIgnore = 0xFFFFFFFF - 1;
    }

    virtual bool ShouldCollideLocked(const JPH::Body& body) const override
    {
        return body.GetCollisionGroup().GetGroupID() != m_ObjectFilterIDToIgnore;
    }
};

/// BroadPhaseLayerInterface implementation
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return BroadphaseLayer::NUM_LAYERS;
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
            case BroadphaseLayer::NON_MOVING: return "NON_MOVING";
            case BroadphaseLayer::MOVING: return "MOVING";
            case BroadphaseLayer::SENSOR: return "SENSOR";
            case BroadphaseLayer::CHARACTER_PROXY: return "CHARACTER_PROXY";
            default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    uint32_t BroadphaseCollisionMask(JPH::BroadPhaseLayer broadphaseLayer) const
    {
        static constexpr Hk::TArray<uint32_t, BroadphaseLayer::NUM_LAYERS> mask =
            {
                // NON_MOVING:
                HK_BIT(BroadphaseLayer::MOVING) | HK_BIT(BroadphaseLayer::CHARACTER_PROXY),

                // MOVING:
                HK_BIT(BroadphaseLayer::NON_MOVING) | HK_BIT(BroadphaseLayer::MOVING) | HK_BIT(BroadphaseLayer::SENSOR) | HK_BIT(BroadphaseLayer::CHARACTER_PROXY),

                // SENSOR
                HK_BIT(BroadphaseLayer::MOVING) | HK_BIT(BroadphaseLayer::CHARACTER_PROXY),

                // CHARACTER_PROXY
                HK_BIT(BroadphaseLayer::CHARACTER_PROXY) | HK_BIT(BroadphaseLayer::SENSOR) | HK_BIT(BroadphaseLayer::NON_MOVING) | HK_BIT(BroadphaseLayer::MOVING),
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

HK_NAMESPACE_BEGIN

HK_INLINE JPH::ObjectLayer MakeObjectLayer(uint32_t group, uint32_t broadphase)
{
    return ((broadphase & 0xff) << 8) | (group & 0xff);
}

HK_INLINE JPH::Vec3 ConvertVector(Float3 const& v)
{
    return JPH::Vec3(v.X, v.Y, v.Z);
}

HK_INLINE JPH::Quat ConvertQuaternion(Quat const& q)
{
    return JPH::Quat(q.X, q.Y, q.Z, q.W);
}

HK_INLINE Float3 ConvertVector(JPH::Vec3 const& v)
{
    return Float3(v.GetX(), v.GetY(), v.GetZ());
}

HK_INLINE Quat ConvertQuaternion(JPH::Quat const& q)
{
    return Quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

struct SphereCastResult
{
    float HitFraction;
};

class PhysicsInterface
{
public:
    PhysicsInterface(ECS::World* world);

    bool CastSphere(Float3 const& start, Float3 const& dir, float sphereRadius, SphereCastResult& result);

    void SetLinearVelocity(ECS::EntityHandle handle, Float3 const& velocity);

    JPH::PhysicsSystem& GetImpl()
    {
        return m_PhysicsSystem;
    }

    CollisionFilter& GetCollisionFilter()
    {
        return m_CollisionFilter;
    }

private:
    ECS::World* m_World;
    JPH::PhysicsSystem m_PhysicsSystem;

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    BPLayerInterfaceImpl m_BroadPhaseLayerInterface;
    // Class that filters object vs broadphase layers
    ObjectVsBroadPhaseLayerFilterImpl m_ObjectVsBroadPhaseLayerFilter;
    // Class that filters object vs object layers
    ObjectLayerPairFilterImpl m_ObjectVsObjectLayerFilter;

    CollisionFilter m_CollisionFilter;
};

HK_NAMESPACE_END
