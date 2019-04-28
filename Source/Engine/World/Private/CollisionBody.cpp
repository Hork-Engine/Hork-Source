/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Engine/World/Public/CollisionBody.h>
#include <Engine/World/Public/IndexedMesh.h>
#include <Engine/World/Public/MeshAsset.h>

#include "BulletCompatibility/BulletCompatibility.h"
#include <Bullet3Common/b3AlignedAllocator.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>

AN_CLASS_META_NO_ATTRIBS( FCollisionBody )
AN_CLASS_META_NO_ATTRIBS( FCollisionSphere )
AN_CLASS_META_NO_ATTRIBS( FCollisionBox )
AN_CLASS_META_NO_ATTRIBS( FCollisionCylinder )
AN_CLASS_META_NO_ATTRIBS( FCollisionCone )
AN_CLASS_META_NO_ATTRIBS( FCollisionCapsule )
AN_CLASS_META_NO_ATTRIBS( FCollisionPlane )
AN_CLASS_META_NO_ATTRIBS( FCollisionConvexHull )
AN_CLASS_META_NO_ATTRIBS( FCollisionSharedConvexHull )
AN_CLASS_META_NO_ATTRIBS( FCollisionSharedTriangleSoup )
AN_CLASS_META_NO_ATTRIBS( FCollisionConvexHullData )
AN_CLASS_META_NO_ATTRIBS( FCollisionTriangleSoupData )

btCollisionShape * FCollisionSphere::Create() {
    return b3New( btSphereShape, Radius );
}

btCollisionShape * FCollisionBox::Create() {
    return b3New( btBoxShape, btVectorToFloat3( HalfExtents ) );
}

btCollisionShape * FCollisionCylinder::Create() {
    return b3New( btCylinderShape, btVectorToFloat3( HalfExtents ) );
}

btCollisionShape * FCollisionCone::Create() {
    return b3New( btConeShape, Radius, Height );
}

btCollisionShape * FCollisionCapsule::Create() {
    return b3New( btCapsuleShape, Radius, Height );
}

btCollisionShape * FCollisionPlane::Create() {
    return b3New( btStaticPlaneShape, btVectorToFloat3( Plane.Normal ), Plane.D );
}

btCollisionShape * FCollisionConvexHull::Create() {
    return b3New( btConvexHullShape, ( btScalar const * )Vertices.ToPtr(), Vertices.Length(), sizeof( Float3 ) );
}

btCollisionShape * FCollisionSharedConvexHull::Create() {
    constexpr bool bComputeAabb = false; // FIXME: Нужно ли сейчас считать aabb?
    return b3New( btConvexPointCloudShape, ( btVector3 * )HullData->Vertices.ToPtr(), HullData->Vertices.Length(), btVector3(1.f,1.f,1.f), bComputeAabb );
}

btCollisionShape * FCollisionSharedTriangleSoup::Create() {
    return b3New( btScaledBvhTriangleMeshShape, TrisData->GetData(), btVector3(1.f,1.f,1.f) );
}

#include <BulletCollision/CollisionShapes/btStridingMeshInterface.h>
//#include <BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h>

