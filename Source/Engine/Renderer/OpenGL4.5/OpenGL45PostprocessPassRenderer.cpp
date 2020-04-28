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
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45RenderBackend.h"
#include "OpenGL45ExposureRenderer.h"

ARuntimeVariable RVShowDefaultExposure( _CTS( "ShowDefaultExposure" ), _CTS( "0" ) );

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

    CreateFullscreenQuadPipeline( PostprocessPipeline, "postprocess/final.vert", "postprocess/final.frag", PostprocessPass );

    CreateSampler();
}

void APostprocessPassRenderer::Deinitialize() {
    PostprocessPass.Deinitialize();
    PostprocessPipeline.Deinitialize();
}

void APostprocessPassRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    PostprocessSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    BloomSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_NEAREST;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    LuminanceSampler = GDevice.GetOrCreateSampler( samplerCI );

    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    ColorGradingSampler = GDevice.GetOrCreateSampler( samplerCI );
}

void APostprocessPassRenderer::Render( GHI::Framebuffer & TargetFB ) {
    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &PostprocessPass;
    renderPassBegin.pFramebuffer = &TargetFB;
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
    
    GFrameResources.TextureBindings[0].pTexture = &GRenderTarget.GetFramebufferTexture();
    GFrameResources.SamplerBindings[0].pSampler = PostprocessSampler;

    if ( GRenderView->CurrentColorGradingLUT ) {
        GFrameResources.TextureBindings[1].pTexture = GPUTextureHandle( GRenderView->CurrentColorGradingLUT );
        GFrameResources.SamplerBindings[1].pSampler = ColorGradingSampler;
    }

    GFrameResources.TextureBindings[2].pTexture = &GRenderTarget.GetBloomTexture().Textures[0];
    GFrameResources.SamplerBindings[2].pSampler = BloomSampler;

    GFrameResources.TextureBindings[3].pTexture = &GRenderTarget.GetBloomTexture().Textures_2[0];
    GFrameResources.SamplerBindings[3].pSampler = BloomSampler;

    GFrameResources.TextureBindings[4].pTexture = &GRenderTarget.GetBloomTexture().Textures_4[0];
    GFrameResources.SamplerBindings[4].pSampler = BloomSampler;

    GFrameResources.TextureBindings[5].pTexture = &GRenderTarget.GetBloomTexture().Textures_6[0];
    GFrameResources.SamplerBindings[5].pSampler = BloomSampler;

    if ( GRenderView->CurrentExposure && !RVShowDefaultExposure ) {
        GFrameResources.TextureBindings[6].pTexture = GPUTextureHandle( GRenderView->CurrentExposure );
        GFrameResources.SamplerBindings[6].pSampler = LuminanceSampler; // whatever (used texelFetch)
    } else {
        // TODO: set default exposure?
        GFrameResources.TextureBindings[6].pTexture = GExposureRenderer.GetDefaultLuminance();
        GFrameResources.SamplerBindings[6].pSampler = LuminanceSampler; // whatever (used texelFetch)
    }
    
    Cmd.BindPipeline( &PostprocessPipeline );
    Cmd.BindVertexBuffer( 0, &GFrameResources.Saq, 0 );
    Cmd.BindIndexBuffer( NULL, INDEX_TYPE_UINT16, 0 );
    Cmd.BindShaderResources( &GFrameResources.Resources );
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}

}
