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
#include <Engine/Resource/Public/GLTF.h>

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Public/BV/BvIntersect.h>

AN_CLASS_META( FIndexedMesh )
AN_CLASS_META( FIndexedMeshSubpart )
AN_CLASS_META( FLightmapUV )
AN_CLASS_META( FVertexLight )
AN_CLASS_META( FAABBTree )
AN_CLASS_META( FSocketDef )

FRuntimeVariable RVDrawIndexedMeshBVH( _CTS( "DrawIndexedMeshBVH" ), _CTS( "0" ), VAR_CHEAT );

///////////////////////////////////////////////////////////////////////////////////////////////////////

FIndexedMesh::FIndexedMesh() {
    VertexBufferGPU = GRenderBackend->CreateBuffer( this );
    IndexBufferGPU = GRenderBackend->CreateBuffer( this );
    WeightsBufferGPU = GRenderBackend->CreateBuffer( this );
}

FIndexedMesh::~FIndexedMesh() {
    GRenderBackend->DestroyBuffer( VertexBufferGPU );
    GRenderBackend->DestroyBuffer( IndexBufferGPU );
    GRenderBackend->DestroyBuffer( WeightsBufferGPU );

    Purge();
}

void FIndexedMesh::Initialize( int _NumVertices, int _NumIndices, int _NumSubparts, bool _SkinnedMesh, bool _DynamicStorage ) {
    //if ( VertexCount == _NumVertices && IndexCount == _NumIndices && bSkinnedMesh == _SkinnedMesh && bDynamicStorage == _DynamicStorage ) {
    //    return;
    //}

    Purge();

    bSkinnedMesh = _SkinnedMesh;
    bDynamicStorage = _DynamicStorage;
    bBoundingBoxDirty = true;
    BoundingBox.Clear();
    BindposeBounds.Clear();

    Vertices.ResizeInvalidate( _NumVertices );
    if ( bSkinnedMesh ) {
        Weights.ResizeInvalidate( _NumVertices );
    }
    Indices.ResizeInvalidate( _NumIndices );

    GRenderBackend->InitializeBuffer( VertexBufferGPU, Vertices.Size() * sizeof( FMeshVertex ), bDynamicStorage );
    GRenderBackend->InitializeBuffer( IndexBufferGPU, Indices.Size() * sizeof( unsigned int ), bDynamicStorage );

    if ( _SkinnedMesh ) {
        GRenderBackend->InitializeBuffer( WeightsBufferGPU, Weights.Size() * sizeof( FMeshVertexJoint ), bDynamicStorage );
    }

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
        subpart->VertexCount = Vertices.Size();
        subpart->IndexCount = Indices.Size();
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

    for ( FSocketDef * socket : Sockets ) {
        socket->RemoveRef();
    }

    Sockets.Clear();

    //Joints.Clear();

    BodyComposition.Clear();
}

bool FIndexedMesh::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {

    FMeshAsset asset;

    int i = FString::FindExt( _Path );
    if ( !FString::Icmp( &_Path[i], ".gltf" ) ) {
        LoadGeometryGLTF( _Path, asset );
    } else {
        FFileStream f;

        if ( !f.OpenRead( _Path ) ) {

            if ( _CreateDefultObjectIfFails ) {
                InitializeDefaultObject();
                return true;
            }

            return false;
        }
    
        asset.Read( f );

//        // TODO: объединить и хранить скелет в файле с мешем
//        FString skeletonAsset = _Path;
//        skeletonAsset.Resize( i );
//        skeletonAsset += ".angie_skeleton";
//        if ( f.OpenRead( skeletonAsset.ToConstChar() ) ) {
//            skelAsset.Read( f );
//        }
    }

    TPodArray< FMaterialInstance * > matInstances;
    matInstances.Resize( asset.Materials.Size() );

    static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.DefaultPBR" ) );

    for ( int j = 0; j < asset.Materials.Size(); j++ ) {

        FMaterialInstance * matInst = CreateInstanceOf< FMaterialInstance >();
        matInstances[ j ] = matInst;

        matInst->SetMaterial( MaterialResource.GetObject() );

        FMeshMaterial const & material = asset.Materials[ j ];
        for ( int n = 0; n < material.NumTextures; n++ ) {
            FMaterialTexture const & texture = asset.Textures[ material.Textures[ n ] ];
            FTexture2D * texObj = GetOrCreateResource< FTexture2D >( texture.FileName.ToConstChar() );
            matInst->SetTexture( n, texObj );
        }
    }

    bool bSkinned = asset.Weights.Size() == asset.Vertices.Size();

    //for ( int j = 0; j < asset.Subparts.size(); j++ ) {
    //    FSubpart const & s = asset.Subparts[j];

    //    CalcTangentSpace( asset.Vertices.ToPtr() + s.BaseVertex, s.VertexCount, asset.Indices.ToPtr() + s.FirstIndex, s.IndexCount );
    //}

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
        if ( s.Material < matInstances.Size() ) {
            subpart->MaterialInstance = matInstances[ s.Material ];
        }
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

    // TODO: load BVH tree for raycast


    // TODO: Read sockets!!!

    //ResetSkeleton( skelAsset.Joints.ToPtr(), skelAsset.Joints.Size(), skelAsset.BindposeBounds );

    return true;
}

FLightmapUV * FIndexedMesh::CreateLightmapUVChannel() {
    FLightmapUV * channel = NewObject< FLightmapUV >();

    channel->OwnerMesh = this;
    channel->IndexInArrayOfUVs = LightmapUVs.Size();

    LightmapUVs.Append( channel );

    channel->OnInitialize( Vertices.Size() );

    return channel;
}

FVertexLight * FIndexedMesh::CreateVertexLightChannel() {
    FVertexLight * channel = NewObject< FVertexLight >();

    channel->OwnerMesh = this;
    channel->IndexInArrayOfChannels = VertexLightChannels.Size();

    VertexLightChannels.Append( channel );

    channel->OnInitialize( Vertices.Size() );

    return channel;
}

//void FIndexedMesh::ResetSkeleton( FJoint const * _Joints, int _JointCount, BvAxisAlignedBox const & _BindposeBounds ) {
//    Joints.ResizeInvalidate( _JointCount );
//    memcpy( Joints.ToPtr(), _Joints, sizeof( FJoint ) * _JointCount );
//    BindposeBounds = _BindposeBounds;
//}

void FIndexedMesh::AddSocket( FSocketDef * _Socket ) {
    _Socket->AddRef();
    Sockets.Append( _Socket );
}

FSocketDef * FIndexedMesh::FindSocket( const char * _Name ) {
    for ( FSocketDef * socket : Sockets ) {
        if ( socket->GetName().Icmp( _Name ) ) {
            return socket;
        }
    }
    return nullptr;
}

//int FIndexedMesh::FindJoint( const char * _Name ) const {
//    for ( int j = 0 ; j < Joints.Size() ; j++ ) {
//        if ( !FString::Icmp( Joints[j].Name, _Name ) ) {
//            return j;
//        }
//    }
//    return -1;
//}

void FIndexedMesh::CreateBVH() {
    if ( bSkinnedMesh ) {
        GLogger.Printf( "FIndexedMesh::CreateBVH: called for skinned mesh\n" );
        return;
    }

    for ( FIndexedMeshSubpart * subpart : Subparts ) {
        subpart->CreateBVH();
    }
}

