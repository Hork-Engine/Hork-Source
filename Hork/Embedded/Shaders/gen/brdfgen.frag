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

layout( location = 0 ) out vec4 FS_FragColor; 

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

vec2 Hammersley( int k, int n )
{
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

const float PI = 3.1415926;

vec3 ImportanceSampleGGX( vec2 Xi, float Roughness, vec3 N )
{
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

float GeometrySchlickGGX( float NdotV, float roughness )
{
    float a = roughness;
    float k = (a * a) / 2.0; // for IBL

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith( vec3 N, vec3 V, vec3 L, float roughness )
{
    float NdotV = max( dot( N, V ), 0.0f );
    float NdotL = max( dot( N, L ), 0.0f );
    float ggx2 = GeometrySchlickGGX( NdotV, roughness );
    float ggx1 = GeometrySchlickGGX( NdotL, roughness );

    return ggx1 * ggx2;
}

vec2 IntegrateBRDF( float NdotV, float roughness )
{
    vec3 V;
    V.x = sqrt( 1.0 - NdotV*NdotV );
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3( 0.0, 0.0, 1.0 );

    const int SAMPLE_COUNT = 1024;
    for ( int i = 0; i < SAMPLE_COUNT; ++i )
    {
        vec2 Xi = Hammersley( i, SAMPLE_COUNT );
        vec3 H = ImportanceSampleGGX( Xi, roughness, N );
        vec3 L = normalize(2.0f * dot( V, H ) * H - V);

        float NdotL = max( L.z, 0.0f );
        float NdotH = max( H.z, 0.0f );
        float VdotH = max( dot( V, H ), 0.0f );

        if ( NdotL > 0.0 )
        {
            float G = GeometrySmith( N, V, L, roughness );
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow( 1.0 - VdotH, 5.0 );

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float( SAMPLE_COUNT );
    B /= float( SAMPLE_COUNT );
    return vec2( A, B );
}

void main()
{
    vec2 uv = (VS_TexCoord * vec2(511, 255) + vec2(1.0)) / vec2(512, 256);
    FS_FragColor = vec4( IntegrateBRDF( uv.x, uv.y ), 0.0, 0.0 );
}
