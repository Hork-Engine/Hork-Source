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

#include "CollisionModel.h"
#include "IndexedMesh.h"

#include <Platform/Logger.h>
#include <Geometry/ConvexHull.h>
#include <Geometry/ConvexDecomposition.h>

#include "BulletCompatibility.h"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btMultiSphereShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btStridingMeshInterface.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

#ifdef HK_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4456 4828)
#endif
#include <BulletCollision/Gimpact/btGImpactShape.h>
#ifdef HK_COMPILER_MSVC
#    pragma warning(pop)
#endif

#include <BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h>

#ifdef BULLET_WORLD_IMPORTER
#    include <bullet3/Extras/Serialize/BulletWorldImporter/btBulletWorldImporter.h>
#endif

HK_CLASS_META(CollisionModel)

ATTRIBUTE_ALIGNED16(class)
StridingMeshInterface : public btStridingMeshInterface
{
public:
    Float3*                Vertices;
    unsigned int*          Indices;
    CollisionMeshSubpart* Subparts;
    int                    SubpartCount;
    mutable btVector3      AABBMin;
    mutable btVector3      AABBMax;
    mutable bool           bHasAABB{};

    void getLockedVertexIndexBase(unsigned char** VertexBase,
                                  int&            VertexCount,
                                  PHY_ScalarType& Type,
                                  int&            VertexStride,
                                  unsigned char** IndexBase,
                                  int&            IndexStride,
                                  int&            FaceCount,
                                  PHY_ScalarType& IndexType,
                                  int             Subpart = 0) override
    {

        HK_ASSERT(Subpart < SubpartCount);

        auto* subpart = Subparts + Subpart;

        (*VertexBase) = (unsigned char*)(Vertices + subpart->BaseVertex);
        VertexCount   = subpart->VertexCount;
        Type          = PHY_FLOAT;
        VertexStride  = sizeof(Vertices[0]);

        (*IndexBase) = (unsigned char*)(Indices + subpart->FirstIndex);
        IndexStride  = sizeof(Indices[0]) * 3;
        FaceCount    = subpart->IndexCount / 3;
        IndexType    = PHY_INTEGER;
    }

    void getLockedReadOnlyVertexIndexBase(const unsigned char** VertexBase,
                                          int&                  VertexCount,
                                          PHY_ScalarType&       Type,
                                          int&                  VertexStride,
                                          const unsigned char** IndexBase,
                                          int&                  IndexStride,
                                          int&                  FaceCount,
                                          PHY_ScalarType&       IndexType,
                                          int                   Subpart = 0) const override
    {
        HK_ASSERT(Subpart < SubpartCount);

        auto* subpart = Subparts + Subpart;

        (*VertexBase) = (const unsigned char*)(Vertices + subpart->BaseVertex);
        VertexCount   = subpart->VertexCount;
        Type          = PHY_FLOAT;
        VertexStride  = sizeof(Vertices[0]);

        (*IndexBase) = (const unsigned char*)(Indices + subpart->FirstIndex);
        IndexStride  = sizeof(Indices[0]) * 3;
        FaceCount    = subpart->IndexCount / 3;
        IndexType    = PHY_INTEGER;
    }

    // unLockVertexBase finishes the access to a subpart of the triangle mesh
    // make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
    void unLockVertexBase(int subpart) override { (void)subpart; }

    void unLockReadOnlyVertexBase(int subpart) const override { (void)subpart; }

    // getNumSubParts returns the number of seperate subparts
    // each subpart has a continuous array of vertices and indices
    int getNumSubParts() const override
    {
        return SubpartCount;
    }

    void preallocateVertices(int numverts) override { (void)numverts; }
    void preallocateIndices(int numindices) override { (void)numindices; }

    bool hasPremadeAabb() const override { return bHasAABB; }

    void setPremadeAabb(const btVector3& aabbMin, const btVector3& aabbMax) const override
    {
        AABBMin  = aabbMin;
        AABBMax  = aabbMax;
        bHasAABB = true;
    }

    void getPremadeAabb(btVector3 * aabbMin, btVector3 * aabbMax) const override
    {
        *aabbMin = AABBMin;
        *aabbMax = AABBMax;
    }
};

struct CollisionSphere : CollisionBody
{
    float Radius = 0.5f;

    btCollisionShape* Create(Float3 const& Scale) override
    {
        const float Epsilon = 0.0001f;
        if (Math::Abs(Scale.X - Scale.Y) < Epsilon && Math::Abs(Scale.X - Scale.Z) < Epsilon)
        {
            return new btSphereShape(Radius * Scale.X);
        }

        btVector3 pos(0, 0, 0);
        btMultiSphereShape* shape = new btMultiSphereShape(&pos, &Radius, 1);
        shape->setLocalScaling(btVectorToFloat3(Scale));

        return shape;
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        float sinTheta, cosTheta, sinPhi, cosPhi;

        const float detail = Math::Floor(Math::Max(1.0f, Radius) + 0.5f);

        const int numStacks = 8 * detail;
        const int numSlices = 12 * detail;

        const int vertexCount = (numStacks + 1) * numSlices;
        const int indexCount  = numStacks * numSlices * 6;

        const int firstVertex = _Vertices.Size();
        const int firstIndex  = _Indices.Size();

        _Vertices.Resize(firstVertex + vertexCount);
        _Indices.Resize(firstIndex + indexCount);

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        for (int stack = 0; stack <= numStacks; ++stack)
        {
            const float theta = stack * Math::_PI / numStacks;
            Math::SinCos(theta, sinTheta, cosTheta);

            for (int slice = 0; slice < numSlices; ++slice)
            {
                const float phi = slice * Math::_2PI / numSlices;
                Math::SinCos(phi, sinPhi, cosPhi);

                *pVertices++ = Transform * (Float3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta) * Radius + Position);
            }
        }

        for (int stack = 0; stack < numStacks; ++stack)
        {
            const int stackOffset     = firstVertex + stack * numSlices;
            const int nextStackOffset = firstVertex + (stack + 1) * numSlices;

            for (int slice = 0; slice < numSlices; ++slice)
            {
                const int nextSlice = (slice + 1) % numSlices;
                *pIndices++         = stackOffset + slice;
                *pIndices++         = stackOffset + nextSlice;
                *pIndices++         = nextStackOffset + nextSlice;
                *pIndices++         = nextStackOffset + nextSlice;
                *pIndices++         = nextStackOffset + slice;
                *pIndices++         = stackOffset + slice;
            }
        }
    }
};

struct CollisionSphereRadii : CollisionBody
{
    Float3 Radius = Float3(0.5f);

    btCollisionShape* Create(Float3 const& Scale) override
    {
        btVector3           pos(0, 0, 0);
        float               radius = 1.0f;
        btMultiSphereShape* shape  = new btMultiSphereShape(&pos, &radius, 1);
        shape->setLocalScaling(btVectorToFloat3(Radius * Scale));
        return shape;
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        float sinTheta, cosTheta, sinPhi, cosPhi;

        const float detail = Math::Floor(Math::Max(1.0f, Radius.Max()) + 0.5f);

        const int numStacks = 8 * detail;
        const int numSlices = 12 * detail;

        const int vertexCount = (numStacks + 1) * numSlices;
        const int indexCount  = numStacks * numSlices * 6;

        const int firstVertex = _Vertices.Size();
        const int firstIndex  = _Indices.Size();

        _Vertices.Resize(firstVertex + vertexCount);
        _Indices.Resize(firstIndex + indexCount);

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        for (int stack = 0; stack <= numStacks; ++stack)
        {
            const float theta = stack * Math::_PI / numStacks;
            Math::SinCos(theta, sinTheta, cosTheta);

            for (int slice = 0; slice < numSlices; ++slice)
            {
                const float phi = slice * Math::_2PI / numSlices;
                Math::SinCos(phi, sinPhi, cosPhi);

                *pVertices++ = Transform * (Rotation * (Float3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta) * Radius) + Position);
            }
        }

