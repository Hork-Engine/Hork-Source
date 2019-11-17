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

#pragma once

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Base/Public/DebugDraw.h>
#include "Material.h"
#include "CollisionBody.h"
#include "Skeleton.h"

class AIndexedMesh;

/**

ASocketDef

Socket for attaching

*/
class ASocketDef : public ABaseObject {
    AN_CLASS( ASocketDef, ABaseObject )

public:
    Float3 Position;
    Float3 Scale;
    Quat Rotation;
    int JointIndex;

protected:
    ASocketDef() : Position(0.0f), Scale(1.0f), Rotation(Quat::Identity()), JointIndex(-1)
    {
    }
};

/**

STriangleHitResult

Raycast hit result

*/
struct STriangleHitResult {
    Float3 Location;
    Float3 Normal;
    Float2 UV;
    float Distance;
    unsigned int Indices[3];
    AMaterialInstance * Material;
};

/**

SNodeAABB

*/
struct SNodeAABB {
    BvAxisAlignedBox Bounds;
    int32_t Index;          // First primitive in leaf (Index >= 0), next node index (Index < 0)
    int32_t PrimitiveCount;

    bool IsLeaf() const {
        return Index >= 0;
    }

    void Read( IStreamBase & _Stream ) {
        _Stream.ReadObject( Bounds );
        Index = _Stream.ReadInt32();
        PrimitiveCount = _Stream.ReadInt32();
    }

    void Write( IStreamBase & _Stream ) const {
        _Stream.WriteObject( Bounds );
        _Stream.WriteInt32( Index );
        _Stream.WriteInt32( PrimitiveCount );
    }
};

/**

ATreeAABB

Binary AABB-based BVH tree

*/
class ATreeAABB : public ABaseObject {
    AN_CLASS( ATreeAABB, ABaseObject )

public:
    void Initialize( SMeshVertex const * _Vertices, unsigned int const * _Indices, unsigned int _IndexCount, int _BaseVertex, unsigned int _PrimitivesPerLeaf );

    void Purge();

    int MarkRayOverlappingLeafs( Float3 const & _RayStart, Float3 const & _RayEnd, unsigned int * _MarkLeafs, int _MaxLeafs ) const;

    int MarkBoxOverlappingLeafs( BvAxisAlignedBox const & _Bounds, unsigned int * _MarkLeafs, int _MaxLeafs ) const;

    TPodArray< SNodeAABB > const & GetNodes() const { return Nodes; }

    unsigned int const * GetIndirection() const { return Indirection.ToPtr(); }

    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

    void Read( IStreamBase & _Stream );
    void Write( IStreamBase & _Stream ) const;

protected:
    ATreeAABB();
    ~ATreeAABB() {}

private:
    void Subdivide( struct SAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _MaxPrimitive, unsigned int _PrimitivesPerLeaf,
        int & _PrimitiveIndex, const unsigned int * _Indices );

    TPodArray< SNodeAABB > Nodes;
    TPodArray< unsigned int > Indirection;
    BvAxisAlignedBox BoundingBox;
};

/**

AIndexedMeshSubpart

Part of indexed mesh (submesh / element)

*/
class AIndexedMeshSubpart : public ABaseObject {
    AN_CLASS( AIndexedMeshSubpart, ABaseObject )

    friend class AIndexedMesh;

public:
    void SetBaseVertex( int _BaseVertex );
    void SetFirstIndex( int _FirstIndex );
    void SetVertexCount( int _VertexCount );
    void SetIndexCount( int _IndexCount );
    void SetMaterialInstance( AMaterialInstance * _MaterialInstance );

    int GetBaseVertex() const { return BaseVertex; }
    int GetFirstIndex() const { return FirstIndex; }
    int GetVertexCount() const { return VertexCount; }
    int GetIndexCount() const { return IndexCount; }
    AMaterialInstance * GetMaterialInstance() { return MaterialInstance; }

    void SetBoundingBox( BvAxisAlignedBox const & _BoundingBox );

    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

    AIndexedMesh * GetOwner() { return OwnerMesh; }

    void GenerateBVH( unsigned int PrimitivesPerLeaf = 16 );

