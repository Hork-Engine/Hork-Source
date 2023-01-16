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

#include "base/common.frag"
#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Light;
layout( binding = 1 ) uniform sampler2D Smp_Velocity;
layout( binding = 2 ) uniform sampler2D Smp_Depth;

#define MAX_SAMPLES 32

void main() {
    vec2 uv = vec2( VS_TexCoord.x, 1.0-VS_TexCoord.y );
    
    // Adjust texture coordinates for dynamic resolution
    //vec2 tc = AdjustTexCoord( VS_TexCoord );
    
    vec2 tc = min( VS_TexCoord, vec2(1.0) ) * GetDynamicResolutionRatio();
    tc.y = 1.0 - tc.y;
    
    // Fragment depth
    float srcDepth = texture( Smp_Depth, tc ).x;

    // Unpack dynamic object velocity
    vec2 v = texture( Smp_Velocity, tc ).xy;
    if ( v.x < 1.0 ) {
        v = v * ( 255.0 / 254.0 );
        v = v * 2.0 - 1.0;
        v = v * v * sign( v );
    }
    else {
        // Calc fragment UV at previous frame
        const vec3 fragmentPosViewspace = UVToView( uv, -srcDepth );
        const vec2 prevUV = ViewToUVReproj( fragmentPosViewspace );

        // Calc camera velocity
        v = uv - prevUV;
    }

    //if ( tc.x > 0.5 ) {
    //    FS_FragColor = vec4( (v), texture( Smp_Velocity, tc ).x < 1.0 ? 0.005 : 0.0, 1.0 );
    //    return;
    //}

    // Adjust velocity by current frame rate
    const float currentFPS = 1.0/GameplayFrameDelta();
    const float targetFPS = 60;
    const float velocityScale = saturate( currentFPS / targetFPS ); // Move to uniform?
    v *= velocityScale;

    // Clamp velocity to max threshold
    const float maxVelocity = 0.05;
    const float maxVelocitySqr = maxVelocity * maxVelocity;
    if ( dot( v, v ) > maxVelocitySqr ) {
       v = normalize( v ) * maxVelocity;
    }
    
    const vec2 texSize = 1.0 / (InvViewportSize * DynamicResolutionRatio);
    const float speed = length( v * texSize );
    const int nSamples = clamp( int(speed), 1, MAX_SAMPLES );

    // Smp_Light has linear sampler, but we need nearest sampler here
    FS_FragColor = texelFetch( Smp_Light, ivec2( tc * textureSize( Smp_Light, 0 ) ), 0 );
    
    const float bias = -0.5;

    float total = 1;
    for ( int i = 1 ; i < nSamples ; ++i ) {
        vec2 offset = v * (float(i) / float(nSamples - 1) + bias);
        vec2 samplePos = uv + offset;
        
        samplePos = min( vec2(samplePos.x,1.0-samplePos.y), vec2(1.0) - GetViewportSizeInverted() ) * GetDynamicResolutionRatio();    
        samplePos.y = 1.0 - samplePos.y;
        
        float depth = texture( Smp_Depth, samplePos ).x;
#if 0
        if ( srcDepth - depth < 0.001 ) {
            FS_FragColor += texture( Smp_Light, samplePos );
            total++;
        }
#else
        float weight = 1.0 - saturate( srcDepth - depth - 0.003 );

        FS_FragColor += texture( Smp_Light, samplePos ) * weight;
        total += weight;
#endif
    }
    FS_FragColor *= 1.0 / float(total);
}
