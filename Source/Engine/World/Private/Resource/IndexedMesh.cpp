/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/Resource/IndexedMesh.h>
#include <World/Public/Resource/Asset.h>
#include <World/Public/Resource/Animation.h>
#include <World/Public/Level.h>
#include <World/Public/Base/ResourceManager.h>

#include <Runtime/Public/ScopedTimeCheck.h>
#include <Runtime/Public/Runtime.h>

#include <Core/Public/Logger.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Core/Public/BV/BvIntersect.h>

AN_CLASS_META( AIndexedMesh )
AN_CLASS_META( AIndexedMeshSubpart )
AN_CLASS_META( ALightmapUV )
AN_CLASS_META( AVertexLight )
AN_CLASS_META( ATreeAABB )
AN_CLASS_META( ASocketDef )
AN_CLASS_META( AProceduralMesh )

///////////////////////////////////////////////////////////////////////////////////////////////////////

AIndexedMesh::AIndexedMesh() {
    static TStaticResourceFinder< ASkeleton > SkeletonResource( _CTS( "/Default/Skeleton/Default" ) );
    Skeleton = SkeletonResource.GetObject();

    BoundingBox.Clear();
}

AIndexedMesh::~AIndexedMesh() {
    Purge();

    AN_ASSERT( LightmapUVs.IsEmpty() );
    AN_ASSERT( VertexLightChannels.IsEmpty() );
}

void AIndexedMesh::Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh ) {
    Purge();

    bSkinnedMesh = _SkinnedMesh;
    bBoundingBoxDirty = true;
    BoundingBox.Clear();

    Vertices.ResizeInvalidate( _NumVertices );
    Indices.ResizeInvalidate( _NumIndices );

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    VertexHandle = vertexMemory->AllocateVertex( Vertices.Size() * sizeof( SMeshVertex ), nullptr, GetVertexMemory, this );
    IndexHandle = vertexMemory->AllocateIndex( Indices.Size() * sizeof( unsigned int ), nullptr, GetIndexMemory, this );

    if ( bSkinnedMesh ) {
        Weights.ResizeInvalidate( _NumVertices );
        WeightsHandle = vertexMemory->AllocateVertex( Weights.Size() * sizeof( SMeshVertexSkin ), nullptr, GetWeightMemory, this );
    }

    _NumSubparts = _NumSubparts < 1 ? 1 : _NumSubparts;

    Subparts.ResizeInvalidate( _NumSubparts );
    for ( int i = 0 ; i < _NumSubparts ; i++ ) {
        AIndexedMeshSubpart * subpart = CreateInstanceOf< AIndexedMeshSubpart >();
        subpart->AddRef();
        subpart->OwnerMesh = this;
        Subparts[i] = subpart;
    }

    if ( _NumSubparts == 1 ) {
        AIndexedMeshSubpart * subpart = Subparts[0];
        subpart->BaseVertex = 0;
        subpart->FirstIndex = 0;
        subpart->VertexCount = Vertices.Size();
        subpart->IndexCount = Indices.Size();
    }

    InvalidateChannels();
}

void AIndexedMesh::Purge() {
    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        subpart->OwnerMesh = nullptr;
        subpart->RemoveRef();
    }

    Subparts.Clear();

    InvalidateChannels();

    for ( ASocketDef * socket : Sockets ) {
        socket->RemoveRef();
    }

    Sockets.Clear();

    Skin.JointIndices.Clear();
    Skin.OffsetMatrices.Clear();

    CollisionModel.Reset();

    Vertices.Free();
    Weights.Free();
    Indices.Free();

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Deallocate( VertexHandle );
    VertexHandle = nullptr;

    vertexMemory->Deallocate( IndexHandle );
    IndexHandle = nullptr;

    vertexMemory->Deallocate( WeightsHandle );
    WeightsHandle = nullptr;
}

void AIndexedMesh::InvalidateChannels() {
    for ( ALightmapUV * channel : LightmapUVs ) {
        channel->Invalidate();
    }

    for ( AVertexLight * channel : VertexLightChannels ) {
        channel->Invalidate();
    }
}

static AIndexedMeshSubpart * ReadIndexedMeshSubpart( IBinaryStream & f ) {
    AString name;
    int32_t baseVertex;
    uint32_t firstIndex;
    uint32_t vertexCount;
    uint32_t indexCount;
    //AString dummy;
    BvAxisAlignedBox boundingBox;

    f.ReadObject( name );
    baseVertex = f.ReadInt32();
    firstIndex = f.ReadUInt32();
    vertexCount = f.ReadUInt32();
    indexCount = f.ReadUInt32();
    //f.ReadObject( dummy ); // deprecated field
    f.ReadObject( boundingBox );

    AIndexedMeshSubpart * subpart = CreateInstanceOf< AIndexedMeshSubpart >();
    subpart->AddRef();
    subpart->SetObjectName( name );
    subpart->SetBaseVertex( baseVertex );
    subpart->SetFirstIndex( firstIndex );
    subpart->SetVertexCount( vertexCount );
    subpart->SetIndexCount( indexCount );
    //subpart->SetMaterialInstance( GetOrCreateResource< AMaterialInstance >( materialInstance.CStr() ) );
    subpart->SetBoundingBox( boundingBox );
    return subpart;
}

static ASocketDef * ReadSocket( IBinaryStream & f ) {
    AString name;
    uint32_t jointIndex;

    f.ReadObject( name );
    jointIndex = f.ReadUInt32();

    ASocketDef * socket = CreateInstanceOf< ASocketDef >();
    socket->AddRef();
    socket->SetObjectName( name );
    socket->JointIndex = jointIndex;

    f.ReadObject( socket->Position );
    f.ReadObject( socket->Scale );
    f.ReadObject( socket->Rotation );

    return socket;
}

bool AIndexedMesh::LoadResource( IBinaryStream & Stream ) {
    AScopedTimeCheck ScopedTime( Stream.GetFileName() );

    Purge();

    AString text;
    text.FromFile( Stream );

    SDocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = text.CStr();
    deserializeInfo.bInsitu = true;

    ADocument doc;
    doc.DeserializeFromString( deserializeInfo );

    ADocMember * member;

    member = doc.FindMember( "Mesh" );
    if ( !member ) {
        GLogger.Printf( "AIndexedMesh::LoadResource: invalid mesh\n" );
        return false;
    }

    AString meshFile = member->GetString();
    if ( meshFile.IsEmpty() ) {
        GLogger.Printf( "AIndexedMesh::LoadResource: invalid mesh\n" );
        return false;
    }

    ABinaryResource * meshBinary = CreateInstanceOf< ABinaryResource >();
    meshBinary->InitializeFromFile( meshFile.CStr() );

    if ( !meshBinary->GetSizeInBytes() ) {
        GLogger.Printf( "AIndexedMesh::LoadResource: invalid mesh\n" );
        return false;
    }

    AMemoryStream meshData;
    if ( !meshData.OpenRead( meshFile.CStr(), meshBinary->GetBinaryData(), meshBinary->GetSizeInBytes() ) ) {
        GLogger.Printf( "AIndexedMesh::LoadResource: invalid mesh\n" );
        return false;
    }

    uint32_t fileFormat = meshData.ReadUInt32();

    if ( fileFormat != FMT_FILE_TYPE_MESH ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_MESH );
        return false;
    }

    uint32_t fileVersion = meshData.ReadUInt32();

    if ( fileVersion != FMT_VERSION_MESH ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_MESH );
        return false;
    }

    bool bRaycastBVH;

    AString guidStr;
    meshData.ReadObject( guidStr );

    bSkinnedMesh = meshData.ReadBool();
    //meshData.ReadBool(); // dummy (TODO: remove in the future)
    meshData.ReadObject( BoundingBox );
    meshData.ReadArrayUInt32( Indices );
    meshData.ReadArrayOfStructs( Vertices );
    meshData.ReadArrayOfStructs( Weights );
    bRaycastBVH = meshData.ReadBool();
    RaycastPrimitivesPerLeaf = meshData.ReadUInt16();

    uint32_t subpartsCount = meshData.ReadUInt32();
    Subparts.ResizeInvalidate( subpartsCount );
    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        Subparts[i] = ReadIndexedMeshSubpart( meshData );
    }

    member = doc.FindMember( "Subparts" );
    if ( member ) {
        ADocValue * values = member->GetArrayValues();
        int subpartIndex = 0;
        for ( ADocValue * v = values ; v && subpartIndex < Subparts.Size() ; v = v->GetNext() ) {
            Subparts[subpartIndex]->SetMaterialInstance( GetOrCreateResource< AMaterialInstance >( v->GetString().CStr() ) );
            subpartIndex++;
        }
    }

    if ( bRaycastBVH ) {
        for ( AIndexedMeshSubpart * subpart : Subparts ) {
            ATreeAABB * bvh = CreateInstanceOf< ATreeAABB >();

            bvh->Read( meshData );

            subpart->SetBVH( bvh );
        }
    }

    uint32_t socketsCount = meshData.ReadUInt32();
    Sockets.ResizeInvalidate( socketsCount );
    for ( int i = 0 ; i < Sockets.Size() ; i++ ) {
        Sockets[i] = ReadSocket( meshData );
    }

    //AString dummy;
    //meshData.ReadObject( dummy ); // deprecated field

    if ( bSkinnedMesh ) {
        meshData.ReadArrayInt32( Skin.JointIndices );
        meshData.ReadArrayOfStructs( Skin.OffsetMatrices );
    }

    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        subpart->OwnerMesh = this;
    }

    member = doc.FindMember( "Skeleton" );
    SetSkeleton( GetOrCreateResource< ASkeleton >( member ? member->GetString().CStr() : "/Default/Skeleton/Default" ) );

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    VertexHandle = vertexMemory->AllocateVertex( Vertices.Size() * sizeof( SMeshVertex ), nullptr, GetVertexMemory, this );
    IndexHandle = vertexMemory->AllocateIndex( Indices.Size() * sizeof( unsigned int ), nullptr, GetIndexMemory, this );

    if ( bSkinnedMesh ) {
        WeightsHandle = vertexMemory->AllocateVertex( Weights.Size() * sizeof( SMeshVertexSkin ), nullptr, GetWeightMemory, this );
    }

    SendVertexDataToGPU( Vertices.Size(), 0 );
    SendIndexDataToGPU( Indices.Size(), 0 );
    if ( bSkinnedMesh ) {
        SendJointWeightsToGPU( Weights.Size(), 0 );
    }

    bBoundingBoxDirty = false;

    InvalidateChannels();

    if ( !bSkinnedMesh ) {
        GenerateRigidbodyCollisions();   // TODO: load collision from file
    }
    









    

#if 0
    int i = _Path.FindExt();
    if ( !Core::Icmp( &_Path[i], ".gltf" ) ) {
        SMeshAsset asset;

        if ( !LoadGeometryGLTF( _Path.CStr(), asset ) ) {
            return false;
        }

        bool bSkinned = asset.Weights.Size() == asset.Vertices.Size();

        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/PBRMetallicRoughness" ) );

        Initialize( asset.Vertices.Size(), asset.Indices.Size(), asset.Subparts.size(), bSkinned, false );
        WriteVertexData( asset.Vertices.ToPtr(), asset.Vertices.Size(), 0 );
        WriteIndexData( asset.Indices.ToPtr(), asset.Indices.Size(), 0 );
        if ( bSkinned ) {
            WriteJointWeights( asset.Weights.ToPtr(), asset.Weights.Size(), 0 );
        }

        TPodVector< AMaterialInstance * > matInstances;
        matInstances.Resize( asset.Materials.Size() );
        for ( int j = 0; j < asset.Materials.Size(); j++ ) {
            AMaterialInstance * matInst = CreateInstanceOf< AMaterialInstance >();
            matInstances[j] = matInst;
            matInst->SetMaterial( MaterialResource.GetObject() );
            SMeshMaterial const & material = asset.Materials[j];
            for ( int n = 0; n < material.NumTextures; n++ ) {
                SMaterialTexture const & texture = asset.Textures[material.Textures[n]];
                ATexture * texObj = GetOrCreateResource< ATexture >( texture.FileName.CStr() );
                matInst->SetTexture( n, texObj );
            }
        }

        for ( int j = 0; j < GetSubparts().Size(); j++ ) {
            SSubpart const & s = asset.Subparts[j];
            AIndexedMeshSubpart * subpart = GetSubpart( j );
            subpart->SetObjectName( s.Name );
            subpart->SetBaseVertex( s.BaseVertex );
            subpart->SetFirstIndex( s.FirstIndex );
            subpart->SetVertexCount( s.VertexCount );
            subpart->SetIndexCount( s.IndexCount );
            subpart->SetBoundingBox( s.BoundingBox );
            if ( s.Material < matInstances.Size() ) {
                subpart->SetMaterialInstance( matInstances[s.Material] );
            }
        }

        GenerateRigidbodyCollisions();
        GenerateBVH();

        return true;
    }
