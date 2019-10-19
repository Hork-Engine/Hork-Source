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

#include <Engine/Resource/Public/CollisionBody.h>
#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/Resource/Public/Asset.h>

#include <Engine/Core/Public/Logger.h>

#include <Engine/BulletCompatibility/BulletCompatibility.h>

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

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning( disable : 4456 )
#endif
#include <BulletCollision/Gimpact/btGImpactShape.h>
#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif

#include <BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h>

AN_CLASS_META( FCollisionBody )
AN_CLASS_META( FCollisionSphere )
AN_CLASS_META( FCollisionSphereRadii )
AN_CLASS_META( FCollisionBox )
AN_CLASS_META( FCollisionCylinder )
AN_CLASS_META( FCollisionCone )
AN_CLASS_META( FCollisionCapsule )
AN_CLASS_META( FCollisionConvexHull )
AN_CLASS_META( FCollisionTriangleSoupBVH )
AN_CLASS_META( FCollisionTriangleSoupGimpact )
AN_CLASS_META( FCollisionConvexHullData )
AN_CLASS_META( FCollisionTriangleSoupData )
AN_CLASS_META( FCollisionTriangleSoupBVHData )

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
    switch ( Axial ) {
    case AXIAL_X:
        return b3New( btCylinderShapeX, btVectorToFloat3( HalfExtents ) );
    case AXIAL_Y:
        return b3New( btCylinderShape, btVectorToFloat3( HalfExtents ) );
    case AXIAL_Z:
        return b3New( btCylinderShapeZ, btVectorToFloat3( HalfExtents ) );
    }
    return b3New( btCylinderShape, btVectorToFloat3( HalfExtents ) );
}

btCollisionShape * FCollisionCone::Create() {
    switch ( Axial ) {
    case AXIAL_X:
        return b3New( btConeShapeX, Radius, Height );
    case AXIAL_Y:
        return b3New( btConeShape, Radius, Height );
    case AXIAL_Z:
        return b3New( btConeShapeZ, Radius, Height );
    }
    return b3New( btConeShape, Radius, Height );
}

btCollisionShape * FCollisionCapsule::Create() {
    switch ( Axial ) {
    case AXIAL_X:
        return b3New( btCapsuleShapeX, Radius, Height );
    case AXIAL_Y:
        return b3New( btCapsuleShape, Radius, Height );
    case AXIAL_Z:
        return b3New( btCapsuleShapeZ, Radius, Height );
    }
    return b3New( btCapsuleShape, Radius, Height );
}

FCollisionConvexHullData::FCollisionConvexHullData() {
}

FCollisionConvexHullData::~FCollisionConvexHullData() {
    GZoneMemory.Dealloc( Data );
}

void FCollisionConvexHullData::Initialize( Float3 const * _Vertices, int _VertexCount, unsigned int * _Indices, int _IndexCount ) {
    Vertices.Resize( _VertexCount );
    Indices.Resize( _IndexCount );

    memcpy( Vertices.ToPtr(), _Vertices, _VertexCount * sizeof( Float3 ) );
    memcpy( Indices.ToPtr(), _Indices, _IndexCount * sizeof( unsigned int ) );

    GZoneMemory.Dealloc( Data );

    Data = ( btVector3 * )GZoneMemory.Alloc( sizeof( btVector3 ) * _VertexCount, 1 );
    for ( int i = 0 ; i < _VertexCount ; i++ ) {
        Data[i] = btVectorToFloat3( _Vertices[i] );
    }
}

//btCollisionShape * FCollisionConvexHull::Create() {
//    return b3New( btConvexHullShape, ( btScalar const * )Vertices.ToPtr(), Vertices.Length(), sizeof( Float3 ) );
//}

btCollisionShape * FCollisionConvexHull::Create() {
    constexpr bool bComputeAabb = false; // FIXME: Нужно ли сейчас считать aabb?
    return b3New( btConvexPointCloudShape, HullData->Data, HullData->GetVertexCount(), btVector3(1.f,1.f,1.f), bComputeAabb );

    //return b3New( btConvexHullShape, ( btScalar const * )HullData->Vertices.ToPtr(), HullData->Vertices.Length(), sizeof( Float3 ) );
}

