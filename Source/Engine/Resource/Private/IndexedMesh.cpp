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

#include <Engine/Resource/Public/IndexedMesh.h>
#include <Engine/Resource/Public/Asset.h>
#include <Engine/Resource/Public/ResourceManager.h>

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Public/BV/BvIntersect.h>

AN_CLASS_META( FIndexedMesh )
AN_CLASS_META( FIndexedMeshSubpart )
AN_CLASS_META( FLightmapUV )
AN_CLASS_META( FVertexLight )

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

    VertexCount = _NumVertices;
    IndexCount = _NumIndices;
    bSkinnedMesh = _SkinnedMesh;
    bDynamicStorage = _DynamicStorage;
    bBoundingBoxDirty = true;
    BoundingBox.Clear();

    Vertices.ResizeInvalidate( VertexCount );
    if ( bSkinnedMesh ) {
        Weights.ReserveInvalidate( VertexCount );
    }
    Indices.ResizeInvalidate( IndexCount );

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data;
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
        subpart->OwnerMesh = nullptr;
        subpart->RemoveRef();
    }

    static TStaticInternalResourceFinder< FMaterialInstance > DefaultMaterialInstance( _CTS( "FMaterialInstance.Default" ) );

    FMaterialInstance * materialInstance = DefaultMaterialInstance.GetObject();

    Subparts.ResizeInvalidate( _NumSubparts );
    for ( int i = 0 ; i < _NumSubparts ; i++ ) {
        FIndexedMeshSubpart * subpart = NewObject< FIndexedMeshSubpart >();
        subpart->AddRef();
        subpart->OwnerMesh = this;
        subpart->MaterialInstance = materialInstance;
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
        subpart->OwnerMesh = nullptr;
        subpart->RemoveRef();
    }

    for ( FLightmapUV * channel : LightmapUVs ) {
        channel->OwnerMesh = nullptr;
        channel->IndexInArrayOfUVs = -1;
    }

    for ( FVertexLight * channel : VertexLightChannels ) {
        channel->OwnerMesh = nullptr;
        channel->IndexInArrayOfChannels = -1;
    }

    BodyComposition.Clear();
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
    matInstances.Resize( asset.Materials.Size() );

    for ( int j = 0; j < asset.Materials.Size(); j++ ) {

        FMaterialInstance * matInst = CreateInstanceOf< FMaterialInstance >();
        //matInst->Material = Material;
        matInstances[ j ] = matInst;

        FMeshMaterial const & material = asset.Materials[ j ];
        for ( int n = 0; n < 1/*material.NumTextures*/; n++ ) {
            FMaterialTexture const & texture = asset.Textures[ material.Textures[ n ] ];
            FTexture * texObj = GetOrCreateResource< FTexture >( texture.FileName.ToConstChar() );
            matInst->SetTexture( n, texObj );
        }
    }

    bool bSkinned = asset.Weights.Size() == asset.Vertices.Size();

    Initialize( asset.Vertices.Size(), asset.Indices.Size(), asset.Subparts.size(), bSkinned, false );
    WriteVertexData( asset.Vertices.ToPtr(), asset.Vertices.Size(), 0 );
    WriteIndexData( asset.Indices.ToPtr(), asset.Indices.Size(), 0 );
    if ( bSkinned ) {
        WriteJointWeights( asset.Weights.ToPtr(), asset.Weights.Size(), 0 );
    }
    for ( int j = 0; j < GetSubparts().Size(); j++ ) {
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
    tris->Initialize( ( float * )&asset.Vertices.ToPtr()->Position, sizeof( asset.Vertices[ 0 ] ), asset.Vertices.Size(),
        asset.Indices.ToPtr(), asset.Indices.Size(), asset.Subparts.data(), asset.Subparts.size() );

    FCollisionTriangleSoupBVHData * bvh = CreateInstanceOf< FCollisionTriangleSoupBVHData >();
    bvh->TrisData = tris;
    bvh->BuildBVH();

    BodyComposition.Clear();
    FCollisionTriangleSoupBVH * CollisionBody = BodyComposition.AddCollisionBody< FCollisionTriangleSoupBVH >();
    CollisionBody->BvhData = bvh;
    //CollisionBody->Margin = 0.2;

    return true;
}

FLightmapUV * FIndexedMesh::CreateLightmapUVChannel() {
    FLightmapUV * channel = NewObject< FLightmapUV >();

    channel->OwnerMesh = this;
    channel->IndexInArrayOfUVs = LightmapUVs.Size();

    LightmapUVs.Append( channel );

    channel->OnInitialize( VertexCount );

    return channel;
}

FVertexLight * FIndexedMesh::CreateVertexLightChannel() {
    FVertexLight * channel = NewObject< FVertexLight >();

    channel->OwnerMesh = this;
    channel->IndexInArrayOfChannels = VertexLightChannels.Size();

    VertexLightChannels.Append( channel );

    channel->OnInitialize( VertexCount );

    return channel;
}

