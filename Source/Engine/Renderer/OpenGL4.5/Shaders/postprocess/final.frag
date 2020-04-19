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

#include "base/viewuniforms.glsl"
#include "base/srgb.glsl"
#include "base/tonemapping.glsl"
#include "base/debug.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec4 VS_TexCoord;
layout( location = 1 ) flat in float VS_Exposure;

layout( binding = 0 ) uniform sampler2D Smp_Source;
layout( binding = 1 ) uniform sampler2D Smp_Reserved; // Reserved for future
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
    if ( PostprocessAttrib.x > 0.0 ) {
		FS_FragColor += CalcBloom();
	}

	vec3 fragColor = FS_FragColor.rgb;

	// Tonemapping
    if ( PostprocessAttrib.y > 0.0 ) {
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

    // Vignette
    if ( VignetteColorIntensity.a > 0.0 ) {
        vec2 VignetteOffset = VS_TexCoord.zw - 0.5;
        float LengthSqr = dot( VignetteOffset, VignetteOffset );
        float VignetteShade = smoothstep( VignetteOuterInnerRadiusSqr.x, VignetteOuterInnerRadiusSqr.y, LengthSqr );
        fragColor = mix( VignetteColorIntensity.rgb, fragColor, mix( 1.0, VignetteShade, VignetteColorIntensity.a ) );
	}

	// Apply brightness
    fragColor *= GetFrameBrightness();
	
	FS_FragColor.rgb = fragColor;

    // Pack pixel luminance to alpha channel for FXAA algorithm
    FS_FragColor.a = PostprocessAttrib.w > 0.0
	                       ? LinearToSRGB( builtin_saturate( builtin_luminance( fragColor ) ) )
						   : 1.0;
						   
#ifdef DEBUG_RENDER_MODE
    uint DebugMode = NumDirectionalLights.w;
    switch( DebugMode ) {
	case 0:
	    break;
    case DEBUG_BLOOM:
        FS_FragColor = CalcBloom();
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
