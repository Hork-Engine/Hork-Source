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


#if defined( DEPTH_WITH_VELOCITY_MAP )

layout( location = 0 ) out vec2 FS_Velocity;

layout( location = DEPTH_PASS_VARYING_VERTEX_POSITION_CURRENT ) in vec4 VS_VertexPos;
layout( location = DEPTH_PASS_VARYING_VERTEX_POSITION_PREVIOUS ) in vec4 VS_VertexPosP;

#endif

#include "$DEPTH_PASS_FRAGMENT_SAMPLERS$"
#include "$DEPTH_PASS_FRAGMENT_INPUT_VARYINGS$"

void main() {
    #include "$DEPTH_PASS_FRAGMENT_CODE$"

    #if defined( DEPTH_WITH_VELOCITY_MAP )
    vec2 p1 = VS_VertexPos.xy / VS_VertexPos.w;
    vec2 p2 = VS_VertexPosP.xy / VS_VertexPosP.w;
    FS_Velocity = ( p1 - p2 ) * (vec2(0.5,-0.5) * MOTION_BLUR_SCALE);
    FS_Velocity = saturate( sqrt( abs(FS_Velocity) ) * sign( FS_Velocity ) * 0.5 + 0.5 ) * ( 254.0 / 255.0 );
    #endif
}

