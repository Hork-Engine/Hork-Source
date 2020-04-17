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

#include "viewuniforms.glsl"
#include "srgb.glsl"
#include "tonemapping.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) noperspective in vec4 VS_TexCoord;
layout( location = 1 ) flat in float VS_Exposure;

layout( binding = 0 ) uniform sampler2D Smp_Source;
layout( binding = 1 ) uniform sampler2D Smp_Dither;
layout( binding = 2 ) uniform sampler2D Smp_Bloom2;
layout( binding = 3 ) uniform sampler2D Smp_Bloom8;
layout( binding = 4 ) uniform sampler2D Smp_Bloom32;
layout( binding = 5 ) uniform sampler2D Smp_Bloom128;

vec4 CalcBloom() {
	vec2 tc = VS_TexCoord.zw;
	vec4 dither = vec4( (texture( Smp_Dither, tc*3.141592 ).r-0.5)*2.0 );

    vec4 bloom0[4];
    vec4 bloom1[4];

    bloom0[0] = texture( Smp_Bloom2, tc );
    bloom0[1] = texture( Smp_Bloom8, tc );
    bloom0[2] = texture( Smp_Bloom32, tc );
    bloom0[3] = texture( Smp_Bloom128, tc );

    bloom1[0] = texture( Smp_Bloom2, tc+dither.xz   / 512.0 );
    bloom1[1] = texture( Smp_Bloom8, tc+dither.zx   / 128.0 );
    bloom1[2] = texture( Smp_Bloom32, tc+dither.xy  /  32.0 );
    bloom1[3] = texture( Smp_Bloom128, tc+dither.yz /   8.0 );

    return mat4(
		bloom0[0]+clamp( bloom1[0]-bloom0[0], -1.0/256.0, +1.0/256.0 ),
		bloom0[1]+clamp( bloom1[1]-bloom0[1], -1.0/256.0, +1.0/256.0 ),
		bloom0[2]+clamp( bloom1[2]-bloom0[2], -1.0/256.0, +1.0/256.0 ),
		bloom0[3]+clamp( bloom1[3]-bloom0[3], -1.0/256.0, +1.0/256.0 )
    ) * PostprocessBloomMix;
}

void main() {
	FS_FragColor = texture( Smp_Source, VS_TexCoord.xy );

	// Bloom
    if ( PostprocessAttrib.x > 0.0 ) {
		FS_FragColor += CalcBloom();
	}

    // Debug bloom
	//FS_FragColor = CalcBloom();

	// Tonemapping
    if ( PostprocessAttrib.y > 0.0 ) {
		if ( VS_TexCoord.x > 0.5 ) {
			FS_FragColor.rgb = ToneLinear( FS_FragColor.rgb, VS_Exposure );
		} else {
			FS_FragColor.rgb = ACESFilm( FS_FragColor.rgb, VS_Exposure );
		}
	}

    // Vignette
    //  if ( VignetteColorIntensity.a > 0.0 ) {
    //    vec2 VignetteOffset = VS_TexCoord.zw - 0.5;
    //    float LengthSqr = dot( VignetteOffset, VignetteOffset );
    //    float VignetteShade = smoothstep( VignetteOuterInnerRadiusSqr.x, VignetteOuterInnerRadiusSqr.y, LengthSqr );
    //    FS_FragColor.rgb = mix( VignetteColorIntensity.rgb, FS_FragColor.rgb, mix( 1.0, VignetteShade, VignetteColorIntensity.a ) );
	//  }

	// Apply brightness
    FS_FragColor.rgb *= VignetteOuterInnerRadiusSqr.z;

    // Pack pixel luminance to alpha channel for FXAA algorithm
    const vec3 RGB_TO_GRAYSCALE = vec3( 0.2125, 0.7154, 0.0721 );
    FS_FragColor.a = PostprocessAttrib.w > 0.0 ? LinearToSRGB( clamp( dot( FS_FragColor.rgb, RGB_TO_GRAYSCALE ), 0.0, 1.0 ) ) : 1.0;

    // Debug output
    //if ( VS_TexCoord.x < 0.05 && VS_TexCoord.y < 0.05 ) {
    //    FS_FragColor = vec4( VS_Exposure, VS_Exposure, VS_Exposure, 1.0 );
    //}

}
