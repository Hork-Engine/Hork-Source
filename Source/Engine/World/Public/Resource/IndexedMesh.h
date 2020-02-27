/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Base/BaseObject.h>
#include <World/Public/Base/DebugRenderer.h>
#include <World/Public/Level.h>
#include "Material.h"
#include "CollisionBody.h"
#include "Skeleton.h"

class AIndexedMesh;
class ALevel;
struct SVertexHandle;

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
    void InitializeTriangleSoup( SMeshVertex const * _Vertices, unsigned int const * _Indices, unsigned int _IndexCount, int _BaseVertex, unsigned int _PrimitivesPerLeaf );

    void InitializePrimitiveSoup( SPrimitiveDef const * _Primitives, unsigned int _PrimitiveCount, unsigned int _PrimitivesPerLeaf );

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
    void Subdivide( struct SAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _MaxPrimitive, unsigned int _PrimitivesPerLeaf, int & _PrimitiveIndex );

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
    void SetBoundingBox( BvAxisAlignedBox const & _BoundingBox );

    int GetBaseVertex() const { return BaseVertex; }
    int GetFirstIndex() const { return FirstIndex; }
    int GetVertexCount() const { return VertexCount; }
    int GetIndexCount() const { return IndexCount; }
    AMaterialInstance * GetMaterialInstance() { return MaterialInstance; }
    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

    AIndexedMesh * GetOwner() { return OwnerMesh; }

    void GenerateBVH( unsigned int PrimitivesPerLeaf = 16 );

    void SetBVH( ATreeAABB * BVH );

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _InvRayDir, float _Distance, TPodArray< STriangleHitResult > & _HitResult ) const;

    /** Check ray intersection */
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _InvRayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const;

    void DrawBVH( ADebugRenderer * InRenderer, Float3x4 const & _TransformMatrix );

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
class ALightmapUV : public ABaseObject {
    AN_CLASS( ALightmapUV, ABaseObject )

    friend class AIndexedMesh;

public:
    void Initialize( AIndexedMesh * InSourceMesh, ALevel * InLightingLevel );
    void Purge();

    SMeshVertexUV * GetVertices() { return Vertices.ToPtr(); }
    SMeshVertexUV const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( SMeshVertexUV const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    void GetVertexBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );

    AIndexedMesh * GetSourceMesh() { return SourceMesh; }

    ALevel * GetLightingLevel() { return LightingLevel; }

protected:
    ALightmapUV();
    ~ALightmapUV();

    void Invalidate() { bInvalid = true; }

private:
    static void * GetVertexMemory( void * _This );

    SVertexHandle * VertexBufferGPU;
    TRef< AIndexedMesh > SourceMesh;
    TWeakRef< ALevel > LightingLevel;
    int IndexInArrayOfUVs = -1;
    TPodArrayHeap< SMeshVertexUV > Vertices;
    bool bInvalid;
};

/**

AVertexLight

Vertex light channel

*/
class AVertexLight : public ABaseObject {
    AN_CLASS( AVertexLight, ABaseObject )

    friend class AIndexedMesh;

public:
    void Initialize( AIndexedMesh * InSourceMesh, ALevel * InLightingLevel );
    void Purge();

    SMeshVertexLight * GetVertices() { return Vertices.ToPtr(); }
    SMeshVertexLight const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( SMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    void GetVertexBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );

    AIndexedMesh * GetSourceMesh() { return SourceMesh; }

    ALevel * GetLightingLevel() { return LightingLevel; }

protected:
    AVertexLight();
    ~AVertexLight();

    void Invalidate() { bInvalid = true; }

private:
    static void * GetVertexMemory( void * _This );

    SVertexHandle * VertexBufferGPU;
    TRef< AIndexedMesh > SourceMesh;
    TWeakRef< ALevel > LightingLevel;
    int IndexInArrayOfChannels = -1;
    TPodArrayHeap< SMeshVertexLight > Vertices;
    bool bInvalid;
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

Triangulated 3d surfaces with indexed vertices

*/
class AIndexedMesh : public AResource {
    AN_CLASS( AIndexedMesh, AResource )

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
    void Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh = false );

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

    /** Helper. Create skybox mesh */
    void InitializeSkyboxMesh( Float3 const & _Size, float _TexCoordScale );

    /** Helper. Create skydome mesh */
    void InitializeSkydomeMesh( float _Radius, float _TexCoordScale, int _NumVerticalSubdivs = 32, int _NumHorizontalSubdivs = 32, bool _Hemisphere = true );

    /** Purge model data */
    void Purge();

    /** Skinned mesh have 4 weights for each vertex */
    bool IsSkinned() const { return bSkinnedMesh; }

    /** Get mesh part */
    AIndexedMeshSubpart * GetSubpart( int _SubpartIndex );

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
    SMeshVertexSkin * GetWeights() { return Weights.ToPtr(); }

    /** Get weights for vertex skinning */
    SMeshVertexSkin const * GetWeights() const { return Weights.ToPtr(); }

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

    /** Get all lightmap channels for the mesh */
    ALightmapUVChannels const & GetLightmapUVChannels() const { return LightmapUVs; }

    /** Get all vertex light channels for the mesh */
    AVertexLightChannels const & GetVertexLightChannels() const { return VertexLightChannels; }

