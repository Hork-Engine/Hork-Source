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

#include "base/viewuniforms.glsl"

out gl_PerVertex
{
    vec4 gl_Position;
};

layout( location = 0 ) noperspective out vec2 VS_TexCoord;

void main() {
/*
    uint idx = gl_VertexID % 3; // allows rendering multiple fullscreen triangles
    vec4 pos =  vec4(
        (float( idx     &1U)) * 4.0 - 1.0,
        (float((idx>>1U)&1U)) * 4.0 - 1.0,
        0, 1.0);
    gl_Position = pos;
    VS_TexCoord = pos.xy * 0.5 + 0.5;*/
  
    gl_Position = vec4( InPosition, 0.0, 1.0 );
    VS_TexCoord = InPosition * 0.5 + 0.5;
}
