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

#include <World/Public/Components/MeshComponent.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/World.h>
#include <World/Public/Base/ResourceManager.h>

ARuntimeVariable RVDrawMeshBounds( _CTS( "DrawMeshBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawIndexedMeshBVH( _CTS( "DrawIndexedMeshBVH" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( AMeshComponent )

AMeshComponent::AMeshComponent() {
    bLightPass = true;
    bCastShadow = true;
    LightmapOffset.Z = LightmapOffset.W = 1;

    static TStaticResourceFinder< AIndexedMesh > MeshResource( _CTS( "/Default/Meshes/Box" ) );
    Mesh = MeshResource.GetObject();
    Bounds = Mesh->GetBoundingBox();
}

void AMeshComponent::InitializeComponent() {
    ARenderWorld & RenderWorld = GetWorld()->GetRenderWorld();

    Super::InitializeComponent();

    RenderWorld.AddMesh( this );

    if ( bCastShadow )
    {
        RenderWorld.AddShadowCaster( this );
    }
}

void AMeshComponent::DeinitializeComponent() {
    ARenderWorld & RenderWorld = GetWorld()->GetRenderWorld();

    Super::DeinitializeComponent();

    ClearMaterials();

    RenderWorld.RemoveMesh( this );

    if ( bCastShadow )
    {
        RenderWorld.RemoveShadowCaster( this );
    }
}

void AMeshComponent::SetCastShadow( bool _CastShadow ) {
    if ( bCastShadow == _CastShadow )
    {
        return;
    }

    bCastShadow = _CastShadow;

    if ( IsInitialized() )
    {
        ARenderWorld & RenderWorld = GetWorld()->GetRenderWorld();

        if ( bCastShadow )
        {
            RenderWorld.AddShadowCaster( this );
        }
        else
        {
            RenderWorld.RemoveShadowCaster( this );
        }
    }
}

void AMeshComponent::SetMesh( AIndexedMesh * _Mesh ) {
    if ( IsSame( Mesh, _Mesh ) ) {
        return;
    }

    Mesh = _Mesh;

    for ( SSocket & socket : Sockets ) {
        socket.SocketDef->RemoveRef();
    }

    if ( !Mesh ) {
        static TStaticResourceFinder< AIndexedMesh > MeshResource( _CTS( "/Default/Meshes/Box" ) );
        Mesh = MeshResource.GetObject();
    }

    // Update bounding box
    Bounds = Mesh->GetBoundingBox();

    // Update sockets
    TPodArray< ASocketDef * > const & socketDef = Mesh->GetSockets();
    Sockets.ResizeInvalidate( socketDef.Size() );
    for ( int i = 0 ; i < socketDef.Size() ; i++ ) {
        socketDef[i]->AddRef();
        Sockets[i].SocketDef = socketDef[i];
        Sockets[i].SkinnedMesh = IsSkinnedMesh() ? static_cast< ASkinnedComponent * >( this ) : nullptr;
    }

    NotifyMeshChanged();

    // Mark to update world bounds
    MarkWorldBoundsDirty();
}

void AMeshComponent::ClearMaterials() {
    for ( AMaterialInstance * material : Materials ) {
        if ( material ) {
            material->RemoveRef();
        }
    }
    Materials.Clear();
}

void AMeshComponent::CopyMaterialsFromMeshResource() {
    ClearMaterials();

    AIndexedMeshSubpartArray const & subparts = Mesh->GetSubparts();
    for ( int i = 0 ; i < subparts.Size() ; i++ ) {
        SetMaterialInstance( i, subparts[ i ]->GetMaterialInstance() );
    }
}

void AMeshComponent::SetMaterialInstance( int _SubpartIndex, AMaterialInstance * _Instance ) {
    AN_ASSERT( _SubpartIndex >= 0 );

    if ( _SubpartIndex >= Materials.Size() ) {

        if ( _Instance ) {
            int n = Materials.Size();

            Materials.Resize( n + _SubpartIndex + 1 );
            for ( ; n < Materials.Size() ; n++ ) {
                Materials[n] = nullptr;
            }
            Materials[_SubpartIndex] = _Instance;
            _Instance->AddRef();
        }

        return;
    }

    if ( Materials[_SubpartIndex] ) {
        Materials[_SubpartIndex]->RemoveRef();
    }
    Materials[_SubpartIndex] = _Instance;
    if ( _Instance ) {
        _Instance->AddRef();
    }
}

AMaterialInstance * AMeshComponent::GetMaterialInstanceUnsafe( int _SubpartIndex ) const {
    if ( _SubpartIndex < 0 ) {
        return nullptr;
    }

    if ( bOverrideMeshMaterials ) {
        if ( _SubpartIndex >= Materials.Size() ) {
            return nullptr;
        }
        return Materials[_SubpartIndex];
    }

    AIndexedMeshSubpartArray const & subparts = Mesh->GetSubparts();
    if ( _SubpartIndex >= subparts.Size() ) {
        return nullptr;
    }

    return subparts[_SubpartIndex]->GetMaterialInstance();
}

AMaterialInstance * AMeshComponent::GetMaterialInstance( int _SubpartIndex ) const {
    AMaterialInstance * pInstance = GetMaterialInstanceUnsafe( _SubpartIndex );
    if ( !pInstance ) {
        static TStaticResourceFinder< AMaterialInstance > DefaultInstance( _CTS( "/Default/MaterialInstance/Default" ) );
        pInstance = DefaultInstance.GetObject();
    }
    return pInstance;
}

BvAxisAlignedBox AMeshComponent::GetSubpartWorldBounds( int _SubpartIndex ) const {
    AIndexedMeshSubpart const * subpart = const_cast< AMeshComponent * >( this )->Mesh->GetSubpart( _SubpartIndex );
    if ( !subpart ) {
        GLogger.Printf( "AMeshComponent::GetSubpartWorldBounds: invalid subpart index\n" );
        return BvAxisAlignedBox::Empty();
    }
    return subpart->GetBoundingBox().Transform( GetWorldTransformMatrix() );
}

ACollisionBodyComposition const & AMeshComponent::DefaultBodyComposition() const {
    return Mesh->BodyComposition;
}

void AMeshComponent::NotifyMeshChanged() {
    OnMeshChanged();
}

void AMeshComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawIndexedMeshBVH )
    {
        Mesh->DrawBVH( InRenderer, GetWorldTransformMatrix() );
    }

    if ( RVDrawMeshBounds )
    {
        InRenderer->SetDepthTest( true );

        if ( IsSkinnedMesh() )
        {
            InRenderer->SetColor( AColor4( 0.5f,0.5f,1,1 ) );
        }
        else
        {
            InRenderer->SetColor( AColor4( 1,1,1,1 ) );
        }

        InRenderer->DrawAABB( GetWorldBounds() );
    }
}
