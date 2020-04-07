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

#include "OpenGL45FxaaPassRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"

#include <Core/Public/CriticalError.h>

using namespace GHI;

namespace OpenGL45 {

AFxaaPassRenderer GFxaaPassRenderer;

void AFxaaPassRenderer::Initialize() {
    RenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 1;

    AttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassCI.pColorAttachments = &colorAttachment;
    renderPassCI.pDepthStencilAttachment = NULL;

    AttachmentRef colorAttachmentRef = {};
    colorAttachmentRef.Attachment = 0;

    SubpassInfo subpass = {};
    subpass.NumColorAttachments = 1;
    subpass.pColorAttachmentRefs = &colorAttachmentRef;

    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpass;

    FxaaPass.Initialize( renderPassCI );

    CreatePipeline();
    CreateSampler();
}

void AFxaaPassRenderer::Deinitialize() {
    FxaaPass.Deinitialize();
    FxaaPipeline.Deinitialize();
}

void AFxaaPassRenderer::CreatePipeline() {
    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;//true;

    BlendingStateInfo bsd;
    bsd.SetDefaults();

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();

    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const VertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            0
        }
    };

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    ShaderModule vertexShaderModule, fragmentShaderModule;

    const char * vertexSourceCode =
            "out gl_PerVertex\n"
            "{\n"
            "    vec4 gl_Position;\n"
            "};\n"
            "layout( location = 0 ) centroid noperspective out vec2 VS_TexCoord;\n"
            "void main() {\n"
            "  gl_Position = vec4( InPosition, 0.0, 1.0 );\n"
            "  VS_TexCoord = InPosition * 0.5 + 0.5;\n"
            "  VS_TexCoord *= Timers.zw;\n"
            "  VS_TexCoord.y = 1.0 - VS_TexCoord.y;\n"
            "}\n";

    const char * fragmentShader = AN_STRINGIFY(

    //layout( origin_upper_left ) in vec4 gl_FragCoord;

    layout( location = 0 ) centroid noperspective in vec2 VS_TexCoord;
    layout( location = 0 ) out vec4 FS_FragColor;

    layout( binding = 0 ) uniform sampler2D imageTexture;

    void main() {
        FS_FragColor = FxaaPixelShader(
            // Use noperspective interpolation here (turn off perspective interpolation).
            // {xy} = center of pixel
            VS_TexCoord,
                        // Used only for FXAA Console, and not used on the 360 version.
                        // Use noperspective interpolation here (turn off perspective interpolation).
                        // {xy__} = upper left of pixel
                        // {__zw} = lower right of pixel
            FxaaFloat4( 0 ),
            // Input color texture.
            // {rgb_} = color in linear or perceptual color space
            // if (FXAA_GREEN_AS_LUMA == 0)
            //     {___a} = luma in perceptual color space (not linear)
            imageTexture,
            imageTexture, // Only used on the optimized 360 version of FXAA Console.
            imageTexture, // Only used on the optimized 360 version of FXAA Console.
                          // Only used on FXAA Quality.
                          // This must be from a constant/uniform.
                          // {x_} = 1.0/screenWidthInPixels
                          // {_y} = 1.0/screenHeightInPixels
            ViewportParams.xy*Timers.zw,
            FxaaFloat4( 0 ), // Only used on FXAA Console.
                             // Only used on FXAA Console.
                             // Not used on 360, but used on PS3 and PC.
                             // This must be from a constant/uniform.
                             // {x___} = -2.0/screenWidthInPixels  
                             // {_y__} = -2.0/screenHeightInPixels
                             // {__z_} =  2.0/screenWidthInPixels  
                             // {___w} =  2.0/screenHeightInPixels 
            FxaaFloat4( -2, -2, 2, 2 ) * ViewportParams.xyxy*Timers.zwzw,
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
    }

    );
    const char * fxaaPredefines =
        "#define FXAA_PC 1\n"
        "#define FXAA_GLSL_130 1\n"
        //"#define FXAA_QUALITY__PRESET 12\n"
        "#define FXAA_QUALITY__PRESET 39\n"
        "#define FXAA_GATHER4_ALPHA 1\n";

    AFileStream f;
    if ( !f.OpenRead( "FXAA_3_11.h" ) ) {
        CriticalError( "Couldn't open FXAA_3_11.h\n" );
    }

    AString fxaaSource;
    fxaaSource.FromFile( f );
    fxaaSource = fxaaPredefines + fxaaSource + fragmentShader;

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( fxaaSource.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[2] = { vs, fs };

    pipelineCI.NumStages = AN_ARRAY_SIZE( stages );
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.pRenderPass = &FxaaPass;
    pipelineCI.Subpass = 0;

    pipelineCI.pBlending = &bsd;
    FxaaPipeline.Initialize( pipelineCI );
}

void AFxaaPassRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    FxaaSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void AFxaaPassRenderer::Render() {
    // Begin render pass
    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &FxaaPass;
    renderPassBegin.pFramebuffer = &GRenderTarget.GetFxaaFramebuffer();
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width = GRenderView->Width;
    renderPassBegin.RenderArea.Height = GRenderView->Height;
    renderPassBegin.pColorClearValues = NULL;
    renderPassBegin.pDepthStencilClearValue = NULL;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = GRenderView->Width;
    vp.Height = GRenderView->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawCmd drawCmd;
    drawCmd.VertexCountPerInstance = 4;
    drawCmd.InstanceCount = 1;
    drawCmd.StartVertexLocation = 0;
    drawCmd.StartInstanceLocation = 0;
    
    Cmd.BindPipeline( &FxaaPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );

    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.GetPostprocessTexture();
    GFrameResources.SamplerBindings[0].pSampler = FxaaSampler;

#if 0
    Rect2D scissorRect;
    scissorRect.X = cmd->ClipMins.X;
    scissorRect.Y = cmd->ClipMins.Y;
    scissorRect.Width = cmd->ClipMaxs.X - cmd->ClipMins.X;
    scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
    Cmd.SetScissor( scissorRect );
#endif

    Cmd.BindShaderResources( &GFrameResources.Resources );

    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}

}
