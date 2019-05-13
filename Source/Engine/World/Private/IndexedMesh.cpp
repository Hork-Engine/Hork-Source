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
#include <Engine/World/Public/MeshAsset.h>
#include <Engine/World/Public/ResourceManager.h>

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

    Purge();
}

void FIndexedMesh::Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh, bool _DynamicStorage ) {
    //if ( VertexCount == _NumVertices && IndexCount == _NumIndices && bSkinnedMesh == _SkinnedMesh && bDynamicStorage == _DynamicStorage ) {
    //    return;
    //}

    Purge();

    FRenderFrame * frameData = GRuntime.GetFrameData();

    VertexCount = _NumVertices;
    IndexCount = _NumIndices;
    bSkinnedMesh = _SkinnedMesh;
    bDynamicStorage = _DynamicStorage;

    Vertices.ResizeInvalidate( VertexCount );
    if ( bSkinnedMesh ) {
        Weights.ReserveInvalidate( VertexCount );
    }
    Indices.ResizeInvalidate( IndexCount );

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->WriteIndex];
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

void FIndexedMesh::Purge() {
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

    BodyComposition.Clear();
}

void FIndexedMesh::InitializeDefaultObject() {
    InitializeInternalMesh( "*box*" );
}

bool FIndexedMesh::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    FFileStream f;

    if ( !f.OpenRead( _Path ) ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    FMeshAsset asset;
    asset.Read( f );

    TPodArray< FMaterialInstance * > matInstances;
    matInstances.Resize( asset.Materials.Length() );

    for ( int j = 0; j < asset.Materials.Length(); j++ ) {
        FMeshMaterial const & material = asset.Materials[ j ];

        FMaterialInstance * matInst = CreateInstanceOf< FMaterialInstance >();
        //matInst->Material = Material;
        matInstances[ j ] = matInst;

        for ( int n = 0; n < 1/*material.NumTextures*/; n++ ) {
            FMaterialTexture const & texture = asset.Textures[ material.Textures[ n ] ];
            FTexture * texObj = CreateResource< FTexture >( texture.FileName.ToConstChar() );
            matInst->SetTexture( n, texObj );
        }
    }

    bool bSkinned = asset.Weights.Length() == asset.Vertices.Length();

    Initialize( asset.Vertices.Length(), asset.Indices.Length(), asset.Subparts.size(), bSkinned, false );
    WriteVertexData( asset.Vertices.ToPtr(), asset.Vertices.Length(), 0 );
    WriteIndexData( asset.Indices.ToPtr(), asset.Indices.Length(), 0 );
    if ( bSkinned ) {
        WriteJointWeights( asset.Weights.ToPtr(), asset.Weights.Length(), 0 );
    }
    for ( int j = 0; j < GetSubparts().Length(); j++ ) {
        FSubpart const & s = asset.Subparts[ j ];
        FIndexedMeshSubpart * subpart = GetSubpart( j );
        subpart->SetName( s.Name );
        subpart->BaseVertex = s.BaseVertex;
        subpart->FirstIndex = s.FirstIndex;
        subpart->VertexCount = s.VertexCount;
        subpart->IndexCount = s.IndexCount;
        subpart->BoundingBox = s.BoundingBox;
        subpart->MaterialInstance = matInstances[ s.Material ];
    }

    // TODO: load collision from file. This code is only for test!!!

    FCollisionTriangleSoupData * tris = CreateInstanceOf< FCollisionTriangleSoupData >();
    tris->Initialize( ( float * )&asset.Vertices.ToPtr()->Position, sizeof( asset.Vertices[ 0 ] ), asset.Vertices.Length(),
        asset.Indices.ToPtr(), asset.Indices.Length(), asset.Subparts.data(), asset.Subparts.size() );

    FCollisionTriangleSoupBVHData * bvh = CreateInstanceOf< FCollisionTriangleSoupBVHData >();
    bvh->TrisData = tris;
    bvh->BuildBVH();

    BodyComposition.Clear();
    FCollisionSharedTriangleSoupBVH * CollisionBody = BodyComposition.NewCollisionBody< FCollisionSharedTriangleSoupBVH >();
    CollisionBody->BvhData = bvh;
    //CollisionBody->Margin = 0.2;

    return true;
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

bool FIndexedMesh::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::SendVertexDataToGPU: Referencing outside of buffer\n" );
        return false;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;

    FVertexChunk * chunk = ( FVertexChunk * )frameData->AllocFrameData( sizeof( FVertexChunk ) + sizeof( FMeshVertex ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return false;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.VertexChunks, data.VertexChunksTail );

    RenderProxy->MarkUpdated();

    memcpy( &chunk->Vertices[ 0 ], Vertices.ToPtr() + _StartVertexLocation, _VerticesCount * sizeof( FMeshVertex ) );

    return true;
}

bool FIndexedMesh::WriteVertexData( FMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteVertexData: Referencing outside of buffer\n" );
        return false;
    }

    memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshVertex ) );

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}

