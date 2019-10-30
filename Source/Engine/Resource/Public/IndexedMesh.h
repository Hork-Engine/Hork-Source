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

class FIndexedMesh;

/*

FMaterialTexture

Mesh subpart plain data

*/
struct FSubpart {
    FString Name;
    int BaseVertex;
    int VertexCount;
    int FirstIndex;
    int IndexCount;
    BvAxisAlignedBox BoundingBox;
    int Material;
};

/*

FMaterialTexture

Material texture plain data

*/
struct FMaterialTexture {
    FString FileName;
    // TODO: keep file CRC?
};

/*

FMeshMaterial

Mesh material plain data

*/
struct FMeshMaterial {
    int Textures[MAX_MATERIAL_TEXTURES];
    int NumTextures;
};

/*

FMeshAsset

Mesh plain data

*/
struct FMeshAsset {
    TStdVector< FSubpart > Subparts;
    TStdVector< FMaterialTexture > Textures;
    TPodArray< FMeshMaterial > Materials;
    TPodArrayHeap< FMeshVertex > Vertices;
    TPodArrayHeap< unsigned int > Indices;
    TPodArrayHeap< FMeshVertexJoint > Weights;

    void Clear();
    void Read( FFileStream & f );
    void Write( FFileStream & f );
};


/*

FTriangleHitResult

Raycast hit result

*/
struct FTriangleHitResult {
    Float3 Location;
    Float3 Normal;
    Float2 UV;
    float Distance;
    unsigned int Indices[3];
    FMaterialInstance * Material;
};

/*

FAABBNode

*/
struct FAABBNode {
    BvAxisAlignedBox Bounds;
    int Index;          // First primitive in leaf (Index >= 0), next node index (Index < 0)
    int PrimitiveCount;

    bool IsLeaf() const {
        return Index >= 0;
    }
};

/*

FAABBTree

Binary AABB-based BVH tree

*/
class FAABBTree : public FBaseObject {
    AN_CLASS( FAABBTree, FBaseObject )

public:
    void Initialize( FMeshVertex const * _Vertices, unsigned int const * _Indices, unsigned int _IndexCount, int _BaseVertex, unsigned int _PrimitivesPerLeaf );

    void Purge();

    int MarkRayOverlappingLeafs( Float3 const & _RayStart, Float3 const & _RayEnd, unsigned int * _MarkLeafs, int _MaxLeafs ) const;

    int MarkBoxOverlappingLeafs( BvAxisAlignedBox const & _Bounds, unsigned int * _MarkLeafs, int _MaxLeafs ) const;

    TPodArray< FAABBNode > const & GetNodes() const { return Nodes; }

    unsigned int const * GetIndirection() const { return Indirection.ToPtr(); }

    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

protected:
    FAABBTree() {}
    ~FAABBTree() {}

private:
    void Subdivide( struct FAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _MaxPrimitive, unsigned int _PrimitivesPerLeaf,
        int & _PrimitiveIndex, const unsigned int * _Indices );

    TPodArray< FAABBNode > Nodes;
    TPodArray< unsigned int > Indirection;
    BvAxisAlignedBox BoundingBox;
};

/*

FIndexedMeshSubpart

Part of indexed mesh (submesh / element)

*/
class FIndexedMeshSubpart : public FBaseObject {
    AN_CLASS( FIndexedMeshSubpart, FBaseObject )

    friend class FIndexedMesh;

public:
    void SetBaseVertex( int _BaseVertex );
    void SetFirstIndex( int _FirstIndex );
    void SetVertexCount( int _VertexCount );
    void SetIndexCount( int _IndexCount );
    void SetMaterialInstance( FMaterialInstance * _MaterialInstance );

    int GetBaseVertex() const { return BaseVertex; }
    int GetFirstIndex() const { return FirstIndex; }
    int GetVertexCount() const { return VertexCount; }
    int GetIndexCount() const { return IndexCount; }
    FMaterialInstance * GetMaterialInstance() { return MaterialInstance; }

    void SetBoundingBox( BvAxisAlignedBox const & _BoundingBox );

    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

    FIndexedMesh * GetOwner() { return OwnerMesh; }

    // Check ray intersection. Result is unordered by distance to save performance
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const;

    // Check ray intersection
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const;

    void DrawBVH( FDebugDraw * _DebugDraw );

protected:
    FIndexedMeshSubpart();
    ~FIndexedMeshSubpart();

private:
    void CreateBVH();

    FIndexedMesh * OwnerMesh;
    BvAxisAlignedBox BoundingBox;
    int BaseVertex;
    int FirstIndex;
    int VertexCount;
    int IndexCount;
    TRef< FMaterialInstance > MaterialInstance;
    TRef< FAABBTree > AABBTree;
    bool bAABBTreeDirty;
};