#endif

#if 0
    uint32_t fileFormat = Stream.ReadUInt32();

    if ( fileFormat != FMT_FILE_TYPE_MESH ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_MESH );
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if ( fileVersion != FMT_VERSION_MESH ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_MESH );
        return false;
    }

    Purge();

    bool bRaycastBVH;

    AString guidStr;
    Stream.ReadObject( guidStr );

    bSkinnedMesh = Stream.ReadBool();
    Stream.ReadBool(); // dummy (TODO: remove in the future)
    Stream.ReadObject( BoundingBox );
    Stream.ReadArrayUInt32( Indices );
    Stream.ReadArrayOfStructs( Vertices );
    Stream.ReadArrayOfStructs( Weights );
    bRaycastBVH = Stream.ReadBool();
    RaycastPrimitivesPerLeaf = Stream.ReadUInt16();

    uint32_t subpartsCount = Stream.ReadUInt32();
    Subparts.ResizeInvalidate( subpartsCount );
    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        Subparts[i] = ReadIndexedMeshSubpart( Stream );
    }

    if ( bRaycastBVH ) {
        for ( AIndexedMeshSubpart * subpart : Subparts ) {
            ATreeAABB * bvh = CreateInstanceOf< ATreeAABB >();

            bvh->Read( Stream );

            subpart->SetBVH( bvh );
        }
    }

    uint32_t socketsCount = Stream.ReadUInt32();
    Sockets.ResizeInvalidate( socketsCount );
    for ( int i = 0 ; i < Sockets.Size() ; i++ ) {
        Sockets[i] = ReadSocket( Stream );
    }

    AString skeleton;
    Stream.ReadObject( skeleton );

    if ( bSkinnedMesh ) {
        Stream.ReadArrayInt32( Skin.JointIndices );
        Stream.ReadArrayOfStructs( Skin.OffsetMatrices );
    }

    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        subpart->OwnerMesh = this;
    }

    SetSkeleton( GetOrCreateResource< ASkeleton >( skeleton.CStr() ) );

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    VertexHandle = vertexMemory->AllocateVertex( Vertices.Size() * sizeof( SMeshVertex ), nullptr, GetVertexMemory, this );
    IndexHandle = vertexMemory->AllocateIndex( Indices.Size() * sizeof( unsigned int ), nullptr, GetIndexMemory, this );

    if ( bSkinnedMesh ) {
        WeightsHandle = vertexMemory->AllocateVertex( Weights.Size() * sizeof( SMeshVertexSkin ), nullptr, GetWeightMemory, this );
    }

    SendVertexDataToGPU( Vertices.Size(), 0 );
    SendIndexDataToGPU( Indices.Size(), 0 );
    if ( bSkinnedMesh ) {
        SendJointWeightsToGPU( Weights.Size(), 0 );
    }

    bBoundingBoxDirty = false;

    InvalidateChannels();

    if ( !bSkinnedMesh ) {
        GenerateRigidbodyCollisions();   // TODO: load collision from file
    }
#endif
    return true;
}

void *AIndexedMesh::GetVertexMemory( void * _This ) {
    return static_cast< AIndexedMesh * >( _This )->GetVertices();
}

void *AIndexedMesh::GetIndexMemory( void * _This ) {
    return static_cast< AIndexedMesh * >(_This)->GetIndices();
}

void *AIndexedMesh::GetWeightMemory( void * _This ) {
    return static_cast< AIndexedMesh * >(_This)->GetWeights();
}

void AIndexedMesh::GetVertexBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    if ( VertexHandle ) {
        AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset( VertexHandle, _Buffer, _Offset );
    }
}

void AIndexedMesh::GetIndexBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    if ( IndexHandle ) {
        AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset( IndexHandle, _Buffer, _Offset );
    }
}

void AIndexedMesh::GetWeightsBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    if ( WeightsHandle ) {
        AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset( WeightsHandle, _Buffer, _Offset );
    }
}

void AIndexedMesh::AddSocket( ASocketDef * _Socket ) {
    _Socket->AddRef();
    Sockets.Append( _Socket );
}

ASocketDef * AIndexedMesh::FindSocket( const char * _Name ) {
    for ( ASocketDef * socket : Sockets ) {
        if ( socket->GetObjectName().Icmp( _Name ) ) {
            return socket;
        }
    }
    return nullptr;
}

void AIndexedMesh::GenerateBVH( unsigned int PrimitivesPerLeaf ) {
    AScopedTimeCheck ScopedTime( "GenerateBVH" );

    if ( bSkinnedMesh ) {
        GLogger.Printf( "AIndexedMesh::GenerateBVH: called for skinned mesh\n" );
        return;
    }

    const unsigned int MaxPrimitivesPerLeaf = 1024;

    // Don't allow to generate large leafs
    if ( PrimitivesPerLeaf > MaxPrimitivesPerLeaf ) {
        PrimitivesPerLeaf = MaxPrimitivesPerLeaf;
    }

    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        subpart->GenerateBVH( PrimitivesPerLeaf );
    }

    RaycastPrimitivesPerLeaf = PrimitivesPerLeaf;
}

void AIndexedMesh::SetSkeleton( ASkeleton * _Skeleton ) {
    Skeleton = _Skeleton;

    if ( !Skeleton ) {
        static TStaticResourceFinder< ASkeleton > SkeletonResource( _CTS( "/Default/Skeleton/Default" ) );
        Skeleton = SkeletonResource.GetObject();
    }
}

void AIndexedMesh::SetSkin( int32_t const * _JointIndices, Float3x4 const * _OffsetMatrices, int _JointsCount ) {
    Skin.JointIndices.ResizeInvalidate( _JointsCount );
    Skin.OffsetMatrices.ResizeInvalidate( _JointsCount );

    Core::Memcpy( Skin.JointIndices.ToPtr(), _JointIndices, _JointsCount * sizeof(*_JointIndices) );
    Core::Memcpy( Skin.OffsetMatrices.ToPtr(), _OffsetMatrices, _JointsCount * sizeof(*_OffsetMatrices) );
}

void AIndexedMesh::SetCollisionModel( ACollisionModel * _CollisionModel )
{
    CollisionModel = _CollisionModel;
}

void AIndexedMesh::SetMaterialInstance( int _SubpartIndex, AMaterialInstance * _MaterialInstance ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
        return;
    }
    Subparts[_SubpartIndex]->SetMaterialInstance( _MaterialInstance );
}

void AIndexedMesh::SetBoundingBox( int _SubpartIndex, BvAxisAlignedBox const & _BoundingBox ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
        return;
    }
    Subparts[_SubpartIndex]->SetBoundingBox( _BoundingBox );
}

AIndexedMeshSubpart * AIndexedMesh::GetSubpart( int _SubpartIndex ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
        return nullptr;
    }
    return Subparts[ _SubpartIndex ];
}

bool AIndexedMesh::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "AIndexedMesh::SendVertexDataToGPU: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Update( VertexHandle, _StartVertexLocation * sizeof( SMeshVertex ), _VerticesCount * sizeof( SMeshVertex ), Vertices.ToPtr() + _StartVertexLocation );

    return true;
}

bool AIndexedMesh::WriteVertexData( SMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "AIndexedMesh::WriteVertexData: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    Core::Memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( SMeshVertex ) );

    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        subpart->bAABBTreeDirty = true;
    }

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}

