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

vec3 FetchLocalReflection( vec3 ReflectionDir, float Roughness ) {
#if defined WITH_SSLR && defined ALLOW_SSLR
    vec2 UV = InScreenUV;
    float L = SSLRSampleOffset;
    
    // calc fragment position at prev frame
    vec3 positionPrevFrame = (ViewspaceReprojection * vec4( VS_Position, 1.0 )).xyz;
    
    int error = 1;
    float z;
    for ( int i = 0; i < 10; i++ )
    {
        vec3 dir = VS_Position + ReflectionDir * L;

        // calc UV for previous frame
        vec2 tempUV = ViewToUVReproj( dir );

        // read reflection depth
        z = -textureLod( ReflectionDepth, tempUV, 0 ).x;
        
        if ( z > VS_Position.z ) {
            L += SSLRMaxDist;
            //break;
            error++;
        } else {
            // calc viewspace position at prev frame
            vec3 p = UVToView( tempUV, z );

            // calc distance between fragments
            L = distance( positionPrevFrame, p );
            
            UV = tempUV;
        }
    }
    
    //L = saturate(L * SSLRMaxDist);
    
    vec3 reflectColor = textureLod( ReflectionColor, UV, 0 ).rgb;

    vec2 coords = smoothstep( 0.2, 0.6, abs( vec2(0.5) - UV.xy ) );
    float screenFalloff = saturate( 1.0 - (coords.x + coords.y) );
    const float falloffExponent = 3.0;
    float falloff = pow( 1 - Roughness, falloffExponent ) * screenFalloff *  saturate(-ReflectionDir.z) / float(error);// * (1.0-L);
    
    return clamp( reflectColor * falloff, 0.0, 20.0 );
#else
    return vec3( 0.0 );
#endif
}
