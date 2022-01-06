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

#include "SpotLightComponent.h"
#include "MeshComponent.h"
#include "World.h"
#include "DebugRenderer.h"
#include "RuntimeVariable.h"

static const float DEFAULT_RADIUS = 15;//1.0f;
static const float DEFAULT_INNER_CONE_ANGLE = 100.0f;
static const float DEFAULT_OUTER_CONE_ANGLE = 120.0f;
static const float DEFAULT_SPOT_EXPONENT = 1.0f;
static const float MIN_CONE_ANGLE = 1.0f;
static const float MIN_RADIUS = 0.01f;

ARuntimeVariable com_DrawSpotLights( _CTS( "com_DrawSpotLights" ), _CTS( "0" ), VAR_CHEAT );

AN_BEGIN_CLASS_META( ASpotLightComponent )
AN_ATTRIBUTE( Radius, float, SetRadius, GetRadius, AF_DEFAULT )
AN_ATTRIBUTE( InnerConeAngle, float, SetInnerConeAngle, GetInnerConeAngle, AF_DEFAULT )
AN_ATTRIBUTE( OuterConeAngle, float, SetOuterConeAngle, GetOuterConeAngle, AF_DEFAULT )
AN_ATTRIBUTE( SpotExponent, float, SetSpotExponent, GetSpotExponent, AF_DEFAULT )
AN_END_CLASS_META()

ASpotLightComponent::ASpotLightComponent() {
    Radius = DEFAULT_RADIUS;
    InverseSquareRadius = 1.0f / ( Radius * Radius );
    InnerConeAngle = DEFAULT_INNER_CONE_ANGLE;
    OuterConeAngle = DEFAULT_OUTER_CONE_ANGLE;
    CosHalfInnerConeAngle = Math::Cos( Math::Radians( InnerConeAngle * 0.5f ) );
    CosHalfOuterConeAngle = Math::Cos( Math::Radians( OuterConeAngle * 0.5f ) );
    SpotExponent = DEFAULT_SPOT_EXPONENT;

    UpdateWorldBounds();
}

void ASpotLightComponent::OnCreateAvatar() {
    Super::OnCreateAvatar();

    // TODO: Create mesh or sprite for avatar
    static TStaticResourceFinder< AIndexedMesh > Mesh( _CTS( "/Default/Meshes/Cone" ) );
    static TStaticResourceFinder< AMaterialInstance > MaterialInstance( _CTS( "AvatarMaterialInstance" ) );
    AMeshComponent * meshComponent = GetOwnerActor()->CreateComponent< AMeshComponent >( "SpotLightAvatar" );
    meshComponent->SetMotionBehavior( MB_KINEMATIC );
    meshComponent->SetCollisionGroup( CM_NOCOLLISION );
    meshComponent->SetMesh( Mesh.GetObject() );
    meshComponent->SetMaterialInstance( MaterialInstance.GetObject() );
    meshComponent->SetCastShadow( false );
    meshComponent->SetAbsoluteScale( true );
    meshComponent->SetAngles( 90, 0, 0 );
    meshComponent->SetScale( 0.1f );
    meshComponent->AttachTo( this );
    meshComponent->SetHideInEditor( true );
}

void ASpotLightComponent::SetRadius( float _Radius ) {
    Radius = Math::Max( MIN_RADIUS, _Radius );
    InverseSquareRadius = 1.0f / ( Radius * Radius );

    UpdateWorldBounds();
}

float ASpotLightComponent::GetRadius() const {
    return Radius;
}

void ASpotLightComponent::SetInnerConeAngle( float _Angle ) {
    InnerConeAngle = Math::Clamp( _Angle, MIN_CONE_ANGLE, 180.0f );
    CosHalfInnerConeAngle = Math::Cos( Math::Radians( InnerConeAngle * 0.5f ) );
}

float ASpotLightComponent::GetInnerConeAngle() const {
    return InnerConeAngle;
}

void ASpotLightComponent::SetOuterConeAngle( float _Angle ) {
    OuterConeAngle = Math::Clamp( _Angle, MIN_CONE_ANGLE, 180.0f );
    CosHalfOuterConeAngle = Math::Cos( Math::Radians( OuterConeAngle * 0.5f ) );

    UpdateWorldBounds();
}

float ASpotLightComponent::GetOuterConeAngle() const {
    return OuterConeAngle;
}

void ASpotLightComponent::SetSpotExponent( float _Exponent ) {
    SpotExponent = _Exponent;
}

float ASpotLightComponent::GetSpotExponent() const {
    return SpotExponent;
}

