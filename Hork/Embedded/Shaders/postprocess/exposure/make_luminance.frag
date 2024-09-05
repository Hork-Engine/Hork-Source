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

#include "base/core.glsl"

layout( location = 0 ) out vec2 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Input;

void main() {
    //const vec4 minColorThreshold = vec4(0.01);
    const vec4 minColorThreshold = vec4(0.1);
    const vec4 maxColorThreshold = vec4(10);
    
    vec4 color = texture( Smp_Input, VS_TexCoord );
    
    color = clamp( color, minColorThreshold, maxColorThreshold );

    FS_FragColor = vec2( builtin_luminance( color ) );

    //const float Sigma = 0.4;//0.5;
    //FS_FragColor.x = max( 0.0, log( Sigma + FS_FragColor.x ) );
}