        for (int stack = 0; stack < numStacks; ++stack)
        {
            const int stackOffset     = firstVertex + stack * numSlices;
            const int nextStackOffset = firstVertex + (stack + 1) * numSlices;

            for (int slice = 0; slice < numSlices; ++slice)
            {
                const int nextSlice = (slice + 1) % numSlices;
                *pIndices++         = stackOffset + slice;
                *pIndices++         = stackOffset + nextSlice;
                *pIndices++         = nextStackOffset + nextSlice;
                *pIndices++         = nextStackOffset + nextSlice;
                *pIndices++         = nextStackOffset + slice;
                *pIndices++         = stackOffset + slice;
            }
        }
    }
};

struct CollisionBox : CollisionBody
{
    Float3 HalfExtents = Float3(0.5f);

    btCollisionShape* Create(Float3 const& Scale) override
    {
        return new btBoxShape(btVectorToFloat3(HalfExtents * Scale));
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        unsigned int const indices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

        const int firstVertex = _Vertices.Size();
        const int firstIndex  = _Indices.Size();

        _Vertices.Resize(firstVertex + 8);
        _Indices.Resize(firstIndex + 36);

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        *pVertices++ = Transform * (Rotation * Float3(-HalfExtents.X, HalfExtents.Y, -HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(HalfExtents.X, HalfExtents.Y, -HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(HalfExtents.X, HalfExtents.Y, HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(-HalfExtents.X, HalfExtents.Y, HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(-HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(HalfExtents.X, -HalfExtents.Y, HalfExtents.Z) + Position);
        *pVertices++ = Transform * (Rotation * Float3(-HalfExtents.X, -HalfExtents.Y, HalfExtents.Z) + Position);

        for (int i = 0; i < 36; i++)
        {
            *pIndices++ = firstVertex + indices[i];
        }
    }
};

struct CollisionCylinder : CollisionBody
{
    float Radius = 0.5f;
    float Height = 1;
    int   Axial  = COLLISION_SHAPE_AXIAL_DEFAULT;

    btCollisionShape* Create(Float3 const& Scale) override
    {
        switch (Axial)
        {
            case COLLISION_SHAPE_AXIAL_X:
                return new btCylinderShapeX(btVector3(Height * 0.5f * Scale.X, Radius * Scale.Y, Radius * Scale.Y));
            case COLLISION_SHAPE_AXIAL_Z:
                return new btCylinderShapeZ(btVector3(Radius * Scale.X, Radius * Scale.X, Height * 0.5f * Scale.Z));
            case COLLISION_SHAPE_AXIAL_Y:
            default:
                return new btCylinderShape(btVector3(Radius * Scale.X, Height * 0.5f * Scale.Y, Radius * Scale.X));
        }
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        float sinPhi, cosPhi;
        int   idxRadius, idxRadius2, idxHeight;

        Float3x4 transform = Transform;

        Float3 scale = {Float3(transform[0][0], transform[1][0], transform[2][0]).Length(),
                        Float3(transform[0][1], transform[1][1], transform[2][1]).Length(),
                        Float3(transform[0][2], transform[1][2], transform[2][2]).Length()};
        Float3 invScale = Float3(1.0f) / scale;

        // Remove scaling from the transform
        transform[0][0] *= invScale.X;
        transform[0][1] *= invScale.Y;
        transform[0][2] *= invScale.Z;
        transform[1][0] *= invScale.X;
        transform[1][1] *= invScale.Y;
        transform[1][2] *= invScale.Z;
        transform[2][0] *= invScale.X;
        transform[2][1] *= invScale.Y;
        transform[2][2] *= invScale.Z;

        float halfHeight = Height * 0.5f;
        float scaledRadius = Radius;

        switch (Axial)
        {
            case COLLISION_SHAPE_AXIAL_X:
                idxRadius  = 1;
                idxRadius2 = 2;
                idxHeight  = 0;

                halfHeight *= scale.X;
                scaledRadius *= scale.Y;
                break;
            case COLLISION_SHAPE_AXIAL_Z:
                idxRadius  = 0;
                idxRadius2 = 1;
                idxHeight  = 2;

                halfHeight *= scale.Z;
                scaledRadius *= scale.X;
                break;
            case COLLISION_SHAPE_AXIAL_Y:
            default:
                idxRadius  = 0;
                idxRadius2 = 2;
                idxHeight  = 1;

                halfHeight *= scale.Y;
                scaledRadius *= scale.X;
                break;
        }

        const float detail = Math::Floor(Math::Max(1.0f, scaledRadius) + 0.5f);

        const int numSlices     = 8 * detail;
        const int faceTriangles = numSlices - 2;

        const int vertexCount = numSlices * 2;
        const int indexCount  = faceTriangles * 3 * 2 + numSlices * 6;

        const int firstIndex  = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize(firstVertex + vertexCount);
        _Indices.Resize(firstIndex + indexCount);

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        Float3 vert;

        for (int slice = 0; slice < numSlices; slice++, pVertices++)
        {
            Math::SinCos(slice * Math::_2PI / numSlices, sinPhi, cosPhi);

            vert[idxRadius]  = cosPhi * scaledRadius;
            vert[idxRadius2] = sinPhi * scaledRadius;
            vert[idxHeight]  = halfHeight;

            *pVertices = transform * (Rotation * vert + Position);

            vert[idxHeight] = -vert[idxHeight];

            *(pVertices + numSlices) = transform * (Rotation * vert + Position);
        }

        const int offset     = firstVertex;
        const int nextOffset = firstVertex + numSlices;

        // top face
        for (int i = 0; i < faceTriangles; i++)
        {
            *pIndices++ = offset + i + 2;
            *pIndices++ = offset + i + 1;
            *pIndices++ = offset + 0;
        }

        // bottom face
        for (int i = 0; i < faceTriangles; i++)
        {
            *pIndices++ = nextOffset + i + 1;
            *pIndices++ = nextOffset + i + 2;
            *pIndices++ = nextOffset + 0;
        }

        for (int slice = 0; slice < numSlices; ++slice)
        {
            const int nextSlice = (slice + 1) % numSlices;
            *pIndices++         = offset + slice;
            *pIndices++         = offset + nextSlice;
            *pIndices++         = nextOffset + nextSlice;
            *pIndices++         = nextOffset + nextSlice;
            *pIndices++         = nextOffset + slice;
            *pIndices++         = offset + slice;
        }
    }
};

struct CollisionCone : CollisionBody
{
    float Radius = 0.5f;
    float Height = 1;
    int   Axial  = COLLISION_SHAPE_AXIAL_DEFAULT;

    btCollisionShape* Create(Float3 const& Scale) override
    {
        switch (Axial)
        {
            case COLLISION_SHAPE_AXIAL_X:
                return new btConeShapeX(Radius * Scale.Y, Height * Scale.X);
            case COLLISION_SHAPE_AXIAL_Z:
                return new btConeShapeZ(Radius * Scale.X, Height * Scale.Z);
            case COLLISION_SHAPE_AXIAL_Y:
            default:
                return new btConeShape(Radius * Scale.X, Height * Scale.Y);
        }
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        float sinPhi, cosPhi;
        int idxRadius, idxRadius2, idxHeight;

        Float3x4 transform = Transform;

        Float3 scale = {Float3(transform[0][0], transform[1][0], transform[2][0]).Length(),
                        Float3(transform[0][1], transform[1][1], transform[2][1]).Length(),
                        Float3(transform[0][2], transform[1][2], transform[2][2]).Length()};
        Float3 invScale = Float3(1.0f) / scale;

        // Remove scaling from the transform
        transform[0][0] *= invScale.X;
        transform[0][1] *= invScale.Y;
        transform[0][2] *= invScale.Z;
        transform[1][0] *= invScale.X;
        transform[1][1] *= invScale.Y;
        transform[1][2] *= invScale.Z;
        transform[2][0] *= invScale.X;
        transform[2][1] *= invScale.Y;
        transform[2][2] *= invScale.Z;

        float scaledHeight = Height;
        float scaledRadius = Radius;

        switch (Axial)
        {
            case COLLISION_SHAPE_AXIAL_X:
                idxRadius  = 1;
                idxRadius2 = 2;
                idxHeight  = 0;

                scaledHeight *= scale.X;
                scaledRadius *= scale.Y;
                break;
            case COLLISION_SHAPE_AXIAL_Z:
                idxRadius  = 0;
                idxRadius2 = 1;
                idxHeight  = 2;

                scaledHeight *= scale.Z;
                scaledRadius *= scale.X;
                break;
            case COLLISION_SHAPE_AXIAL_Y:
            default:
                idxRadius  = 0;
                idxRadius2 = 2;
                idxHeight  = 1;

                scaledHeight *= scale.Y;
                scaledRadius *= scale.X;
                break;
        }

        const float detail = Math::Floor(Math::Max(1.0f, scaledRadius) + 0.5f);

        const int numSlices     = 8 * detail;
        const int faceTriangles = numSlices - 2;

        const int vertexCount = numSlices + 1;
        const int indexCount  = faceTriangles * 3 + numSlices * 3;

        const int firstIndex  = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize(firstVertex + vertexCount);
        _Indices.Resize(firstIndex + indexCount);

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        Float3 vert;

        vert.Clear();
        vert[idxHeight] = scaledHeight * 0.5f;

        // top point
        *pVertices++ = transform * (Rotation * vert + Position);

        vert[idxHeight] = -scaledHeight * 0.5f;

        for (int slice = 0; slice < numSlices; slice++)
        {
            Math::SinCos(slice * Math::_2PI / numSlices, sinPhi, cosPhi);

            vert[idxRadius]  = cosPhi * scaledRadius;
            vert[idxRadius2] = sinPhi * scaledRadius;

            *pVertices++ = transform * (Rotation * vert + Position);
        }

        const int offset = firstVertex + 1;

        // bottom face
        for (int i = 0; i < faceTriangles; i++)
        {
            *pIndices++ = offset + 0;
            *pIndices++ = offset + i + 1;
            *pIndices++ = offset + i + 2;
        }

        // sides
        for (int slice = 0; slice < numSlices; ++slice)
        {
            *pIndices++ = firstVertex;
            *pIndices++ = offset + (slice + 1) % numSlices;
            *pIndices++ = offset + slice;
        }
    }
};

struct CollisionCapsule : CollisionBody
{
    /** Radius of the capsule. The total height is Height + 2 * Radius */
    float Radius = 0.5f;

    /** Height between the center of each sphere of the capsule caps */
    float Height = 1;

    int Axial = COLLISION_SHAPE_AXIAL_DEFAULT;

    btCollisionShape* Create(Float3 const& Scale) override
    {
        switch (Axial)
        {
            case COLLISION_SHAPE_AXIAL_X:
                return new btCapsuleShapeX(Radius * Scale.X, Height * Scale.X);
            case COLLISION_SHAPE_AXIAL_Z:
                return new btCapsuleShapeZ(Radius * Scale.Z, Height * Scale.Z);
            case COLLISION_SHAPE_AXIAL_Y:
            default:
                return new btCapsuleShape(Radius * Scale.Y, Height * Scale.Y);
        }
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        Float3x4 transform = Transform;

        Float3 scale = {Float3(transform[0][0], transform[1][0], transform[2][0]).Length(),
                        Float3(transform[0][1], transform[1][1], transform[2][1]).Length(),
                        Float3(transform[0][2], transform[1][2], transform[2][2]).Length()};
        Float3 invScale = Float3(1.0f) / scale;

        // Remove scaling from the transform
        transform[0][0] *= invScale.X;
        transform[0][1] *= invScale.Y;
        transform[0][2] *= invScale.Z;
        transform[1][0] *= invScale.X;
        transform[1][1] *= invScale.Y;
        transform[1][2] *= invScale.Z;
        transform[2][0] *= invScale.X;
        transform[2][1] *= invScale.Y;
        transform[2][2] *= invScale.Z;

        float scaledHeight = Height;
        float scaledRadius = Radius;

        int idxRadius, idxRadius2, idxHeight;
        switch (Axial)
        {
            case COLLISION_SHAPE_AXIAL_X:
                idxRadius  = 1;
                idxRadius2 = 2;
                idxHeight  = 0;

                scaledHeight *= scale.X;
                scaledRadius *= scale.X;
                break;
            case COLLISION_SHAPE_AXIAL_Z:
                idxRadius  = 0;
                idxRadius2 = 1;
                idxHeight  = 2;

                scaledHeight *= scale.Z;
                scaledRadius *= scale.Z;
                break;
            case COLLISION_SHAPE_AXIAL_Y:
            default:
                idxRadius  = 0;
                idxRadius2 = 2;
                idxHeight  = 1;

                scaledHeight *= scale.Y;
                scaledRadius *= scale.Y;
                break;
        }

#if 0
        // Capsule is implemented as cylinder

        float sinPhi, cosPhi;

        const float detail = Math::Floor( Math::Max( 1.0f, scaledRadius ) + 0.5f );

        const int numSlices = 8 * detail;
        const int faceTriangles = numSlices - 2;

        const int vertexCount = numSlices * 2;
        const int indexCount = faceTriangles * 3 * 2 + numSlices * 6;

        const int firstIndex = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize( firstVertex + vertexCount );
        _Indices.Resize( firstIndex + indexCount );

        Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

        Float3 vert;

        const float halfOfTotalHeight = scaledHeight*0.5f + scaledRadius;

        for ( int slice = 0; slice < numSlices; slice++, pVertices++ ) {
            Math::SinCos( slice * Math::_2PI / numSlices, sinPhi, cosPhi );

            vert[idxRadius] = cosPhi * scaledRadius;
            vert[idxRadius2] = sinPhi * scaledRadius;
            vert[idxHeight] = halfOfTotalHeight;

            *pVertices = Rotation * vert + Position;

            vert[idxHeight] = -vert[idxHeight];

            *(pVertices + numSlices ) = Rotation * vert + Position;
        }

        const int offset = firstVertex;
        const int nextOffset = firstVertex + numSlices;

        // top face
        for ( int i = 0 ; i < faceTriangles; i++ ) {
            *pIndices++ = offset + i + 2;
            *pIndices++ = offset + i + 1;
            *pIndices++ = offset + 0;
        }

        // bottom face
        for ( int i = 0 ; i < faceTriangles; i++ ) {
            *pIndices++ = nextOffset + i + 1;
            *pIndices++ = nextOffset + i + 2;
            *pIndices++ = nextOffset + 0;
        }

        for ( int slice = 0; slice < numSlices; ++slice ) {
            int nextSlice = (slice + 1) % numSlices;
            *pIndices++ = offset + slice;
            *pIndices++ = offset + nextSlice;
            *pIndices++ = nextOffset + nextSlice;
            *pIndices++ = nextOffset + nextSlice;
            *pIndices++ = nextOffset + slice;
            *pIndices++ = offset + slice;
        }
#else
        int   x, y;
        float verticalAngle, horizontalAngle;

        unsigned int quad[4];

        const float detail = Math::Floor(Math::Max(1.0f, scaledRadius) + 0.5f);

        const int numVerticalSubdivs   = 6 * detail;
        const int numHorizontalSubdivs = 8 * detail;

        const int halfVerticalSubdivs = numVerticalSubdivs >> 1;

        const int vertexCount = (numHorizontalSubdivs + 1) * (numVerticalSubdivs + 1) * 2;
        const int indexCount  = numHorizontalSubdivs * (numVerticalSubdivs + 1) * 6;

        const int firstIndex  = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize(firstVertex + vertexCount);
        _Indices.Resize(firstIndex + indexCount);

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        const float verticalStep   = Math::_PI / numVerticalSubdivs;
        const float horizontalStep = Math::_2PI / numHorizontalSubdivs;

        const float halfHeight = scaledHeight * 0.5f;

        for (y = 0, verticalAngle = -Math::_HALF_PI; y <= halfVerticalSubdivs; y++)
        {
            float h, r;
            Math::SinCos(verticalAngle, h, r);
            h = h * scaledRadius - halfHeight;
            r *= scaledRadius;
            for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
            {
                float   s, c;
                Float3& v = *pVertices++;
                Math::SinCos(horizontalAngle, s, c);
                v[idxRadius]  = r * c;
                v[idxRadius2] = r * s;
                v[idxHeight]  = h;
                v             = transform * (Rotation * v + Position);
                horizontalAngle += horizontalStep;
            }
            verticalAngle += verticalStep;
        }

        for (y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++)
        {
            float h, r;
            Math::SinCos(verticalAngle, h, r);
            h = h * scaledRadius + halfHeight;
            r *= scaledRadius;
            for (x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++)
            {
                float   s, c;
                Float3& v = *pVertices++;
                Math::SinCos(horizontalAngle, s, c);
                v[idxRadius]  = r * c;
                v[idxRadius2] = r * s;
                v[idxHeight]  = h;
                v             = transform * (Rotation * v + Position);
                horizontalAngle += horizontalStep;
            }
            verticalAngle += verticalStep;
        }

        for (y = 0; y <= numVerticalSubdivs; y++)
        {
            const int y2 = y + 1;
            for (x = 0; x < numHorizontalSubdivs; x++)
            {
                const int x2 = x + 1;
                quad[0]      = firstVertex + y * (numHorizontalSubdivs + 1) + x;
                quad[1]      = firstVertex + y2 * (numHorizontalSubdivs + 1) + x;
                quad[2]      = firstVertex + y2 * (numHorizontalSubdivs + 1) + x2;
                quad[3]      = firstVertex + y * (numHorizontalSubdivs + 1) + x2;
                *pIndices++  = quad[0];
                *pIndices++  = quad[1];
                *pIndices++  = quad[2];
                *pIndices++  = quad[2];
                *pIndices++  = quad[3];
                *pIndices++  = quad[0];
            }
        }
#endif
    }
};

struct CollisionConvexHull : CollisionBody
{
    TVector<Float3>       Vertices;
    TVector<unsigned int> Indices;

    btCollisionShape* Create(Float3 const& Scale) override
    {
#if 0
            constexpr bool bComputeAabb = false; // FIXME: Do we need to calc aabb now?
            // NOTE: btConvexPointCloudShape keeps pointer to vertices
            return new btConvexPointCloudShape( &Vertices[0][0], Vertices.Size(), btVectorToFloat3(Scale), bComputeAabb );
#else
        // NOTE: btConvexHullShape keeps copy of vertices
        auto* shape = new btConvexHullShape(&Vertices[0][0], Vertices.Size(), sizeof(Float3));
        shape->setLocalScaling(btVectorToFloat3(Scale));
        return shape;
#endif
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        if (_Vertices.IsEmpty())
        {
            return;
        }

        const int firstVertex = _Vertices.Size();
        const int firstIndex  = _Indices.Size();

        _Vertices.Resize(firstVertex + Vertices.Size());
        _Indices.Resize(firstIndex + Indices.Size());

        Float3*       pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = _Indices.ToPtr() + firstIndex;

        for (int i = 0; i < Vertices.Size(); i++)
        {
            *pVertices++ = Transform * (Rotation * Vertices[i] + Position);
        }

        for (int i = 0; i < Indices.Size(); i++)
        {
            *pIndices++ = firstVertex + Indices[i];
        }
    }
};


// CollisionTriangleSoupBVH can be used only for static or kinematic objects
struct CollisionTriangleSoupBVH : CollisionBody
{
    TVector<Float3>                Vertices;
    TVector<unsigned int>          Indices;
    TVector<CollisionMeshSubpart> Subparts;
    BvAxisAlignedBox                      BoundingBox;
    TUniqueRef<StridingMeshInterface>    pInterface;

    btCollisionShape* Create(Float3 const& Scale) override
    {
        return new btScaledBvhTriangleMeshShape(Data.GetObject(), btVectorToFloat3(Scale));

        // TODO: Create GImpact mesh shape for dynamic objects
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        if (Vertices.IsEmpty())
        {
            return;
        }

        const int firstIndex  = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize(firstVertex + Vertices.Size());

        int indexCount = 0;
        for (auto& subpart : Subparts)
        {
            indexCount += subpart.IndexCount;
        }

        _Indices.Resize(firstIndex + indexCount);

        unsigned int* pIndices = _Indices.ToPtr() + firstIndex;

        for (auto& subpart : Subparts)
        {
            for (int i = 0; i < subpart.IndexCount; i++)
            {
                *pIndices++ = firstVertex + subpart.BaseVertex + Indices[subpart.FirstIndex + i];
            }
        }

        Float3* pVertices = _Vertices.ToPtr() + firstVertex;

        for (int i = 0; i < Vertices.Size(); i++)
        {
            *pVertices++ = Transform * (Rotation * Vertices[i] + Position);
        }
    }

    void BuildBVH(bool bForceQuantizedAabbCompression = false)
    {
        pInterface->Vertices     = Vertices.ToPtr();
        pInterface->Indices      = Indices.ToPtr();
        pInterface->Subparts     = Subparts.ToPtr();
        pInterface->SubpartCount = Subparts.Size();

        if (!bForceQuantizedAabbCompression)
        {
            constexpr unsigned int QUANTIZED_AABB_COMPRESSION_MAX_TRIANGLES = 1000000;

            int indexCount = 0;
            for (int i = 0; i < pInterface->SubpartCount; i++)
            {
                indexCount += pInterface->Subparts[i].IndexCount;
            }

            // NOTE: With too many triangles, Bullet will not work correctly with Quantized Aabb Compression
            bUsedQuantizedAabbCompression = indexCount / 3 <= QUANTIZED_AABB_COMPRESSION_MAX_TRIANGLES;
        }
        else
        {
            bUsedQuantizedAabbCompression = true;
        }

        Data = MakeUnique<btBvhTriangleMeshShape>(pInterface.GetObject(),
                                                  bUsedQuantizedAabbCompression,
                                                  btVectorToFloat3(BoundingBox.Mins),
                                                  btVectorToFloat3(BoundingBox.Maxs),
                                                  true);

        TriangleInfoMap = MakeUnique<btTriangleInfoMap>();
        btGenerateInternalEdgeInfo(Data.GetObject(), TriangleInfoMap.GetObject());
    }

    bool UsedQuantizedAabbCompression() const
    {
        return bUsedQuantizedAabbCompression;
    }

#ifdef BULLET_WORLD_IMPORTER
    void Read(IBinaryStreamReadInterface& _Stream)
    {
        uint32_t bufferSize;
        _Stream >> bufferSize;
        byte* buffer = (byte*)Platform::MemoryAllocSafe(bufferSize);
        _Stream.Read(buffer, bufferSize);

        btBulletWorldImporter Importer(0);
        if (Importer.loadFileFromMemory((char*)buffer, bufferSize))
        {
            Data = (btBvhTriangleMeshShape*)Importer.getCollisionShapeByIndex(0);
        }

        GHeapMemory.HeapFree(buffer);
    }

    void Write(IBinaryStreamWriteInterface& _Stream) const
    {
        if (Data)
        {
            btDefaultSerializer Serializer;

            Serializer.startSerialization();

            Data->serializeSingleBvh(&Serializer);
            Data->serializeSingleTriangleInfoMap(&Serializer);

            Serializer.finishSerialization();

            _Stream << uint32_t(Serializer.getCurrentBufferSize());
            _Stream.Write(Serializer.getBufferPointer(), Serializer.getCurrentBufferSize());
        }
    }
#endif

private:
    TUniqueRef<btBvhTriangleMeshShape> Data; // TODO: Try btMultimaterialTriangleMeshShape
    TUniqueRef<btTriangleInfoMap>      TriangleInfoMap;

    bool bUsedQuantizedAabbCompression = false;
};

struct CollisionTriangleSoupGimpact : CollisionBody
{
    TVector<Float3>               Vertices;
    TVector<unsigned int>         Indices;
    TVector<CollisionMeshSubpart> Subparts;
    BvAxisAlignedBox                  BoundingBox;
    TUniqueRef<StridingMeshInterface> pInterface;

    btCollisionShape* Create(Float3 const& Scale) override
    {
        // FIXME: This shape don't work. Why?
        pInterface->Vertices     = Vertices.ToPtr();
        pInterface->Indices      = Indices.ToPtr();
        pInterface->Subparts     = Subparts.ToPtr();
        pInterface->SubpartCount = Subparts.Size();
        auto* shape = new btGImpactMeshShape(pInterface.GetObject());
        shape->setLocalScaling(btVectorToFloat3(Scale));
        return shape;
    }

    void GatherGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices, Float3x4 const& Transform) const override
    {
        if (Vertices.IsEmpty())
        {
            return;
        }

        const int firstIndex  = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize(firstVertex + Vertices.Size());

        int indexCount = 0;
        for (auto& subpart : Subparts)
        {
            indexCount += subpart.IndexCount;
        }

        _Indices.Resize(firstIndex + indexCount);

        unsigned int* pIndices = _Indices.ToPtr() + firstIndex;

        for (auto& subpart : Subparts)
        {
            for (int i = 0; i < subpart.IndexCount; i++)
            {
                *pIndices++ = firstVertex + subpart.BaseVertex + Indices[subpart.FirstIndex + i];
            }
        }

        Float3* pVertices = _Vertices.ToPtr() + firstVertex;

        for (int i = 0; i < Vertices.Size(); i++)
        {
            *pVertices++ = Transform * (Rotation * Vertices[i] + Position);
        }
    }
};

CollisionModel::CollisionModel()
{}

CollisionModel::CollisionModel(void const* pShapes)
{
    int numShapes = 0;

    while (pShapes)
    {
        COLLISION_SHAPE type = *(COLLISION_SHAPE const*)pShapes;
        switch (type)
        {
            case COLLISION_SHAPE_SPHERE:
                AddSphere((CollisionSphereDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_SPHERE_RADII:
                AddSphereRadii((CollisionSphereRadiiDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_BOX:
                AddBox((CollisionBoxDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_CYLINDER:
                AddCylinder((CollisionCylinderDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_CONE:
                AddCone((CollisionConeDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_CAPSULE:
                AddCapsule((CollisionCapsuleDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_CONVEX_HULL:
                AddConvexHull((CollisionConvexHullDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_TRIANGLE_SOUP_BVH:
                AddTriangleSoupBVH((CollisionTriangleSoupBVHDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_TRIANGLE_SOUP_GIMPACT:
                AddTriangleSoupGimpact((CollisionTriangleSoupGimpactDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_CONVEX_DECOMPOSITION:
                AddConvexDecomposition((CollisionConvexDecompositionDef const*)pShapes, numShapes);
                break;
            case COLLISION_SHAPE_CONVEX_DECOMPOSITION_VHACD:
                AddConvexDecompositionVHACD((CollisionConvexDecompositionVHACDDef const*)pShapes, numShapes);
                break;
            default:
                LOG("CollisionModel::Initialize: unknown shape type\n");
                pShapes = nullptr;
                continue;
        }
        pShapes = ((CollisionSphereDef const*)pShapes)->pNext;
    }

    if (numShapes)
    {
        m_CenterOfMass /= numShapes;
    }
}

CollisionModel::CollisionModel(CollisionModelCreateInfo const& CreateInfo) :
    CollisionModel(CreateInfo.pShapes)
{
    if (CreateInfo.bOverrideCenterOfMass)
    {
        m_CenterOfMass = CreateInfo.CenterOfMass;
    }
}

CollisionModel::~CollisionModel()
{}

bool CollisionModel::LoadResource(IBinaryStreamReadInterface& Stream)
{
    // TODO
    return false;
}

void CollisionModel::LoadInternalResource(StringView Path)
{
    // TODO
}

void CollisionModel::AddSphere(CollisionSphereDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionSphere>();

    body->Position = pShape->Position;
    body->Margin   = pShape->Margin;
    body->Radius   = pShape->Radius;

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddSphereRadii(CollisionSphereRadiiDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionSphereRadii>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;
    body->Radius   = pShape->Radius;

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddBox(CollisionBoxDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionBox>();

    body->Position    = pShape->Position;
    body->Rotation    = pShape->Rotation;
    body->Margin      = pShape->Margin;
    body->HalfExtents = pShape->HalfExtents;

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddCylinder(CollisionCylinderDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionCylinder>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;
    body->Radius   = pShape->Radius;
    body->Height   = pShape->Height;
    body->Axial    = pShape->Axial;

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddCone(CollisionConeDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionCone>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;
    body->Radius   = pShape->Radius;
    body->Height   = pShape->Height;
    body->Axial    = pShape->Axial;

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddCapsule(CollisionCapsuleDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionCapsule>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;
    body->Radius   = pShape->Radius;
    body->Height   = pShape->Height;
    body->Axial    = pShape->Axial;

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddConvexHull(CollisionConvexHullDef const* pShape, int& NumShapes)
{
    auto body = MakeUnique<CollisionConvexHull>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;

    if (pShape->pVertices && pShape->pIndices && pShape->VertexCount && pShape->IndexCount)
    {
        body->Vertices.ResizeInvalidate(pShape->VertexCount);
        body->Indices.ResizeInvalidate(pShape->IndexCount);

        Platform::Memcpy(body->Vertices.ToPtr(), pShape->pVertices, pShape->VertexCount * sizeof(Float3));
        Platform::Memcpy(body->Indices.ToPtr(), pShape->pIndices, pShape->IndexCount * sizeof(unsigned int));
    }
    else if (pShape->pPlanes && pShape->PlaneCount)
    {
        ConvexHull hull;
        ConvexHull frontHull;
        ConvexHull* phull = &hull;
        ConvexHull* pfrontHull = &frontHull;
        for (int i = 0; i < pShape->PlaneCount; i++)
        {
            phull->FromPlane(pShape->pPlanes[i]);

            for (int j = 0; j < pShape->PlaneCount && !phull->IsEmpty(); j++)
            {
                if (i != j)
                {
                    phull->Clip(-pShape->pPlanes[j], 0.001f, *pfrontHull);
                    Core::Swap(phull, pfrontHull);
                }
            }

            if (phull->NumPoints() < 3)
            {
                LOG("CollisionModel::AddConvexHull: hull is clipped off\n");
                continue;
            }

            int firstIndex = body->Indices.Size();
            for (int v = 0, numPoints = phull->NumPoints(); v < numPoints; v++)
            {
                int hasVert = body->Vertices.Size();
                for (int t = 0; t < body->Vertices.Size(); t++)
                {
                    Float3& vert = body->Vertices[t];
                    if ((vert - (*phull)[v]).LengthSqr() > FLT_EPSILON)
                    {
                        continue;
                    }
                    hasVert = t;
                    break;
                }
                if (hasVert == body->Vertices.Size())
                {
                    body->Vertices.Add((*phull)[v]);
                }
                if (v > 2)
                {
                    body->Indices.Add(body->Indices[firstIndex]);
                    body->Indices.Add(body->Indices[body->Indices.Size() - 2]);
                }
                body->Indices.Add(hasVert);
            }
        }
    }
    else
    {
        LOG("CollisionModel::AddConvexHull: undefined geometry\n");
        return;
    }

    if (pShape->Bone.JointIndex >= 0)
    {
        BoneCollision boneCol;
        boneCol.JointIndex     = pShape->Bone.JointIndex;
        boneCol.CollisionGroup = pShape->Bone.CollisionGroup;
        boneCol.CollisionMask  = pShape->Bone.CollisionMask;
        boneCol.CollisionBody  = std::move(body);
        m_BoneCollisions.Add(std::move(boneCol));
    }
    else
    {
        m_CenterOfMass += body->Position;
        NumShapes++;
        m_CollisionBodies.Add(std::move(body));
    }
}

void CollisionModel::AddTriangleSoupBVH(CollisionTriangleSoupBVHDef const* pShape, int& NumShapes)
{
    if (pShape->VertexStride <= 0)
    {
        LOG("CollisionModel::AddTriangleSoupBVH: invalid VertexStride\n");
        return;
    }

    auto body = MakeUnique<CollisionTriangleSoupBVH>();

    body->pInterface = MakeUnique<StridingMeshInterface>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;

    body->Vertices.ResizeInvalidate(pShape->VertexCount);
    body->Indices.ResizeInvalidate(pShape->IndexCount);
    body->Subparts.ResizeInvalidate(pShape->SubpartCount);

    if (pShape->VertexStride == sizeof(body->Vertices[0]))
    {
        Platform::Memcpy(body->Vertices.ToPtr(), pShape->pVertices, sizeof(body->Vertices[0]) * pShape->VertexCount);
    }
    else
    {
        byte const* ptr = (byte const*)pShape->pVertices;
        for (int i = 0; i < pShape->VertexCount; i++)
        {
            Platform::Memcpy(body->Vertices.ToPtr() + i, ptr, sizeof(body->Vertices[0]));
            ptr += pShape->VertexStride;
        }
    }

    Platform::Memcpy(body->Indices.ToPtr(), pShape->pIndices, sizeof(body->Indices[0]) * pShape->IndexCount);

    if (pShape->pSubparts)
    {
        body->BoundingBox.Clear();
        body->Subparts.Resize(pShape->SubpartCount);
        for (int i = 0; i < pShape->SubpartCount; i++)
        {
            CollisionMeshSubpart const& subpart = pShape->pSubparts[i];

            body->Subparts[i].BaseVertex  = subpart.BaseVertex;
            body->Subparts[i].VertexCount = subpart.VertexCount;
            body->Subparts[i].FirstIndex  = subpart.FirstIndex;
            body->Subparts[i].IndexCount  = subpart.IndexCount;

            Float3 const*       pVertices = pShape->pVertices + subpart.BaseVertex;
            unsigned int const* pIndices  = pShape->pIndices + subpart.FirstIndex;
            for (int n = 0; n < subpart.IndexCount; n += 3)
            {
                unsigned int i0 = pIndices[n];
                unsigned int i1 = pIndices[n + 1];
                unsigned int i2 = pIndices[n + 2];

                body->BoundingBox.AddPoint(pVertices[i0]);
                body->BoundingBox.AddPoint(pVertices[i1]);
                body->BoundingBox.AddPoint(pVertices[i2]);
            }
        }
    }
    else if (pShape->pIndexedMeshSubparts)
    {
        body->BoundingBox.Clear();
        body->Subparts.Resize(pShape->SubpartCount);
        for (int i = 0; i < pShape->SubpartCount; i++)
        {
            body->Subparts[i].BaseVertex  = pShape->pIndexedMeshSubparts[i]->GetBaseVertex();
            body->Subparts[i].VertexCount = pShape->pIndexedMeshSubparts[i]->GetVertexCount();
            body->Subparts[i].FirstIndex  = pShape->pIndexedMeshSubparts[i]->GetFirstIndex();
            body->Subparts[i].IndexCount  = pShape->pIndexedMeshSubparts[i]->GetIndexCount();
            body->BoundingBox.AddAABB(pShape->pIndexedMeshSubparts[i]->GetBoundingBox());
        }
    }
    else
    {
        body->BoundingBox.Clear();

        body->Subparts.Resize(1);
        body->Subparts[0].BaseVertex  = 0;
        body->Subparts[0].VertexCount = pShape->VertexCount;
        body->Subparts[0].FirstIndex  = 0;
        body->Subparts[0].IndexCount  = pShape->IndexCount;

        Float3 const*       pVertices = pShape->pVertices;
        unsigned int const* pIndices  = pShape->pIndices;
        for (int n = 0; n < pShape->IndexCount; n += 3)
        {
            unsigned int i0 = pIndices[n];
            unsigned int i1 = pIndices[n + 1];
            unsigned int i2 = pIndices[n + 2];

            body->BoundingBox.AddPoint(pVertices[i0]);
            body->BoundingBox.AddPoint(pVertices[i1]);
            body->BoundingBox.AddPoint(pVertices[i2]);
        }
    }

    body->BuildBVH(pShape->bForceQuantizedAabbCompression);

    m_CenterOfMass += body->Position;
    NumShapes++;

    m_CollisionBodies.Add(std::move(body));
}

void CollisionModel::AddTriangleSoupGimpact(CollisionTriangleSoupGimpactDef const* pShape, int& NumShapes)
{
    if (pShape->VertexStride <= 0)
    {
        LOG("CollisionModel::AddTriangleSoupGimpact: invalid VertexStride\n");
        return;
    }

    auto body = MakeUnique<CollisionTriangleSoupGimpact>();

    body->pInterface = MakeUnique<StridingMeshInterface>();

    body->Position = pShape->Position;
    body->Rotation = pShape->Rotation;
    body->Margin   = pShape->Margin;

    body->Vertices.ResizeInvalidate(pShape->VertexCount);
    body->Indices.ResizeInvalidate(pShape->IndexCount);
    body->Subparts.ResizeInvalidate(pShape->SubpartCount);

    if (pShape->VertexStride == sizeof(body->Vertices[0]))
    {
        Platform::Memcpy(body->Vertices.ToPtr(), pShape->pVertices, sizeof(body->Vertices[0]) * pShape->VertexCount);
    }
    else
    {
        byte const* ptr = (byte const*)pShape->pVertices;
        for (int i = 0; i < pShape->VertexCount; i++)
        {
            Platform::Memcpy(body->Vertices.ToPtr() + i, ptr, sizeof(body->Vertices[0]));
            ptr += pShape->VertexStride;
        }
    }

    Platform::Memcpy(body->Indices.ToPtr(), pShape->pIndices, sizeof(body->Indices[0]) * pShape->IndexCount);

    if (pShape->pSubparts)
    {
        body->BoundingBox.Clear();
        body->Subparts.Resize(pShape->SubpartCount);
        for (int i = 0; i < pShape->SubpartCount; i++)
        {
            CollisionMeshSubpart const& subpart = pShape->pSubparts[i];

            body->Subparts[i].BaseVertex  = subpart.BaseVertex;
            body->Subparts[i].VertexCount = subpart.VertexCount;
            body->Subparts[i].FirstIndex  = subpart.FirstIndex;
            body->Subparts[i].IndexCount  = subpart.IndexCount;

            Float3 const*       pVertices = pShape->pVertices + subpart.BaseVertex;
            unsigned int const* pIndices  = pShape->pIndices + subpart.FirstIndex;
            for (int n = 0; n < subpart.IndexCount; n += 3)
            {
                unsigned int i0 = pIndices[n];
                unsigned int i1 = pIndices[n + 1];
                unsigned int i2 = pIndices[n + 2];

                body->BoundingBox.AddPoint(pVertices[i0]);
                body->BoundingBox.AddPoint(pVertices[i1]);
                body->BoundingBox.AddPoint(pVertices[i2]);
            }
        }
    }
    else if (pShape->pIndexedMeshSubparts)
    {
        body->BoundingBox.Clear();
        body->Subparts.Resize(pShape->SubpartCount);
        for (int i = 0; i < pShape->SubpartCount; i++)
        {
            body->Subparts[i].BaseVertex  = pShape->pIndexedMeshSubparts[i]->GetBaseVertex();
            body->Subparts[i].VertexCount = pShape->pIndexedMeshSubparts[i]->GetVertexCount();
            body->Subparts[i].FirstIndex  = pShape->pIndexedMeshSubparts[i]->GetFirstIndex();
            body->Subparts[i].IndexCount  = pShape->pIndexedMeshSubparts[i]->GetIndexCount();
            body->BoundingBox.AddAABB(pShape->pIndexedMeshSubparts[i]->GetBoundingBox());
        }
    }
    else
    {
        body->BoundingBox.Clear();

        body->Subparts.Resize(1);
        body->Subparts[0].BaseVertex  = 0;
        body->Subparts[0].VertexCount = pShape->VertexCount;
        body->Subparts[0].FirstIndex  = 0;
        body->Subparts[0].IndexCount  = pShape->IndexCount;

        Float3 const*       pVertices = pShape->pVertices;
        unsigned int const* pIndices  = pShape->pIndices;
        for (int n = 0; n < pShape->IndexCount; n += 3)
        {
            unsigned int i0 = pIndices[n];
            unsigned int i1 = pIndices[n + 1];
            unsigned int i2 = pIndices[n + 2];

            body->BoundingBox.AddPoint(pVertices[i0]);
            body->BoundingBox.AddPoint(pVertices[i1]);
            body->BoundingBox.AddPoint(pVertices[i2]);
        }
    }

    m_CenterOfMass += body->Position;
    NumShapes++;

    m_CollisionBodies.Add(std::move(body));
}

void CollisionModel::AddConvexDecomposition(CollisionConvexDecompositionDef const* pShape, int& NumShapes)
{
    TPodVector<Float3>          hullVertices;
    TPodVector<unsigned int>    hullIndices;
    TPodVector<ConvexHullDesc> hulls;

    if (pShape->VertexStride <= 0)
    {
        LOG("CollisionModel::AddConvexDecomposition: invalid VertexStride\n");
        return;
    }

    Geometry::PerformConvexDecomposition(pShape->pVertices,
                                         pShape->VerticesCount,
                                         pShape->VertexStride,
                                         pShape->pIndices,
                                         pShape->IndicesCount,
                                         hullVertices,
                                         hullIndices,
                                         hulls);

    if (hulls.IsEmpty())
    {
        LOG("CollisionModel::AddConvexDecomposition: failed on convex decomposition\n");
        return;
    }

    Float3 saveCenterOfMass = m_CenterOfMass;

    m_CenterOfMass.Clear();

    int n{};
    for (ConvexHullDesc const& hull : hulls)
    {
        CollisionConvexHullDef hulldef;

        hulldef.Position    = hull.Centroid;
        hulldef.Margin      = 0.01f;
        hulldef.pVertices   = hullVertices.ToPtr() + hull.FirstVertex;
        hulldef.VertexCount = hull.VertexCount;
        hulldef.pIndices    = hullIndices.ToPtr() + hull.FirstIndex;
        hulldef.IndexCount  = hull.IndexCount;

        AddConvexHull(&hulldef, n);
    }

    m_CenterOfMass /= n;
    m_CenterOfMass += saveCenterOfMass;
    NumShapes++;
}

void CollisionModel::AddConvexDecompositionVHACD(CollisionConvexDecompositionVHACDDef const* pShape, int& NumShapes)
{
    TPodVector<Float3>          hullVertices;
    TPodVector<unsigned int>    hullIndices;
    TPodVector<ConvexHullDesc> hulls;
    Float3                      decompositionCenterOfMass;

    if (pShape->VertexStride <= 0)
    {
        LOG("CollisionModel::AddConvexDecompositionVHACD: invalid VertexStride\n");
        return;
    }

    Geometry::PerformConvexDecompositionVHACD(pShape->pVertices,
                                              pShape->VerticesCount,
                                              pShape->VertexStride,
                                              pShape->pIndices,
                                              pShape->IndicesCount,
                                              hullVertices,
                                              hullIndices,
                                              hulls,
                                              decompositionCenterOfMass);

    if (hulls.IsEmpty())
    {
        return;
    }

    m_CenterOfMass += decompositionCenterOfMass;
    NumShapes++;

    // Save current center of mass
    Float3 saveCenterOfMass = m_CenterOfMass;

    int n{};
    for (ConvexHullDesc const& hull : hulls)
    {
        CollisionConvexHullDef hulldef;

        hulldef.Position    = hull.Centroid;
        hulldef.Margin      = 0.01f;
        hulldef.pVertices   = hullVertices.ToPtr() + hull.FirstVertex;
        hulldef.VertexCount = hull.VertexCount;
        hulldef.pIndices    = hullIndices.ToPtr() + hull.FirstIndex;
        hulldef.IndexCount  = hull.IndexCount;

        AddConvexHull(&hulldef, n);
    }

    // Restore center of mass to ignore computations in AddConvexHull
    m_CenterOfMass = saveCenterOfMass;
}

void CollisionModel::GatherGeometry(TVector<Float3>& Vertices, TVector<unsigned int>& Indices, Float3x4 const& Transform) const
{
    for (TUniqueRef<CollisionBody> const& collisionBody : m_CollisionBodies)
    {
        collisionBody->GatherGeometry(Vertices, Indices, Transform);
    }
}

TRef<CollisionInstance> CollisionModel::Instantiate(Float3 const& Scale)
{
    return MakeRef<CollisionInstance>(this, Scale);
}

CollisionInstance::CollisionInstance(CollisionModel* CollisionModel, Float3 const& Scale)
{
    constexpr float POSITION_COMPARE_EPSILON{0.0001f};

    m_Model         = CollisionModel;
    m_CompoundShape = MakeUnique<btCompoundShape>();
    m_CenterOfMass  = Scale * CollisionModel->GetCenterOfMass();

    if (!CollisionModel->m_CollisionBodies.IsEmpty())
    {
        btTransform     shapeTransform;

        for (TUniqueRef<CollisionBody> const& collisionBody : CollisionModel->m_CollisionBodies)
        {
            btCollisionShape* shape = collisionBody->Create(Scale);

            shape->setMargin(collisionBody->Margin);

            shapeTransform.setOrigin(btVectorToFloat3(Scale * collisionBody->Position - m_CenterOfMass));
            shapeTransform.setRotation(btQuaternionToQuat(collisionBody->Rotation));

            m_CompoundShape->addChildShape(shapeTransform, shape);
        }
    }

    int  numShapes    = m_CompoundShape->getNumChildShapes();
    bool bUseCompound = !numShapes || numShapes > 1;
    if (!bUseCompound)
    {
        btTransform const& childTransform = m_CompoundShape->getChildTransform(0);

        if (!btVectorToFloat3(childTransform.getOrigin()).CompareEps(Float3::Zero(), POSITION_COMPARE_EPSILON) || btQuaternionToQuat(childTransform.getRotation()) != Quat::Identity())
        {
            bUseCompound = true;
        }
    }

    m_CollisionShape = bUseCompound ? m_CompoundShape.GetObject() : m_CompoundShape->getChildShape(0);
}

CollisionInstance::~CollisionInstance()
{
    int numShapes = m_CompoundShape->getNumChildShapes();
    for (int i = numShapes - 1; i >= 0; i--)
    {
        btCollisionShape* shape = m_CompoundShape->getChildShape(i);
        delete shape;
    }
}

Float3 CollisionInstance::CalculateLocalInertia(float Mass) const
{
    btVector3 localInertia;
    m_CollisionShape->calculateLocalInertia(Mass, localInertia);
    return btVectorToFloat3(localInertia);
}

void CollisionInstance::GetCollisionBodiesWorldBounds(Float3 const& WorldPosition, Quat const& WorldRotation, TPodVector<BvAxisAlignedBox>& BoundingBoxes) const
{
    btVector3 mins, maxs;

    btTransform transform;
    transform.setOrigin(btVectorToFloat3(WorldPosition));
    transform.setRotation(btQuaternionToQuat(WorldRotation));

    int numShapes = m_CompoundShape->getNumChildShapes();
    BoundingBoxes.ResizeInvalidate(numShapes);

    for (int i = 0; i < numShapes; i++)
    {
        btCompoundShapeChild& shape = m_CompoundShape->getChildList()[i];

        shape.m_childShape->getAabb(transform * shape.m_transform, mins, maxs);

        BoundingBoxes[i].Mins = btVectorToFloat3(mins);
        BoundingBoxes[i].Maxs = btVectorToFloat3(maxs);
    }
}

void CollisionInstance::GetCollisionWorldBounds(Float3 const& WorldPosition, Quat const& WorldRotation, BvAxisAlignedBox& BoundingBox) const
{
    btVector3 mins, maxs;

    btTransform transform;
    transform.setOrigin(btVectorToFloat3(WorldPosition));
    transform.setRotation(btQuaternionToQuat(WorldRotation));

    BoundingBox.Clear();

    int numShapes = m_CompoundShape->getNumChildShapes();

    for (int i = 0; i < numShapes; i++)
    {
        btCompoundShapeChild& shape = m_CompoundShape->getChildList()[i];

        shape.m_childShape->getAabb(transform * shape.m_transform, mins, maxs);

        BoundingBox.AddAABB(btVectorToFloat3(mins), btVectorToFloat3(maxs));
    }
}

void CollisionInstance::GetCollisionBodyWorldBounds(int Index, Float3 const& WorldPosition, Quat const& WorldRotation, BvAxisAlignedBox& BoundingBox) const
{
    if (Index < 0 || Index >= m_CompoundShape->getNumChildShapes())
    {
        LOG("CollisionInstance::GetCollisionBodyWorldBounds: invalid index\n");

        BoundingBox.Clear();
        return;
    }

    btVector3 mins, maxs;

    btTransform transform;
    transform.setOrigin(btVectorToFloat3(WorldPosition));
    transform.setRotation(btQuaternionToQuat(WorldRotation));

    btCompoundShapeChild& shape = m_CompoundShape->getChildList()[Index];

    shape.m_childShape->getAabb(transform * shape.m_transform, mins, maxs);

    BoundingBox.Mins = btVectorToFloat3(mins);
    BoundingBox.Maxs = btVectorToFloat3(maxs);
}

void CollisionInstance::GetCollisionBodyLocalBounds(int Index, BvAxisAlignedBox& BoundingBox) const
{
    if (Index < 0 || Index >= m_CompoundShape->getNumChildShapes())
    {
        LOG("CollisionInstance::GetCollisionBodyLocalBounds: invalid index\n");

        BoundingBox.Clear();
        return;
    }

    btVector3 mins, maxs;

    btCompoundShapeChild& shape = m_CompoundShape->getChildList()[Index];

    shape.m_childShape->getAabb(shape.m_transform, mins, maxs);

    BoundingBox.Mins = btVectorToFloat3(mins);
    BoundingBox.Maxs = btVectorToFloat3(maxs);
}

float CollisionInstance::GetCollisionBodyMargin(int Index) const
{
    if (Index < 0 || Index >= m_CompoundShape->getNumChildShapes())
    {
        LOG("CollisionInstance::GetCollisionBodyMargin: invalid index\n");

        return 0;
    }

    btCompoundShapeChild& shape = m_CompoundShape->getChildList()[Index];

    return shape.m_childShape->getMargin();
}

int CollisionInstance::GetCollisionBodiesCount() const
{
    return m_CompoundShape->getNumChildShapes();
}