/*

FLightmapUV

Lightmap UV channel

*/
class FLightmapUV : public FBaseObject, public IGPUResourceOwner {
    AN_CLASS( FLightmapUV, FBaseObject )

    friend class FIndexedMesh;

public:
    FMeshLightmapUV * GetVertices() { return Vertices.ToPtr(); }
    FMeshLightmapUV const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( FMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    FBufferGPU * GetGPUResource() { return VertexBufferGPU; }

    FIndexedMesh * GetOwner() { return OwnerMesh; }

protected:
    FLightmapUV();
    ~FLightmapUV();

    void OnInitialize( int _NumVertices );

    // IGPUResourceOwner interface
    void UploadResourceGPU( FResourceGPU * _Resource ) override {}

private:
    FBufferGPU * VertexBufferGPU;
    FIndexedMesh * OwnerMesh;
    int IndexInArrayOfUVs = -1;
    TPodArrayHeap< FMeshLightmapUV > Vertices;
    bool bDynamicStorage;
};

/*

FVertexLight

Vertex light channel

*/
class FVertexLight : public FBaseObject, public IGPUResourceOwner {
    AN_CLASS( FVertexLight, FBaseObject )

    friend class FIndexedMesh;

public:
    FMeshVertexLight * GetVertices() { return Vertices.ToPtr(); }
    FMeshVertexLight const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( FMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    FBufferGPU * GetGPUResource() { return VertexBufferGPU; }

    FIndexedMesh * GetOwner() { return OwnerMesh; }

protected:
    FVertexLight();
    ~FVertexLight();

    void OnInitialize( int _NumVertices );

    // IGPUResourceOwner interface
    void UploadResourceGPU( FResourceGPU * _Resource ) override {}

private:
    FBufferGPU * VertexBufferGPU;
    FIndexedMesh * OwnerMesh;
    int IndexInArrayOfChannels = -1;
    TPodArrayHeap< FMeshVertexLight > Vertices;
    bool bDynamicStorage;
};

using FLightmapUVChannels = TPodArray< FLightmapUV *, 1 >;
using FVertexLightChannels = TPodArray< FVertexLight *, 1 >;
using FIndexedMeshSubpartArray = TPodArray< FIndexedMeshSubpart *, 1 >;

struct FSoftbodyLink {
    unsigned int Indices[2];
};

struct FSoftbodyFace {
    unsigned int Indices[3];
};

/*

FIndexedMesh

Triangulated 3d surface with indexed vertices

*/
class FIndexedMesh : public FBaseObject, public IGPUResourceOwner {
    AN_CLASS( FIndexedMesh, FBaseObject )

    friend class FLightmapUV;
    friend class FVertexLight;
    friend class FIndexedMeshSubpart;

public:
    // Rigid body collision model
    FCollisionBodyComposition BodyComposition;

    // Soft body collision model
    TPodArray< FSoftbodyLink > SoftbodyLinks;
    TPodArray< FSoftbodyFace > SoftbodyFaces;

    // Allocate mesh
    void Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh = false, bool _DynamicStorage = false );

    // Helper. Create box mesh
    void InitializeBoxMesh( Float3 const & _Size, float _TexCoordScale );

    // Helper. Create sphere mesh
    void InitializeSphereMesh( float _Radius, float _TexCoordScale, int _NumVerticalSubdivs = 32, int _NumHorizontalSubdivs = 32 );

    // Helper. Create plane mesh
    void InitializePlaneMesh( float _Width, float _Height, float _TexCoordScale );

    // Helper. Create patch mesh
    void InitializePatchMesh( Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs );

    // Helper. Create cylinder mesh
    void InitializeCylinderMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

    // Helper. Create cone mesh
    void InitializeConeMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

    // Helper. Create capsule mesh
    void InitializeCapsuleMesh( float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs = 6, int _NumHorizontalSubdivs = 8 );

    // Create mesh from string (IndexedMesh.***)
    void InitializeInternalResource( const char * _InternalResourceName ) override;

    // Initialize object from file
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    // Purge model data
    void Purge();

    // Skinned mesh have 4 weights for each vertex
    bool IsSkinned() const { return bSkinnedMesh; }

    // Dynamic storage is the mesh that updates every or almost every frame
    bool IsDynamicStorage() const { return bDynamicStorage; }

    // Get mesh part
    FIndexedMeshSubpart * GetSubpart( int _SubpartIndex );

    // Create lightmap channel to store lighmap UVs
    FLightmapUV * CreateLightmapUVChannel();

    // Create vertex light channel to store light colors
    FVertexLight * CreateVertexLightChannel();