void ASpotLightComponent::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void ASpotLightComponent::UpdateWorldBounds() {
    const float ToHalfAngleRadians = 0.5f / 180.0f * Math::_PI;
    const float HalfConeAngle = OuterConeAngle * ToHalfAngleRadians;
    const Float3 WorldPos = GetWorldPosition();
    const float SinHalfConeAngle = Math::Sin( HalfConeAngle );

    // Compute cone OBB for voxelization
    OBBWorldBounds.Orient = GetWorldRotation().ToMatrix3x3();

    const Float3 SpotDir = -OBBWorldBounds.Orient[ 2 ];

    //OBBWorldBounds.HalfSize.X = OBBWorldBounds.HalfSize.Y = tan( HalfConeAngle ) * Radius;
    OBBWorldBounds.HalfSize.X = OBBWorldBounds.HalfSize.Y = SinHalfConeAngle * Radius;
    OBBWorldBounds.HalfSize.Z = Radius * 0.5f;
    OBBWorldBounds.Center = WorldPos + SpotDir * ( OBBWorldBounds.HalfSize.Z );

    // TODO: Optimize?
    Float4x4 OBBTransform = Float4x4::Translation( OBBWorldBounds.Center ) * Float4x4( OBBWorldBounds.Orient ) * Float4x4::Scale( OBBWorldBounds.HalfSize );
    OBBTransformInverse = OBBTransform.Inversed();

    // Compute cone AABB for culling
    AABBWorldBounds.Clear();
    AABBWorldBounds.AddPoint( WorldPos );
    Float3 v = WorldPos + SpotDir * Radius;
    Float3 vx = OBBWorldBounds.Orient[ 0 ] * OBBWorldBounds.HalfSize.X;
    Float3 vy = OBBWorldBounds.Orient[ 1 ] * OBBWorldBounds.HalfSize.X;
    AABBWorldBounds.AddPoint( v + vx );
    AABBWorldBounds.AddPoint( v - vx );
    AABBWorldBounds.AddPoint( v + vy );
    AABBWorldBounds.AddPoint( v - vy );

    // Посмотреть, как более эффективно распределяется площадь - у сферы или AABB
    // Compute cone Sphere bounds
    if ( HalfConeAngle > Math::_PI / 4 ) {
        SphereWorldBounds.Radius = SinHalfConeAngle * Radius;
        SphereWorldBounds.Center = WorldPos + SpotDir * ( CosHalfOuterConeAngle * Radius );
    } else {
        SphereWorldBounds.Radius = Radius / ( 2.0 * CosHalfOuterConeAngle );
        SphereWorldBounds.Center = WorldPos + SpotDir * SphereWorldBounds.Radius;
    }

    Primitive.Sphere = SphereWorldBounds;

    if ( IsInitialized() )
    {
        GetLevel()->MarkPrimitive( &Primitive );
    }
}

void ASpotLightComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( com_DrawSpotLights )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            Float3 pos = GetWorldPosition();
            Float3x3 orient = GetWorldRotation().ToMatrix3x3();
            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( Color4( 0.5f, 0.5f, 0.5f, 1 ) );
            InRenderer->DrawCone( pos, orient, Radius, Math::Radians( InnerConeAngle ) * 0.5f );
            InRenderer->SetColor( Color4( 1, 1, 1, 1 ) );
            InRenderer->DrawCone( pos, orient, Radius, Math::Radians( OuterConeAngle ) * 0.5f );
        }
    }
}

void ASpotLightComponent::PackLight( Float4x4 const & InViewMatrix, SLightParameters & Light ) {
    Light.Position = Float3( InViewMatrix * GetWorldPosition() );
    Light.Radius = GetRadius();
    Light.CosHalfOuterConeAngle = CosHalfOuterConeAngle;
    Light.CosHalfInnerConeAngle = CosHalfInnerConeAngle;
    Light.InverseSquareRadius = InverseSquareRadius;
    Light.Direction = InViewMatrix.TransformAsFloat3x3( -GetWorldDirection() );
    Light.SpotExponent = SpotExponent;
    Light.Color = GetEffectiveColor( Math::Min( CosHalfOuterConeAngle, 0.9999f ) );
    Light.LightType = CLUSTER_LIGHT_SPOT;
    Light.RenderMask = ~0u;//RenderMask; // TODO
    APhotometricProfile * profile = GetPhotometricProfile();
    Light.PhotometricProfile = profile ? profile->GetPhotometricProfileIndex() : 0xffffffff;
}
