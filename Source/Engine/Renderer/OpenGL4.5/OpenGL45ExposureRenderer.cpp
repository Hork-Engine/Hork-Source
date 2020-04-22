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

#include "OpenGL45ExposureRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"
#include "OpenGL45RenderBackend.h"

using namespace GHI;

namespace OpenGL45 {

AExposureRenderer GExposureRenderer;

void AExposureRenderer::Initialize() {
    RenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 1;

    AttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_LOAD;
    renderPassCI.pColorAttachments = &colorAttachment;

    AttachmentRef colorAttachmentRef0 = {};
    colorAttachmentRef0.Attachment = 0;

    SubpassInfo subpasses[1] = {};
    subpasses[0].NumColorAttachments = 1;
    subpasses[0].pColorAttachmentRefs = &colorAttachmentRef0;

    renderPassCI.NumSubpasses = AN_ARRAY_SIZE( subpasses );
    renderPassCI.pSubpasses = subpasses;

    LuminancePass.Initialize( renderPassCI );

    CreatePipelines();
    CreateSampler();
}

void AExposureRenderer::Deinitialize() {
    LuminancePass.Deinitialize();
    MakeLuminanceMapPipe.Deinitialize();
    SumLuminanceMapPipe.Deinitialize();
    DynamicExposurePipe.Deinitialize();
}

void AExposureRenderer::CreatePipelines() {
    RasterizerStateInfo rs;
    rs.SetDefaults();
    rs.CullMode = POLYGON_CULL_FRONT;
    rs.bScissorEnable = false;

    BlendingStateInfo bs;
    bs.SetDefaults();

    DepthStencilStateInfo ds;
    ds.SetDefaults();
    ds.bDepthEnable = false;
    ds.DepthWriteMask = DEPTH_WRITE_DISABLE;

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

    ShaderModule makeLuminanceMapVSModule;
    ShaderModule makeLuminanceMapFSModule;

    ShaderModule sumLuminanceMapVSModule;
    ShaderModule sumLuminanceMapFSModule;

    ShaderModule dynamicExposureFSModule;

    AString makeLuminanceMapVS = LoadShader( "postprocess/makeLuminanceMap.vert" );
    AString makeLuminanceMapFS = LoadShader( "postprocess/makeLuminanceMap.frag" );
    AString sumLuminanceMapVS = LoadShader( "postprocess/sumLuminanceMap.vert" );
    AString sumLuminanceMapFS = LoadShader( "postprocess/sumLuminanceMap.frag" );
    AString dynamicExposureFS = LoadShader( "postprocess/dynamicExposure.frag" );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( makeLuminanceMapVS.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &makeLuminanceMapVSModule );

    GShaderSources.Clear();
    GShaderSources.Add( makeLuminanceMapFS.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &makeLuminanceMapFSModule );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( sumLuminanceMapVS.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &sumLuminanceMapVSModule );

    GShaderSources.Clear();
    GShaderSources.Add( sumLuminanceMapFS.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &sumLuminanceMapFSModule );

    GShaderSources.Clear();
    GShaderSources.Add( dynamicExposureFS.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &dynamicExposureFSModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bs;
    pipelineCI.pRasterizer = &rs;
    pipelineCI.pDepthStencil = &ds;

    ShaderStageInfo makeLuminanceMapVSStage = {};
    makeLuminanceMapVSStage.Stage = SHADER_STAGE_VERTEX_BIT;
    makeLuminanceMapVSStage.pModule = &makeLuminanceMapVSModule;

    ShaderStageInfo makeLuminanceMapFSStage = {};
    makeLuminanceMapFSStage.Stage = SHADER_STAGE_FRAGMENT_BIT;
    makeLuminanceMapFSStage.pModule = &makeLuminanceMapFSModule;

    ShaderStageInfo makeLuminanceMapStages[2] = { makeLuminanceMapVSStage, makeLuminanceMapFSStage };

    ShaderStageInfo sumLuminanceMapVSStage = {};
    sumLuminanceMapVSStage.Stage = SHADER_STAGE_VERTEX_BIT;
    sumLuminanceMapVSStage.pModule = &sumLuminanceMapVSModule;

    ShaderStageInfo sumLuminanceMapFSStage = {};
    sumLuminanceMapFSStage.Stage = SHADER_STAGE_FRAGMENT_BIT;
    sumLuminanceMapFSStage.pModule = &sumLuminanceMapFSModule;

    ShaderStageInfo sumLuminanceMapStages[2] = { sumLuminanceMapVSStage, sumLuminanceMapFSStage };

    ShaderStageInfo dynamicExposureFSStage = {};
    dynamicExposureFSStage.Stage = SHADER_STAGE_FRAGMENT_BIT;
    dynamicExposureFSStage.pModule = &dynamicExposureFSModule;

    ShaderStageInfo dynamicExposureStages[2] = { sumLuminanceMapVSStage, dynamicExposureFSStage };

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.pRenderPass = &LuminancePass;
    pipelineCI.Subpass = 0;
    
    pipelineCI.NumStages = AN_ARRAY_SIZE( makeLuminanceMapStages );
    pipelineCI.pStages = makeLuminanceMapStages;
    MakeLuminanceMapPipe.Initialize( pipelineCI );

    pipelineCI.NumStages = AN_ARRAY_SIZE( sumLuminanceMapStages );
    pipelineCI.pStages = sumLuminanceMapStages;
    SumLuminanceMapPipe.Initialize( pipelineCI );

    bs.RenderTargetSlots[0].SetBlendingPreset( GHI::BLENDING_ALPHA );
    pipelineCI.NumStages = AN_ARRAY_SIZE( dynamicExposureStages );
    pipelineCI.pStages = dynamicExposureStages;
    DynamicExposurePipe.Initialize( pipelineCI );
}

void AExposureRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    samplerCI.Filter = FILTER_LINEAR;
    LuminanceSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void AExposureRenderer::Render( GHI::Texture & _SrcTexture ) {
    ATextureGPU * exposureTexture = GRenderView->CurrentExposure;

    if ( !exposureTexture ) {
        return;
    }

    DrawCmd drawCmd;
    drawCmd.VertexCountPerInstance = 4;
    drawCmd.InstanceCount = 1;
    drawCmd.StartVertexLocation = 0;
    drawCmd.StartInstanceLocation = 0;

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = 0;
    vp.Height = 0;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;

    GFrameResources.TextureBindings[0].pTexture = &_SrcTexture;
    GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &LuminancePass;

    // Make luminance map 64x64
    renderPassBegin.pFramebuffer = &GRenderTarget.FramebufferLum64;
    renderPassBegin.RenderArea.Width = vp.Width = 64;
    renderPassBegin.RenderArea.Height = vp.Height = 64;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &MakeLuminanceMapPipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();

    // Downscale luminance to 32x32
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.Luminance64;
    renderPassBegin.pFramebuffer = &GRenderTarget.FramebufferLum32;
    renderPassBegin.RenderArea.Width = vp.Width = 32;
    renderPassBegin.RenderArea.Height = vp.Height = 32;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &SumLuminanceMapPipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();
    
    // Downscale luminance to 16x16
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.Luminance32;
    renderPassBegin.pFramebuffer = &GRenderTarget.FramebufferLum16;
    renderPassBegin.RenderArea.Width = vp.Width = 16;
    renderPassBegin.RenderArea.Height = vp.Height = 16;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &SumLuminanceMapPipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();

    // Downscale luminance to 8x8
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.Luminance16;
    renderPassBegin.pFramebuffer = &GRenderTarget.FramebufferLum8;
    renderPassBegin.RenderArea.Width = vp.Width = 8;
    renderPassBegin.RenderArea.Height = vp.Height = 8;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &SumLuminanceMapPipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();

    // Downscale luminance to 4x4
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.Luminance8;
    renderPassBegin.pFramebuffer = &GRenderTarget.FramebufferLum4;
    renderPassBegin.RenderArea.Width = vp.Width = 4;
    renderPassBegin.RenderArea.Height = vp.Height = 4;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &SumLuminanceMapPipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();

    // Downscale luminance to 2x2
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.Luminance4;
    renderPassBegin.pFramebuffer = &GRenderTarget.FramebufferLum2;
    renderPassBegin.RenderArea.Width = vp.Width = 2;
    renderPassBegin.RenderArea.Height = vp.Height = 2;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &SumLuminanceMapPipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();

    //
    // Render final exposure
    //

    GHI::Framebuffer * framebuffer;
    if ( !exposureTexture->pFramebuffer ) {
        exposureTexture->pFramebuffer = GZoneMemory.Alloc( sizeof( GHI::Framebuffer ) );
        framebuffer = new (exposureTexture->pFramebuffer) GHI::Framebuffer;

        FramebufferAttachmentInfo colorAttachment = {};
        colorAttachment.bLayered = false;
        colorAttachment.LayerNum = 0;
        colorAttachment.LodNum = 0;
        colorAttachment.pTexture = GPUTextureHandle( exposureTexture );
        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.NumColorAttachments = 1;
        framebufferCI.pColorAttachments = &colorAttachment;
        framebufferCI.Width = 1;
        framebufferCI.Height = 1;
        framebuffer->Initialize( framebufferCI );
    } else {
        framebuffer = static_cast< GHI::Framebuffer * >(exposureTexture->pFramebuffer);
    }

    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.Luminance2;
    GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.RenderArea.Width = vp.Width = 1;
    renderPassBegin.RenderArea.Height = vp.Height = 1;
    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.SetViewport( vp );
    Cmd.BindPipeline( &DynamicExposurePipe );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );
    Cmd.EndRenderPass();
}

}