    // Create BVH for raycast optimization
    void CreateBVH();

    void SetMaterialInstance( int _SubpartIndex, FMaterialInstance * _MaterialInstance );

    void SetBoundingBox( int _SubpartIndex, BvAxisAlignedBox const & _BoundingBox );

    // Get mesh vertices
    FMeshVertex * GetVertices() { return Vertices.ToPtr(); }
    FMeshVertex const * GetVertices() const { return Vertices.ToPtr(); }

    // Get weights for vertex skinning
    FMeshVertexJoint * GetWeights() { return Weights.ToPtr(); }
    FMeshVertexJoint const * GetWeights() const { return Weights.ToPtr(); }

    // Get mesh indices
    unsigned int * GetIndices() { return Indices.ToPtr(); }
    unsigned int const * GetIndices() const { return Indices.ToPtr(); }

    // Get total vertex count
    int GetVertexCount() const { return Vertices.Size(); }

    // Get total index count
    int GetIndexCount() const { return Indices.Size(); }

    // Get all mesh subparts
    FIndexedMeshSubpartArray const & GetSubparts() const { return Subparts; }

    // Get all lightmap channels
    FLightmapUVChannels const & GetLightmapUVChannels() const { return LightmapUVs; }

    // Get all vertex light channels
    FVertexLightChannels const & GetVertexLightChannels() const { return VertexLightChannels; }

    // Write vertices at location and send them to GPU
    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );

    // Write vertices at location and send them to GPU
    bool WriteVertexData( FMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    // Write joint weights at location and send them to GPU
    bool SendJointWeightsToGPU( int _VerticesCount, int _StartVertexLocation );

    // Write joint weights at location and send them to GPU
    bool WriteJointWeights( FMeshVertexJoint const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    // Write indices at location and send them to GPU
    bool SendIndexDataToGPU( int _IndexCount, int _StartIndexLocation );

    // Write indices at location and send them to GPU
    bool WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation );

    void UpdateBoundingBox();

    BvAxisAlignedBox const & GetBoundingBox() const;

    // Get mesh GPU buffers
    FBufferGPU * GetVertexBufferGPU() { return VertexBufferGPU; }
    FBufferGPU * GetIndexBufferGPU() { return IndexBufferGPU; }
    FBufferGPU * GetWeightsBufferGPU() { return WeightsBufferGPU; }

    // Check ray intersection. Result is unordered by distance to save performance
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const;

    // Check ray intersection
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], TRef< FMaterialInstance > & _Material ) const;

    void GenerateSoftbodyFacesFromMeshIndices();

    void GenerateSoftbodyLinksFromFaces();

    void DrawDebug( FDebugDraw * _DebugDraw );

protected:
    FIndexedMesh();
    ~FIndexedMesh();

    // IGPUResourceOwner interface
    void UploadResourceGPU( FResourceGPU * _Resource ) override {}

private:
    FBufferGPU * VertexBufferGPU;
    FBufferGPU * IndexBufferGPU;
    FBufferGPU * WeightsBufferGPU;
    FIndexedMeshSubpartArray Subparts;
    FLightmapUVChannels LightmapUVs;
    FVertexLightChannels VertexLightChannels;
    TPodArrayHeap< FMeshVertex > Vertices;
    TPodArrayHeap< FMeshVertexJoint > Weights;
    TPodArrayHeap< unsigned int > Indices;
    bool bSkinnedMesh;
    bool bDynamicStorage;
    mutable bool bBoundingBoxDirty;
    BvAxisAlignedBox BoundingBox;
};


/*

Utilites


*/

void CreateBoxMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, Float3 const & _Size, float _TexCoordScale );

void CreateSphereMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _NumVerticalSubdivs = 32, int _NumHorizontalSubdivs = 32 );

void CreatePlaneMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale );

void CreatePatchMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds,
    Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs );

void CreateCylinderMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

void CreateConeMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs = 32 );

void CreateCapsuleMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs = 6, int _NumHorizontalSubdivs = 8 );

void CalcTangentSpace( FMeshVertex * _VertexArray, unsigned int _NumVerts, unsigned int const * _IndexArray, unsigned int _NumIndices );

// binormal = cross( normal, tangent ) * handedness
AN_FORCEINLINE float CalcHandedness( Float3 const & _Tangent, Float3 const & _Binormal, Float3 const & _Normal ) {
    return ( _Normal.Cross( _Tangent ).Dot( _Binormal ) < 0.0f ) ? -1.0f : 1.0f;
}

AN_FORCEINLINE Float3 CalcBinormal( Float3 const & _Tangent, Float3 const & _Normal, float _Handedness ) {
    return _Normal.Cross( _Tangent ).Normalized() * _Handedness;
}
