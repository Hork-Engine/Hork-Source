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

#include "OpenGL45BrightPassRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"
#include "OpenGL45RenderBackend.h"

using namespace GHI;

namespace OpenGL45 {

ABrightPassRenderer GBrightPassRenderer;

void ABrightPassRenderer::Initialize() {
    RenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 1;

    AttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassCI.pColorAttachments = &colorAttachment;

    AttachmentRef colorAttachmentRef0 = {};
    colorAttachmentRef0.Attachment = 0;

    AttachmentRef colorAttachmentRef1 = {};
    colorAttachmentRef1.Attachment = 1;

    SubpassInfo subpasses[2] = {};
    subpasses[0].NumColorAttachments = 1;
    subpasses[0].pColorAttachmentRefs = &colorAttachmentRef0;

    subpasses[1].NumColorAttachments = 1;
    subpasses[1].pColorAttachmentRefs = &colorAttachmentRef1;

    renderPassCI.NumSubpasses = AN_ARRAY_SIZE( subpasses );
    renderPassCI.pSubpasses = subpasses;

    BrightPass.Initialize( renderPassCI );

    CreateBrightPipeline();
    CreateBlurPipeline();
    CreateCopyPipeline();
    CreateSampler();
}

void ABrightPassRenderer::Deinitialize() {
    BrightPass.Deinitialize();
    BrightPipeline.Deinitialize();
    BlurPipeline0.Deinitialize();
    BlurPipeline1.Deinitialize();
    BlurFragmentShaderModule.Deinitialize();
    CopyPipeline.Deinitialize();
}

void ABrightPassRenderer::CreateBrightPipeline() {
    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

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

    AString vertexSourceCode = LoadShader( "postprocess/brightpass.vert" );
    AString fragmentSourceCode = LoadShader( "postprocess/brightpass.frag" );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentSourceCode.CStr() );
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

    pipelineCI.pRenderPass = &BrightPass;
    pipelineCI.Subpass = 0;

    pipelineCI.pBlending = &bsd;
    BrightPipeline.Initialize( pipelineCI );
}

