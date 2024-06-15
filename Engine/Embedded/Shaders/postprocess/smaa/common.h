/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#ifndef SMAA_COMMON_H
#define SMAA_COMMON_H

#include "base/viewuniforms.glsl"

// TODO: Precalc uniform
vec4 SMAA_RT_Metrics = vec4(InvViewportSize * DynamicResolutionRatio, vec2(1) / (InvViewportSize * DynamicResolutionRatio));

#define SMAA_GLSL_4
#define SMAA_PRESET_HIGH
#define SMAA_RT_METRICS SMAA_RT_Metrics
#define SMAA_AREATEX_SELECT(sample) sample.rg
#define SMAA_SEARCHTEX_SELECT(sample) sample.r

#ifdef VERTEX_SHADER
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
#endif

#ifdef FRAGMENT_SHADER
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#endif

#include "postprocess/smaa/SMAA.hlsl"

#endif // SMAA_COMMON_H

