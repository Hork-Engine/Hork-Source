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
#include "OpenGL45FrameResources.h"
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

    CreateFullscreenQuadPipelineGS( PipelineLUT,
                                    "postprocess/colorgrading.vert",
                                    "postprocess/colorgrading.frag",
                                    "postprocess/colorgrading.geom",
                                    Pass,
                                    GHI::BLENDING_ALPHA );

    CreateFullscreenQuadPipelineGS( PipelineProcedural,
                                    "postprocess/colorgrading.vert",
                                    "postprocess/colorgrading_procedural.frag",
                                    "postprocess/colorgrading.geom",
                                    Pass,
                                    GHI::BLENDING_ALPHA );

    CreateSamplers();
}

void AColorGradingRenderer::Deinitialize() {
    Pass.Deinitialize();
    PipelineLUT.Deinitialize();
    PipelineProcedural.Deinitialize();
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
    
    if ( source ) {
        GFrameResources.TextureBindings[0].pTexture = GPUTextureHandle( source );
        GFrameResources.SamplerBindings[0].pSampler = ColorGradingSampler;

        Cmd.BindPipeline( &PipelineLUT );
    } else {
        Cmd.BindPipeline( &PipelineProcedural );
    }
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}

}
