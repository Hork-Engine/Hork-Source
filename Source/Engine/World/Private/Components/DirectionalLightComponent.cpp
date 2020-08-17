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

#include <World/Public/Components/DirectionalLightComponent.h>
#include <World/Public/World.h>
#include <World/Public/Base/DebugRenderer.h>

static const float DEFAULT_MAX_SHADOW_CASCADES = 4;
static const float DEFAULT_ILLUMINANCE_IN_LUX = 110000.0f;
static const float DEFAULT_TEMPERATURE = 6590.0f;
static const Float3 DEFAULT_COLOR( 1.0f );

ARuntimeVariable RVDrawDirectionalLights( _CTS( "DrawDirectionalLights" ), _CTS( "0" ), VAR_CHEAT );

AN_BEGIN_CLASS_META( ADirectionalLightComponent )
AN_ATTRIBUTE_( bCastShadow, AF_DEFAULT )
AN_END_CLASS_META()

ADirectionalLightComponent::ADirectionalLightComponent() {
    IlluminanceInLux = DEFAULT_ILLUMINANCE_IN_LUX;
    Temperature = DEFAULT_TEMPERATURE;
    Color = DEFAULT_COLOR;    
    EffectiveColor = Float4( 0.0f );
    MaxShadowCascades = DEFAULT_MAX_SHADOW_CASCADES;
    Next = Prev = nullptr;
}

void ADirectionalLightComponent::SetIlluminance( float _IlluminanceInLux ) {
    IlluminanceInLux = _IlluminanceInLux;
    bEffectiveColorDirty = true;
}

float ADirectionalLightComponent::GetIlluminance() const {
    return IlluminanceInLux;
}

void ADirectionalLightComponent::InitializeComponent() {
    Super::InitializeComponent();

    GetWorld()->GetRenderWorld().AddDirectionalLight( this );
}

void ADirectionalLightComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    GetWorld()->GetRenderWorld().RemoveDirectionalLight( this );
}

