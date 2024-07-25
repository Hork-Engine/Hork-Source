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

#include <Engine/World/WorldInterface.h>
#include <Engine/World/Component.h>
#include <Engine/World/ComponentManager.h>
#include <Engine/Core/Ref.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

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

struct ObjectLayerMask
{
    ObjectLayerMask&        AddLayer(uint32_t layer) { m_Bits |= HK_BIT(layer); return *this; }
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

    bool                    IsValid() const { return ID != InvalidID; }
};

struct RayCastFilter
{
    // TODO: Add list of entities to ignore

    BroadphaseLayerMask     BroadphaseLayers;
    ObjectLayerMask         ObjectLayers;

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
    ObjectLayerMask         ObjectLayers;

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

enum class ScalingMode : uint8_t
{
    NonUniform,
    UniformXZ,
    Uniform
};

class CollisionFilter;

class PhysicsInterface : public WorldInterfaceBase
{
public:
                            PhysicsInterface();

    bool                    CastRayClosest(Float3 const& inRayStart, Float3 const& inRayDir, RayCastResult& outResult, RayCastFilter const& inFilter = {});
    bool                    CastRay(Float3 const& inRayStart, Float3 const& inRayDir, Vector<RayCastResult>& outResult, RayCastFilter const& inFilter = {});

    bool                    CastBoxClosest(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastBox(Float3 const& inRayStart, Float3 const& inRayDir, Float3 const& inHalfExtent, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastBoxMinMaxClosest(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Float3 const& inRayDir, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastSphereClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastSphere(Float3 const& inRayStart, Float3 const& inRayDir, float inRadius, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastCapsuleClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastCapsule(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    bool                    CastCylinderClosest(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastResult& outResult, ShapeCastFilter const& inFilter = {});
    bool                    CastCylinder(Float3 const& inRayStart, Float3 const& inRayDir, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCastResult>& outResult, ShapeCastFilter const& inFilter = {});

    void                    OverlapBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void                    OverlapBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void                    OverlapSphere(Float3 const& inPosition, float inRadius, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});
    void                    OverlapPoint(Float3 const& inPosition, Vector<PhysBodyID>& outResult, ShapeOverlapFilter const& inFilter = {});

    bool                    CheckBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool                    CheckBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, ShapeCastFilter const& inFilter = {});
    bool                    CheckSphere(Float3 const& inPosition, float inRadius, ShapeCastFilter const& inFilter = {});
    bool                    CheckCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool                    CheckCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, ShapeCastFilter const& inFilter = {});
    bool                    CheckPoint(Float3 const& inPosition, BroadphaseLayerMask inBroadphaseLayers = {}, ObjectLayerMask inObjectLayers = {});

    void                    CollideBox(Float3 const& inPosition, Float3 const& inHalfExtent, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideBoxMinMax(Float3 const& inMins, Float3 const& inMaxs, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideSphere(Float3 const& inPosition, float inRadius, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideCapsule(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollideCylinder(Float3 const& inPosition, float inHalfHeight, float inRadius, Quat const& inRotation, Vector<ShapeCollideResult>& outResult, ShapeCastFilter const& inFilter = {});
    void                    CollidePoint(Float3 const& inPosition, Vector<PhysBodyID>& outResult, BroadphaseLayerMask inBroadphaseLayers = {}, ObjectLayerMask inObjectLayers = {});

    void                    SetGravity(Float3 const inGravity);
    Float3                  GetGravity() const;

    void                    SetCollisionFilter(CollisionFilter const& inCollisionFilter);
    CollisionFilter const&  GetCollisionFilter() const;

    Component*              TryGetComponent(PhysBodyID inBodyID);

    template <typename ComponentType>
    ComponentType*          TryGetComponent(PhysBodyID inBodyID);

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

    UniqueRef<class PhysicsInterfaceImpl> m_pImpl;

    // Used for debug draw
    Vector<PhysBodyID>      m_BodyQueryResult;
    Vector<Float3>          m_DebugDrawVertices;
    Vector<unsigned int>    m_DebugDrawIndices;
};

template <typename ComponentType>
HK_INLINE ComponentType* PhysicsInterface::TryGetComponent(PhysBodyID inBodyID)
{
    Component* component = TryGetComponent(inBodyID);
    if (!component || component->GetManager()->GetComponentTypeID() != ComponentRTTR::TypeID<ComponentType>)
        return nullptr;

    return static_cast<ComponentType*>(component);
}

HK_NAMESPACE_END