void FIndexedMesh::SetMaterialInstance( int _SubpartIndex, FMaterialInstance * _MaterialInstance ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
        return;
    }
    Subparts[_SubpartIndex]->SetMaterialInstance( _MaterialInstance );
}

void FIndexedMesh::SetBoundingBox( int _SubpartIndex, BvAxisAlignedBox const & _BoundingBox ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
        return;
    }
    Subparts[_SubpartIndex]->SetBoundingBox( _BoundingBox );
}

FIndexedMeshSubpart * FIndexedMesh::GetSubpart( int _SubpartIndex ) {
    if ( _SubpartIndex < 0 || _SubpartIndex >= Subparts.Size() ) {
        return nullptr;
    }
    return Subparts[ _SubpartIndex ];
}

bool FIndexedMesh::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "FIndexedMesh::SendVertexDataToGPU: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    GRenderBackend->WriteBuffer( VertexBufferGPU, _StartVertexLocation * sizeof( FMeshVertex ), _VerticesCount * sizeof( FMeshVertex ), Vertices.ToPtr() + _StartVertexLocation );

    return true;
}

bool FIndexedMesh::WriteVertexData( FMeshVertex const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "FIndexedMesh::WriteVertexData: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshVertex ) );

    for ( FIndexedMeshSubpart * subpart : Subparts ) {
        subpart->bAABBTreeDirty = true;
    }

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}

