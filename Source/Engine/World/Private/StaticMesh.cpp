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

#include <Engine/World/Public/StaticMesh.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META_NO_ATTRIBS( FIndexedMesh )
AN_CLASS_META_NO_ATTRIBS( FIndexedMeshSubpart )
AN_CLASS_META_NO_ATTRIBS( FLightmapUV )
AN_CLASS_META_NO_ATTRIBS( FVertexLight )

FIndexedMesh::FIndexedMesh() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_IndexedMesh >();
    RenderProxy->SetOwner( this );

    FIndexedMeshSubpart * persistentSubpart = CreateSubpart( "Persistent", 0, 0, 0, 0 );
    persistentSubpart->AddRef();
}

FIndexedMesh::~FIndexedMesh() {
    RenderProxy->KillProxy();

    FIndexedMeshSubpart * persistentSubpart = Subparts[0];
    persistentSubpart->RemoveRef();

    for ( FIndexedMeshSubpart * subpart : Subparts ) {
        subpart->ParentMesh = nullptr;
        subpart->IndexInArrayOfSubparts = -1;
    }

    for ( FLightmapUV * channel : LightmapUVs ) {
        channel->ParentMesh = nullptr;
        channel->IndexInArrayOfUVs = -1;
    }

    for ( FVertexLight * channel : VertexLightChannels ) {
        channel->ParentMesh = nullptr;
        channel->IndexInArrayOfChannels = -1;
    }
}

