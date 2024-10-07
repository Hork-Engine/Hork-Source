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

#include "base/viewuniforms.glsl"
#include "base/srgb.glsl"
#include "base/tonemapping.glsl"
#include "base/debug.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec4 VS_TexCoord;
layout( location = 1 ) flat in float VS_Exposure;

layout( binding = 0 ) uniform sampler2D Smp_Source;
layout( binding = 1 ) uniform sampler3D Smp_ColorGradingLUT;
layout( binding = 2 ) uniform sampler2D Smp_Bloom2;
layout( binding = 3 ) uniform sampler2D Smp_Bloom8;
layout( binding = 4 ) uniform sampler2D Smp_Bloom32;
layout( binding = 5 ) uniform sampler2D Smp_Bloom128;

vec4 CalcBloom() {
    vec2 tc = VS_TexCoord.zw;
    
    return mat4( texture( Smp_Bloom2, tc ),
                 texture( Smp_Bloom8, tc ),
                 texture( Smp_Bloom32, tc ),
                 texture( Smp_Bloom128, tc ) ) * PostprocessBloomMix;
}

void main() {
    FS_FragColor = texture( Smp_Source, VS_TexCoord.xy );

    // Bloom
    if ( IsBloomEnabled() ) {
        FS_FragColor += CalcBloom();
    }

    vec3 fragColor = FS_FragColor.rgb;

    // Tonemapping
    if ( IsTonemappingEnabled() ) {
        //if ( VS_TexCoord.x > 0.5 ) {
            //fragColor = ToneLinear( fragColor, VS_Exposure );
            fragColor = ToneReinhard3( fragColor, VS_Exposure, 1.9 );
        //} else {
            //fragColor = ToneReinhard( fragColor, VS_Exposure );
            //fragColor = ToneReinhard2( fragColor, VS_Exposure, 1.9 );
            //fragColor = Uncharted2Tonemap( fragColor, VS_Exposure );
            //fragColor = ACESFilm( fragColor, VS_Exposure );
            //fragColor = ToneFilmicALU( fragColor, VS_Exposure );
            //fragColor = ToneFilmicALU( fragColor, 1.0 );
        //}
    }
    
    // Apply brightness
    fragColor *= GetFrameBrightness();

    //float srclum = builtin_luminance(fragColor);
    //float lum = srclum*GetFrameBrightness();
    //fragColor = fragColor * (lum / srclum);

    // Saturate color
    fragColor = saturate( fragColor );

    // Color grading
    if ( IsColorGradingEnabled() ) {
        //if ( VS_TexCoord.x > 0.5 ) {
            fragColor = texture( Smp_ColorGradingLUT, LinearToSRGB( fragColor ) ).rgb;
        //}
    }

    // Vignette
    if ( IsVignetteEnabled() ) {
        vec2 VignetteOffset = VS_TexCoord.zw - 0.5;
        float LengthSqr = dot( VignetteOffset, VignetteOffset );
        float VignetteShade = smoothstep( VignetteOuterRadiusSqr, VignetteInnerRadiusSqr, LengthSqr );
        fragColor = mix( VignetteColorIntensity.rgb, fragColor, mix( 1.0, VignetteShade, VignetteColorIntensity.a ) );
    }

    // Apply brightness
    //fragColor *= GetFrameBrightness();
    
    FS_FragColor.rgb = fragColor;
//FS_FragColor.rgb = sepia( floor(FS_FragColor.rgb * 64)/64, 0.5 );

    //float srclum = builtin_luminance(FS_FragColor.rgb);
    //float lum = floor(srclum*128)/128;
    //FS_FragColor.rgb = FS_FragColor.rgb/srclum * lum;

    // Pack pixel luminance to alpha channel for FXAA algorithm
    FS_FragColor.a = IsFXAAEnabled()
                           ? LinearToSRGB( builtin_luminance( fragColor ) )
                           : 1.0;

#ifdef DEBUG_RENDER_MODE
    uint DebugMode = GetDebugMode();
    switch( DebugMode ) {
    case 0:
        break;
    case DEBUG_BLOOM:
        FS_FragColor = CalcBloom();
        break;
    case DEBUG_BLOOMTEX1:
        FS_FragColor = texture( Smp_Bloom2, VS_TexCoord.zw );
        break;
    case DEBUG_BLOOMTEX2:
        FS_FragColor = texture( Smp_Bloom8, VS_TexCoord.zw );
        break;
    case DEBUG_BLOOMTEX3:
        FS_FragColor = texture( Smp_Bloom32, VS_TexCoord.zw );
        break;
    case DEBUG_BLOOMTEX4:
        FS_FragColor = texture( Smp_Bloom128, VS_TexCoord.zw );
        break;
    case DEBUG_EXPOSURE:
        if ( VS_TexCoord.x < 0.05 && VS_TexCoord.y < 0.05 ) {
            FS_FragColor = vec4( VS_Exposure, VS_Exposure, VS_Exposure, 1.0 );
        }
        break;
    default:
        FS_FragColor = texture( Smp_Source, VS_TexCoord.xy );
        break;
    }
#endif
}
