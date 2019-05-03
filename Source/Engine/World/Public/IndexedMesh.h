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

#include "BaseObject.h"
#include "Shape.h"
#include "MaterialAssembly.h"
#include "CollisionBody.h"

class FIndexedMesh;

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

    BvAxisAlignedBox BoundingBox;
    TRefHolder< FMaterialInstance > MaterialInstance;

    FIndexedMesh * GetParent() { return ParentMesh; }

protected:
    FIndexedMeshSubpart();
    ~FIndexedMeshSubpart();

private:
    FIndexedMesh * ParentMesh;
    //int IndexInArrayOfSubparts = -1;
};

/*

FLightmapUV

Lightmap UV channel

*/
class FLightmapUV : public FBaseObject, public IRenderProxyOwner {
    AN_CLASS( FLightmapUV, FBaseObject )

    friend class FIndexedMesh;

public:
    int GetVertexCount() const { return VertexCount; }

    FMeshLightmapUV * WriteVertexData( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( FMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    FRenderProxy_LightmapUVChannel * GetRenderProxy() { return RenderProxy; }

    FIndexedMesh * GetParent() { return ParentMesh; }

protected:
    FLightmapUV();
    ~FLightmapUV();

    void OnInitialize( int _NumVertices );

private:
    FRenderProxy_LightmapUVChannel * RenderProxy;
    FIndexedMesh * ParentMesh;
    int IndexInArrayOfUVs = -1;

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
    int GetVertexCount() const { return VertexCount; }

    FMeshVertexLight * WriteVertexData( int _VerticesCount, int _StartVertexLocation );
    bool WriteVertexData( FMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    FRenderProxy_VertexLightChannel * GetRenderProxy() { return RenderProxy; }

    FIndexedMesh * GetParent() { return ParentMesh; }

protected:
    FVertexLight();
    ~FVertexLight();

    void OnInitialize( int _NumVertices );

private:
    FRenderProxy_VertexLightChannel * RenderProxy;
    FIndexedMesh * ParentMesh;
    int IndexInArrayOfChannels = -1;

    int VertexCount;
    bool bDynamicStorage;
};

using FLightmapUVChannels = TPodArray< FLightmapUV *, 1 >;
using FVertexLightChannels = TPodArray< FVertexLight *, 1 >;
using FIndexedMeshSubpartArray = TPodArray< FIndexedMeshSubpart *, 1 >;

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
    FMeshVertex * WriteVertexData( int _VerticesCount, int _StartVertexLocation );

    // Write vertices at location and send them to GPU
    bool WriteVertexData( FMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    // Write joint weights at location and send them to GPU
    FMeshVertexJoint * WriteJointWeights( int _VerticesCount, int _StartVertexLocation );

    // Write joint weights at location and send them to GPU
    bool WriteJointWeights( FMeshVertexJoint const * _Vertices, int _VerticesCount, int _StartVertexLocation );

    // Write indices at location and send them to GPU
    unsigned int * WriteIndexData( int _IndexCount, int _StartIndexLocation );

    // Write indices at location and send them to GPU
    bool WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation );

    // Get mesh rendering proxy
    FRenderProxy_IndexedMesh * GetRenderProxy() { return RenderProxy; }

    FCollisionBodyComposition BodyComposition;

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
    int VertexCount;
    int IndexCount;
    bool bSkinnedMesh;
    bool bDynamicStorage;
};

template< typename _Shape, class... _Valty >
void FIndexedMesh::InitializeShape( _Valty &&... _Val ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    _Shape::CreateMesh( vertices, indices, bounds, std::forward< _Valty >( _Val )... );

    Initialize( vertices.Length(), indices.Length(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Length(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Length(), 0 );

    Subparts[0]->BoundingBox = bounds;
}