FIndexedMeshSubpart * FIndexedMesh::GetSubpart( int _SubpartIndex ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
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

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data;

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

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data;

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

    FRenderProxy_IndexedMesh::FrameData & data = RenderProxy->Data;

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

void FIndexedMesh::UpdateBoundingBox() {
    BoundingBox.Clear();
    for ( FIndexedMeshSubpart const * subpart : Subparts ) {
        BoundingBox.AddAABB( subpart->GetBoundingBox() );
    }
    bBoundingBoxDirty = false;
}

BvAxisAlignedBox const & FIndexedMesh::GetBoundingBox() const {
    if ( bBoundingBoxDirty ) {
        const_cast< ThisClass * >( this )->UpdateBoundingBox();
    }

    return BoundingBox;
}

void FIndexedMesh::InitializeBoxMesh( const Float3 & _Size, float _TexCoordScale ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateBoxMesh( vertices, indices, bounds, _Size, _TexCoordScale );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeSphereMesh( float _Radius, float _TexCoordScale, int _HDiv, int _VDiv ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;
    
    CreateSphereMesh( vertices, indices, bounds, _Radius, _TexCoordScale, _HDiv, _VDiv );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializePlaneMesh( float _Width, float _Height, float _TexCoordScale ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreatePlaneMesh( vertices, indices, bounds, _Width, _Height, _TexCoordScale );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializePatchMesh( Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, int resx, int resy, float _TexCoordScale, bool _TwoSided ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreatePatchMesh( vertices, indices, bounds,
        Corner00, Corner10, Corner01, Corner11, resx, resy, _TexCoordScale, _TwoSided );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeCylinderMesh( float _Radius, float _Height, float _TexCoordScale, int _VDiv ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateCylinderMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _VDiv );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeInternalResource( const char * _InternalResourceName ) {

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Box" )
         || !FString::Icmp( _InternalResourceName, "FIndexedMesh.Default" ) ) {
        InitializeBoxMesh( Float3(1), 1 );
        //SetName( _InternalResourceName );
        FCollisionBox * collisionBody = BodyComposition.AddCollisionBody< FCollisionBox >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Sphere" ) ) {
        InitializeSphereMesh( 0.5f, 1, 32, 32 );
        //SetName( _InternalResourceName );
        FCollisionSphere * collisionBody = BodyComposition.AddCollisionBody< FCollisionSphere >();
        collisionBody->Radius = 0.5f;
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Cylinder" ) ) {
        InitializeCylinderMesh( 0.5f, 1, 1, 32 );
        //SetName( _InternalResourceName );
        FCollisionCylinder * collisionBody = BodyComposition.AddCollisionBody< FCollisionCylinder >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Plane" ) ) {
        InitializePlaneMesh( 256, 256, 256 );
        //SetName( _InternalResourceName );
        FCollisionBox * box = BodyComposition.AddCollisionBody< FCollisionBox >();
        box->HalfExtents.X = 128;
        box->HalfExtents.Y = 0.1f;
        box->HalfExtents.Z = 128;
        box->Position.Y -= box->HalfExtents.Y;
        return;
    }

    GLogger.Printf( "Unknown internal mesh %s\n", _InternalResourceName );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

FIndexedMeshSubpart::FIndexedMeshSubpart() {
    BoundingBox.Clear();
}

FIndexedMeshSubpart::~FIndexedMeshSubpart() {
}

void FIndexedMeshSubpart::SetBoundingBox( BvAxisAlignedBox const & _BoundingBox ) {
    BoundingBox = _BoundingBox;

    OwnerMesh->bBoundingBoxDirty = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

FLightmapUV::FLightmapUV() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_LightmapUVChannel >();
    RenderProxy->SetOwner( this );
}

FLightmapUV::~FLightmapUV() {
    RenderProxy->KillProxy();

    if ( OwnerMesh ) {
        OwnerMesh->LightmapUVs[ IndexInArrayOfUVs ] = OwnerMesh->LightmapUVs[ OwnerMesh->LightmapUVs.Size() - 1 ];
        OwnerMesh->LightmapUVs[ IndexInArrayOfUVs ]->IndexInArrayOfUVs = IndexInArrayOfUVs;
        IndexInArrayOfUVs = -1;
        OwnerMesh->LightmapUVs.RemoveLast();
    }
}

void FLightmapUV::OnInitialize( int _NumVertices ) {
    if ( VertexCount == _NumVertices && bDynamicStorage == OwnerMesh->bDynamicStorage ) {
        return;
    }

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data;

    VertexCount = _NumVertices;
    bDynamicStorage = OwnerMesh->bDynamicStorage;

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

    FRenderProxy_LightmapUVChannel::FrameData & data = RenderProxy->Data;

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

    if ( OwnerMesh ) {
        OwnerMesh->VertexLightChannels[ IndexInArrayOfChannels ] = OwnerMesh->VertexLightChannels[ OwnerMesh->VertexLightChannels.Size() - 1 ];
        OwnerMesh->VertexLightChannels[ IndexInArrayOfChannels ]->IndexInArrayOfChannels = IndexInArrayOfChannels;
        IndexInArrayOfChannels = -1;
        OwnerMesh->VertexLightChannels.RemoveLast();
    }
}

void FVertexLight::OnInitialize( int _NumVertices ) {
    if ( VertexCount == _NumVertices && bDynamicStorage == OwnerMesh->bDynamicStorage ) {
        return;
    }

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data;

    VertexCount = _NumVertices;
    bDynamicStorage = OwnerMesh->bDynamicStorage;

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

    FRenderProxy_VertexLightChannel::FrameData & data = RenderProxy->Data;

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

void FIndexedMesh::GenerateSoftbodyFacesFromMeshIndices() {
    int totalIndices = 0;

    for ( FIndexedMeshSubpart const * subpart : Subparts ) {
        totalIndices += subpart->IndexCount;
    }

    SoftbodyFaces.ResizeInvalidate( totalIndices / 3 );

    int faceIndex = 0;

    unsigned int const * indices = Indices.ToPtr();

    for ( FIndexedMeshSubpart const * subpart : Subparts ) {
        for ( int i = 0; i < subpart->IndexCount; i += 3 ) {
            FSoftbodyFace & face = SoftbodyFaces[ faceIndex++ ];

            face.Indices[ 0 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i ];
            face.Indices[ 1 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i + 1 ];
            face.Indices[ 2 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i + 2 ];
        }
    }
}

void FIndexedMesh::GenerateSoftbodyLinksFromFaces() {
    TPodArray< bool > checks;
    unsigned int * indices;

    checks.Resize( VertexCount*VertexCount );
    checks.ZeroMem();

    SoftbodyLinks.Clear();

    for ( FSoftbodyFace & face : SoftbodyFaces ) {
        indices = face.Indices;

        for ( int j = 2, k = 0; k < 3; j = k++ ) {

            unsigned int index_j_k = indices[ j ] + indices[ k ]*VertexCount;

            // Check if link not exists
            if ( !checks[ index_j_k ] ) {

                unsigned int index_k_j = indices[ k ] + indices[ j ]*VertexCount;

                // Mark link exits
                checks[ index_j_k ] = checks[ index_k_j ] = true;

                // Append link
                FSoftbodyLink & link = SoftbodyLinks.Append();
                link.Indices[0] = indices[ j ];
                link.Indices[1] = indices[ k ];
            }
        }
        #undef IDX
    }
}

// TODO: Optimize with octree/KDtree?
bool FIndexedMeshSubpart::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const {
    bool ret = false;
    float u, v;

    // TODO: check subpart AABB

    unsigned int const * indices = OwnerMesh->GetIndices() + FirstIndex;
    FMeshVertex const * vertices = OwnerMesh->GetVertices();

    const int numTriangles = IndexCount / 3;

    for ( int tri = 0 ; tri < numTriangles ; tri++, indices+=3 ) {
        const unsigned int i0 = BaseVertex + indices[ 0 ];
        const unsigned int i1 = BaseVertex + indices[ 1 ];
        const unsigned int i2 = BaseVertex + indices[ 2 ];

        Float3 const & v0 = vertices[i0].Position;
        Float3 const & v1 = vertices[i1].Position;
        Float3 const & v2 = vertices[i2].Position;

        float dist;
        if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, dist, u, v ) ) {
            if ( _Distance > dist ) {
                FTriangleHitResult & hitResult = _HitResult.Append();
                hitResult.Location = _RayStart + _RayDir * dist;
                hitResult.Normal = (v1-v0).Cross( v2-v0 ).Normalized();
                hitResult.Distance = dist;
                hitResult.UV.X = u;
                hitResult.UV.Y = v;
                hitResult.Indices[0] = i0;
                hitResult.Indices[1] = i1;
                hitResult.Indices[2] = i2;
                hitResult.Material = MaterialInstance;
                ret = true;
            }
        }
    }
    return ret;
}

bool FIndexedMeshSubpart::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const {
    bool ret = false;
    float u, v;

    // TODO: check subpart AABB

    float minDist = _Distance;

    unsigned int const * indices = OwnerMesh->GetIndices() + FirstIndex;
    FMeshVertex const * vertices = OwnerMesh->GetVertices();

    const int numTriangles = IndexCount / 3;

    for ( int tri = 0 ; tri < numTriangles ; tri++, indices+=3 ) {
        const unsigned int i0 = BaseVertex + indices[ 0 ];
        const unsigned int i1 = BaseVertex + indices[ 1 ];
        const unsigned int i2 = BaseVertex + indices[ 2 ];

        Float3 const & v0 = vertices[i0].Position;
        Float3 const & v1 = vertices[i1].Position;
        Float3 const & v2 = vertices[i2].Position;

        float dist;
        if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, dist, u, v ) ) {
            if ( minDist > dist ) {
                minDist = dist;
                _HitLocation = _RayStart + _RayDir * dist;
                _HitDistance = dist;
                _HitUV.X = u;
                _HitUV.Y = v;
                _Indices[0] = i0;
                _Indices[1] = i1;
                _Indices[2] = i2;
                ret = true;
            }
        }
    }
    return ret;
}

bool FIndexedMesh::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const {
    bool ret = false;
    // TODO: check mesh AABB
    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        FIndexedMeshSubpart * subpart = Subparts[i];
        ret |= subpart->Raycast( _RayStart, _RayDir, _Distance, _HitResult );
    }
    return ret;
}

bool FIndexedMesh::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], TRef< FMaterialInstance > & _Material ) const {
    bool ret = false;
    // TODO: check mesh AABB
    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        FIndexedMeshSubpart * subpart = Subparts[i];
        if ( subpart->RaycastClosest( _RayStart, _RayDir, _Distance, _HitLocation, _HitUV, _HitDistance, _Indices ) ) {
            _Material = subpart->MaterialInstance;
            _Distance = _HitDistance;
            ret = true;
        }
    }
    return ret;
}

