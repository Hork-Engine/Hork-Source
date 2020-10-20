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
#include <Runtime/Public/RuntimeVariable.h>

ARuntimeVariable com_DrawMeshBounds( _CTS( "com_DrawMeshBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_DrawBrushBounds( _CTS( "com_DrawBrushBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_DrawIndexedMeshBVH( _CTS( "com_DrawIndexedMeshBVH" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( AMeshComponent )

static bool RaycastCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits, int & ClosestHit ) {
    AMeshComponent const * mesh = static_cast< AMeshComponent const * >( Self->Owner );

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

    int firstHit = Hits.Size();

    if ( mesh->bOverrideMeshMaterials ) {
        bool ret = false;

        float boxMin, boxMax;

        Float3 invRayDir;

        invRayDir.X = 1.0f / rayDirLocal.X;
        invRayDir.Y = 1.0f / rayDirLocal.Y;
        invRayDir.Z = 1.0f / rayDirLocal.Z;

        if ( !BvRayIntersectBox( rayStartLocal, invRayDir, resource->GetBoundingBox(), boxMin, boxMax ) || boxMin >= hitDistanceLocal ) {
            return false;
        }

        AIndexedMeshSubpartArray const & subparts = resource->GetSubparts();
        for ( int i = 0 ; i < subparts.Size() ; i++ ) {

            int first = Hits.Size();

            // Raycast subpart
            AIndexedMeshSubpart * subpart = subparts[i];
            ret |= subpart->Raycast( rayStartLocal, rayDirLocal, invRayDir, hitDistanceLocal, Hits );

            // Correct material
            int num = Hits.Size() - first;
            if ( num ) {
                AMaterialInstance * material = mesh->GetMaterialInstance( i );
                for ( int j = 0 ; j < num ; j++ ) {
                    Hits[ first + j ].Material = material;
                }
            }
        }

        if ( !ret ) {
            return false;
        }
    } else {
        if ( !resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, Hits ) ) {
            return false;
        }
    }

    // Convert hits to worldspace and find closest hit

    Float3x4 const & transform = mesh->GetWorldTransformMatrix();
    Float3x3 normalMatrix;

    transform.DecomposeNormalMatrix( normalMatrix );

    int numHits = Hits.Size() - firstHit;

    ClosestHit = firstHit;

    for ( int i = 0 ; i < numHits ; i++ ) {
        int hitNum = firstHit + i;
        STriangleHitResult & hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal = ( normalMatrix * hitResult.Normal ).Normalized();
        hitResult.Distance = (hitResult.Location - InRayStart).Length();

        if ( hitResult.Distance < Hits[ ClosestHit ].Distance ) {
            ClosestHit = hitNum;
        }
    }

    return true;
}

static bool RaycastClosestCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, SMeshVertex const ** pVertices, unsigned int Indices[3], TRef< AMaterialInstance > & Material ) {
    AMeshComponent const * mesh = static_cast< AMeshComponent const * >( Self->Owner );

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

    int subpartIndex;

    if ( !resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, HitLocation, HitUV, HitDistance, Indices, subpartIndex ) ) {
        return false;
    }

    Material = mesh->GetMaterialInstance( subpartIndex );

    *pVertices = resource->GetVertices();

    // Transform hit location to world space
    HitLocation = Self->Owner->GetWorldTransformMatrix() * HitLocation;

    // Recalc hit distance in world space
    HitDistance = (HitLocation - InRayStart).Length();

    return true;
}

AMeshComponent::AMeshComponent() {
    DrawableType = DRAWABLE_STATIC_MESH;

    Primitive.RaycastCallback = RaycastCallback;
    Primitive.RaycastClosestCallback = RaycastClosestCallback;

    bAllowRaycast = true;

    static TStaticResourceFinder< AIndexedMesh > MeshResource( _CTS( "/Default/Meshes/Box" ) );
    Mesh = MeshResource.GetObject();
    Bounds = Mesh->GetBoundingBox();

    RenderTransformMatrix.SetIdentity();
}

void AMeshComponent::InitializeComponent() {
    Super::InitializeComponent();
}

void AMeshComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    ClearMaterials();
}

void AMeshComponent::SetAllowRaycast( bool _AllowRaycast ) {
    if ( _AllowRaycast ) {
        Primitive.RaycastCallback = RaycastCallback;
        Primitive.RaycastClosestCallback = RaycastClosestCallback;
    } else {
        Primitive.RaycastCallback = nullptr;
        Primitive.RaycastClosestCallback = nullptr;
    }
    bAllowRaycast = _AllowRaycast;
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

    if ( com_DrawIndexedMeshBVH )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            Mesh->DrawBVH( InRenderer, GetWorldTransformMatrix() );
        }
    }

    if ( com_DrawMeshBounds )
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

