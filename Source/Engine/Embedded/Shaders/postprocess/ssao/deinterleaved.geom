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

/*

#extension GL_NV_geometry_shader_passthrough : enable

layout( triangles ) in;

#if GL_NV_geometry_shader_passthrough

layout(passthrough) in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

layout(passthrough, location = 0) in Inputs {
    noperspective vec2 VS_TexCoord;
} IN[];

void main()
{
    gl_Layer = gl_PrimitiveIDIn;
    gl_PrimitiveID = gl_PrimitiveIDIn;
}

#else

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

layout( triangle_strip, max_vertices = 3 ) out;

layout( location = 0 ) in Inputs {
    noperspective vec2 TexCoord;
} IN[];

layout( location = 0 ) noperspective out vec2 VS_TexCoord;

void main()
{
    for ( int i = 0; i < 3; i++ ){
        VS_TexCoord = IN[i].TexCoord;
        gl_Layer = gl_PrimitiveIDIn;
        gl_PrimitiveID = gl_PrimitiveIDIn;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
}

#endif


*/

layout( triangles ) in;
layout( invocations = 16 ) in;

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

layout( triangle_strip, max_vertices = 3 ) out;

layout( location = 0 ) in Inputs {
    noperspective vec2 TexCoord;
} IN[];

layout( location = 0 ) noperspective out vec2 VS_TexCoord;

void main()
{
    for ( int i = 0; i < gl_in.length(); i++ ) {
        VS_TexCoord = IN[i].TexCoord;
        gl_Layer = gl_InvocationID;
        gl_PrimitiveID = gl_InvocationID;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
}