btCollisionShape * FCollisionTriangleSoupBVH::Create() {
    return b3New( btScaledBvhTriangleMeshShape, BvhData->GetData(), btVector3(1.f,1.f,1.f) );

    // TODO: Create GImpact mesh shape for dynamic objects
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
    int getNumSubParts() const override {
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

    if ( TriangleInfoMap ) {
        b3Destroy( TriangleInfoMap );
    }
}

bool FCollisionTriangleSoupBVHData::UsedQuantizedAabbCompression() const {
    return bUsedQuantizedAabbCompression;
}

void FCollisionTriangleSoupBVHData::BuildBVH( bool bForceQuantizedAabbCompression ) {
    Interface->Vertices = TrisData->Vertices.ToPtr();
    Interface->Indices = TrisData->Indices.ToPtr();
    Interface->Subparts = TrisData->Subparts.ToPtr();
    Interface->SubpartCount = TrisData->Subparts.Size();

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

    TriangleInfoMap = b3New( btTriangleInfoMap );
    btGenerateInternalEdgeInfo( Data, TriangleInfoMap );
}

FCollisionTriangleSoupGimpact::FCollisionTriangleSoupGimpact() {
    Interface = b3New( FStridingMeshInterface );
}

FCollisionTriangleSoupGimpact::~FCollisionTriangleSoupGimpact() {
    b3Destroy( Interface );
}

btCollisionShape * FCollisionTriangleSoupGimpact::Create() {
    // FIXME: This shape don't work. Why?
    Interface->Vertices = TrisData->Vertices.ToPtr();
    Interface->Indices = TrisData->Indices.ToPtr();
    Interface->Subparts = TrisData->Subparts.ToPtr();
    Interface->SubpartCount = TrisData->Subparts.Size();
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

void FCollisionTriangleSoupData::Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, BvAxisAlignedBox const & _BoundingBox ) {
    Vertices.ResizeInvalidate( _VertexCount );
    Indices.ResizeInvalidate( _IndexCount );
    Subparts.ResizeInvalidate( 1 );

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

    BoundingBox = _BoundingBox;
    Subparts[0].BaseVertex  = 0;
    Subparts[0].VertexCount = _VertexCount;
    Subparts[0].FirstIndex  = 0;
    Subparts[0].IndexCount  = _IndexCount;
}

void FCollisionSphere::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    float sinTheta, cosTheta, sinPhi, cosPhi;

    const float detail = FMath::Floor( FMath::Max( 1.0f, Radius ) + 0.5f );

    const int numStacks = 8 * detail;
    const int numSlices = 12 * detail;

    const int vertexCount = ( numStacks + 1) * numSlices;
    const int indexCount = numStacks * numSlices * 6;

    const int firstVertex = _Vertices.Size();
    const int firstIndex = _Indices.Size();

    _Vertices.Resize( firstVertex + vertexCount );
    _Indices.Resize( firstIndex + indexCount );

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    for ( int stack = 0; stack <= numStacks; ++stack ) {
        const float theta = stack * FMath::_PI / numStacks;
        FMath::RadSinCos( theta, sinTheta, cosTheta );

        for ( int slice = 0; slice < numSlices; ++slice ) {            
            const float phi = slice * FMath::_2PI / numSlices;
            FMath::RadSinCos( phi, sinPhi, cosPhi );

            *pVertices++ = Float3( cosPhi * sinTheta, cosTheta, sinPhi * sinTheta ) * Radius + Position;

            // No rotation

            // TODO: work around with bProportionalScale ?
        }
    }

    for ( int stack = 0; stack < numStacks; ++stack ) {
        const int stackOffset = firstVertex + stack * numSlices;
        const int nextStackOffset = firstVertex + ( stack + 1 ) * numSlices;

        for ( int slice = 0; slice < numSlices; ++slice ) {
            const int nextSlice = (slice + 1) % numSlices;
            *pIndices++ = stackOffset + slice;
            *pIndices++ = stackOffset + nextSlice;
            *pIndices++ = nextStackOffset + nextSlice;
            *pIndices++ = nextStackOffset + nextSlice;
            *pIndices++ = nextStackOffset + slice;
            *pIndices++ = stackOffset + slice;
        }
    }
}

void FCollisionSphereRadii::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    float sinTheta, cosTheta, sinPhi, cosPhi;

    const float detail = FMath::Floor( FMath::Max( 1.0f, Radius.Max() ) + 0.5f );

    const int numStacks = 8 * detail;
    const int numSlices = 12 * detail;

    const int vertexCount = ( numStacks + 1 ) * numSlices;
    const int indexCount = numStacks * numSlices * 6;

    const int firstVertex = _Vertices.Size();
    const int firstIndex = _Indices.Size();

    _Vertices.Resize( firstVertex + vertexCount );
    _Indices.Resize( firstIndex + indexCount );

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    for ( int stack = 0; stack <= numStacks; ++stack ) {
        const float theta = stack * FMath::_PI / numStacks;
        FMath::RadSinCos( theta, sinTheta, cosTheta );

        for ( int slice = 0; slice < numSlices; ++slice ) {
            const float phi = slice * FMath::_2PI / numSlices;
            FMath::RadSinCos( phi, sinPhi, cosPhi );

            *pVertices++ = Rotation * ( Float3( cosPhi * sinTheta, cosTheta, sinPhi * sinTheta ) * Radius ) + Position;
        }
    }

    for ( int stack = 0; stack < numStacks; ++stack ) {
        const int stackOffset = firstVertex + stack * numSlices;
        const int nextStackOffset = firstVertex + ( stack + 1 ) * numSlices;

        for ( int slice = 0; slice < numSlices; ++slice ) {
            const int nextSlice = (slice + 1) % numSlices;
            *pIndices++ = stackOffset + slice;
            *pIndices++ = stackOffset + nextSlice;
            *pIndices++ = nextStackOffset + nextSlice;
            *pIndices++ = nextStackOffset + nextSlice;
            *pIndices++ = nextStackOffset + slice;
            *pIndices++ = stackOffset + slice;
        }
    }
}