    /** Write vertices at location and send them to GPU */
    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );

    /** Write vertices at location and send them to GPU */
    bool WriteVertexData( SMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    /** Write joint weights at location and send them to GPU */
    bool SendJointWeightsToGPU( int _VerticesCount, int _StartVertexLocation );

    /** Write joint weights at location and send them to GPU */
    bool WriteJointWeights( SMeshVertexSkin const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    /** Write indices at location and send them to GPU */
    bool SendIndexDataToGPU( int _IndexCount, int _StartIndexLocation );

    /** Write indices at location and send them to GPU */
    bool WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation );

    void UpdateBoundingBox();

    BvAxisAlignedBox const & GetBoundingBox() const;

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );
    void GetIndexBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );
    void GetWeightsBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< STriangleHitResult > & _HitResult ) const;

    /** Check ray intersection */
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], int & _SubpartIndex ) const;

    /** Create BVH for raycast optimization */
    void GenerateBVH( unsigned int PrimitivesPerLeaf = 16 );

    /** Generate static collisions */
    void GenerateRigidbodyCollisions();

    void GenerateSoftbodyFacesFromMeshIndices();

    void GenerateSoftbodyLinksFromFaces();

    void DrawBVH( ADebugRenderer * InRenderer, Float3x4 const & _TransformMatrix );

protected:
    AIndexedMesh();
    ~AIndexedMesh();

    /** Load resource from file */
    bool LoadResource( AString const & _Path ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/Meshes/Box"; }

private:
    void InvalidateChannels();

    static void * GetVertexMemory( void * _This );
    static void * GetIndexMemory( void * _This );
    static void * GetWeightMemory( void * _This );

    SVertexHandle * VertexHandle;
    SVertexHandle * IndexHandle;
    SVertexHandle * WeightsHandle;
    AIndexedMeshSubpartArray Subparts;
    ALightmapUVChannels LightmapUVs;
    AVertexLightChannels VertexLightChannels;
    TPodArrayHeap< SMeshVertex > Vertices;
    TPodArrayHeap< SMeshVertexSkin > Weights;
    TPodArrayHeap< unsigned int > Indices;
    TPodArray< ASocketDef * > Sockets;
    TRef< ASkeleton > Skeleton;
    ASkin Skin;
    BvAxisAlignedBox BoundingBox;
    uint16_t RaycastPrimitivesPerLeaf;
    bool bSkinnedMesh;
    mutable bool bBoundingBoxDirty;
};


/**

AProceduralMesh

Runtime-generated procedural mesh.

*/
class AProceduralMesh : public ABaseObject {
    AN_CLASS( AProceduralMesh, ABaseObject )

public:
    /** Update vertex cache occasionally or every frame */
    TPodArrayHeap< SMeshVertex, 32, 32, 16 > VertexCache;

    /** Update index cache occasionally or every frame */
    TPodArrayHeap< unsigned int, 32, 32, 16 > IndexCache;

    /** Bounding box is used for raycast early exit and VSD culling */
    BvAxisAlignedBox BoundingBox;

    /** Get mesh GPU buffers */
    void GetVertexBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );

    /** Get mesh GPU buffers */
    void GetIndexBufferGPU( ABufferGPU ** _Buffer, size_t * _Offset );

    /** Check ray intersection. Result is unordered by distance to save performance */
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< STriangleHitResult > & _HitResult ) const;

    /** Check ray intersection */
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const;

    /** Called before rendering. Don't call directly. */
    void PreRenderUpdate( SRenderFrontendDef const * _Def );

    // TODO: Add methods like AddTriangle, AddQuad, etc.

protected:
    AProceduralMesh();
    ~AProceduralMesh();

private:
    size_t VertexStream;
    size_t IndexSteam;

    int VisFrame;
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

void CreateSkyboxMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, Float3 const & _Size, float _TexCoordScale );

void CreateSkydomeMesh( TPodArray< SMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _NumVerticalSubdivs = 32, int _NumHorizontalSubdivs = 32, bool _Hemisphere = true );

void CalcTangentSpace( SMeshVertex * _VertexArray, unsigned int _NumVerts, unsigned int const * _IndexArray, unsigned int _NumIndices );

/** binormal = cross( normal, tangent ) * handedness */
AN_FORCEINLINE float CalcHandedness( Float3 const & _Tangent, Float3 const & _Binormal, Float3 const & _Normal ) {
    return ( Math::Dot( Math::Cross( _Normal, _Tangent ), _Binormal ) < 0.0f ) ? -1.0f : 1.0f;
}

AN_FORCEINLINE Float3 CalcBinormal( Float3 const & _Tangent, Float3 const & _Normal, float _Handedness ) {
    return Math::Cross( _Normal, _Tangent ).Normalized() * _Handedness;
}

BvAxisAlignedBox CalcBindposeBounds( SMeshVertex const * InVertices,
                                     SMeshVertexSkin const * InWeights,
                                     int InVertexCount,
                                     ASkin const * InSkin,
                                     SJoint * InJoints,
                                     int InJointsCount );

void CalcBoundingBoxes( SMeshVertex const * InVertices,
                        SMeshVertexSkin const * InWeights,
                        int InVertexCount,
                        ASkin const * InSkin,
                        SJoint const *  InJoints,
                        int InNumJoints,
                        uint32_t FrameCount,
                        struct SAnimationChannel const * InChannels,
                        int InChannelsCount,
                        STransform const * InTransforms,
                        TPodArray< BvAxisAlignedBox > & Bounds );
