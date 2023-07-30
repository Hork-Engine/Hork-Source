#pragma once

#include <Engine/Runtime/GarbageCollector.h>

#include "JoltPhysics.h"
#include <Jolt/Physics/Collision/Shape/Shape.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

struct CollisionSphereDef
{
    Float3          Position;
    float           Radius{0.5f};
};

struct CollisionBoxDef
{
    Float3          Position;
    Quat            Rotation;
    Float3          HalfExtents{0.5f, 0.5f, 0.5f};
};

struct CollisionCylinderDef
{
    Float3          Position;
    Quat            Rotation;
    float           Radius{0.5f};
    float           Height{1};
};

struct CollisionConeDef
{
    Float3          Position;
    Quat            Rotation;
    float           Radius{0.5f};
    float           Height{1};
};

struct CollisionCapsuleDef
{
    Float3          Position;
    Quat            Rotation;
    float           Radius{0.5f};
    float           Height{1};
};

struct CollisionConvexHullDef
{
    Float3              Position;
    Quat                Rotation;
    Float3 const*       pVertices{};
    int                 VertexCount{};
    unsigned int const* pIndices{};
    int                 IndexCount{};
};

struct CollisionTriangleSoupDef
{
    Float3              Position;
    Quat                Rotation;
    Float3 const*       pVertices{};
    int                 VertexStride{};
    int                 VertexCount{};
    unsigned int const* pIndices{};
    int                 IndexCount{};
};

struct CollisionModelCreateInfo
{
    CollisionSphereDef* pSpheres{};
    int SphereCount{};

    CollisionBoxDef* pBoxes{};
    int BoxCount{};

    CollisionCylinderDef* pCylinders{};
    int CylinderCount{};

    CollisionCapsuleDef* pCapsules{};
    int CapsuleCount{};

    CollisionConvexHullDef* pConvexHulls{};
    int ConvexHullCount{};

    CollisionTriangleSoupDef* pTriangleMeshes{};
    int TriangleMeshCount{};
};

using CollisionInstanceRef = JPH::Ref<JPH::Shape>;

class CollisionModel : public GCObject
{
public:
    static CollisionModel* Create(CollisionModelCreateInfo const& createInfo);

    Float3 const& GetCenterOfMass() const;

    Float3 GetCenterOfMassWorldPosition(Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale) const;

    Float3 GetValidScale(Float3 const& scale) const;

    CollisionInstanceRef Instatiate(Float3 const& scale);

    void GatherGeometry(TVector<Float3>& vertices, TVector<unsigned int>& indices, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale) const;

    void DrawDebug(DebugRenderer& renderer, Float3 const& worldPosition, Quat const& worldRotation, Float3 const& worldScale) const;

private:
    CollisionModel() = default;

    JPH::Ref<JPH::Shape> m_Shape;
    Float3 m_CenterOfMass;

    enum SCALE_MODE : uint8_t
    {
        NON_UNIFORM,
        UNIFORM_XZ,
        UNIFORM
    };
    SCALE_MODE m_AllowedScalingMode{NON_UNIFORM};
};

CollisionModel* CreateConvexDecomposition(Float3 const* vertices,
                                          int vertexCount,
                                          int vertexStride,
                                          unsigned int const* indices,
                                          int indexCount);

CollisionModel* CreateConvexDecompositionVHACD(Float3 const* vertices,
                                               int vertexCount,
                                               int vertexStride,
                                               unsigned int const* indices,
                                               int indexCount);

HK_NAMESPACE_END
