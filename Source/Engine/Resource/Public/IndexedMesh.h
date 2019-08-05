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
#include "Shape.h"
#include "MaterialAssembly.h"
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
    TVector< FSubpart > Subparts;
    TVector< FMaterialTexture > Textures;
    TPodArray< FMeshMaterial > Materials;
    TPodArray< FMeshVertex > Vertices;
    TPodArray< unsigned int > Indices;
    TPodArray< FMeshVertexJoint > Weights;

    void Clear();
    void Read( FFileStream & f );
    void Write( FFileStream & f );
};


/*

FTriangleHitResult

Raycast hit result

*/
struct FTriangleHitResult {
    Float3 HitLocation;
    Float3 HitNormal;
    Float2 HitUV;
    float HitDistance;
    unsigned int Indices[3];
    FMaterialInstance * Material;
};

/*

FIndexedMeshSubpart

Part of indexed mesh (submesh / element)

*/
class FIndexedMeshSubpart : public FBaseObject {
    AN_CLASS( FIndexedMeshSubpart, FBaseObject )

    friend class FIndexedMesh;

public:
    int BaseVertex;
    int FirstIndex;
    int VertexCount;
    int IndexCount;
    TRef< FMaterialInstance > MaterialInstance;

    void SetBoundingBox( BvAxisAlignedBox const & _BoundingBox );

    BvAxisAlignedBox const & GetBoundingBox() const { return BoundingBox; }

    FIndexedMesh * GetOwner() { return OwnerMesh; }

    // Check ray intersection. Result is unordered by distance to save performance
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const;

    // Check ray intersection
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const;

protected:
    FIndexedMeshSubpart();
    ~FIndexedMeshSubpart();

private:
    FIndexedMesh * OwnerMesh;
    BvAxisAlignedBox BoundingBox;
};

/*

FLightmapUV

Lightmap UV channel

*/
class FLightmapUV : public FBaseObject, public IRenderProxyOwner {
    AN_CLASS( FLightmapUV, FBaseObject )

    friend class FIndexedMesh;

public:
    FMeshLightmapUV * GetVertices() { return Vertices.ToPtr(); }
    FMeshLightmapUV const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return VertexCount; }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( FMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    FRenderProxy_LightmapUVChannel * GetRenderProxy() { return RenderProxy; }

    FIndexedMesh * GetOwner() { return OwnerMesh; }

protected:
    FLightmapUV();
    ~FLightmapUV();

    void OnInitialize( int _NumVertices );

private:
    FRenderProxy_LightmapUVChannel * RenderProxy;
    FIndexedMesh * OwnerMesh;
    int IndexInArrayOfUVs = -1;
    TPodArray< FMeshLightmapUV > Vertices;
    int VertexCount;
    bool bDynamicStorage;
};

/*

FVertexLight

Vertex light channel

*/
class FVertexLight : public FBaseObject, public IRenderProxyOwner {
    AN_CLASS( FVertexLight, FBaseObject )

    friend class FIndexedMesh;

public:
    FMeshVertexLight * GetVertices() { return Vertices.ToPtr(); }
    FMeshVertexLight const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return VertexCount; }

    bool SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( FMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    FRenderProxy_VertexLightChannel * GetRenderProxy() { return RenderProxy; }

    FIndexedMesh * GetOwner() { return OwnerMesh; }

protected:
    FVertexLight();
    ~FVertexLight();

    void OnInitialize( int _NumVertices );

private:
    FRenderProxy_VertexLightChannel * RenderProxy;
    FIndexedMesh * OwnerMesh;
    int IndexInArrayOfChannels = -1;
    TPodArray< FMeshVertexLight > Vertices;
    int VertexCount;
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
class FIndexedMesh : public FBaseObject, public IRenderProxyOwner {
    AN_CLASS( FIndexedMesh, FBaseObject )

    friend class FLightmapUV;
    friend class FVertexLight;
    friend class FIndexedMeshSubpart;

public:
    // Allocate mesh
    void Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh = false, bool _DynamicStorage = false );

    // Allocate mesh and create base shape
    template< typename _Shape, class... _Valty >
    void InitializeShape( _Valty &&... _Val );

    // Create mesh from string (*box* *sphere* *cylinder* *plane*)
    void InitializeInternalMesh( const char * _Name );

    // Initialize default object representation
    void InitializeDefaultObject() override;

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
    int GetVertexCount() const { return VertexCount; }

    // Get total index count
    int GetIndexCount() const { return IndexCount; }

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

    // Get mesh rendering proxy
    FRenderProxy_IndexedMesh * GetRenderProxy() { return RenderProxy; }

    // Check ray intersection. Result is unordered by distance to save performance
    bool Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const;

    // Check ray intersection
    bool RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], TRef< FMaterialInstance > & _Material ) const;

    // Rigid body collision model
    FCollisionBodyComposition BodyComposition;

    // Soft body collision model
    TPodArray< FSoftbodyLink > SoftbodyLinks;
    TPodArray< FSoftbodyFace > SoftbodyFaces;

    void GenerateSoftbodyFacesFromMeshIndices();
    void GenerateSoftbodyLinksFromFaces();

protected:
    FIndexedMesh();
    ~FIndexedMesh();

    // IRenderProxyOwner interface
    void OnLost() override { /* ... */ }

private:
    FRenderProxy_IndexedMesh * RenderProxy;
    FIndexedMeshSubpartArray Subparts;
    FLightmapUVChannels LightmapUVs;
    FVertexLightChannels VertexLightChannels;
    TPodArray< FMeshVertex > Vertices;
    TPodArray< FMeshVertexJoint > Weights;
    TPodArray< unsigned int > Indices;
    int VertexCount;
    int IndexCount;
    bool bSkinnedMesh;
    bool bDynamicStorage;
    mutable bool bBoundingBoxDirty;
    BvAxisAlignedBox BoundingBox;
};

template< typename _Shape, class... _Valty >
void FIndexedMesh::InitializeShape( _Valty &&... _Val ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    _Shape::CreateMesh( vertices, indices, bounds, std::forward< _Valty >( _Val )... );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[0]->BoundingBox = bounds;
}
