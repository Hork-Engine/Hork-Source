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

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Source;
layout( binding = 1 ) uniform sampler2D Smp_LinearDepth;

layout( binding = 1, std140 ) uniform DrawCall
{
    vec2 InvSize;
};

const float KERNEL_RADIUS = 3;
const float BLUR_FALLOFF = 2.0 / ( KERNEL_RADIUS * KERNEL_RADIUS );
const float SHARPNESS = 40;

float Blur( vec2 uv, float r, float depth, inout float weight )
{
#ifdef INTERLEAVED_DEPTH
    vec2 aoz = texture( Smp_Source, uv ).xy;
    float c = aoz.x;
    float d = aoz.y;
#else
    float c = texture( Smp_Source, uv ).x;
    float d = texture( Smp_LinearDepth, uv ).x;
#endif
    float depthDiff = ( d - depth ) * SHARPNESS;
    float w = exp2( -r*r*BLUR_FALLOFF - depthDiff*depthDiff );
    weight += w;
    return c * w;
}

void BilateralBlur( vec2 tc ) {
#ifdef INTERLEAVED_DEPTH
    vec2 aoz = texture( Smp_Source, tc ).xy;
    float color = aoz.x;
    float depth = aoz.y;
#else
    float color = texture( Smp_Source, tc ).x;
    float depth = texture( Smp_LinearDepth, tc ).x;
#endif

    float weight = 1.0;

    for ( float r = 1 ; r <= KERNEL_RADIUS ; ++r ) {
        vec2 uv = tc + InvSize * r;
        color += Blur( uv, r, depth, weight );
    }

    for ( float r = 1 ; r <= KERNEL_RADIUS ; ++r ) {
        vec2 uv = tc - InvSize * r;
        color += Blur( uv, r, depth, weight );
    }

    FS_FragColor = vec4(color / weight);
}

void main() {
    BilateralBlur( VS_TexCoord );
}
