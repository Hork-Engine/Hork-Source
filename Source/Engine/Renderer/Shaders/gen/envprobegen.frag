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

layout( location = 0 ) in vec3 GS_Normal;

// Envmap must be HDRI in linear space
layout( binding = 0 ) uniform samplerCube Envmap;

layout( binding = 0, std140 ) uniform UniformBlock
{
    mat4 uTransform[6];
    vec4 uRoughness;
};

vec2 Hammersley( int k, int n ) {
    float u = 0;
    float p = 0.5;
    for ( int kk = k; bool( kk ); p *= 0.5f, kk /= 2 ) {
       if ( bool( kk & 1 ) ) {
           u += p;
       }
    }
    float x = u;
    float y = (k + 0.5f) / n;
    return vec2( x, y );
}

float radicalInverse_VdC( uint bits ) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float( bits ) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley2d( uint i, uint N ) {
    return vec2( float( i )/float( N ), radicalInverse_VdC( i ) );
}

const float PI = 3.1415926;

vec3 ImportanceSampleGGX( vec2 Xi, float Roughness, vec3 N ) {
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt( (1 - Xi.y) / (1 + (a*a - 1) * Xi.y) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    
    // Sphere to cart
    vec3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;

    // Tangent to world space    
    vec3 UpVector = abs( N.z ) < 0.99 ? vec3( 0, 0, 1 ) : vec3( 1, 0, 0 );
    vec3 Tangent = normalize( cross( UpVector, N ) );
    vec3 Bitangent = cross( N, Tangent );

    return normalize( Tangent * H.x + Bitangent * H.y + N * H.z );
}

vec3 PrefilterEnvMap( float Roughness, vec3 R ) {
    vec3 PrefilteredColor = vec3( 0.0 );
    float TotalWeight = 0.0;
    const int NumSamples = 1024;
    for ( int i = 0; i < NumSamples; i++ ) {
        vec2 Xi = Hammersley( i, NumSamples );
        vec3 H = ImportanceSampleGGX( Xi, Roughness, R );
        vec3 L = normalize( 2 * dot( R, H ) * H - R );
        float NdL = clamp( dot( R, L ), 0.0, 1.0 );
        if ( NdL > 0 ) {
            PrefilteredColor += textureLod( Envmap, L, 0 ).rgb * NdL;
            TotalWeight += NdL;
        }
    }
    return PrefilteredColor / TotalWeight;
}

void main() {
    FS_FragColor = vec4( PrefilterEnvMap( uRoughness.x, normalize( GS_Normal ) ), 1.0 );
}
