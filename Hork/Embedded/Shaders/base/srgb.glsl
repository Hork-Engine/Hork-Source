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

#ifndef SRGB_H
#define SRGB_H

float LinearToSRGB(in float lin)
{
#ifdef SRGB_GAMMA_APPROX
    return pow(lin, 1.0 / 2.2);
#else
    return mix(1.055 * pow(lin, 1.0 / 2.4) - 0.055, lin * 12.92, step(lin, 0.0031308));
#endif
}

vec3 LinearToSRGB(in vec3 lin)
{
#ifdef SRGB_GAMMA_APPROX
    return pow(lin, vec3(1.0 / 2.2));
#else
    return mix(1.055 * pow(lin, vec3(1.0 / 2.4)) - 0.055, lin * 12.92, step(lin, vec3(0.0031308)));
#endif
}

vec4 LinearToSRGB_Alpha(in vec4 lin)
{
#ifdef SRGB_GAMMA_APPROX
    return pow(lin, vec4(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2, 1.0));
#else
    const vec4 Shift = vec4(-0.055, -0.055, -0.055, 0.0);
    const vec4 Scale = vec4(1.055, 1.055, 1.055, 1.0);
    const vec4 Pow = vec4(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4, 1.0);
    const vec4 Scale2 = vec4(12.92, 12.92, 12.92, 1.0);
    return mix(Scale * pow(lin, Pow) + Shift, lin * Scale2, step(lin, vec4(0.0031308)));
#endif
}

float LinearFromSRGB(in float srgb)
{
#ifdef SRGB_GAMMA_APPROX
    return pow(srgb, 2.2);
#else
    return mix(pow((color + 0.055) / 1.055, 2.4), color / 12.92, step(color, 0.04045));
#endif
}

vec3 LinearFromSRGB(in vec3 srgb)
{
#ifdef SRGB_GAMMA_APPROX
    return pow(srgb, vec3(2.2));
#else
    return mix(pow((color + 0.055) / 1.055, vec3(2.4)), color / 12.92, step(color, vec3(0.04045)));
#endif
}

vec4 LinearFromSRGB(in vec4 srgb)
{
    return vec4(LinearFromSRGB(srgb.rgb), srgb.a);
}

float LinearFromSRGB_Fast(in float srgb)
{
    return srgb * (srgb * (srgb * 0.305306011f + 0.682171111f) + 0.012522878f);
}

vec3 LinearFromSRGB_Fast(in vec3 srgb)
{
    return srgb * (srgb * (srgb * 0.305306011f + 0.682171111f) + 0.012522878f);
}

vec4 LinearFromSRGB_Fast(in vec4 srgb)
{
    return vec4(LinearFromSRGB_Fast(srgb.rgb), srgb.a);
}

#endif // SRGB_H
