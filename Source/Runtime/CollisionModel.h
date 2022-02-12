/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "Resource.h"
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/Plane.h>

class AIndexedMeshSubpart;

enum COLLISION_MASK : uint32_t
{
    CM_NOCOLLISION          = 0,
    CM_WORLD_STATIC         = HK_BIT(0),
    CM_WORLD_DYNAMIC        = HK_BIT(1),
    CM_WORLD                = CM_WORLD_STATIC | CM_WORLD_DYNAMIC,
    CM_PAWN                 = HK_BIT(2),
    CM_PROJECTILE           = HK_BIT(3),
    CM_TRIGGER              = HK_BIT(4),
    CM_CHARACTER_CONTROLLER = HK_BIT(5),
    CM_WATER                = HK_BIT(6),
    CM_SOLID                = CM_WORLD_STATIC | CM_WORLD_DYNAMIC | CM_PAWN | CM_PROJECTILE | CM_CHARACTER_CONTROLLER,
    CM_UNUSED7              = HK_BIT(7),
    CM_UNUSED8              = HK_BIT(8),
    CM_UNUSED9              = HK_BIT(9),
    CM_UNUSED10             = HK_BIT(10),
    CM_UNUSED11             = HK_BIT(11),
    CM_UNUSED12             = HK_BIT(12),
    CM_UNUSED13             = HK_BIT(13),
    CM_UNUSED14             = HK_BIT(14),
    CM_UNUSED15             = HK_BIT(15),
    CM_UNUSED16             = HK_BIT(16),
    CM_UNUSED17             = HK_BIT(17),
    CM_UNUSED18             = HK_BIT(18),
    CM_UNUSED19             = HK_BIT(19),
    CM_UNUSED20             = HK_BIT(20),
    CM_UNUSED21             = HK_BIT(21),
    CM_UNUSED22             = HK_BIT(22),
    CM_UNUSED23             = HK_BIT(23),
    CM_UNUSED24             = HK_BIT(24),
    CM_UNUSED25             = HK_BIT(25),
    CM_UNUSED26             = HK_BIT(26),
    CM_UNUSED27             = HK_BIT(27),
    CM_UNUSED28             = HK_BIT(28),
    CM_UNUSED29             = HK_BIT(29),
    CM_UNUSED30             = HK_BIT(30),
    CM_UNUSED31             = HK_BIT(31),
    CM_ALL                  = 0xffffffff
};

HK_FLAG_ENUM_OPERATORS(COLLISION_MASK)

enum COLLISION_SHAPE
{
    COLLISION_SHAPE_SPHERE,
    COLLISION_SHAPE_SPHERE_RADII,
    COLLISION_SHAPE_BOX,
    COLLISION_SHAPE_CYLINDER,
    COLLISION_SHAPE_CONE,
    COLLISION_SHAPE_CAPSULE,
    COLLISION_SHAPE_CONVEX_HULL,
    COLLISION_SHAPE_TRIANGLE_SOUP_BVH,
    COLLISION_SHAPE_TRIANGLE_SOUP_GIMPACT,
    COLLISION_SHAPE_CONVEX_DECOMPOSITION,
    COLLISION_SHAPE_CONVEX_DECOMPOSITION_VHACD
};

enum
{
    COLLISION_SHAPE_AXIAL_X,
    COLLISION_SHAPE_AXIAL_Y,
    COLLISION_SHAPE_AXIAL_Z,
    COLLISION_SHAPE_AXIAL_DEFAULT = COLLISION_SHAPE_AXIAL_Y
};

struct SCollisionMeshSubpart
{
    int BaseVertex;
    int VertexCount;
    int FirstIndex;
    int IndexCount;
};

struct SCollisionBone
{
    int            JointIndex{-1};
    COLLISION_MASK CollisionGroup{CM_NOCOLLISION};
    COLLISION_MASK CollisionMask{CM_NOCOLLISION};
};

struct SCollisionSphereDef
{
    COLLISION_SHAPE Type{COLLISION_SHAPE_SPHERE};
    void*           pNext{};
    Float3          Position;
    float           Margin{0.01f};
    float           Radius{0.5f};
    SCollisionBone  Bone;
};

struct SCollisionSphereRadiiDef
{
    COLLISION_SHAPE Type{COLLISION_SHAPE_SPHERE_RADII};
    void*           pNext{};
    Float3          Position;
    Quat            Rotation;
    float           Margin{0.01f};
    Float3          Radius{0.5f, 0.5f, 0.5f};
    SCollisionBone  Bone;
};

