#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Math/Quat.h>
#include "SceneGraph.h"

#include "JoltPhysics.h"
#include <Jolt/Physics/PhysicsSystem.h>

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
static constexpr uint8_t DEFAULT   = 0;
static constexpr uint8_t CHARACTER = 1;
static constexpr uint8_t PLATFORM = 2;
static constexpr uint8_t TRIGGER_CHARACTER = 3;
static constexpr uint8_t WATER = 4;

//static constexpr uint8_t CHARACTER = 1;
//static constexpr uint8_t NON_MOVING = 2;
//static constexpr uint8_t MOVING = 3;
//static constexpr uint8_t SENSOR = 4; // Sensors only collide with MOVING objects
}; // namespace CollisionGroup

/// Broadphase layers
namespace BroadphaseLayer
{
// Static non-movable objects
static constexpr uint8_t NON_MOVING(0);

// Dynamic/Kinematic movable object
static constexpr uint8_t MOVING(1);

// Triggers
static constexpr uint8_t SENSOR(2);

// Character proxy is only to collide with triggers
static constexpr uint8_t CHARACTER_PROXY(3);

static constexpr JPH::uint NUM_LAYERS(4);

struct Mask
{
    Mask& AddLayer(uint8_t layer)
    {
        m_Bits |= HK_BIT(layer);
        return *this;
    }

    uint32_t Get() const
    {
        return m_Bits ? m_Bits : ~0u;
    }

private:
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

using PhysBodyID = JPH::BodyID;

struct RayCastFilter
{
    // TODO: Add list of entities to ignore

    BroadphaseLayer::Mask BroadphaseLayerMask;

    bool bIgonreBackFaces : 1;
    bool bSortByDistance : 1;
    bool bCalcSurfcaceNormal : 1;

    RayCastFilter() :
        bIgonreBackFaces(true),
        bSortByDistance(true),
        bCalcSurfcaceNormal(false)
    {}
};

struct RayCastResult
{
    PhysBodyID BodyID;
    /// Hit fraction
    float Fraction;
    /// World space surface normal
    Float3 Normal;
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
    {}
};

struct ShapeCastResult
{
    PhysBodyID BodyID;
    /// Contact point on the surface of shape 1 (in world space or relative to base offset)
    Float3 ContactPointOn1;
    /// Contact point on the surface of shape 2 (in world space or relative to base offset). If the penetration depth is 0, this will be the same as ContactPointOn1.
    Float3 ContactPointOn2;
    /// Direction to move shape 2 out of collision along the shortest path (magnitude is meaningless, in world space). You can use -PenetrationAxis.Normalized() as contact normal.
    Float3 PenetrationAxis;
    /// Penetration depth (move shape 2 by this distance to resolve the collision)
    float PenetrationDepth;
    /// This is the fraction where the shape hit the other shape: CenterOfMassOnHit = Start + value * (End - Start)
    float Fraction;
    /// True if the shape was hit from the back side
    bool IsBackFaceHit;
};

struct ShapeCollideResult
{
    PhysBodyID BodyID;
    /// Contact point on the surface of shape 1 (in world space or relative to base offset)
    Float3 ContactPointOn1;
    /// Contact point on the surface of shape 2 (in world space or relative to base offset). If the penetration depth is 0, this will be the same as ContactPointOn1.
    Float3 ContactPointOn2;
    /// Direction to move shape 2 out of collision along the shortest path (magnitude is meaningless, in world space). You can use -PenetrationAxis.Normalized() as contact normal.
    Float3 PenetrationAxis;
    /// Penetration depth (move shape 2 by this distance to resolve the collision)
    float PenetrationDepth;
};

struct ShapeOverlapFilter
{
    BroadphaseLayer::Mask BroadphaseLayerMask;
};


enum MOTION_BEHAVIOR
{
    /// Non movable
    MB_STATIC,

    /// Responds to forces as a normal physics object
    MB_DYNAMIC,

    /// Movable, does not respond to forces, velocities are calculated from node position and rotation.
    MB_KINEMATIC
};

/// Motion quality, or how well it detects collisions when it has a high velocity
enum MOTION_QUALITY
{
    /// Update the body in discrete steps. Body will tunnel throuh thin objects if its velocity is high enough.
    /// This is the cheapest way of simulating a body.
    MQ_DISCRETE,

