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

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Source;

const vec4 BloomStart = vec4( 1.0 ); // TODO: input uniform

void main()
{
    // Adjust texture coordinates for dynamic resoution
    vec2 tc = min( VS_TexCoord, vec2(1.0) - GetViewportSizeInverted() ) * GetDynamicResolutionRatio();
    tc.y = 1.0 - tc.y;
    
    // Bloom texture has RGB8 format, so there is no need to clamp value
    FS_FragColor = texture( Smp_Source, tc ) - BloomStart;
}