struct SCollisionBoxDef
{
    COLLISION_SHAPE Type{COLLISION_SHAPE_BOX};
    void*           pNext{};
    Float3          Position;
    Quat            Rotation;
    float           Margin{0.01f};
    Float3          HalfExtents{0.5f, 0.5f, 0.5f};
    SCollisionBone  Bone;
};

struct SCollisionCylinderDef
{
    COLLISION_SHAPE Type{COLLISION_SHAPE_CYLINDER};
    void*           pNext{};
    Float3          Position;
    Quat            Rotation;
    float           Margin{0.01f};
    float           Radius{0.5f};
    float           Height{1};
    int             Axial{COLLISION_SHAPE_AXIAL_DEFAULT};
    SCollisionBone  Bone;
};

struct SCollisionConeDef
{
    COLLISION_SHAPE Type{COLLISION_SHAPE_CONE};
    void*           pNext{};
    Float3          Position;
    Quat            Rotation;
    float           Margin{0.01f};
    float           Radius{0.5f};
    float           Height{1};
    int             Axial{COLLISION_SHAPE_AXIAL_DEFAULT};
    SCollisionBone  Bone;
};

struct SCollisionCapsuleDef
{
    COLLISION_SHAPE Type{COLLISION_SHAPE_CAPSULE};
    void*           pNext{};
    Float3          Position;
    Quat            Rotation;
    float           Margin{0.01f};
    float           Radius{0.5f};
    float           Height{1};
    int             Axial{COLLISION_SHAPE_AXIAL_DEFAULT};
    SCollisionBone  Bone;
};

struct SCollisionConvexHullDef
{
    COLLISION_SHAPE     Type{COLLISION_SHAPE_CONVEX_HULL};
    void*               pNext{};
    Float3              Position;
    Quat                Rotation;
    float               Margin{0.01f};
    Float3 const*       pVertices{};
    int                 VertexCount{};
    unsigned int const* pIndices{};
    int                 IndexCount{};
    PlaneF const*       pPlanes{};
    int                 PlaneCount{};
    SCollisionBone      Bone;
};

struct SCollisionTriangleSoupBVHDef
{
    COLLISION_SHAPE              Type{COLLISION_SHAPE_TRIANGLE_SOUP_BVH};
    void*                        pNext{};
    Float3                       Position;
    Quat                         Rotation;
    float                        Margin{0.01f};
    Float3 const*                pVertices{};
    int                          VertexStride{};
    int                          VertexCount{};
    unsigned int const*          pIndices{};
    int                          IndexCount{};
    SCollisionMeshSubpart const* pSubparts{};
    AIndexedMeshSubpart* const*  pIndexedMeshSubparts{};
    int                          SubpartCount{};
    bool                         bForceQuantizedAabbCompression{false};
};

struct SCollisionTriangleSoupGimpactDef
{
    COLLISION_SHAPE              Type{COLLISION_SHAPE_TRIANGLE_SOUP_GIMPACT};
    void*                        pNext{};
    Float3                       Position;
    Quat                         Rotation;
    float                        Margin{0.01f};
    Float3 const*                pVertices{};
    int                          VertexStride{};
    int                          VertexCount{};
    unsigned int const*          pIndices{};
    int                          IndexCount{};
    SCollisionMeshSubpart const* pSubparts{};
    AIndexedMeshSubpart* const*  pIndexedMeshSubparts{};
    int                          SubpartCount{};
};

struct SCollisionConvexDecompositionDef
{
    COLLISION_SHAPE     Type{COLLISION_SHAPE_CONVEX_DECOMPOSITION};
    void*               pNext{};
    Float3 const*       pVertices{};
    int                 VerticesCount{};
    int                 VertexStride{};
    unsigned int const* pIndices{};
    int                 IndicesCount{};
};

struct SCollisionConvexDecompositionVHACDDef
{
    COLLISION_SHAPE     Type{COLLISION_SHAPE_CONVEX_DECOMPOSITION_VHACD};
    void*               pNext{};
    Float3 const*       pVertices{};
    int                 VerticesCount{};
    int                 VertexStride{};
    unsigned int const* pIndices{};
    int                 IndicesCount{};
};

struct ACollisionBody
{
    Float3 Position;
    Quat   Rotation;
    float  Margin{0.01f};

