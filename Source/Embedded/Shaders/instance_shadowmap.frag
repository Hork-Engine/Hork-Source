/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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


#include "$SHADOWMAP_PASS_FRAGMENT_SAMPLERS$"
#include "$SHADOWMAP_PASS_FRAGMENT_INPUT_VARYINGS$"

#if defined SHADOWMAP_EVSM

layout( location = 0 ) out vec4 FS_FragColor;

void main() {

    #include "$SHADOWMAP_PASS_FRAGMENT_CODE$"

    const float EVSM_positiveExponent = 40.0;
    const float EVSM_negativeExponent = 5.0;

    float Depth = 2.0 * gl_FragCoord.z - 1.0;
    vec2 WarpDepth = vec2( exp( EVSM_positiveExponent * Depth ), -exp( -EVSM_negativeExponent * Depth ) );
    FS_FragColor = vec4( WarpDepth, WarpDepth * WarpDepth );
}

#elif defined SHADOWMAP_VSM

//layout( location = 0 ) in vec4 GS_Position;

void main() {

    #include "$SHADOWMAP_PASS_FRAGMENT_CODE$"

    float depth = gl_FragCoord.z;
    //float depth = GS_Position.z / GS_Position.w;

    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx( depth );
    float dy = dFdy( depth );

    FS_FragColor = vec4( depth, depth * depth + 0.25*( dx*dx + dy*dy ), 0.0, 0.0 );
}

#else

void main() {
    #include "$SHADOWMAP_PASS_FRAGMENT_CODE$"
}

#endif // SHADOWMAP_VSM/EVSM