void ADirectionalLightComponent::SetDirection( Float3 const & _Direction ) {
    Float3x3 orientation;

    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis( orientation[0], orientation[1] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetRotation( rotation );

    //bShadowMatrixDirty = true;
}

Float3 ADirectionalLightComponent::GetDirection() const {
    return GetForwardVector();
}

void ADirectionalLightComponent::SetWorldDirection( Float3 const & _Direction ) {
    Float3x3 orientation;
    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis( orientation[0], orientation[1] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetWorldRotation( rotation );

    //bShadowMatrixDirty = true;
}

Float3 ADirectionalLightComponent::GetWorldDirection() const {
    return GetWorldForwardVector();
}

void ADirectionalLightComponent::SetMaxShadowCascades( int _MaxShadowCascades ) {
    MaxShadowCascades = Math::Clamp( _MaxShadowCascades, 1, MAX_SHADOW_CASCADES );
}

int ADirectionalLightComponent::GetMaxShadowCascades() const {
    return MaxShadowCascades;
}

void ADirectionalLightComponent::OnTransformDirty() {
    Super::OnTransformDirty();

//    bShadowMatrixDirty = true;
}

#if 0
Float4x4 const & ADirectionalLightComponent::GetShadowMatrix() const {
    if ( bShadowMatrixDirty ) {
        // Update shadow matrix

        #define DEFAULT_ZNEAR 0.04f//0.1f
        #define DEFAULT_ZFAR 10000.0f//800.0f//10000.0f//99999.0f

        Float3x3 BillboardMatrix = GetWorldRotation().ToMatrix();
        Float3x3 Basis = BillboardMatrix.Transposed();
        //Float3 Origin = -Basis[2] * 1000.0f;//Basis * ( -GetWorldPosition() );
        Float3 Origin = Basis * ( -GetWorldBackVector()*400.0f );

        LightViewMatrix[ 0 ] = Float4( Basis[ 0 ], 0.0f );
        LightViewMatrix[ 1 ] = Float4( Basis[ 1 ], 0.0f );
        LightViewMatrix[ 2 ] = Float4( Basis[ 2 ], 0.0f );
        LightViewMatrix[ 3 ] = Float4( Origin, 1.0f );

        //const float SHADOWMAP_SIZE = 4096;
        //Float4 OrthoRect = Float4( -1, 1, -1, 1 )*150.0f;//(SHADOWMAP_SIZE*0.5f/100.0f);
        Float2 OrthoMins = Float2( -1,-1 )*150.0f;
        Float2 OrthoMaxs = Float2(  1, 1 )*150.0f;
        Float4x4 ProjectionMatrix = Float4x4::OrthoCC( OrthoMins, OrthoMaxs, DEFAULT_ZNEAR, DEFAULT_ZFAR );
        //Float4x4 ProjectionMatrix = Math::PerspectiveProjectionMatrixCC( Math::_PI/1.1f,Math::_PI/1.1f,DEFAULT_ZNEAR, DEFAULT_ZFAR );


        //Float4x4 Test = Math::PerspectiveProjectionMatrix( OrthoRect.X, OrthoRect.Y, OrthoRect.Z, OrthoRect.W, DEFAULT_ZNEAR, DEFAULT_ZFAR );
        //Float4x4 Test = PerspectiveProjectionMatrixRevCC( Math::_PI/4,Math::_PI/4,DEFAULT_ZNEAR, DEFAULT_ZFAR );




        //Float4 v1 = Test * Float4(0,0,-DEFAULT_ZFAR,1.0f);
        //v1/=v1.W;
        //Float4 v2 = Test * Float4(0,0,-DEFAULT_ZNEAR,1.0f);
        //v2/=v2.W;

        //Float4x4 ProjectionMatrix = PerspectiveProjectionMatrixRevCC( Math::_PI/4.0f, Math::_PI/4.0f, /*SHADOWMAP_SIZE, SHADOWMAP_SIZE, */DEFAULT_ZNEAR, DEFAULT_ZFAR );
        ShadowMatrix = ProjectionMatrix * LightViewMatrix;

        bShadowMatrixDirty = false;
    }

    return ShadowMatrix;
}

const Float4x4 & ADirectionalLightComponent::GetLightViewMatrix() const {
    GetShadowMatrix(); // Update matrix

    return LightViewMatrix;
}
#endif


void ADirectionalLightComponent::SetColor( Float3 const & _Color ) {
    Color = _Color;
    bEffectiveColorDirty = true;
}

void ADirectionalLightComponent::SetColor( float _R, float _G, float _B ) {
    Color.X = _R;
    Color.Y = _G;
    Color.Z = _B;
    bEffectiveColorDirty = true;
}

Float3 const & ADirectionalLightComponent::GetColor() const {
    return Color;
}

void ADirectionalLightComponent::SetTemperature( float _Temperature ) {
    Temperature = _Temperature;
    bEffectiveColorDirty = true;
}

float ADirectionalLightComponent::GetTemperature() const {
    return Temperature;
}

Float4 const & ADirectionalLightComponent::GetEffectiveColor() const {
    if ( bEffectiveColorDirty ) {
        const float EnergyUnitScale = 1.0f / 100.0f / 100.0f;

        float energy = IlluminanceInLux * EnergyUnitScale * GetAnimationBrightness();

        AColor4 temperatureColor;
        temperatureColor.SetTemperature( GetTemperature() );

        *(Float3 *)&EffectiveColor = GetColor() * temperatureColor.GetRGB() * energy;

        bEffectiveColorDirty = false;
    }
    return EffectiveColor;
}

void ADirectionalLightComponent::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawDirectionalLights ) {
        Float3 pos = GetWorldPosition();
        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 1, 1, 1, 1 ) );
        InRenderer->DrawLine( pos, pos + GetWorldDirection() * 10.0f );
    }
}
