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

#include "MeshBase.h"
#include "Shape.h"

class FIndexedMesh;

class FIndexedMeshSubpart : public FBaseObject {
    AN_CLASS( FIndexedMeshSubpart, FBaseObject )

    friend class FIndexedMesh;

public:
    int FirstVertex;
    int VertexCount;
    int FirstIndex;
    int IndexCount;

    BvAxisAlignedBox BoundingBox;
    //TRefHolder< FMaterialInstance > Material;

    FIndexedMesh * GetParent() { return ParentMesh; }

protected:
    FIndexedMeshSubpart();
    ~FIndexedMeshSubpart();

private:
    FIndexedMesh * ParentMesh;
    int IndexInArrayOfSubparts = -1;
};

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
};

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
};

using FLightmapUVChannels = TPodArray< FLightmapUV *, 1 >;
using FVertexLightChannels = TPodArray< FVertexLight *, 1 >;
using FIndexedMeshSubpartArray = TPodArray< FIndexedMeshSubpart *, 1 >;

class FIndexedMesh : public FBaseObject, public IRenderProxyOwner {
    AN_CLASS( FIndexedMesh, FBaseObject )

    friend class FLightmapUV;
    friend class FVertexLight;
    friend class FIndexedMeshSubpart;

public:
    // Allocate mesh
    void Initialize( int _NumVertices, int _NumIndices );

    // Allocate mesh and create base shape
    template< typename _Shape, class... _Valty >
    void InitializeShape( _Valty &&... _Val );

    // Create mesh from string (*box* *sphere* *cylinder* *plane*)
    void InitializeInternalMesh( const char * _Name );

    // Create mesh part
    FIndexedMeshSubpart * CreateSubpart( FString const & _Name, int _FirstVertex, int _VertexCount, int _FirstIndex, int _IndexCount );

    // Get persistent subpart
    FIndexedMeshSubpart * GetPersistentSubpart() { return Subparts[0]; }

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

    // Write indices at location and send them to GPU
    unsigned int * WriteIndexData( int _IndexCount, int _StartIndexLocation );

    // Write indices at location and send them to GPU
    bool WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation );

    // Get mesh rendering proxy
    FRenderProxy_IndexedMesh * GetRenderProxy() { return RenderProxy; }

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
};

template< typename _Shape, class... _Valty >
void FIndexedMesh::InitializeShape( _Valty &&... _Val ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;

    _Shape::CreateMesh( vertices, indices, GetPersistentSubpart()->BoundingBox, std::forward< _Valty >( _Val )... );

    Initialize( vertices.Length(), indices.Length() );
    WriteVertexData( vertices.ToPtr(), vertices.Length(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Length(), 0 );
}
