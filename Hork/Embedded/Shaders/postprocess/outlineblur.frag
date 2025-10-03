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

#include "base/common.frag"
#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec2 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_OutlineMask;

vec4 textureRegion( sampler2D s, vec2 texCoord )
{
    vec2 tc = min( texCoord, vec2(1.0) - GetViewportSizeInverted() ) * GetDynamicResolutionRatio();
    
    tc.y = 1.0 - tc.y;
    
    return texture( s, tc );
}

vec2 GaussianBlur13RGEx( sampler2D Image, vec2 TexCoord, vec2 Direction ) {
    vec2 Offset1 = Direction * 1.411764705882353;
    vec2 Offset2 = Direction * 3.2941176470588234;
    vec2 Offset3 = Direction * 5.176470588235294;
    return textureRegion( Image, TexCoord ).rg * 0.1964825501511404
       + ( textureRegion( Image, TexCoord + Offset1 ).rg + textureRegion( Image, TexCoord - Offset1 ).rg ) * 0.2969069646728344
       + ( textureRegion( Image, TexCoord + Offset2 ).rg + textureRegion( Image, TexCoord - Offset2 ).rg ) * 0.09447039785044732
       + ( textureRegion( Image, TexCoord + Offset3 ).rg + textureRegion( Image, TexCoord - Offset3 ).rg ) * 0.010381362401148057;
}

void main() {
    // Adjust texture coordinates for dynamic resolution
    //vec2 tc = AdjustTexCoord( VS_TexCoord );
    
    FS_FragColor = GaussianBlur13RGEx( Smp_OutlineMask, VS_TexCoord, vec2( InvViewportSize.x * DynamicResolutionRatio.x, 0.0 ) );
}