bool AIndexedMesh::SendJointWeightsToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "AIndexedMesh::SendJointWeightsToGPU: Cannot write joint weights for static mesh\n" );
        return false;
    }

    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Weights.Size() ) {
        GLogger.Printf( "AIndexedMesh::SendJointWeightsToGPU: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Update( WeightsHandle, _StartVertexLocation * sizeof( SMeshVertexSkin ), _VerticesCount * sizeof( SMeshVertexSkin ), Weights.ToPtr() + _StartVertexLocation );

    return true;
}

bool AIndexedMesh::WriteJointWeights( SMeshVertexSkin const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "AIndexedMesh::WriteJointWeights: Cannot write joint weights for static mesh\n" );
        return false;
    }

    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Weights.Size() ) {
        GLogger.Printf( "AIndexedMesh::WriteJointWeights: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    Core::Memcpy( Weights.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( SMeshVertexSkin ) );

    return SendJointWeightsToGPU( _VerticesCount, _StartVertexLocation );
}

bool AIndexedMesh::SendIndexDataToGPU( int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount ) {
        return true;
    }

    if ( _StartIndexLocation + _IndexCount > Indices.Size() ) {
        GLogger.Printf( "AIndexedMesh::SendIndexDataToGPU: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Update( IndexHandle, _StartIndexLocation * sizeof( unsigned int ), _IndexCount * sizeof( unsigned int ), Indices.ToPtr() + _StartIndexLocation );

    return true;
}

bool AIndexedMesh::WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount ) {
        return true;
    }

    if ( _StartIndexLocation + _IndexCount > Indices.Size() ) {
        GLogger.Printf( "AIndexedMesh::WriteIndexData: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    Core::Memcpy( Indices.ToPtr() + _StartIndexLocation, _Indices, _IndexCount * sizeof( unsigned int ) );

    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        if ( _StartIndexLocation >= subpart->FirstIndex
            && _StartIndexLocation + _IndexCount <= subpart->FirstIndex + subpart->IndexCount ) {
            subpart->bAABBTreeDirty = true;
        }
    }

    return SendIndexDataToGPU( _IndexCount, _StartIndexLocation );
}

void AIndexedMesh::UpdateBoundingBox() {
    BoundingBox.Clear();
    for ( AIndexedMeshSubpart const * subpart : Subparts ) {
        BoundingBox.AddAABB( subpart->GetBoundingBox() );
    }
    bBoundingBoxDirty = false;
}

BvAxisAlignedBox const & AIndexedMesh::GetBoundingBox() const {
    if ( bBoundingBoxDirty ) {
        const_cast< ThisClass * >( this )->UpdateBoundingBox();
    }

    return BoundingBox;
}

void AIndexedMesh::InitializeBoxMesh( const Float3 & _Size, float _TexCoordScale ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateBoxMesh( vertices, indices, bounds, _Size, _TexCoordScale );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializeSphereMesh( float _Radius, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;
    
    CreateSphereMesh( vertices, indices, bounds, _Radius, _TexCoordScale, _NumVerticalSubdivs, _NumHorizontalSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializePlaneMeshXZ( float _Width, float _Height, float _TexCoordScale ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreatePlaneMeshXZ( vertices, indices, bounds, _Width, _Height, _TexCoordScale );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializePlaneMeshXY( float _Width, float _Height, float _TexCoordScale ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreatePlaneMeshXY( vertices, indices, bounds, _Width, _Height, _TexCoordScale );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[0]->BoundingBox = bounds;
}

void AIndexedMesh::InitializePatchMesh( Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreatePatchMesh( vertices, indices, bounds,
        Corner00, Corner10, Corner01, Corner11, _TexCoordScale, _TwoSided, _NumVerticalSubdivs, _NumHorizontalSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializeCylinderMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateCylinderMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _NumSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializeConeMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateConeMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _NumSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializeCapsuleMesh( float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateCapsuleMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _NumVerticalSubdivs, _NumHorizontalSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void AIndexedMesh::InitializeSkyboxMesh( const Float3 & _Size, float _TexCoordScale ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateSkyboxMesh( vertices, indices, bounds, _Size, _TexCoordScale );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[0]->BoundingBox = bounds;
}

void AIndexedMesh::InitializeSkydomeMesh( float _Radius, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs, bool _Hemisphere ) {
    TPodVector< SMeshVertex > vertices;
    TPodVector< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateSkydomeMesh( vertices, indices, bounds, _Radius, _TexCoordScale, _NumVerticalSubdivs, _NumHorizontalSubdivs, _Hemisphere );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[0]->BoundingBox = bounds;
}

void AIndexedMesh::LoadInternalResource( const char * _Path ) {

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Box" ) ) {
        InitializeBoxMesh( Float3(1), 1 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionBox * collisionBody = CollisionModel->CreateBody< ACollisionBox >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Sphere" ) ) {
        InitializeSphereMesh( 0.5f, 1 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionSphere * collisionBody = CollisionModel->CreateBody< ACollisionSphere >();
        collisionBody->Radius = 0.5f;
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Cylinder" ) ) {
        InitializeCylinderMesh( 0.5f, 1, 1 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionCylinder * collisionBody = CollisionModel->CreateBody< ACollisionCylinder >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Cone" ) ) {
        InitializeConeMesh( 0.5f, 1, 1 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionCone * collisionBody = CollisionModel->CreateBody< ACollisionCone >();
        collisionBody->Radius = 0.5f;
        collisionBody->Height = 1.0f;
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Capsule" ) ) {
        InitializeCapsuleMesh( 0.5f, 1.0f, 1 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionCapsule * collisionBody = CollisionModel->CreateBody< ACollisionCapsule >();
        collisionBody->Radius = 0.5f;
        collisionBody->Height = 1;
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/PlaneXZ") ) {
        InitializePlaneMeshXZ( 256, 256, 256 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionBox * box = CollisionModel->CreateBody< ACollisionBox >();
        box->HalfExtents.X = 128;
        box->HalfExtents.Y = 0.1f;
        box->HalfExtents.Z = 128;
        box->Position.Y -= box->HalfExtents.Y;
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/PlaneXY" ) ) {
        InitializePlaneMeshXY( 256, 256, 256 );

        CollisionModel = CreateInstanceOf< ACollisionModel >();
        ACollisionBox * box = CollisionModel->CreateBody< ACollisionBox >();
        box->HalfExtents.X = 128;
        box->HalfExtents.Y = 128;
        box->HalfExtents.Z = 0.1f;
        box->Position.Z -= box->HalfExtents.Z;
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Skybox" ) ) {
        InitializeSkyboxMesh( Float3( 1 ), 1 );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/Skydome" ) ) {
        InitializeSkydomeMesh( 0.5f, 1, 32, 32, false );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Meshes/SkydomeHemisphere" ) ) {
        InitializeSkydomeMesh( 0.5f, 1, 16, 32, true );
        return;
    }    

    GLogger.Printf( "Unknown internal mesh %s\n", _Path );

    LoadInternalResource( "/Default/Meshes/Box" );
}

void AIndexedMesh::GenerateRigidbodyCollisions() {
    AScopedTimeCheck ScopedTime( "GenerateRigidbodyCollisions" );

    ACollisionTriangleSoupData * tris = CreateInstanceOf< ACollisionTriangleSoupData >();
    tris->Initialize( (float *)&Vertices.ToPtr()->Position, sizeof( Vertices[0] ), Vertices.Size(),
        Indices.ToPtr(), Indices.Size(), Subparts.ToPtr(), Subparts.Size() );

    ACollisionTriangleSoupBVHData * bvh = CreateInstanceOf< ACollisionTriangleSoupBVHData >();
    bvh->TrisData = tris;
    bvh->BuildBVH();

    CollisionModel = CreateInstanceOf< ACollisionModel >();
    ACollisionTriangleSoupBVH * CollisionBody = CollisionModel->CreateBody< ACollisionTriangleSoupBVH >();
    CollisionBody->BvhData = bvh;
}

void AIndexedMesh::GenerateSoftbodyFacesFromMeshIndices() {
    AScopedTimeCheck ScopedTime( "GenerateSoftbodyFacesFromMeshIndices" );

    int totalIndices = 0;

    for ( AIndexedMeshSubpart const * subpart : Subparts ) {
        totalIndices += subpart->IndexCount;
    }

    SoftbodyFaces.ResizeInvalidate( totalIndices / 3 );

    int faceIndex = 0;

    unsigned int const * indices = Indices.ToPtr();

    for ( AIndexedMeshSubpart const * subpart : Subparts ) {
        for ( int i = 0; i < subpart->IndexCount; i += 3 ) {
            SSoftbodyFace & face = SoftbodyFaces[ faceIndex++ ];

            face.Indices[ 0 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i ];
            face.Indices[ 1 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i + 1 ];
            face.Indices[ 2 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i + 2 ];
        }
    }
}

void AIndexedMesh::GenerateSoftbodyLinksFromFaces() {
    AScopedTimeCheck ScopedTime( "GenerateSoftbodyLinksFromFaces" );

    TPodVector< bool > checks;
    unsigned int * indices;

    checks.Resize( Vertices.Size()*Vertices.Size() );
    checks.ZeroMem();

    SoftbodyLinks.Clear();

    for ( SSoftbodyFace & face : SoftbodyFaces ) {
        indices = face.Indices;

        for ( int j = 2, k = 0; k < 3; j = k++ ) {

            unsigned int index_j_k = indices[ j ] + indices[ k ]*Vertices.Size();

            // Check if link not exists
            if ( !checks[ index_j_k ] ) {

                unsigned int index_k_j = indices[ k ] + indices[ j ]*Vertices.Size();

                // Mark link exits
                checks[ index_j_k ] = checks[ index_k_j ] = true;

                // Append link
                SSoftbodyLink & link = SoftbodyLinks.Append();
                link.Indices[0] = indices[ j ];
                link.Indices[1] = indices[ k ];
            }
        }
        #undef IDX
    }
}

bool AIndexedMesh::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, bool bCullBackFace, TPodVector< STriangleHitResult > & _HitResult ) const {
    bool ret = false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / _RayDir.X;
    invRayDir.Y = 1.0f / _RayDir.Y;
    invRayDir.Z = 1.0f / _RayDir.Z;

    if ( !BvRayIntersectBox( _RayStart, invRayDir, GetBoundingBox(), boxMin, boxMax ) || boxMin >= _Distance ) {
        return false;
    }

    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        AIndexedMeshSubpart * subpart = Subparts[i];
        ret |= subpart->Raycast( _RayStart, _RayDir, invRayDir, _Distance, bCullBackFace, _HitResult );
    }
    return ret;
}

bool AIndexedMesh::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, bool bCullBackFace, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], int & _SubpartIndex ) const {
    bool ret = false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / _RayDir.X;
    invRayDir.Y = 1.0f / _RayDir.Y;
    invRayDir.Z = 1.0f / _RayDir.Z;

    if ( !BvRayIntersectBox( _RayStart, invRayDir, GetBoundingBox(), boxMin, boxMax ) || boxMin >= _Distance ) {
        return false;
    }

    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        AIndexedMeshSubpart * subpart = Subparts[i];
        if ( subpart->RaycastClosest( _RayStart, _RayDir, invRayDir, _Distance, bCullBackFace, _HitLocation, _HitUV, _HitDistance, _Indices ) ) {
            _SubpartIndex = i;
            _Distance = _HitDistance;
            ret = true;
        }
    }

    return ret;
}

void AIndexedMesh::DrawBVH( ADebugRenderer * InRenderer, Float3x4 const & _TransformMatrix ) {
    for ( AIndexedMeshSubpart * subpart : Subparts ) {
        subpart->DrawBVH( InRenderer, _TransformMatrix );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

AIndexedMeshSubpart::AIndexedMeshSubpart() {
    BoundingBox.Clear();

    static TStaticResourceFinder< AMaterialInstance > DefaultMaterialInstance( _CTS( "/Default/MaterialInstance/Default" ) );
    MaterialInstance = DefaultMaterialInstance.GetObject();
}

AIndexedMeshSubpart::~AIndexedMeshSubpart() {
}

void AIndexedMeshSubpart::SetBaseVertex( int _BaseVertex ) {
    BaseVertex = _BaseVertex;
    bAABBTreeDirty = true;
}

void AIndexedMeshSubpart::SetFirstIndex( int _FirstIndex ) {
    FirstIndex = _FirstIndex;
    bAABBTreeDirty = true;
}

void AIndexedMeshSubpart::SetVertexCount( int _VertexCount ) {
    VertexCount = _VertexCount;
}

void AIndexedMeshSubpart::SetIndexCount( int _IndexCount ) {
    IndexCount = _IndexCount;
    bAABBTreeDirty = true;
}

void AIndexedMeshSubpart::SetMaterialInstance( AMaterialInstance * _MaterialInstance ) {
    MaterialInstance = _MaterialInstance;

    if ( !MaterialInstance ) {
        static TStaticResourceFinder< AMaterialInstance > DefaultMaterialInstance( _CTS( "/Default/MaterialInstance/Default" ) );
        MaterialInstance = DefaultMaterialInstance.GetObject();
    }
}

void AIndexedMeshSubpart::SetBoundingBox( BvAxisAlignedBox const & _BoundingBox ) {
    BoundingBox = _BoundingBox;

    if ( OwnerMesh ) {
        OwnerMesh->bBoundingBoxDirty = true;
    }
}

void AIndexedMeshSubpart::GenerateBVH( unsigned int PrimitivesPerLeaf ) {
    // TODO: Try KD-tree

    if ( OwnerMesh )
    {
        AABBTree = CreateInstanceOf< ATreeAABB >();
        AABBTree->InitializeTriangleSoup( OwnerMesh->Vertices.ToPtr(), OwnerMesh->Indices.ToPtr() + FirstIndex, IndexCount, BaseVertex, PrimitivesPerLeaf );
        bAABBTreeDirty = false;
    }
}

void AIndexedMeshSubpart::SetBVH( ATreeAABB * BVH ) {
    AABBTree = BVH;
    bAABBTreeDirty = false;
}

bool AIndexedMeshSubpart::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _InvRayDir, float _Distance, bool bCullBackFace, TPodVector< STriangleHitResult > & _HitResult ) const {
    bool ret = false;
    float d, u, v;
    unsigned int const * indices = OwnerMesh->GetIndices() + FirstIndex;
    SMeshVertex const * vertices = OwnerMesh->GetVertices();

    if ( _Distance < 0.0001f ) {
        return false;
    }

    if ( AABBTree ) {
        if ( bAABBTreeDirty ) {
            GLogger.Printf( "AIndexedMeshSubpart::Raycast: bvh is outdated\n" );
            return false;
        }

        TPodVector< SNodeAABB > const & nodes = AABBTree->GetNodes();
        unsigned int const * indirection = AABBTree->GetIndirection();

        float hitMin, hitMax;

        for ( int nodeIndex = 0; nodeIndex < nodes.Size(); ) {
            SNodeAABB const * node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox( _RayStart, _InvRayDir, node->Bounds, hitMin, hitMax ) && hitMin <= _Distance;
            const bool bLeaf = node->IsLeaf();

            if ( bLeaf && bOverlap ) {
                for ( int t = 0; t < node->PrimitiveCount; t++ ) {
                    const int triangleNum = node->Index + t;
                    const unsigned int baseInd = indirection[triangleNum];
                    const unsigned int i0 = BaseVertex + indices[baseInd + 0];
                    const unsigned int i1 = BaseVertex + indices[baseInd + 1];
                    const unsigned int i2 = BaseVertex + indices[baseInd + 2];
                    Float3 const & v0 = vertices[i0].Position;
                    Float3 const & v1 = vertices[i1].Position;
                    Float3 const & v2 = vertices[i2].Position;
                    if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                        if ( _Distance > d ) {
                            STriangleHitResult & hitResult = _HitResult.Append();
                            hitResult.Location = _RayStart + _RayDir * d;
                            hitResult.Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
                            hitResult.Distance = d;
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
            }

            nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
        }
    } else {
        float hitMin, hitMax;

        if ( !BvRayIntersectBox( _RayStart, _InvRayDir, BoundingBox, hitMin, hitMax ) || hitMin >= _Distance ) {
            return false;
        }

        const int primCount = IndexCount / 3;

        for ( int tri = 0 ; tri < primCount; tri++, indices+=3 ) {
            const unsigned int i0 = BaseVertex + indices[ 0 ];
            const unsigned int i1 = BaseVertex + indices[ 1 ];
            const unsigned int i2 = BaseVertex + indices[ 2 ];

            Float3 const & v0 = vertices[i0].Position;
            Float3 const & v1 = vertices[i1].Position;
            Float3 const & v2 = vertices[i2].Position;

            if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                if ( _Distance > d ) {
                    STriangleHitResult & hitResult = _HitResult.Append();
                    hitResult.Location = _RayStart + _RayDir * d;
                    hitResult.Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
                    hitResult.Distance = d;
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
    }
    return ret;
}

bool AIndexedMeshSubpart::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, Float3 const & _InvRayDir, float _Distance, bool bCullBackFace, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const {
    bool ret = false;
    float d, u, v;
    unsigned int const * indices = OwnerMesh->GetIndices() + FirstIndex;
    SMeshVertex const * vertices = OwnerMesh->GetVertices();

    if ( _Distance < 0.0001f ) {
        return false;
    }

    if ( AABBTree ) {
        if ( bAABBTreeDirty ) {
            GLogger.Printf( "AIndexedMeshSubpart::RaycastClosest: bvh is outdated\n" );
            return false;
        }

        TPodVector< SNodeAABB > const & nodes = AABBTree->GetNodes();
        unsigned int const * indirection = AABBTree->GetIndirection();

        float hitMin, hitMax;

        for ( int nodeIndex = 0; nodeIndex < nodes.Size(); ) {
            SNodeAABB const * node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox( _RayStart, _InvRayDir, node->Bounds, hitMin, hitMax ) && hitMin <= _Distance;
            const bool bLeaf = node->IsLeaf();

            if ( bLeaf && bOverlap ) {
                for ( int t = 0; t < node->PrimitiveCount; t++ ) {
                    const int triangleNum = node->Index + t;
                    const unsigned int baseInd = indirection[triangleNum];
                    const unsigned int i0 = BaseVertex + indices[baseInd + 0];
                    const unsigned int i1 = BaseVertex + indices[baseInd + 1];
                    const unsigned int i2 = BaseVertex + indices[baseInd + 2];
                    Float3 const & v0 = vertices[i0].Position;
                    Float3 const & v1 = vertices[i1].Position;
                    Float3 const & v2 = vertices[i2].Position;
                    if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                        if ( _Distance > d ) {
                            _Distance = d;
                            _HitDistance = d;
                            _HitLocation = _RayStart + _RayDir * d;
                            _HitUV.X = u;
                            _HitUV.Y = v;
                            _Indices[0] = i0;
                            _Indices[1] = i1;
                            _Indices[2] = i2;
                            ret = true;
                        }
                    }
                }
            }

            nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
        }
    } else {
        float hitMin, hitMax;

        if ( !BvRayIntersectBox( _RayStart, _InvRayDir, BoundingBox, hitMin, hitMax ) || hitMin >= _Distance ) {
            return false;
        }

        const int primCount = IndexCount / 3;

        for ( int tri = 0 ; tri < primCount; tri++, indices+=3 ) {
            const unsigned int i0 = BaseVertex + indices[ 0 ];
            const unsigned int i1 = BaseVertex + indices[ 1 ];
            const unsigned int i2 = BaseVertex + indices[ 2 ];

            Float3 const & v0 = vertices[i0].Position;
            Float3 const & v1 = vertices[i1].Position;
            Float3 const & v2 = vertices[i2].Position;

            if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
                if ( _Distance > d ) {
                    _Distance = d;
                    _HitDistance = d;
                    _HitLocation = _RayStart + _RayDir * d;
                    _HitUV.X = u;
                    _HitUV.Y = v;
                    _Indices[0] = i0;
                    _Indices[1] = i1;
                    _Indices[2] = i2;
                    ret = true;
                }
            }
        }
    }
    return ret;
}

void AIndexedMeshSubpart::DrawBVH( ADebugRenderer * InRenderer, Float3x4 const & _TransformMatrix ) {
    if ( !AABBTree ) {
        return;
    }

    InRenderer->SetDepthTest( false );
    InRenderer->SetColor( AColor4::White() );

    BvOrientedBox orientedBox;

    for ( SNodeAABB const & n : AABBTree->GetNodes() ) {
        if ( n.IsLeaf() ) {
            orientedBox.FromAxisAlignedBox( n.Bounds, _TransformMatrix );
            InRenderer->DrawOBB( orientedBox );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

ALightmapUV::ALightmapUV() {
}

ALightmapUV::~ALightmapUV() {
    Purge();
}

void ALightmapUV::Purge() {
    if ( SourceMesh ) {
        SourceMesh->LightmapUVs[ IndexInArrayOfUVs ] = SourceMesh->LightmapUVs[ SourceMesh->LightmapUVs.Size() - 1 ];
        SourceMesh->LightmapUVs[ IndexInArrayOfUVs ]->IndexInArrayOfUVs = IndexInArrayOfUVs;
        IndexInArrayOfUVs = -1;
        SourceMesh->LightmapUVs.RemoveLast();
        SourceMesh.Reset();
    }

    LightingLevel.Reset();

    Vertices.Free();

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Deallocate( VertexBufferGPU );
    VertexBufferGPU = nullptr;
}

void ALightmapUV::Initialize( AIndexedMesh * InSourceMesh, ALevel * InLightingLevel ) {
    Purge();

    SourceMesh = InSourceMesh;
    LightingLevel = InLightingLevel;
    bInvalid = false;

    IndexInArrayOfUVs = InSourceMesh->LightmapUVs.Size();
    InSourceMesh->LightmapUVs.Append( this );

    Vertices.ResizeInvalidate( InSourceMesh->GetVertexCount() );

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    VertexBufferGPU = vertexMemory->AllocateVertex( Vertices.Size() * sizeof( SMeshVertexUV ), nullptr, GetVertexMemory, this );
}

bool ALightmapUV::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "ALightmapUV::SendVertexDataToGPU: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Update( VertexBufferGPU, _StartVertexLocation * sizeof( SMeshVertexUV ), _VerticesCount * sizeof( SMeshVertexUV ), Vertices.ToPtr() + _StartVertexLocation );

    return true;
}

bool ALightmapUV::WriteVertexData( SMeshVertexUV const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "ALightmapUV::WriteVertexData: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    Core::Memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( SMeshVertexUV ) );

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}

void ALightmapUV::GetVertexBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    if ( VertexBufferGPU ) {
        AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset( VertexBufferGPU, _Buffer, _Offset );
    }
}

void *ALightmapUV::GetVertexMemory( void * _This ) {
    return static_cast< ALightmapUV * >( _This )->GetVertices();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

AVertexLight::AVertexLight() {
}

AVertexLight::~AVertexLight() {
    Purge();
}

void AVertexLight::Purge() {
    if ( SourceMesh ) {
        SourceMesh->VertexLightChannels[ IndexInArrayOfChannels ] = SourceMesh->VertexLightChannels[ SourceMesh->VertexLightChannels.Size() - 1 ];
        SourceMesh->VertexLightChannels[ IndexInArrayOfChannels ]->IndexInArrayOfChannels = IndexInArrayOfChannels;
        IndexInArrayOfChannels = -1;
        SourceMesh->VertexLightChannels.RemoveLast();
        SourceMesh.Reset();
    }

    LightingLevel.Reset();

    Vertices.Free();

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Deallocate( VertexBufferGPU );
    VertexBufferGPU = nullptr;
}

void AVertexLight::Initialize( AIndexedMesh * InSourceMesh, ALevel * InLightingLevel ) {
    Purge();

    SourceMesh = InSourceMesh;
    LightingLevel = InLightingLevel;
    bInvalid = false;

    IndexInArrayOfChannels = InSourceMesh->VertexLightChannels.Size();
    InSourceMesh->VertexLightChannels.Append( this );

    Vertices.ResizeInvalidate( InSourceMesh->GetVertexCount() );

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    VertexBufferGPU = vertexMemory->AllocateVertex( Vertices.Size() * sizeof( SMeshVertexLight ), nullptr, GetVertexMemory, this );
}

bool AVertexLight::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "AVertexLight::SendVertexDataToGPU: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();

    vertexMemory->Update( VertexBufferGPU, _StartVertexLocation * sizeof( SMeshVertexLight ), _VerticesCount * sizeof( SMeshVertexLight ), Vertices.ToPtr() + _StartVertexLocation );

    return true;
}

bool AVertexLight::WriteVertexData( SMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "AVertexLight::WriteVertexData: Referencing outside of buffer (%s)\n", GetObjectNameCStr() );
        return false;
    }

    Core::Memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( SMeshVertexLight ) );

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}

void AVertexLight::GetVertexBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    if ( VertexBufferGPU ) {
        AVertexMemoryGPU * vertexMemory = GRuntime.GetVertexMemoryGPU();
        vertexMemory->GetPhysicalBufferAndOffset( VertexBufferGPU, _Buffer, _Offset );
    }
}

void *AVertexLight::GetVertexMemory( void * _This ) {
    return static_cast< AVertexLight * >( _This )->GetVertices();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

AProceduralMesh::AProceduralMesh() {
    BoundingBox.Clear();
}

AProceduralMesh::~AProceduralMesh() {

}

void AProceduralMesh::GetVertexBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    AStreamedMemoryGPU * streamedMemory = GRuntime.GetStreamedMemoryGPU();

    streamedMemory->GetPhysicalBufferAndOffset( VertexStream, _Buffer, _Offset );
}

void AProceduralMesh::GetIndexBufferGPU( RenderCore::IBuffer ** _Buffer, size_t * _Offset ) {
    AStreamedMemoryGPU * streamedMemory = GRuntime.GetStreamedMemoryGPU();

    streamedMemory->GetPhysicalBufferAndOffset( IndexSteam, _Buffer, _Offset );
}

void AProceduralMesh::PreRenderUpdate( SRenderFrontendDef const * _Def ) {
    if ( VisFrame == _Def->FrameNumber ) {
        return;
    }

    VisFrame = _Def->FrameNumber;

    if ( !VertexCache.IsEmpty() && !IndexCache.IsEmpty() ) {
        AStreamedMemoryGPU * streamedMemory = GRuntime.GetStreamedMemoryGPU();

        VertexStream = streamedMemory->AllocateVertex( sizeof( SMeshVertex ) * VertexCache.Size(), VertexCache.ToPtr() );
        IndexSteam = streamedMemory->AllocateIndex( sizeof( unsigned int ) * IndexCache.Size(), IndexCache.ToPtr() );
    }
}

bool AProceduralMesh::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, bool bCullBackFace, TPodVector< STriangleHitResult > & _HitResult ) const {
    if ( _Distance < 0.0001f ) {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / _RayDir.X;
    invRayDir.Y = 1.0f / _RayDir.Y;
    invRayDir.Z = 1.0f / _RayDir.Z;

    if ( !BvRayIntersectBox( _RayStart, invRayDir, BoundingBox, boxMin, boxMax ) || boxMin >= _Distance ) {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool ret = false;
    float d, u, v;
    unsigned int const * indices = IndexCache.ToPtr() + FirstIndex;
    SMeshVertex const * vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for ( int tri = 0 ; tri < primCount; tri++, indices+=3 ) {
        const unsigned int i0 = BaseVertex + indices[ 0 ];
        const unsigned int i1 = BaseVertex + indices[ 1 ];
        const unsigned int i2 = BaseVertex + indices[ 2 ];

        Float3 const & v0 = vertices[i0].Position;
        Float3 const & v1 = vertices[i1].Position;
        Float3 const & v2 = vertices[i2].Position;

        if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
            if ( _Distance > d ) {
                STriangleHitResult & hitResult = _HitResult.Append();
                hitResult.Location = _RayStart + _RayDir * d;
                hitResult.Normal = Math::Cross( v1 - v0, v2 - v0 ).Normalized();
                hitResult.Distance = d;
                hitResult.UV.X = u;
                hitResult.UV.Y = v;
                hitResult.Indices[0] = i0;
                hitResult.Indices[1] = i1;
                hitResult.Indices[2] = i2;
                hitResult.Material = nullptr;
                ret = true;
            }
        }
    }

    return ret;
}

bool AProceduralMesh::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, bool bCullBackFace, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const {
    if ( _Distance < 0.0001f ) {
        return false;
    }

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / _RayDir.X;
    invRayDir.Y = 1.0f / _RayDir.Y;
    invRayDir.Z = 1.0f / _RayDir.Z;

    if ( !BvRayIntersectBox( _RayStart, invRayDir, BoundingBox, boxMin, boxMax ) || boxMin >= _Distance ) {
        return false;
    }

    const int FirstIndex = 0;
    const int BaseVertex = 0;

    bool ret = false;
    float d, u, v;
    unsigned int const * indices = IndexCache.ToPtr() + FirstIndex;
    SMeshVertex const * vertices = VertexCache.ToPtr();

    const int primCount = IndexCache.Size() / 3;

    for ( int tri = 0 ; tri < primCount; tri++, indices+=3 ) {
        const unsigned int i0 = BaseVertex + indices[ 0 ];
        const unsigned int i1 = BaseVertex + indices[ 1 ];
        const unsigned int i2 = BaseVertex + indices[ 2 ];

        Float3 const & v0 = vertices[i0].Position;
        Float3 const & v1 = vertices[i1].Position;
        Float3 const & v2 = vertices[i2].Position;

        if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v, bCullBackFace ) ) {
            if ( _Distance > d ) {
                _Distance = d;
                _HitLocation = _RayStart + _RayDir * d;
                _HitDistance = d;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////

void CalcTangentSpace( SMeshVertex * _VertexArray, unsigned int _NumVerts, unsigned int const * _IndexArray, unsigned int _NumIndices ) {
    Float3 binormal, tangent;

    TPodVector< Float3 > binormals;
    binormals.ResizeInvalidate( _NumVerts );
    binormals.ZeroMem();

    TPodVector< Float3 > tangents;
    tangents.ResizeInvalidate( _NumVerts );
    tangents.ZeroMem();

    for ( unsigned int i = 0; i < _NumIndices; i += 3 ) {
        const unsigned int a = _IndexArray[ i ];
        const unsigned int b = _IndexArray[ i + 1 ];
        const unsigned int c = _IndexArray[ i + 2 ];

        Float3 e1 = _VertexArray[ b ].Position - _VertexArray[ a ].Position;
        Float3 e2 = _VertexArray[ c ].Position - _VertexArray[ a ].Position;
        Float2 et1 = _VertexArray[ b ].GetTexCoord() - _VertexArray[ a ].GetTexCoord();
        Float2 et2 = _VertexArray[ c ].GetTexCoord() - _VertexArray[ a ].GetTexCoord();

        const float denom = et1.X*et2.Y - et1.Y*et2.X;
        const float scale = ( fabsf( denom ) < 0.0001f ) ? 1.0f : ( 1.0 / denom );
        tangent = ( e1 * et2.Y - e2 * et1.Y ) * scale;
        binormal = ( e2 * et1.X - e1 * et2.X ) * scale;

        tangents[ a ] += tangent;
        tangents[ b ] += tangent;
        tangents[ c ] += tangent;

        binormals[ a ] += binormal;
        binormals[ b ] += binormal;
        binormals[ c ] += binormal;
    }

    for ( int i = 0; i < _NumVerts; i++ ) {
        const Float3 n = _VertexArray[ i ].GetNormal();
        const Float3 & t = tangents[ i ];
        _VertexArray[ i ].SetTangent( ( t - n * Math::Dot( n, t ) ).Normalized() );
        _VertexArray[ i ].Handedness = (int8_t)CalcHandedness( t, binormals[ i ].Normalized(), n );
    }
}


BvAxisAlignedBox CalcBindposeBounds( SMeshVertex const * InVertices,
                                     SMeshVertexSkin const * InWeights,
                                     int InVertexCount,
                                     ASkin const * InSkin,
                                     SJoint * InJoints,
                                     int InJointsCount )
{
    Float3x4 absoluteTransforms[ASkeleton::MAX_JOINTS+1];
    Float3x4 vertexTransforms[ASkeleton::MAX_JOINTS];

    BvAxisAlignedBox BindposeBounds;

    BindposeBounds.Clear();

    absoluteTransforms[0].SetIdentity();
    for ( unsigned int j = 0 ; j < InJointsCount ; j++ ) {
        SJoint const & joint = InJoints[ j ];

        absoluteTransforms[ j + 1 ] = absoluteTransforms[ joint.Parent + 1 ] * joint.LocalTransform;
    }

    for ( unsigned int j = 0 ; j < InSkin->JointIndices.Size() ; j++ ) {
        int jointIndex = InSkin->JointIndices[j];

        vertexTransforms[ j ] = absoluteTransforms[ jointIndex + 1 ] * InSkin->OffsetMatrices[j];
    }

    for ( int v = 0 ; v < InVertexCount ; v++ ) {
        Float4 const position = Float4( InVertices[v].Position, 1.0f );
        SMeshVertexSkin const & w = InWeights[v];

        const float weights[4] = { w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f };

        Float4 const * t = &vertexTransforms[0][0];

        BindposeBounds.AddPoint(
         Math::Dot( ( t[ w.JointIndices[0] * 3 + 0 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 0 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 0 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 0 ] * weights[3] ), position ),

         Math::Dot( ( t[ w.JointIndices[0] * 3 + 1 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 1 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 1 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 1 ] * weights[3] ), position ),

         Math::Dot( ( t[ w.JointIndices[0] * 3 + 2 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 2 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 2 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 2 ] * weights[3] ), position ) );
    }

    return BindposeBounds;
}

void CalcBoundingBoxes( SMeshVertex const * InVertices,
                        SMeshVertexSkin const * InWeights,
                        int InVertexCount,
                        ASkin const * InSkin,
                        SJoint const *  InJoints,
                        int InNumJoints,
                        uint32_t FrameCount,
                        SAnimationChannel const * InChannels,
                        int InChannelsCount,
                        STransform const * InTransforms,
                        TPodVector< BvAxisAlignedBox > & Bounds )
{
    Float3x4 absoluteTransforms[ASkeleton::MAX_JOINTS+1];
    TPodVector< Float3x4 > relativeTransforms[ASkeleton::MAX_JOINTS];
    Float3x4 vertexTransforms[ASkeleton::MAX_JOINTS];

    Bounds.ResizeInvalidate( FrameCount );

    for ( int i = 0 ; i < InChannelsCount ; i++ ) {
        SAnimationChannel const & anim = InChannels[i];

        relativeTransforms[anim.JointIndex].ResizeInvalidate( FrameCount );

        for ( int frameNum = 0; frameNum < FrameCount ; frameNum++ ) {

            STransform const & transform = InTransforms[ anim.TransformOffset + frameNum ];

            transform.ComputeTransformMatrix( relativeTransforms[anim.JointIndex][frameNum] );
        }
    }

    for ( int frameNum = 0 ; frameNum < FrameCount ; frameNum++ ) {

        BvAxisAlignedBox & bounds = Bounds[frameNum];

        bounds.Clear();

        absoluteTransforms[0].SetIdentity();
        for ( unsigned int j = 0 ; j < InNumJoints ; j++ ) {
            SJoint const & joint = InJoints[ j ];

            Float3x4 const & parentTransform = absoluteTransforms[ joint.Parent + 1 ];

            if ( relativeTransforms[j].IsEmpty() ) {
                absoluteTransforms[ j + 1 ] = parentTransform * joint.LocalTransform;
            } else {
                absoluteTransforms[ j + 1 ] = parentTransform * relativeTransforms[j][ frameNum ];
            }
        }

        for ( unsigned int j = 0 ; j < InSkin->JointIndices.Size() ; j++ ) {
            int jointIndex = InSkin->JointIndices[j];

            vertexTransforms[ j ] = absoluteTransforms[ jointIndex + 1 ] * InSkin->OffsetMatrices[j];
        }

        for ( int v = 0 ; v < InVertexCount ; v++ ) {
            Float4 const position = Float4( InVertices[v].Position, 1.0f );
            SMeshVertexSkin const & w = InWeights[v];

            const float weights[4] = { w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f };

            Float4 const * t = &vertexTransforms[0][0];

            bounds.AddPoint(
             Math::Dot( ( t[ w.JointIndices[0] * 3 + 0 ] * weights[0]
                        + t[ w.JointIndices[1] * 3 + 0 ] * weights[1]
                        + t[ w.JointIndices[2] * 3 + 0 ] * weights[2]
                        + t[ w.JointIndices[3] * 3 + 0 ] * weights[3] ), position ),

             Math::Dot( ( t[ w.JointIndices[0] * 3 + 1 ] * weights[0]
                        + t[ w.JointIndices[1] * 3 + 1 ] * weights[1]
                        + t[ w.JointIndices[2] * 3 + 1 ] * weights[2]
                        + t[ w.JointIndices[3] * 3 + 1 ] * weights[3] ), position ),

             Math::Dot( ( t[ w.JointIndices[0] * 3 + 2 ] * weights[0]
                        + t[ w.JointIndices[1] * 3 + 2 ] * weights[1]
                        + t[ w.JointIndices[2] * 3 + 2 ] * weights[2]
                        + t[ w.JointIndices[3] * 3 + 2 ] * weights[3] ), position ) );
        }
    }
}

void CreateBoxMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, const Float3 & _Size, float _TexCoordScale ) {
    constexpr unsigned int indices[ 6 * 6 ] =
    {
        0, 1, 2, 2, 3, 0, // front face
        4, 5, 6, 6, 7, 4, // back face
        5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face
        1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
        3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
        1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
    };

    _Vertices.ResizeInvalidate( 24 );
    _Indices.ResizeInvalidate( 36 );

    Core::Memcpy( _Indices.ToPtr(), indices, sizeof( indices ) );

    const Float3 halfSize = _Size * 0.5f;

    _Bounds.Mins = -halfSize;
    _Bounds.Maxs = halfSize;

    const Float3 & mins = _Bounds.Mins;
    const Float3 & maxs = _Bounds.Maxs;

    SMeshVertex * pVerts = _Vertices.ToPtr();

    uint16_t zero = 0;
    uint16_t pos = Math::FloatToHalf( 1.0f );
    uint16_t neg = Math::FloatToHalf( -1.0f );

    pVerts[ 0 + 8 * 0 ].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[ 0 + 8 * 0 ].SetNormalNative( zero, zero, pos );
    pVerts[ 0 + 8 * 0 ].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[ 1 + 8 * 0 ].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[ 1 + 8 * 0 ].SetNormalNative( zero, zero, pos );
    pVerts[ 1 + 8 * 0 ].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[ 2 + 8 * 0 ].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[ 2 + 8 * 0 ].SetNormalNative( zero, zero, pos );
    pVerts[ 2 + 8 * 0 ].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[ 3 + 8 * 0 ].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[ 3 + 8 * 0 ].SetNormalNative( zero, zero, pos );
    pVerts[ 3 + 8 * 0 ].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );


    pVerts[ 4 + 8 * 0 ].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[ 4 + 8 * 0 ].SetNormalNative( zero, zero, neg );
    pVerts[ 4 + 8 * 0 ].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[ 5 + 8 * 0 ].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[ 5 + 8 * 0 ].SetNormalNative( zero, zero, neg );
    pVerts[ 5 + 8 * 0 ].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[ 6 + 8 * 0 ].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[ 6 + 8 * 0 ].SetNormalNative( zero, zero, neg );
    pVerts[ 6 + 8 * 0 ].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[ 7 + 8 * 0 ].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[ 7 + 8 * 0 ].SetNormalNative( zero, zero, neg );
    pVerts[ 7 + 8 * 0 ].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );


    pVerts[ 0 + 8 * 1 ].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[ 0 + 8 * 1 ].SetNormalNative( neg, zero, zero );
    pVerts[ 0 + 8 * 1 ].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[ 1 + 8 * 1 ].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[ 1 + 8 * 1 ].SetNormalNative( pos, zero, zero );
    pVerts[ 1 + 8 * 1 ].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[ 2 + 8 * 1 ].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[ 2 + 8 * 1 ].SetNormalNative( pos, zero, zero );
    pVerts[ 2 + 8 * 1 ].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    pVerts[ 3 + 8 * 1 ].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[ 3 + 8 * 1 ].SetNormalNative( neg, zero, zero );
    pVerts[ 3 + 8 * 1 ].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );


    pVerts[ 4 + 8 * 1 ].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[ 4 + 8 * 1 ].SetNormalNative( pos, zero, zero );
    pVerts[ 4 + 8 * 1 ].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[ 5 + 8 * 1 ].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[ 5 + 8 * 1 ].SetNormalNative( neg, zero, zero );
    pVerts[ 5 + 8 * 1 ].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[ 6 + 8 * 1 ].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[ 6 + 8 * 1 ].SetNormalNative( neg, zero, zero );
    pVerts[ 6 + 8 * 1 ].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    pVerts[ 7 + 8 * 1 ].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[ 7 + 8 * 1 ].SetNormalNative( pos, zero, zero );
    pVerts[ 7 + 8 * 1 ].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );


    pVerts[ 1 + 8 * 2 ].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[ 1 + 8 * 2 ].SetNormalNative( zero, neg, zero );
    pVerts[ 1 + 8 * 2 ].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[ 0 + 8 * 2 ].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[ 0 + 8 * 2 ].SetNormalNative( zero, neg, zero );
    pVerts[ 0 + 8 * 2 ].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    pVerts[ 5 + 8 * 2 ].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[ 5 + 8 * 2 ].SetNormalNative( zero, neg, zero );
    pVerts[ 5 + 8 * 2 ].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[ 4 + 8 * 2 ].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[ 4 + 8 * 2 ].SetNormalNative( zero, neg, zero );
    pVerts[ 4 + 8 * 2 ].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );


    pVerts[ 3 + 8 * 2 ].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[ 3 + 8 * 2 ].SetNormalNative( zero, pos, zero );
    pVerts[ 3 + 8 * 2 ].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[ 2 + 8 * 2 ].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[ 2 + 8 * 2 ].SetNormalNative( zero, pos, zero );
    pVerts[ 2 + 8 * 2 ].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[ 7 + 8 * 2 ].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[ 7 + 8 * 2 ].SetNormalNative( zero, pos, zero );
    pVerts[ 7 + 8 * 2 ].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[ 6 + 8 * 2 ].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[ 6 + 8 * 2 ].SetNormalNative( zero, pos, zero );
    pVerts[ 6 + 8 * 2 ].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateSphereMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    int x, y;
    float verticalAngle, horizontalAngle;
    unsigned int quad[ 4 ];

    _NumVerticalSubdivs = Math::Max( _NumVerticalSubdivs, 4 );
    _NumHorizontalSubdivs = Math::Max( _NumHorizontalSubdivs, 4 );

    _Vertices.ResizeInvalidate( ( _NumHorizontalSubdivs + 1 )*( _NumVerticalSubdivs + 1 ) );
    _Indices.ResizeInvalidate( _NumHorizontalSubdivs * _NumVerticalSubdivs * 6 );

    _Bounds.Mins.X = _Bounds.Mins.Y = _Bounds.Mins.Z = -_Radius;
    _Bounds.Maxs.X = _Bounds.Maxs.Y = _Bounds.Maxs.Z = _Radius;

    SMeshVertex * pVert = _Vertices.ToPtr();

    const float verticalStep = Math::_PI / _NumVerticalSubdivs;
    const float horizontalStep = Math::_2PI / _NumHorizontalSubdivs;
    const float verticalScale = 1.0f / _NumVerticalSubdivs;
    const float horizontalScale = 1.0f / _NumHorizontalSubdivs;

    for ( y = 0, verticalAngle = -Math::_HALF_PI; y <= _NumVerticalSubdivs; y++ ) {
        float h, r;
        Math::SinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            Math::SinCos( horizontalAngle, s, c );
            pVert->Position = Float3( scaledR*c, scaledH, scaledR*s );
            pVert->SetTexCoord( Float2( 1.0f - static_cast< float >( x ) * horizontalScale, 1.0f - static_cast< float >( y ) * verticalScale ) * _TexCoordScale );
            pVert->SetNormal( r*c, h, r*s );
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int * pIndices = _Indices.ToPtr();
    for ( y = 0; y < _NumVerticalSubdivs; y++ ) {
        const int y2 = y + 1;
        for ( x = 0; x < _NumHorizontalSubdivs; x++ ) {
            const int x2 = x + 1;

            quad[ 0 ] = y  * ( _NumHorizontalSubdivs + 1 ) + x;
            quad[ 1 ] = y2 * ( _NumHorizontalSubdivs + 1 ) + x;
            quad[ 2 ] = y2 * ( _NumHorizontalSubdivs + 1 ) + x2;
            quad[ 3 ] = y  * ( _NumHorizontalSubdivs + 1 ) + x2;

            *pIndices++ = quad[ 0 ];
            *pIndices++ = quad[ 1 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 3 ];
            *pIndices++ = quad[ 0 ];
        }
    }

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreatePlaneMeshXZ( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale ) {
    _Vertices.ResizeInvalidate( 4 );
    _Indices.ResizeInvalidate( 6 );

    const float halfWidth = _Width * 0.5f;
    const float halfHeight = _Height * 0.5f;

    const SMeshVertex Verts[ 4 ] = {
        MakeMeshVertex( Float3( -halfWidth,0,-halfHeight ), Float2( 0,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) ),
        MakeMeshVertex( Float3( -halfWidth,0,halfHeight ), Float2( 0,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) ),
        MakeMeshVertex( Float3( halfWidth,0,halfHeight ), Float2( _TexCoordScale,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) ),
        MakeMeshVertex( Float3( halfWidth,0,-halfHeight ), Float2( _TexCoordScale,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) )
    };

    Core::Memcpy( _Vertices.ToPtr(), &Verts, 4 * sizeof( SMeshVertex ) );

    constexpr unsigned int indices[ 6 ] = { 0,1,2,2,3,0 };
    Core::Memcpy( _Indices.ToPtr(), &indices, sizeof( indices ) );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );

    _Bounds.Mins.X = -halfWidth;
    _Bounds.Mins.Y = 0.0f;
    _Bounds.Mins.Z = -halfHeight;
    _Bounds.Maxs.X = halfWidth;
    _Bounds.Maxs.Y = 0.0f;
    _Bounds.Maxs.Z = halfHeight;
}

void CreatePlaneMeshXY( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale ) {
    _Vertices.ResizeInvalidate( 4 );
    _Indices.ResizeInvalidate( 6 );

    const float halfWidth = _Width * 0.5f;
    const float halfHeight = _Height * 0.5f;

    const SMeshVertex Verts[ 4 ] = {
        MakeMeshVertex( Float3( -halfWidth,-halfHeight,0 ), Float2( 0,_TexCoordScale ), Float3( 0,0,0 ), 1.0f, Float3( 0,0,1 ) ),
        MakeMeshVertex( Float3( halfWidth,-halfHeight,0 ), Float2( _TexCoordScale,_TexCoordScale ), Float3( 0,0,0 ), 1.0f, Float3( 0,0,1 ) ),
        MakeMeshVertex( Float3( halfWidth,halfHeight,0 ), Float2( _TexCoordScale,0 ), Float3( 0,0,0 ), 1.0f, Float3( 0,0,1 ) ),
        MakeMeshVertex( Float3( -halfWidth,halfHeight,0 ), Float2( 0,0 ), Float3( 0,0,0 ), 1.0f, Float3( 0,0,1 ) )
    };

    Core::Memcpy( _Vertices.ToPtr(), &Verts, 4 * sizeof( SMeshVertex ) );

    constexpr unsigned int indices[ 6 ] = { 0,1,2,2,3,0 };
    Core::Memcpy( _Indices.ToPtr(), &indices, sizeof( indices ) );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );

    _Bounds.Mins.X = -halfWidth;
    _Bounds.Mins.Y = -halfHeight;
    _Bounds.Mins.Z = 0.0f;
    _Bounds.Maxs.X = halfWidth;
    _Bounds.Maxs.Y = halfHeight;
    _Bounds.Maxs.Z = 0.0f;
}

void CreatePatchMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds,
    Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11,
    float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {

    _NumVerticalSubdivs = Math::Max( _NumVerticalSubdivs, 2 );
    _NumHorizontalSubdivs = Math::Max( _NumHorizontalSubdivs, 2 );

    const float scaleX = 1.0f / ( float )( _NumHorizontalSubdivs - 1 );
    const float scaleY = 1.0f / ( float )( _NumVerticalSubdivs - 1 );

    const int vertexCount = _NumHorizontalSubdivs * _NumVerticalSubdivs;
    const int indexCount = ( _NumHorizontalSubdivs - 1 ) * ( _NumVerticalSubdivs - 1 ) * 6;

    Float3 normal = Math::Cross( Corner10 - Corner00, Corner01 - Corner00 ).Normalized();

    uint16_t normalNative[3];
    normalNative[0] = Math::FloatToHalf( normal.X );
    normalNative[1] = Math::FloatToHalf( normal.Y );
    normalNative[2] = Math::FloatToHalf( normal.Z );

    _Vertices.ResizeInvalidate( _TwoSided ? vertexCount << 1 : vertexCount );
    _Indices.ResizeInvalidate( _TwoSided ? indexCount << 1 : indexCount );

    SMeshVertex * pVert = _Vertices.ToPtr();
    unsigned int * pIndices = _Indices.ToPtr();

    for ( int y = 0; y < _NumVerticalSubdivs; ++y ) {
        const float lerpY = y * scaleY;
        const Float3 py0 = Math::Lerp( Corner00, Corner01, lerpY );
        const Float3 py1 = Math::Lerp( Corner10, Corner11, lerpY );
        const float ty = lerpY * _TexCoordScale;

        for ( int x = 0; x < _NumHorizontalSubdivs; ++x ) {
            const float lerpX = x * scaleX;

            pVert->Position = Math::Lerp( py0, py1, lerpX );
            pVert->SetTexCoord( lerpX * _TexCoordScale, ty );
            pVert->SetNormalNative( normalNative[0], normalNative[1], normalNative[2] );

            ++pVert;
        }
    }

    if ( _TwoSided ) {
        normal = -normal;

        normalNative[0] = Math::FloatToHalf( normal.X );
        normalNative[1] = Math::FloatToHalf( normal.Y );
        normalNative[2] = Math::FloatToHalf( normal.Z );

        for ( int y = 0; y < _NumVerticalSubdivs; ++y ) {
            const float lerpY = y * scaleY;
            const Float3 py0 = Math::Lerp( Corner00, Corner01, lerpY );
            const Float3 py1 = Math::Lerp( Corner10, Corner11, lerpY );
            const float ty = lerpY * _TexCoordScale;

            for ( int x = 0; x < _NumHorizontalSubdivs; ++x ) {
                const float lerpX = x * scaleX;

                pVert->Position = Math::Lerp( py0, py1, lerpX );
                pVert->SetTexCoord( lerpX * _TexCoordScale, ty );
                pVert->SetNormalNative( normalNative[0], normalNative[1], normalNative[2] );

                ++pVert;
            }
        }
    }

    for ( int y = 0; y < _NumVerticalSubdivs; ++y ) {

        const int index0 = y*_NumHorizontalSubdivs;
        const int index1 = ( y + 1 )*_NumHorizontalSubdivs;

        for ( int x = 0; x < _NumHorizontalSubdivs; ++x ) {
            const int quad00 = index0 + x;
            const int quad01 = index0 + x + 1;
            const int quad10 = index1 + x;
            const int quad11 = index1 + x + 1;

            if ( ( x + 1 ) < _NumHorizontalSubdivs && ( y + 1 ) < _NumVerticalSubdivs ) {
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
        for ( int y = 0; y < _NumVerticalSubdivs; ++y ) {

            const int index0 = vertexCount + y*_NumHorizontalSubdivs;
            const int index1 = vertexCount + ( y + 1 )*_NumHorizontalSubdivs;

            for ( int x = 0; x < _NumHorizontalSubdivs; ++x ) {
                const int quad00 = index0 + x;
                const int quad01 = index0 + x + 1;
                const int quad10 = index1 + x;
                const int quad11 = index1 + x + 1;

                if ( ( x + 1 ) < _NumHorizontalSubdivs && ( y + 1 ) < _NumVerticalSubdivs ) {
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

void CreateCylinderMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    int i, j;
    float angle;
    unsigned int quad[ 4 ];

    _NumSubdivs = Math::Max( _NumSubdivs, 4 );

    const float invSubdivs = 1.0f / _NumSubdivs;
    const float angleStep = Math::_2PI * invSubdivs;
    const float halfHeight = _Height * 0.5f;

    _Vertices.ResizeInvalidate( 6 * ( _NumSubdivs + 1 ) );
    _Indices.ResizeInvalidate( 3 * _NumSubdivs * 6 );

    _Bounds.Mins.X = -_Radius;
    _Bounds.Mins.Z = -_Radius;
    _Bounds.Mins.Y = -halfHeight;

    _Bounds.Maxs.X = _Radius;
    _Bounds.Maxs.Z = _Radius;
    _Bounds.Maxs.Y = halfHeight;

    SMeshVertex * pVerts = _Vertices.ToPtr();

    int firstVertex = 0;

    uint16_t pos = Math::FloatToHalf( 1.0f );
    uint16_t neg = Math::FloatToHalf( -1.0f );

    for ( j = 0; j <= _NumSubdivs; j++ ) {
        pVerts[ firstVertex + j ].Position = Float3( 0.0f, -halfHeight, 0.0f );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( j * invSubdivs, 0.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormalNative( 0, neg, 0 );
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, -halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( j * invSubdivs, 1.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormalNative( 0, neg, 0 );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, -halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( 1.0f - j * invSubdivs, 1.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormal( c, 0.0f, s );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( 1.0f - j * invSubdivs, 0.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormal( c, 0.0f, s );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( j * invSubdivs, 0.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormalNative( 0, pos, 0 );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0; j <= _NumSubdivs; j++ ) {
        pVerts[ firstVertex + j ].Position = Float3( 0.0f, halfHeight, 0.0f );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( j * invSubdivs, 1.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormalNative( 0, pos, 0 );
    }
    firstVertex += _NumSubdivs + 1;

    // generate indices

    unsigned int * pIndices = _Indices.ToPtr();

    firstVertex = 0;
    for ( i = 0; i < 3; i++ ) {
        for ( j = 0; j < _NumSubdivs; j++ ) {
            quad[ 3 ] = firstVertex + j;
            quad[ 2 ] = firstVertex + j + 1;
            quad[ 1 ] = firstVertex + j + 1 + ( _NumSubdivs + 1 );
            quad[ 0 ] = firstVertex + j + ( _NumSubdivs + 1 );

            *pIndices++ = quad[ 0 ];
            *pIndices++ = quad[ 1 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 3 ];
            *pIndices++ = quad[ 0 ];
        }
        firstVertex += ( _NumSubdivs + 1 ) * 2;
    }

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateConeMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    int i, j;
    float angle;
    unsigned int quad[ 4 ];

    _NumSubdivs = Math::Max( _NumSubdivs, 4 );

    const float invSubdivs = 1.0f / _NumSubdivs;
    const float angleStep = Math::_2PI * invSubdivs;

    _Vertices.ResizeInvalidate( 4 * ( _NumSubdivs + 1 ) );
    _Indices.ResizeInvalidate( 2 * _NumSubdivs * 6 );

    _Bounds.Mins.X = -_Radius;
    _Bounds.Mins.Z = -_Radius;
    _Bounds.Mins.Y = 0;

    _Bounds.Maxs.X = _Radius;
    _Bounds.Maxs.Z = _Radius;
    _Bounds.Maxs.Y = _Height;

    uint16_t neg = Math::FloatToHalf( -1.0f );

    SMeshVertex * pVerts = _Vertices.ToPtr();

    int firstVertex = 0;

    for ( j = 0; j <= _NumSubdivs; j++ ) {
        pVerts[ firstVertex + j ].Position = Float3::Zero();
        pVerts[ firstVertex + j ].SetTexCoord( Float2( j * invSubdivs, 0.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormalNative( 0, neg, 0 );
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, 0.0f, _Radius*s );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( j * invSubdivs, 1.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormalNative( 0, neg, 0 );;
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, 0.0f, _Radius*s );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( 1.0f - j * invSubdivs, 1.0f ) * _TexCoordScale );
        pVerts[ firstVertex + j ].SetNormal( c, 0.0f, s );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    Float3 vx;
    Float3 vy( 0, _Height, 0);
    Float3 v;
    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        Math::SinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( 0, _Height, 0 );
        pVerts[ firstVertex + j ].SetTexCoord( Float2( 1.0f - j * invSubdivs, 0.0f ) * _TexCoordScale );

        vx = Float3( c, 0.0f, s );
        v = vy - vx;
        pVerts[ firstVertex + j ].SetNormal( Math::Cross( Math::Cross( v, vx ), v ).Normalized() );

        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    AN_ASSERT( firstVertex == _Vertices.Size() );

    // generate indices

    unsigned int * pIndices = _Indices.ToPtr();

    firstVertex = 0;
    for ( i = 0; i < 2; i++ ) {
        for ( j = 0; j < _NumSubdivs; j++ ) {
            quad[ 3 ] = firstVertex + j;
            quad[ 2 ] = firstVertex + j + 1;
            quad[ 1 ] = firstVertex + j + 1 + ( _NumSubdivs + 1 );
            quad[ 0 ] = firstVertex + j + ( _NumSubdivs + 1 );

            *pIndices++ = quad[ 0 ];
            *pIndices++ = quad[ 1 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 3 ];
            *pIndices++ = quad[ 0 ];
        }
        firstVertex += ( _NumSubdivs + 1 ) * 2;
    }

    AN_ASSERT( pIndices == _Indices.ToPtr() + _Indices.Size() );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateCapsuleMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    int x, y, tcY;
    float verticalAngle, horizontalAngle;
    const float halfHeight = _Height * 0.5f;
    unsigned int quad[ 4 ];

    _NumVerticalSubdivs = Math::Max( _NumVerticalSubdivs, 4 );
    _NumHorizontalSubdivs = Math::Max( _NumHorizontalSubdivs, 4 );

    const int halfVerticalSubdivs = _NumVerticalSubdivs >> 1;

    _Vertices.ResizeInvalidate( ( _NumHorizontalSubdivs + 1 ) * ( _NumVerticalSubdivs + 1 ) * 2 );
    _Indices.ResizeInvalidate( _NumHorizontalSubdivs * ( _NumVerticalSubdivs + 1 ) * 6 );

    _Bounds.Mins.X = _Bounds.Mins.Z = -_Radius;
    _Bounds.Mins.Y =  -_Radius - halfHeight;
    _Bounds.Maxs.X = _Bounds.Maxs.Z = _Radius;
    _Bounds.Maxs.Y = _Radius + halfHeight;

    SMeshVertex * pVert = _Vertices.ToPtr();

    const float verticalStep = Math::_PI / _NumVerticalSubdivs;
    const float horizontalStep = Math::_2PI / _NumHorizontalSubdivs;
    const float verticalScale = 1.0f / ( _NumVerticalSubdivs + 1 );
    const float horizontalScale = 1.0f / _NumHorizontalSubdivs;

    tcY = 0;

    for ( y = 0, verticalAngle = -Math::_HALF_PI; y <= halfVerticalSubdivs; y++, tcY++ ) {
        float h, r;
        Math::SinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        const float posY = scaledH - halfHeight;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            Math::SinCos( horizontalAngle, s, c );
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->SetTexCoord( ( 1.0f - static_cast< float >( x ) * horizontalScale ) * _TexCoordScale,
                                ( 1.0f - static_cast< float >( tcY ) * verticalScale ) * _TexCoordScale );
            pVert->SetNormal( r * c, h, r * s );
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for ( y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++, tcY++ ) {
        float h, r;
        Math::SinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        const float posY = scaledH + halfHeight;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            Math::SinCos( horizontalAngle, s, c );
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->SetTexCoord( ( 1.0f - static_cast< float >( x ) * horizontalScale ) * _TexCoordScale,
                                ( 1.0f - static_cast< float >( tcY ) * verticalScale ) * _TexCoordScale );
            pVert->SetNormal( r * c, h, r * s );
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int * pIndices = _Indices.ToPtr();
    for ( y = 0; y <= _NumVerticalSubdivs; y++ ) {
        const int y2 = y + 1;
        for ( x = 0; x < _NumHorizontalSubdivs; x++ ) {
            const int x2 = x + 1;

            quad[ 0 ] = y  * ( _NumHorizontalSubdivs + 1 ) + x;
            quad[ 1 ] = y2 * ( _NumHorizontalSubdivs + 1 ) + x;
            quad[ 2 ] = y2 * ( _NumHorizontalSubdivs + 1 ) + x2;
            quad[ 3 ] = y  * ( _NumHorizontalSubdivs + 1 ) + x2;

            *pIndices++ = quad[ 0 ];
            *pIndices++ = quad[ 1 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 2 ];
            *pIndices++ = quad[ 3 ];
            *pIndices++ = quad[ 0 ];
        }
    }

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateSkyboxMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, const Float3 & _Size, float _TexCoordScale ) {
    constexpr unsigned int indices[6 * 6] =
    {
        0, 1, 2, 2, 3, 0, // front face
        4, 5, 6, 6, 7, 4, // back face
        5 + 8 * 1, 0 + 8 * 1, 3 + 8 * 1, 3 + 8 * 1, 6 + 8 * 1, 5 + 8 * 1, // left face

        1 + 8 * 1, 4 + 8 * 1, 7 + 8 * 1, 7 + 8 * 1, 2 + 8 * 1, 1 + 8 * 1, // right face
        3 + 8 * 2, 2 + 8 * 2, 7 + 8 * 2, 7 + 8 * 2, 6 + 8 * 2, 3 + 8 * 2, // top face
        1 + 8 * 2, 0 + 8 * 2, 5 + 8 * 2, 5 + 8 * 2, 4 + 8 * 2, 1 + 8 * 2, // bottom face
    };

    _Vertices.ResizeInvalidate( 24 );
    _Indices.ResizeInvalidate( 36 );

    for ( int i = 0 ; i < 36 ; i+=3 ) {
        _Indices[i  ] = indices[i+2];
        _Indices[i+1] = indices[i+1];
        _Indices[i+2] = indices[i ];
    }

    const Float3 halfSize = _Size * 0.5f;

    _Bounds.Mins = -halfSize;
    _Bounds.Maxs = halfSize;

    const Float3 & mins = _Bounds.Mins;
    const Float3 & maxs = _Bounds.Maxs;

    SMeshVertex * pVerts = _Vertices.ToPtr();

    uint16_t zero = 0;
    uint16_t pos = Math::FloatToHalf( 1.0f );
    uint16_t neg = Math::FloatToHalf( -1.0f );

    pVerts[0 + 8 * 0].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[0 + 8 * 0].SetNormalNative( zero, zero, neg );
    pVerts[0 + 8 * 0].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[1 + 8 * 0].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[1 + 8 * 0].SetNormalNative( zero, zero, neg );
    pVerts[1 + 8 * 0].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[2 + 8 * 0].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[2 + 8 * 0].SetNormalNative( zero, zero, neg );
    pVerts[2 + 8 * 0].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[3 + 8 * 0].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[3 + 8 * 0].SetNormalNative( zero, zero, neg );
    pVerts[3 + 8 * 0].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );


    pVerts[4 + 8 * 0].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[4 + 8 * 0].SetNormalNative( zero, zero, pos );
    pVerts[4 + 8 * 0].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[5 + 8 * 0].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[5 + 8 * 0].SetNormalNative( zero, zero, pos );
    pVerts[5 + 8 * 0].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[6 + 8 * 0].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[6 + 8 * 0].SetNormalNative( zero, zero, pos );
    pVerts[6 + 8 * 0].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[7 + 8 * 0].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[7 + 8 * 0].SetNormalNative( zero, zero, pos );
    pVerts[7 + 8 * 0].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );


    pVerts[0 + 8 * 1].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[0 + 8 * 1].SetNormalNative( pos, zero, zero );
    pVerts[0 + 8 * 1].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[1 + 8 * 1].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[1 + 8 * 1].SetNormalNative( neg, zero, zero );
    pVerts[1 + 8 * 1].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[2 + 8 * 1].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[2 + 8 * 1].SetNormalNative( neg, zero, zero );
    pVerts[2 + 8 * 1].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    pVerts[3 + 8 * 1].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[3 + 8 * 1].SetNormalNative( pos, zero, zero );
    pVerts[3 + 8 * 1].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );


    pVerts[4 + 8 * 1].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[4 + 8 * 1].SetNormalNative( neg, zero, zero );
    pVerts[4 + 8 * 1].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[5 + 8 * 1].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[5 + 8 * 1].SetNormalNative( pos, zero, zero );
    pVerts[5 + 8 * 1].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[6 + 8 * 1].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[6 + 8 * 1].SetNormalNative( pos, zero, zero );
    pVerts[6 + 8 * 1].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    pVerts[7 + 8 * 1].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[7 + 8 * 1].SetNormalNative( neg, zero, zero );
    pVerts[7 + 8 * 1].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );


    pVerts[1 + 8 * 2].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[1 + 8 * 2].SetNormalNative( zero, pos, zero );
    pVerts[1 + 8 * 2].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[0 + 8 * 2].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[0 + 8 * 2].SetNormalNative( zero, pos, zero );
    pVerts[0 + 8 * 2].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    pVerts[5 + 8 * 2].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[5 + 8 * 2].SetNormalNative( zero, pos, zero );
    pVerts[5 + 8 * 2].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[4 + 8 * 2].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[4 + 8 * 2].SetNormalNative( zero, pos, zero );
    pVerts[4 + 8 * 2].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );


    pVerts[3 + 8 * 2].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[3 + 8 * 2].SetNormalNative( zero, neg, zero );
    pVerts[3 + 8 * 2].SetTexCoord( Float2( 0, 1 )*_TexCoordScale );

    pVerts[2 + 8 * 2].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[2 + 8 * 2].SetNormalNative( zero, neg, zero );
    pVerts[2 + 8 * 2].SetTexCoord( Float2( 1, 1 )*_TexCoordScale );

    pVerts[7 + 8 * 2].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[7 + 8 * 2].SetNormalNative( zero, neg, zero );
    pVerts[7 + 8 * 2].SetTexCoord( Float2( 1, 0 )*_TexCoordScale );

    pVerts[6 + 8 * 2].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[6 + 8 * 2].SetNormalNative( zero, neg, zero );
    pVerts[6 + 8 * 2].SetTexCoord( Float2( 0, 0 )*_TexCoordScale );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateSkydomeMesh( TPodVector< SMeshVertex > & _Vertices, TPodVector< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs, bool _Hemisphere ) {
    int x, y;
    float verticalAngle, horizontalAngle;
    unsigned int quad[4];

    _NumVerticalSubdivs = Math::Max( _NumVerticalSubdivs, 4 );
    _NumHorizontalSubdivs = Math::Max( _NumHorizontalSubdivs, 4 );

    _Vertices.ResizeInvalidate( (_NumHorizontalSubdivs + 1)*(_NumVerticalSubdivs + 1) );
    _Indices.ResizeInvalidate( _NumHorizontalSubdivs * _NumVerticalSubdivs * 6 );

    _Bounds.Mins.X = _Bounds.Mins.Y = _Bounds.Mins.Z = -_Radius;
    _Bounds.Maxs.X = _Bounds.Maxs.Y = _Bounds.Maxs.Z = _Radius;

    SMeshVertex * pVert = _Vertices.ToPtr();

    const float verticalRange = _Hemisphere ? Math::_HALF_PI : Math::_PI;
    const float verticalStep = verticalRange / _NumVerticalSubdivs;
    const float horizontalStep = Math::_2PI / _NumHorizontalSubdivs;
    const float verticalScale = 1.0f / _NumVerticalSubdivs;
    const float horizontalScale = 1.0f / _NumHorizontalSubdivs;

    for ( y = 0, verticalAngle = _Hemisphere ? 0 : -Math::_HALF_PI; y <= _NumVerticalSubdivs; y++ ) {
        float h, r;
        Math::SinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            Math::SinCos( horizontalAngle, s, c );
            pVert->Position = Float3( scaledR*c, scaledH, scaledR*s );
            pVert->SetTexCoord( Float2( 1.0f - static_cast< float >(x) * horizontalScale, 1.0f - static_cast< float >(y) * verticalScale ) * _TexCoordScale );
            pVert->SetNormal( -r*c, -h, -r*s );
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    unsigned int * pIndices = _Indices.ToPtr();
    for ( y = 0; y < _NumVerticalSubdivs; y++ ) {
        const int y2 = y + 1;
        for ( x = 0; x < _NumHorizontalSubdivs; x++ ) {
            const int x2 = x + 1;

            quad[0] = y  * (_NumHorizontalSubdivs + 1) + x;
            quad[1] = y  * (_NumHorizontalSubdivs + 1) + x2;
            quad[2] = y2 * (_NumHorizontalSubdivs + 1) + x2;
            quad[3] = y2 * (_NumHorizontalSubdivs + 1) + x;            

            *pIndices++ = quad[0];
            *pIndices++ = quad[1];
            *pIndices++ = quad[2];
            *pIndices++ = quad[2];
            *pIndices++ = quad[3];
            *pIndices++ = quad[0];
        }
    }

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

struct SPrimitiveBounds {
    BvAxisAlignedBox Bounds;
    int PrimitiveIndex;
};

struct SBestSplitResult {
    int Axis;
    int PrimitiveIndex;
};

struct SAABBTreeBuild {
    TPodVector< BvAxisAlignedBox > RightBounds;
    TPodVector< SPrimitiveBounds > Primitives[3];
};

static void CalcNodeBounds( SPrimitiveBounds const * _Primitives, int _PrimCount, BvAxisAlignedBox & _Bounds ) {
    AN_ASSERT( _PrimCount > 0 );

    SPrimitiveBounds const * primitive = _Primitives;

    _Bounds = primitive->Bounds;

    primitive++;

    for ( ; primitive < &_Primitives[_PrimCount]; primitive++ ) {
        _Bounds.AddAABB( primitive->Bounds );
    }
}

static float CalcAABBVolume( BvAxisAlignedBox const & _Bounds ) {
    Float3 extents = _Bounds.Maxs - _Bounds.Mins;
    return extents.X * extents.Y * extents.Z;
}

static SBestSplitResult FindBestSplitPrimitive( SAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _PrimCount ) {
    struct CompareBoundsMax {
        CompareBoundsMax( const int _Axis ) : Axis( _Axis ) {}
        bool operator()( SPrimitiveBounds const & a, SPrimitiveBounds const & b ) const {
            return ( a.Bounds.Maxs[ Axis ] < b.Bounds.Maxs[ Axis ] );
        }
        const int Axis;
    };

    SPrimitiveBounds * primitives[ 3 ] = {
        _Build.Primitives[ 0 ].ToPtr() + _FirstPrimitive,
        _Build.Primitives[ 1 ].ToPtr() + _FirstPrimitive,
        _Build.Primitives[ 2 ].ToPtr() + _FirstPrimitive
    };

    for ( int i = 0; i < 3; i++ ) {
        if ( i != _Axis ) {
            Core::Memcpy( primitives[ i ], primitives[ _Axis ], sizeof( SPrimitiveBounds ) * _PrimCount );
        }
    }

    BvAxisAlignedBox right;
    BvAxisAlignedBox left;

    SBestSplitResult result;
    result.Axis = -1;

    float bestSAH = Math::MaxValue< float >(); // Surface area heuristic

    const float emptyCost = 1.0f;

    for ( int axis = 0; axis < 3; axis++ ) {
        SPrimitiveBounds * primBounds = primitives[ axis ];

        StdSort( primBounds, primBounds + _PrimCount, CompareBoundsMax( axis ) );

        right.Clear();
        for ( size_t i = _PrimCount - 1; i > 0; i-- ) {
            right.AddAABB( primBounds[ i ].Bounds );
            _Build.RightBounds[ i - 1 ] = right;
        }

        left.Clear();
        for ( size_t i = 1; i < _PrimCount; i++ ) {
            left.AddAABB( primBounds[ i - 1 ].Bounds );

            float sah = emptyCost + CalcAABBVolume( left ) * i + CalcAABBVolume( _Build.RightBounds[ i - 1 ] ) * ( _PrimCount - i );
            if ( bestSAH > sah ) {
                bestSAH = sah;
                result.Axis = axis;
                result.PrimitiveIndex = i;
            }
        }
    }

    AN_ASSERT( ( result.Axis != -1 ) && ( bestSAH < Math::MaxValue< float >() ) );

    return result;
}

ATreeAABB::ATreeAABB() {
    BoundingBox.Clear();
}

void ATreeAABB::InitializeTriangleSoup( SMeshVertex const * _Vertices, unsigned int const * _Indices, unsigned int _IndexCount, int _BaseVertex, unsigned int _PrimitivesPerLeaf ) {
    Purge();

    _PrimitivesPerLeaf = Math::Max( _PrimitivesPerLeaf, 16u );

    int primCount = _IndexCount / 3;

    int numLeafs = ( primCount + _PrimitivesPerLeaf - 1 ) / _PrimitivesPerLeaf;

    Nodes.Clear();
    Nodes.ReserveInvalidate( numLeafs * 4 );

    Indirection.ResizeInvalidate( primCount );

    SAABBTreeBuild build;
    build.RightBounds.ResizeInvalidate( primCount );
    build.Primitives[0].ResizeInvalidate( primCount );
    build.Primitives[1].ResizeInvalidate( primCount );
    build.Primitives[2].ResizeInvalidate( primCount );

    int primitiveIndex = 0;
    for ( unsigned int i = 0; i < _IndexCount; i += 3, primitiveIndex++ ) {
        const size_t i0 = _Indices[ i ];
        const size_t i1 = _Indices[ i + 1 ];
        const size_t i2 = _Indices[ i + 2 ];

        Float3 const & v0 = _Vertices[ _BaseVertex + i0 ].Position;
        Float3 const & v1 = _Vertices[ _BaseVertex + i1 ].Position;
        Float3 const & v2 = _Vertices[ _BaseVertex + i2 ].Position;

        SPrimitiveBounds & primitive = build.Primitives[ 0 ][ primitiveIndex ];
        primitive.PrimitiveIndex = i;//primitiveIndex * 3; // FIXME *3

        primitive.Bounds.Mins.X = Math::Min3( v0.X, v1.X, v2.X );
        primitive.Bounds.Mins.Y = Math::Min3( v0.Y, v1.Y, v2.Y );
        primitive.Bounds.Mins.Z = Math::Min3( v0.Z, v1.Z, v2.Z );

        primitive.Bounds.Maxs.X = Math::Max3( v0.X, v1.X, v2.X );
        primitive.Bounds.Maxs.Y = Math::Max3( v0.Y, v1.Y, v2.Y );
        primitive.Bounds.Maxs.Z = Math::Max3( v0.Z, v1.Z, v2.Z );
    }

    primitiveIndex = 0;
    Subdivide( build, 0, 0, primCount, _PrimitivesPerLeaf, primitiveIndex );
    Nodes.ShrinkToFit();

    BoundingBox = Nodes[0].Bounds;

    //size_t sz = Nodes.Size() * sizeof( Nodes[ 0 ] )
    //    + Indirection.Size() * sizeof( Indirection[ 0 ] )
    //    + sizeof( *this );

    //size_t sz2 = Nodes.Reserved() * sizeof( Nodes[0] )
    //    + Indirection.Reserved() * sizeof( Indirection[0] )
    //    + sizeof( *this );
    
    //GLogger.Printf( "AABBTree memory usage: %i  %i\n", sz, sz2 );
}

void ATreeAABB::InitializePrimitiveSoup( SPrimitiveDef const * _Primitives, unsigned int _PrimitiveCount, unsigned int _PrimitivesPerLeaf ) {
    Purge();

    _PrimitivesPerLeaf = Math::Max( _PrimitivesPerLeaf, 16u );

    int numLeafs = ( _PrimitiveCount + _PrimitivesPerLeaf - 1 ) / _PrimitivesPerLeaf;

    Nodes.Clear();
    Nodes.ReserveInvalidate( numLeafs * 4 );

    Indirection.ResizeInvalidate( _PrimitiveCount );

    SAABBTreeBuild build;
    build.RightBounds.ResizeInvalidate( _PrimitiveCount );
    build.Primitives[0].ResizeInvalidate( _PrimitiveCount );
    build.Primitives[1].ResizeInvalidate( _PrimitiveCount );
    build.Primitives[2].ResizeInvalidate( _PrimitiveCount );

    int primitiveIndex;
    for ( primitiveIndex = 0; primitiveIndex < _PrimitiveCount; primitiveIndex++ ) {
        SPrimitiveDef const * primitiveDef = _Primitives + primitiveIndex;
        SPrimitiveBounds & primitive = build.Primitives[ 0 ][ primitiveIndex ];

        switch ( primitiveDef->Type ) {
        case VSD_PRIMITIVE_SPHERE:
            primitive.Bounds.FromSphere( primitiveDef->Sphere.Center, primitiveDef->Sphere.Radius );
            break;
        case VSD_PRIMITIVE_BOX:
        default:
            primitive.Bounds = primitiveDef->Box;
            break;
        }

        primitive.PrimitiveIndex = primitiveIndex;
    }

    primitiveIndex = 0;
    Subdivide( build, 0, 0, _PrimitiveCount, _PrimitivesPerLeaf, primitiveIndex );
    Nodes.ShrinkToFit();

    BoundingBox = Nodes[0].Bounds;
}

void ATreeAABB::Purge() {
    Nodes.Free();
    Indirection.Free();
}

int ATreeAABB::MarkBoxOverlappingLeafs( BvAxisAlignedBox const & _Bounds, unsigned int * _MarkLeafs, int _MaxLeafs ) const {
    if ( !_MaxLeafs ) {
        return 0;
    }
    int n = 0;
    for ( int nodeIndex = 0; nodeIndex < Nodes.Size(); ) {
        SNodeAABB const * node = &Nodes[ nodeIndex ];

        const bool bOverlap = BvBoxOverlapBox( _Bounds, node->Bounds );
        const bool bLeaf = node->IsLeaf();

        if ( bLeaf && bOverlap ) {
            _MarkLeafs[ n++ ] = nodeIndex;
            if ( n == _MaxLeafs ) {
                return n;
            }
        }
        nodeIndex += ( bOverlap || bLeaf ) ? 1 : ( -node->Index );
    }
    return n;
}

int ATreeAABB::MarkRayOverlappingLeafs( Float3 const & _RayStart, Float3 const & _RayEnd, unsigned int * _MarkLeafs, int _MaxLeafs ) const {
    if ( !_MaxLeafs ) {
        return 0;
    }

    Float3 rayDir = _RayEnd - _RayStart;
    Float3 invRayDir;

    float rayLength = rayDir.Length();

    if ( rayLength < 0.0001f ) {
        return 0;
    }

    //rayDir = rayDir / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float hitMin, hitMax;

    int n = 0;
    for ( int nodeIndex = 0; nodeIndex < Nodes.Size(); ) {
        SNodeAABB const * node = &Nodes[ nodeIndex ];

        const bool bOverlap = BvRayIntersectBox( _RayStart, invRayDir, node->Bounds, hitMin, hitMax ) && hitMin <= 1.0f;// rayLength;
        const bool bLeaf = node->IsLeaf();

        if ( bLeaf && bOverlap ) {
            _MarkLeafs[ n++ ] = nodeIndex;
            if ( n == _MaxLeafs ) {
                return n;
            }
        }
        nodeIndex += ( bOverlap || bLeaf ) ? 1 : ( -node->Index );
    }

    return n;
}

void ATreeAABB::Read( IBinaryStream & _Stream ) {
    _Stream.ReadArrayOfStructs( Nodes );
    _Stream.ReadArrayUInt32( Indirection );
    _Stream.ReadObject( BoundingBox );
}

void ATreeAABB::Write( IBinaryStream & _Stream ) const {
    _Stream.WriteArrayOfStructs( Nodes );
    _Stream.WriteArrayUInt32( Indirection );
    _Stream.WriteObject( BoundingBox );
}

void ATreeAABB::Subdivide( SAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _MaxPrimitive, unsigned int _PrimitivesPerLeaf, int & _PrimitiveIndex )
{
    int primCount = _MaxPrimitive - _FirstPrimitive;
    int curNodeInex = Nodes.Size();

    SPrimitiveBounds * pPrimitives = _Build.Primitives[_Axis].ToPtr() + _FirstPrimitive;

    SNodeAABB & node = Nodes.Append();

    CalcNodeBounds( pPrimitives, primCount, node.Bounds );

    if ( primCount <= _PrimitivesPerLeaf ) {
        // Leaf

        node.Index = _PrimitiveIndex;
        node.PrimitiveCount = primCount;

        for ( int i = 0; i < primCount; ++i ) {
            Indirection[ _PrimitiveIndex + i ] = pPrimitives[ i ].PrimitiveIndex;// * 3; // FIXME * 3
        }

        _PrimitiveIndex += primCount;

    } else {
        // Node
        SBestSplitResult s = FindBestSplitPrimitive( _Build, _Axis, _FirstPrimitive, primCount );

        int mid = _FirstPrimitive + s.PrimitiveIndex;

        Subdivide( _Build, s.Axis, _FirstPrimitive, mid, _PrimitivesPerLeaf, _PrimitiveIndex );
        Subdivide( _Build, s.Axis, mid, _MaxPrimitive, _PrimitivesPerLeaf, _PrimitiveIndex );

        int nextNode = Nodes.Size() - curNodeInex;
        node.Index = -nextNode;
    }
}
