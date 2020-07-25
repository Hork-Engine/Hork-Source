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

#include "base/common.frag"
#include "base/viewuniforms.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Light;
layout( binding = 1 ) uniform sampler2D Smp_Velocity;
layout( binding = 2 ) uniform sampler2D Smp_Depth;

#define MAX_SAMPLES 32

void main() {
    vec2 velocity = -texture( Smp_Velocity, VS_TexCoord ).xy;
    velocity.y=-velocity.y;
    
    const float currentFPS = 1.0/GameplayFrameDelta();
    const float targetFPS = 60;
    float velocityScale = saturate( currentFPS / targetFPS ); // Move to uniform?
    velocity *= velocityScale;
    
    //velocity*=40;
    
    
    
    
    
    //const float maxVelocity = 10.0f;
    
    //float len = min( length(velocity), maxVelocity );
    //if ( len > 0.0 )
    {
        //velocity = normalize( velocity ) * len;
    
        vec2 texSize = vec2(textureSize(Smp_Light, 0)); // TODO: Use from viewuniforms
        float speed = length(velocity * texSize);
        int nSamples = clamp(int(speed), 1, MAX_SAMPLES);
    
        if(false/*VS_TexCoord.x > 0.5*/)
        {  
            float weight = exp2( float(nSamples) );
            //float weight = float(nSamples);
            float sum = weight;
            FS_FragColor = texture(Smp_Light, VS_TexCoord) * weight;
        
    
            const float bias =0;// -0.2; //-0.5 
            float srcDepth = texture( Smp_Depth, VS_TexCoord ).x;
            int total = 1;
            for ( int i = 1 ; i < nSamples ; ++i ) {
                vec2 offset = velocity * (float(i) / float(nSamples - 1) + bias);
                vec2 samplePos = VS_TexCoord + offset;
                
                float depth = texture( Smp_Depth, samplePos ).x;
                
                //if ( srcDepth - depth < 0.1 )
                {
                    //weight -= 1.0f;
                    weight *= 0.5f;
                    FS_FragColor += texture(Smp_Light, samplePos) * weight;
                    sum += weight;
                    total++;
                }
                
            }
            FS_FragColor /= sum;//float(total);
        } else {
            FS_FragColor = texture(Smp_Light, VS_TexCoord);
    
            const float bias = -0.2; //-0.5 
            float srcDepth = texture( Smp_Depth, VS_TexCoord ).x;
            int total = 1;
            for ( int i = 1 ; i < nSamples ; ++i ) {
                vec2 offset = velocity * (float(i) / float(nSamples - 1) + bias);
                vec2 samplePos = VS_TexCoord + offset;
                
                float depth = texture( Smp_Depth, samplePos ).x;
                
                if ( srcDepth - depth < 0.001 )
                {
                    FS_FragColor += texture(Smp_Light, samplePos);
                    total++;
                }
                
            }
            FS_FragColor /= float(total);
        }
        //FS_FragColor=vec4(velocity*10,0,1);
    
    }
}
