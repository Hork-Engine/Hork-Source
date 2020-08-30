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

#include "base/common.frag"
#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_OutlineMask;
layout( binding = 1 ) uniform sampler2D Smp_OutlineBlur;

#if 0
float Sobel( vec2 UV ) {
    float c00 = textureOffset( Smp_OutlineMask, UV, ivec2(-1,-1) ).r;
    float c01 = textureOffset( Smp_OutlineMask, UV, ivec2(-1, 0) ).r;
    float c02 = textureOffset( Smp_OutlineMask, UV, ivec2(-1, 1) ).r;
    float c10 = textureOffset( Smp_OutlineMask, UV, ivec2( 0,-1) ).r;
    float c12 = textureOffset( Smp_OutlineMask, UV, ivec2( 0, 1) ).r;    
    float c20 = textureOffset( Smp_OutlineMask, UV, ivec2( 1,-1) ).r;
    float c21 = textureOffset( Smp_OutlineMask, UV, ivec2( 1, 0) ).r;
    float c22 = textureOffset( Smp_OutlineMask, UV, ivec2( 1, 1) ).r;

    vec2 v = vec2( 2.0 * ( c01 - c21 ) + ( c02 - c20 ), 2.0 * ( c10 - c12 ) + ( c20 - c02 ) ) + ( c00 - c22 );

    return length( v );
}
#endif
 
void main() {
    // TODO: Move to uniforms?
    const vec3 OutlineColor = vec3( 1.0, 0.456, 0.1 );
    const float Hardness = 4.0;
    
    vec2 FinalBlur = GaussianBlur13RG( Smp_OutlineBlur, VS_TexCoord, vec2( 0.0, InvViewportSize.y ) );
    
    vec2 outline = saturate( FinalBlur - texture( Smp_OutlineMask, VS_TexCoord ).rg );
    
    // Additive blending
    //FS_FragColor = vec4( OutlineColor * saturate( (outline.x + outline.y) * Hardness ), 1.0 );
    
    // Alpha blending
    FS_FragColor = vec4( OutlineColor, saturate( (outline.x + outline.y) * Hardness ) );
    
#if 0
    FS_FragColor = vec4( OutlineColor, saturate( Sobel(VS_TexCoord) ) );
#endif
}
