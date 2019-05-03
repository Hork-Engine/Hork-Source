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

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning( disable : 4456 )
#endif
#include <BulletCollision/Gimpact/btGImpactShape.h>
#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif

#include <BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h>

AN_CLASS_META_NO_ATTRIBS( FCollisionBody )
AN_CLASS_META_NO_ATTRIBS( FCollisionSphere )
AN_CLASS_META_NO_ATTRIBS( FCollisionSphereRadii )
AN_CLASS_META_NO_ATTRIBS( FCollisionBox )
AN_CLASS_META_NO_ATTRIBS( FCollisionCylinder )
AN_CLASS_META_NO_ATTRIBS( FCollisionCone )
AN_CLASS_META_NO_ATTRIBS( FCollisionCapsule )
AN_CLASS_META_NO_ATTRIBS( FCollisionPlane )
AN_CLASS_META_NO_ATTRIBS( FCollisionConvexHull )
AN_CLASS_META_NO_ATTRIBS( FCollisionSharedConvexHull )
AN_CLASS_META_NO_ATTRIBS( FCollisionSharedTriangleSoupBVH )
AN_CLASS_META_NO_ATTRIBS( FCollisionSharedTriangleSoupGimpact )
AN_CLASS_META_NO_ATTRIBS( FCollisionConvexHullData )
AN_CLASS_META_NO_ATTRIBS( FCollisionTriangleSoupData )
AN_CLASS_META_NO_ATTRIBS( FCollisionTriangleSoupBVHData )

btCollisionShape * FCollisionSphere::Create() {
    if ( bProportionalScale ) {
        return b3New( btSphereShape, Radius );
    } else {
        btVector3 pos(0,0,0);
        return b3New( btMultiSphereShape, &pos, &Radius, 1 );
    }
}

btCollisionShape * FCollisionSphereRadii::Create() {
    btVector3 pos(0,0,0);
    float radius = 1.0f;
    btMultiSphereShape * shape = b3New( btMultiSphereShape, &pos, &radius, 1 );
    shape->setLocalScaling( btVectorToFloat3( Radius ) );
    return shape;
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

btCollisionShape * FCollisionSharedTriangleSoupBVH::Create() {
    return b3New( btScaledBvhTriangleMeshShape, BvhData->GetData(), btVector3(1.f,1.f,1.f) );
}

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

FCollisionTriangleSoupBVHData::FCollisionTriangleSoupBVHData() {
    Interface = b3New( FStridingMeshInterface );
}

FCollisionTriangleSoupBVHData::~FCollisionTriangleSoupBVHData() {
    b3Destroy( Interface );

    if ( Data ) {
        b3Destroy( Data );
    }
}

bool FCollisionTriangleSoupBVHData::UsedQuantizedAabbCompression() const {
    return bUsedQuantizedAabbCompression;
}

void FCollisionTriangleSoupBVHData::BuildBVH( bool bForceQuantizedAabbCompression ) {
    Interface->Vertices = TrisData->Vertices.ToPtr();
    Interface->Indices = TrisData->Indices.ToPtr();
    Interface->Subparts = TrisData->Subparts.ToPtr();
    Interface->SubpartCount = TrisData->Subparts.Length();

    if ( !bForceQuantizedAabbCompression ) {
        constexpr unsigned int QUANTIZED_AABB_COMPRESSION_MAX_TRIANGLES = 1000000;

        int indexCount = 0;
        for ( int i = 0 ; i < Interface->SubpartCount ; i++ ) {
            indexCount += Interface->Subparts[i].IndexCount;
        }

        // При слишком большом количестве треугольников Bullet не будет работать правильно с Quantized Aabb Compression
        bUsedQuantizedAabbCompression = indexCount / 3 <= QUANTIZED_AABB_COMPRESSION_MAX_TRIANGLES;
    } else {
        bUsedQuantizedAabbCompression = true;
    }

    if ( Data ) {
        b3Destroy( Data );
    }

    Data = b3New( btBvhTriangleMeshShape, Interface,
                                          UsedQuantizedAabbCompression(),
                                          btVectorToFloat3( TrisData->BoundingBox.Mins ),
                                          btVectorToFloat3( TrisData->BoundingBox.Maxs ),
                                          true );
}

FCollisionSharedTriangleSoupGimpact::FCollisionSharedTriangleSoupGimpact() {
    Interface = b3New( FStridingMeshInterface );
}

FCollisionSharedTriangleSoupGimpact::~FCollisionSharedTriangleSoupGimpact() {
    b3Destroy( Interface );
}

btCollisionShape * FCollisionSharedTriangleSoupGimpact::Create() {
    // FIXME: This shape don't work. Why?
    Interface->Vertices = TrisData->Vertices.ToPtr();
    Interface->Indices = TrisData->Indices.ToPtr();
    Interface->Subparts = TrisData->Subparts.ToPtr();
    Interface->SubpartCount = TrisData->Subparts.Length();
    return b3New( btGImpactMeshShape, Interface );
}

void FCollisionTriangleSoupData::Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, ::FSubpart const * _Subparts, int _SubpartsCount ) {
    Vertices.ResizeInvalidate( _VertexCount );
    Indices.ResizeInvalidate( _IndexCount );
    Subparts.ResizeInvalidate( _SubpartsCount );

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

    BoundingBox.Clear();
    for ( int i = 0 ; i < _SubpartsCount ; i++ ) {
        Subparts[i].BaseVertex  = _Subparts[i].BaseVertex;
        Subparts[i].VertexCount = _Subparts[i].VertexCount;
        Subparts[i].FirstIndex  = _Subparts[i].FirstIndex;
        Subparts[i].IndexCount  = _Subparts[i].IndexCount;
        BoundingBox.AddAABB( _Subparts[i].BoundingBox );
    }
}