void ABrightPassRenderer::CreateBlurPipeline() {
    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

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

    ShaderModule vertexShaderModule;

    AString vertexSourceCode = LoadShader( "postprocess/gauss.vert" );
    AString gaussFragmentShaderSource = LoadShader( "postprocess/gauss.frag" );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( gaussFragmentShaderSource.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &BlurFragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &BlurFragmentShaderModule;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.pRenderPass = &BrightPass;

    ShaderStageInfo stages[2] = { vs, fs };
    pipelineCI.NumStages = AN_ARRAY_SIZE( stages );
    pipelineCI.pStages = stages;

    pipelineCI.Subpass = 1;
    BlurPipeline1.Initialize( pipelineCI );

    pipelineCI.Subpass = 0;
    BlurPipeline0.Initialize( pipelineCI );
}

void ABrightPassRenderer::CreateCopyPipeline() {
    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

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

    ShaderModule vertexShaderModule;
    ShaderModule fragmentShaderModule;

    AString vertexSourceCode = LoadShader( "postprocess/copy.vert" );
    AString fragmentShaderSource = LoadShader( "postprocess/copy.frag" );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentShaderSource.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.pRenderPass = &BrightPass;

    ShaderStageInfo stages[2] = { vs, fs };
    pipelineCI.NumStages = AN_ARRAY_SIZE( stages );
    pipelineCI.pStages = stages;

    pipelineCI.Subpass = 0;

    CopyPipeline.Initialize( pipelineCI );
}

void ABrightPassRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    samplerCI.Filter = FILTER_LINEAR;
    LinearSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void ABrightPassRenderer::Render( GHI::Texture & _SrcTexture ) {
    SBloomTexture & bloom = GRenderTarget.GetBloomTexture();

    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &BrightPass;
    renderPassBegin.pFramebuffer = &bloom.Framebuffer;
    renderPassBegin.RenderArea.Width = bloom.Width;
    renderPassBegin.RenderArea.Height = bloom.Height;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = bloom.Width;
    vp.Height = bloom.Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawCmd drawCmd;
    drawCmd.VertexCountPerInstance = 4;
    drawCmd.InstanceCount = 1;
    drawCmd.StartVertexLocation = 0;
    drawCmd.StartInstanceLocation = 0;

    GFrameResources.SamplerBindings[0].pSampler = LinearSampler;

    // Make bright texture. Result in Texture[0]
    GFrameResources.TextureBindings[0].pTexture = &_SrcTexture;
    Cmd.BindPipeline( &BrightPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    //
    // Perform gaussian blur
    //

    // X pass. Result in Texture[1]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Texture[0];
    BlurFragmentShaderModule.SetUniform2f( 0, 1.0f / vp.Width, 0.0f );
    Cmd.BindPipeline( &BlurPipeline1 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // Y pass. Result in Texture[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Texture[1];
    BlurFragmentShaderModule.SetUniform2f( 0, 0.0f, 1.0f / vp.Height );
    Cmd.BindPipeline( &BlurPipeline0 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();

    renderPassBegin.pRenderPass = &BrightPass;
    renderPassBegin.pFramebuffer = &bloom.Framebuffer_2;
    renderPassBegin.RenderArea.Width = bloom.Width>>2;
    renderPassBegin.RenderArea.Height = bloom.Height>>2;

    Cmd.BeginRenderPass( renderPassBegin );

    vp.Width = bloom.Width>>2;
    vp.Height = bloom.Height>>2;
    Cmd.SetViewport( vp );

    // Copy Texture[0] to Textures_2[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Texture[0];
    Cmd.BindPipeline( &CopyPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // X pass. Result in Textures_2[1]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_2[0];
    BlurFragmentShaderModule.SetUniform2f( 0, 1.0f / vp.Width, 0.0f );
    Cmd.BindPipeline( &BlurPipeline1 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // Y pass. Result in Textures_2[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_2[1];
    BlurFragmentShaderModule.SetUniform2f( 0, 0.0f, 1.0f / vp.Height );
    Cmd.BindPipeline( &BlurPipeline0 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();


    renderPassBegin.pRenderPass = &BrightPass;
    renderPassBegin.pFramebuffer = &bloom.Framebuffer_4;
    renderPassBegin.RenderArea.Width = bloom.Width>>4;
    renderPassBegin.RenderArea.Height = bloom.Height>>4;

    Cmd.BeginRenderPass( renderPassBegin );

    vp.Width = bloom.Width>>4;
    vp.Height = bloom.Height>>4;
    Cmd.SetViewport( vp );

    // Copy Textures_2[0] to Textures_4[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_2[0];
    Cmd.BindPipeline( &CopyPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // X pass. Result in bloom.Textures_4[1]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_4[0];
    BlurFragmentShaderModule.SetUniform2f( 0, 1.0f / vp.Width, 0.0f );
    Cmd.BindPipeline( &BlurPipeline1 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // Y pass. Result in bloom.Textures_4[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_4[1];
    BlurFragmentShaderModule.SetUniform2f( 0, 0.0f, 1.0f / vp.Height );
    Cmd.BindPipeline( &BlurPipeline0 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();


    renderPassBegin.pRenderPass = &BrightPass;
    renderPassBegin.pFramebuffer = &bloom.Framebuffer_6;
    renderPassBegin.RenderArea.Width = bloom.Width>>6;
    renderPassBegin.RenderArea.Height = bloom.Height>>6;

    Cmd.BeginRenderPass( renderPassBegin );

    vp.Width = bloom.Width>>6;
    vp.Height = bloom.Height>>6;
    Cmd.SetViewport( vp );

    // Copy Textures_4[0] to Textures_6[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_4[0];
    Cmd.BindPipeline( &CopyPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // X pass. Result in bloom.Textures_6[1]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_6[0];
    BlurFragmentShaderModule.SetUniform2f( 0, 1.0f / vp.Width, 0.0f );
    Cmd.BindPipeline( &BlurPipeline1 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    // Y pass. Result in bloom.Textures_6[0]
    GFrameResources.TextureBindings[0].pTexture = &bloom.Textures_6[1];
    BlurFragmentShaderModule.SetUniform2f( 0, 0.0f, 1.0f / vp.Height );
    Cmd.BindPipeline( &BlurPipeline0 );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();

    //GLogger.Printf( "Bloom size %dx%d %dx%d %dx%d %dx%d\n",
    //                bloom.Width,bloom.Height,
    //                bloom.Width>>2,bloom.Height>>2,
    //                bloom.Width>>4,bloom.Height>>4,
    //                bloom.Width>>6,bloom.Height>>6
    //                );
}

}