ATTRIBUTE_ALIGNED16( class ) FStridingMeshInterface : public btStridingMeshInterface
{
protected:
    mutable int bHasAABB; // using int instead of bool to maintain alignment
    mutable btVector3 AABBMin;
    mutable btVector3 AABBMax;
    // FIXME: padding?

public:
    Float3 * Vertices;
    unsigned int * Indices;
    FCollisionTriangleSoupData::FSubpart * Subparts;
    int SubpartCount;

public:
    //BT_DECLARE_ALIGNED_ALLOCATOR();

    FStridingMeshInterface()
        : bHasAABB(0)
    {
    }

    ~FStridingMeshInterface() {

    }

    void getLockedVertexIndexBase( unsigned char **VertexBase,
                                   int & VertexCount,
                                   PHY_ScalarType & Type,
                                   int & VertexStride,
                                   unsigned char **IndexBase,
                                   int & IndexStride,
                                   int & FaceCount,
                                   PHY_ScalarType & IndexType,
                                   int Subpart = 0 ) override {

        AN_Assert( Subpart < SubpartCount );

        FCollisionTriangleSoupData::FSubpart * subpart = Subparts + Subpart;

        (*VertexBase) = ( unsigned char * )( Vertices + subpart->BaseVertex );
        VertexCount = subpart->VertexCount;
        Type = PHY_FLOAT;
        VertexStride = sizeof( Vertices[0] );

        (*IndexBase) = ( unsigned char * )( Indices + subpart->FirstIndex );
        IndexStride = sizeof( Indices[0] ) * 3;
        FaceCount = subpart->IndexCount / 3;
        IndexType = PHY_INTEGER;
    }

    void getLockedReadOnlyVertexIndexBase( const unsigned char **VertexBase,
                                           int & VertexCount,
                                           PHY_ScalarType & Type,
                                           int & VertexStride,
                                           const unsigned char **IndexBase,
                                           int & IndexStride,
                                           int & FaceCount,
                                           PHY_ScalarType & IndexType,
                                           int Subpart = 0 ) const override {
        AN_Assert( Subpart < SubpartCount );

        FCollisionTriangleSoupData::FSubpart * subpart = Subparts + Subpart;

        (*VertexBase) = ( const unsigned char * )( Vertices + subpart->BaseVertex );
        VertexCount = subpart->VertexCount;
        Type = PHY_FLOAT;
        VertexStride = sizeof( Vertices[0] );

        (*IndexBase) = ( const unsigned char * )( Indices + subpart->FirstIndex );
        IndexStride = sizeof( Indices[0] ) * 3;
        FaceCount = subpart->IndexCount / 3;
        IndexType = PHY_INTEGER;
    }

    // unLockVertexBase finishes the access to a subpart of the triangle mesh
    // make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
    void unLockVertexBase(int subpart) override {(void)subpart;}

    void unLockReadOnlyVertexBase(int subpart) const override { (void)subpart; }

    // getNumSubParts returns the number of seperate subparts
    // each subpart has a continuous array of vertices and indices
    virtual int getNumSubParts() const {
        return SubpartCount;
    }


    void preallocateVertices( int numverts ) override { (void) numverts; }
    void preallocateIndices( int numindices ) override { (void) numindices; }

    bool hasPremadeAabb() const override { return (bHasAABB == 1); }

    void setPremadeAabb(const btVector3& aabbMin, const btVector3& aabbMax ) const override {
        AABBMin = aabbMin;
        AABBMax = aabbMax;
        bHasAABB = 1;
    }

    void getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax ) const override {
        *aabbMin = AABBMin;
        *aabbMax = AABBMax;
    }
};

FCollisionTriangleSoupData::FCollisionTriangleSoupData() {
    Interface = b3New( FStridingMeshInterface );
}

FCollisionTriangleSoupData::~FCollisionTriangleSoupData() {
    b3Destroy( Interface );

    if ( Data ) {
        b3Destroy( Data );
    }
}

bool FCollisionTriangleSoupData::UsedQuantizedAabbCompression() const {
    return bUsedQuantizedAabbCompression;
}

void FCollisionTriangleSoupData::Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, ::FSubpart const * _Subparts, int _SubpartsCount ) {

    Vertices.ResizeInvalidate( _VertexCount );
    Indices.ResizeInvalidate( _IndexCount );
    Subparts.ResizeInvalidate( _SubpartsCount );

    Interface->Vertices = Vertices.ToPtr();
    Interface->Indices = Indices.ToPtr();
    Interface->Subparts = Subparts.ToPtr();
    Interface->SubpartCount = _SubpartsCount;

    if ( _VertexStride == sizeof( Vertices[0] ) ) {
        memcpy( Vertices.ToPtr(), _Vertices, sizeof( Vertices[0] ) * _VertexCount );
    } else {
        byte const * ptr = (byte const *)_Vertices;
        for ( int i = 0 ; i < _VertexCount ; i++ ) {
            memcpy( Vertices.ToPtr() + i, ptr, sizeof( Vertices[0] ) );
            ptr += _VertexStride;
        }
    }

    memcpy( Indices.ToPtr(), _Indices, sizeof( Indices[0] ) * _IndexCount );

    BvAxisAlignedBox bounds;
    bounds.Clear();
    for ( int i = 0 ; i < _SubpartsCount ; i++ ) {
        Subparts[i].BaseVertex  = _Subparts[i].BaseVertex;
        Subparts[i].VertexCount = _Subparts[i].VertexCount;
        Subparts[i].FirstIndex  = _Subparts[i].FirstIndex;
        Subparts[i].IndexCount  = _Subparts[i].IndexCount;
        bounds.AddAABB( _Subparts[i].BoundingBox );
    }

    if ( Data ) {
        b3Destroy( Data );
    }

    // TODO: calc from mesh bounds!
    const btVector3 bvhAabbMin = btVectorToFloat3( bounds.Mins );// btVector3(-999999999.0f,-999999999.0f,-999999999.0f);
    const btVector3 bvhAabbMax = btVectorToFloat3( bounds.Maxs );//btVector3(999999999.0f,999999999.0f,999999999.0f);

    constexpr unsigned int QUANTIZED_AABB_COMPRESSION_MAX_TRIANGLES = 1000000;

    // При слишком большом количестве треугольников Bullet не будет работать правильно с Quantized Aabb Compression
    bUsedQuantizedAabbCompression = _IndexCount / 3 <= QUANTIZED_AABB_COMPRESSION_MAX_TRIANGLES;

    Data = b3New( btBvhTriangleMeshShape, Interface,
                                          UsedQuantizedAabbCompression(),
                                          bvhAabbMin,
                                          bvhAabbMax,
                                          true );
}
