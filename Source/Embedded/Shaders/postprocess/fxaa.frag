/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#define FXAA_PC 1
#define FXAA_GLSL_130 1
//#define FXAA_QUALITY__PRESET 12
#define FXAA_QUALITY__PRESET 39
#define FXAA_GATHER4_ALPHA 1
        
#include "postprocess/FXAA_3_11.h"
#include "base/viewuniforms.glsl"
//#include "base/srgb.glsl"

layout( location = 0 ) out vec4 FS_FragColor;

layout( location = 0 ) centroid noperspective in vec2 VS_TexCoord;

layout( binding = 0 ) uniform sampler2D Smp_Source;

void main() {
    // Adjust texture coordinates for dynamic resolution
    vec2 tc = AdjustTexCoord( VS_TexCoord );
    
    vec2 inverseSizeInPixels = InvViewportSize * DynamicResolutionRatio;

    FS_FragColor = FxaaPixelShader(
            // Use noperspective interpolation here (turn off perspective interpolation).
            // {xy} = center of pixel
            tc,
                        // Used only for FXAA Console, and not used on the 360 version.
                        // Use noperspective interpolation here (turn off perspective interpolation).
                        // {xy__} = upper left of pixel
                        // {__zw} = lower right of pixel
            FxaaFloat4( 0 ),
            // Input color texture.
            // {rgb_} = color in linear or perceptual color space
            // if (FXAA_GREEN_AS_LUMA == 0)
            //     {___a} = luma in perceptual color space (not linear)
            Smp_Source,
            Smp_Source,   // Only used on the optimized 360 version of FXAA Console.
            Smp_Source,   // Only used on the optimized 360 version of FXAA Console.
                          // Only used on FXAA Quality.
                          // This must be from a constant/uniform.
                          // {x_} = 1.0/screenWidthInPixels
                          // {_y} = 1.0/screenHeightInPixels
            inverseSizeInPixels,
            FxaaFloat4( 0 ), // Only used on FXAA Console.
                             // Only used on FXAA Console.
                             // Not used on 360, but used on PS3 and PC.
                             // This must be from a constant/uniform.
                             // {x___} = -2.0/screenWidthInPixels  
                             // {_y__} = -2.0/screenHeightInPixels
                             // {__z_} =  2.0/screenWidthInPixels  
                             // {___w} =  2.0/screenHeightInPixels 
            FxaaFloat4( -2, -2, 2, 2 ) * inverseSizeInPixels.xyxy,
            FxaaFloat4( 0 ), // Only used on FXAA Console.
                             // Only used on FXAA Quality.
                             // This used to be the FXAA_QUALITY__SUBPIX define.
                             // It is here now to allow easier tuning.
                             // Choose the amount of sub-pixel aliasing removal.
                             // This can effect sharpness.
                             //   1.00 - upper limit (softer)
                             //   0.75 - default amount of filtering
                             //   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
                             //   0.25 - almost off
                             //   0.00 - completely off
            0.75,
            // Only used on FXAA Quality.
            // This used to be the FXAA_QUALITY__EDGE_THRESHOLD define.
            // It is here now to allow easier tuning.
            // The minimum amount of local contrast required to apply algorithm.
            //   0.333 - too little (faster)
            //   0.250 - low quality
            //   0.166 - default
            //   0.125 - high quality 
            //   0.063 - overkill (slower)
            0.125,//0.166,
                  // Only used on FXAA Quality.
                  // This used to be the FXAA_QUALITY__EDGE_THRESHOLD_MIN define.
                  // It is here now to allow easier tuning.
                  // Trims the algorithm from processing darks.
                  //   0.0833 - upper limit (default, the start of visible unfiltered edges)
                  //   0.0625 - high quality (faster)
                  //   0.0312 - visible limit (slower)
                  // Special notes when using FXAA_GREEN_AS_LUMA,
                  //   Likely want to set this to zero.
                  //   As colors that are mostly not-green
                  //   will appear very dark in the green channel!
                  //   Tune by looking at mostly non-green content,
                  //   then start at zero and increase until aliasing is a problem.
            0.0625,
            // Only used on FXAA Console.
            // This used to be the FXAA_CONSOLE__EDGE_SHARPNESS define.
            // It is here now to allow easier tuning.
            // This does not effect PS3, as this needs to be compiled in.
            //   Use FXAA_CONSOLE__PS3_EDGE_SHARPNESS for PS3.
            //   Due to the PS3 being ALU bound,
            //   there are only three safe values here: 2 and 4 and 8.
            //   These options use the shaders ability to a free *|/ by 2|4|8.
            // For all other platforms can be a non-power of two.
            //   8.0 is sharper (default!!!)
            //   4.0 is softer
            //   2.0 is really soft (good only for vector graphics inputs)
            8.0,
            // Only used on FXAA Console.
            // This used to be the FXAA_CONSOLE__EDGE_THRESHOLD define.
            // It is here now to allow easier tuning.
            // This does not effect PS3, as this needs to be compiled in.
            //   Use FXAA_CONSOLE__PS3_EDGE_THRESHOLD for PS3.
            //   Due to the PS3 being ALU bound,
            //   there are only two safe values here: 1/4 and 1/8.
            //   These options use the shaders ability to a free *|/ by 2|4|8.
            // The console setting has a different mapping than the quality setting.
            // Other platforms can use other values.
            //   0.125 leaves less aliasing, but is softer (default!!!)
            //   0.25 leaves more aliasing, and is sharper
            0.125,
            // Only used on FXAA Console.
            // This used to be the FXAA_CONSOLE__EDGE_THRESHOLD_MIN define.
            // It is here now to allow easier tuning.
            // Trims the algorithm from processing darks.
            // The console setting has a different mapping than the quality setting.
            // This only applies when FXAA_EARLY_EXIT is 1.
            // This does not apply to PS3, 
            // PS3 was simplified to avoid more shader instructions.
            //   0.06 - faster but more aliasing in darks
            //   0.05 - default
            //   0.04 - slower and less aliasing in darks
            // Special notes when using FXAA_GREEN_AS_LUMA,
            //   Likely want to set this to zero.
            //   As colors that are mostly not-green
            //   will appear very dark in the green channel!
            //   Tune by looking at mostly non-green content,
            //   then start at zero and increase until aliasing is a problem.
            0.05,
            // Extra constants for 360 FXAA Console only.
            // Use zeros or anything else for other platforms.
            // These must be in physical constant registers and NOT immedates.
            // Immedates will result in compiler un-optimizing.
            // {xyzw} = float4(1.0, -1.0, 0.25, -0.25)
            FxaaFloat4( 1.0, -1.0, 0.25, -0.25 )
    );
	
	//FS_FragColor = LinearToSRGB_Alpha(FS_FragColor);
}