void FCollisionBox::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    unsigned int const indices[36] = { 0,3,2, 2,1,0, 7,4,5, 5,6,7, 3,7,6, 6,2,3, 2,6,5, 5,1,2, 1,5,4, 4,0,1, 0,4,7, 7,3,0 };

    const int firstVertex = _Vertices.Size();
    const int firstIndex = _Indices.Size();

    _Vertices.Resize( firstVertex + 8 );
    _Indices.Resize( firstIndex + 36 );

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    *pVertices++ = Rotation * Float3( -HalfExtents.X, HalfExtents.Y, -HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3(  HalfExtents.X, HalfExtents.Y, -HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3(  HalfExtents.X, HalfExtents.Y,  HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3( -HalfExtents.X, HalfExtents.Y,  HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3( -HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3(  HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3(  HalfExtents.X, -HalfExtents.Y,  HalfExtents.Z ) + Position;
    *pVertices++ = Rotation * Float3( -HalfExtents.X, -HalfExtents.Y,  HalfExtents.Z ) + Position;

    for ( int i = 0 ; i < 36 ; i++ ) {
        *pIndices++ = firstVertex + indices[i];
    }
}

void FCollisionCylinder::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    float sinPhi, cosPhi;

    int idxRadius, idxRadius2, idxHeight;
    switch ( Axial ) {
    case AXIAL_X:
        idxRadius = 1;
        idxRadius2 = 2;
        idxHeight = 0;
        break;
    case AXIAL_Z:
        idxRadius = 0;
        idxRadius2 = 1;
        idxHeight = 2;
        break;
    case AXIAL_Y:
    default:
        idxRadius = 0;
        idxRadius2 = 2;
        idxHeight = 1;
        break;
    }

    const float detal = FMath::Floor( FMath::Max( 1.0f, HalfExtents[idxRadius] ) + 0.5f );

    const int numSlices = 8 * detal;
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

    for ( int slice = 0; slice < numSlices; slice++, pVertices++ ) {
        FMath::RadSinCos( slice * FMath::_2PI / numSlices, sinPhi, cosPhi );

        vert[idxRadius] = cosPhi * HalfExtents[idxRadius];
        vert[idxRadius2] = sinPhi * HalfExtents[idxRadius];
        vert[idxHeight] = HalfExtents[idxHeight];

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
        const int nextSlice = (slice + 1) % numSlices;
        *pIndices++ = offset + slice;
        *pIndices++ = offset + nextSlice;
        *pIndices++ = nextOffset + nextSlice;
        *pIndices++ = nextOffset + nextSlice;
        *pIndices++ = nextOffset + slice;
        *pIndices++ = offset + slice;
    }
}

void FCollisionCone::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    float sinPhi, cosPhi;

    int idxRadius, idxRadius2, idxHeight;
    switch ( Axial ) {
    case AXIAL_X:
        idxRadius = 1;
        idxRadius2 = 2;
        idxHeight = 0;
        break;
    case AXIAL_Z:
        idxRadius = 0;
        idxRadius2 = 1;
        idxHeight = 2;
        break;
    case AXIAL_Y:
    default:
        idxRadius = 0;
        idxRadius2 = 2;
        idxHeight = 1;
        break;
    }

    const float detal = FMath::Floor( FMath::Max( 1.0f, Radius ) + 0.5f );

    const int numSlices = 8 * detal;
    const int faceTriangles = numSlices - 2;

    const int vertexCount = numSlices + 1;
    const int indexCount = faceTriangles * 3 + numSlices * 3;

    const int firstIndex = _Indices.Size();
    const int firstVertex = _Vertices.Size();

    _Vertices.Resize( firstVertex + vertexCount );
    _Indices.Resize( firstIndex + indexCount );

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    Float3 vert;

    vert.Clear();
    vert[idxHeight] = Height;

    // top point
    *pVertices++ = Rotation * vert + Position;

    vert[idxHeight] = 0;

    for ( int slice = 0; slice < numSlices; slice++ ) {
        FMath::RadSinCos( slice * FMath::_2PI / numSlices, sinPhi, cosPhi );

        vert[idxRadius] = cosPhi * Radius;
        vert[idxRadius2] = sinPhi * Radius;

        *pVertices++ = Rotation * vert + Position;
    }

    const int offset = firstVertex + 1;

    // bottom face
    for ( int i = 0 ; i < faceTriangles; i++ ) {
        *pIndices++ = offset + 0;
        *pIndices++ = offset + i + 1;
        *pIndices++ = offset + i + 2;
    }

    // sides
    for ( int slice = 0; slice < numSlices; ++slice ) {
        *pIndices++ = firstVertex;
        *pIndices++ = offset + (slice + 1) % numSlices;
        *pIndices++ = offset + slice;
    }
}

void FCollisionCapsule::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {

    int idxRadius, idxRadius2, idxHeight;
    switch ( Axial ) {
    case AXIAL_X:
        idxRadius = 1;
        idxRadius2 = 2;
        idxHeight = 0;
        break;
    case AXIAL_Z:
        idxRadius = 0;
        idxRadius2 = 1;
        idxHeight = 2;
        break;
    case AXIAL_Y:
    default:
        idxRadius = 0;
        idxRadius2 = 2;
        idxHeight = 1;
        break;
    }

#if 0
    // Capsule is implemented as cylinder

    float sinPhi, cosPhi;

    const float detal = FMath::Floor( FMath::Max( 1.0f, Radius ) + 0.5f );

    const int numSlices = 8 * detal;
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

    const float halfOfTotalHeight = Height*0.5f + Radius;

    for ( int slice = 0; slice < numSlices; slice++, pVertices++ ) {
        FMath::RadSinCos( slice * FMath::_2PI / numSlices, sinPhi, cosPhi );

        vert[idxRadius] = cosPhi * Radius;
        vert[idxRadius2] = sinPhi * Radius;
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
    int x, y;
    float verticalAngle, horizontalAngle;
    
    unsigned int quad[ 4 ];

    const float detail = FMath::Floor( FMath::Max( 1.0f, Radius ) + 0.5f );

    const int numVerticalSubdivs = 6 * detail;
    const int numHorizontalSubdivs = 8 * detail;

    const int halfVerticalSubdivs = numVerticalSubdivs >> 1;

    const int vertexCount = ( numHorizontalSubdivs + 1 ) * ( numVerticalSubdivs + 1 ) * 2;
    const int indexCount = numHorizontalSubdivs * ( numVerticalSubdivs + 1 ) * 6;

    const int firstIndex = _Indices.Size();
    const int firstVertex = _Vertices.Size();

    _Vertices.Resize( firstVertex + vertexCount );
    _Indices.Resize( firstIndex + indexCount );

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    const float verticalStep = FMath::_PI / numVerticalSubdivs;
    const float horizontalStep = FMath::_2PI / numHorizontalSubdivs;

    const float halfHeight = Height * 0.5f;

    for ( y = 0, verticalAngle = -FMath::_HALF_PI; y <= halfVerticalSubdivs; y++ ) {
        float h, r;
        FMath::RadSinCos( verticalAngle, h, r );
        h = h * Radius - halfHeight;
        r *= Radius;
        for ( x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++ ) {
            float s, c;
            Float3 & v = *pVertices++;
            FMath::RadSinCos( horizontalAngle, s, c );
            v[ idxRadius ] = r * c;
            v[ idxRadius2 ] = r * s;
            v[ idxHeight ] = h;
            v = Rotation * v + Position;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for ( y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++ ) {
        float h, r;
        FMath::RadSinCos( verticalAngle, h, r );
        h = h * Radius + halfHeight;
        r *= Radius;
        for ( x = 0, horizontalAngle = 0; x <= numHorizontalSubdivs; x++ ) {
            float s, c;
            Float3 & v = *pVertices++;
            FMath::RadSinCos( horizontalAngle, s, c );
            v[ idxRadius ] = r * c;
            v[ idxRadius2 ] = r * s;
            v[ idxHeight ] = h;
            v = Rotation * v + Position;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for ( y = 0; y <= numVerticalSubdivs; y++ ) {
        const int y2 = y + 1;
        for ( x = 0; x < numHorizontalSubdivs; x++ ) {
            const int x2 = x + 1;
            quad[ 0 ] = firstVertex + y  * ( numHorizontalSubdivs + 1 ) + x;
            quad[ 1 ] = firstVertex + y2 * ( numHorizontalSubdivs + 1 ) + x;
            quad[ 2 ] = firstVertex + y2 * ( numHorizontalSubdivs + 1 ) + x2;
            quad[ 3 ] = firstVertex + y  * ( numHorizontalSubdivs + 1 ) + x2;
            *pIndices++ = quad[ 0 ];
            *pIndices++ = quad[ 1 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 3 ];
            *pIndices++ = quad[ 0 ];
        }
    }
#endif
}

void FCollisionConvexHull::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {

    if ( !HullData ) {
        return;
    }

    const int firstVertex = _Vertices.Size();
    const int firstIndex = _Indices.Size();

    _Vertices.Resize( firstVertex + HullData->GetVertexCount() );
    _Indices.Resize( firstIndex + HullData->GetIndexCount() );

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    for ( int i = 0 ; i < HullData->GetVertexCount() ; i++ ) {
        *pVertices++ = Rotation * HullData->GetVertices()[i] + Position;
    }

    for ( int i = 0 ; i < HullData->GetIndexCount() ; i++ ) {
        *pIndices++ = firstVertex + HullData->GetIndices()[i];
    }
}

void FCollisionTriangleSoupBVH::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {

    if ( BvhData ) {

        // Create from triangle mesh

        FCollisionTriangleSoupData * trisData = BvhData->TrisData;
        if ( !trisData ) {
            return;
        }

        const int firstIndex = _Indices.Size();
        const int firstVertex = _Vertices.Size();

        _Vertices.Resize( firstVertex + trisData->Vertices.Size() );

        int indexCount = 0;
        for ( FCollisionTriangleSoupData::FSubpart & subpart : trisData->Subparts ) {
            indexCount += subpart.IndexCount;
        }

        _Indices.Resize( firstIndex + indexCount );

        unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

        for ( FCollisionTriangleSoupData::FSubpart & subpart : trisData->Subparts ) {
            for ( int i = 0 ; i < subpart.IndexCount ; i++ ) {
                *pIndices++ = firstVertex + subpart.BaseVertex + trisData->Indices[ subpart.FirstIndex + i ];
            }
        }

        Float3 * pVertices = _Vertices.ToPtr() + firstVertex;

        for ( int i = 0 ; i < trisData->Vertices.Size() ; i++ ) {
            *pVertices++ = Rotation * trisData->Vertices[i] + Position;
        }
    }
#if 0
    else if ( HullData ) {

        // Create from hulls

        const int firstVertex = _Vertices.Length();
        const int firstIndex = _Indices.Length();

        _Vertices.Resize( firstVertex + HullData->GetVertexCount() );
        _Indices.Resize( firstIndex + HullData->GetIndexCount() );

        Float3 * pVertices = _Vertices.ToPtr() + firstVertex;
        unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

        for ( int i = 0 ; i < HullData->GetVertexCount() ; i++ ) {
            *pVertices++ = Rotation * HullData->GetVertices()[i] + Position;
        }

        for ( int i = 0 ; i < HullData->GetIndexCount() ; i++ ) {
            *pIndices++ = firstVertex + HullData->GetIndices()[i];
        }
    }
#endif
}

void FCollisionTriangleSoupGimpact::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    FCollisionTriangleSoupData * trisData = TrisData;
    if ( !trisData ) {
        return;
    }

    const int firstIndex = _Indices.Size();
    const int firstVertex = _Vertices.Size();

    _Vertices.Resize( firstVertex + trisData->Vertices.Size() );

    int indexCount = 0;
    for ( FCollisionTriangleSoupData::FSubpart & subpart : trisData->Subparts ) {
        indexCount += subpart.IndexCount;
    }

    _Indices.Resize( firstIndex + indexCount );

    unsigned int * pIndices = _Indices.ToPtr() + firstIndex;

    for ( FCollisionTriangleSoupData::FSubpart & subpart : trisData->Subparts ) {
        for ( int i = 0 ; i < subpart.IndexCount ; i++ ) {
            *pIndices++ = firstVertex + subpart.BaseVertex + trisData->Indices[ subpart.FirstIndex + i ];
        }
    }

    Float3 * pVertices = _Vertices.ToPtr() + firstVertex;

    for ( int i = 0 ; i < trisData->Vertices.Size() ; i++ ) {
        *pVertices++ = Rotation * trisData->Vertices[i] + Position;
    }
}

void FCollisionBodyComposition::CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {
    for ( FCollisionBody * collisionBody : CollisionBodies ) {
        collisionBody->CreateGeometry( _Vertices, _Indices );
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

static int FindPlane( PlaneF const & _Plane , PlaneF const * _Planes, int _NumPlanes ) {
    for ( int i = 0 ; i < _NumPlanes ; i++ ) {
        if ( _Plane.Normal.Dot( _Planes[i].Normal ) > 0.999f ) {
            return i;
        }
    }
    return -1;
}

static bool AreVerticesBehindPlane( PlaneF const & _Plane, Float3 const * _Vertices, int _NumVertices, float _Margin ) {
    for ( int i = 0 ; i < _NumVertices ; i++ ) {
        float dist = _Plane.Normal.Dot( _Vertices[i] ) + _Plane.D - _Margin;
        if ( dist > 0.0f ) {
            return false;
        }
    }
    return true;
}

void ConvexHullPlanesFromVertices( Float3 const * _Vertices, int _NumVertices, TPodArray< PlaneF > & _Planes ) {
    PlaneF plane;
    Float3 edge0, edge1;

    const float margin = 0.01f;

    _Planes.Clear();

    for ( int i = 0 ; i < _NumVertices ; i++ )  {
        Float3 const & normal1 = _Vertices[i];

        for ( int j = i + 1 ; j < _NumVertices ; j++ ) {
            Float3 const & normal2 = _Vertices[j];

            edge0 = normal2 - normal1;

            for ( int k = j + 1 ; k < _NumVertices ; k++ ) {
                Float3 const & normal3 = _Vertices[k];

                edge1 = normal3 - normal1;

                float normalSign = 1;

                for ( int ww = 0 ; ww < 2 ; ww++ ) {
                    plane.Normal = normalSign * edge0.Cross( edge1 );
                    if ( plane.Normal.LengthSqr() > 0.0001f ) {
                        plane.Normal.NormalizeSelf();

                        if ( FindPlane( plane, _Planes.ToPtr(), _Planes.Size() ) == -1 ) {
                            plane.D = -plane.Normal.Dot( normal1 );

                            if ( AreVerticesBehindPlane( plane, _Vertices, _NumVertices, margin ) ) {
                                _Planes.Append( plane );
                            }
                        }
                    }
                    normalSign = -1;
                }

            }
        }
    }
}

void ConvexHullVerticesFromPlanes( PlaneF const * _Planes, int _NumPlanes, TPodArray< Float3 > & _Vertices ) {
    constexpr float tolerance = 0.0001f;
    constexpr float quotientTolerance = 0.000001f;

    _Vertices.Clear();

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
                        float quotient = normal1.Dot( n2n3 );
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

//struct FConvexDecompositionDesc {
//    // options
//    unsigned int  Depth;    // depth to split, a maximum of 10, generally not over 7.
//    float         ConcavityThreshold; // the concavity threshold percentage.  0=20 is reasonable.
//    float         VolumeConservationThreshold; // the percentage volume conservation threshold to collapse hulls. 0-30 is reasonable.

//    // hull output limits.
//    unsigned int  MaxHullVertices; // maximum number of vertices in the output hull. Recommended 32 or less.
//    float         HullSkinWidth;   // a skin width to apply to the output hulls.
//};

//#include <bullet3/Extras/ConvexDecomposition/ConvexDecomposition.h>
#include <bullet3/Extras/HACD/hacdHACD.h>

//class MyConvexDecompInterface : public ConvexDecomposition::ConvexDecompInterface {
//public:
//    MyConvexDecompInterface() {

//    }

//    void ConvexDecompResult( ConvexDecomposition::ConvexResult & _Result ) override {

//#if 0
//        // the convex hull.
//        unsigned int	    mHullVcount;
//        float *			    mHullVertices;
//        unsigned  int       mHullTcount;
//        unsigned int	*   mHullIndices;

//        float               mHullVolume;		    // the volume of the convex hull.

//        float               mOBBSides[3];			  // the width, height and breadth of the best fit OBB
//        float               mOBBCenter[3];      // the center of the OBB
//        float               mOBBOrientation[4]; // the quaternion rotation of the OBB.
//        float               mOBBTransform[16];  // the 4x4 transform of the OBB.
//        float               mOBBVolume;         // the volume of the OBB

//        float               mSphereRadius;      // radius and center of best fit sphere
//        float               mSphereCenter[3];
//        float               mSphereVolume;      // volume of the best fit sphere
//#endif

//        FConvexHullDesc & hull = Hulls.Append();

//        hull.FirstVertex = Vertices.Length();
//        hull.VertexCount = _Result.mHullVcount;
//        hull.FirstIndex = Indices.Length();
//        hull.IndexCount = _Result.mHullTcount;

//        Vertices.Resize( Vertices.Length() + hull.VertexCount );
//        Indices.Resize( Indices.Length() + hull.IndexCount );

//        memcpy( &Vertices[ hull.FirstVertex ], _Result.mHullVertices, hull.VertexCount * (sizeof( float ) * 3) );
//        memcpy( &Indices[ hull.FirstIndex ], _Result.mHullIndices, hull.IndexCount * sizeof( unsigned int ) );



//    }

//    struct FConvexHullDesc {
//        int FirstVertex;
//        int VertexCount;
//        int FirstIndex;
//        int IndexCount;

//#if 0
//        float   HullVolume;           // the volume of the convex hull.

//        float   OBBSides[3];          // the width, height and breadth of the best fit OBB
//        Float3  OBBCenter;         // the center of the OBB
//        float   OBBOrientation[4];    // the quaternion rotation of the OBB.
//        Float4x4 OBBTransform;     // the 4x4 transform of the OBB.
//        float   OBBVolume;            // the volume of the OBB

//        float   SphereRadius;         // radius and center of best fit sphere
//        Float3  SphereCenter;
//        float   SphereVolume;         // volume of the best fit sphere
//#endif
//    };

//    TPodArray< Float3 > Vertices;
//    TPodArray< unsigned int > Indices;
//    TPodArray< FConvexHullDesc > Hulls;
//};
// TODO: try ConvexBuilder

void BakeCollisionMarginConvexHull( Float3 const * _InVertices, int _VertexCount, TPodArray< Float3 > & _OutVertices, float _Margin ) {
    TPodArray< PlaneF > planes;

    ConvexHullPlanesFromVertices( _InVertices, _VertexCount, planes );

    for ( int i = 0 ; i < planes.Size() ; i++ ) {
        PlaneF & plane = planes[i];

        plane.D += _Margin;
    }

    ConvexHullVerticesFromPlanes( planes.ToPtr(), planes.Size(), _OutVertices );
}

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 TPodArray< Float3 > & _OutVertices,
                                 TPodArray< unsigned int > & _OutIndices,
                                 TPodArray< FConvexHullDesc > & _OutHulls )
{
    int hunkMark = GHunkMemory.SetHunkMark();

    HACD::Vec3< HACD::Real > * points =
            ( HACD::Vec3< HACD::Real > * )GHunkMemory.HunkMemory( _VerticesCount * sizeof( HACD::Vec3< HACD::Real > ), 1 );

    HACD::Vec3< long > * triangles =
            ( HACD::Vec3< long > * )GHunkMemory.HunkMemory( ( _IndicesCount / 3 ) * sizeof( HACD::Vec3< long > ), 1 );

    byte const * srcVertices = ( byte const * )_Vertices;
    for ( int i = 0 ; i < _VerticesCount ; i++ ) {
        Float3 const * vertex = ( Float3 const * )srcVertices;

        points[i] = HACD::Vec3< HACD::Real >( vertex->X, vertex->Y, vertex->Z );

        srcVertices += _VertexStride;
    }

    int triangleNum = 0;
    for ( int i = 0 ; i < _IndicesCount ; i+=3, triangleNum++ ) {
        triangles[triangleNum] = HACD::Vec3< long >( _Indices[i], _Indices[i+1], _Indices[i+2] );
    }

    HACD::HACD hacd;
    hacd.SetPoints( points );
    hacd.SetNPoints( _VerticesCount );
    hacd.SetTriangles( triangles );
    hacd.SetNTriangles( _IndicesCount / 3 );
//    hacd.SetCompacityWeight( 0.1 );
//    hacd.SetVolumeWeight( 0.0 );
//    hacd.SetNClusters( 2 );                     // recommended 2
//    hacd.SetNVerticesPerCH( 100 );
//    hacd.SetConcavity( 100 );                   // recommended 100
//    hacd.SetAddExtraDistPoints( false );        // recommended false
//    hacd.SetAddNeighboursDistPoints( false );   // recommended false
//    hacd.SetAddFacesPoints( false );            // recommended false

    hacd.SetCompacityWeight( 0.1 );
    hacd.SetVolumeWeight( 0.0 );
    hacd.SetNClusters( 2 );                     // recommended 2
    hacd.SetNVerticesPerCH( 100 );
    hacd.SetConcavity( 0.01 );                   // recommended 100
    hacd.SetAddExtraDistPoints( true );        // recommended false
    hacd.SetAddNeighboursDistPoints( true );   // recommended false
    hacd.SetAddFacesPoints( true );            // recommended false

    hacd.Compute();

    int maxPointsPerCluster = 0;
    int maxTrianglesPerCluster = 0;
    int totalPoints = 0;
    int totalTriangles = 0;

    int numClusters = hacd.GetNClusters();
    for ( int cluster = 0 ; cluster < numClusters ; cluster++ ) {
        int numPoints = hacd.GetNPointsCH( cluster );
        int numTriangles = hacd.GetNTrianglesCH( cluster );

        totalPoints += numPoints;
        totalTriangles += numTriangles;

        maxPointsPerCluster = FMath::Max( maxPointsPerCluster, numPoints );
        maxTrianglesPerCluster = FMath::Max( maxTrianglesPerCluster, numTriangles );
    }

    HACD::Vec3< HACD::Real > * hullPoints =
            ( HACD::Vec3< HACD::Real > * )GHunkMemory.HunkMemory( maxPointsPerCluster * sizeof( HACD::Vec3< HACD::Real > ), 1 );

    HACD::Vec3< long > * hullTriangles =
            ( HACD::Vec3< long > * )GHunkMemory.HunkMemory( maxTrianglesPerCluster * sizeof( HACD::Vec3< long > ), 1 );


    _OutHulls.ResizeInvalidate( numClusters );
    _OutVertices.ResizeInvalidate( totalPoints );
    _OutIndices.ResizeInvalidate( totalTriangles * 3 );

    totalPoints = 0;
    totalTriangles = 0;

    for ( int cluster = 0 ; cluster < numClusters ; cluster++ ) {
        int numPoints = hacd.GetNPointsCH( cluster );
        int numTriangles = hacd.GetNTrianglesCH( cluster );

        hacd.GetCH( cluster, hullPoints, hullTriangles );

        FConvexHullDesc & hull = _OutHulls[cluster];
        hull.FirstVertex = totalPoints;
        hull.VertexCount = numPoints;
        hull.FirstIndex = totalTriangles * 3;
        hull.IndexCount = numTriangles * 3;
        hull.Centroid.Clear();

        totalPoints += numPoints;
        totalTriangles += numTriangles;

        Float3 * pVertices = _OutVertices.ToPtr() + hull.FirstVertex;
        for ( int i = 0 ; i < numPoints ; i++, pVertices++ ) {
            pVertices->X = hullPoints[i].X();
            pVertices->Y = hullPoints[i].Y();
            pVertices->Z = hullPoints[i].Z();

            hull.Centroid += *pVertices;
        }

        hull.Centroid /= (float)numPoints;

        // Adjust vertices
        pVertices = _OutVertices.ToPtr() + hull.FirstVertex;
        for ( int i = 0 ; i < numPoints ; i++, pVertices++ ) {
            *pVertices -= hull.Centroid;
        }

        unsigned int * pIndices = _OutIndices.ToPtr() + hull.FirstIndex;
        int n = 0;
        for ( int i = 0 ; i < hull.IndexCount ; i+=3, n++ ) {
            *pIndices++ = hullTriangles[n].X();
            *pIndices++ = hullTriangles[n].Y();
            *pIndices++ = hullTriangles[n].Z();
        }
    }

    GHunkMemory.ClearToMark( hunkMark );
}

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 FCollisionBodyComposition & _BodyComposition ) {

    TPodArray< Float3 > HullVertices;
    TPodArray< unsigned int > HullIndices;
    TPodArray< FConvexHullDesc > Hulls;

    PerformConvexDecomposition( _Vertices,
                                _VerticesCount,
                                _VertexStride,
                                _Indices,
                                _IndicesCount,
                                HullVertices,
                                HullIndices,
                                Hulls );

    _BodyComposition.Clear();

    for ( FConvexHullDesc const & hull : Hulls ) {

        FCollisionConvexHullData * hullData = CreateInstanceOf< FCollisionConvexHullData >();

#if 0
        BakeCollisionMarginConvexHull( HullVertices.ToPtr() + hull.FirstVertex, hull.VertexCount, hullData->Vertices );
#else
        hullData->Initialize( HullVertices.ToPtr() + hull.FirstVertex, hull.VertexCount, HullIndices.ToPtr() + hull.FirstIndex, hull.IndexCount );
        //hullData->Vertices.Resize( hull.VertexCount );
        //memcpy( hullData->Vertices.ToPtr(), HullVertices.ToPtr() + hull.FirstVertex, hull.VertexCount * sizeof( Float3 ) );
#endif

        FCollisionConvexHull * collisionBody = _BodyComposition.AddCollisionBody< FCollisionConvexHull >();
        collisionBody->Position = hull.Centroid;
        collisionBody->Margin = 0.01f;
        collisionBody->HullData = hullData;
    }
}

#include <VHACD.h>

enum EVHACDMode {
    VHACD_MODE_VOXEL = 0,               // recommended
    VHACD_MODE_TETRAHEDRON = 1
};

void PerformConvexDecompositionVHACD( Float3 const * _Vertices,
                                      int _VerticesCount,
                                      int _VertexStride,
                                      unsigned int const * _Indices,
                                      int _IndicesCount,
                                      TPodArray< Float3 > & _OutVertices,
                                      TPodArray< unsigned int > & _OutIndices,
                                      TPodArray< FConvexHullDesc > & _OutHulls,
                                      Float3 & _CenterOfMass ) {


    class Callback : public VHACD::IVHACD::IUserCallback {
    public:
        void Update( const double overallProgress,
                     const double stageProgress,
                     const double operationProgress,
                     const char* const stage,
                     const char* const operation ) override {

            GLogger.Printf( "Overall progress %f, %s progress %f, %s progress %f\n",
                            overallProgress, stage, stageProgress, operation, operationProgress );
        }
    };
    class Logger : public VHACD::IVHACD::IUserLogger {
    public:
        void Log( const char * const msg ) override {
            GLogger.Printf( "%s", msg );
        }
    };

    Callback callback;
    Logger logger;

    VHACD::IVHACD * vhacd = VHACD::CreateVHACD();

    VHACD::IVHACD::Parameters params;

//    params.m_resolution = 80000000;//100000;
//    params.m_concavity = 0.00001;
//    params.m_planeDownsampling = 4;
//    params.m_convexhullDownsampling = 4;
//    params.m_alpha = 0.005;
//    params.m_beta = 0.005;
//    params.m_pca = 0;
//    params.m_mode = VHACD_MODE_VOXEL;
//    params.m_maxNumVerticesPerCH = 64;
//    params.m_minVolumePerCH = 0.0001;
    params.m_callback = &callback;
    params.m_logger = &logger;
//    params.m_convexhullApproximation = true;
//    params.m_oclAcceleration = false;//true;
//    params.m_maxConvexHulls = 1024*10;
//    params.m_projectHullVertices = true; // This will project the output convex hull vertices onto the original source mesh to increase the floating point accuracy of the results

    params.m_resolution = 100000;
params.m_planeDownsampling = 1;
params.m_convexhullDownsampling = 1;
    params.m_alpha = 0.0001;
    params.m_beta = 0.0001;
    params.m_pca = 0;
    params.m_convexhullApproximation = false;
    params.m_concavity = 0.00000001;
    params.m_mode = VHACD_MODE_VOXEL;//VHACD_MODE_TETRAHEDRON;
    params.m_oclAcceleration = false;
    params.m_projectHullVertices = false;

    int hunkMark = GHunkMemory.SetHunkMark();

    Double3 * tempVertices = ( Double3 * )GHunkMemory.HunkMemory( _VerticesCount * sizeof( Double3 ), 1 );

    byte const * srcVertices = ( byte const * )_Vertices;
    for ( int i = 0 ; i < _VerticesCount ; i++ ) {
        tempVertices[i] = Double3( *( Float3 const * )srcVertices );

        srcVertices += _VertexStride;
    }

    bool bResult = vhacd->Compute( ( const double * )tempVertices, _VerticesCount, _Indices, _IndicesCount / 3, params );

    if ( bResult ) {
        double centerOfMass[3];
        if ( !vhacd->ComputeCenterOfMass( centerOfMass ) ) {
            centerOfMass[0] = centerOfMass[1] = centerOfMass[2] = 0;
        }

        _CenterOfMass[0] = centerOfMass[0];
        _CenterOfMass[1] = centerOfMass[1];
        _CenterOfMass[2] = centerOfMass[2];

        VHACD::IVHACD::ConvexHull ch;
        _OutHulls.ResizeInvalidate( vhacd->GetNConvexHulls() );
        int totalVertices = 0;
        int totalIndices = 0;
        for ( int i = 0 ; i < _OutHulls.Size() ; i++ ) {
            FConvexHullDesc & hull = _OutHulls[i];

            vhacd->GetConvexHull( i, ch );

            hull.FirstVertex = totalVertices;
            hull.VertexCount = ch.m_nPoints;
            hull.FirstIndex = totalIndices;
            hull.IndexCount = ch.m_nTriangles * 3;
            hull.Centroid[0] = ch.m_center[0];
            hull.Centroid[1] = ch.m_center[1];
            hull.Centroid[2] = ch.m_center[2];

            totalVertices += hull.VertexCount;
            totalIndices += hull.IndexCount;
        }

        _OutVertices.ResizeInvalidate( totalVertices );
        _OutIndices.ResizeInvalidate( totalIndices );

        for ( int i = 0 ; i < _OutHulls.Size() ; i++ ) {
            FConvexHullDesc & hull = _OutHulls[i];

            vhacd->GetConvexHull( i, ch );

            Float3 * pVertices = _OutVertices.ToPtr() + hull.FirstVertex;
            for ( int v = 0 ; v < hull.VertexCount ; v++, pVertices++ ) {
                pVertices->X = ch.m_points[v*3+0] - ch.m_center[0];
                pVertices->Y = ch.m_points[v*3+1] - ch.m_center[1];
                pVertices->Z = ch.m_points[v*3+2] - ch.m_center[2];
            }

            unsigned int * pIndices = _OutIndices.ToPtr() + hull.FirstIndex;
            for ( int v = 0 ; v < hull.IndexCount ; v += 3, pIndices += 3 ) {
                pIndices[0] = ch.m_triangles[v+0];
                pIndices[1] = ch.m_triangles[v+1];
                pIndices[2] = ch.m_triangles[v+2];
            }
        }
    } else {
        GLogger.Printf( "PerformConvexDecompositionVHACD: convex decomposition error\n" );

        _OutVertices.Clear();
        _OutIndices.Clear();
        _OutHulls.Clear();
    }

    vhacd->Clean();
    vhacd->Release();

    GHunkMemory.ClearToMark( hunkMark );
}

void CreateCollisionShape( FCollisionBodyComposition const & BodyComposition, Float3 const & _Scale, btCompoundShape ** _CompoundShape, Float3 * _CenterOfMass ) {
    *_CompoundShape = b3New( btCompoundShape );
    *_CenterOfMass = _Scale * BodyComposition.CenterOfMass;

    if ( !BodyComposition.CollisionBodies.IsEmpty() ) {
        const btVector3 scaling = btVectorToFloat3( _Scale );
        btTransform shapeTransform;

        for ( FCollisionBody * collisionBody : BodyComposition.CollisionBodies ) {
            btCollisionShape * shape = collisionBody->Create();

            shape->setMargin( collisionBody->Margin );
            shape->setUserPointer( collisionBody );
            shape->setLocalScaling( shape->getLocalScaling() * scaling );

            shapeTransform.setOrigin( btVectorToFloat3( _Scale * collisionBody->Position - *_CenterOfMass ) );
            shapeTransform.setRotation( btQuaternionToQuat( collisionBody->Rotation ) );

            (*_CompoundShape)->addChildShape( shapeTransform, shape );

            collisionBody->AddRef();
        }
    }
}

void DestroyCollisionShape( btCompoundShape * _CompoundShape ) {
    int numShapes = _CompoundShape->getNumChildShapes();
    for ( int i = numShapes-1 ; i >= 0 ; i-- ) {
        btCollisionShape * shape = _CompoundShape->getChildShape( i );
        static_cast< FCollisionBody * >( shape->getUserPointer() )->RemoveRef();
        b3Destroy( shape );
    }
    b3Destroy( _CompoundShape );
}