    void SetBVH( ATreeAABB * BVH );

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< STriangleHitResult > & _HitResult ) const;

    /** Check ray intersection */
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const;

    void DrawBVH( ADebugDraw * _DebugDraw );

protected:
    AIndexedMeshSubpart();
    ~AIndexedMeshSubpart();

private:
    AIndexedMesh * OwnerMesh;
    BvAxisAlignedBox BoundingBox;
    int BaseVertex;
    int FirstIndex;
    int VertexCount;
    int IndexCount;
    TRef< AMaterialInstance > MaterialInstance;
    TRef< ATreeAABB > AABBTree;
    bool bAABBTreeDirty;
};

/**

ALightmapUV

Lightmap UV channel

*/
class ALightmapUV : public ABaseObject, public IGPUResourceOwner {
    AN_CLASS( ALightmapUV, ABaseObject )

    friend class AIndexedMesh;

public:
    SMeshLightmapUV * GetVertices() { return Vertices.ToPtr(); }
    SMeshLightmapUV const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( SMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    ABufferGPU * GetGPUResource() { return VertexBufferGPU; }

    AIndexedMesh * GetOwner() { return OwnerMesh; }

protected:
    ALightmapUV();
    ~ALightmapUV();

    void OnInitialize( int _NumVertices );

    /** IGPUResourceOwner interface */
    void UploadResourceGPU( AResourceGPU * _Resource ) override {}

private:
    ABufferGPU * VertexBufferGPU;
    AIndexedMesh * OwnerMesh;
    int IndexInArrayOfUVs = -1;
    TPodArrayHeap< SMeshLightmapUV > Vertices;
    bool bDynamicStorage;
};

/**

AVertexLight

Vertex light channel

*/
class AVertexLight : public ABaseObject, public IGPUResourceOwner {
    AN_CLASS( AVertexLight, ABaseObject )

    friend class AIndexedMesh;

public:
    SMeshVertexLight * GetVertices() { return Vertices.ToPtr(); }
    SMeshVertexLight const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( SMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    ABufferGPU * GetGPUResource() { return VertexBufferGPU; }

    AIndexedMesh * GetOwner() { return OwnerMesh; }

protected:
    AVertexLight();
    ~AVertexLight();

    void OnInitialize( int _NumVertices );

    /** IGPUResourceOwner interface */
    void UploadResourceGPU( AResourceGPU * _Resource ) override {}

private:
    ABufferGPU * VertexBufferGPU;
    AIndexedMesh * OwnerMesh;
    int IndexInArrayOfChannels = -1;
    TPodArrayHeap< SMeshVertexLight > Vertices;
    bool bDynamicStorage;
};

using ALightmapUVChannels = TPodArray< ALightmapUV *, 1 >;
using AVertexLightChannels = TPodArray< AVertexLight *, 1 >;
using AIndexedMeshSubpartArray = TPodArray< AIndexedMeshSubpart *, 1 >;

struct SSoftbodyLink {
    unsigned int Indices[2];
};

struct SSoftbodyFace {
    unsigned int Indices[3];
};


/**

ASkin

*/
struct ASkin
{
    /** Index of the joint in skeleton */
    TPodArray< int32_t > JointIndices;

    /** Transform vertex to joint-space */
    TPodArray< Float3x4 > OffsetMatrices;
};


/**

AIndexedMesh

Triangulated 3d surface with indexed vertices

*/
class AIndexedMesh : public AResourceBase, public IGPUResourceOwner {
    AN_CLASS( AIndexedMesh, AResourceBase )

    friend class ALightmapUV;
    friend class AVertexLight;
    friend class AIndexedMeshSubpart;

public:
    /** Rigid body collision model */
    ACollisionBodyComposition BodyComposition; // TODO: StaticBody, DynamicBody???

    /** Soft body collision model */
    TPodArray< SSoftbodyLink > SoftbodyLinks;
    TPodArray< SSoftbodyFace > SoftbodyFaces;

    /** Allocate mesh */
    void Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh = false, bool _DynamicStorage = false );

    /** Helper. Create box mesh */
    void InitializeBoxMesh( Float3 const & _Size, float _TexCoordScale );