void FMeshAsset::Clear() {
    Subparts.clear();
    Textures.clear();
    Materials.Clear();
    Vertices.Clear();
    Indices.Clear();
    Weights.Clear();
}

void FMeshAsset::Read( FFileStream & f ) {
    char buf[1024];
    char * s;
    int format, version;

    Clear();

    if ( !AssetReadFormat( f, &format, &version ) ) {
        return;
    }

    if ( format != FMT_FILE_TYPE_MESH ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_MESH );
        return;
    }

    if ( version != FMT_VERSION_MESH ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_MESH );
        return;
    }

    while ( f.Gets( buf, sizeof( buf ) ) ) {
        if ( nullptr != ( s = AssetParseTag( buf, "textures " ) ) ) {
            int numTextures = 0;
            sscanf( s, "%d", &numTextures );
            Textures.resize( numTextures );
            for ( int i = 0 ; i < numTextures ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }
                for ( s = buf ; *s && *s != '\n' ; s++ ) {} *s = 0;
                Textures[i].FileName = buf;
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "materials " ) ) ) {
            int numMaterials = 0;
            sscanf( s, "%d", &numMaterials );
            Materials.ResizeInvalidate( numMaterials );
            for ( int i = 0 ; i < numMaterials ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }
                if ( nullptr != ( s = AssetParseTag( buf, "maps " ) ) ) {
                    Materials[i].NumTextures = 0;
                    sscanf( s, "%d", &Materials[i].NumTextures );
                    for ( int j = 0 ; j < Materials[i].NumTextures ; j++ ) {
                        if ( !f.Gets( buf, sizeof( buf ) ) ) {
                            GLogger.Printf( "Unexpected EOF\n" );
                            return;
                        }
                        Materials[i].Textures[j] = Int().FromString( buf );
                    }
                }
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "subparts " ) ) ) {
            int numSubparts = 0;
            sscanf( s, "%d", &numSubparts );
            Subparts.resize( numSubparts );
            for ( int i = 0 ; i < numSubparts ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                char * name;
                s = AssetParseName( buf, &name );

                FSubpart & subpart = Subparts[i];
                subpart.Name = name;
                sscanf( s, "%d %d %d %d %d ( %f %f %f ) ( %f %f %f )", &subpart.BaseVertex, &subpart.VertexCount, &subpart.FirstIndex, &subpart.IndexCount, &subpart.Material,
                        &subpart.BoundingBox.Mins.X, &subpart.BoundingBox.Mins.Y, &subpart.BoundingBox.Mins.Z,
                        &subpart.BoundingBox.Maxs.X, &subpart.BoundingBox.Maxs.Y, &subpart.BoundingBox.Maxs.Z );
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "verts " ) ) ) {
            int numVerts = 0;
            sscanf( s, "%d", &numVerts );
            Vertices.ResizeInvalidate( numVerts );
            for ( int i = 0 ; i < numVerts ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                FMeshVertex & v = Vertices[i];

                sscanf( buf, "( %f %f %f ) ( %f %f ) ( %f %f %f ) %f ( %f %f %f )\n",
                        &v.Position.X,&v.Position.Y,&v.Position.Z,
                        &v.TexCoord.X,&v.TexCoord.Y,
                        &v.Tangent.X,&v.Tangent.Y,&v.Tangent.Z,
                        &v.Handedness,
                        &v.Normal.X,&v.Normal.Y,&v.Normal.Z );
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "indices " ) ) ) {
            int numIndices = 0;
            sscanf( s, "%d", &numIndices );
            Indices.ResizeInvalidate( numIndices );
            for ( int i = 0 ; i < numIndices ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                Indices[i] = UInt().FromString( buf );
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "weights " ) ) ) {
            int numWeights = 0;
            sscanf( s, "%d", &numWeights );
            Weights.ResizeInvalidate( numWeights );
            for ( int i = 0 ; i < numWeights ; i++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                FMeshVertexJoint & w = Weights[i];

                int d[8];

                sscanf( buf, "%d %d %d %d %d %d %d %d", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7] );

                for ( int n = 0 ; n < 8 ; n++ ) {
                    d[n] = FMath::Max( d[n], 0 );
                    d[n] = FMath::Min( d[n], 255 );
                }

                w.JointIndices[0] = d[0];
                w.JointIndices[1] = d[1];
                w.JointIndices[2] = d[2];
                w.JointIndices[3] = d[3];
                w.JointWeights[0] = d[4];
                w.JointWeights[1] = d[5];
                w.JointWeights[2] = d[6];
                w.JointWeights[3] = d[7];
            }
        } else {
            GLogger.Printf( "Unknown tag1\n" );
        }
    }

    if ( Weights.Size() > 0 && Vertices.Size() != Weights.Size() ) {
        GLogger.Printf( "Warning: num weights != num vertices\n" );
    }
}

