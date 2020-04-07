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

#include "OpenGL45PostprocessPassRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"

#include <Core/Public/CriticalError.h>

using namespace GHI;

namespace OpenGL45 {

APostprocessPassRenderer GPostprocessPassRenderer;

void APostprocessPassRenderer::Initialize() {
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

    PostprocessPass.Initialize( renderPassCI );

    CreatePipeline();
    CreateSampler();
}

void APostprocessPassRenderer::Deinitialize() {
    PostprocessPass.Deinitialize();
    PostprocessPipeline.Deinitialize();
}

void APostprocessPassRenderer::CreatePipeline() {
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
        "layout( location = 0 ) noperspective out vec2 VS_TexCoord;\n"
        "void main() {\n"
        "  gl_Position = vec4( InPosition, 0.0, 1.0 );\n"
        "  VS_TexCoord = InPosition * 0.5 + 0.5;\n"
        "  VS_TexCoord *= Timers.zw;\n"
        "  VS_TexCoord.y = 1.0 - VS_TexCoord.y;\n"
        "}\n";

    const char * tonamappingSource = AN_STRINGIFY(
    // Convert linear RGB into a CIE xyY (xy = chroma, Y = luminance).
    vec3 RGB2xyY( in vec3 rgb ) {
        const mat3 RGB2XYZ = mat3
        (
            0.4124, 0.3576, 0.1805,
            0.2126, 0.7152, 0.0722,
            0.0193, 0.1192, 0.9505
        );

        vec3 XYZ = RGB2XYZ * rgb;

        // XYZ to xyY
        return vec3( XYZ.xy / max( XYZ.x + XYZ.y + XYZ.z, 1e-10 ), XYZ.y );
    }

    // Convert a CIE xyY value into linear RGB.
    vec3 xyY2RGB( in vec3 xyY ) {
        const mat3 XYZ2RGB = mat3
        (
            3.2406, -1.5372, -0.4986,
            -0.9689, 1.8758, 0.0415,
            0.0557, -0.2040, 1.0570
        );

        // xyY to XYZ
        float z_div_y = xyY.z / max( xyY.y, 1e-10 );
        return XYZ2RGB * vec3( z_div_y * xyY.x, xyY.z, z_div_y * (1.0 - xyY.x - xyY.y) );
    }

    vec3 ToneReinhard( in vec3 Color, in float Average, in float Exposure, in float WhitePoint ) {
        vec3 xyY = RGB2xyY( Color );

        float Lp = xyY.z * Exposure / Average;
        xyY.z = Lp * (1.0f + Lp / (WhitePoint * WhitePoint)) / (1.0f + Lp);

        return xyY2RGB( xyY );
    }
    );

    const char * fragmentSourceCode =
        "layout( location = 0 ) noperspective in vec2 VS_TexCoord;\n"
        "layout( location = 0 ) out vec4 FS_FragColor;\n"

        "layout( binding = 0 ) uniform sampler2D imageTexture;\n"

        "vec4 ENCODE_SRGB( in vec4 LinearValue ) {\n"
        "#ifdef SRGB_GAMMA_APPROX\n"
        "  return pow( LinearValue, vec4( 1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2, 1.0 ) );\n"
        "#else\n"
        "  const vec4 Shift = vec4( -0.055, -0.055, -0.055, 0.0 );\n"
        "  const vec4 Scale = vec4( 1.055, 1.055, 1.055, 1.0 );\n"
        "  const vec4 Pow = vec4( 1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4, 1.0 );\n"
        "  const vec4 Scale2 = vec4( 12.92, 12.92, 12.92, 1.0 );\n"
        "  return mix( Scale * pow( LinearValue, Pow ) + Shift, LinearValue * Scale2, step( LinearValue, vec4( 0.0031308 ) ) );\n"
        "#endif\n"
        "}\n"

        "vec3 ENCODE_SRGB( in vec3 LinearValue ) {\n"
        "#ifdef SRGB_GAMMA_APPROX\n"
        "  return pow( LinearValue, vec3( 1.0 / 2.2 ) );\n"
        "#else\n"
        "  return mix( 1.055 * pow( LinearValue, vec3( 1.0 / 2.4 ) ) - 0.055, LinearValue * 12.92, step( LinearValue, vec3( 0.0031308 ) ) );\n"
        "#endif\n"
        "}\n"

        "float ENCODE_SRGB( in float LinearValue ) {\n"
        "#ifdef SRGB_GAMMA_APPROX\n"
        "  return pow( LinearValue, 1.0/2.2 );\n"
        "#else\n"
        "  return mix( 1.055 * pow( LinearValue, 1.0 / 2.4 ) - 0.055, LinearValue * 12.92, step( LinearValue, 0.0031308 ) );\n"
        "#endif\n"
        "}\n"

        "void main() {\n"
        "  FS_FragColor = texture( imageTexture, VS_TexCoord );\n"
        //"  FS_FragColor = texelFetch( imageTexture, ivec2(gl_FragCoord.xy), 0 );\n"
//        "  if ( VS_TexCoord.x > 0.5 )\n"
//        "    FS_FragColor.rgb = ToneReinhard( FS_FragColor.rgb, 2.0, 0.7, 2.0 );\n"

        "  const vec3 RGB_TO_GRAYSCALE = vec3( 0.2125, 0.7154, 0.0721 );\n"
        "  FS_FragColor.a = ENCODE_SRGB( clamp( dot( FS_FragColor.rgb, RGB_TO_GRAYSCALE ), 0.0, 1.0 ) );\n"
        "}\n";

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( tonamappingSource );
    GShaderSources.Add( fragmentSourceCode );
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

    pipelineCI.pRenderPass = &PostprocessPass;
    pipelineCI.Subpass = 0;

    pipelineCI.pBlending = &bsd;
    PostprocessPipeline.Initialize( pipelineCI );
}

void APostprocessPassRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    PostprocessSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void APostprocessPassRenderer::Render() {
    // Begin render pass
    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &PostprocessPass;
    renderPassBegin.pFramebuffer = &GRenderTarget.GetPostprocessFramebuffer();
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
    
    Cmd.BindPipeline( &PostprocessPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );

    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.GetFramebufferTexture();
    GFrameResources.SamplerBindings[0].pSampler = PostprocessSampler;

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
