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

#ifndef PBR_H
#define PBR_H

vec3 Diffuse(vec3 Albedo) {
    return Albedo / PI;
}

float NormalDistribution_GGX(float SqrRoughness, float NdH) {
    float a2 = SqrRoughness * SqrRoughness;
    float NdH2 = NdH * NdH;

    float denominator = NdH2 * (a2 - 1.0) + 1.0;
    denominator *= denominator;
    denominator *= PI;

    return a2 / denominator;
}

float Geometric_Smith_Schlick_GGX(float SqrRoughness, float NdV, float NdL) {
    float k = SqrRoughness * 0.5f;
    float GV = NdV / (NdV * (1 - k) + k);
    float GL = NdL / (NdL * (1 - k) + k);
    return GV * GL;
}

//float Fresnel_Schlick(float u) {
//    float m = Saturate(1.0 - u);
//    float m2 = m * m;
//    return m2 * m2 * m;
//}

vec3 Fresnel_Schlick(vec3 SpecularColor, float VdH ) {
    return (SpecularColor + (1.0 - SpecularColor) * pow(1.0 - VdH, 5.0));
}

// Распределение отражающих микрограней
float Specular_D(float SqrRoughness, float NdH) {
    return NormalDistribution_GGX(SqrRoughness, NdH);
}

// Расчет геометрии перекрытия
float Specular_G(float SqrRoughness, float NdV, float NdL, float NdH, float VdH, float LdV) {
    return Geometric_Smith_Schlick_GGX(SqrRoughness, NdV, NdL);
}

// Коэффициент отражения Френеля
vec3 Specular_F(vec3 SpecularColor, float VdH) {
    return Fresnel_Schlick(SpecularColor, VdH);
}

vec3 Specular(vec3 SpecularColor, float SqrRoughness, float NdL, float NdV, float NdH, float VdH, float LdV) {
    return ((Specular_D(SqrRoughness, NdH) * Specular_G(SqrRoughness, NdV, NdL, NdH, VdH, LdV)) * Specular_F(SpecularColor, VdH) ) / (4.0 * NdL * NdV + 0.0001);
}

vec3 Specular_F_Roughness(vec3 SpecularColor, float SqrRoughness, float VdH ) {
    return (SpecularColor + (max(vec3(1.0 - SqrRoughness), SpecularColor) - SpecularColor) * pow(1.0 - VdH, 5.0));
}

#endif // PBR_H
