/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "TerrainComponent.h"
#include "World.h"

#include <Core/ConsoleVar.h>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "BulletCompatibility.h"

AConsoleVar com_DrawTerrainBounds( _CTS( "com_DrawTerrainBounds" ), _CTS( "0" ), CVAR_CHEAT );

AN_CLASS_META( ATerrainComponent )

static bool RaycastCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodVector< STriangleHitResult > & Hits )
{
    ATerrainComponent const * terrain = static_cast< ATerrainComponent const * >(Self->Owner);
    bool bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    ATerrain * resource = terrain->GetTerrain();
    if ( !resource ) {
        return false;
    }

    Float3x4 const & transformInverse = terrain->GetTerrainWorldTransformInversed();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    int firstHit = Hits.Size();

    if ( !resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hits ) ) {
        return false;
    }

    // Convert hits to worldspace and find closest hit

    Float3x4 const & transform = terrain->GetTerrainWorldTransform();

#if 0 // General case
    Float3x3 normalMatrix;
    transform.DecomposeNormalMatrix( normalMatrix );
#else
    Float3x3 normalMatrix = terrain->GetWorldRotation().ToMatrix3x3();
#endif

    int numHits = Hits.Size() - firstHit;
    for ( int i = 0 ; i < numHits ; i++ ) {
        int hitNum = firstHit + i;
        STriangleHitResult & hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal = (normalMatrix * hitResult.Normal).Normalized();

        // No need to recalc hit distance
        //hitResult.Distance = (hitResult.Location - InRayStart).Length();
    }

    return true;
}

static bool RaycastClosestCallback( SPrimitiveDef const * Self,
                                    Float3 const & InRayStart,
                                    Float3 const & InRayEnd,
                                    STriangleHitResult & Hit,
                                    SMeshVertex const ** pVertices )
{
    ATerrainComponent const * terrain = static_cast< ATerrainComponent const * >(Self->Owner);
    bool bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    ATerrain * resource = terrain->GetTerrain();
    if ( !resource ) {
        return false;
    }

    Float3x4 const & transformInverse = terrain->GetTerrainWorldTransformInversed();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    if ( !resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hit ) ) {
        return false;
    }

    if ( pVertices ) {
        *pVertices = nullptr;
    }

    // Transform hit location to world space
    Hit.Location = terrain->GetTerrainWorldTransform() * Hit.Location;

    // No need to recalc hit distance
    //Hit.Distance = (Hit.Location - InRayStart).Length();

#if 0 // General case
    Float3x3 normalMatrix;
    TerrainWorldTransform.DecomposeNormalMatrix( normalMatrix );
    Hit.Normal = (normalMatrix * Hit.Normal).Normalized();
#else
    Hit.Normal = terrain->GetWorldRotation().ToMatrix3x3() * Hit.Normal;
    Hit.Normal.NormalizeSelf();
#endif

    return true;
}

static void EvaluateRaycastResult( SPrimitiveDef * Self,
                                   ALevel const * LightingLevel,
                                   SMeshVertex const * pVertices,
                                   SMeshVertexUV const * pLightmapVerts,
                                   int LightmapBlock,
                                   unsigned int const * pIndices,
                                   Float3 const & HitLocation,
                                   Float2 const & HitUV,
                                   Float3 * Vertices,
                                   Float2 & TexCoord,
                                   Float3 & LightmapSample )
{
    ATerrainComponent * terrain = static_cast< ATerrainComponent * >( Self->Owner );

    STerrainTriangle triangle;

    terrain->GetTerrainTriangle( HitLocation, triangle );

    Vertices[0] = triangle.Vertices[0];
    Vertices[1] = triangle.Vertices[1];
    Vertices[2] = triangle.Vertices[2];
    TexCoord = triangle.Texcoord;
    LightmapSample = Float3( 0.0f );
}

ATerrainComponent::ATerrainComponent()
    : RigidBody( nullptr )
{
    HitProxy = CreateInstanceOf< AHitProxy >();

    Platform::ZeroMem( &Primitive, sizeof( Primitive ) );
    Primitive.Owner = this;
    Primitive.Type = VSD_PRIMITIVE_BOX;
    Primitive.VisGroup = VISIBILITY_GROUP_TERRAIN;
    Primitive.QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS/* | VSD_QUERY_MASK_SHADOW_CAST*/;
    Primitive.bIsOutdoor = true;
    Primitive.RaycastCallback = RaycastCallback;
    Primitive.RaycastClosestCallback = RaycastClosestCallback;
    Primitive.EvaluateRaycastResult = EvaluateRaycastResult;
    Primitive.Box.Clear();

    bAllowRaycast = true;

    TerrainWorldTransform.SetIdentity();
    TerrainWorldTransformInv.SetIdentity();
}