void FMeshAsset::Write( FFileStream & f ) {
    f.Printf( "format %d %d\n", FMT_FILE_TYPE_MESH, FMT_VERSION_MESH );
    f.Printf( "textures %d\n", (int)Textures.size() );
    for ( FMaterialTexture & texture : Textures ) {
        f.Printf( "%s\n", texture.FileName.ToConstChar() );
    }
    f.Printf( "materials %d\n", Materials.Size() );
    for ( FMeshMaterial & material : Materials ) {
        f.Printf( "maps %d\n", material.NumTextures );
        for ( int i = 0 ; i < material.NumTextures ; i++ ) {
            f.Printf( "%d\n", material.Textures[i] );
        }
    }
    f.Printf( "subparts %d\n", (int)Subparts.size() );
    for ( FSubpart & subpart : Subparts ) {
        f.Printf( "\"%s\" %d %d %d %d %d %s %s\n", subpart.Name.ToConstChar(), subpart.BaseVertex, subpart.VertexCount, subpart.FirstIndex, subpart.IndexCount, subpart.Material, subpart.BoundingBox.Mins.ToString().ToConstChar(), subpart.BoundingBox.Maxs.ToString().ToConstChar() );
    }
    f.Printf( "verts %d\n", Vertices.Size() );
    for ( FMeshVertex & v : Vertices ) {
        f.Printf( "%s %s %s %f %s\n",
                  v.Position.ToString().ToConstChar(),
                  v.TexCoord.ToString().ToConstChar(),
                  v.Tangent.ToString().ToConstChar(),
                  v.Handedness,
                  v.Normal.ToString().ToConstChar() );
    }
    f.Printf( "indices %d\n", Indices.Size() );
    for ( unsigned int & i : Indices ) {
        f.Printf( "%d\n", i );
    }
    f.Printf( "weights %d\n", Weights.Size() );
    for ( FMeshVertexJoint & v : Weights ) {
        f.Printf( "%d %d %d %d %d %d %d %d\n",
                  v.JointIndices[0],v.JointIndices[1],v.JointIndices[2],v.JointIndices[3],
                v.JointWeights[0],v.JointWeights[1],v.JointWeights[2],v.JointWeights[3] );
    }
}

