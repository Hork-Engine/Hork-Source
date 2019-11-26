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
#ifdef FUTURE
    SphereWorldBounds.Radius = OuterRadius;
    SphereWorldBounds.Center = Float3(0);
    AABBWorldBounds.Mins = SphereWorldBounds.Center - OuterRadius;
    AABBWorldBounds.Maxs = SphereWorldBounds.Center + OuterRadius;
#endif
    UpdateBoundingBox();
}

void APointLightComponent::InitializeComponent() {
    Super::InitializeComponent();

    GetWorld()->GetRenderWorld().AddPointLight( this );
}

void APointLightComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    GetWorld()->GetRenderWorld().RemovePointLight( this );
}

void APointLightComponent::SetInnerRadius( float _Radius ) {
    InnerRadius = Math::Max( 0.001f, _Radius );
}

float APointLightComponent::GetInnerRadius() const {
    return InnerRadius;
}

void APointLightComponent::SetOuterRadius( float _Radius ) {
    OuterRadius = Math::Max( 0.001f, _Radius );

    UpdateBoundingBox();
}

float APointLightComponent::GetOuterRadius() const {
    return OuterRadius;
}

BvAxisAlignedBox const & APointLightComponent::GetWorldBounds() const {
    return AABBWorldBounds;
}

void APointLightComponent::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdateBoundingBox();
    //MarkAreaDirty();
}

void APointLightComponent::UpdateBoundingBox() {
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
}

void APointLightComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawPointLights ) {
        Float3 pos = GetWorldPosition();

        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 0.5f, 0.5f, 0.5f, 1 ) );
        InRenderer->DrawSphere( pos, InnerRadius );
        InRenderer->SetColor( AColor4( 1, 1, 1, 1 ) );
        InRenderer->DrawSphere( pos, OuterRadius );
    }
}