bool FIndexedMesh::SendJointWeightsToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "FIndexedMesh::SendJointWeightsToGPU: Cannot write joint weights for static mesh\n" );
        return false;
    }

    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Weights.Size() ) {
        GLogger.Printf( "FIndexedMesh::SendJointWeightsToGPU: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    GRenderBackend->WriteBuffer( WeightsBufferGPU, _StartVertexLocation * sizeof( FMeshVertexJoint ), _VerticesCount * sizeof( FMeshVertexJoint ), Weights.ToPtr() + _StartVertexLocation );

    return true;
}

bool FIndexedMesh::WriteJointWeights( FMeshVertexJoint const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !bSkinnedMesh ) {
        GLogger.Printf( "FIndexedMesh::WriteJointWeights: Cannot write joint weights for static mesh\n" );
        return false;
    }

    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Weights.Size() ) {
        GLogger.Printf( "FIndexedMesh::WriteJointWeights: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    memcpy( Weights.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshVertexJoint ) );

    return SendJointWeightsToGPU( _VerticesCount, _StartVertexLocation );
}

bool FIndexedMesh::SendIndexDataToGPU( int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount ) {
        return true;
    }

    if ( _StartIndexLocation + _IndexCount > Indices.Size() ) {
        GLogger.Printf( "FIndexedMesh::SendIndexDataToGPU: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    GRenderBackend->WriteBuffer( IndexBufferGPU, _StartIndexLocation * sizeof( unsigned int ), _IndexCount * sizeof( unsigned int ), Indices.ToPtr() + _StartIndexLocation );

    return true;
}

bool FIndexedMesh::WriteIndexData( unsigned int const * _Indices, int _IndexCount, int _StartIndexLocation ) {
    if ( !_IndexCount ) {
        return true;
    }

    if ( _StartIndexLocation + _IndexCount > Indices.Size() ) {
        GLogger.Printf( "FIndexedMesh::WriteIndexData: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    memcpy( Indices.ToPtr() + _StartIndexLocation, _Indices, _IndexCount * sizeof( unsigned int ) );

    for ( FIndexedMeshSubpart * subpart : Subparts ) {
        if ( _StartIndexLocation >= subpart->FirstIndex
            && _StartIndexLocation + _IndexCount <= subpart->FirstIndex + subpart->IndexCount ) {
            subpart->bAABBTreeDirty = true;
        }
    }

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

void FIndexedMesh::InitializeSphereMesh( float _Radius, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;
    
    CreateSphereMesh( vertices, indices, bounds, _Radius, _TexCoordScale, _NumVerticalSubdivs, _NumHorizontalSubdivs );

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

void FIndexedMesh::InitializePatchMesh( Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11, float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreatePatchMesh( vertices, indices, bounds,
        Corner00, Corner10, Corner01, Corner11, _TexCoordScale, _TwoSided, _NumVerticalSubdivs, _NumHorizontalSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeCylinderMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateCylinderMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _NumSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeConeMesh( float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateConeMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _NumSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeCapsuleMesh( float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    TPodArray< FMeshVertex > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox bounds;

    CreateCapsuleMesh( vertices, indices, bounds, _Radius, _Height, _TexCoordScale, _NumVerticalSubdivs, _NumHorizontalSubdivs );

    Initialize( vertices.Size(), indices.Size(), 1 );
    WriteVertexData( vertices.ToPtr(), vertices.Size(), 0 );
    WriteIndexData( indices.ToPtr(), indices.Size(), 0 );

    Subparts[ 0 ]->BoundingBox = bounds;
}

void FIndexedMesh::InitializeInternalResource( const char * _InternalResourceName ) {

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Box" )
         || !FString::Icmp( _InternalResourceName, "FIndexedMesh.Default" ) ) {
        InitializeBoxMesh( Float3(1), 1 );
        FCollisionBox * collisionBody = BodyComposition.AddCollisionBody< FCollisionBox >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Sphere" ) ) {
        InitializeSphereMesh( 0.5f, 1 );
        FCollisionSphere * collisionBody = BodyComposition.AddCollisionBody< FCollisionSphere >();
        collisionBody->Radius = 0.5f;
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Cylinder" ) ) {
        InitializeCylinderMesh( 0.5f, 1, 1 );
        FCollisionCylinder * collisionBody = BodyComposition.AddCollisionBody< FCollisionCylinder >();
        collisionBody->HalfExtents = Float3(0.5f);
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Cone" ) ) {
        InitializeConeMesh( 0.5f, 1, 1 );
        FCollisionCone * collisionBody = BodyComposition.AddCollisionBody< FCollisionCone >();
        collisionBody->Radius = 0.5f;
        collisionBody->Height = 1.0f;
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Capsule" ) ) {
        InitializeCapsuleMesh( 0.5f, 1.0f, 1 );
        FCollisionCapsule * collisionBody = BodyComposition.AddCollisionBody< FCollisionCapsule >();
        collisionBody->Radius = 0.5f;
        collisionBody->Height = 1;
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FIndexedMesh.Plane" ) ) {
        InitializePlaneMesh( 256, 256, 256 );
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

void FIndexedMeshSubpart::SetBaseVertex( int _BaseVertex ) {
    BaseVertex = _BaseVertex;
    bAABBTreeDirty = true;
}

void FIndexedMeshSubpart::SetFirstIndex( int _FirstIndex ) {
    FirstIndex = _FirstIndex;
    bAABBTreeDirty = true;
}

void FIndexedMeshSubpart::SetVertexCount( int _VertexCount ) {
    VertexCount = _VertexCount;
}

void FIndexedMeshSubpart::SetIndexCount( int _IndexCount ) {
    IndexCount = _IndexCount;
    bAABBTreeDirty = true;
}

void FIndexedMeshSubpart::SetMaterialInstance( FMaterialInstance * _MaterialInstance ) {
    MaterialInstance = _MaterialInstance;
}

void FIndexedMeshSubpart::SetBoundingBox( BvAxisAlignedBox const & _BoundingBox ) {
    BoundingBox = _BoundingBox;

    OwnerMesh->bBoundingBoxDirty = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

FLightmapUV::FLightmapUV() {
    VertexBufferGPU = GRenderBackend->CreateBuffer( this );
}

FLightmapUV::~FLightmapUV() {
    GRenderBackend->DestroyBuffer( VertexBufferGPU );

    if ( OwnerMesh ) {
        OwnerMesh->LightmapUVs[ IndexInArrayOfUVs ] = OwnerMesh->LightmapUVs[ OwnerMesh->LightmapUVs.Size() - 1 ];
        OwnerMesh->LightmapUVs[ IndexInArrayOfUVs ]->IndexInArrayOfUVs = IndexInArrayOfUVs;
        IndexInArrayOfUVs = -1;
        OwnerMesh->LightmapUVs.RemoveLast();
    }
}

void FLightmapUV::OnInitialize( int _NumVertices ) {
    if ( Vertices.Size() == _NumVertices && bDynamicStorage == OwnerMesh->bDynamicStorage ) {
        return;
    }

    bDynamicStorage = OwnerMesh->bDynamicStorage;

    Vertices.ResizeInvalidate( _NumVertices );

    GRenderBackend->InitializeBuffer( VertexBufferGPU, Vertices.Size() * sizeof( FMeshLightmapUV ), bDynamicStorage );
}

bool FLightmapUV::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "FLightmapUV::SendVertexDataToGPU: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    GRenderBackend->WriteBuffer( VertexBufferGPU, _StartVertexLocation * sizeof( FMeshLightmapUV ), _VerticesCount * sizeof( FMeshLightmapUV ), Vertices.ToPtr() + _StartVertexLocation );

    return true;
}

bool FLightmapUV::WriteVertexData( FMeshLightmapUV const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "FLightmapUV::WriteVertexData: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    memcpy( Vertices.ToPtr() + _StartVertexLocation, _Vertices, _VerticesCount * sizeof( FMeshLightmapUV ) );

    return SendVertexDataToGPU( _VerticesCount, _StartVertexLocation );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

FVertexLight::FVertexLight() {
    VertexBufferGPU = GRenderBackend->CreateBuffer( this );
}

FVertexLight::~FVertexLight() {
    GRenderBackend->DestroyBuffer( VertexBufferGPU );

    if ( OwnerMesh ) {
        OwnerMesh->VertexLightChannels[ IndexInArrayOfChannels ] = OwnerMesh->VertexLightChannels[ OwnerMesh->VertexLightChannels.Size() - 1 ];
        OwnerMesh->VertexLightChannels[ IndexInArrayOfChannels ]->IndexInArrayOfChannels = IndexInArrayOfChannels;
        IndexInArrayOfChannels = -1;
        OwnerMesh->VertexLightChannels.RemoveLast();
    }
}

void FVertexLight::OnInitialize( int _NumVertices ) {
    if ( Vertices.Size() == _NumVertices && bDynamicStorage == OwnerMesh->bDynamicStorage ) {
        return;
    }

    bDynamicStorage = OwnerMesh->bDynamicStorage;

    Vertices.ResizeInvalidate( _NumVertices );

    GRenderBackend->InitializeBuffer( VertexBufferGPU, Vertices.Size() * sizeof( FMeshVertexLight ), bDynamicStorage );
}

bool FVertexLight::SendVertexDataToGPU( int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "FVertexLight::SendVertexDataToGPU: Referencing outside of buffer (%s)\n", GetNameConstChar() );
        return false;
    }

    GRenderBackend->WriteBuffer( VertexBufferGPU, _StartVertexLocation * sizeof( FMeshVertexLight ), _VerticesCount * sizeof( FMeshVertexLight ), Vertices.ToPtr() + _StartVertexLocation );

    return true;
}

bool FVertexLight::WriteVertexData( FMeshVertexLight const * _Vertices, int _VerticesCount, int _StartVertexLocation ) {
    if ( !_VerticesCount ) {
        return true;
    }

    if ( _StartVertexLocation + _VerticesCount > Vertices.Size() ) {
        GLogger.Printf( "FVertexLight::WriteVertexData: Referencing outside of buffer (%s)\n", GetNameConstChar() );
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

    checks.Resize( Vertices.Size()*Vertices.Size() );
    checks.ZeroMem();

    SoftbodyLinks.Clear();

    for ( FSoftbodyFace & face : SoftbodyFaces ) {
        indices = face.Indices;

        for ( int j = 2, k = 0; k < 3; j = k++ ) {

            unsigned int index_j_k = indices[ j ] + indices[ k ]*Vertices.Size();

            // Check if link not exists
            if ( !checks[ index_j_k ] ) {

                unsigned int index_k_j = indices[ k ] + indices[ j ]*Vertices.Size();

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


void FIndexedMeshSubpart::CreateBVH() {
    // TODO: Try KD-tree
    const int PrimitivesPerLeaf = 16;
    AABBTree = NewObject< FAABBTree >();
    AABBTree->Initialize( OwnerMesh->Vertices.ToPtr(), OwnerMesh->Indices.ToPtr() + FirstIndex, IndexCount, BaseVertex, PrimitivesPerLeaf );
    bAABBTreeDirty = false;
}

bool FIndexedMeshSubpart::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const {
    bool ret = false;
    float d, u, v;
    unsigned int const * indices = OwnerMesh->GetIndices() + FirstIndex;
    FMeshVertex const * vertices = OwnerMesh->GetVertices();

    if ( _Distance < 0.0001f ) {
        return false;
    }

    if ( AABBTree ) {
        if ( bAABBTreeDirty ) {
            GLogger.Printf( "FIndexedMeshSubpart::Raycast: bvh is outdated\n" );
            return false;
        }

        Float3 invRayDir;
        invRayDir.X = 1.0f / _RayDir.X;
        invRayDir.Y = 1.0f / _RayDir.Y;
        invRayDir.Z = 1.0f / _RayDir.Z;

        TPodArray< FAABBNode > const & nodes = AABBTree->GetNodes();
        unsigned int const * indirection = AABBTree->GetIndirection();

        float hitMin, hitMax;

        for ( int nodeIndex = 0; nodeIndex < nodes.Size(); ) {
            FAABBNode const * node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox( _RayStart, invRayDir, node->Bounds, hitMin, hitMax ) && hitMin <= _Distance;
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
                    if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v ) ) {
                        if ( _Distance > d ) {
                            FTriangleHitResult & hitResult = _HitResult.Append();
                            hitResult.Location = _RayStart + _RayDir * d;
                            hitResult.Normal = (v1 - v0).Cross( v2-v0 ).Normalized();
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
        // TODO: check subpart AABB

        const int primCount = IndexCount / 3;

        for ( int tri = 0 ; tri < primCount; tri++, indices+=3 ) {
            const unsigned int i0 = BaseVertex + indices[ 0 ];
            const unsigned int i1 = BaseVertex + indices[ 1 ];
            const unsigned int i2 = BaseVertex + indices[ 2 ];

            Float3 const & v0 = vertices[i0].Position;
            Float3 const & v1 = vertices[i1].Position;
            Float3 const & v2 = vertices[i2].Position;

            if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v ) ) {
                if ( _Distance > d ) {
                    FTriangleHitResult & hitResult = _HitResult.Append();
                    hitResult.Location = _RayStart + _RayDir * d;
                    hitResult.Normal = ( v1 - v0 ).Cross( v2-v0 ).Normalized();
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

bool FIndexedMeshSubpart::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3] ) const {
    bool ret = false;
    float d, u, v;
    unsigned int const * indices = OwnerMesh->GetIndices() + FirstIndex;
    FMeshVertex const * vertices = OwnerMesh->GetVertices();

    if ( _Distance < 0.0001f ) {
        return false;
    }

    float minDist = _Distance;

    if ( AABBTree ) {
        if ( bAABBTreeDirty ) {
            GLogger.Printf( "FIndexedMeshSubpart::RaycastClosest: bvh is outdated\n" );
            return false;
        }

        Float3 invRayDir;
        invRayDir.X = 1.0f / _RayDir.X;
        invRayDir.Y = 1.0f / _RayDir.Y;
        invRayDir.Z = 1.0f / _RayDir.Z;

        TPodArray< FAABBNode > const & nodes = AABBTree->GetNodes();
        unsigned int const * indirection = AABBTree->GetIndirection();

        float hitMin, hitMax;

        for ( int nodeIndex = 0; nodeIndex < nodes.Size(); ) {
            FAABBNode const * node = &nodes[nodeIndex];

            const bool bOverlap = BvRayIntersectBox( _RayStart, invRayDir, node->Bounds, hitMin, hitMax ) && hitMin <= _Distance;
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
                    if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v ) ) {
                        if ( minDist > d ) {
                            minDist = d;
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
            }

            nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
        }
    } else {
        // TODO: check subpart AABB

        const int primCount = IndexCount / 3;

        for ( int tri = 0 ; tri < primCount; tri++, indices+=3 ) {
            const unsigned int i0 = BaseVertex + indices[ 0 ];
            const unsigned int i1 = BaseVertex + indices[ 1 ];
            const unsigned int i2 = BaseVertex + indices[ 2 ];

            Float3 const & v0 = vertices[i0].Position;
            Float3 const & v1 = vertices[i1].Position;
            Float3 const & v2 = vertices[i2].Position;

            if ( BvRayIntersectTriangle( _RayStart, _RayDir, v0, v1, v2, d, u, v ) ) {
                if ( minDist > d ) {
                    minDist = d;
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
    }
    return ret;
}

bool FIndexedMesh::Raycast( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, TPodArray< FTriangleHitResult > & _HitResult ) const {
    bool ret = false;

    // TODO: check mesh AABB?

    for ( int i = 0 ; i < Subparts.Size() ; i++ ) {
        FIndexedMeshSubpart * subpart = Subparts[i];
        ret |= subpart->Raycast( _RayStart, _RayDir, _Distance, _HitResult );
    }
    return ret;
}

bool FIndexedMesh::RaycastClosest( Float3 const & _RayStart, Float3 const & _RayDir, float _Distance, Float3 & _HitLocation, Float2 & _HitUV, float & _HitDistance, unsigned int _Indices[3], TRef< FMaterialInstance > & _Material ) const {
    bool ret = false;

    // TODO: check mesh AABB?

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

void FIndexedMesh::DrawDebug( FDebugDraw * _DebugDraw ) {
    if ( RVDrawIndexedMeshBVH ) {
        for ( FIndexedMeshSubpart * subpart : Subparts ) {
            subpart->DrawBVH( _DebugDraw );
        }
    }
}

void FIndexedMeshSubpart::DrawBVH( FDebugDraw * _DebugDraw ) {
    if ( !AABBTree ) {
        return;
    }

    _DebugDraw->SetDepthTest( false );
    _DebugDraw->SetColor( FColor4::White() );

    for ( FAABBNode const & n : AABBTree->GetNodes() ) {
        if ( n.IsLeaf() ) {
            _DebugDraw->DrawAABB( n.Bounds );
        }
    }
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

    TPodArray< Float3 > binormals;
    binormals.ResizeInvalidate( _NumVerts );

    for ( int i = 0; i < _NumVerts; i++ ) {
        _VertexArray[ i ].Tangent = Float3( 0 );
        binormals[ i ] = Float3( 0 );
    }

    for ( unsigned int i = 0; i < _NumIndices; i += 3 ) {
        const unsigned int a = _IndexArray[ i ];
        const unsigned int b = _IndexArray[ i + 1 ];
        const unsigned int c = _IndexArray[ i + 2 ];

        Float3 e1 = _VertexArray[ b ].Position - _VertexArray[ a ].Position;
        Float3 e2 = _VertexArray[ c ].Position - _VertexArray[ a ].Position;
        Float2 et1 = _VertexArray[ b ].TexCoord - _VertexArray[ a ].TexCoord;
        Float2 et2 = _VertexArray[ c ].TexCoord - _VertexArray[ a ].TexCoord;

        const float denom = et1.X*et2.Y - et1.Y*et2.X;
        const float scale = ( fabsf( denom ) < 0.0001f ) ? 1.0f : ( 1.0 / denom );
        tangent = ( e1 * et2.Y - e2 * et1.Y ) * scale;
        binormal = ( e2 * et1.X - e1 * et2.X ) * scale;

        _VertexArray[ a ].Tangent += tangent;
        _VertexArray[ b ].Tangent += tangent;
        _VertexArray[ c ].Tangent += tangent;

        binormals[ a ] += binormal;
        binormals[ b ] += binormal;
        binormals[ c ] += binormal;
    }

    for ( int i = 0; i < _NumVerts; i++ ) {
        const Float3 & n = _VertexArray[ i ].Normal;
        const Float3 & t = _VertexArray[ i ].Tangent;
        _VertexArray[ i ].Tangent = ( t - n * FMath::Dot( n, t ) ).Normalized();
        _VertexArray[ i ].Handedness = CalcHandedness( t, binormals[ i ].Normalized(), n );
    }
}

void CreateBoxMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, const Float3 & _Size, float _TexCoordScale ) {
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

    memcpy( _Indices.ToPtr(), indices, sizeof( indices ) );

    const Float3 halfSize = _Size * 0.5f;

    _Bounds.Mins = -halfSize;
    _Bounds.Maxs = halfSize;

    const Float3 & mins = _Bounds.Mins;
    const Float3 & maxs = _Bounds.Maxs;

    FMeshVertex * pVerts = _Vertices.ToPtr();

    pVerts[ 0 + 8 * 0 ].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[ 0 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 0 + 8 * 0 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 1 + 8 * 0 ].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[ 1 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 1 + 8 * 0 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 2 + 8 * 0 ].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[ 2 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 2 + 8 * 0 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 3 + 8 * 0 ].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[ 3 + 8 * 0 ].Normal = Float3( 0, 0, 1 );
    pVerts[ 3 + 8 * 0 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;


    pVerts[ 4 + 8 * 0 ].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[ 4 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 4 + 8 * 0 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 5 + 8 * 0 ].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[ 5 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 5 + 8 * 0 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 6 + 8 * 0 ].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[ 6 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 6 + 8 * 0 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 7 + 8 * 0 ].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[ 7 + 8 * 0 ].Normal = Float3( 0, 0, -1 );
    pVerts[ 7 + 8 * 0 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;


    pVerts[ 0 + 8 * 1 ].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[ 0 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 0 + 8 * 1 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 1 + 8 * 1 ].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[ 1 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 1 + 8 * 1 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 2 + 8 * 1 ].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[ 2 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 2 + 8 * 1 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    pVerts[ 3 + 8 * 1 ].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[ 3 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 3 + 8 * 1 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;


    pVerts[ 4 + 8 * 1 ].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[ 4 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 4 + 8 * 1 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 5 + 8 * 1 ].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[ 5 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 5 + 8 * 1 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 6 + 8 * 1 ].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[ 6 + 8 * 1 ].Normal = Float3( -1, 0, 0 );
    pVerts[ 6 + 8 * 1 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    pVerts[ 7 + 8 * 1 ].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[ 7 + 8 * 1 ].Normal = Float3( 1, 0, 0 );
    pVerts[ 7 + 8 * 1 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;


    pVerts[ 1 + 8 * 2 ].Position = Float3( maxs.X, mins.Y, maxs.Z ); // 1
    pVerts[ 1 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 1 + 8 * 2 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 0 + 8 * 2 ].Position = Float3( mins.X, mins.Y, maxs.Z ); // 0
    pVerts[ 0 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 0 + 8 * 2 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    pVerts[ 5 + 8 * 2 ].Position = Float3( mins.X, mins.Y, mins.Z ); // 5
    pVerts[ 5 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 5 + 8 * 2 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 4 + 8 * 2 ].Position = Float3( maxs.X, mins.Y, mins.Z ); // 4
    pVerts[ 4 + 8 * 2 ].Normal = Float3( 0, -1, 0 );
    pVerts[ 4 + 8 * 2 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;


    pVerts[ 3 + 8 * 2 ].Position = Float3( mins.X, maxs.Y, maxs.Z ); // 3
    pVerts[ 3 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 3 + 8 * 2 ].TexCoord = Float2( 0, 1 )*_TexCoordScale;

    pVerts[ 2 + 8 * 2 ].Position = Float3( maxs.X, maxs.Y, maxs.Z ); // 2
    pVerts[ 2 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 2 + 8 * 2 ].TexCoord = Float2( 1, 1 )*_TexCoordScale;

    pVerts[ 7 + 8 * 2 ].Position = Float3( maxs.X, maxs.Y, mins.Z ); // 7
    pVerts[ 7 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 7 + 8 * 2 ].TexCoord = Float2( 1, 0 )*_TexCoordScale;

    pVerts[ 6 + 8 * 2 ].Position = Float3( mins.X, maxs.Y, mins.Z ); // 6
    pVerts[ 6 + 8 * 2 ].Normal = Float3( 0, 1, 0 );
    pVerts[ 6 + 8 * 2 ].TexCoord = Float2( 0, 0 )*_TexCoordScale;

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateSphereMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    int x, y;
    float verticalAngle, horizontalAngle;
    unsigned int quad[ 4 ];

    _NumVerticalSubdivs = FMath::Max( _NumVerticalSubdivs, 4 );
    _NumHorizontalSubdivs = FMath::Max( _NumHorizontalSubdivs, 4 );

    _Vertices.ResizeInvalidate( ( _NumHorizontalSubdivs + 1 )*( _NumVerticalSubdivs + 1 ) );
    _Indices.ResizeInvalidate( _NumHorizontalSubdivs * _NumVerticalSubdivs * 6 );

    _Bounds.Mins.X = _Bounds.Mins.Y = _Bounds.Mins.Z = -_Radius;
    _Bounds.Maxs.X = _Bounds.Maxs.Y = _Bounds.Maxs.Z = _Radius;

    FMeshVertex * pVert = _Vertices.ToPtr();

    const float verticalStep = FMath::_PI / _NumVerticalSubdivs;
    const float horizontalStep = FMath::_2PI / _NumHorizontalSubdivs;
    const float verticalScale = 1.0f / _NumVerticalSubdivs;
    const float horizontalScale = 1.0f / _NumHorizontalSubdivs;

    for ( y = 0, verticalAngle = -FMath::_HALF_PI; y <= _NumVerticalSubdivs; y++ ) {
        float h, r;
        FMath::RadSinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            FMath::RadSinCos( horizontalAngle, s, c );
            pVert->Position = Float3( scaledR*c, scaledH, scaledR*s );
            pVert->TexCoord = Float2( 1.0f - static_cast< float >( x ) * horizontalScale, 1.0f - static_cast< float >( y ) * verticalScale ) * _TexCoordScale;
            pVert->Normal.X = r*c;
            pVert->Normal.Y = h;
            pVert->Normal.Z = r*s;
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

void CreatePlaneMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Width, float _Height, float _TexCoordScale ) {
    _Vertices.ResizeInvalidate( 4 );
    _Indices.ResizeInvalidate( 6 );

    const float halfWidth = _Width * 0.5f;
    const float halfHeight = _Height * 0.5f;

    const FMeshVertex Verts[ 4 ] = {
        { Float3( -halfWidth,0,-halfHeight ), Float2( 0,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
        { Float3( -halfWidth,0,halfHeight ), Float2( 0,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
        { Float3( halfWidth,0,halfHeight ), Float2( _TexCoordScale,_TexCoordScale ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) },
        { Float3( halfWidth,0,-halfHeight ), Float2( _TexCoordScale,0 ), Float3( 0,0,1 ), 1.0f, Float3( 0,1,0 ) }
    };

    memcpy( _Vertices.ToPtr(), &Verts, 4 * sizeof( FMeshVertex ) );

    constexpr unsigned int indices[ 6 ] = { 0,1,2,2,3,0 };
    memcpy( _Indices.ToPtr(), &indices, sizeof( indices ) );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );

    _Bounds.Mins.X = -halfWidth;
    _Bounds.Mins.Y = 0.0f;
    _Bounds.Mins.Z = -halfHeight;
    _Bounds.Maxs.X = halfWidth;
    _Bounds.Maxs.Y = 0.0f;
    _Bounds.Maxs.Z = halfHeight;
}

void CreatePatchMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds,
    Float3 const & Corner00, Float3 const & Corner10, Float3 const & Corner01, Float3 const & Corner11,
    float _TexCoordScale, bool _TwoSided, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {

    _NumVerticalSubdivs = FMath::Max( _NumVerticalSubdivs, 2 );
    _NumHorizontalSubdivs = FMath::Max( _NumHorizontalSubdivs, 2 );

    const float scaleX = 1.0f / ( float )( _NumHorizontalSubdivs - 1 );
    const float scaleY = 1.0f / ( float )( _NumVerticalSubdivs - 1 );

    const int vertexCount = _NumHorizontalSubdivs * _NumVerticalSubdivs;
    const int indexCount = ( _NumHorizontalSubdivs - 1 ) * ( _NumVerticalSubdivs - 1 ) * 6;

    Float3 normal = ( Corner10 - Corner00 ).Cross( Corner01 - Corner00 ).Normalized();

    _Vertices.ResizeInvalidate( _TwoSided ? vertexCount << 1 : vertexCount );
    _Indices.ResizeInvalidate( _TwoSided ? indexCount << 1 : indexCount );

    FMeshVertex * pVert = _Vertices.ToPtr();
    unsigned int * pIndices = _Indices.ToPtr();

    for ( int y = 0; y < _NumVerticalSubdivs; ++y ) {
        const float lerpY = y * scaleY;
        const Float3 py0 = Corner00.Lerp( Corner01, lerpY );
        const Float3 py1 = Corner10.Lerp( Corner11, lerpY );
        const float ty = lerpY * _TexCoordScale;

        for ( int x = 0; x < _NumHorizontalSubdivs; ++x ) {
            const float lerpX = x * scaleX;

            pVert->Position = py0.Lerp( py1, lerpX );
            pVert->TexCoord.X = lerpX * _TexCoordScale;
            pVert->TexCoord.Y = ty;
            pVert->Normal = normal;

            ++pVert;
        }
    }

    if ( _TwoSided ) {
        normal = -normal;

        for ( int y = 0; y < _NumVerticalSubdivs; ++y ) {
            const float lerpY = y * scaleY;
            const Float3 py0 = Corner00.Lerp( Corner01, lerpY );
            const Float3 py1 = Corner10.Lerp( Corner11, lerpY );
            const float ty = lerpY * _TexCoordScale;

            for ( int x = 0; x < _NumHorizontalSubdivs; ++x ) {
                const float lerpX = x * scaleX;

                pVert->Position = py0.Lerp( py1, lerpX );
                pVert->TexCoord.X = lerpX * _TexCoordScale;
                pVert->TexCoord.Y = ty;
                pVert->Normal = normal;

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

void CreateCylinderMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    int i, j;
    float angle;
    unsigned int quad[ 4 ];

    _NumSubdivs = FMath::Max( _NumSubdivs, 4 );

    const float invSubdivs = 1.0f / _NumSubdivs;
    const float angleStep = FMath::_2PI * invSubdivs;
    const float halfHeight = _Height * 0.5f;

    _Vertices.ResizeInvalidate( 6 * ( _NumSubdivs + 1 ) );
    _Indices.ResizeInvalidate( 3 * _NumSubdivs * 6 );

    _Bounds.Mins.X = -_Radius;
    _Bounds.Mins.Z = -_Radius;
    _Bounds.Mins.Y = -halfHeight;

    _Bounds.Maxs.X = _Radius;
    _Bounds.Maxs.Z = _Radius;
    _Bounds.Maxs.Y = halfHeight;

    FMeshVertex * pVerts = _Vertices.ToPtr();

    int firstVertex = 0;

    for ( j = 0; j <= _NumSubdivs; j++ ) {
        pVerts[ firstVertex + j ].Position = Float3( 0.0f, -halfHeight, 0.0f );
        pVerts[ firstVertex + j ].TexCoord = Float2( j * invSubdivs, 0.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, -halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].TexCoord = Float2( j * invSubdivs, 1.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, -halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].TexCoord = Float2( 1.0f - j * invSubdivs, 1.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( c, 0.0f, s );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].TexCoord = Float2( 1.0f - j * invSubdivs, 0.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( c, 0.0f, s );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, halfHeight, _Radius*s );
        pVerts[ firstVertex + j ].TexCoord = Float2( j * invSubdivs, 0.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( 0, 1.0f, 0 );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0; j <= _NumSubdivs; j++ ) {
        pVerts[ firstVertex + j ].Position = Float3( 0.0f, halfHeight, 0.0f );
        pVerts[ firstVertex + j ].TexCoord = Float2( j * invSubdivs, 1.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( 0, 1.0f, 0 );
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

void CreateConeMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumSubdivs ) {
    int i, j;
    float angle;
    unsigned int quad[ 4 ];

    _NumSubdivs = FMath::Max( _NumSubdivs, 4 );

    const float invSubdivs = 1.0f / _NumSubdivs;
    const float angleStep = FMath::_2PI * invSubdivs;

    _Vertices.ResizeInvalidate( 4 * ( _NumSubdivs + 1 ) );
    _Indices.ResizeInvalidate( 2 * _NumSubdivs * 6 );

    _Bounds.Mins.X = -_Radius;
    _Bounds.Mins.Z = -_Radius;
    _Bounds.Mins.Y = 0;

    _Bounds.Maxs.X = _Radius;
    _Bounds.Maxs.Z = _Radius;
    _Bounds.Maxs.Y = _Height;

    FMeshVertex * pVerts = _Vertices.ToPtr();

    int firstVertex = 0;

    for ( j = 0; j <= _NumSubdivs; j++ ) {
        pVerts[ firstVertex + j ].Position = Float3::Zero();
        pVerts[ firstVertex + j ].TexCoord = Float2( j * invSubdivs, 0.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, 0.0f, _Radius*s );
        pVerts[ firstVertex + j ].TexCoord = Float2( j * invSubdivs, 1.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( 0, -1.0f, 0 );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( _Radius*c, 0.0f, _Radius*s );
        pVerts[ firstVertex + j ].TexCoord = Float2( 1.0f - j * invSubdivs, 1.0f ) * _TexCoordScale;
        pVerts[ firstVertex + j ].Normal = Float3( c, 0.0f, s );
        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    Float3 vx;
    Float3 vy( 0, _Height, 0);
    Float3 v;
    for ( j = 0, angle = 0; j <= _NumSubdivs; j++ ) {
        float s, c;
        FMath::RadSinCos( angle, s, c );
        pVerts[ firstVertex + j ].Position = Float3( 0, _Height, 0 );
        pVerts[ firstVertex + j ].TexCoord = Float2( 1.0f - j * invSubdivs, 0.0f ) * _TexCoordScale;

        vx = Float3( c, 0.0f, s );
        v = vy - vx;
        pVerts[ firstVertex + j ].Normal = FMath::Cross( FMath::Cross( v, vx ), v ).Normalized();

        angle += angleStep;
    }
    firstVertex += _NumSubdivs + 1;

    AN_Assert( firstVertex == _Vertices.Size() );

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

    AN_Assert( pIndices == _Indices.ToPtr() + _Vertices.Size() );

    CalcTangentSpace( _Vertices.ToPtr(), _Vertices.Size(), _Indices.ToPtr(), _Indices.Size() );
}

void CreateCapsuleMesh( TPodArray< FMeshVertex > & _Vertices, TPodArray< unsigned int > & _Indices, BvAxisAlignedBox & _Bounds, float _Radius, float _Height, float _TexCoordScale, int _NumVerticalSubdivs, int _NumHorizontalSubdivs ) {
    int x, y, tcY;
    float verticalAngle, horizontalAngle;
    const float halfHeight = _Height * 0.5f;
    unsigned int quad[ 4 ];

    _NumVerticalSubdivs = FMath::Max( _NumVerticalSubdivs, 4 );
    _NumHorizontalSubdivs = FMath::Max( _NumHorizontalSubdivs, 4 );

    const int halfVerticalSubdivs = _NumVerticalSubdivs >> 1;

    _Vertices.ResizeInvalidate( ( _NumHorizontalSubdivs + 1 ) * ( _NumVerticalSubdivs + 1 ) * 2 );
    _Indices.ResizeInvalidate( _NumHorizontalSubdivs * ( _NumVerticalSubdivs + 1 ) * 6 );

    _Bounds.Mins.X = _Bounds.Mins.Z = -_Radius;
    _Bounds.Mins.Y =  -_Radius - halfHeight;
    _Bounds.Maxs.X = _Bounds.Maxs.Z = _Radius;
    _Bounds.Maxs.Y = _Radius + halfHeight;

    FMeshVertex * pVert = _Vertices.ToPtr();

    const float verticalStep = FMath::_PI / _NumVerticalSubdivs;
    const float horizontalStep = FMath::_2PI / _NumHorizontalSubdivs;
    const float verticalScale = 1.0f / ( _NumVerticalSubdivs + 1 );
    const float horizontalScale = 1.0f / _NumHorizontalSubdivs;

    tcY = 0;

    for ( y = 0, verticalAngle = -FMath::_HALF_PI; y <= halfVerticalSubdivs; y++, tcY++ ) {
        float h, r;
        FMath::RadSinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        const float posY = scaledH - halfHeight;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            FMath::RadSinCos( horizontalAngle, s, c );
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->TexCoord.X = ( 1.0f - static_cast< float >( x ) * horizontalScale ) * _TexCoordScale;
            pVert->TexCoord.Y = ( 1.0f - static_cast< float >( tcY ) * verticalScale ) * _TexCoordScale;
            pVert->Normal.X = r * c;
            pVert->Normal.Y = h;
            pVert->Normal.Z = r * s;
            pVert++;
            horizontalAngle += horizontalStep;
        }
        verticalAngle += verticalStep;
    }

    for ( y = 0, verticalAngle = 0; y <= halfVerticalSubdivs; y++, tcY++ ) {
        float h, r;
        FMath::RadSinCos( verticalAngle, h, r );
        const float scaledH = h * _Radius;
        const float scaledR = r * _Radius;
        const float posY = scaledH + halfHeight;
        for ( x = 0, horizontalAngle = 0; x <= _NumHorizontalSubdivs; x++ ) {
            float s, c;
            FMath::RadSinCos( horizontalAngle, s, c );
            pVert->Position.X = scaledR * c;
            pVert->Position.Y = posY;
            pVert->Position.Z = scaledR * s;
            pVert->TexCoord.X = ( 1.0f - static_cast< float >( x ) * horizontalScale ) * _TexCoordScale;
            pVert->TexCoord.Y = ( 1.0f - static_cast< float >( tcY ) * verticalScale ) * _TexCoordScale;
            pVert->Normal.X = r * c;
            pVert->Normal.Y = h;
            pVert->Normal.Z = r * s;
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

struct FPrimitiveBounds {
    BvAxisAlignedBox Bounds;
    int PrimitiveIndex;
};

struct FBestSplitResult {
    int Axis;
    int PrimitiveIndex;
};

struct FAABBTreeBuild {
    TPodArray< BvAxisAlignedBox > RightBounds;
    TPodArray< FPrimitiveBounds > Primitives[3];
};

static void CalcNodeBounds( FPrimitiveBounds const * _Primitives, int _PrimCount, BvAxisAlignedBox & _Bounds ) {
    AN_Assert( _PrimCount > 0 );

    FPrimitiveBounds const * primitive = _Primitives;

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

static FBestSplitResult FindBestSplitPrimitive( FAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _PrimCount ) {
    struct CompareBoundsMax {
        CompareBoundsMax( const int _Axis ) : Axis( _Axis ) {}
        bool operator()( FPrimitiveBounds const & a, FPrimitiveBounds const & b ) const {
            return ( a.Bounds.Maxs[ Axis ] < b.Bounds.Maxs[ Axis ] );
        }
        const int Axis;
    };

    FPrimitiveBounds * primitives[ 3 ] = {
        _Build.Primitives[ 0 ].ToPtr() + _FirstPrimitive,
        _Build.Primitives[ 1 ].ToPtr() + _FirstPrimitive,
        _Build.Primitives[ 2 ].ToPtr() + _FirstPrimitive
    };

    for ( int i = 0; i < 3; i++ ) {
        if ( i != _Axis ) {
            memcpy( primitives[ i ], primitives[ _Axis ], sizeof( FPrimitiveBounds ) * _PrimCount );
        }
    }

    BvAxisAlignedBox right;
    BvAxisAlignedBox left;

    FBestSplitResult result;
    result.Axis = -1;

    float bestSAH = Float::MaxValue(); // Surface area heuristic

    const float emptyCost = 1.0f;

    for ( int axis = 0; axis < 3; axis++ ) {
        FPrimitiveBounds * primBounds = primitives[ axis ];

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

    AN_Assert( ( result.Axis != -1 ) && ( bestSAH < Float::MaxValue() ) );

    return result;
}

void FAABBTree::Subdivide( FAABBTreeBuild & _Build, int _Axis, int _FirstPrimitive, int _MaxPrimitive, unsigned int _PrimitivesPerLeaf,
    int & _PrimitiveIndex, const unsigned int * _Indices )
{
    int primCount = _MaxPrimitive - _FirstPrimitive;
    int curNodeInex = Nodes.Size();

    FPrimitiveBounds * pPrimitives = _Build.Primitives[_Axis].ToPtr() + _FirstPrimitive;

    FAABBNode & node = Nodes.Append();

    CalcNodeBounds( pPrimitives, primCount, node.Bounds );

    if ( primCount <= _PrimitivesPerLeaf ) {
        // Leaf

        node.Index = _PrimitiveIndex;
        node.PrimitiveCount = primCount;

        for ( int i = 0; i < primCount; ++i ) {
            Indirection[ _PrimitiveIndex + i ] = pPrimitives[ i ].PrimitiveIndex * 3;
        }

        _PrimitiveIndex += primCount;

    } else {
        // Node
        FBestSplitResult s = FindBestSplitPrimitive( _Build, _Axis, _FirstPrimitive, primCount );

        int mid = _FirstPrimitive + s.PrimitiveIndex;

        Subdivide( _Build, s.Axis, _FirstPrimitive, mid, _PrimitivesPerLeaf, _PrimitiveIndex, _Indices );
        Subdivide( _Build, s.Axis, mid, _MaxPrimitive, _PrimitivesPerLeaf, _PrimitiveIndex, _Indices );

        int nextNode = Nodes.Size() - curNodeInex;
        node.Index = -nextNode;
    }
}

void FAABBTree::Initialize( FMeshVertex const * _Vertices, unsigned int const * _Indices, unsigned int _IndexCount, int _BaseVertex, unsigned int _PrimitivesPerLeaf ) {
    Purge();

    _PrimitivesPerLeaf = FMath::Max( _PrimitivesPerLeaf, 16u );

    int primCount = _IndexCount / 3;

    int numLeafs = ( primCount + _PrimitivesPerLeaf - 1 ) / _PrimitivesPerLeaf;

    Nodes.Clear();
    Nodes.ReserveInvalidate( numLeafs * 4 );

    Indirection.ResizeInvalidate( primCount );

    FAABBTreeBuild build;
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

        FPrimitiveBounds & primitive = build.Primitives[ 0 ][ primitiveIndex ];
        primitive.PrimitiveIndex = primitiveIndex;

        primitive.Bounds.Mins.X = FMath::Min( v0.X, v1.X, v2.X );
        primitive.Bounds.Mins.Y = FMath::Min( v0.Y, v1.Y, v2.Y );
        primitive.Bounds.Mins.Z = FMath::Min( v0.Z, v1.Z, v2.Z );

        primitive.Bounds.Maxs.X = FMath::Max( v0.X, v1.X, v2.X );
        primitive.Bounds.Maxs.Y = FMath::Max( v0.Y, v1.Y, v2.Y );
        primitive.Bounds.Maxs.Z = FMath::Max( v0.Z, v1.Z, v2.Z );
    }

    primitiveIndex = 0;
    Subdivide( build, 0, 0, primCount, _PrimitivesPerLeaf, primitiveIndex, _Indices );
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

void FAABBTree::Purge() {
    Nodes.Free();
    Indirection.Free();
}

int FAABBTree::MarkBoxOverlappingLeafs( BvAxisAlignedBox const & _Bounds, unsigned int * _MarkLeafs, int _MaxLeafs ) const {
    if ( !_MaxLeafs ) {
        return 0;
    }
    int n = 0;
    for ( int nodeIndex = 0; nodeIndex < Nodes.Size(); ) {
        FAABBNode const * node = &Nodes[ nodeIndex ];

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

int FAABBTree::MarkRayOverlappingLeafs( Float3 const & _RayStart, Float3 const & _RayEnd, unsigned int * _MarkLeafs, int _MaxLeafs ) const {
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
        FAABBNode const * node = &Nodes[ nodeIndex ];

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



#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////////

void FSkeletonAsset::Clear() {
    Joints.Clear();
    BindposeBounds.Clear();
}

void FSkeletonAsset::Read( FFileStream & f ) {
    char buf[1024];
    char * s;
    int format, version;

    Clear();

    if ( !AssetReadFormat( f, &format, &version ) ) {
        return;
    }

    if ( format != FMT_FILE_TYPE_SKELETON ) {
        GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_SKELETON );
        return;
    }

    if ( version != FMT_VERSION_SKELETON ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_SKELETON );
        return;
    }

    while ( f.Gets( buf, sizeof( buf ) ) ) {
        if ( nullptr != ( s = AssetParseTag( buf, "joints " ) ) ) {
            int numJoints = 0;
            sscanf( s, "%d", &numJoints );
            Joints.ResizeInvalidate( numJoints );
            for ( int jointIndex = 0 ; jointIndex < numJoints ; jointIndex++ ) {
                if ( !f.Gets( buf, sizeof( buf ) ) ) {
                    GLogger.Printf( "Unexpected EOF\n" );
                    return;
                }

                char * name;
                s = AssetParseName( buf, &name );

                FJoint & joint = Joints[jointIndex];
                FString::CopySafe( joint.Name, sizeof( joint.Name ), name );

                joint.LocalTransform.SetIdentity();

                sscanf( s, "%d ( ( %f %f %f %f ) ( %f %f %f %f ) ( %f %f %f %f ) ) ( ( %f %f %f %f ) ( %f %f %f %f ) ( %f %f %f %f ) )", &joint.Parent,
                        &joint.OffsetMatrix[0][0], &joint.OffsetMatrix[0][1], &joint.OffsetMatrix[0][2], &joint.OffsetMatrix[0][3],
                        &joint.OffsetMatrix[1][0], &joint.OffsetMatrix[1][1], &joint.OffsetMatrix[1][2], &joint.OffsetMatrix[1][3],
                        &joint.OffsetMatrix[2][0], &joint.OffsetMatrix[2][1], &joint.OffsetMatrix[2][2], &joint.OffsetMatrix[2][3],
                        &joint.LocalTransform[0][0], &joint.LocalTransform[0][1], &joint.LocalTransform[0][2], &joint.LocalTransform[0][3],
                        &joint.LocalTransform[1][0], &joint.LocalTransform[1][1], &joint.LocalTransform[1][2], &joint.LocalTransform[1][3],
                        &joint.LocalTransform[2][0], &joint.LocalTransform[2][1], &joint.LocalTransform[2][2], &joint.LocalTransform[2][3] );
            }
        } else if ( nullptr != ( s = AssetParseTag( buf, "bindpose_bounds " ) ) ) {

            sscanf( s, "( %f %f %f ) ( %f %f %f )",
                    &BindposeBounds.Mins.X, &BindposeBounds.Mins.Y, &BindposeBounds.Mins.Z,
                    &BindposeBounds.Maxs.X, &BindposeBounds.Maxs.Y, &BindposeBounds.Maxs.Z );

        } else {
            GLogger.Printf( "Unknown tag '%s'\n", buf );
        }
    }
}

void FSkeletonAsset::Write( FFileStream & f ) {
    f.Printf( "format %d %d\n", FMT_FILE_TYPE_SKELETON, FMT_VERSION_SKELETON );
    f.Printf( "joints %d\n", Joints.Size() );
    for ( FJoint & joint : Joints ) {
        f.Printf( "\"%s\" %d %s %s\n",
                  joint.Name, joint.Parent,
                  joint.OffsetMatrix.ToString().ToConstChar(),
                  joint.LocalTransform.ToString().ToConstChar() );
    }
    f.Printf( "bindpose_bounds %s %s\n", BindposeBounds.Mins.ToString().ToConstChar(), BindposeBounds.Maxs.ToString().ToConstChar() );
}

void FSkeletonAsset::CalcBindposeBounds( FMeshAsset const * InMeshData ) {
    Float3x4 absoluteTransforms[FSkeleton::MAX_JOINTS+1];
    Float3x4 vertexTransforms[FSkeleton::MAX_JOINTS];

    BindposeBounds.Clear();

    absoluteTransforms[0].SetIdentity();
    for ( unsigned int j = 0 ; j < Joints.Size() ; j++ ) {
        FJoint const & joint = Joints[ j ];

        absoluteTransforms[ j + 1 ] = absoluteTransforms[ joint.Parent + 1 ] * joint.LocalTransform;

        vertexTransforms[ j ] = absoluteTransforms[ j + 1 ] * joint.OffsetMatrix;
    }

    for ( int v = 0 ; v < InMeshData->Vertices.Size() ; v++ ) {
        Float4 const position = Float4( InMeshData->Vertices[v].Position, 1.0f );
        FMeshVertexJoint const & w = InMeshData->Weights[v];

        const float weights[4] = { w.JointWeights[0] / 255.0f, w.JointWeights[1] / 255.0f, w.JointWeights[2] / 255.0f, w.JointWeights[3] / 255.0f };

        Float4 const * t = &vertexTransforms[0][0];

        BindposeBounds.AddPoint(
                    ( t[ w.JointIndices[0] * 3 + 0 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 0 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 0 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 0 ] * weights[3] ).Dot( position ),

                    ( t[ w.JointIndices[0] * 3 + 1 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 1 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 1 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 1 ] * weights[3] ).Dot( position ),

                    ( t[ w.JointIndices[0] * 3 + 2 ] * weights[0]
                    + t[ w.JointIndices[1] * 3 + 2 ] * weights[1]
                    + t[ w.JointIndices[2] * 3 + 2 ] * weights[2]
                    + t[ w.JointIndices[3] * 3 + 2 ] * weights[3] ).Dot( position ) );
    }
}
#endif
