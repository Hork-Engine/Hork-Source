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

#ifndef COMMON_H
#define COMMON_H

#include "base/core.glsl"

vec4 TextureSampleClampRegion( sampler2D Image, vec2 TexCoord, vec2 Region )
{
    vec2 TexSize = vec2(textureSize( Image, 0 ));
    vec2 ClampedTexCoord = min( TexCoord * Region, Region - 1.0 ) / TexSize;
    ClampedTexCoord.y = 1.0 - ClampedTexCoord.y;
    return texture( Image, ClampedTexCoord );
}

float CalcLinearDepth_Perpective( float Depth, float NearZ, float FarZ ) {
    return NearZ * FarZ / ( Depth * ( FarZ - NearZ ) + NearZ );
}

float CalcLinearDepth_Ortho( float Depth, float NearZ, float FarZ ) {
    return -( Depth * ( FarZ - NearZ ) + NearZ );
}

// Gaussian blur approximation with linear sampling. 9-tap filter using five texture fetches instead of nine.
vec4 GaussianBlur9( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset1 = Direction * 1.3846153846;
    vec2 Offset2 = Direction * 3.2307692308;
    return texture( Image, TexCoord ) * 0.2270270270
       + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.3162162162
       + ( texture( Image, TexCoord + Offset2 ) + texture( Image, TexCoord - Offset2 ) ) * 0.0702702703;
}

// Gaussian blur approximation with linear sampling. 5-tap filter using three texture fetches instead of five.
vec4 GaussianBlur5( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset1 = Direction * 1.3333333333333333;
    return texture( Image, TexCoord ) * 0.29411764705882354
       + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.35294117647058826;
}

// Gaussian blur approximation with linear sampling. 13-tap filter using 7 vs 13 texture fetches.
vec4 GaussianBlur13( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset1 = Direction * 1.411764705882353;
    vec2 Offset2 = Direction * 3.2941176470588234;
    vec2 Offset3 = Direction * 5.176470588235294;
    return texture( Image, TexCoord ) * 0.1964825501511404
       + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.2969069646728344
       + ( texture( Image, TexCoord + Offset2 ) + texture( Image, TexCoord - Offset2 ) ) * 0.09447039785044732
       + ( texture( Image, TexCoord + Offset3 ) + texture( Image, TexCoord - Offset3 ) ) * 0.010381362401148057;
}

float GaussianBlur13R( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset1 = Direction * 1.411764705882353;
    vec2 Offset2 = Direction * 3.2941176470588234;
    vec2 Offset3 = Direction * 5.176470588235294;
    return texture( Image, TexCoord ).r * 0.1964825501511404
       + ( texture( Image, TexCoord + Offset1 ).r + texture( Image, TexCoord - Offset1 ).r ) * 0.2969069646728344
       + ( texture( Image, TexCoord + Offset2 ).r + texture( Image, TexCoord - Offset2 ).r ) * 0.09447039785044732
       + ( texture( Image, TexCoord + Offset3 ).r + texture( Image, TexCoord - Offset3 ).r ) * 0.010381362401148057;
}

vec2 GaussianBlur13RG( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset1 = Direction * 1.411764705882353;
    vec2 Offset2 = Direction * 3.2941176470588234;
    vec2 Offset3 = Direction * 5.176470588235294;
    return texture( Image, TexCoord ).rg * 0.1964825501511404
       + ( texture( Image, TexCoord + Offset1 ).rg + texture( Image, TexCoord - Offset1 ).rg ) * 0.2969069646728344
       + ( texture( Image, TexCoord + Offset2 ).rg + texture( Image, TexCoord - Offset2 ).rg ) * 0.09447039785044732
       + ( texture( Image, TexCoord + Offset3 ).rg + texture( Image, TexCoord - Offset3 ).rg ) * 0.010381362401148057;
}

vec4 MyBlur( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset0 = Direction * 0.5;
    vec2 Offset1 = Direction * 1.5;
    vec2 Offset2 = Direction * 2.5;
    vec2 Offset3 = Direction * 3.5;
    return ( texture( Image, TexCoord + Offset0 ) + texture( Image, TexCoord - Offset0 ) ) * 0.015
         + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.035
         + ( texture( Image, TexCoord + Offset2 ) + texture( Image, TexCoord - Offset2 ) ) * 0.05
         + ( texture( Image, TexCoord + Offset3 ) + texture( Image, TexCoord - Offset3 ) ) * 0.4;
}

float CalcDistanceAttenuationFrostbite( float DistanceSqr, float InverseSquareRadius ) {
    const float factor = DistanceSqr * InverseSquareRadius;
    const float smoothFactor = saturate( 1.0 - factor * factor );
    return smoothFactor * smoothFactor / max( DistanceSqr, 0.01*0.01 );
}

float CalcDistanceAttenuationSkyforge( float Distance, float InnerRadius, float OuterRadius ) {
    const float d = max( InnerRadius, Distance );
            
    return saturate( 1.0 - pow( d / OuterRadius, 4.0 ) ) / ( d*d + 1.0 );
}

float CalcDistanceAttenuationUnreal( float Distance, float OuterRadius ) {
    return pow( saturate( 1.0 - pow( Distance / OuterRadius, 4.0 ) ), 2.0 );// / (Distance*Distance + 1.0 );
}

float CalcDistanceAttenuation( float Distance, float OuterRadius ) {
    return pow( 1.0 - min( Distance / OuterRadius, 1.0 ), 2.2 );
}

float CalcSpotAttenuation( float LdotDir, float CosHalfInnerAngle, float CosHalfOuterAngle, float SpotExponent ) {
#if 0
    // TODO: precalc scale and offset on CPU-side
    const float Scale = 1.0 / max( CosHalfInnerAngle - CosHalfOuterAngle, 1e-4 );
    const float Offset = -CosHalfOuterAngle * Scale;
    
    const float Attenuation = saturate( LdotDir * Scale + Offset );
    return Attenuation * Attenuation;
#else
    return pow( smoothstep( CosHalfOuterAngle, CosHalfInnerAngle, LdotDir ), SpotExponent );
#endif
}

#endif // COMMON_H
