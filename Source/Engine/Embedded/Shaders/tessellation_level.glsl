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

// Edge-based subdivide in view space
void SetTessLevelViewSpace( float TessellationFactor ) {
    // FIXME: This doesn't take into account camera field of view
    float EdgeFactor = TessellationFactor * TessellationLevel;

    vec3 EdgeCenter0 = ( VS_Position[1] + VS_Position[2] ) * 0.5;
    vec3 EdgeCenter1 = ( VS_Position[2] + VS_Position[0] ) * 0.5;
    vec3 EdgeCenter2 = ( VS_Position[0] + VS_Position[1] ) * 0.5;

    float d0 = distance(VS_Position[1], VS_Position[2]) * EdgeFactor / length(EdgeCenter0);
    float d1 = distance(VS_Position[2], VS_Position[0]) * EdgeFactor / length(EdgeCenter1);
    float d2 = distance(VS_Position[0], VS_Position[1]) * EdgeFactor / length(EdgeCenter2);

    gl_TessLevelOuter[0] = d0;
    gl_TessLevelOuter[1] = d1;
    gl_TessLevelOuter[2] = d2;
    gl_TessLevelInner[0] = ( d0 + d1 + d2 ) * ( 1.0 / 3.0 );
}

// Edge-based subdivide in world space
void SetTessLevelWorldSpace( float TessellationFactor ) {
    // FIXME: This doesn't take into account camera field of view
    float EdgeFactor = TessellationFactor * TessellationLevel;

    vec3 EdgeCenter0 = ( VS_Position[1] + VS_Position[2] ) * 0.5;
    vec3 EdgeCenter1 = ( VS_Position[2] + VS_Position[0] ) * 0.5;
    vec3 EdgeCenter2 = ( VS_Position[0] + VS_Position[1] ) * 0.5;

    float ViewDistance0 = distance(EdgeCenter0, ViewPosition.xyz);
    float ViewDistance1 = distance(EdgeCenter1, ViewPosition.xyz);
    float ViewDistance2 = distance(EdgeCenter2, ViewPosition.xyz);

    float d0 = distance(VS_Position[1], VS_Position[2]) * EdgeFactor / ViewDistance0;
    float d1 = distance(VS_Position[2], VS_Position[0]) * EdgeFactor / ViewDistance1;
    float d2 = distance(VS_Position[0], VS_Position[1]) * EdgeFactor / ViewDistance2;

    gl_TessLevelOuter[0] = d0;
    gl_TessLevelOuter[1] = d1;
    gl_TessLevelOuter[2] = d2;
    gl_TessLevelInner[0] = ( d0 + d1 + d2 ) * ( 1.0 / 3.0 );
}

#if 0
// Edge-based subdivide in screen space
void SetTessLevelWorldSpace( float TessellationFactor ) {
    vec4 p0 = TransformMatrix * vec4( VS_Position[0], 1.0 );
    vec4 p1 = TransformMatrix * vec4( VS_Position[1], 1.0 );
    vec4 p2 = TransformMatrix * vec4( VS_Position[2], 1.0 );

    float EdgeFactor = TessellationFactor * TessellationLevel;

    vec2 s0 = p0.xy / p0.w;
    vec2 s1 = p1.xy / p1.w;
    vec2 s2 = p2.xy / p2.w;

    float d0 = distance(s1, s2) * EdgeFactor;
    float d1 = distance(s2, s0) * EdgeFactor;
    float d2 = distance(s0, s1) * EdgeFactor;

    gl_TessLevelOuter[0] = d0;
    gl_TessLevelOuter[1] = d1;
    gl_TessLevelOuter[2] = d2;
    gl_TessLevelInner[0] = ( d0 + d1 + d2 ) * ( 1.0 / 3.0 );
}
#endif