    /// Update the body using linear casting. When stepping the body, its collision shape is cast from
    /// start to destination using the starting rotation. The body will not be able to tunnel through thin
    /// objects at high velocity, but tunneling is still possible if the body is long and thin and has high
    /// angular velocity. Time is stolen from the object (which means it will move up to the first collision
    /// and will not bounce off the surface until the next integration step). This will make the body appear
    /// to go slower when it collides with high velocity. In order to not get stuck, the body is always
    /// allowed to move by a fraction of it's inner radius, which may eventually lead it to pass through geometry.
    ///
    /// Note that if you're using a collision listener, you can receive contact added/persisted notifications of contacts
    /// that may in the end not happen. This happens between bodies that are using casting: If bodies A and B collide at t1
    /// and B and C collide at t2 where t2 < t1 and A and C don't collide. In this case you may receive an incorrect contact
    /// point added callback between A and B (which will be removed the next frame).
    MQ_LINEAR_CAST,
};

class CollisionModel;

/// Rigid body scene node
struct RigidBodyDesc
{
    /// Scene node parent
    ECS::EntityHandle   Parent;

    /// Position of the body (not of the center of mass)
    Float3              Position;

    /// Rotation of the body
    Quat                Rotation;

    /// Scale of the body
    Float3              Scale = Float3(1);

    /// For dynamic bodies node flags are forced to SCENE_NODE_ABSOLUTE_*.
    SCENE_NODE_FLAGS    NodeFlags = SCENE_NODE_FLAGS_DEFAULT;

    /// Collision model of the body
    TRef<CollisionModel>Model;

    /// Motion behavior, determines if the object is static, dynamic or kinematic
    MOTION_BEHAVIOR     MotionBehavior = MB_STATIC;

    /// Motion quality, or how well it detects collisions when it has a high velocity
    MOTION_QUALITY      MotionQuality = MQ_DISCRETE;

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t             CollisionGroup = CollisionGroup::DEFAULT; 

    struct DynamicSettings
    {
        /// World space linear velocity of the center of mass (m/s)
        Float3              LinearVelocity;

        /// World space angular velocity (rad/s)
        Float3              AngularVelocity;

        /// Linear damping: dv/dt = -c * v. c must be between 0 and 1 but is usually close to 0.
        float               LinearDamping = 0.05f;

        /// Angular damping: dw/dt = -c * w. c must be between 0 and 1 but is usually close to 0.
        float               AngularDamping = 0.05f;

        /// Maximum linear velocity that this body can reach (m/s)
        float               MaxLinearVelocity = 500;

        /// Maximum angular velocity that this body can reach (rad/s)
        float               MaxAngularVelocity = 0.25f * Math::_PI * 60;

        /// Value to multiply gravity with for this body
        float               GravityFactor = 1.0f;

        // Mass of the body (kg)
        float               Mass = 100.0f;

        /// Tells the system to calculate the inertia based on density of the shapes and to scale it to the provided mass
        bool                bCalculateInertia = true;

        /// When calculating the inertia (not when it is provided) the calculated inertia will be multiplied by this value
        float               InertiaMultiplier = 1.0f;

        // Inertia tensor of the body (kg m^2). Used if bCalculateInertiaTensor is false.
        Float4x4            Inertia = Float4x4(0);
    };

    /// Dynamic settings are applied only to body with motion type MB_DYNAMIC
    DynamicSettings     Dynamic;

    /// Friction of the body (dimensionless number, usually between 0 and 1, 0 = no friction, 1 = friction force equals force that presses the two bodies together)
    float               Friction = 0.2f;

    /// Restitution of body (dimensionless number, usually between 0 and 1, 0 = completely inelastic collision response, 1 = completely elastic collision response)
    float               Restitution = 0.0f;

    /// This flag will enable dynamic rescaling of a rigid body. Disabled for performance.
    bool                bAllowRigidBodyScaling = false;

    /// If this body can go to sleep or not
    bool                bAllowSleeping = true;

    /// Perform node transform interpolation between fixed time steps
    bool                bTransformInterpolation = true;

    /// If this body is a trigger volume. A trigger will not cause any collision responses.
    bool                bIsTrigger = false;

    /// Trigger class is used only if body is a trigger volume.
    ECS::ComponentTypeId TriggerClass = ECS::ComponentTypeId(-1);
};

class TerrainCollision;

struct HeightFieldDesc
{
    /// Scene node parent
    ECS::EntityHandle Parent;

    /// Position of the terrain
    Float3            Position;

    /// Rotation of the terrain
    Quat              Rotation;

