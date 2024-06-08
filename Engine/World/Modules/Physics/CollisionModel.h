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

// TODO: Remove JPH from public headers
#include "JoltPhysics.h"
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

#include <Engine/Core/Containers/Vector.h>
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

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
    //unsigned int const* pIndices{};
    //int                 IndexCount{};
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

class CollisionModel : public RefCounted
{
public:
    static Ref<CollisionModel>     Create(CollisionModelCreateInfo const& inCreateInfo);

    Float3 const&                   GetCenterOfMass() const;

    Float3                          GetValidScale(Float3 const& inScale) const;

    CollisionInstanceRef            Instatiate(Float3 const& inScale);

    void                            GatherGeometry(Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const;

    void                            DrawDebug(DebugRenderer& inRenderer, Float3x4 const& inTransform) const;

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

class TerrainCollision : public RefCounted
{
public:
    static Ref<TerrainCollision>   Create(const float* inSamples, uint32_t inSampleCount, const uint8_t* inMaterialIndices = nullptr, const JPH::PhysicsMaterialList& inMaterialList = JPH::PhysicsMaterialList());

    CollisionInstanceRef            Instatiate();

    /// Get height field position at sampled location (inX, inY).
    /// where inX and inY are integers in the range inX e [0, mSampleCount - 1] and inY e [0, mSampleCount - 1].
    Float3                          GetPosition(uint32_t inX, uint32_t inY) const;

    /// Check if height field at sampled location (inX, inY) has collision (has a hole or not)
    bool                            IsNoCollision(uint32_t inX, uint32_t inY) const;

    /// Projects inLocalPosition (a point in the space of the shape) along the Y axis onto the surface and returns it in outSurfacePosition.
    /// When there is no surface position (because of a hole or because the point is outside the heightfield) the function will return false.
    bool                            ProjectOntoSurface(Float3 const& inLocalPosition, Float3& outSurfacePosition, Float3& outSurfaceNormal) const;

    /// Amount of memory used by height field (size in bytes)
    size_t                          GetMemoryUsage() const;

    void                            GatherGeometry(BvAxisAlignedBox const& inLocalBounds, Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const;

private:
    TerrainCollision() = default;

    JPH::Ref<JPH::HeightFieldShape> m_Shape;
};

HK_INLINE void TransformVertices(Float3* inoutVertices, uint32_t inVertexCount, Float3x4 const& inTransform)
{
    for (uint32_t i = 0; i < inVertexCount; i++)
    {
        inoutVertices[i] = inTransform * inoutVertices[i];
    }
}

Ref<CollisionModel> CreateConvexDecomposition(Float3 const* inVertices, int inVertexCount, int inVertexStride, unsigned int const* inIndices, int inIndexCount);
Ref<CollisionModel> CreateConvexDecompositionVHACD(Float3 const* inVertices, int inVertexCount, int inVertexStride, unsigned int const* inIndices, int inIndexCount);

void DrawShape(DebugRenderer& renderer, JPH::Shape const* shape, Float3x4 const& transform);

HK_NAMESPACE_END