void ATerrainComponent::SetVisibilityGroup( int InVisibilityGroup )
{
    Primitive.VisGroup = InVisibilityGroup;
}

int ATerrainComponent::GetVisibilityGroup() const
{
    return Primitive.VisGroup;
}

void ATerrainComponent::SetVisible( bool _Visible )
{
    if ( _Visible ) {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    }
    else {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

bool ATerrainComponent::IsVisible() const
{
    return !!(Primitive.QueryGroup & VSD_QUERY_MASK_VISIBLE);
}

void ATerrainComponent::SetHiddenInLightPass( bool _HiddenInLightPass )
{
    if ( _HiddenInLightPass ) {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
    else {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
}

bool ATerrainComponent::IsHiddenInLightPass() const
{
    return !(Primitive.QueryGroup & VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS);
}

void ATerrainComponent::SetQueryGroup( int _UserQueryGroup )
{
    Primitive.QueryGroup |= _UserQueryGroup & 0xffff0000;
}

void ATerrainComponent::SetTwoSidedSurface( bool bTwoSidedSurface )
{
    if ( bTwoSidedSurface ) {
        Primitive.Flags |= SURF_TWOSIDED;
    }
    else {
        Primitive.Flags &= ~SURF_TWOSIDED;
    }
}

uint8_t ATerrainComponent::GetSurfaceFlags() const
{
    return Primitive.Flags;
}

void ATerrainComponent::AddTerrainPhysics()
{
    if ( IsInEditor() ) {
        // Do not add/remove physics for objects in editor
        return;
    }

    if ( !Terrain ) {
        // No terrain resource assigned to component
        return;
    }

    AN_ASSERT( RigidBody == nullptr );

    Float3 worldPosition = TerrainWorldTransform * Float3( 0.0f, (Terrain->GetMinHeight()+Terrain->GetMaxHeight())*0.5f, 0.0f );
    Float3x3 worldRotation = GetWorldRotation().ToMatrix3x3();

    btRigidBody::btRigidBodyConstructionInfo contructInfo( 0.0f, nullptr, Terrain->GetHeightfieldShape() );
    contructInfo.m_startWorldTransform.setOrigin( btVectorToFloat3( worldPosition ) );
    contructInfo.m_startWorldTransform.setBasis( btMatrixToFloat3x3( worldRotation.Transposed() ) );

    RigidBody = new btRigidBody( contructInfo );
    RigidBody->setCollisionFlags( btCollisionObject::CF_STATIC_OBJECT /*| btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT*/ );
    RigidBody->setUserPointer( HitProxy.GetObject() );

    HitProxy->Initialize( this, RigidBody );
}

void ATerrainComponent::RemoveTerrainPhysics()
{
    if ( IsInEditor() ) {
        // Do not add/remove physics for objects in editor
        return;
    }

    if ( RigidBody == nullptr ) {
        return;
    }

    HitProxy->Deinitialize();

    delete RigidBody;
    RigidBody = nullptr;
}

void ATerrainComponent::InitializeComponent()
{
    Super::InitializeComponent();

    UpdateTransform();

    AddTerrainPhysics();

    GetLevel()->AddPrimitive( &Primitive );
}

void ATerrainComponent::DeinitializeComponent()
{
    RemoveTerrainPhysics();

    GetLevel()->RemovePrimitive( &Primitive );

    Super::DeinitializeComponent();
}

void ATerrainComponent::SetTerrain( ATerrain * InTerrain )
{
    Terrain = InTerrain;

    if ( IsInitialized() ) {
        // Keep terrain physics in sync with terrain resource
        RemoveTerrainPhysics();
        AddTerrainPhysics();

        UpdateWorldBounds();
    }    
}

void ATerrainComponent::SetAllowRaycast( bool _AllowRaycast )
{
    if ( _AllowRaycast ) {
        Primitive.RaycastCallback = RaycastCallback;
        Primitive.RaycastClosestCallback = RaycastClosestCallback;
    }
    else {
        Primitive.RaycastCallback = nullptr;
        Primitive.RaycastClosestCallback = nullptr;
    }
    bAllowRaycast = _AllowRaycast;
}

bool ATerrainComponent::Raycast( Float3 const & InRayStart, Float3 const & InRayEnd, TPodVector< STriangleHitResult > & Hits ) const
{
    if ( !Primitive.RaycastCallback ) {
        return false;
    }

    Hits.Clear();

    return Primitive.RaycastCallback( &Primitive, InRayStart, InRayEnd, Hits );
}

bool ATerrainComponent::RaycastClosest( Float3 const & InRayStart, Float3 const & InRayEnd, STriangleHitResult & Hit ) const
{
    if ( !Primitive.RaycastCallback ) {
        return false;
    }

    return Primitive.RaycastClosestCallback( &Primitive, InRayStart, InRayEnd, Hit, nullptr );
}

void ATerrainComponent::UpdateTransform()
{
    Float3 worldPosition = GetWorldPosition();
    Float3x3 worldRotation = GetWorldRotation().ToMatrix3x3();

    // Terrain transform without scale
    TerrainWorldTransform.Compose( worldPosition, worldRotation );

    // Terrain inversed transform
    TerrainWorldTransformInv = TerrainWorldTransform.Inversed();

    UpdateWorldBounds();
}

void ATerrainComponent::UpdateWorldBounds()
{
    if ( !Terrain ) {
        return;
    }

    Primitive.Box = Terrain->GetBoundingBox().Transform( TerrainWorldTransform );

    // Terrain is always in outdoor area. So we don't need to update primitive
    //if ( IsInitialized() )
    //{
    //    GetLevel()->MarkPrimitive( &Primitive );
    //}
}

void ATerrainComponent::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateTransform();

    if ( !IsInEditor() ) {
        GLogger.Printf( "WARNING: Set transform for terrain %s\n", GetObjectNameCStr() );
    }

    // Update rigid body transform
    if ( Terrain && RigidBody ) {
        btTransform worldTransform;
        Float3 worldPosition = TerrainWorldTransform * Float3( 0.0f, (Terrain->GetMinHeight()+Terrain->GetMaxHeight())*0.5f, 0.0f );
        Float3x3 worldRotation = GetWorldRotation().ToMatrix3x3();

        worldTransform.setOrigin( btVectorToFloat3( worldPosition ) );
        worldTransform.setBasis( btMatrixToFloat3x3( worldRotation.Transposed() ) );
        RigidBody->setWorldTransform( worldTransform );
    }
}

void ATerrainComponent::GetLocalXZ( Float3 const & InPosition, float & X, float & Z ) const
{
    // position in terrain space
    Float3 localPosition = TerrainWorldTransformInv * InPosition;

    X = localPosition.X;
    Z = localPosition.Z;
}

bool ATerrainComponent::GetTerrainTriangle( Float3 const & InPosition, STerrainTriangle & Triangle ) const
{
    if ( !Terrain ) {
        return false;
    }

    // position in terrain space
    Float3 localPosition = TerrainWorldTransformInv * InPosition;

    if ( !Terrain->GetTerrainTriangle( localPosition.X, localPosition.Z, Triangle ) ) {
        return false;
    }

    // Convert triangle to world space
    Triangle.Vertices[0] = TerrainWorldTransform * Triangle.Vertices[0];
    Triangle.Vertices[1] = TerrainWorldTransform * Triangle.Vertices[1];
    Triangle.Vertices[2] = TerrainWorldTransform * Triangle.Vertices[2];

#if 0 // General case
    Float3x3 normalMatrix;
    TerrainWorldTransform.DecomposeNormalMatrix( normalMatrix );
    Triangle.Normal = (normalMatrix * Triangle.Normal).Normalized();
#else
    Triangle.Normal = (GetWorldRotation().ToMatrix3x3() * Triangle.Normal).Normalized();
#endif

    return true;
}

void ATerrainComponent::SetCollisionGroup( int _CollisionGroup )
{
    HitProxy->SetCollisionGroup( _CollisionGroup );
}

void ATerrainComponent::SetCollisionMask( int _CollisionMask )
{
    HitProxy->SetCollisionMask( _CollisionMask );
}

void ATerrainComponent::SetCollisionFilter( int _CollisionGroup, int _CollisionMask )
{
    HitProxy->SetCollisionFilter( _CollisionGroup, _CollisionMask );
}

void ATerrainComponent::AddCollisionIgnoreActor( AActor * _Actor )
{
    HitProxy->AddCollisionIgnoreActor( _Actor );
}

void ATerrainComponent::RemoveCollisionIgnoreActor( AActor * _Actor )
{
    HitProxy->RemoveCollisionIgnoreActor( _Actor );
}

void ATerrainComponent::DrawDebug( ADebugRenderer * InRenderer )
{
    Super::DrawDebug( InRenderer );

    if ( com_DrawTerrainBounds && Terrain )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( Color4( 1, 0, 0, 1 ) );
            InRenderer->DrawAABB( Primitive.Box );
        }
    }
}