void CalcTangentSpace( FMeshVertex * _VertexArray, unsigned int _NumVerts, unsigned int const * _IndexArray, unsigned int _NumIndices ) {
    Float3 binormal, tangent;

    TPodArray< Float3 > Binormals;
    Binormals.ResizeInvalidate( _NumVerts );

    for ( int i = 0; i < _NumVerts; i++ ) {
        _VertexArray[ i ].Tangent = Float3( 0 );
        Binormals[ i ] = Float3( 0 );
    }

    for ( unsigned int i = 0; i < _NumIndices; i += 3 ) {
        unsigned int a = _IndexArray[ i ];
        unsigned int b = _IndexArray[ i + 1 ];
        unsigned int c = _IndexArray[ i + 2 ];

        Float3 e1 = _VertexArray[ b ].Position - _VertexArray[ a ].Position;
        Float3 e2 = _VertexArray[ c ].Position - _VertexArray[ a ].Position;
        Float2 et1 = _VertexArray[ b ].TexCoord - _VertexArray[ a ].TexCoord;
        Float2 et2 = _VertexArray[ c ].TexCoord - _VertexArray[ a ].TexCoord;

        float denom = et1.X*et2.Y - et1.Y*et2.X;
        float scale = ( fabsf( denom ) < 0.0001f ) ? 1.0f : ( 1.0 / denom );
        tangent = ( e1 * et2.Y - e2 * et1.Y ) * scale;
        binormal = ( e2 * et1.X - e1 * et2.X ) * scale;

        _VertexArray[ a ].Tangent += tangent;
        _VertexArray[ b ].Tangent += tangent;
        _VertexArray[ c ].Tangent += tangent;

        Binormals[ a ] += binormal;
        Binormals[ b ] += binormal;
        Binormals[ c ] += binormal;
    }

    for ( int i = 0; i < _NumVerts; i++ ) {
        const Float3 & n = _VertexArray[ i ].Normal;
        const Float3 & t = _VertexArray[ i ].Tangent;

        _VertexArray[ i ].Tangent = ( t - n * FMath::Dot( n, t ) ).Normalized();

        _VertexArray[ i ].Handedness = CalcHandedness( t, Binormals[ i ].Normalized(), n );
    }
}

void CreateBoxMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, const Float3 & _Size, float _TexCoordScale ) {
    const unsigned int indices[ 6 * 6 ] =
    { 0,1,2,2,3,0, // front face
        4,5,6,6,7,4, // back face

        5 + 8 * 1,0 + 8 * 1,3 + 8 * 1,3 + 8 * 1,6 + 8 * 1,5 + 8 * 1, // left face
        1 + 8 * 1,4 + 8 * 1,7 + 8 * 1,7 + 8 * 1,2 + 8 * 1,1 + 8 * 1, // right face

        3 + 8 * 2,2 + 8 * 2,7 + 8 * 2,7 + 8 * 2,6 + 8 * 2,3 + 8 * 2, // top face
        1 + 8 * 2,0 + 8 * 2,5 + 8 * 2,5 + 8 * 2,4 + 8 * 2,1 + 8 * 2, // bottom face
    };

    _Vertices.ResizeInvalidate( 24 );
    _Indices.ResizeInvalidate( 36 );

    memcpy( _Indices.ToPtr(), indices, sizeof( indices ) );

    Float3 HalfSize = _Size * 0.5f;

    _Bounds.Mins = -HalfSize;
    _Bounds.Maxs = HalfSize;

    const Float3 & Mins = _Bounds.Mins;
    const Float3 & Maxs = _Bounds.Maxs;

    FMeshVertex * pVerts = _Vertices.ToPtr();

    pVerts[ 0 + 8 * 0 ].Position = Float3( Mins.X, Mins.Y, Maxs.Z ); // 0
    pVerts[ 0 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 0 + 8 * 0 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 1 + 8 * 0 ].Position = Float3( Maxs.X, Mins.Y, Maxs.Z ); // 1
    pVerts[ 1 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 1 + 8 * 0 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 2 + 8 * 0 ].Position = Float3( Maxs.X, Maxs.Y, Maxs.Z ); // 2
    pVerts[ 2 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 2 + 8 * 0 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 3 + 8 * 0 ].Position = Float3( Mins.X, Maxs.Y, Maxs.Z ); // 3
    pVerts[ 3 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 3 + 8 * 0 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;


    pVerts[ 4 + 8 * 0 ].Position = Float3( Maxs.X, Mins.Y, Mins.Z ); // 4
    pVerts[ 4 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 4 + 8 * 0 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 5 + 8 * 0 ].Position = Float3( Mins.X, Mins.Y, Mins.Z ); // 5
    pVerts[ 5 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 5 + 8 * 0 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 6 + 8 * 0 ].Position = Float3( Mins.X, Maxs.Y, Mins.Z ); // 6
    pVerts[ 6 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 6 + 8 * 0 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 7 + 8 * 0 ].Position = Float3( Maxs.X, Maxs.Y, Mins.Z ); // 7
    pVerts[ 7 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 7 + 8 * 0 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;


    pVerts[ 0 + 8 * 1 ].Position = Float3( Mins.X, Mins.Y, Maxs.Z ); // 0
    pVerts[ 0 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 0 + 8 * 1 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 1 + 8 * 1 ].Position = Float3( Maxs.X, Mins.Y, Maxs.Z ); // 1
    pVerts[ 1 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 1 + 8 * 1 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 2 + 8 * 1 ].Position = Float3( Maxs.X, Maxs.Y, Maxs.Z ); // 2
    pVerts[ 2 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 2 + 8 * 1 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    pVerts[ 3 + 8 * 1 ].Position = Float3( Mins.X, Maxs.Y, Maxs.Z ); // 3
    pVerts[ 3 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 3 + 8 * 1 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;


    pVerts[ 4 + 8 * 1 ].Position = Float3( Maxs.X, Mins.Y, Mins.Z ); // 4
    pVerts[ 4 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 4 + 8 * 1 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 5 + 8 * 1 ].Position = Float3( Mins.X, Mins.Y, Mins.Z ); // 5
    pVerts[ 5 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 5 + 8 * 1 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 6 + 8 * 1 ].Position = Float3( Mins.X, Maxs.Y, Mins.Z ); // 6
    pVerts[ 6 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 6 + 8 * 1 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    pVerts[ 7 + 8 * 1 ].Position = Float3( Maxs.X, Maxs.Y, Mins.Z ); // 7
    pVerts[ 7 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 7 + 8 * 1 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;


    pVerts[ 1 + 8 * 2 ].Position = Float3( Maxs.X, Mins.Y, Maxs.Z ); // 1
    pVerts[ 1 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 1 + 8 * 2 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 0 + 8 * 2 ].Position = Float3( Mins.X, Mins.Y, Maxs.Z ); // 0
    pVerts[ 0 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 0 + 8 * 2 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    pVerts[ 5 + 8 * 2 ].Position = Float3( Mins.X, Mins.Y, Mins.Z ); // 5
    pVerts[ 5 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 5 + 8 * 2 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 4 + 8 * 2 ].Position = Float3( Maxs.X, Mins.Y, Mins.Z ); // 4
    pVerts[ 4 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 4 + 8 * 2 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;


    pVerts[ 3 + 8 * 2 ].Position = Float3( Mins.X, Maxs.Y, Maxs.Z ); // 3
    pVerts[ 3 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 3 + 8 * 2 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 2 + 8 * 2 ].Position = Float3( Maxs.X, Maxs.Y, Maxs.Z ); // 2
    pVerts[ 2 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 2 + 8 * 2 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 7 + 8 * 2 ].Position = Float3( Maxs.X, Maxs.Y, Mins.Z ); // 7
    pVerts[ 7 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 7 + 8 * 2 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 6 + 8 * 2 ].Position = Float3( Mins.X, Maxs.Y, Mins.Z ); // 6
    pVerts[ 6 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 6 + 8 * 2 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    CalcTangentSpace( pVerts, _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateSphereMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _HDiv, int _VDiv ) {
    int x, y;
    float VerticalAngle, HorizontalAngle;
    const float RadWidth = _Radius;
    const float RadHeight = _Radius;
    unsigned int Quad[ 4 ];

    _Vertices.ResizeInvalidate( ( _VDiv + 1 )*( _HDiv + 1 ) );
    _Indices.ResizeInvalidate( _VDiv * _HDiv * 6 );

    _Bounds.Mins.X = _Bounds.Mins.Y = _Bounds.Mins.Z = -_Radius;
    _Bounds.Maxs.X = _Bounds.Maxs.Y = _Bounds.Maxs.Z = _Radius;

    FMeshVertex * pVerts = _Vertices.ToPtr();
    FMeshVertex * pVert;

    const float VerticalStep = FMath::_PI / _HDiv;
    const float HorizontalStep = FMath::_2PI / _VDiv;
    const float VerticalScale = 1.0f / _VDiv;
    const float HorizontalScale = 1.0f / _HDiv;

    for ( y = 0, VerticalAngle = -FMath::_HALF_PI; y <= _HDiv; y++ ) {
        float h, r;
        FMath::RadSinCos( VerticalAngle, h, r );
        h *= RadHeight;
        r *= RadWidth;
        for ( x = 0, HorizontalAngle = 0; x <= _VDiv; x++ ) {
            float s, c;
            FMath::RadSinCos( HorizontalAngle, s, c );
            pVert = pVerts + ( y*( _VDiv + 1 ) + x );
            pVert->Position = Float3( r*c, h, r*s );
            pVert->TexCoord = Float2( 1.0f - static_cast< float >( x ) * VerticalScale, 1.0f - static_cast< float >( y ) * HorizontalScale ) * _TexCoordScale;
            pVert->Normal = pVert->Position / _Radius;
            HorizontalAngle += HorizontalStep;
        }
        VerticalAngle += VerticalStep;
    }

    unsigned int * pIndices = _Indices.ToPtr();
    for ( y = 0; y < _HDiv; y++ ) {
        int y2 = y + 1;
        for ( x = 0; x < _VDiv; x++ ) {
            int x2 = x + 1;

            Quad[ 0 ] = y  * ( _VDiv + 1 ) + x;
            Quad[ 1 ] = y2 * ( _VDiv + 1 ) + x;
            Quad[ 2 ] = y2 * ( _VDiv + 1 ) + x2;
            Quad[ 3 ] = y  * ( _VDiv + 1 ) + x2;

            *pIndices++ = Quad[ 0 ];
            *pIndices++ = Quad[ 1 ];
            *pIndices++ = Quad[ 2 ];
            *pIndices++ = Quad[ 2 ];
            *pIndices++ = Quad[ 3 ];
            *pIndices++ = Quad[ 0 ];
        }
    }

    CalcTangentSpace( pVerts, _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreatePlaneMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale ) {
    _Vertices.ResizeInvalidate( 4 );
    _Indices.ResizeInvalidate( 6 );

    float HalfWidth = _Width * 0.5f;
    float HalfHeight = _Height * 0.5f;

    const FMeshVertex Verts[ 4 ] = {
        { Float3( -HalfWidth,0,-HalfHeight ), Float2( 0,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
        { Float3( -HalfWidth,0,HalfHeight ), Float2( 0,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
        { Float3( HalfWidth,0,HalfHeight ), Float2( _TexCoordScale,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
        { Float3( HalfWidth,0,-HalfHeight ), Float2( _TexCoordScale,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) }
    };

    memcpy( _Vertices.ToPtr(), &Verts, 4 * sizeof( FMeshVertex ) );

    const unsigned int Indices[ 6 ] = { 0,1,2,2,3,0 };
    memcpy( _Indices.ToPtr(), &Indices, sizeof( Indices ) );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );

    _Bounds.Mins.X = -HalfWidth;
    _Bounds.Mins.Y = 0.0f;
    _Bounds.Mins.Z = -HalfHeight;
    _Bounds.Maxs.X = HalfWidth;
    _Bounds.Maxs.Y = 0.0f;
    _Bounds.Maxs.Z = HalfHeight;
}

void CreatePatchMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds,
    Float3 const & Corner00,
    Float3 const & Corner10,
    Float3 const & Corner01,
    Float3 const & Corner11,
    int resx, int resy,
    float _TexCoordScale,
    bool _TwoSided ) {

    if ( resx < 2 ) {
        resx = 2;
    }

    if ( resy < 2 ) {
        resy = 2;
    }

    const int vertexCount = resx * resy;
    const int indexCount = ( resx - 1 ) * ( resy - 1 ) * 6;

    Float3 normal = ( Corner10 - Corner00 ).Cross( Corner01 - Corner00 ).Normalized();

    _Vertices.ResizeInvalidate( _TwoSided ? vertexCount << 1 : vertexCount );
    _Indices.ResizeInvalidate( _TwoSided ? indexCount << 1 : indexCount );

    FMeshVertex * pVert = _Vertices.ToPtr();
    unsigned int * pIndices = _Indices.ToPtr();

    for ( int y = 0; y < resy; ++y ) {
        const float lerpY = y / ( float )( resy - 1 );
        const Float3 py0 = Corner00.Lerp( Corner01, lerpY );
        const Float3 py1 = Corner10.Lerp( Corner11, lerpY );
        const float ty = lerpY * _TexCoordScale;

        for ( int x = 0; x < resx; ++x ) {
            const float lerpX = x / ( float )( resx - 1 );

            pVert->Position = py0.Lerp( py1, lerpX );
            pVert->TexCoord.X = lerpX * _TexCoordScale;
            pVert->TexCoord.Y = ty;
            pVert->Normal = normal;

            ++pVert;
        }
    }

    if ( _TwoSided ) {
        normal = -normal;

        for ( int y = 0; y < resy; ++y ) {
            const float lerpY = y / ( float )( resy - 1 );
            const Float3 py0 = Corner00.Lerp( Corner01, lerpY );
            const Float3 py1 = Corner10.Lerp( Corner11, lerpY );
            const float ty = lerpY * _TexCoordScale;

            for ( int x = 0; x < resx; ++x ) {
                const float lerpX = x / ( float )( resx - 1 );

                pVert->Position = py0.Lerp( py1, lerpX );
                pVert->TexCoord.X = lerpX * _TexCoordScale;
                pVert->TexCoord.Y = ty;
                pVert->Normal = normal;

                ++pVert;
            }
        }
    }

    for ( int y = 0; y < resy; ++y ) {

        int index0 = y*resx;
        int index1 = ( y + 1 )*resx;

        for ( int x = 0; x < resx; ++x ) {
            int quad00 = index0 + x;
            int quad01 = index0 + x + 1;
            int quad10 = index1 + x;
            int quad11 = index1 + x + 1;

            if ( ( x + 1 ) < resx && ( y + 1 ) < resy ) {
                *pIndices++ = quad00;
                *pIndices++ = quad10;
                *pIndices++ = quad11;
                *pIndices++ = quad11;
                *pIndices++ = quad01;
                *pIndices++ = quad00;
            }
        }
    }

    if ( _TwoSided ) {
        for ( int y = 0; y < resy; ++y ) {

            int index0 = vertexCount + y*resx;
            int index1 = vertexCount + ( y + 1 )*resx;

            for ( int x = 0; x < resx; ++x ) {
                int quad00 = index0 + x;
                int quad01 = index0 + x + 1;
                int quad10 = index1 + x;
                int quad11 = index1 + x + 1;

                if ( ( x + 1 ) < resx && ( y + 1 ) < resy ) {
                    *pIndices++ = quad00;
                    *pIndices++ = quad01;
                    *pIndices++ = quad11;
                    *pIndices++ = quad11;
                    *pIndices++ = quad10;
                    *pIndices++ = quad00;
                }
            }
        }
    }

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );

    _Bounds.Clear();
    _Bounds.AddPoint( Corner00 );
    _Bounds.AddPoint( Corner01 );
    _Bounds.AddPoint( Corner10 );
    _Bounds.AddPoint( Corner11 );
}

void CreateCylinderMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _VDiv ) {
    int i, j;
    float Angle;
    const float RadWidth = _Radius;
    const float RadHeight = _Height * 0.5f;
    const float InvRadius = 1.0f / _Radius;
    unsigned int Quad[ 4 ];

    _Vertices.ResizeInvalidate( 6 * ( _VDiv + 1 ) );
    _Indices.ResizeInvalidate( 3 * _VDiv * 6 );

    _Bounds.Mins.X = -RadWidth;
    _Bounds.Mins.Z = -RadWidth;
    _Bounds.Mins.Y = -RadHeight;

    _Bounds.Maxs.X = RadWidth;
    _Bounds.Maxs.Z = RadWidth;
    _Bounds.Maxs.Y = RadHeight;

    FMeshVertex * pVerts = _Vertices.ToPtr();

    int FirstVertex = 0;

    // Rings

    for ( j = 0; j <= _VDiv; j++ ) {
        pVerts[ FirstVertex + j ].Position = Float3( 0.0f, -RadHeight, 0.0f );
        pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 0.0f ) * _TexCoordScale;
        pVerts[ FirstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
    }
    FirstVertex += _VDiv + 1;

    for ( j = 0, Angle = 0; j <= _VDiv; j++ ) {
        float s, c;
        FMath::RadSinCos( Angle, s, c );
        pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, -RadHeight, RadWidth*s );
        pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 1.0f ) * _TexCoordScale;
        pVerts[ FirstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
        Angle += FMath::_2PI / ( _VDiv );
    }
    FirstVertex += _VDiv + 1;

    for ( j = 0, Angle = 0; j <= _VDiv; j++ ) {
        float s, c;
        FMath::RadSinCos( Angle, s, c );
        pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, -RadHeight, RadWidth*s );
        pVerts[ FirstVertex + j ].TexCoord = Float2( 1.0f - j / float( _VDiv ), 1.0f ) * _TexCoordScale;
        pVerts[ FirstVertex + j ].Normal = Float3( pVerts[ FirstVertex + j ].Position.X * InvRadius, 0.0f, pVerts[ FirstVertex + j ].Position.Z * InvRadius );
        Angle += FMath::_2PI / ( _VDiv );
    }
    FirstVertex += _VDiv + 1;

    for ( j = 0, Angle = 0; j <= _VDiv; j++ ) {
        float s, c;
        FMath::RadSinCos( Angle, s, c );
        pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, RadHeight, RadWidth*s );
        pVerts[ FirstVertex + j ].TexCoord = Float2( 1.0f - j / float( _VDiv ), 0.0f ) * _TexCoordScale;
        pVerts[ FirstVertex + j ].Normal = Float3( pVerts[ FirstVertex + j ].Position.X * InvRadius, 0.0f, pVerts[ FirstVertex + j ].Position.Z * InvRadius );
        Angle += FMath::_2PI / ( _VDiv );
    }
    FirstVertex += _VDiv + 1;

    for ( j = 0, Angle = 0; j <= _VDiv; j++ ) {
        float s, c;
        FMath::RadSinCos( Angle, s, c );
        pVerts[ FirstVertex + j ].Position = Float3( RadWidth*c, RadHeight, RadWidth*s );
        pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 0.0f ) * _TexCoordScale;
        pVerts[ FirstVertex + j ].Normal = Float3( 0, 1.0f, 0 );
        Angle += FMath::_2PI / ( _VDiv );
    }
    FirstVertex += _VDiv + 1;

    for ( j = 0; j <= _VDiv; j++ ) {
        pVerts[ FirstVertex + j ].Position = Float3( 0.0f, RadHeight, 0.0f );
        pVerts[ FirstVertex + j ].TexCoord = Float2( j / float( _VDiv ), 1.0f ) * _TexCoordScale;
        pVerts[ FirstVertex + j ].Normal = Float3( 0, 1.0f, 0 );
    }
    FirstVertex += _VDiv + 1;

    AN_Assert( FirstVertex == _Vertices.Size() );

    // generate indices

    unsigned int * pIndices = _Indices.ToPtr();

    _Indices.Memset( 0 );

    FirstVertex = 0;
    for ( i = 0; i < 3; i++ ) {
        for ( j = 0; j < _VDiv; j++ ) {
            Quad[ 3 ] = FirstVertex + j;
            Quad[ 2 ] = FirstVertex + j + 1;
            Quad[ 1 ] = FirstVertex + j + 1 + ( _VDiv + 1 );
            Quad[ 0 ] = FirstVertex + j + ( _VDiv + 1 );

            *pIndices++ = Quad[ 0 ];
            *pIndices++ = Quad[ 1 ];
            *pIndices++ = Quad[ 2 ];
            *pIndices++ = Quad[ 2 ];
            *pIndices++ = Quad[ 3 ];
            *pIndices++ = Quad[ 0 ];
        }
        FirstVertex += ( _VDiv + 1 ) * 2;
    }

    AN_Assert( FirstVertex == _Vertices.Size() );

    CalcTangentSpace( pVerts, _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}