void FIndexedMesh::Initialize( int _NumVertices, int _NumIndices ) {
    if ( VertexCount == _NumVertices && IndexCount == _NumIndices ) {
        return;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    VertexCount = _NumVertices;
    IndexCount = _NumIndices;

    FIndexedMeshSubpart * persistentSubpart = Subparts[0];
    persistentSubpart->VertexCount = VertexCount;
    persistentSubpart->IndexCount = IndexCount;

    data.VerticesCount = _NumVertices;
    data.IndicesCount = _NumIndices;

    data.VertexType = VERT_MESHVERTEX;
    data.IndexType = INDEX_UINT32;

    if ( data.VertexChunks ) {
        data.VertexMapRange[1] = FMath::Min( data.VertexMapRange[1], _NumVertices );

        if ( data.VertexMapRange[0] >= data.VertexMapRange[1] ) {
            data.VertexChunks = nullptr;
        }
    }

    if ( data.IndexChunks ) {
        data.IndexMapRange[1] = FMath::Min( data.IndexMapRange[1], _NumIndices );

        if ( data.IndexMapRange[0] >= data.IndexMapRange[1] ) {
            data.IndexChunks = nullptr;
        }
    }

    data.bReallocated = true;

    RenderProxy->MarkUpdated();

    for ( FLightmapUV * channel : LightmapUVs ) {
        channel->OnInitialize( _NumVertices );
    }

    for ( FVertexLight * channel : VertexLightChannels ) {
        channel->OnInitialize( _NumVertices );
    }
}

FLightmapUV * FIndexedMesh::CreateLightmapUVChannel() {
    FLightmapUV * channel = NewObject< FLightmapUV >();

    channel->ParentMesh = this;
    channel->IndexInArrayOfUVs = LightmapUVs.Length();

    LightmapUVs.Append( channel );

    channel->OnInitialize( VertexCount );

    return channel;
}

FVertexLight * FIndexedMesh::CreateVertexLightChannel() {
    FVertexLight * channel = NewObject< FVertexLight >();

    channel->ParentMesh = this;
    channel->IndexInArrayOfChannels = VertexLightChannels.Length();

    VertexLightChannels.Append( channel );

    channel->OnInitialize( VertexCount );

    return channel;
}

FIndexedMeshSubpart * FIndexedMesh::CreateSubpart( FString const & _Name, int _FirstVertex, int _VertexCount, int _FirstIndex, int _IndexCount ) {
    FIndexedMeshSubpart * subpart = NewObject< FIndexedMeshSubpart >();

    subpart->SetName( _Name );
    subpart->FirstVertex = _FirstVertex;
    subpart->VertexCount = _VertexCount;
    subpart->FirstIndex = _FirstIndex;
    subpart->IndexCount = _IndexCount;

    subpart->ParentMesh = this;
    subpart->IndexInArrayOfSubparts = Subparts.Length();

    Subparts.Append( subpart );

    return subpart;
}

FMeshVertex * FIndexedMesh::WriteVertexData( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteVertexData: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    FVertexChunk * chunk = ( FVertexChunk * )frameData->AllocFrameData( sizeof( FVertexChunk ) + sizeof( FMeshVertex ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    data.VertexType = VERT_MESHVERTEX;

    if ( !data.VertexChunks ) {
        data.VertexMapRange[0] = _StartVertexLocation;
        data.VertexMapRange[1] = _StartVertexLocation + _VerticesCount;
    } else {
        data.VertexMapRange[0] = FMath::Min( data.VertexMapRange[0], _StartVertexLocation );
        data.VertexMapRange[1] = FMath::Max( data.VertexMapRange[1], _StartVertexLocation + _VerticesCount );
    }

    IntrusiveAddToList( chunk, Next, Prev, data.VertexChunks, data.VertexChunksTail );

    RenderProxy->MarkUpdated();

    return &chunk->Vertices[0];
}

bool FIndexedMesh::WriteVertexData( FMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    FMeshVertex * pVerts = WriteVertexData( _VerticesCount, _StartVertexLocation );
    if ( !pVerts ) {
        return false;
    }

    memcpy( pVerts, _Vertices, _VerticesCount * sizeof( FMeshVertex ) );
    return true;
}

unsigned int * FIndexedMesh::WriteIndexData( int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount || _StartIndexLocation + _IndexCount > IndexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteIndexData: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    FIndexChunk * chunk = ( FIndexChunk * )frameData->AllocFrameData( sizeof( FIndexChunk ) + sizeof( unsigned int ) * ( _IndexCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->IndexCount = _IndexCount;
    chunk->StartIndexLocation = _StartIndexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.IndexChunks, data.IndexChunksTail );

    data.IndexType = INDEX_UINT32;

    if ( !data.IndexChunks ) {
        data.IndexMapRange[0] = _StartIndexLocation;
        data.IndexMapRange[1] = _StartIndexLocation + _IndexCount;
    } else {
        data.IndexMapRange[0] = FMath::Min( data.IndexMapRange[0], _StartIndexLocation );
        data.IndexMapRange[1] = FMath::Max( data.IndexMapRange[1], _StartIndexLocation + _IndexCount );
    }

    RenderProxy->MarkUpdated();

    return &chunk->Indices[0];
}

bool FIndexedMesh::WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation ) {
    unsigned int * pIndices = WriteIndexData( _IndexCount, _StartIndexLocation );
    if ( !pIndices ) {
        return false;
    }

    memcpy( pIndices, _Indices, _IndexCount * sizeof( unsigned int ) );
    return true;
}

void FIndexedMesh::InitializeInternalMesh( const char * _Name ) {

    if ( !FString::Cmp( _Name, "*box*" ) ) {
        InitializeShape< FBoxShape >( Float3(1), 1 );
        return;
    }

    if ( !FString::Cmp( _Name, "*sphere*" ) ) {
        InitializeShape< FSphereShape >( 0.5f, 1, 32, 32 );
        return;
    }

    if ( !FString::Cmp( _Name, "*cylinder*" ) ) {
        InitializeShape< FCylinderShape >( 0.5f, 1, 1, 32 );
        return;
    }

    if ( !FString::Cmp( _Name, "*plane*" ) ) {
        InitializeShape< FPlaneShape >( 1.0f, 1.0f, 1 );
        return;
    }

    GLogger.Printf( "Unknown internal mesh %s\n", _Name );
}

FIndexedMeshSubpart::FIndexedMeshSubpart() {
    BoundingBox.Clear();
}

FIndexedMeshSubpart::~FIndexedMeshSubpart() {
    if ( ParentMesh ) {
        ParentMesh->Subparts[ IndexInArrayOfSubparts ] = ParentMesh->Subparts[ ParentMesh->Subparts.Length() - 1 ];
        ParentMesh->Subparts[ IndexInArrayOfSubparts ]->IndexInArrayOfSubparts = IndexInArrayOfSubparts;
        IndexInArrayOfSubparts = -1;
        ParentMesh->Subparts.RemoveLast();
    }
}


FLightmapUV::FLightmapUV() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_LightmapUVChannel >();
    RenderProxy->SetOwner( this );
}

FLightmapUV::~FLightmapUV() {
    RenderProxy->KillProxy();

    if ( ParentMesh ) {
        ParentMesh->LightmapUVs[ IndexInArrayOfUVs ] = ParentMesh->LightmapUVs[ ParentMesh->LightmapUVs.Length() - 1 ];
        ParentMesh->LightmapUVs[ IndexInArrayOfUVs ]->IndexInArrayOfUVs = IndexInArrayOfUVs;
        IndexInArrayOfUVs = -1;
        ParentMesh->LightmapUVs.RemoveLast();
    }
}

void FLightmapUV::OnInitialize( int _NumVertices ) {
    if ( VertexCount == _NumVertices ) {
        return;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    VertexCount = _NumVertices;

    data.VerticesCount = _NumVertices;

    if ( data.Chunks ) {
        data.VertexMapRange[1] = FMath::Min( data.VertexMapRange[1], _NumVertices );

        if ( data.VertexMapRange[0] >= data.VertexMapRange[1] ) {
            data.Chunks = nullptr;
        }
    }

    data.bReallocated = true;

    RenderProxy->MarkUpdated();
}

FMeshLightmapUV * FLightmapUV::WriteVertexData( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FLightmapUV::WriteVertexData: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    FLightmapChunk * chunk = ( FLightmapChunk * )frameData->AllocFrameData( sizeof( FLightmapChunk ) + sizeof( FMeshLightmapUV ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    if ( !data.Chunks ) {
        data.VertexMapRange[0] = _StartVertexLocation;
        data.VertexMapRange[1] = _StartVertexLocation + _VerticesCount;
    } else {
        data.VertexMapRange[0] = FMath::Min( data.VertexMapRange[0], _StartVertexLocation );
        data.VertexMapRange[1] = FMath::Max( data.VertexMapRange[1], _StartVertexLocation + _VerticesCount );
    }

    IntrusiveAddToList( chunk, Next, Prev, data.Chunks, data.ChunksTail );

    RenderProxy->MarkUpdated();

    return &chunk->Vertices[0];
}

bool FLightmapUV::WriteVertexData( FMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    FMeshLightmapUV * pVerts = WriteVertexData( _VerticesCount, _StartVertexLocation );
    if ( !pVerts ) {
        return false;
    }

    memcpy( pVerts, _Vertices, _VerticesCount * sizeof( FMeshLightmapUV ) );
    return true;
}

FVertexLight::FVertexLight() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_VertexLightChannel >();
    RenderProxy->SetOwner( this );
}

FVertexLight::~FVertexLight() {
    RenderProxy->KillProxy();

    if ( ParentMesh ) {
        ParentMesh->VertexLightChannels[ IndexInArrayOfChannels ] = ParentMesh->VertexLightChannels[ ParentMesh->VertexLightChannels.Length() - 1 ];
        ParentMesh->VertexLightChannels[ IndexInArrayOfChannels ]->IndexInArrayOfChannels = IndexInArrayOfChannels;
        IndexInArrayOfChannels = -1;
        ParentMesh->VertexLightChannels.RemoveLast();
    }
}

void FVertexLight::OnInitialize( int _NumVertices ) {
    if ( VertexCount == _NumVertices ) {
        return;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    VertexCount = _NumVertices;

    data.VerticesCount = _NumVertices;

    if ( data.Chunks ) {
        data.VertexMapRange[1] = FMath::Min( data.VertexMapRange[1], _NumVertices );

        if ( data.VertexMapRange[0] >= data.VertexMapRange[1] ) {
            data.Chunks = nullptr;
        }
    }

    data.bReallocated = true;

    RenderProxy->MarkUpdated();
}

FMeshVertexLight * FVertexLight::WriteVertexData( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FVertexLight::WriteVertexData: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    FVertexLightChunk * chunk = ( FVertexLightChunk * )frameData->AllocFrameData( sizeof( FVertexLightChunk ) + sizeof( FMeshVertexLight ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    if ( !data.Chunks ) {
        data.VertexMapRange[0] = _StartVertexLocation;
        data.VertexMapRange[1] = _StartVertexLocation + _VerticesCount;
    } else {
        data.VertexMapRange[0] = FMath::Min( data.VertexMapRange[0], _StartVertexLocation );
        data.VertexMapRange[1] = FMath::Max( data.VertexMapRange[1], _StartVertexLocation + _VerticesCount );
    }

    IntrusiveAddToList( chunk, Next, Prev, data.Chunks, data.ChunksTail );

    RenderProxy->MarkUpdated();

    return &chunk->Vertices[0];
}

bool FVertexLight::WriteVertexData( FMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    FMeshVertexLight * pVerts = WriteVertexData( _VerticesCount, _StartVertexLocation );
    if ( !pVerts ) {
        return false;
    }

    memcpy( pVerts, _Vertices, _VerticesCount * sizeof( FMeshVertexLight ) );
    return true;
}






#if 0
AN_CLASS_META_NO_ATTRIBS( FStaticMesh )

FStaticMesh::FStaticMesh() {
}

FStaticMesh::~FStaticMesh() {
    AN_Assert( !bMapped );

    GRenderFrontend.DestroyVertexCache( VertexCache );

    RemoveFromLoadList();
}

void FStaticMesh::LoadObject( const char * _Path ) {

    if ( bMapped ) {
        GLogger.Printf( "FStaticMesh::LoadObject: missing Unmap()\n" );
        return;
    }

    ResourcePath = _Path;

    AddToLoadList();

    if ( _Path[0] == '*' ) {
        CreateInternalMesh( _Path );
        return;
    }

    // TODO: load from file

    // For test:
    InitializeShape< FBoxShape >( Float3( 1.0f ), 1.0f );
}

void FStaticMesh::ComputeBounds() {
    if ( !VertexCache ) {
        GLogger.Printf( "FStaticMesh::ComputeBounds: mesh has no data\n" );
        return;
    }
    ComputeBounds( ( FMeshVertex * )VertexCache->IndexedMesh.pVertices, VertexCache->IndexedMesh.NumVertices );
}

void FStaticMesh::ComputeBounds( const FMeshVertex * _Vertices, int _NumVertices ) {
    Bounds.Clear();
    for ( const FMeshVertex * pVertex = _Vertices, *pLast = _Vertices + _NumVertices ; pVertex < pLast ; pVertex++ ) {
        Bounds.AddPoint( pVertex->Position );
    }
}



#if 0
void FStaticMesh::Map( FMeshVertex const **_Vertices,
                       int & _VerticesCount,
                       unsigned int const **_Indices,
                       int & _IndicesCount ) {

    if ( !VertexCache ) {
        GLogger.Printf( "FStaticMesh::Map: mesh has no data\n" );

        *_Vertices = nullptr;
        *_Indices = nullptr;
        _VerticesCount = 0;
        _IndicesCount = 0;

        return;
    }

    if ( bMapped ) {
        GLogger.Printf( "FStaticMesh::Map: already mapped\n" );
    }

    *_Vertices = ( FMeshVertex * )VertexCache->IndexedMesh.pVertices;
    *_Indices = ( unsigned int * )VertexCache->IndexedMesh.pIndices;

    _VerticesCount = VertexCache->IndexedMesh.NumVertices;
    _IndicesCount = VertexCache->IndexedMesh.NumIndices;

    bMapped = true;
}

void FStaticMesh::Unmap() {
    if ( !bMapped ) {
        GLogger.Printf( "FStaticMesh::Unmap: already unmapped\n" );
        return;
    }

    bMapped = false;
}
#endif

int FStaticMesh::GetVerticesCount() const {
    return VertexCache ? VertexCache->IndexedMesh.NumVertices : 0;
}

int FStaticMesh::GetIndicesCount() const {
    return VertexCache ? VertexCache->IndexedMesh.NumIndices : 0;
}
#endif