    virtual ~ACollisionBody() {}

    virtual class btCollisionShape* Create(Float3 const& Scale) { return nullptr; }
    virtual void                    GatherGeometry(TPodVectorHeap<Float3>& Vertices, TPodVectorHeap<unsigned int>& Indices, Float3x4 const& Transform) const {}
};

struct SBoneCollision
{
    int                        JointIndex;
    COLLISION_MASK             CollisionGroup;
    COLLISION_MASK             CollisionMask;
    TUniqueRef<ACollisionBody> CollisionBody;
};

struct SCollisionModelCreateInfo
{
    void const* pShapes{};
    Float3      CenterOfMass;
    bool        bOverrideCenterOfMass{};
};

class ACollisionInstance;

// TODO: Collision model should be an immutable object
class ACollisionModel : public AResource
{
    HK_CLASS(ACollisionModel, AResource)

public:
    ACollisionModel();
    ACollisionModel(void const* pShapes);
    ACollisionModel(SCollisionModelCreateInfo const& CreateInfo);
    ~ACollisionModel();

    Float3 const& GetCenterOfMass() const { return CenterOfMass; }

    bool IsEmpty() const { return CollisionBodies.IsEmpty(); }

    TStdVector<SBoneCollision> const& GetBoneCollisions() const { return BoneCollisions; }

    void GatherGeometry(TPodVectorHeap<Float3>& Vertices, TPodVectorHeap<unsigned int>& Indices, Float3x4 const& Transform) const;

    /** Create a scaled instance of the collision model. */
    TRef<ACollisionInstance> Instantiate(Float3 const& Scale);

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStream& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(const char* Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/CollisionModel/Default"; }

private:
    void AddSphere(SCollisionSphereDef const* pShape, int& NumShapes);
    void AddSphereRadii(SCollisionSphereRadiiDef const* pShape, int& NumShapes);
    void AddBox(SCollisionBoxDef const* pShape, int& NumShapes);
    void AddCylinder(SCollisionCylinderDef const* pShape, int& NumShapes);
    void AddCone(SCollisionConeDef const* pShape, int& NumShapes);
    void AddCapsule(SCollisionCapsuleDef const* pShape, int& NumShapes);
    void AddConvexHull(SCollisionConvexHullDef const* pShape, int& NumShapes);
    void AddTriangleSoupBVH(SCollisionTriangleSoupBVHDef const* pShape, int& NumShapes);
    void AddTriangleSoupGimpact(SCollisionTriangleSoupGimpactDef const* pShape, int& NumShapes);
    void AddConvexDecomposition(SCollisionConvexDecompositionDef const* pShape, int& NumShapes);
    void AddConvexDecompositionVHACD(SCollisionConvexDecompositionVHACDDef const* pShape, int& NumShapes);

    TStdVector<TUniqueRef<ACollisionBody>> CollisionBodies;
    TStdVector<SBoneCollision>     BoneCollisions;
    Float3                         CenterOfMass;

    // Collision instance has access to CollisionBodies
    friend class ACollisionInstance;
};

/** Collision instance is an immutable object */
class ACollisionInstance : public ARefCounted
{
public:
    ACollisionInstance(ACollisionModel* CollisionModel, Float3 const& Scale);
    ~ACollisionInstance();

    Float3 CalculateLocalInertia(float Mass) const;

    Float3 const& GetCenterOfMass() const { return CenterOfMass; }

    void GetCollisionBodiesWorldBounds(Float3 const& WorldPosition, Quat const& WorldRotation, TPodVector<BvAxisAlignedBox>& BoundingBoxes) const;

    void GetCollisionWorldBounds(Float3 const& WorldPosition, Quat const& WorldRotation, BvAxisAlignedBox& BoundingBox) const;

    void GetCollisionBodyWorldBounds(int Index, Float3 const& WorldPosition, Quat const& WorldRotation, BvAxisAlignedBox& BoundingBox) const;

    void GetCollisionBodyLocalBounds(int Index, BvAxisAlignedBox& BoundingBox) const;

    float GetCollisionBodyMargin(int Index) const;

    int GetCollisionBodiesCount() const;

    btCollisionShape* GetCollisionShape() const { return CollisionShape; }

private:
    TRef<ACollisionModel>             Model;
    TUniqueRef<class btCompoundShape> CompoundShape;
    btCollisionShape*                 CollisionShape;
    Float3                            CenterOfMass;
};
