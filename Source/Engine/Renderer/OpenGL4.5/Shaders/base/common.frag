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

#ifndef COMMON_H
#define COMMON_H

vec4 TextureSampleClampRegion( sampler2D tex, vec2 texCoord, vec2 region )
{
    vec2 texSize = vec2(textureSize( tex, 0 ));
    vec2 tc = min( texCoord * region, region - 1.0 ) / texSize;
    tc.y = 1.0 - tc.y;
    return texture( tex, tc );
}

float ComputeLinearDepth( in float _Depth, in float _NearZ, in float _FarZ ) {
    return _NearZ * _FarZ / ( _Depth * ( _FarZ - _NearZ ) + _NearZ );
}

vec3 UnpackPositionFromLinearDepth( in sampler2D _LinearDepth, in vec2 _Coord, in vec3 _Dir ) {
    return _Dir * texture( _LinearDepth, _Coord ).x;
}

#if 0
vec3 UnpackPositionFromDepth( in sampler2D _Depth, in vec2 _Coord, in vec3 _Dir ) {
#if 1 // Оптимизированный вариант, работает только для перспективной матрицы проекции
    float zNear = InvViewportSize.z;
    float zFar = InvViewportSize.w;

    //float LinearDepth = zFar * zNear / ( texture( _Depth, _Coord ).x * ( zFar - zNear ) - zFar );
    float LinearDepth = zNear * zFar / ( texture( _Depth, _Coord ).x * ( zFar - zNear ) + zNear );

    return _Dir * LinearDepth;
#else // Универсальный вариант, подходит также для ортогональной матрицы проекции
    vec2 tc = _Coord;
    tc.y = 1.0 - tc.y;

    //vec4 p = InverseProjectionMatrix * vec4( tc * 2.0 - 1.0, texture( _Depth, _Coord ).x * 2.0 - 1.0, 1.0 );
    vec4 p = InverseProjectionMatrix * vec4( tc * 2.0 - 1.0, texture( _Depth, _Coord ).x, 1.0 );
    return p.xyz / p.w;
#endif
}

vec3 UnpackPositionFromDepth( in sampler2D _Depth, in vec2 _Coord ) {
    vec2 tc = _Coord;
    tc.y = 1.0 - tc.y;

    //vec4 p = InverseProjectionMatrix * vec4( tc * 2.0 - 1.0, texture( _Depth, _Coord ).x * 2.0 - 1.0, 1.0 );
    vec4 p = InverseProjectionMatrix * vec4( tc * 2.0 - 1.0, texture( _Depth, _Coord ).x, 1.0 );
    return p.xyz / p.w;
}
#endif

// Gaussian blur approximation with linear sampling. 9-tap filter using five texture fetches instead of nine.
vec4 GaussianBlur9( in sampler2D Image, in vec2 TexCoord, in vec2 Direction ) {
    vec2 Offset1 = Direction * 1.3846153846;
    vec2 Offset2 = Direction * 3.2307692308;
    return texture( Image, TexCoord ) * 0.2270270270
       + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.3162162162
       + ( texture( Image, TexCoord + Offset2 ) + texture( Image, TexCoord - Offset2 ) ) * 0.0702702703;
}

// Gaussian blur approximation with linear sampling. 5-tap filter using three texture fetches instead of five.
vec4 GaussianBlur5( in sampler2D Image, in vec2 TexCoord, in vec2 Direction ) {
    vec2 Offset1 = Direction * 1.3333333333333333;
    return texture( Image, TexCoord ) * 0.29411764705882354
       + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.35294117647058826;
}

// Gaussian blur approximation with linear sampling. 13-tap filter using 7 vs 13 texture fetches.
vec4 GaussianBlur13( in sampler2D Image, in vec2 TexCoord, in vec2 Direction ) {
    vec2 Offset1 = Direction * 1.411764705882353;
    vec2 Offset2 = Direction * 3.2941176470588234;
    vec2 Offset3 = Direction * 5.176470588235294;
    return texture( Image, TexCoord ) * 0.1964825501511404
       + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.2969069646728344
       + ( texture( Image, TexCoord + Offset2 ) + texture( Image, TexCoord - Offset2 ) ) * 0.09447039785044732
       + ( texture( Image, TexCoord + Offset3 ) + texture( Image, TexCoord - Offset3 ) ) * 0.010381362401148057;
}

vec4 MyBlur( in sampler2D Image, in vec2 TexCoord, in vec2 Direction ) {
    vec2 Offset0 = Direction * 0.5;
    vec2 Offset1 = Direction * 1.5;
    vec2 Offset2 = Direction * 2.5;
    vec2 Offset3 = Direction * 3.5;
    return ( texture( Image, TexCoord + Offset0 ) + texture( Image, TexCoord - Offset0 ) ) * 0.015
         + ( texture( Image, TexCoord + Offset1 ) + texture( Image, TexCoord - Offset1 ) ) * 0.035
         + ( texture( Image, TexCoord + Offset2 ) + texture( Image, TexCoord - Offset2 ) ) * 0.05
         + ( texture( Image, TexCoord + Offset3 ) + texture( Image, TexCoord - Offset3 ) ) * 0.4;
}

#endif // COMMON_H
