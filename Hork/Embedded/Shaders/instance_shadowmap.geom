/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

#include "$SHADOWMAP_PASS_GEOMETRY_INPUT_VARYINGS$"
#include "$SHADOWMAP_PASS_GEOMETRY_OUTPUT_VARYINGS$"

layout( invocations = MAX_SHADOW_CASCADES )
layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;
//layout( location = SHADOWMAP_PASS_VARYING_INSTANCE_ID ) in flat int VS_InstanceID[];

layout( binding = 3, std140 ) uniform ShadowMatrixBuffer {
    mat4 CascadeViewProjection[ MAX_SHADOW_CASCADES ];
};

void main() {
    if ( ( CascadeMask.x & (1<<gl_InvocationID) ) != 0 ) {
        gl_Layer = gl_InvocationID; //VS_InstanceID[ 0 ];
        for ( int i = 0; i < gl_in.length(); i++ ) {
            gl_Position = CascadeViewProjection[ gl_InvocationID ] * gl_in[ i ].gl_Position;

#           include "$SHADOWMAP_PASS_GEOMETRY_COPY_VARYINGS$"
            EmitVertex();
        }
        EndPrimitive();
    }
}
