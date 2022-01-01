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

#include "base/viewuniforms.glsl"

layout( location = 0 ) out float FS_FragColor[8];

layout( binding = 0 ) uniform sampler2D Smp_LinearDepth;

layout( binding = 1, std140 ) uniform DrawCall
{
    vec2 UVOffset;
    vec2 InvFullResolution;
};

void main() {
    vec2 uv = floor( gl_FragCoord.xy ) * 4.0 + ( UVOffset + 0.5 );
    uv *= InvFullResolution;

    vec4 s0 = textureGather( Smp_LinearDepth, uv, 0 );
    vec4 s1 = textureGatherOffset( Smp_LinearDepth, uv, ivec2( 2, 0 ), 0 );

    FS_FragColor[0] = s0.w; // (0,0)
    FS_FragColor[1] = s0.z; // (1,0)
    FS_FragColor[2] = s1.w; // (2,0)
    FS_FragColor[3] = s1.z; // (3,0)
    FS_FragColor[4] = s0.x; // (0,1)
    FS_FragColor[5] = s0.y; // (1,1)
    FS_FragColor[6] = s1.x; // (2,1)
    FS_FragColor[7] = s1.y; // (3,1)
}

