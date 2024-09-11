/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "BloomRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

ConsoleVar r_BloomTextureFormat("r_BloomTextureFormat"_s, "0"_s, 0, "0 - R11F_G11F_B10F, 1 - RGBA16F, 2 - RGBA8"_s);
ConsoleVar r_BloomStart("r_BloomStart"_s, "1"_s);
ConsoleVar r_BloomThreshold("r_BloomThreshold"_s, "1"_s);

using namespace RenderCore;

BloomRenderer::BloomRenderer()
{
    PipelineResourceLayout resourceLayout;

    SamplerDesc samplerCI;
    samplerCI.Filter   = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;

    BufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT;
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &samplerCI;
    resourceLayout.NumBuffers  = HK_ARRAY_SIZE(bufferInfo);
    resourceLayout.Buffers     = bufferInfo;

    ShaderFactory::sCreateFullscreenQuadPipeline(&BrightPipeline, "postprocess/brightpass.vert", "postprocess/brightpass.frag", &resourceLayout);
    ShaderFactory::sCreateFullscreenQuadPipeline(&BlurPipeline, "postprocess/gauss.vert", "postprocess/gauss.frag", &resourceLayout);

    resourceLayout.NumBuffers = 0;
    ShaderFactory::sCreateFullscreenQuadPipeline(&CopyPipeline, "postprocess/copy.vert", "postprocess/copy.frag", &resourceLayout);
}

void BloomRenderer::AddPasses(FrameGraph& FrameGraph, FGTextureProxy* SourceTexture, BloomRenderer::Textures* pResult)
{
    TEXTURE_FORMAT pf;

    switch (r_BloomTextureFormat.GetInteger())
    {
        case 0:
            pf = TEXTURE_FORMAT_R11G11B10_FLOAT;
            break;
        case 1:
            pf = TEXTURE_FORMAT_RGBA16_FLOAT;
            break;
        default:
            // TODO: We can use RGBA8 format, but it need some way of bloom compression to not lose in quality.
            pf = TEXTURE_FORMAT_RGBA8_UNORM;
            break;
    }

    TextureResolution2D bloomResolution = GetFrameResoultion();
    bloomResolution.Width >>= 1;
    bloomResolution.Height >>= 1;
    bloomResolution.Width = Math::Max(64u, bloomResolution.Width);
    bloomResolution.Height = Math::Max(64u, bloomResolution.Height);

    FGTextureProxy *BrightTexture, *BrightBlurXTexture, *BrightBlurTexture;
    FGTextureProxy *BrightTexture2, *BrightBlurXTexture2, *BrightBlurTexture2;
    FGTextureProxy *BrightTexture4, *BrightBlurXTexture4, *BrightBlurTexture4;
    FGTextureProxy *BrightTexture6, *BrightBlurXTexture6, *BrightBlurTexture6;

    // Make bright texture
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: Bright Pass");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(SourceTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright texture",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct BrightPassDrawCall
                            {
                                Float4 BloomStart;
                                Float4 BloomThreshold;
                            };

                            BrightPassDrawCall* drawCall = MapDrawCallConstants<BrightPassDrawCall>();
                            drawCall->BloomStart          = Float4(r_BloomStart.GetFloat());
                            drawCall->BloomThreshold      = Float4(r_BloomThreshold.GetFloat());

                            rtbl->BindTexture(0, SourceTexture->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BrightPipeline);
                        });

        BrightTexture = pass.GetColorAttachments()[0].pResource;
    }


    //
    // Perform gaussian blur
    //

    // X pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: X pass. Result in BrightBlurXTexture");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright Blur X texture",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurXTexture = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: Y pass. Result in BrightBlurTexture");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright Blur texture",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurTexture = pass.GetColorAttachments()[0].pResource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Downsample BrightBlurTexture to BrightTexture2");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright texture 2",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            rtbl->BindTexture(0, BrightBlurTexture->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, CopyPipeline);
                        });

        BrightTexture2 = pass.GetColorAttachments()[0].pResource;
    }

    // X pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: X pass. Result in BrightBlurXTexture2");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture2, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright blur X texture 2",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture2->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurXTexture2 = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: Y pass. Result in BrightBlurTexture2");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture2, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright blur texture 2",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture2->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurTexture2 = pass.GetColorAttachments()[0].pResource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Downsample BrightBlurTexture2 to BrightTexture4");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurTexture2, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright texture 4",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            rtbl->BindTexture(0, BrightBlurTexture2->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, CopyPipeline);
                        });

        BrightTexture4 = pass.GetColorAttachments()[0].pResource;
    }

    // X pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: X pass.Result in BrightBlurXTexture4");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture4, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright blur X texture 4",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture4->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurXTexture4 = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: Y pass. Result in BrightBlurTexture4");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture4, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright blur texture 4",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture4->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurTexture4 = pass.GetColorAttachments()[0].pResource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Downsample BrightBlurTexture4 to BrightTexture6");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurTexture4, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright texture 6",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            rtbl->BindTexture(0, BrightBlurTexture4->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, CopyPipeline);
                        });

        BrightTexture6 = pass.GetColorAttachments()[0].pResource;
    }

    // X pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: X pass. Result in BrightBlurXTexture6");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture6, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright blur X texture 6",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture6->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurXTexture6 = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        RenderPass& pass = FrameGraph.AddTask<RenderPass>("Bloom: Y pass. Result in BrightBlurTexture6");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture6, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            TextureAttachment("Bright blur texture 6",
                               TextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            struct DrawCall
                            {
                                Float2 InvSize;
                            };

                            DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture6->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, BlurPipeline);
                        });

        BrightBlurTexture6 = pass.GetColorAttachments()[0].pResource;
    }

    pResult->BloomTexture0 = BrightBlurTexture;
    pResult->BloomTexture1 = BrightBlurTexture2;
    pResult->BloomTexture2 = BrightBlurTexture4;
    pResult->BloomTexture3 = BrightBlurTexture6;
}

HK_NAMESPACE_END
