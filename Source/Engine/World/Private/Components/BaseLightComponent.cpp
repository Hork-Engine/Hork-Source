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

static const float DEFAULT_TEMPERATURE = 6590.0f;
static const float DEFAULT_LUMENS = 3000.0f;
static const Float3 DEFAULT_COLOR( 1.0f );

AN_CLASS_META( ALightComponent )

ALightComponent::ALightComponent() {
    Color = DEFAULT_COLOR;
    Temperature = DEFAULT_TEMPERATURE;
    bEffectiveColorDirty = true;
    bCastShadow = false;
    bEnabled = true;
    AnimationBrightness = 1;
}

void ALightComponent::SetEnabled( bool _Enabled ) {
    bEnabled = _Enabled;
}

void ALightComponent::SetAnimation( const char * _Pattern, float _Speed, float _Quantizer ) {
    AAnimationPattern * anim = NewObject< AAnimationPattern >();
    anim->Pattern = _Pattern;
    anim->Speed = _Speed;
    anim->Quantizer = _Quantizer;
    SetAnimation( anim );
}

void ALightComponent::SetAnimation( AAnimationPattern * _Animation ) {
    if ( IsSame( Animation, _Animation ) ) {
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

void ALightComponent::SetColor( Float3 const & _Color ) {
    Color = _Color;
    bEffectiveColorDirty = true;
}

void ALightComponent::SetColor( float _R, float _G, float _B ) {
    Color.X = _R;
    Color.Y = _G;
    Color.Z = _B;
    bEffectiveColorDirty = true;
}

Float3 const & ALightComponent::GetColor() const {
    return Color;
}

void ALightComponent::SetTemperature( float _Temperature ) {
    Temperature = _Temperature;
    bEffectiveColorDirty = true;
}

float ALightComponent::GetTemperature() const {
    return Temperature;
}

void ALightComponent::TickComponent( float _TimeStep ) {
    if ( !bEnabled ) {
        return;
    }

    // FIXME: Update light animation only if light is visible?

    AnimationBrightness = Animation->Calculate( AnimTime );
    AnimTime += _TimeStep;
    bEffectiveColorDirty = true;
}

AN_CLASS_META( APunctualLightComponent )

APunctualLightComponent::APunctualLightComponent() {
    Lumens = DEFAULT_LUMENS;
    EffectiveColor = Float4( 0.0f );
}

Float4 const & APunctualLightComponent::GetEffectiveColor( float CosHalfConeAngle ) const {
    if ( bEffectiveColorDirty ) {
        //const float EnergyUnitScale = 100.0f * 100.0f / 16.0f;
        const float EnergyUnitScale = 1.0f / 16.0f;
        //const float CandelasToEnergy = EnergyUnitScale;
        const float LumensToEnergy = EnergyUnitScale / Math::_2PI / (1.0f - CosHalfConeAngle);

        float energy = Lumens * LumensToEnergy;
        energy *= GetAnimationBrightness();

        AColor4 temperatureColor;
        temperatureColor.SetTemperature( GetTemperature() );

        *(Float3 *)&EffectiveColor = GetColor() * temperatureColor.GetRGB() * energy;

        bEffectiveColorDirty = false;
    }
    return EffectiveColor;
}

void APunctualLightComponent::SetLumens( float _Lumens ) {
    Lumens = Math::Max( 0.0f, _Lumens );
    bEffectiveColorDirty = true;
}

float APunctualLightComponent::GetLumens() const {
    return Lumens;
}

void APunctualLightComponent::PackLight( Float4x4 const & InViewMatrix, SClusterLight & Light ) {

}