bool FIndexedMesh::SendJointWeightsToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "FIndexedMesh::SendJointWeightsToGPU: Cannot write joint weights for static mesh\n" );
        return false;
    }

    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::SendJointWeightsToGPU: Referencing outside of buffer\n" );
        return false;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;

    FVertexJointChunk * chunk = ( FVertexJointChunk * )frameData->AllocFrameData( sizeof( FVertexJointChunk ) + sizeof( FMeshVertexJoint ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return false;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.VertexJointChunks, data.VertexJointChunksTail );

    RenderProxy->MarkUpdated();

    memcpy( &chunk->Vertices[ 0 ], Weights.ToPtr() + _StartVertexLocation, _VerticesCount * sizeof( FMeshVertexJoint ) );

    return true;
}

bool FIndexedMesh::WriteJointWeights( FMeshVertexJoint const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "FIndexedMesh::WriteJointWeights: Cannot write joint weights for static mesh\n" );
        return false;
    }

    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteJointWeights: Referencing outside of buffer\n" );
        return false;
    }

    memcpy( Weights.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshVertexJoint ) );

    return SendJointWeightsToGPU( _VerticesCount, _StartVertexLocation );
}

bool FIndexedMesh::SendIndexDataToGPU( int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount || _StartIndexLocation + _IndexCount > IndexCount ) {
        GLogger.Printf( "FIndexedMesh::SendIndexDataToGPU: Referencing outside of buffer\n" );
        return false;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    data.bSkinnedMesh = bSkinnedMesh;
    data.bDynamicStorage = bDynamicStorage;
    data.IndexType = INDEX_UINT32;

    FIndexChunk * chunk = ( FIndexChunk * )frameData->AllocFrameData( sizeof( FIndexChunk ) + sizeof( unsigned int ) * ( _IndexCount - 1 ) );
    if ( !chunk ) {
        return false;
    }

    chunk->IndexCount = _IndexCount;
    chunk->StartIndexLocation = _StartIndexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.IndexChunks, data.IndexChunksTail );

    RenderProxy->MarkUpdated();

    memcpy( &chunk->Indices[ 0 ], Indices.ToPtr() + _StartIndexLocation, _IndexCount * sizeof( unsigned int ) );

    return true;
}

bool FIndexedMesh::WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount || _StartIndexLocation + _IndexCount > IndexCount ) {
        GLogger.Printf( "FIndexedMesh::WriteIndexData: Referencing outside of buffer\n" );
        return false;
    }

    memcpy( Indices.ToPtr() + _StartIndexLocation, _Indices, _IndexCount * sizeof( unsigned int ) );

    return SendIndexDataToGPU( _IndexCount, _StartIndexLocation );
}

