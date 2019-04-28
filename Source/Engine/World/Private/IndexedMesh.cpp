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

#include <Engine/World/Public/IndexedMesh.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META_NO_ATTRIBS( FIndexedMesh )
AN_CLASS_META_NO_ATTRIBS( FIndexedMeshSubpart )
AN_CLASS_META_NO_ATTRIBS( FLightmapUV )
AN_CLASS_META_NO_ATTRIBS( FVertexLight )

///////////////////////////////////////////////////////////////////////////////////////////////////////

FIndexedMesh::FIndexedMesh() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_IndexedMesh >();
    RenderProxy->SetOwner( this );
}

FIndexedMesh::~FIndexedMesh() {
    RenderProxy->KillProxy();

    for ( FIndexedMeshSubpart * subpart : Subparts ) {
        subpart->ParentMesh = nullptr;
        subpart->RemoveRef();
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

void FIndexedMesh::Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh, bool _DynamicStorage ) {
    if ( VertexCount == _NumVertices && IndexCount == _NumIndices && bSkinnedMesh == _SkinnedMesh && bDynamicStorage == _DynamicStorage ) {
        return;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    VertexCount = _NumVertices;
    IndexCount = _NumIndices;
    bSkinnedMesh = _SkinnedMesh;
    bDynamicStorage = _DynamicStorage;

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];
    data.VerticesCount = _NumVertices;
    data.IndicesCount = _NumIndices;
    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;
    data.IndexType = INDEX_UINT32;
    data.VertexChunks = nullptr;
    data.VertexJointChunks = nullptr;
    data.IndexChunks = nullptr;
    data.bReallocated = true;
    RenderProxy->MarkUpdated();

    for ( FLightmapUV * channel : LightmapUVs ) {
        channel->OnInitialize( _NumVertices );
    }

    for ( FVertexLight * channel : VertexLightChannels ) {
        channel->OnInitialize( _NumVertices );
    }

    if ( _NumSubparts <= 0 ) {
        _NumSubparts = 1;
    }

    for ( FIndexedMeshSubpart * subpart : Subparts ) {
        subpart->ParentMesh = nullptr;
        subpart->RemoveRef();
    }

    Subparts.ResizeInvalidate( _NumSubparts );
    for ( int i = 0 ; i < _NumSubparts ; i++ ) {
        FIndexedMeshSubpart * subpart = NewObject< FIndexedMeshSubpart >();
        subpart->AddRef();
        subpart->ParentMesh = this;
        Subparts[i] = subpart;
    }

    if ( _NumSubparts == 1 ) {
        FIndexedMeshSubpart * subpart = Subparts[0];
        subpart->BaseVertex = 0;
        subpart->FirstIndex = 0;
        subpart->VertexCount = VertexCount;
        subpart->IndexCount = IndexCount;
    }
}

void FIndexedMesh::InitializeDefaultObject() {
    InitializeInternalMesh( "*box*" );
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

FIndexedMeshSubpart * FIndexedMesh::GetSubpart( int _SubpartIndex ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Length() ) {
        return nullptr;
    }
    return Subparts[ _SubpartIndex ];
}

FMeshVertex * FIndexedMesh::WriteVertexData( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteVertexData: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;

    FVertexChunk * chunk = ( FVertexChunk * )frameData->AllocFrameData( sizeof( FVertexChunk ) + sizeof( FMeshVertex ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

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

FMeshVertexJoint * FIndexedMesh::WriteJointWeights( int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "FIndexedMesh::WriteJointWeights: Cannot write joint weights for static mesh\n" );
        return nullptr;
    }

    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteJointWeights: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;

    FVertexJointChunk * chunk = ( FVertexJointChunk * )frameData->AllocFrameData( sizeof( FVertexJointChunk ) + sizeof( FMeshVertexJoint ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.VertexJointChunks, data.VertexJointChunksTail );

    RenderProxy->MarkUpdated();

    return &chunk->Vertices[0];
}

bool FIndexedMesh::WriteJointWeights( FMeshVertexJoint const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    FMeshVertexJoint * pVerts = WriteJointWeights( _VerticesCount, _StartVertexLocation );
    if ( !pVerts ) {
        return false;
    }

    memcpy( pVerts, _Vertices, _VerticesCount * sizeof( FMeshVertexJoint ) );
    return true;
}

unsigned int * FIndexedMesh::WriteIndexData( int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount || _StartIndexLocation + _IndexCount > IndexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteIndexData: Referencing outside of buffer\n" );
        return nullptr;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;
    data.IndexType = INDEX_UINT32;

    FIndexChunk * chunk = ( FIndexChunk * )frameData->AllocFrameData( sizeof( FIndexChunk ) + sizeof( unsigned int ) * ( _IndexCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->IndexCount = _IndexCount;
    chunk->StartIndexLocation = _StartIndexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.IndexChunks, data.IndexChunksTail );

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
        SetName( _Name );
        BodyComposition.Clear();
        FCollisionBox * collisionBody = BodyComposition.NewCollisionBody< FCollisionBox >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Cmp( _Name, "*sphere*" ) ) {
        InitializeShape< FSphereShape >( 0.5f, 1, 32, 32 );
        SetName( _Name );
        BodyComposition.Clear();
        FCollisionSphere * collisionBody = BodyComposition.NewCollisionBody< FCollisionSphere >();
        collisionBody->Radius = 0.5f;
        return;
    }

    if ( !FString::Cmp( _Name, "*cylinder*" ) ) {
        InitializeShape< FCylinderShape >( 0.5f, 1, 1, 32 );
        SetName( _Name );
        BodyComposition.Clear();
        FCollisionCylinder * collisionBody = BodyComposition.NewCollisionBody< FCollisionCylinder >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Cmp( _Name, "*plane*" ) ) {
        InitializeShape< FPlaneShape >( 1.0f, 1.0f, 1 );
        SetName( _Name );
        BodyComposition.Clear();
        BodyComposition.NewCollisionBody< FCollisionPlane >();
        return;
    }

    GLogger.Printf( "Unknown internal mesh %s\n", _Name );
}


//#include <BulletCollision/CollisionShapes/btStridingMeshInterface.h>

//btStridingMeshInterface * FIndexedMesh::CreateStridingMeshInterface() {

//}

///////////////////////////////////////////////////////////////////////////////////////////////////////

FIndexedMeshSubpart::FIndexedMeshSubpart() {
    BoundingBox.Clear();
}

FIndexedMeshSubpart::~FIndexedMeshSubpart() {
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

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
    if ( VertexCount == _NumVertices && bDynamicStorage == ParentMesh->bDynamicStorage ) {
        return;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    VertexCount = _NumVertices;
    bDynamicStorage = ParentMesh->bDynamicStorage;

    data.VerticesCount = _NumVertices;
    data.bDynamicStorage = bDynamicStorage;
    data.Chunks = nullptr;
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

    data.bDynamicStorage = bDynamicStorage;

    FLightmapChunk * chunk = ( FLightmapChunk * )frameData->AllocFrameData( sizeof( FLightmapChunk ) + sizeof( FMeshLightmapUV ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

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

///////////////////////////////////////////////////////////////////////////////////////////////////////

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
    if ( VertexCount == _NumVertices && bDynamicStorage == ParentMesh->bDynamicStorage ) {
        return;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data[frameData->SmpIndex];

    VertexCount = _NumVertices;
    bDynamicStorage = ParentMesh->bDynamicStorage;

    data.VerticesCount = _NumVertices;
    data.bDynamicStorage = bDynamicStorage;
    data.Chunks = nullptr;
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

    data.bDynamicStorage = bDynamicStorage;

    FVertexLightChunk * chunk = ( FVertexLightChunk * )frameData->AllocFrameData( sizeof( FVertexLightChunk ) + sizeof( FMeshVertexLight ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return nullptr;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

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