    /// Collision model of the terrain
    TRef<TerrainCollision> Model;

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t           CollisionGroup = CollisionGroup::DEFAULT;
};

/// Character controller scene node
struct CharacterControllerDesc
{
    /// Position of the character
    Float3     Position;

    /// Rotation of the character
    Quat       Rotation;

    /// Perform node transform interpolation between fixed time steps
    bool       bTransformInterpolation = true;

    uint8_t    CollisionGroup = CollisionGroup::CHARACTER;

    float      HeightStanding = 1.35f;
    float      RadiusStanding = 0.3f;
    float      HeightCrouching = 0.8f;
    float      RadiusCrouching = 0.3f;
    float      MaxSlopeAngle = Math::Radians(45.0f);
    float      MaxStrength = 100.0f;
    float      CharacterPadding = 0.02f;
    float      PenetrationRecoverySpeed = 1.0f;
    float      PredictiveContactDistance = 0.1f;
};

class PhysicsInterface
{
public:
    PhysicsInterface(ECS::World* world);

    bool CastRayClosest(Float3 const& inRayStart, Float3 const& inRayDir, RayCastResult& outResult, RayCastFilter const& inFilter = {});
    bool CastRay(Float3 const& inRayStart, Float3 const& inRayDir, TVector<RayCastResult>& outResult, RayCastFilter const& inFilter = {});

