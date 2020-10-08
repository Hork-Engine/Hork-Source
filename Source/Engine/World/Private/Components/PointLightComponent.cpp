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

static const float DEFAULT_RADIUS = 1.0f;
static const float MIN_RADIUS = 0.01f;

ARuntimeVariable dd_PointLights( _CTS( "dd_PointLights" ), _CTS( "0" ), VAR_CHEAT );

AN_CLASS_META( APointLightComponent )

APointLightComponent::APointLightComponent() {
    Radius = DEFAULT_RADIUS;
    InverseSquareRadius = 1.0f / ( Radius * Radius );

    UpdateWorldBounds();
}

void APointLightComponent::SetRadius( float _Radius ) {
    Radius = Math::Max( MIN_RADIUS, _Radius );
    InverseSquareRadius = 1.0f / ( Radius * Radius );

    UpdateWorldBounds();
}

void APointLightComponent::OnTransformDirty() {
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void APointLightComponent::UpdateWorldBounds() {
    SphereWorldBounds.Radius = Radius;
    SphereWorldBounds.Center = GetWorldPosition();
    AABBWorldBounds.Mins = SphereWorldBounds.Center - Radius;
    AABBWorldBounds.Maxs = SphereWorldBounds.Center + Radius;
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

    if ( dd_PointLights )
    {
        if ( Primitive.VisPass == InRenderer->GetVisPass() )
        {
            Float3 pos = GetWorldPosition();

            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 1, 1, 1, 1 ) );
            InRenderer->DrawSphere( pos, Radius );
        }
    }
}

void APointLightComponent::PackLight( Float4x4 const & InViewMatrix, SLightParameters & Light ) {
    Light.Position = Float3( InViewMatrix * GetWorldPosition() );
    Light.Radius = GetRadius();
    Light.CosHalfOuterConeAngle = 0;
    Light.CosHalfInnerConeAngle = 0;
    Light.InverseSquareRadius = InverseSquareRadius;
    Light.Direction = InViewMatrix.TransformAsFloat3x3( -GetWorldDirection() ); // Only for photometric light
    Light.SpotExponent = 0;
    Light.Color = GetEffectiveColor( -1.0f );
    Light.LightType = CLUSTER_LIGHT_POINT;
    Light.RenderMask = ~0u;//RenderMask; // TODO
    APhotometricProfile * profile = GetPhotometricProfile();
    Light.PhotometricProfile = profile ? profile->GetPhotometricProfileIndex() : 0xffffffff;
}
