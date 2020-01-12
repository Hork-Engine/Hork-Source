/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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
#include <Core/Public/Logger.h>

ARuntimeVariable RVDrawMeshBounds( _CTS( "DrawMeshBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawBrushBounds( _CTS( "DrawBrushBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawIndexedMeshBVH( _CTS( "DrawIndexedMeshBVH" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( AMeshComponent )
AN_CLASS_META( ABrushComponent )

static bool RaycastCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits ) {
    AMeshComponent const * mesh = static_cast< AMeshComponent const * >( Self->Owner );

    if ( mesh->bUseDynamicRange ) {
        // Raycasting of meshes with dynamic range is not supported yet
        return false;
    }

    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    AIndexedMesh * resource = mesh->GetMesh();

    return resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, Hits );
}

static bool RaycastClosestCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, SMeshVertex const ** pVertices, unsigned int Indices[3], TRef< AMaterialInstance > & Material ) {
    AMeshComponent const * mesh = static_cast< AMeshComponent const * >( Self->Owner );

    if ( mesh->bUseDynamicRange ) {
        // Raycasting of meshes with dynamic range is not supported yet
        return false;
    }

    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * HitLocation;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    AIndexedMesh * resource = mesh->GetMesh();

    if ( !resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, HitLocation, HitUV, HitDistance, Indices, Material ) ) {
        return false;
    }

    *pVertices = resource->GetVertices();

    return true;
}

AMeshComponent::AMeshComponent() {
    bCastShadow = true;

    Primitive.QueryGroup |= VSD_QUERY_MASK_SHADOW_CAST;
    Primitive.RaycastCallback = RaycastCallback;
    Primitive.RaycastClosestCallback = RaycastClosestCallback;

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

    if ( bCastShadow )
    {
        Primitive.QueryGroup |= VSD_QUERY_MASK_SHADOW_CAST;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_NO_SHADOW_CAST;
    }
    else
    {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_SHADOW_CAST;
        Primitive.QueryGroup |= VSD_QUERY_MASK_NO_SHADOW_CAST;
    }

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
    UpdateWorldBounds();
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
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            Mesh->DrawBVH( InRenderer, GetWorldTransformMatrix() );
        }
    }

    if ( RVDrawMeshBounds )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            InRenderer->SetDepthTest( false );

            if ( IsSkinnedMesh() )
            {
                InRenderer->SetColor( AColor4( 0.5f,0.5f,1,1 ) );
            }
            else
            {
                InRenderer->SetColor( AColor4( 1,1,1,1 ) );
            }

            InRenderer->DrawAABB( WorldBounds );
        }
    }
}


static bool BrushRaycastCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits ) {
    ABrushComponent const * brush = static_cast< ABrushComponent const * >(Self->Owner);

#if 0
    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    AIndexedMesh * resource = mesh->GetMesh();

    return resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, Hits );
#endif

    AN_UNUSED( brush );

    GLogger.Printf( "BrushRaycastCallback: todo\n" );
    return false;
}

static bool BrushRaycastClosestCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, SMeshVertex const ** pVertices, unsigned int Indices[3], TRef< AMaterialInstance > & Material ) {
    ABrushComponent const * brush = static_cast< ABrushComponent const * >(Self->Owner);

#if 0
    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * HitLocation;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    AIndexedMesh * resource = mesh->GetMesh();

    if ( !resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, HitLocation, HitUV, HitDistance, Indices, Material ) ) {
        return false;
    }

    *pVertices = resource->GetVertices();

    return true;

#endif

    AN_UNUSED( brush );

    GLogger.Printf( "BrushRaycastClosestCallback: todo\n" );
    return false;
}

ABrushComponent::ABrushComponent() {
    Primitive.RaycastCallback = BrushRaycastCallback;
    Primitive.RaycastClosestCallback = BrushRaycastClosestCallback;
}

void ABrushComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawBrushBounds )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 1, 0.5f, 0.5f, 1 ) );
            InRenderer->DrawAABB( WorldBounds );
        }
    }
}
