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

#include <World/Public/Components/PointLightComponent.h>
#include <World/Public/World.h>
#include <World/Public/Base/DebugRenderer.h>

constexpr float DEFAULT_INNER_RADIUS = 0.5f;
constexpr float DEFAULT_OUTER_RADIUS = 1.0f;

ARuntimeVariable RVDrawPointLights( _CTS( "DrawPointLights" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( APointLightComponent )

APointLightComponent::APointLightComponent() {
    InnerRadius = DEFAULT_INNER_RADIUS;
    OuterRadius = DEFAULT_OUTER_RADIUS;

    Primitive.Owner = this;
    Primitive.Type = VSD_PRIMITIVE_SPHERE;
    Primitive.VisGroup = VISIBILITY_GROUP_DEFAULT;
    Primitive.QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;

    UpdateWorldBounds();
}

void APointLightComponent::InitializeComponent() {
    Super::InitializeComponent();

    GetLevel()->AddPrimitive( &Primitive );
}

void APointLightComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    GetLevel()->RemovePrimitive( &Primitive );
}

void APointLightComponent::SetVisibilityGroup( int InVisibilityGroup ) {
    Primitive.VisGroup = InVisibilityGroup;
}

int APointLightComponent::GetVisibilityGroup() const {
    return Primitive.VisGroup;
}

void APointLightComponent::SetEnabled( bool _Enabled ) {
    Super::SetEnabled( _Enabled );

    if ( _Enabled ) {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    } else {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

void APointLightComponent::SetMovable( bool _Movable ) {
    if ( Primitive.bMovable == _Movable ) {
        return;
    }

    Primitive.bMovable = _Movable;

    if ( IsInitialized() )
    {
        GetLevel()->MarkPrimitive( &Primitive );
    }
}

bool APointLightComponent::IsMovable() const {
    return Primitive.bMovable;
}

void APointLightComponent::SetInnerRadius( float _Radius ) {
    InnerRadius = Math::Max( 0.001f, _Radius );
}

void APointLightComponent::SetOuterRadius( float _Radius ) {
    OuterRadius = Math::Max( 0.001f, _Radius );

    UpdateWorldBounds();
}

void APointLightComponent::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void APointLightComponent::UpdateWorldBounds() {
    SphereWorldBounds.Radius = OuterRadius;
    SphereWorldBounds.Center = GetWorldPosition();
    AABBWorldBounds.Mins = SphereWorldBounds.Center - OuterRadius;
    AABBWorldBounds.Maxs = SphereWorldBounds.Center + OuterRadius;
    OBBWorldBounds.Center = SphereWorldBounds.Center;
    OBBWorldBounds.HalfSize = Float3( SphereWorldBounds.Radius );
    OBBWorldBounds.Orient.SetIdentity();

    // TODO: Optimize?
    Float4x4 OBBTransform = Float4x4::Translation( OBBWorldBounds.Center ) * Float4x4::Scale( OBBWorldBounds.HalfSize );
    OBBTransformInverse = OBBTransform.Inversed();

    Primitive.Sphere = SphereWorldBounds;

    if ( IsInitialized() )
    {
        GetLevel()->MarkPrimitive( &Primitive );
    }
}

void APointLightComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawPointLights )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            Float3 pos = GetWorldPosition();

            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 0.5f, 0.5f, 0.5f, 1 ) );
            InRenderer->DrawSphere( pos, InnerRadius );
            InRenderer->SetColor( AColor4( 1, 1, 1, 1 ) );
            InRenderer->DrawSphere( pos, OuterRadius );
        }
    }
}

void APointLightComponent::PackLight( Float4x4 const & InViewMatrix, SClusterLight & Light ) {
    Light.Position = Float3( InViewMatrix * GetWorldPosition() );
    Light.OuterRadius = OuterRadius;
    Light.InnerRadius = Math::Min( InnerRadius, OuterRadius );
    Light.Color = GetEffectiveColor( -1.0f );
    Light.RenderMask = ~0u;//RenderMask;
    Light.LightType = CLUSTER_LIGHT_POINT;
}
