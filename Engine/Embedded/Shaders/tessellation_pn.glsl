/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

vec3 ProjectToPlane( vec3 Point, vec3 PlanePoint, vec3 PlaneNormal ) {
    vec3 v = Point - PlanePoint;
    float len = dot( v, PlaneNormal );
    vec3 d = len * PlaneNormal;
    return Point - d;
}

void SetPatchVertices() {
#if TESSELLATION_METHOD == TESSELLATION_PN
    TCS_Patch.B030 = VS_Position[0];
    TCS_Patch.B003 = VS_Position[1];
    TCS_Patch.B300 = VS_Position[2];

    const vec3 EdgeB300 = TCS_Patch.B003 - TCS_Patch.B030;
    const vec3 EdgeB030 = TCS_Patch.B300 - TCS_Patch.B003;
    const vec3 EdgeB003 = TCS_Patch.B030 - TCS_Patch.B300;

    TCS_Patch.B021 = ProjectToPlane( TCS_Patch.B030 + EdgeB300 * ( 1.0 / 3.0 ), TCS_Patch.B030, TCS_Patch.N[0] );
    TCS_Patch.B012 = ProjectToPlane( TCS_Patch.B030 + EdgeB300 * ( 2.0 / 3.0 ), TCS_Patch.B003, TCS_Patch.N[1] );
    TCS_Patch.B102 = ProjectToPlane( TCS_Patch.B003 + EdgeB030 * ( 1.0 / 3.0 ), TCS_Patch.B003, TCS_Patch.N[1] );
    TCS_Patch.B201 = ProjectToPlane( TCS_Patch.B003 + EdgeB030 * ( 2.0 / 3.0 ), TCS_Patch.B300, TCS_Patch.N[2] );
    TCS_Patch.B210 = ProjectToPlane( TCS_Patch.B300 + EdgeB003 * ( 1.0 / 3.0 ), TCS_Patch.B300, TCS_Patch.N[2] );
    TCS_Patch.B120 = ProjectToPlane( TCS_Patch.B300 + EdgeB003 * ( 2.0 / 3.0 ), TCS_Patch.B030, TCS_Patch.N[0] );

    TCS_Patch.B111 = ( TCS_Patch.B021 + TCS_Patch.B012 + TCS_Patch.B102 + TCS_Patch.B201 + TCS_Patch.B210 + TCS_Patch.B120 ) * 0.25 - ( TCS_Patch.B003 + TCS_Patch.B030 + TCS_Patch.B300 ) * ( 1.0 / 6.0 );
#endif
}

