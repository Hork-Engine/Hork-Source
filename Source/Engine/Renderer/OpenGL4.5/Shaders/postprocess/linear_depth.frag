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

#include "base/viewuniforms.glsl"

layout( location = 0 ) out float FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Depth;

void main() {

#if 1
    const float znear = GetViewportZNear();
    const float zfar = GetViewportZFar();
    
    vec2 tc = AdjustTexCoord( VS_TexCoord );
    
    float d;
    
    #if 0
        vec2 d1 = textureGather( Smp_Depth, tc, 0 ).xy;
        vec2 d2 = textureGatherOffset( Smp_Depth, tc, ivec2(0,1) ).xy;
        
        //vec2 d1;
        //d1.x = textureOffset( Smp_Depth, tc, ivec2(0,0) ).x;
        //d1.y = textureOffset( Smp_Depth, tc, ivec2(0,1) ).x;
        //vec2 d2;
        //d2.x = textureOffset( Smp_Depth, tc, ivec2(1,0) ).x;
        //d2.y = textureOffset( Smp_Depth, tc, ivec2(1,1) ).x;
        
        // get farthest depth
        d = max( max( d1.x, d1.y ), max( d2.x, d2.y ) );
    #else
        // get nearest depth
        d = texture( Smp_Depth, tc ).x;
    #endif
        
    float z = d * ( zfar - znear ) + znear;

    FS_FragColor = znear * zfar / z;

#else    // Universal but slow method
    vec2 tc = AdjustTexCoord( VS_TexCoord );
    vec2 ndc = VS_TexCoord;
    //ndc.y=1.0-ndc.y;
    ndc = ndc * 2 - 1.0;
    vec4 p = InverseProjectionMatrix * vec4( ndc, texture( Smp_Depth, tc ).x, 1.0 );
    FS_FragColor = p.z / p.w;
    // viewspacePos = p.xyz / p.w
#endif

//FS_FragColor=-FS_FragColor;
}

