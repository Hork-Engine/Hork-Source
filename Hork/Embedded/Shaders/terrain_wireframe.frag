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

layout( location = 0 ) out vec4 FS_FragColor;
layout( location = 0 ) in vec3 GS_Barycentric;

void main() {
    //const vec4 Color = vec4(0.0,0.3,0.0,0.5);
    //const vec4 Color = vec4(1.0);
    const vec4 Color = vec4(1.0,0.0,0.0,0.9);
    const float LineWidth = 1;//1.5;
    vec3 SmoothStep = smoothstep( vec3( 0.0 ), fwidth( GS_Barycentric ) * LineWidth, GS_Barycentric );
    FS_FragColor = Color;
    FS_FragColor.a *= 1.0 - min( min( SmoothStep.x, SmoothStep.y ), SmoothStep.z );
}