    bool CastBoxClosest(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool CastBox(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool CastBoxMinMaxClosest(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool CastBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool CastSphereClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool CastSphere(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool CastCapsuleClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool CastCapsule(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool CastCylinderClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool CastCylinder(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    void OverlapBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void OverlapBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void OverlapSphere(Float3 const& inPosition, float inRadius, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void OverlapPoint(Float3 const& inPosition, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});

    bool CheckBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool CheckBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, ShapeCastFilter const& inFilter = {});
    bool CheckSphere(Float3 const& inPosition, float inRadius, ShapeCastFilter const& inFilter = {});
    bool CheckCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool CheckCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool CheckPoint(Float3 const& inPosition, BroadphaseLayer::Mask inBroadphaseLayrs = {});

    void CollideBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void CollideBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void CollideSphere(Float3 const& inPosition, float inRadius, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void CollideCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void CollideCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void CollidePoint(Float3 const& inPosition, TVector<PhysBodyID>& outResult, BroadphaseLayer::Mask inBroadphaseLayrs = {});

    auto GetEntity(PhysBodyID const& inBodyID) -> ECS::EntityHandle;
    auto GetPhysBodyID(ECS::EntityHandle inEntityHandle) -> PhysBodyID;

    ECS::EntityHandle CreateBody(ECS::CommandBuffer& inCB, RigidBodyDesc const& inDesc);
    ECS::EntityHandle CreateHeightField(ECS::CommandBuffer& inCB, HeightFieldDesc const& inDesc);
    ECS::EntityHandle CreateCharacterController(ECS::CommandBuffer& inCB, CharacterControllerDesc const& inDesc);

    void ActivateBody(PhysBodyID const& inBodyID);
    void ActivateBodies(TArrayView<PhysBodyID> inBodyIDs);
    void DeactivateBody(PhysBodyID const& inBodyID);
    void DeactivateBodies(TArrayView<PhysBodyID> inBodyIDs);
    bool IsActive(PhysBodyID const& inBodyID) const;

    //void SetPositionAndRotation(PhysBodyID const& inBodyID, Float3 const& inPosition, Quat const& inRotation, JPH::EActivation inActivationMode);

    // Will only update the position/rotation and activate the body when the difference is larger than a very small number.
    // This avoids updating the broadphase/waking up a body when the resulting position/orientation doesn't really change.
    //void SetPositionAndRotationWhenChanged(PhysBodyID const& inBodyID, Float3 const& inPosition, Quat const& inRotation, JPH::EActivation inActivationMode);

    //void GetPositionAndRotation(PhysBodyID const& inBodyID, Float3& outPosition, Quat& outRotation) const;
    //void SetPosition(PhysBodyID const& inBodyID, Float3 const& inPosition, JPH::EActivation inActivationMode);
    //auto GetPosition(PhysBodyID const& inBodyID) const -> Float3;
    auto GetCenterOfMassPosition(PhysBodyID const& inBodyID) const -> Float3;
    //void SetRotation(PhysBodyID const& inBodyID, Quat const& inRotation, JPH::EActivation inActivationMode);
    //auto GetRotation(PhysBodyID const& inBodyID) const -> Quat;
    //auto GetWorldTransform(PhysBodyID const& inBodyID) const -> Float4x4;
    auto GetCenterOfMassTransform(PhysBodyID const& inBodyID) const -> Float4x4;

    // Linear or angular velocity (functions will activate body if needed).
    // Note that the linear velocity is the velocity of the center of mass, which may not coincide with the position of your object, to correct for this: \f$VelocityCOM = Velocity - AngularVelocity \times ShapeCOM\f$
    void SetLinearAndAngularVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity, Float3 const& inAngularVelocity);
    void GetLinearAndAngularVelocity(PhysBodyID const& inBodyID, Float3& outLinearVelocity, Float3& outAngularVelocity) const;
    void SetLinearVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity);
    auto GetLinearVelocity(PhysBodyID const& inBodyID) const -> Float3;
    // Add velocity to current velocity
    void AddLinearVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity);
    // Add linear and angular to current velocities
    void AddLinearAndAngularVelocity(PhysBodyID const& inBodyID, Float3 const& inLinearVelocity, Float3 const& inAngularVelocity);
    void SetAngularVelocity(PhysBodyID const& inBodyID, Float3 const& inAngularVelocity);
    auto GetAngularVelocity(PhysBodyID const& inBodyID) const -> Float3;
    // Velocity of point inPoint (in world space, e.g. on the surface of the body) of the body
    auto GetPointVelocity(PhysBodyID const& inBodyID, Float3 const& inPoint) const -> Float3;

    // Set the complete motion state of a body.
    // Note that the linear velocity is the velocity of the center of mass, which may not coincide with the position of your object, to correct for this: \f$VelocityCOM = Velocity - AngularVelocity \times ShapeCOM\f$
    //void SetPositionRotationAndVelocity(PhysBodyID const& inBodyID, Float3 const& inPosition, Quat const& inRotation, Float3 const& inLinearVelocity, Float3 const& inAngularVelocity);

    void AddForce(PhysBodyID const& inBodyID, Float3 const& inForce);
    // Applied at inPoint
    void AddForce(PhysBodyID const& inBodyID, Float3 const& inForce, Float3 const& inPoint);
    void AddTorque(PhysBodyID const& inBodyID, Float3 const& inTorque);
    // A combination of AddForce and AddTorque
    void AddForceAndTorque(PhysBodyID const& inBodyID, Float3 const& inForce, Float3 const& inTorque);

    // Applied at center of mass
    void AddImpulse(PhysBodyID const& inBodyID, Float3 const& inImpulse);
    // Applied at inPoint
    void AddImpulse(PhysBodyID const& inBodyID, Float3 const& inImpulse, Float3 const& inPoint);
    void AddAngularImpulse(PhysBodyID const& inBodyID, Float3 const& inAngularImpulse);

    auto GetMotionBehavior(PhysBodyID const& inBodyID) const -> MOTION_BEHAVIOR;

    // Body motion quality
    void SetMotionQuality(PhysBodyID const& inBodyID, MOTION_QUALITY inMotionQuality);
    auto GetMotionQuality(PhysBodyID const& inBodyID) const -> MOTION_QUALITY;

    // Get inverse inertia tensor in world space
    auto GetInverseInertia(PhysBodyID const& inBodyID) const -> Float4x4;

    void SetRestitution(PhysBodyID const& inBodyID, float inRestitution);
    auto GetRestitution(PhysBodyID const& inBodyID) const -> float;

    void SetFriction(PhysBodyID const& inBodyID, float inFriction);
    auto GetFriction(PhysBodyID const& inBodyID) const -> float;

    void SetGravityFactor(PhysBodyID const& inBodyID, float inGravityFactor);
    auto GetGravityFactor(PhysBodyID const& inBodyID) const -> float;

    void SetGravity(Float3 const inGravity);
    Float3 GetGravity() const;

    auto GetImpl() -> JPH::PhysicsSystem&
    {
        return m_PhysicsSystem;
    }

    auto GetCollisionFilter() -> CollisionFilter&
    {
        return m_CollisionFilter;
    }

private:
    bool CastShapeClosest(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, ShapeCastResult& outResult, ShapeCastFilter const& inFilter);
    bool CastShape(JPH::RShapeCast const& inShapeCast, JPH::RVec3Arg inBaseOffset, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter);

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

public: // TODO: make it private
    THashMap<ECS::EntityHandle, PhysBodyID> m_PendingBodies;
    SpinLock m_PendingBodiesMutex;
};

HK_NAMESPACE_END
