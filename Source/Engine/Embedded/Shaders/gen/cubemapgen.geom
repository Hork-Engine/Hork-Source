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

// TODO: NV passthrough

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout( location = 0 ) out vec3 GS_Normal;
layout( location = 0 ) in vec3 VS_Normal[];
layout( location = 1 ) in flat int VS_InstanceID[];

void main() {
    gl_Layer = VS_InstanceID[0];
    
    gl_Position = gl_in[ 0 ].gl_Position;
    GS_Normal = VS_Normal[ 0 ];
    EmitVertex();
    
    gl_Position = gl_in[ 1 ].gl_Position;
    GS_Normal = VS_Normal[ 1 ];
    EmitVertex();
    
    gl_Position = gl_in[ 2 ].gl_Position;
    GS_Normal = VS_Normal[ 2 ];
    EmitVertex();
    
    EndPrimitive();
}