void FIndexedMesh::InitializeInternalMesh( const char * _Name ) {

    if ( !FString::Cmp( _Name, "*box*" ) ) {
        InitializeShape< FBoxShape >( Float3(1), 1 );
        SetName( _Name );
        FCollisionBox * collisionBody = BodyComposition.NewCollisionBody< FCollisionBox >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Cmp( _Name, "*sphere*" ) ) {
        InitializeShape< FSphereShape >( 0.5f, 1, 32, 32 );
        SetName( _Name );
        FCollisionSphere * collisionBody = BodyComposition.NewCollisionBody< FCollisionSphere >();
        collisionBody->Radius = 0.5f;
        return;
    }

    if ( !FString::Cmp( _Name, "*cylinder*" ) ) {
        InitializeShape< FCylinderShape >( 0.5f, 1, 1, 32 );
        SetName( _Name );
        FCollisionCylinder * collisionBody = BodyComposition.NewCollisionBody< FCollisionCylinder >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Cmp( _Name, "*plane*" ) ) {
        InitializeShape< FPlaneShape >( 1.0f, 1.0f, 1 );
        SetName( _Name );
        BodyComposition.NewCollisionBody< FCollisionPlane >();
        return;
    }

    GLogger.Printf( "Unknown internal mesh %s\n", _Name );
}


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

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    VertexCount = _NumVertices;
    bDynamicStorage = ParentMesh->bDynamicStorage;

    data.VerticesCount = _NumVertices;
    data.bDynamicStorage = bDynamicStorage;
    data.Chunks = nullptr;
    data.bReallocated = true;

    Vertices.ResizeInvalidate( _NumVertices );

    RenderProxy->MarkUpdated();
}

bool FLightmapUV::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FLightmapUV::SendVertexDataToGPU: Referencing outside of buffer\n" );
        return false;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    data.bDynamicStorage = bDynamicStorage;

    FLightmapChunk * chunk = ( FLightmapChunk * )frameData->AllocFrameData( sizeof( FLightmapChunk ) + sizeof( FMeshLightmapUV ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return false;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.Chunks, data.ChunksTail );

    RenderProxy->MarkUpdated();

    memcpy( &chunk->Vertices[ 0 ], Vertices.ToPtr() + _StartVertexLocation, _VerticesCount * sizeof( FMeshLightmapUV ) );

    return true;
}

bool FLightmapUV::WriteVertexData( FMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FLightmapUV::WriteVertexData: Referencing outside of buffer\n" );
        return false;
    }

    memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshLightmapUV ) );

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
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

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    VertexCount = _NumVertices;
    bDynamicStorage = ParentMesh->bDynamicStorage;

    data.VerticesCount = _NumVertices;
    data.bDynamicStorage = bDynamicStorage;
    data.Chunks = nullptr;
    data.bReallocated = true;

    Vertices.ResizeInvalidate( _NumVertices );

    RenderProxy->MarkUpdated();
}

bool FVertexLight::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FVertexLight::SendVertexDataToGPU: Referencing outside of buffer\n" );
        return false;
    }

    FRenderFrame * frameData = GRuntime.GetFrameData();

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data[frameData->WriteIndex];

    data.bDynamicStorage = bDynamicStorage;

    FVertexLightChunk * chunk = ( FVertexLightChunk * )frameData->AllocFrameData( sizeof( FVertexLightChunk ) + sizeof( FMeshVertexLight ) * ( _VerticesCount - 1 ) );
    if ( !chunk ) {
        return false;
    }

    chunk->VerticesCount = _VerticesCount;
    chunk->StartVertexLocation = _StartVertexLocation;

    IntrusiveAddToList( chunk, Next, Prev, data.Chunks, data.ChunksTail );

    RenderProxy->MarkUpdated();

    memcpy( &chunk->Vertices[ 0 ], Vertices.ToPtr() + _StartVertexLocation, _VerticesCount * sizeof( FMeshVertexLight ) );

    return true;
}

bool FVertexLight::WriteVertexData( FMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount || _StartVertexLocation + _VerticesCount > VertexCount ) {
        GLogger.Printf( "FVertexLight::WriteVertexData: Referencing outside of buffer\n" );
        return false;
    }

    memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshVertexLight ) );

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}