    /** Helper. Create sphere mesh */
    void InitializeSphereMesh( float _Radius, float _TexCoordScale, int _NumVerticalSubdivs = 32, int _NumHorizontalSubdivs = 32 );

    /** Helper. Create plane mesh */
    void InitializePlaneMesh( float _Width, float _Height, float _TexCoordScale );

    /** Helper. Create patch mesh */
    void InitializePatchMesh( Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs );

    /** Helper. Create cylinder mesh */
    void InitializeCylinderMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

    /** Helper. Create cone mesh */
    void InitializeConeMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

    /** Helper. Create capsule mesh */
    void InitializeCapsuleMesh( float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs = 6, int _NumHorizontalSubdivs = 8 );

    /** Purge model data */
    void Purge();

    /** Skinned mesh have 4 weights for each vertex */
    bool IsSkinned() const { return bSkinnedMesh; }

    /** Dynamic storage is the mesh that updates every or almost every frame */
    bool IsDynamicStorage() const { return bDynamicStorage; }

    /** Get mesh part */
    AIndexedMeshSubpart * GetSubpart( int _SubpartIndex );

    /** Create lightmap channel to store lighmap UVs */
    ALightmapUV * CreateLightmapUVChannel();

    /** Create vertex light channel to store light colors */
    AVertexLight * CreateVertexLightChannel();

    /** Add the socket */
    void AddSocket( ASocketDef * _Socket );
    // TODO: RemoveSocket?

    /** Find socket by name */
    ASocketDef * FindSocket( const char * _Name );

    /** Get array of sockets */
    TPodArray< ASocketDef * > const & GetSockets() const { return Sockets; }

    /** Set skeleton for the mesh. */
    void SetSkeleton( ASkeleton * _Skeleton );

    /** Skeleton for the mesh. Never return null. */
    ASkeleton * GetSkeleton() { return Skeleton; }

    /** Set mesh skin */
    void SetSkin( int32_t const * _JointIndices, Float3x4 const * _OffsetMatrices, int _JointsCount );

    /** Get mesh skin */
    ASkin const & GetSkin() const { return Skin; }

    /** Set subpart material */
    void SetMaterialInstance( int _SubpartIndex, AMaterialInstance * _MaterialInstance );

    /** Set subpart bounding box */
    void SetBoundingBox( int _SubpartIndex, BvAxisAlignedBox const & _BoundingBox );

    /** Get mesh vertices */
    SMeshVertex * GetVertices() { return Vertices.ToPtr(); }

    /** Get mesh vertices */
    SMeshVertex const * GetVertices() const { return Vertices.ToPtr(); }

    /** Get weights for vertex skinning */
    SMeshVertexJoint * GetWeights() { return Weights.ToPtr(); }

    /** Get weights for vertex skinning */
    SMeshVertexJoint const * GetWeights() const { return Weights.ToPtr(); }

    /** Get mesh indices */
    unsigned int * GetIndices() { return Indices.ToPtr(); }

    /** Get mesh indices */
    unsigned int const * GetIndices() const { return Indices.ToPtr(); }

    /** Get total vertex count */
    int GetVertexCount() const { return Vertices.Size(); }

    /** Get total index count */
    int GetIndexCount() const { return Indices.Size(); }

    /** Get all mesh subparts */
    AIndexedMeshSubpartArray const & GetSubparts() const { return Subparts; }

    /** Max primitives per leaf. For raycasting */
    unsigned int GetRaycastPrimitivesPerLeaf() const { return RaycastPrimitivesPerLeaf; }

    /** Get all lightmap channels */
    ALightmapUVChannels const & GetLightmapUVChannels() const { return LightmapUVs; }

    /** Get all vertex light channels */
    AVertexLightChannels const & GetVertexLightChannels() const { return VertexLightChannels; }

    /** Write vertices at location and send them to GPU */
    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );

    /** Write vertices at location and send them to GPU */
    bool WriteVertexData( SMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    /** Write joint weights at location and send them to GPU */
    bool SendJointWeightsToGPU( int _VerticesCount, int _StartVertexLocation );

    /** Write joint weights at location and send them to GPU */
    bool WriteJointWeights( SMeshVertexJoint const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    /** Write indices at location and send them to GPU */
    bool SendIndexDataToGPU( int _IndexCount, int _StartIndexLocation );

    /** Write indices at location and send them to GPU */
    bool WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation );

    void UpdateBoundingBox();

    BvAxisAlignedBox const & GetBoundingBox() const;

    /** Get mesh GPU buffers */
    ABufferGPU * GetVertexBufferGPU() { return VertexBufferGPU; }
    ABufferGPU * GetIndexBufferGPU() { return IndexBufferGPU; }
    ABufferGPU * GetWeightsBufferGPU() { return WeightsBufferGPU; }

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< STriangleHitResult > & _HitResult ) const;

    /** Check ray intersection */
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], TRef< AMaterialInstance > & _Material ) const;

    /** Create BVH for raycast optimization */
    void GenerateBVH( unsigned int PrimitivesPerLeaf = 16 );

    /** Generate static collisions */
    void GenerateRigidbodyCollisions();

    void GenerateSoftbodyFacesFromMeshIndices();

    void GenerateSoftbodyLinksFromFaces();

    void DrawDebug( ADebugDraw * _DebugDraw );

protected:
    AIndexedMesh();
    ~AIndexedMesh();

    /** Load resource from file */
    bool LoadResource( AString const & _Path ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/Meshes/Box"; }

    /** IGPUResourceOwner interface */
    void UploadResourceGPU( AResourceGPU * _Resource ) override {}

private:
    ABufferGPU * VertexBufferGPU;
    ABufferGPU * IndexBufferGPU;
    ABufferGPU * WeightsBufferGPU;
    AIndexedMeshSubpartArray Subparts;
    ALightmapUVChannels LightmapUVs;
    AVertexLightChannels VertexLightChannels;
    TPodArrayHeap< SMeshVertex > Vertices;
    TPodArrayHeap< SMeshVertexJoint > Weights;
    TPodArrayHeap< unsigned int > Indices;
    TPodArray< ASocketDef * > Sockets;
    TRef< ASkeleton > Skeleton;
    ASkin Skin;
    BvAxisAlignedBox BoundingBox;
    uint16_t RaycastPrimitivesPerLeaf;
    bool bSkinnedMesh;
    bool bDynamicStorage;
    mutable bool bBoundingBoxDirty;
};


/*

Utilites


*/

void CreateBoxMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, Float3 const & _Size, float _TexCoordScale );

void CreateSphereMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _NumVerticalSubdivs = 32, int _NumHorizontalSubdivs = 32 );

void CreatePlaneMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale );

void CreatePatchMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds,
    Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs );

void CreateCylinderMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

void CreateConeMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

void CreateCapsuleMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs = 6, int _NumHorizontalSubdivs = 8 );

void CalcTangentSpace( SMeshVertex * _VertexArray, unsigned int _NumVerts, unsigned int const * _IndexArray, unsigned int _NumIndices );

/** binormal = cross( normal, tangent ) * handedness */
AN_FORCEINLINE float CalcHandedness( Float3 const & _Tangent, Float3 const & _Binormal, Float3 const & _Normal ) {
    return ( _Normal.Cross( _Tangent ).Dot( _Binormal ) < 0.0f ) ? -1.0f : 1.0f;
}

AN_FORCEINLINE Float3 CalcBinormal( Float3 const & _Tangent, Float3 const & _Normal, float _Handedness ) {
    return _Normal.Cross( _Tangent ).Normalized() * _Handedness;
}

BvAxisAlignedBox CalcBindposeBounds( SMeshVertex const * InVertices,
                                     SMeshVertexJoint const * InWeights,
                                     int InVertexCount,
                                     ASkin const * InSkin,
                                     SJoint * InJoints,
                                     int InJointsCount );

void CalcBoundingBoxes( SMeshVertex const * InVertices,
                        SMeshVertexJoint const * InWeights,
                        int InVertexCount,
                        ASkin const * InSkin,
                        SJoint const *  InJoints,
                        int InNumJoints,
                        uint32_t FrameCount,
                        struct SAnimationChannel const * InChannels,
                        int InChannelsCount,
                        ATransform const * InTransforms,
                        TPodArray< BvAxisAlignedBox > & Bounds );
