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

#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec3 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_LinearDepth;

vec3 UVToView( vec2 UV, float ViewZ )
{
    return vec3( ( UV * ProjectionInfo.xy + ProjectionInfo.zw ) * ViewZ, ViewZ );
}

vec3 FetchPos( vec2 UV )
{
    float ViewDepth = textureLod( Smp_LinearDepth, UV, 0 ).x;
    return UVToView(UV, ViewDepth);
}

vec3 MinDiff( vec3 P, vec3 Pr, vec3 Pl )
{
    vec3 V1 = Pr - P;
    vec3 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal( vec2 UV, vec3 P )
{
    const vec2 InvViewportSize = GetViewportSizeInverted();
    vec3 Pr = FetchPos(UV + vec2(InvViewportSize.x, 0));
    vec3 Pl = FetchPos(UV + vec2(-InvViewportSize.x, 0));
    vec3 Pt = FetchPos(UV + vec2(0, InvViewportSize.y));
    vec3 Pb = FetchPos(UV + vec2(0, -InvViewportSize.y));
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

void main() {
    FS_FragColor = ReconstructNormal( VS_TexCoord, FetchPos( VS_TexCoord ) ) * 0.5 + 0.5;
}
