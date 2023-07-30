#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Geometry/Quat.h>
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

using PhysBodyID = JPH::BodyID;

enum MOTION_BEHAVIOR
{
    /// Non movable
    MB_STATIC,

    /// Responds to forces as a normal physics object
    MB_SIMULATED,

    /// Movable using velocities only, does not respond to forces
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

/// Enum used in RigidBodyDesc to indicate how mass and inertia should be calculated
enum OVERRIDE_MASS_PROPERTIES
{
    /// Tells the system to calculate the mass and inertia based on density
    OVERRIDE_MASS_PROPERTIES_CALCULATE_MASS_AND_INERTIA,

    /// Tells the system to take the mass from MassPropertiesOverride and to calculate the inertia based on density of the shapes and to scale it to the provided mass
    OVERRIDE_MASS_PROPERTIES_CALCULATE_INERTIA,

    /// Tells the system to take the mass and inertia from MassPropertiesOverride
    OVERRIDE_MASS_PROPERTIES_MASS_AND_INERTIA_PROVIDED
};

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

    SCENE_NODE_FLAGS    NodeFlags = SCENE_NODE_FLAGS_DEFAULT;

    /// Collision model of the body
    TRef<CollisionModel>Model;

    /// Motion behavior, determines if the object is static, dynamic or kinematic
    MOTION_BEHAVIOR     MotionBehavior = MB_STATIC;

    /// Motion quality, or how well it detects collisions when it has a high velocity
    MOTION_QUALITY      MotionQuality = MQ_DISCRETE;

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t             CollisionGroup = CollisionGroup::DEFAULT; 

    /// World space linear velocity of the center of mass (m/s)
    Float3              LinearVelocity;

    /// World space angular velocity (rad/s)
    Float3              AngularVelocity;

    /// Friction of the body (dimensionless number, usually between 0 and 1, 0 = no friction, 1 = friction force equals force that presses the two bodies together)
    float               Friction = 0.2f;

    /// Restitution of body (dimensionless number, usually between 0 and 1, 0 = completely inelastic collision response, 1 = completely elastic collision response)
    float               Restitution = 0.0f;

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

    /// Determines how MassPropertiesOverride will be used
    OVERRIDE_MASS_PROPERTIES OverrideMassProperties = OVERRIDE_MASS_PROPERTIES_CALCULATE_MASS_AND_INERTIA;

    /// When calculating the inertia (not when it is provided) the calculated inertia will be multiplied by this value
    float               InertiaMultiplier = 1.0f;

    /// Mass properties of the body (by default calculated by the shape)
    struct MassProperties
    {
        // Mass of the shape (kg)
        float Mass = 0.0f;
        // Inertia tensor of the shape (kg m^2)
        Float4x4 Inertia = Float4x4(0);
    };

    /// Contains replacement mass settings which override the automatically calculated values
    MassProperties      MassPropertiesOverride;

    // This flag will enable dynamic rescaling of a rigid body. Disabled for performance.
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

/// Character controller scene node
struct CharacterControllerDesc
{
    /// Position of the character
    Hk::Float3 Position;

    /// Rotation of the character
    Hk::Quat   Rotation;

    /// Perform node transform interpolation between fixed time steps
    bool       bTransformInterpolation = true;
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

    bool CollidePointAll(Float3 const& point, TVector<PhysBodyID>& bodies, BroadphaseLayer::Mask broadphaseLayrs);

    bool CheckBox(Float3 const& position, Float3 const& halfExtent, Quat const& boxRotation, ShapeCastFilter const* filter = nullptr);
    bool CheckBoxMinMax(Float3 const& mins, Float3 const& maxs, ShapeCastFilter const* filter = nullptr);
    bool CheckSphere(Float3 const& position, float sphereRadius, ShapeCastFilter const* filter = nullptr);
    bool CheckCapsule(Float3 const& position, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, ShapeCastFilter const* filter = nullptr);
    bool CheckCylinder(Float3 const& position, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, ShapeCastFilter const* filter = nullptr);

    void OverlapBox(Float3 const& position, Float3 const& halfExtent, Quat const& boxRotation, TVector<PhysBodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapBoxMinMax(Float3 const& mins, Float3 const& maxs, TVector<PhysBodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapSphere(Float3 const& position, float sphereRadius, TVector<PhysBodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapCapsule(Float3 const& position, float halfHeightOfCylinder, float capsuleRadius, Quat const& capsuleRotation, TVector<PhysBodyID>& result, ShapeCastFilter const* filter = nullptr);
    void OverlapCylinder(Float3 const& position, float halfHeightOfCylinder, float cylinderRadius, Quat const& cylinderRotation, TVector<PhysBodyID>& result, ShapeCastFilter const* filter = nullptr);

    auto GetEntity(PhysBodyID const& bodyID) -> ECS::EntityHandle;
    auto GetPhysBodyID(ECS::EntityHandle entityHandle) -> PhysBodyID;

    ECS::EntityHandle CreateBody(ECS::CommandBuffer& commandBuffer, RigidBodyDesc const& desc);
    ECS::EntityHandle CreateCharacterController(ECS::CommandBuffer& commandBuffer, CharacterControllerDesc const& desc);

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

    // Set velocity of body such that it will be positioned at inTargetPosition/Rotation in inDeltaTime seconds (will activate body if needed)
    //void MoveKinematic(PhysBodyID const& inBodyID, Float3 const& inTargetPosition, Quat const& inTargetRotation, float inDeltaTime);

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

    void SetLinearVelocity(ECS::EntityHandle entityHandle, Float3 const& velocity);

    auto GetImpl() -> JPH::PhysicsSystem&
    {
        return m_PhysicsSystem;
    }

    auto GetCollisionFilter() -> CollisionFilter&
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

public: // TODO: make it private
    THashMap<ECS::EntityHandle, PhysBodyID> m_PendingBodies;
    SpinLock m_PendingBodiesMutex;
};

HK_NAMESPACE_END
