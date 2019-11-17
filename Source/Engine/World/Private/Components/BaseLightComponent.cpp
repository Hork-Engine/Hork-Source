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

#include <Engine/World/Public/Components/PointLightComponent.h>
#include <Engine/World/Public/World.h>
#include <Engine/Base/Public/DebugDraw.h>
#include "TemperatureToColor.h"

constexpr float DEFAULT_AMBIENT_INTENSITY   = 1.0f;
constexpr float DEFAULT_TEMPERATURE         = 6590.0f;
constexpr float DEFAULT_LUMENS              = 3000.0f;
constexpr Float3 DEFAULT_COLOR(1.0f);

AN_CLASS_META( ABaseLightComponent )

ABaseLightComponent::ABaseLightComponent() {
    Color = DEFAULT_COLOR;
    Temperature = DEFAULT_TEMPERATURE;
    Lumens = DEFAULT_LUMENS;
    EffectiveColor = Float4(0.0f,0.0f,0.0f,DEFAULT_AMBIENT_INTENSITY);
    bEffectiveColorDirty = true;
    RenderingGroup = RENDERING_GROUP_DEFAULT;
    bCastShadow = false;
    bEnabled = true;
    AnimationBrightness = 1;
}

void ABaseLightComponent::SetEnabled( bool _Enabled ) {
    bEnabled = _Enabled;
}

void ABaseLightComponent::SetAnimation( const char * _Pattern, float _Speed, float _Quantizer ) {
    AAnimationPattern * anim = NewObject< AAnimationPattern >();
    anim->Pattern = _Pattern;
    anim->Speed = _Speed;
    anim->Quantizer = _Quantizer;
    SetAnimation( anim );
}

void ABaseLightComponent::SetAnimation( AAnimationPattern * _Animation ) {
    if ( Animation == _Animation ) {
        return;
    }

    Animation = _Animation;
    AnimTime = 0;
    AnimationBrightness = 1;

    if ( Animation ) {
        AnimationBrightness = Animation->Calculate( 0.0f );
    }

    bCanEverTick = ( _Animation != nullptr );

    bEffectiveColorDirty = true;
}

void ABaseLightComponent::SetColor( Float3 const & _Color ) {
    Color = _Color;
    bEffectiveColorDirty = true;
}

void ABaseLightComponent::SetColor( float _R, float _G, float _B ) {
    Color.X = _R;
    Color.Y = _G;
    Color.Z = _B;
    bEffectiveColorDirty = true;
}

Float3 const & ABaseLightComponent::GetColor() const {
    return Color;
}

Float4 const & ABaseLightComponent::GetEffectiveColor() const {
    if ( bEffectiveColorDirty ) {
        float energy = STemperatureToColor::GetLightEnergyFromLumens( Lumens );
        energy *= AnimationBrightness;
        *(Float3 *)&EffectiveColor = Color * STemperatureToColor::GetRGBFromTemperature( Temperature ) * energy;
        bEffectiveColorDirty = false;
    }
    return EffectiveColor;
}

void ABaseLightComponent::SetTemperature( float _Temperature ) {
    Temperature = Math::Clamp( _Temperature, STemperatureToColor::MIN_TEMPERATURE, STemperatureToColor::MAX_TEMPERATURE );
    bEffectiveColorDirty = true;
}

float ABaseLightComponent::GetTemperature() const {
    return Temperature;
}

void ABaseLightComponent::SetLumens( float _Lumens ) {
    Lumens = Math::Max( 0.0f, _Lumens );
    bEffectiveColorDirty = true;
}

float ABaseLightComponent::GetLumens() const {
    return Lumens;
}

void ABaseLightComponent::SetAmbientIntensity( float _Intensity ) {
    EffectiveColor.W = Math::Max( 0.1f, _Intensity );
}

float ABaseLightComponent::GetAmbientIntensity() const {
    return EffectiveColor.W;
}

void ABaseLightComponent::TickComponent( float _TimeStep ) {
    if ( !bEnabled ) {
        return;
    }

    AnimationBrightness = Animation->Calculate( AnimTime );
    AnimTime += _TimeStep;
    bEffectiveColorDirty = true;
}
