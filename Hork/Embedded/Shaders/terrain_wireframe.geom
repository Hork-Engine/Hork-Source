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

#include "terrain_common.glsl"

in gl_PerVertex
{
    vec4 gl_Position;
#ifdef TERRAIN_CLIP_BOUNDS
    float gl_ClipDistance[4];
#endif
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
#ifdef TERRAIN_CLIP_BOUNDS
    float gl_ClipDistance[4];
#endif
};

layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;
layout( location = 0 ) out vec3 GS_Barycentric;

void main() {
    if (gl_in[ 0 ].gl_Position.w != 0.0 && gl_in[ 1 ].gl_Position.w != 0.0 && gl_in[ 2 ].gl_Position.w != 0.0)
    {
        gl_Position = gl_in[ 0 ].gl_Position;
        GS_Barycentric = vec3( 1, 0, 0 );
#ifdef TERRAIN_CLIP_BOUNDS
        for ( int i = 0 ; i < 4 ; i++ ) gl_ClipDistance[i] = gl_in[ 0 ].gl_ClipDistance[i];
#endif
        EmitVertex();
        gl_Position = gl_in[ 1 ].gl_Position;
        GS_Barycentric = vec3( 0, 1, 0 );
#ifdef TERRAIN_CLIP_BOUNDS
        for ( int i = 0 ; i < 4 ; i++ ) gl_ClipDistance[i] = gl_in[ 1 ].gl_ClipDistance[i];
#endif
        EmitVertex();
        gl_Position = gl_in[ 2 ].gl_Position;
        GS_Barycentric = vec3( 0, 0, 1 );
#ifdef TERRAIN_CLIP_BOUNDS
        for ( int i = 0 ; i < 4 ; i++ ) gl_ClipDistance[i] = gl_in[ 2 ].gl_ClipDistance[i];
#endif
        EmitVertex();
        EndPrimitive();
    }
}
