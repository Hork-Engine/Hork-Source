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

#ifndef COOK_TORRANCE_BRDF_H
#define COOK_TORRANCE_BRDF_H

float DistributionGGX( float RoughnessSqr, float NdH ) {
    const float a2 = RoughnessSqr * RoughnessSqr; // TODO: Precompute once per-fragment!
    const float NdH2 = NdH * NdH;

    float denominator = mad( NdH2, a2 - 1.0, 1.0 );
    denominator *= denominator;
    denominator *= PI;  // TODO: мы можем вынести деление на PI и делить только один раз: ( L1 + L2 + .. + Ln ) / PI, вместо Diffuse использовать Albedo, т.к. Diffuse=Albedo/PI

    return a2 / denominator;
}

// float k = Sqr( Roughness ) * 0.5f; //  IBL
// float k = Sqr( Roughness + 1 ) * 0.125; // Direct light
float GeometrySmith( float NdV, float NdL, float k ) {
    const float invK = 1.0 - k;

    // Geometry schlick GGX for V
    const float ggxV = NdV / mad(NdV, invK, k); // TODO: Precompute once per-fragment!

    // Geometry schlick GGX for L
    const float ggxL = NdL / mad(NdL, invK, k);

    return ggxV * ggxL;
}

vec3 FresnelSchlick( vec3 F0, float VdH ) {
    return mad( pow( 1.0 - VdH, 5.0 ), 1.0 - F0, F0 );
}

vec3 FresnelSchlick_Roughness( vec3 F0, float NdV, float Roughness ) {
    vec3 a = max( vec3( 1.0 - Roughness ), F0 );
    
    return mad( pow( 1.0 - NdV, 5.0 ), a - F0, F0 );
}

#endif

