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

#ifndef TEXTURE_H
#define TEXTURE_H

vec4 texture_srgb_alpha( in sampler1D sampler, in float texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in sampler1DArray sampler, in vec2 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in sampler2D sampler, in vec2 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in sampler2DArray sampler, in vec3 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in sampler3D sampler, in vec3 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in samplerCube sampler, in vec3 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_srgb_alpha( in sampler2DRect sampler, in vec2 texCoord )
{
  vec4 color = texture( sampler, texCoord );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in sampler1D sampler, in float texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in sampler1DArray sampler, in vec2 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in sampler2D sampler, in vec2 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in sampler2DArray sampler, in vec3 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in sampler3D sampler, in vec3 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in samplerCube sampler, in vec3 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_ycocg( in sampler2DRect sampler, in vec2 texCoord )
{
  vec4 ycocg = texture( sampler, texCoord );
  ycocg.z = ( ycocg.z * 31.875 ) + 1.0;
  ycocg.z = 1.0 / ycocg.z;
  ycocg.xy *= ycocg.z;
  vec4 color = vec4( dot( ycocg, vec4( 1.0, -1.0, 0.0, 1.0 ) ),
                     dot( ycocg, vec4( 0.0, 1.0, -0.50196078, 1.0 ) ),
                     dot( ycocg, vec4( -1.0, -1.0, 1.00392156, 1.0 ) ),
                     1.0 );
#ifdef SRGB_GAMMA_APPROX
  return pow( color, vec4( 2.2, 2.2, 2.2, 1.0 ) );
#else
  const vec4 Shift = vec4( 0.055, 0.055, 0.055, 0.0 );
  const vec4 Scale = vec4( 1.0 / 1.055, 1.0 / 1.055, 1.0 / 1.055, 1.0 );
  const vec4 Pow = vec4( 2.4, 2.4, 2.4, 1.0 );
  const vec4 Scale2 = vec4( 1.0 / 12.92, 1.0 / 12.92, 1.0 / 12.92, 1.0 );
  return mix( pow( ( color + Shift ) * Scale, Pow ), color * Scale2, step( color, vec4(0.04045) ) );
#endif
}
vec4 texture_grayscaled( in sampler1D sampler, in float texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in sampler1DArray sampler, in vec2 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in sampler2D sampler, in vec2 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in sampler2DArray sampler, in vec3 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in sampler3D sampler, in vec3 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in samplerCube sampler, in vec3 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in samplerCubeArray sampler, in vec4 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec4 texture_grayscaled( in sampler2DRect sampler, in vec2 texCoord )
{
  return vec4( texture( sampler, texCoord ).r );
}
vec3 texture_nm_xyz( in sampler1D sampler, in float texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in sampler1DArray sampler, in vec2 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in sampler2D sampler, in vec2 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in sampler2DArray sampler, in vec3 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in sampler3D sampler, in vec3 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in samplerCube sampler, in vec3 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in samplerCubeArray sampler, in vec4 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xyz( in sampler2DRect sampler, in vec2 texCoord )
{
  return texture( sampler, texCoord ).xyz * 2.0 - 1.0;
}
vec3 texture_nm_xy( in sampler1D sampler, in float texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in sampler1DArray sampler, in vec2 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in sampler2D sampler, in vec2 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in sampler2DArray sampler, in vec3 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in sampler3D sampler, in vec3 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in samplerCube sampler, in vec3 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_xy( in sampler2DRect sampler, in vec2 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).xyz * 2.0 - 1.0;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_spheremap( in sampler1D sampler, in float texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in sampler1DArray sampler, in vec2 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in sampler2D sampler, in vec2 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in sampler2DArray sampler, in vec3 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in sampler3D sampler, in vec3 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in samplerCube sampler, in vec3 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_spheremap( in sampler2DRect sampler, in vec2 texCoord )
{
  vec2 fenc = texture( sampler, texCoord ).xy * 4.0 - 2.0;
  float f = dot( fenc, fenc );
  vec3 decodedN;
  decodedN.xy = fenc * sqrt( 1.0 - f / 4.0 );
  decodedN.z = 1.0 - f / 2.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in sampler1D sampler, in float texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in sampler1DArray sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in sampler2D sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in sampler2DArray sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in sampler3D sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in samplerCube sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_stereographic( in sampler2DRect sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  float denom = 2.0 / ( 1 + clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 ) );
  decodedN.xy *= denom;
  decodedN.z = denom - 1.0;
  return decodedN;
}
vec3 texture_nm_paraboloid( in sampler1D sampler, in float texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in sampler1DArray sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in sampler2D sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in sampler2DArray sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in sampler3D sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in samplerCube sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_paraboloid( in sampler2DRect sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = 1.0 - clamp( dot( decodedN.xy, decodedN.xy ), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in sampler1D sampler, in float texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in sampler1DArray sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in sampler2D sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in sampler2DArray sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in sampler3D sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in samplerCube sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_quartic( in sampler2DRect sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy * 2.0 - 1.0;
  decodedN.z = clamp( (1.0 - decodedN.x * decodedN.x) * (1.0 - decodedN.y * decodedN.y), 0.0, 1.0 );
  return decodedN;
}
vec3 texture_nm_float( in sampler1D sampler, in float texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in sampler1DArray sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in sampler2D sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in sampler2DArray sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in sampler3D sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in samplerCube sampler, in vec3 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_float( in sampler2DRect sampler, in vec2 texCoord )
{
  vec3 decodedN;
  decodedN.xy = texture( sampler, texCoord ).xy;
  decodedN.z = sqrt( 1.0 - dot( decodedN.xy, decodedN.xy ) );
  return decodedN;
}
vec3 texture_nm_dxt5( in sampler1D sampler, in float texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in sampler1DArray sampler, in vec2 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in sampler2D sampler, in vec2 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in sampler2DArray sampler, in vec3 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in sampler3D sampler, in vec3 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in samplerCube sampler, in vec3 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in samplerCubeArray sampler, in vec4 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}
vec3 texture_nm_dxt5( in sampler2DRect sampler, in vec2 texCoord )
{
  vec3 decodedN = texture( sampler, texCoord ).wyz - 0.5;
  decodedN.z = sqrt( abs( dot( decodedN.xy, decodedN.xy ) - 0.25 ) );
  decodedN = normalize( decodedN );
  return decodedN;
}

#endif // TEXTURE_h
