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

#include "OpenGL45ColorGradingRenderer.h"
#include "OpenGL45ShaderSource.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"
#include "OpenGL45RenderBackend.h"

using namespace GHI;

namespace OpenGL45 {

AColorGradingRenderer GColorGradingRenderer;

void AColorGradingRenderer::Initialize() {
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

    Pass.Initialize( renderPassCI );

    CreatePipeline();
    CreateSamplers();
}

void AColorGradingRenderer::Deinitialize() {
    Pass.Deinitialize();
    Pipeline.Deinitialize();
}

void AColorGradingRenderer::CreatePipeline() {
    RasterizerStateInfo rs;
    rs.SetDefaults();
    rs.CullMode = POLYGON_CULL_FRONT;
    rs.bScissorEnable = false;

    BlendingStateInfo bs;
    bs.SetDefaults();
    bs.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

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

    ShaderModule vertexShaderModule, geometryShaderModule, fragmentShaderModule;

    AString vertexSourceCode = LoadShader( "postprocess/colorgrading.vert" );
    AString geometrySourceCode = LoadShader( "postprocess/colorgrading.geom" );
    AString fragmentSourceCode = LoadShader( "postprocess/colorgrading.frag" );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( geometrySourceCode.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, &geometryShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentSourceCode.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pRasterizer = &rs;
    pipelineCI.pDepthStencil = &ds;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo gs = {};
    gs.Stage = SHADER_STAGE_GEOMETRY_BIT;
    gs.pModule = &geometryShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[] = { vs, gs, fs };

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

    pipelineCI.pRenderPass = &Pass;
    pipelineCI.Subpass = 0;

    pipelineCI.pBlending = &bs;
    Pipeline.Initialize( pipelineCI );
}

void AColorGradingRenderer::CreateSamplers() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();

    samplerCI.Filter = FILTER_NEAREST;// FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    ColorGradingSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void AColorGradingRenderer::Render() {
    ATextureGPU * texture = GRenderView->CurrentColorGradingLUT;
    ATextureGPU * source = GRenderView->ColorGradingLUT;

    if ( !texture ) {
        return;
    }

    if ( !source ) {
        return;
    }

    GHI::Framebuffer * framebuffer;
    if ( !texture->pFramebuffer ) {
        texture->pFramebuffer = GZoneMemory.Alloc( sizeof( GHI::Framebuffer ) );
        framebuffer = new ( texture->pFramebuffer ) GHI::Framebuffer;

        FramebufferAttachmentInfo colorAttachment = {};
        colorAttachment.bLayered = false;
        colorAttachment.LayerNum = 0;
        colorAttachment.LodNum = 0;
        colorAttachment.pTexture = GPUTextureHandle( texture );
        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.NumColorAttachments = 1;
        framebufferCI.pColorAttachments = &colorAttachment;
        framebufferCI.Width = 16;
        framebufferCI.Height = 16;        
        framebuffer->Initialize( framebufferCI );
    } else {
        framebuffer = static_cast< GHI::Framebuffer * >( texture->pFramebuffer );
    }

    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &Pass;
    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.RenderArea.Width = 16;
    renderPassBegin.RenderArea.Height = 16;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = 16;
    vp.Height = 16;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawCmd drawCmd;
    drawCmd.VertexCountPerInstance = 4;
    drawCmd.InstanceCount = 1;
    drawCmd.StartVertexLocation = 0;
    drawCmd.StartInstanceLocation = 0;
    
    GFrameResources.TextureBindings[0].pTexture = GPUTextureHandle( source );
    GFrameResources.SamplerBindings[0].pSampler = ColorGradingSampler;

    Cmd.BindPipeline( &Pipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}

}
