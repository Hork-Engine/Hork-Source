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
#include <Core/Public/Logger.h>

ARuntimeVariable RVDrawDirectionalLights( _CTS( "DrawDirectionalLights" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVShadowCascadeBits( _CTS( "ShadowCascadeBits" ), _CTS( "24" ) );    // Allowed 16, 24 or 32 bits
ARuntimeVariable RVCascadeSplitLambda( _CTS( "CascadeSplitLambda" ), _CTS( "1.0" ) );
//ARuntimeVariable RVShadowCascadeOffset( _CTS( "ShadowCascadeOffset" ), _CTS( "3.0" ) );
//ARuntimeVariable RVShadowMaxDistance( _CTS( "ShadowMaxDistance" ), _CTS( "128" ) );
//ARuntimeVariable RVShadowCascadeResolution( _CTS( "ShadowCascadeResolution" ), _CTS( "2048" ) ); // TODO: move to directional light properties

static constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f,-0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f
);

static const float DEFAULT_MAX_SHADOW_CASCADES = 4;
static const float DEFAULT_ILLUMINANCE_IN_LUX = 110000.0f;
static const float DEFAULT_TEMPERATURE = 6590.0f;
static const Float3 DEFAULT_COLOR( 1.0f );

AN_BEGIN_CLASS_META( ADirectionalLightComponent )
AN_ATTRIBUTE_( bCastShadow, AF_DEFAULT )
AN_END_CLASS_META()

ADirectionalLightComponent::ADirectionalLightComponent() {
    IlluminanceInLux = DEFAULT_ILLUMINANCE_IN_LUX;
    Temperature = DEFAULT_TEMPERATURE;
    Color = DEFAULT_COLOR;    
    EffectiveColor = Float4( 0.0f );
    bCastShadow = true;
    ShadowMaxDistance = 128;
    ShadowCascadeOffset = 3;
    MaxShadowCascades = DEFAULT_MAX_SHADOW_CASCADES;
    ShadowCascadeResolution = 1024;
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
    orientation[0] = Math::Cross( Float3(0.0f,1.0f,0.0f), orientation[2] ).Normalized();
    orientation[1] = Math::Cross( orientation[2], orientation[0] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetRotation( rotation );
}

Float3 ADirectionalLightComponent::GetDirection() const {
    return GetForwardVector();
}

void ADirectionalLightComponent::SetWorldDirection( Float3 const & _Direction ) {
    Float3x3 orientation;

    orientation[2] = -_Direction.Normalized();
    orientation[0] = Math::Cross( Float3( 0.0f, 1.0f, 0.0f ), orientation[2] ).Normalized();
    orientation[1] = Math::Cross( orientation[2], orientation[0] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetWorldRotation( rotation );
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
}

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

void ADirectionalLightComponent::AddShadowmapCascades( SRenderView * View, int * pFirstCascade, int * pNumCascades ) {
    float cascadeSplits[MAX_CASCADE_SPLITS];
    int numSplits = MaxShadowCascades + 1;
    int numVisibleSplits;
    Float4x4 lightViewMatrix;
    Float3 worldspaceVerts[MAX_CASCADE_SPLITS][4];
    Float3 right, up;

    AN_ASSERT( MaxShadowCascades > 0 && MaxShadowCascades <= MAX_SHADOW_CASCADES );

    if ( !bCastShadow ) {
        *pFirstCascade = 0;
        *pNumCascades = 0;
        return;
    }

    if ( View->bPerspective ) {
        float tanFovX = std::tan( View->ViewFovX * 0.5f );
        float tanFovY = std::tan( View->ViewFovY * 0.5f );
        right = View->ViewRightVec * tanFovX;
        up = View->ViewUpVec * tanFovY;
    } else {
        float orthoWidth = View->ViewOrthoMaxs.X - View->ViewOrthoMins.X;
        float orthoHeight = View->ViewOrthoMaxs.Y - View->ViewOrthoMins.Y;
        right = View->ViewRightVec * Math::Abs( orthoWidth * 0.5f );
        up = View->ViewUpVec * Math::Abs( orthoHeight * 0.5f );
    }

    const float shadowMaxDistance = ShadowMaxDistance;
    const float offset = ShadowCascadeOffset;
    const float a = (shadowMaxDistance - offset) / View->ViewZNear;
    const float b = (shadowMaxDistance - offset) - View->ViewZNear;
    const float lambda = RVCascadeSplitLambda.GetFloat();

    // Calc splits
    cascadeSplits[0] = View->ViewZNear;
    cascadeSplits[MAX_CASCADE_SPLITS-1] = shadowMaxDistance;

    for ( int splitIndex = 1 ; splitIndex < MAX_CASCADE_SPLITS-1 ; splitIndex++ ) {
        const float factor = (float)splitIndex / (MAX_CASCADE_SPLITS-1);
        const float logarithmic = View->ViewZNear * Math::Pow( a, factor );
        const float linear = View->ViewZNear + b * factor;
        const float dist = Math::Lerp( linear, logarithmic, lambda );
        cascadeSplits[splitIndex] = offset + dist;
    }

    float maxVisibleDist = Math::Max( View->MaxVisibleDistance, cascadeSplits[0] );

    // Calc worldspace verts
    for ( numVisibleSplits = 0 ;
          numVisibleSplits < numSplits && (cascadeSplits[Math::Max( 0, numVisibleSplits-1 )] <= maxVisibleDist) ;
          numVisibleSplits++ )
    {
        Float3 * pWorldSpaceVerts = worldspaceVerts[numVisibleSplits];

        float d = cascadeSplits[numVisibleSplits];

        // FIXME: variable distance can cause edge shimmering
        //d = d > maxVisibleDist ? maxVisibleDist : d;

        Float3 centerWorldspace = View->ViewPosition + View->ViewDir * d;

        Float3 c1 = right + up;
        Float3 c2 = right - up;

        if ( View->bPerspective ) {
            c1 *= d;
            c2 *= d;
        }

        pWorldSpaceVerts[0] = centerWorldspace - c1;
        pWorldSpaceVerts[1] = centerWorldspace - c2;
        pWorldSpaceVerts[2] = centerWorldspace + c1;
        pWorldSpaceVerts[3] = centerWorldspace + c2;
    }

    int numVisibleCascades = numVisibleSplits - 1;

    BvSphere cascadeSphere;

    Float3x3 basis = GetWorldRotation().ToMatrix().Transposed();
    lightViewMatrix[0] = Float4( basis[0], 0.0f );
    lightViewMatrix[1] = Float4( basis[1], 0.0f );
    lightViewMatrix[2] = Float4( basis[2], 0.0f );

    const float halfCascadeRes = ShadowCascadeResolution >> 1;
    const float oneOverHalfCascadeRes = 1.0f / halfCascadeRes;

    int firstCascade = View->NumShadowMapCascades;

    // Distance from cascade bounds to light source (near clip plane)
    // NOTE: We can calc actual light distance from scene geometry,
    // but now it just a magic number big enough to enclose most scenes = 1km.
    const float lightDistance = 1000.0f;

    for ( int i = 0 ; i < numVisibleCascades ; i++ ) {
        // Calc cascade bounding sphere
        cascadeSphere.FromPointsAverage( worldspaceVerts[i], 8 );

        // Set light position at cascade center
        lightViewMatrix[3] = Float4( basis * -cascadeSphere.Center, 1.0f );

        // Set ortho box
        Float3 cascadeMins = Float3( -cascadeSphere.Radius );
        Float3 cascadeMaxs = Float3( cascadeSphere.Radius );

        // Offset near clip distance
        cascadeMins[2] -= lightDistance;

        // Calc light view projection matrix
        Float4x4 cascadeMatrix = Float4x4::OrthoCC( Float2( cascadeMins ), Float2( cascadeMaxs ), cascadeMins[2], cascadeMaxs[2] )
                * lightViewMatrix;

#if 0
        // Calc pixel fraction in texture space
        Float2 error = Float2( cascadeMatrix[3] );  // same cascadeMatrix * Float4(0,0,0,1)
        error.X = Math::Fract( error.X * halfCascadeRes ) * oneOverHalfCascadeRes;
        error.Y = Math::Fract( error.Y * halfCascadeRes ) * oneOverHalfCascadeRes;

        // Snap light projection to texel grid
        // Same cascadeMatrix = Float4x4::Translation( -error ) * cascadeMatrix;
        cascadeMatrix[3].X -= error.X;
        cascadeMatrix[3].Y -= error.Y;
#else
        // Snap light projection to texel grid
        cascadeMatrix[3].X -= Math::Fract( cascadeMatrix[3].X * halfCascadeRes ) * oneOverHalfCascadeRes;
        cascadeMatrix[3].Y -= Math::Fract( cascadeMatrix[3].Y * halfCascadeRes ) * oneOverHalfCascadeRes;
#endif

        int cascadeIndex = firstCascade + i;

        View->LightViewProjectionMatrices[cascadeIndex] = cascadeMatrix;
        View->ShadowMapMatrices[cascadeIndex] = ShadowMapBias * cascadeMatrix * View->ClipSpaceToWorldSpace;
    }

    View->NumShadowMapCascades += numVisibleCascades;

    *pFirstCascade = firstCascade;
    *pNumCascades = numVisibleCascades;
}
