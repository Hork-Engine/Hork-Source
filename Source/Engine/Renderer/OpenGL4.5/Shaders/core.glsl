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

#ifndef CORE_H
#define CORE_H

#define PI 3.1415926

#define saturate( val ) clamp( val, 0.0, 1.0 )
#define saturate2( val ) clamp( val, vec2(0.0), vec2(1.0) )
#define saturate3( val ) clamp( val, vec3(0.0), vec3(1.0) )
#define saturate4( val ) clamp( val, vec4(0.0), vec4(1.0) )

vec2 builtin_spheremap_coord( in vec3 dir ) {
  vec2 uv = vec2( atan( dir.z, dir.x ), asin( dir.y ) );
  return uv * vec2(0.1591, 0.3183) + 0.5;
}

float builtin_luminance( in vec3 color ) {
  return dot( color, vec3( 0.2126, 0.7152, 0.0722 ) );
}

float builtin_luminance( in vec4 color ) {
  return dot( color, vec4( 0.2126, 0.7152, 0.0722, 0.0 ) );
}

float builtin_saturate( in float color ) {
  return clamp( color, float(0.0), float(1.0) );
}

vec2 builtin_saturate( in vec2 color ) {
  return clamp( color, vec2(0.0), vec2(1.0) );
}

vec3 builtin_saturate( in vec3 color ) {
  return clamp( color, vec3(0.0), vec3(1.0) );
}

vec4 builtin_saturate( in vec4 color ) {
  return clamp( color, vec4(0.0), vec4(1.0) );
}

float log10( in float x )
{
    const float base10 = 1.0 / log(10.0);
    return log( x ) * base10;
}

vec2 log10( in vec2 x )
{
    const float base10 = 1.0 / log(10.0);
    return log( x ) * base10;
}

vec3 log10( in vec3 x )
{
    const float base10 = 1.0 / log(10.0);
    return log( x ) * base10;
}

vec4 log10( in vec4 x )
{
    const float base10 = 1.0 / log(10.0);
    return log( x ) * base10;
}

float rand( vec2 co ) {
    return fract( sin( dot( co, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

float rand( vec4 co ) {
    return fract( sin( dot( co, vec4( 12.9898, 78.233, 45.164, 94.673 ) ) ) * 43758.5453 );
}

vec3 sepia( in vec3 color ) {
	return dot( color, vec3( 0.2126, 0.7152, 0.0722 ) ) * vec3(1.0,0.89,0.71);
}

vec3 grayscale( vec3 color, float scale ) {
	return mix( color, vec3( dot( color, vec3( 0.2126, 0.7152, 0.0722 ) ) ), scale );
}

#endif // CORE_H
