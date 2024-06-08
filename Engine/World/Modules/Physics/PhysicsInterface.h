#pragma once

#include <Engine/World/WorldInterface.h>
#include <Engine/Core/Ref.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

/// Layer that objects can be in, determines which other objects it can collide with
namespace CollisionLayer
{
    static constexpr uint8_t Default             = 0;
    static constexpr uint8_t Character           = 1;
    static constexpr uint8_t Platform            = 2;
    static constexpr uint8_t TriggerCharacter    = 3;
    static constexpr uint8_t Water               = 4;
};

/// Broadphase layers
enum class BroadphaseLayer : uint8_t
{
    // Static non-movable objects
    Static,

    // Dynamic/Kinematic movable object
    Dynamic,

    // Triggers
    Trigger,

    // Character to collide with triggers
    Character,

    Max
};

struct BroadphaseLayerMask
{
    BroadphaseLayerMask&    AddLayer(BroadphaseLayer layer) { m_Bits |= HK_BIT(uint32_t(layer)); return *this; }
    uint32_t                Get() const { return m_Bits ? m_Bits : ~0u; }

private:
    uint32_t                m_Bits{};
};

struct PhysBodyID
{
    static constexpr uint32_t InvalidID = 0xffffffff;

    uint32_t                ID;

                            PhysBodyID() : ID(InvalidID) {}
    explicit                PhysBodyID(uint32_t id) : ID(id) {}

    bool                    IsInvalid() const { return ID == InvalidID; }
};

struct RayCastFilter
{
    // TODO: Add list of entities to ignore

    BroadphaseLayerMask     BroadphaseLayers;

    bool                    IgonreBackFaces : 1;
    bool                    SortByDistance : 1;
    bool                    CalcSurfcaceNormal : 1;

    RayCastFilter()
    {
        IgonreBackFaces = true;
        SortByDistance = true;
        CalcSurfcaceNormal = false;
    }
};

struct RayCastResult
{
    PhysBodyID              BodyID;

    /// Hit fraction
    float                   Fraction;

    /// World space surface normal
    Float3                  Normal;
};

struct ShapeCastFilter
{
    // TODO: Add list of entities to ignore

    BroadphaseLayerMask     BroadphaseLayers;

    bool                    IgonreBackFaces : 1;
    bool                    SortByDistance : 1;

    ShapeCastFilter()
    {
        IgonreBackFaces = true;
        SortByDistance = true;
    }
};

struct ShapeCastResult
{
    PhysBodyID              BodyID;
    /// Contact point on the surface of shape 1 (in world space or relative to base offset)
    Float3                  ContactPointOn1;
    /// Contact point on the surface of shape 2 (in world space or relative to base offset). If the penetration depth is 0, this will be the same as ContactPointOn1.
    Float3                  ContactPointOn2;
    /// Direction to move shape 2 out of collision along the shortest path (magnitude is meaningless, in world space). You can use -PenetrationAxis.Normalized() as contact normal.
    Float3                  PenetrationAxis;
    /// Penetration depth (move shape 2 by this distance to resolve the collision)
    float                   PenetrationDepth;
    /// This is the fraction where the shape hit the other shape: CenterOfMassOnHit = Start + value * (End - Start)
    float                   Fraction;
    /// True if the shape was hit from the back side
    bool                    IsBackFaceHit;
};

struct ShapeCollideResult
{
    PhysBodyID              BodyID;
    /// Contact point on the surface of shape 1 (in world space or relative to base offset)
    Float3                  ContactPointOn1;
    /// Contact point on the surface of shape 2 (in world space or relative to base offset). If the penetration depth is 0, this will be the same as ContactPointOn1.
    Float3                  ContactPointOn2;
    /// Direction to move shape 2 out of collision along the shortest path (magnitude is meaningless, in world space). You can use -PenetrationAxis.Normalized() as contact normal.
    Float3                  PenetrationAxis;
    /// Penetration depth (move shape 2 by this distance to resolve the collision)
    float                   PenetrationDepth;
};

struct ShapeOverlapFilter
{
    BroadphaseLayerMask    BroadphaseLayers;
};

class PhysicsInterface : public WorldInterfaceBase
{
public:
                            PhysicsInterface();

    bool                    CastRayClosest(Float3 const& inRayStart, Float3 const& inRayDir, RayCastResult& outResult, RayCastFilter const& inFilter = {});
    bool                    CastRay(Float3 const& inRayStart, Float3 const& inRayDir, TVector<RayCastResult>& outResult, RayCastFilter const& inFilter = {});

    bool                    CastBoxClosest(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastBox(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastBoxMinMaxClosest(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastSphereClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastSphere(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastCapsuleClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastCapsule(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastCylinderClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastCylinder(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    void                    OverlapBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void                    OverlapBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void                    OverlapSphere(Float3 const& inPosition, float inRadius, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void                    OverlapPoint(Float3 const& inPosition, TVector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});

    bool                    CheckBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool                    CheckBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, ShapeCastFilter const& inFilter = {});
    bool                    CheckSphere(Float3 const& inPosition, float inRadius, ShapeCastFilter const& inFilter = {});
    bool                    CheckCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool                    CheckCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool                    CheckPoint(Float3 const& inPosition, BroadphaseLayerMask inBroadphaseLayers = {});

    void                    CollideBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideSphere(Float3 const& inPosition, float inRadius, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, TVector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollidePoint(Float3 const& inPosition, TVector<PhysBodyID>& outResult, BroadphaseLayerMask inBroadphaseLayers = {});

    void                    SetGravity(Float3 const inGravity);
    Float3                  GetGravity() const;

    class PhysicsInterfaceImpl* GetImpl() { return m_pImpl.RawPtr(); }

protected:
    virtual void            Initialize() override;
    virtual void            Deinitialize() override;
    virtual void            Purge() override;

private:
    void                    Update();
    void                    PostTransform();
    void                    DrawDebug(DebugRenderer& renderer);
    template <typename T>
    void                    DrawRigidBody(DebugRenderer& renderer, PhysBodyID bodyID, T* rigidBody);
    void                    DrawHeightField(DebugRenderer& renderer, PhysBodyID bodyID, class HeightFieldComponent* heightfield);

    TUniqueRef<class PhysicsInterfaceImpl> m_pImpl;

    // Used for debug draw
    TVector<PhysBodyID>     m_BodyQueryResult;
    TVector<Float3>         m_DebugDrawVertices;
    TVector<unsigned int>   m_DebugDrawIndices;
};

HK_NAMESPACE_END