AN_FORCEINLINE bool IsPointInsideConvexHull( Float3 const & _Point, PlaneF const * _Planes, int _NumPlanes, float _Margin ) {
    for ( int i = 0 ; i < _NumPlanes ; i++ ) {
        if ( _Planes[ i ].Normal.Dot( _Point ) + _Planes[ i ].D - _Margin > 0 ) {
            return false;
        }
    }
    return true;
}

void ConvexHullVerticesFromPlanes( PlaneF const * _Planes, int _NumPlanes, TPodArray< Float3 > & _Vertices ) {
    constexpr float tolerance = 0.0001f;
    constexpr float quotientTolerance = 0.000001f;

    for ( int i = 0 ; i < _NumPlanes ; i++ ) {
        Float3 const & normal1 = _Planes[ i ].Normal;

        for ( int j = i + 1 ; j < _NumPlanes ; j++ ) {
            Float3 const & normal2 = _Planes[ j ].Normal;

            Float3 n1n2 = normal1.Cross( normal2 );

            if ( n1n2.LengthSqr() > tolerance ) {
                for ( int k = j + 1 ; k < _NumPlanes ; k++ ) {
                    Float3 const & normal3 = _Planes[ k ].Normal;

                    Float3 n2n3 = normal2.Cross( normal3 );
                    Float3 n3n1 = normal3.Cross( normal1 );

                    if ( ( n2n3.LengthSqr() > tolerance ) && ( n3n1.LengthSqr() > tolerance ) ) {
                        btScalar quotient = normal1.Dot( n2n3 );
                        if ( fabs( quotient ) > quotientTolerance ) {
                            quotient = -1 / quotient;

                            Float3 potentialVertex = n2n3 * _Planes[ i ].D + n3n1 * _Planes[ j ].D + n1n2 * _Planes[ k ].D;
                            potentialVertex *= quotient;

                            if ( IsPointInsideConvexHull( potentialVertex, _Planes, _NumPlanes, 0.01f ) ) {
                                _Vertices.Append( potentialVertex );
                            }
                        }
                    }
                }
            }
        }
    }
}
