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


#include "postprocess/smaa/common.h"

layout(location = 0) out vec4 FS_FragColor;

layout(location = 0) noperspective in vec2 VS_TexCoord;
layout(location = 1) in vec4 VS_Offset;

layout(binding = 0) uniform sampler2D Smp_Source;
layout(binding = 1) uniform sampler2D Smp_BlendTex;
#if SMAA_REPROJECTION
layout(binding = 2) uniform sampler2D Smp_VelocityTex;
#endif

void main()
{
    // Adjust texture coordinates for dynamic resolution
    vec2 tc = VS_TexCoord;//AdjustTexCoord(VS_TexCoord);
    
    #if SMAA_REPROJECTION
    FS_FragColor = SMAANeighborhoodBlendingPS(tc, VS_Offset, Smp_Source, Smp_BlendTex, Smp_VelocityTex);
    #else
    FS_FragColor = SMAANeighborhoodBlendingPS(tc, VS_Offset, Smp_Source, Smp_BlendTex);
    #endif
}