#if 0
AN_CLASS_META( ABrushComponent )

static bool BrushRaycastCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits, int & ClosestHit ) {
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
#endif






AN_CLASS_META( AProceduralMeshComponent )

static bool RaycastCallback_Procedural( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits, int & ClosestHit ) {
    AProceduralMeshComponent const * mesh = static_cast< AProceduralMeshComponent const * >( Self->Owner );

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

    AProceduralMesh * resource = mesh->GetMesh();
    if ( !resource ) {
        // No resource associated with procedural mesh component
        return false;
    }

    int firstHit = Hits.Size();

    if ( !resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, Hits ) ) {
        return false;
    }

    // Convert hits to worldspace and find closest hit

    Float3x4 const & transform = mesh->GetWorldTransformMatrix();
    Float3x3 normalMatrix;

    transform.DecomposeNormalMatrix( normalMatrix );

    int numHits = Hits.Size() - firstHit;

    ClosestHit = firstHit;

    AMaterialInstance * material = mesh->GetMaterialInstance();

    for ( int i = 0 ; i < numHits ; i++ ) {
        int hitNum = firstHit + i;
        STriangleHitResult & hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal = ( normalMatrix * hitResult.Normal ).Normalized();
        hitResult.Distance = (hitResult.Location - InRayStart).Length();

        if ( hitResult.Distance < Hits[ ClosestHit ].Distance ) {
            ClosestHit = hitNum;
        }

        hitResult.Material = material;
    }

    return true;
}

static bool RaycastClosestCallback_Procedural( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, SMeshVertex const ** pVertices, unsigned int Indices[3], TRef< AMaterialInstance > & Material ) {
    AProceduralMeshComponent const * mesh = static_cast< AProceduralMeshComponent const * >( Self->Owner );

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

    AProceduralMesh * resource = mesh->GetMesh();
    if ( !resource ) {
        // No resource associated with procedural mesh component
        return false;
    }

    if ( !resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, HitLocation, HitUV, HitDistance, Indices ) ) {
        return false;
    }

    Material = mesh->GetMaterialInstance();

    *pVertices = resource->VertexCache.ToPtr();

    // Transform hit location to world space
    HitLocation = Self->Owner->GetWorldTransformMatrix() * HitLocation;

    // Recalc hit distance in world space
    HitDistance = (HitLocation - InRayStart).Length();

    return true;
}

AProceduralMeshComponent::AProceduralMeshComponent() {
    DrawableType = DRAWABLE_PROCEDURAL_MESH;

    Primitive.RaycastCallback = RaycastCallback_Procedural;
    Primitive.RaycastClosestCallback = RaycastClosestCallback_Procedural;

    bAllowRaycast = true;

    RenderTransformMatrix.SetIdentity();

    //LightmapOffset.Z = LightmapOffset.W = 1;

    //static TStaticResourceFinder< AIndexedMesh > MeshResource( _CTS( "/Default/Meshes/Box" ) );
    //Mesh = MeshResource.GetObject();
    //Bounds = Mesh->GetBoundingBox();
}

void AProceduralMeshComponent::InitializeComponent() {
    Super::InitializeComponent();
}

void AProceduralMeshComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    //ClearMaterials();
}

void AProceduralMeshComponent::SetAllowRaycast( bool _AllowRaycast ) {
    if ( _AllowRaycast ) {
        Primitive.RaycastCallback = RaycastCallback_Procedural;
        Primitive.RaycastClosestCallback = RaycastClosestCallback_Procedural;
    } else {
        Primitive.RaycastCallback = nullptr;
        Primitive.RaycastClosestCallback = nullptr;
    }
    bAllowRaycast = _AllowRaycast;
}

void AProceduralMeshComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( com_DrawMeshBounds )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 0.5f,1,0.5f,1 ) );
            InRenderer->DrawAABB( WorldBounds );
        }
    }
}

AMaterialInstance * AProceduralMeshComponent::GetMaterialInstance() const {
    AMaterialInstance * pInstance = MaterialInstance;
    if ( !pInstance ) {
        static TStaticResourceFinder< AMaterialInstance > DefaultInstance( _CTS( "/Default/MaterialInstance/Default" ) );
        pInstance = DefaultInstance.GetObject();
    }
    return pInstance;
}
