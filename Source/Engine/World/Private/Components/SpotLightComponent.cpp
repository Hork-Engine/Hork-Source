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

#include <World/Public/Components/SpotLightComponent.h>
#include <World/Public/World.h>
#include <World/Public/Base/DebugRenderer.h>

static const float DEFAULT_INNER_RADIUS = 0.5f;
static const float DEFAULT_OUTER_RADIUS = 1.0f;
static const float DEFAULT_INNER_CONE_ANGLE = 30.0f;
static const float DEFAULT_OUTER_CONE_ANGLE = 35.0f;
static const float DEFAULT_SPOT_EXPONENT = 1.0f;
static const float MIN_CONE_ANGLE = 1.0f;

ARuntimeVariable RVDrawSpotLights( _CTS( "DrawSpotLights" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( ASpotLightComponent )

ASpotLightComponent::ASpotLightComponent() {
    InnerRadius = DEFAULT_INNER_RADIUS;
    OuterRadius = DEFAULT_OUTER_RADIUS;
    InnerConeAngle = DEFAULT_INNER_CONE_ANGLE;
    OuterConeAngle = DEFAULT_OUTER_CONE_ANGLE;
    CosHalfInnerConeAngle = Math::Cos( Math::Radians( InnerConeAngle * 0.5f ) );
    CosHalfOuterConeAngle = Math::Cos( Math::Radians( OuterConeAngle * 0.5f ) );
    SpotExponent = DEFAULT_SPOT_EXPONENT;

    Primitive.Owner = this;
    Primitive.Type = VSD_PRIMITIVE_SPHERE;
    Primitive.VisGroup = VISIBILITY_GROUP_DEFAULT;
    Primitive.QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;

    UpdateWorldBounds();
}

void ASpotLightComponent::InitializeComponent() {
    Super::InitializeComponent();

    GetLevel()->AddPrimitive( &Primitive );
}

void ASpotLightComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    GetLevel()->RemovePrimitive( &Primitive );
}

void ASpotLightComponent::SetVisibilityGroup( int InVisibilityGroup ) {
    Primitive.VisGroup = InVisibilityGroup;
}

int ASpotLightComponent::GetVisibilityGroup() const {
    return Primitive.VisGroup;
}

void ASpotLightComponent::SetEnabled( bool _Enabled ) {
    Super::SetEnabled( _Enabled );

    if ( _Enabled ) {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    } else {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

void ASpotLightComponent::SetMovable( bool _Movable ) {
    if ( Primitive.bMovable == _Movable ) {
        return;
    }

    Primitive.bMovable = _Movable;

    if ( IsInitialized() )
    {
        GetLevel()->MarkPrimitive( &Primitive );
    }
}

bool ASpotLightComponent::IsMovable() const {
    return Primitive.bMovable;
}

void ASpotLightComponent::SetInnerRadius( float _Radius ) {
    InnerRadius = Math::Max( MIN_CONE_ANGLE, _Radius );
}

float ASpotLightComponent::GetInnerRadius() const {
    return InnerRadius;
}

void ASpotLightComponent::SetOuterRadius( float _Radius ) {
    OuterRadius = Math::Max( MIN_CONE_ANGLE, _Radius );

    UpdateWorldBounds();
}

float ASpotLightComponent::GetOuterRadius() const {
    return OuterRadius;
}

void ASpotLightComponent::SetInnerConeAngle( float _Angle ) {
    InnerConeAngle = Math::Clamp( _Angle, 0.0001f, 180.0f );
    CosHalfInnerConeAngle = Math::Cos( Math::Radians( InnerConeAngle * 0.5f ) );
}

float ASpotLightComponent::GetInnerConeAngle() const {
    return InnerConeAngle;
}

void ASpotLightComponent::SetOuterConeAngle( float _Angle ) {
    OuterConeAngle = Math::Clamp( _Angle, 0.0001f, 180.0f );
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

void ASpotLightComponent::SetDirection( Float3 const & _Direction ) {
    Float3x3 orientation;

    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis( orientation[0], orientation[1] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetRotation( rotation );
}

Float3 ASpotLightComponent::GetDirection() const {
    return GetForwardVector();
}

void ASpotLightComponent::SetWorldDirection( Float3 const & _Direction ) {
    Float3x3 orientation;
    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis( orientation[0], orientation[1] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetWorldRotation( rotation );
}

Float3 ASpotLightComponent::GetWorldDirection() const {
    return GetWorldForwardVector();
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
    OBBWorldBounds.Orient = GetWorldRotation().ToMatrix();

    const Float3 SpotDir = -OBBWorldBounds.Orient[ 2 ];

    //OBBWorldBounds.HalfSize.X = OBBWorldBounds.HalfSize.Y = tan( HalfConeAngle ) * OuterRadius;
    OBBWorldBounds.HalfSize.X = OBBWorldBounds.HalfSize.Y = SinHalfConeAngle * OuterRadius;
    OBBWorldBounds.HalfSize.Z = OuterRadius * 0.5f;
    OBBWorldBounds.Center = WorldPos + SpotDir * ( OBBWorldBounds.HalfSize.Z );

    // TODO: Optimize?
    Float4x4 OBBTransform = Float4x4::Translation( OBBWorldBounds.Center ) * Float4x4( OBBWorldBounds.Orient ) * Float4x4::Scale( OBBWorldBounds.HalfSize );
    OBBTransformInverse = OBBTransform.Inversed();

    // Compute cone AABB for culling
    AABBWorldBounds.Clear();
    AABBWorldBounds.AddPoint( WorldPos );
    Float3 v = WorldPos + SpotDir * OuterRadius;
    Float3 vx = OBBWorldBounds.Orient[ 0 ] * OBBWorldBounds.HalfSize.X;
    Float3 vy = OBBWorldBounds.Orient[ 1 ] * OBBWorldBounds.HalfSize.X;
    AABBWorldBounds.AddPoint( v + vx );
    AABBWorldBounds.AddPoint( v - vx );
    AABBWorldBounds.AddPoint( v + vy );
    AABBWorldBounds.AddPoint( v - vy );

    // Посмотреть, как более эффективно распределяется площадь - у сферы или AABB
    // Compute cone Sphere bounds
    if ( HalfConeAngle > Math::_PI / 4 ) {
        SphereWorldBounds.Radius = SinHalfConeAngle * OuterRadius;
        SphereWorldBounds.Center = WorldPos + SpotDir * ( CosHalfOuterConeAngle * OuterRadius );
    } else {
        SphereWorldBounds.Radius = OuterRadius / ( 2.0 * CosHalfOuterConeAngle );
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

    if ( RVDrawSpotLights )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            Float3 pos = GetWorldPosition();
            Float3x3 orient = GetWorldRotation().ToMatrix();
            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 0.5f, 0.5f, 0.5f, 1 ) );
            InRenderer->DrawCone( pos, orient, OuterRadius, InnerConeAngle * 0.5f );
            InRenderer->SetColor( AColor4( 1, 1, 1, 1 ) );
            InRenderer->DrawCone( pos, orient, OuterRadius, OuterConeAngle * 0.5f );
        }
    }
}

void ASpotLightComponent::PackLight( Float4x4 const & InViewMatrix, SClusterLight & Light ) {
    Light.Position = Float3( InViewMatrix * GetWorldPosition() );
    Light.OuterRadius = GetOuterRadius();
    Light.InnerRadius = Math::Min( InnerRadius, OuterRadius );
    Light.Color = GetEffectiveColor( Math::Min( CosHalfOuterConeAngle, 0.9999f ) );
    Light.RenderMask = ~0u;//RenderMask;
    Light.LightType = CLUSTER_LIGHT_SPOT;
    Light.OuterConeAngle = CosHalfOuterConeAngle;
    Light.InnerConeAngle = CosHalfInnerConeAngle;
    Light.SpotDirection = InViewMatrix.TransformAsFloat3x3( -GetWorldDirection() );
    Light.SpotExponent = SpotExponent;
}
