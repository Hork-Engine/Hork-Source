/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "BloomRenderer.h"
#include "RenderLocal.h"

ARuntimeVariable r_BloomTextureFormat(_CTS("r_BloomTextureFormat"), _CTS("0"), 0, _CTS("0 - R11F_G11F_B10F, 1 - RGB16F, 2 - RGB8"));
ARuntimeVariable r_BloomStart(_CTS("r_BloomStart"), _CTS("1"));
ARuntimeVariable r_BloomThreshold(_CTS("r_BloomThreshold"), _CTS("1"));

using namespace RenderCore;

ABloomRenderer::ABloomRenderer()
{
    SPipelineResourceLayout resourceLayout;

    SSamplerDesc samplerCI;
    samplerCI.Filter   = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;

    SBufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT;
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &samplerCI;
    resourceLayout.NumBuffers  = AN_ARRAY_SIZE(bufferInfo);
    resourceLayout.Buffers     = bufferInfo;

    CreateFullscreenQuadPipeline(&BrightPipeline, "postprocess/brightpass.vert", "postprocess/brightpass.frag", &resourceLayout);
    CreateFullscreenQuadPipeline(&BlurPipeline, "postprocess/gauss.vert", "postprocess/gauss.frag", &resourceLayout);

    resourceLayout.NumBuffers = 0;
    CreateFullscreenQuadPipeline(&CopyPipeline, "postprocess/copy.vert", "postprocess/copy.frag", &resourceLayout);
}

void ABloomRenderer::AddPasses(AFrameGraph& FrameGraph, FGTextureProxy* SourceTexture, ABloomRenderer::STextures* pResult)
{
    RenderCore::TEXTURE_FORMAT pf;

    switch (r_BloomTextureFormat.GetInteger())
    {
        case 0:
            pf = RenderCore::TEXTURE_FORMAT_R11F_G11F_B10F;
            break;
        case 1:
            pf = RenderCore::TEXTURE_FORMAT_RGB16F;
            break;
        default:
            // TODO: We can use RGB8 format, but it need some way of bloom compression to not lose in quality.
            pf = RenderCore::TEXTURE_FORMAT_RGB8;
            break;
    }

    STextureResolution2D bloomResolution = GetFrameResoultion();
    bloomResolution.Width >>= 1;
    bloomResolution.Height >>= 1;

    FGTextureProxy *BrightTexture, *BrightBlurXTexture, *BrightBlurTexture;
    FGTextureProxy *BrightTexture2, *BrightBlurXTexture2, *BrightBlurTexture2;
    FGTextureProxy *BrightTexture4, *BrightBlurXTexture4, *BrightBlurTexture4;
    FGTextureProxy *BrightTexture6, *BrightBlurXTexture6, *BrightBlurTexture6;

    // Make bright texture
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: Bright Pass");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(SourceTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright texture",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SBrightPassDrawCall
                            {
                                Float4 BloomStart;
                                Float4 BloomThreshold;
                            };

                            SBrightPassDrawCall* drawCall = MapDrawCallConstants<SBrightPassDrawCall>();
                            drawCall->BloomStart          = Float4(r_BloomStart.GetFloat());
                            drawCall->BloomThreshold      = Float4(r_BloomThreshold.GetFloat());

                            rtbl->BindTexture(0, SourceTexture->Actual());

                            DrawSAQ(BrightPipeline);
                        });

        BrightTexture = pass.GetColorAttachments()[0].pResource;
    }


    //
    // Perform gaussian blur
    //

    // X pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: X pass. Result in BrightBlurXTexture");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright Blur X texture",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurXTexture = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: Y pass. Result in BrightBlurTexture");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright Blur texture",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurTexture = pass.GetColorAttachments()[0].pResource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Downsample BrightBlurTexture to BrightTexture2");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurTexture, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright texture 2",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            rtbl->BindTexture(0, BrightBlurTexture->Actual());

                            DrawSAQ(CopyPipeline);
                        });

        BrightTexture2 = pass.GetColorAttachments()[0].pResource;
    }

    // X pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: X pass. Result in BrightBlurXTexture2");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture2, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright blur X texture 2",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture2->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurXTexture2 = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: Y pass. Result in BrightBlurTexture2");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture2, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright blur texture 2",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture2->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurTexture2 = pass.GetColorAttachments()[0].pResource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Downsample BrightBlurTexture2 to BrightTexture4");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurTexture2, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright texture 4",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            rtbl->BindTexture(0, BrightBlurTexture2->Actual());

                            DrawSAQ(CopyPipeline);
                        });

        BrightTexture4 = pass.GetColorAttachments()[0].pResource;
    }

    // X pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: X pass.Result in BrightBlurXTexture4");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture4, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright blur X texture 4",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture4->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurXTexture4 = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: Y pass. Result in BrightBlurTexture4");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture4, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright blur texture 4",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture4->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurTexture4 = pass.GetColorAttachments()[0].pResource;
    }

    bloomResolution.Width >>= 2;
    bloomResolution.Height >>= 2;

    // Downsample
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Downsample BrightBlurTexture4 to BrightTexture6");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurTexture4, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright texture 6",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            rtbl->BindTexture(0, BrightBlurTexture4->Actual());

                            DrawSAQ(CopyPipeline);
                        });

        BrightTexture6 = pass.GetColorAttachments()[0].pResource;
    }

    // X pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: X pass. Result in BrightBlurXTexture6");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightTexture6, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright blur X texture 6",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                            drawCall->InvSize.Y = 0;

                            rtbl->BindTexture(0, BrightTexture6->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurXTexture6 = pass.GetColorAttachments()[0].pResource;
    }

    // Y pass
    {
        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Bloom: Y pass. Result in BrightBlurTexture6");
        pass.SetRenderArea(bloomResolution.Width, bloomResolution.Height);
        pass.AddResource(BrightBlurXTexture6, FG_RESOURCE_ACCESS_READ);
        pass.SetColorAttachment(
            STextureAttachment("Bright blur texture 6",
                               STextureDesc()
                                   .SetFormat(pf)
                                   .SetResolution(bloomResolution))
                .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
        pass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            struct SDrawCall
                            {
                                Float2 InvSize;
                            };

                            SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                            drawCall->InvSize.X = 0;
                            drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                            rtbl->BindTexture(0, BrightBlurXTexture6->Actual());

                            DrawSAQ(BlurPipeline);
                        });

        BrightBlurTexture6 = pass.GetColorAttachments()[0].pResource;
    }

    pResult->BloomTexture0 = BrightBlurTexture;
    pResult->BloomTexture1 = BrightBlurTexture2;
    pResult->BloomTexture2 = BrightBlurTexture4;
    pResult->BloomTexture3 = BrightBlurTexture6;
}
