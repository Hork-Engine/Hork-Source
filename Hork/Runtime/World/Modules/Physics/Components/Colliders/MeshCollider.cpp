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

#include "MeshCollider.h"
#include "../../PhysicsInterfaceImpl.h"

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <Hork/Geometry/ConvexDecomposition.h>

HK_NAMESPACE_BEGIN

MeshCollisionData::MeshCollisionData() :
    m_Data(new MeshCollisionDataInternal)
{}

bool MeshCollisionData::IsEmpty() const
{
    return m_Data->m_Shape != nullptr;
}

void MeshCollisionData::Clear()
{
    m_Data->m_Shape = {};

    // Reset to default
    m_IsConvex = false;
}

void MeshCollisionData::CreateConvexHull(ArrayView<Float3> hullVertices)
{
    m_IsConvex = true;

    auto vertexCount = hullVertices.Size();
    if (vertexCount < 4)
    {
        Clear();
        return;
    }

    JPH::ConvexHullShapeSettings convexHullSettings;
    convexHullSettings.mMaxConvexRadius = JPH::cDefaultConvexRadius;
    convexHullSettings.mPoints.resize(vertexCount);
    for (int i = 0 ; i < vertexCount ; ++i)
        convexHullSettings.mPoints[i] = ConvertVector(hullVertices[i]);

    JPH::ShapeSettings::ShapeResult result;
    m_Data->m_Shape = new JPH::ConvexHullShape(convexHullSettings, result);
}

void MeshCollisionData::CreateTriangleSoup(ArrayView<Float3> vertices, ArrayView<uint32_t> indices)
{
    CreateTriangleSoup(vertices.ToPtr(), sizeof(Float3), vertices.Size(), indices.ToPtr(), indices.Size());
}

void MeshCollisionData::CreateTriangleSoup(Float3 const* vertices, size_t vertexStride, size_t vertexCount, uint32_t const* indices, size_t indexCount)
{
    m_IsConvex = false;

    uint32_t triangleCount = indexCount / 3;
    if (triangleCount == 0)
    {
        Clear();
        return;
    }

    JPH::MeshShapeSettings meshSettings;
    meshSettings.mTriangleVertices.resize(vertexCount);

    static_assert(sizeof(JPH::Float3) == sizeof(Float3), "Type sizeof mismatch");
    if (vertexStride == sizeof(Float3))
    {
        Core::Memcpy(meshSettings.mTriangleVertices.data(), vertices, vertexCount * sizeof(JPH::Float3));
    }
    else
    {
        for (size_t i = 0; i < vertexCount; ++i)
            *(Float3*)(&meshSettings.mTriangleVertices[i]) = *(Float3 const*)((uint8_t const*)vertices + i * vertexStride);
    }

    meshSettings.mIndexedTriangles.resize(triangleCount);
    for (int i = 0 ; i < triangleCount ; ++i)
    {
        meshSettings.mIndexedTriangles[i].mIdx[0] = indices[i*3 + 0];
        meshSettings.mIndexedTriangles[i].mIdx[1] = indices[i*3 + 1];
        meshSettings.mIndexedTriangles[i].mIdx[2] = indices[i*3 + 2];
    }

    meshSettings.Sanitize();

    JPH::ShapeSettings::ShapeResult result;
    m_Data->m_Shape = new JPH::MeshShape(meshSettings, result);
}

bool CreateConvexDecomposition(GameObject* object, Float3 const* inVertices, int inVertexCount, int inVertexStride, unsigned int const* inIndices, int inIndexCount)
{
    Vector<Float3> hullVertices;
    Vector<unsigned int> hullIndices;
    Vector<ConvexHullDesc> hulls;

    if (inVertexStride <= 0)
    {
        LOG("CreateConvexDecomposition: invalid VertexStride\n");
        return false;
    }

    Geometry::PerformConvexDecomposition(inVertices, inVertexCount, inVertexStride, inIndices, inIndexCount, hullVertices, hullIndices, hulls);
    if (hulls.IsEmpty())
    {
        LOG("CreateConvexDecomposition: failed on convex decomposition\n");
        return false;
    }

    for (ConvexHullDesc const& hull : hulls)
    {
        MeshCollider* collider;
        object->CreateComponent(collider);

        collider->OffsetPosition = hull.Centroid;
        collider->Data = MakeRef<MeshCollisionData>();
        collider->Data->CreateConvexHull(ArrayView<Float3>(hullVertices.ToPtr() + hull.FirstVertex, hull.VertexCount));
    }

    return true;
}

bool CreateConvexDecompositionVHACD(GameObject* object, Float3 const* inVertices, int inVertexCount, int inVertexStride, unsigned int const* inIndices, int inIndexCount)
{
    Vector<Float3> hullVertices;
    Vector<unsigned int> hullIndices;
    Vector<ConvexHullDesc> hulls;
    Float3 decompositionCenterOfMass;

    if (inVertexStride <= 0)
    {
        LOG("CreateConvexDecompositionVHACD: invalid VertexStride\n");
        return {};
    }

    Geometry::PerformConvexDecompositionVHACD(inVertices, inVertexCount, inVertexStride, inIndices, inIndexCount, hullVertices, hullIndices, hulls, decompositionCenterOfMass);
    if (hulls.IsEmpty())
    {
        LOG("CreateConvexDecompositionVHACD: failed on convex decomposition\n");
        return {};
    }

    for (ConvexHullDesc const& hull : hulls)
    {
        MeshCollider* collider;
        object->CreateComponent(collider);

        collider->OffsetPosition = hull.Centroid;
        collider->Data = MakeRef<MeshCollisionData>();
        collider->Data->CreateConvexHull(ArrayView<Float3>(hullVertices.ToPtr() + hull.FirstVertex, hull.VertexCount));
    }

    return true;
}

HK_NAMESPACE_END
