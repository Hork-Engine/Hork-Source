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

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Source;
layout( binding = 1 ) uniform sampler2D Smp_Dither;

layout( location = 0 ) uniform vec2 InvSize;

void main() {
    const float weight0 = 1.0 / 18.0;
    const float weight1 = 2.0 / 18.0;
    const float weight2 = 4.0 / 18.0;
    const float weight3 = 4.0 / 18.0;
    const float weight4 = 4.0 / 18.0;
    const float weight5 = 2.0 / 18.0;
    const float weight6 = 1.0 / 18.0;

    vec4 final = texture( Smp_Source, VS_TexCoord - 3.0*InvSize ) * weight0
        + texture( Smp_Source, VS_TexCoord - 2.0*InvSize ) * weight1
        + texture( Smp_Source, VS_TexCoord - 1.0*InvSize ) * weight2
        + texture( Smp_Source, VS_TexCoord ) * weight3
        + texture( Smp_Source, VS_TexCoord + 1.0*InvSize ) * weight4
        + texture( Smp_Source, VS_TexCoord + 2.0*InvSize ) * weight5
        + texture( Smp_Source, VS_TexCoord + 3.0*InvSize ) * weight6;

    FS_FragColor = final + (texture( Smp_Dither, VS_TexCoord * 3.141592 ).r - 0.5) / 192.0;
}
