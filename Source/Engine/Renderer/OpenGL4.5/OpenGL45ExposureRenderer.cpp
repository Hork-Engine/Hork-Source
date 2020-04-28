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
#include "OpenGL45FrameResources.h"

using namespace GHI;

namespace OpenGL45 {

AExposureRenderer GExposureRenderer;

void AExposureRenderer::Initialize() {
    // Create render pass
    {
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
    }

    // Create textures
    {
        TextureStorageCreateInfo texStorageCI = {};
        texStorageCI.Type = GHI::TEXTURE_2D;
        texStorageCI.NumLods = 1;
        texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG16F;

        texStorageCI.Resolution.Tex2D.Width = 64;
        texStorageCI.Resolution.Tex2D.Height = 64;
        Luminance64.InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = 32;
        texStorageCI.Resolution.Tex2D.Height = 32;
        Luminance32.InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = 16;
        texStorageCI.Resolution.Tex2D.Height = 16;
        Luminance16.InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = 8;
        texStorageCI.Resolution.Tex2D.Height = 8;
        Luminance8.InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = 4;
        texStorageCI.Resolution.Tex2D.Height = 4;
        Luminance4.InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = 2;
        texStorageCI.Resolution.Tex2D.Height = 2;
        Luminance2.InitializeStorage( texStorageCI );

        byte defaultLum[2] = { 30, 30 }; // TODO: choose appropriate value
        texStorageCI.Resolution.Tex2D.Width = 1;
        texStorageCI.Resolution.Tex2D.Height = 1;
        texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG8;
        DefaultLuminance.InitializeStorage( texStorageCI );
        DefaultLuminance.Write( 0, GHI::PIXEL_FORMAT_UBYTE_RG, sizeof( defaultLum ), 1, defaultLum );
    }

    // Create framebuffers
    {
        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.NumColorAttachments = 1;
        FramebufferAttachmentInfo colorAttachment = {};
        colorAttachment.bLayered = false;
        colorAttachment.LayerNum = 0;
        colorAttachment.LodNum = 0;
        framebufferCI.pColorAttachments = &colorAttachment;

        framebufferCI.Width = 64;
        framebufferCI.Height = 64;
        colorAttachment.pTexture = &Luminance64;
        FramebufferLum64.Initialize( framebufferCI );

        framebufferCI.Width = 32;
        framebufferCI.Height = 32;
        colorAttachment.pTexture = &Luminance32;
        FramebufferLum32.Initialize( framebufferCI );

        framebufferCI.Width = 16;
        framebufferCI.Height = 16;
        colorAttachment.pTexture = &Luminance16;
        FramebufferLum16.Initialize( framebufferCI );

        framebufferCI.Width = 8;
        framebufferCI.Height = 8;
        colorAttachment.pTexture = &Luminance8;
        FramebufferLum8.Initialize( framebufferCI );

        framebufferCI.Width = 4;
        framebufferCI.Height = 4;
        colorAttachment.pTexture = &Luminance4;
        FramebufferLum4.Initialize( framebufferCI );

        framebufferCI.Width = 2;
        framebufferCI.Height = 2;
        colorAttachment.pTexture = &Luminance2;
        FramebufferLum2.Initialize( framebufferCI );
    }

    CreateFullscreenQuadPipeline( MakeLuminanceMapPipe, "postprocess/makeLuminanceMap.vert", "postprocess/makeLuminanceMap.frag", LuminancePass );
    CreateFullscreenQuadPipeline( SumLuminanceMapPipe, "postprocess/sumLuminanceMap.vert", "postprocess/sumLuminanceMap.frag", LuminancePass );
    CreateFullscreenQuadPipeline( DynamicExposurePipe, "postprocess/dynamicExposure.vert", "postprocess/dynamicExposure.frag", LuminancePass, GHI::BLENDING_ALPHA );

    CreateSampler();
}

void AExposureRenderer::Deinitialize() {
    LuminancePass.Deinitialize();
    MakeLuminanceMapPipe.Deinitialize();
    SumLuminanceMapPipe.Deinitialize();
    DynamicExposurePipe.Deinitialize();

    Luminance64.Deinitialize();
    Luminance32.Deinitialize();
    Luminance16.Deinitialize();
    Luminance8.Deinitialize();
    Luminance4.Deinitialize();
    Luminance2.Deinitialize();
    DefaultLuminance.Deinitialize();
    FramebufferLum64.Deinitialize();
    FramebufferLum32.Deinitialize();
    FramebufferLum16.Deinitialize();
    FramebufferLum8.Deinitialize();
    FramebufferLum4.Deinitialize();
    FramebufferLum2.Deinitialize();
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

void AExposureRenderer::Render( GHI::Texture & SourceTexture ) {
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

    GFrameResources.TextureBindings[0].pTexture = &SourceTexture;
    GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &LuminancePass;

    // Make luminance map 64x64
    renderPassBegin.pFramebuffer = &FramebufferLum64;
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
    GFrameResources.TextureBindings[0].pTexture = &Luminance64;
    renderPassBegin.pFramebuffer = &FramebufferLum32;
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
    GFrameResources.TextureBindings[0].pTexture = &Luminance32;
    renderPassBegin.pFramebuffer = &FramebufferLum16;
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
    GFrameResources.TextureBindings[0].pTexture = &Luminance16;
    renderPassBegin.pFramebuffer = &FramebufferLum8;
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
    GFrameResources.TextureBindings[0].pTexture = &Luminance8;
    renderPassBegin.pFramebuffer = &FramebufferLum4;
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
    GFrameResources.TextureBindings[0].pTexture = &Luminance4;
    renderPassBegin.pFramebuffer = &FramebufferLum2;
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

    GFrameResources.TextureBindings[0].pTexture = &Luminance2;
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
