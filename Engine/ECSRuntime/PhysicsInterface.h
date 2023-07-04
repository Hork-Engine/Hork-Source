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

struct Mask
{
    Mask& Clear()
    {
        m_Bits = 0;
        return *this;
    }

    Mask& All()
    {
        for (JPH::uint i = 0; i < NUM_LAYERS; ++i)
            m_Bits |= HK_BIT(i);
        return *this;
    }

    Mask& AddLayer(JPH::uint8 layer)
    {
        m_Bits |= HK_BIT(layer);
        return *this;
    }

    uint32_t Get() const
    {
        return m_Bits;
    }

    uint32_t m_Bits{};
};

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

struct ShapeCastResult
{
    // TODO: Add entityHandle, subshapeId

    float HitFraction;
};

struct ShapeCastFilter
{
    // TODO: Add list of entities to ignore

    BroadphaseLayer::Mask BroadphaseLayerMask;

    bool bIgonreBackFaces : 1;
    bool bSortByDistance : 1;

    ShapeCastFilter() :
        bIgonreBackFaces(true),
        bSortByDistance(true)
    {
        BroadphaseLayerMask.All();        
    }
};

class PhysicsInterface
{
public:
    PhysicsInterface(ECS::World* world);

    bool CastRay(Float3 const& start, Float3 const& dir, ShapeCastResult& result, ShapeCastFilter const* filter = nullptr);
    bool CastRayAll(Float3 const& start, Float3 const& dir, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter = nullptr);

    bool CastBox(Float3 const& start, Float3 const& dir, Float3 const& halfExtent, Quat const& boxRotation, ShapeCastResult& result, ShapeCastFilter const* filter = nullptr);
    bool CastBoxAll(Float3 const& start, Float3 const& dir, Float3 const& halfExtent, Quat const& boxRotation, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter = nullptr);

    bool CastBoxMinMax(Float3 const& mins, Float3 const& maxs, Float3 const& dir, ShapeCastResult& result, ShapeCastFilter const* filter = nullptr);
    bool CastBoxMinMaxAll(Float3 const& mins, Float3 const& maxs, Float3 const& dir, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter = nullptr);

    bool CastSphere(Float3 const& start, Float3 const& dir, float sphereRadius, ShapeCastResult& result, ShapeCastFilter const* filter = nullptr);
    bool CastSphereAll(Float3 const& start, Float3 const& dir, float sphereRadius, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter = nullptr);

    bool CastCapsule(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, ShapeCastResult& result, ShapeCastFilter const* filter = nullptr);
    bool CastCapsuleAll(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter = nullptr);

    bool CastCylinder(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, ShapeCastResult& result, ShapeCastFilter const* filter = nullptr);
    bool CastCylinderAll(Float3 const& start, Float3 const& dir, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter = nullptr);

    bool CollidePoint(Float3 const& point, BroadphaseLayer::Mask broadphaseLayrs);

    // TODO: replace TVector<JPH::BodyID> with TVector<ECS::EntityHandle>
    bool CollidePointAll(Float3 const& point, TVector<JPH::BodyID>& bodies, BroadphaseLayer::Mask broadphaseLayrs);

    bool CheckBox(Float3 const& position, Float3 const& halfExtent, Quat const& boxRotation, ShapeCastFilter const* filter = nullptr);
    bool CheckBoxMinMax(Float3 const& mins, Float3 const& maxs, ShapeCastFilter const* filter = nullptr);
    bool CheckSphere(Float3 const& position, float sphereRadius, ShapeCastFilter const* filter = nullptr);
    bool CheckCapsule(Float3 const& position, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, ShapeCastFilter const* filter = nullptr);
    bool CheckCylinder(Float3 const& position, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, ShapeCastFilter const* filter = nullptr);

    // TODO: replace TVector<JPH::BodyID> with TVector<ECS::EntityHandle>
    void OverlapBox(Float3 const& position, Float3 const& halfExtent, Quat const& boxRotation, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapBoxMinMax(Float3 const& mins, Float3 const& maxs, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapSphere(Float3 const& position, float sphereRadius, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapCapsule(Float3 const& position, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapCylinder(Float3 const& position, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, TVector<JPH::BodyID>& result, ShapeCastFilter const* filter = nullptr);

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
    bool CastShape(JPH::RShapeCast const& shapeCast, JPH::RVec3Arg baseOffset, ShapeCastResult& result, ShapeCastFilter const* filter);
    bool CastShapeAll(JPH::RShapeCast const& shapeCast, JPH::RVec3Arg baseOffset, TVector<ShapeCastResult>& result, ShapeCastFilter const* filter);

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
