/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "RenderLocal.h"
#include "SSAORenderer.h"

#include <Core/Random.h>

AConsoleVar r_HBAODeinterleaved(_CTS("r_HBAODeinterleaved"), _CTS("1"));
AConsoleVar r_HBAOBlur(_CTS("r_HBAOBlur"), _CTS("1"));
AConsoleVar r_HBAORadius(_CTS("r_HBAORadius"), _CTS("2"));
AConsoleVar r_HBAOBias(_CTS("r_HBAOBias"), _CTS("0.1"));
AConsoleVar r_HBAOPowExponent(_CTS("r_HBAOPowExponent"), _CTS("1.5"));

using namespace RenderCore;

ASSAORenderer::ASSAORenderer()
{
    SPipelineResourceLayout resourceLayout;

    SSamplerDesc nearestSampler;

    nearestSampler.Filter   = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SSamplerDesc pipeSamplers[3];

    // Linear depth sampler
    pipeSamplers[0] = nearestSampler;

    // Normal texture sampler
    pipeSamplers[1] = nearestSampler;

    // Random map sampler
    pipeSamplers[2].Filter   = FILTER_NEAREST;
    pipeSamplers[2].AddressU = SAMPLER_ADDRESS_WRAP;
    pipeSamplers[2].AddressV = SAMPLER_ADDRESS_WRAP;
    pipeSamplers[2].AddressW = SAMPLER_ADDRESS_WRAP;

    SBufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    resourceLayout.NumBuffers = 2;
    resourceLayout.Buffers    = bufferInfo;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(pipeSamplers);
    resourceLayout.Samplers    = pipeSamplers;

    AShaderFactory::CreateFullscreenQuadPipeline(&Pipe, "postprocess/ssao/ssao.vert", "postprocess/ssao/simple.frag", &resourceLayout);
    AShaderFactory::CreateFullscreenQuadPipeline(&Pipe_ORTHO, "postprocess/ssao/ssao.vert", "postprocess/ssao/simple_ortho.frag", &resourceLayout);

    SSamplerDesc cacheAwareSamplers[2];

    // Deinterleave depth array sampler
    cacheAwareSamplers[0] = nearestSampler;
    // Normal texture sampler
    cacheAwareSamplers[1] = nearestSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(cacheAwareSamplers);
    resourceLayout.Samplers    = cacheAwareSamplers;

    AShaderFactory::CreateFullscreenQuadPipelineGS(&CacheAwarePipe, "postprocess/ssao/ssao.vert", "postprocess/ssao/deinterleaved.frag", "postprocess/ssao/deinterleaved.geom", &resourceLayout);
    AShaderFactory::CreateFullscreenQuadPipelineGS(&CacheAwarePipe_ORTHO, "postprocess/ssao/ssao.vert", "postprocess/ssao/deinterleaved_ortho.frag", "postprocess/ssao/deinterleaved.geom", &resourceLayout);

    SSamplerDesc blurSamplers[2];

    // SSAO texture sampler
    blurSamplers[0].Filter   = FILTER_LINEAR;
    blurSamplers[0].AddressU = SAMPLER_ADDRESS_CLAMP;
    blurSamplers[0].AddressV = SAMPLER_ADDRESS_CLAMP;
    blurSamplers[0].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Linear depth sampler
    blurSamplers[1] = nearestSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(blurSamplers);
    resourceLayout.Samplers    = blurSamplers;

    AShaderFactory::CreateFullscreenQuadPipeline(&BlurPipe, "postprocess/ssao/blur.vert", "postprocess/ssao/blur.frag", &resourceLayout);

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &nearestSampler;

    AShaderFactory::CreateFullscreenQuadPipeline(&DeinterleavePipe, "postprocess/ssao/deinterleave.vert", "postprocess/ssao/deinterleave.frag", &resourceLayout);

    resourceLayout.NumBuffers = 0;
    AShaderFactory::CreateFullscreenQuadPipeline(&ReinterleavePipe, "postprocess/ssao/reinterleave.vert", "postprocess/ssao/reinterleave.frag", &resourceLayout);

    AMersenneTwisterRand rng(0u);

    const float NUM_DIRECTIONS = 8;

    Float3 hbaoRandom[HBAO_RANDOM_ELEMENTS];

    for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
    {
        const float r1 = rng.GetFloat();
        const float r2 = rng.GetFloat();

        // Random rotation angles in [0,2PI/NUM_DIRECTIONS)
        const float angle = Math::_2PI * r1 / NUM_DIRECTIONS;

        float s, c;

        Math::SinCos(angle, s, c);

        hbaoRandom[i].X = c;
        hbaoRandom[i].Y = s;
        hbaoRandom[i].Z = r2;

        std::swap(hbaoRandom[i].X, hbaoRandom[i].Z); // Swap to BGR
    }

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    LOG( "vec2( {}, {} ),\n", float( i % 4 ) + 0.5f, float( i / 4 ) + 0.5f );
    //}

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    LOG( "vec4( {}, {}, {}, 0.0 ),\n", hbaoRandom[i].X, hbaoRandom[i].Y, hbaoRandom[i].Z );
    //}

    GDevice->CreateTexture(STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RGB16F)
                               .SetResolution(STextureResolution2D(HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &RandomMap);
    RandomMap->SetDebugName("SSAO Random Map");

    RandomMap->Write(0, FORMAT_FLOAT3, sizeof(hbaoRandom), 1, hbaoRandom);
}

void ASSAORenderer::ResizeAO(int Width, int Height)
{
    if (AOWidth != Width || AOHeight != Height)
    {
        AOWidth  = Width;
        AOHeight = Height;

        AOQuarterWidth  = ((AOWidth + 3) / 4);
        AOQuarterHeight = ((AOHeight + 3) / 4);

        GDevice->CreateTexture(
            STextureDesc()
                .SetFormat(TEXTURE_FORMAT_R32F)
                .SetResolution(STextureResolution2DArray(AOQuarterWidth, AOQuarterHeight, HBAO_RANDOM_ELEMENTS))
                .SetBindFlags(BIND_SHADER_RESOURCE),
            &SSAODeinterleaveDepthArray);

        //for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
        //{
        //    STextureViewDesc desc;
        //    desc.Type          = RenderCore::TEXTURE_2D;
        //    desc.Format        = TEXTURE_FORMAT_R32F;
        //    desc.FirstMipLevel = 0;
        //    desc.NumMipLevels  = 1;
        //    desc.FirstSlice    = i;
        //    desc.NumSlices     = 1;
        //    SSAODeinterleaveDepthView[i] = SSAODeinterleaveDepthArray->GetTextureView(desc);
        //}
    }
}

void ASSAORenderer::AddDeinterleaveDepthPass(AFrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy** ppDeinterleaveDepthArray)
{
//    static const char * textureViewName[] =
//    {
//            "Deinterleave Depth View 0",
//            "Deinterleave Depth View 1",
//            "Deinterleave Depth View 2",
//            "Deinterleave Depth View 3",
//            "Deinterleave Depth View 4",
//            "Deinterleave Depth View 5",
//            "Deinterleave Depth View 6",
//            "Deinterleave Depth View 7",
//            "Deinterleave Depth View 8",
//            "Deinterleave Depth View 9",
//            "Deinterleave Depth View 10",
//            "Deinterleave Depth View 11",
//            "Deinterleave Depth View 12",
//            "Deinterleave Depth View 13",
//            "Deinterleave Depth View 14",
//            "Deinterleave Depth View 15"
//    };

    FGTextureProxy* SSAODeinterleaveDepthArray_R = FrameGraph.AddExternalResource<FGTextureProxy>("SSAODeinterleaveDepthArray", SSAODeinterleaveDepthArray);

    //FGTextureProxy* SSAODeinterleaveDepthView_R[16];
    //for (int i = 0; i < 16; i++)
    //{
    //    SSAODeinterleaveDepthView_R[i] = FrameGraph.AddTextureView(textureViewName[i], SSAODeinterleaveDepthView[i]);
    //}

    ARenderPass& deinterleavePass = FrameGraph.AddTask<ARenderPass>("Deinterleave Depth Pass");
    deinterleavePass.SetRenderArea(AOQuarterWidth, AOQuarterHeight);
    deinterleavePass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    deinterleavePass.SetColorAttachments(
        {
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(0)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(1)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(2)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(3)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(4)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(5)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(6)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(7)},
        });
    deinterleavePass.AddSubpass({0, 1, 2, 3, 4, 5, 6, 7}, // color attachment refs
                                [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                                {
                                    struct SDrawCall
                                    {
                                        Float2 UVOffset;
                                        Float2 InvFullResolution;
                                    };

                                    SDrawCall* drawCall           = MapDrawCallConstants<SDrawCall>();
                                    drawCall->UVOffset.X          = 0.5f;
                                    drawCall->UVOffset.Y          = 0.5f;
                                    drawCall->InvFullResolution.X = 1.0f / AOWidth;
                                    drawCall->InvFullResolution.Y = 1.0f / AOHeight;

                                    rtbl->BindTexture(0, LinearDepth->Actual());

                                    DrawSAQ(RenderPassContext.pImmediateContext, DeinterleavePipe);
                                });

    ARenderPass& deinterleavePass2 = FrameGraph.AddTask<ARenderPass>("Deinterleave Depth Pass 2");
    deinterleavePass2.SetRenderArea(AOQuarterWidth, AOQuarterHeight);
    deinterleavePass2.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    deinterleavePass2.SetColorAttachments(
        {
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(8)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(9)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(10)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(11)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(12)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(13)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(14)},
            {STextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(15)},
        });
    deinterleavePass2.AddSubpass({0, 1, 2, 3, 4, 5, 6, 7}, // color attachment refs
                                 [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                                 {
                                     struct SDrawCall
                                     {
                                         Float2 UVOffset;
                                         Float2 InvFullResolution;
                                     };

                                     SDrawCall* drawCall           = MapDrawCallConstants<SDrawCall>();
                                     drawCall->UVOffset.X          = float(8 % 4) + 0.5f;
                                     drawCall->UVOffset.Y          = float(8 / 4) + 0.5f;
                                     drawCall->InvFullResolution.X = 1.0f / AOWidth;
                                     drawCall->InvFullResolution.Y = 1.0f / AOHeight;

                                     rtbl->BindTexture(0, LinearDepth->Actual());

                                     DrawSAQ(RenderPassContext.pImmediateContext, DeinterleavePipe);
                                 });

    FGTextureProxy* DeinterleaveDepthArray_R = FrameGraph.AddExternalResource<FGTextureProxy>("Deinterleave Depth Array", SSAODeinterleaveDepthArray);

    *ppDeinterleaveDepthArray = DeinterleaveDepthArray_R;
}

void ASSAORenderer::AddCacheAwareAOPass(AFrameGraph& FrameGraph, FGTextureProxy* DeinterleaveDepthArray, FGTextureProxy* NormalTexture, FGTextureProxy** ppSSAOTextureArray)
{
    ARenderPass& cacheAwareAO = FrameGraph.AddTask<ARenderPass>("Cache Aware AO Pass");
    cacheAwareAO.SetRenderArea(AOQuarterWidth, AOQuarterHeight);
    cacheAwareAO.AddResource(DeinterleaveDepthArray, FG_RESOURCE_ACCESS_READ);
    cacheAwareAO.AddResource(NormalTexture, FG_RESOURCE_ACCESS_READ);
    cacheAwareAO.SetColorAttachment(
        STextureAttachment("SSAO Texture Array",
                           STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8)
                               .SetResolution(STextureResolution2DArray(AOQuarterWidth, AOQuarterHeight, HBAO_RANDOM_ELEMENTS)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    cacheAwareAO.AddSubpass({0}, // color attachment refs
                            [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                            {
                                struct SDrawCall
                                {
                                    float  Bias;
                                    float  FallofFactor;
                                    float  RadiusToScreen;
                                    float  PowExponent;
                                    float  Multiplier;
                                    float  Pad;
                                    Float2 InvFullResolution;
                                    Float2 InvQuarterResolution;
                                };

                                SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();

                                float projScale;

                                if (GRenderView->bPerspective)
                                {
                                    projScale = (float)AOHeight / std::tan(GRenderView->ViewFovY * 0.5f) * 0.5f;
                                }
                                else
                                {
                                    projScale = (float)AOHeight * GRenderView->ProjectionMatrix[1][1] * 0.5f;
                                }

                                drawCall->Bias                   = r_HBAOBias.GetFloat();
                                drawCall->FallofFactor           = -1.0f / (r_HBAORadius.GetFloat() * r_HBAORadius.GetFloat());
                                drawCall->RadiusToScreen         = r_HBAORadius.GetFloat() * 0.5f * projScale;
                                drawCall->PowExponent            = r_HBAOPowExponent.GetFloat();
                                drawCall->Multiplier             = 1.0f / (1.0f - r_HBAOBias.GetFloat());
                                drawCall->InvFullResolution.X    = 1.0f / AOWidth;
                                drawCall->InvFullResolution.Y    = 1.0f / AOHeight;
                                drawCall->InvQuarterResolution.X = 1.0f / AOQuarterWidth;
                                drawCall->InvQuarterResolution.Y = 1.0f / AOQuarterHeight;

                                rtbl->BindTexture(0, DeinterleaveDepthArray->Actual());
                                rtbl->BindTexture(1, NormalTexture->Actual());

                                if (GRenderView->bPerspective)
                                {
                                    DrawSAQ(RenderPassContext.pImmediateContext, CacheAwarePipe);
                                }
                                else
                                {
                                    DrawSAQ(RenderPassContext.pImmediateContext, CacheAwarePipe_ORTHO);
                                }
                            });

    *ppSSAOTextureArray = cacheAwareAO.GetColorAttachments()[0].pResource;
}

void ASSAORenderer::AddReinterleavePass(AFrameGraph& FrameGraph, FGTextureProxy* SSAOTextureArray, FGTextureProxy** ppSSAOTexture)
{
    ARenderPass& reinterleavePass = FrameGraph.AddTask<ARenderPass>("Reinterleave Pass");
    reinterleavePass.SetRenderArea(AOWidth, AOHeight);
    reinterleavePass.AddResource(SSAOTextureArray, FG_RESOURCE_ACCESS_READ);
    reinterleavePass.SetColorAttachment(
        STextureAttachment("SSAO Texture",
                           STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8)
                               .SetResolution(STextureResolution2D(AOWidth, AOHeight))
                               .SetBindFlags(BIND_SHADER_RESOURCE))                               
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    reinterleavePass.AddSubpass({0}, // color attachment refs
                                [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                                {
                                    rtbl->BindTexture(0, SSAOTextureArray->Actual());

                                    DrawSAQ(RenderPassContext.pImmediateContext, ReinterleavePipe);
                                });

    *ppSSAOTexture = reinterleavePass.GetColorAttachments()[0].pResource;
}

void ASSAORenderer::AddSimpleAOPass(AFrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy* NormalTexture, FGTextureProxy** ppSSAOTexture)
{
    FGTextureProxy* RandomMapTexture_R = FrameGraph.AddExternalResource<FGTextureProxy>("SSAO Random Map", RandomMap);

    ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Simple AO Pass");
    pass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    pass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    pass.AddResource(NormalTexture, FG_RESOURCE_ACCESS_READ);
    pass.AddResource(RandomMapTexture_R, FG_RESOURCE_ACCESS_READ);
    pass.SetColorAttachment(
        STextureAttachment("SSAO Texture (Interleaved)",
                           STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8)
                               .SetResolution(STextureResolution2D(AOWidth, AOHeight)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    pass.AddSubpass({0}, // color attachment refs
                    [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                    {
                        struct SDrawCall
                        {
                            float  Bias;
                            float  FallofFactor;
                            float  RadiusToScreen;
                            float  PowExponent;
                            float  Multiplier;
                            float  Pad;
                            Float2 InvFullResolution;
                            Float2 InvQuarterResolution;
                        };

                        SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();

                        float projScale;

                        if (GRenderView->bPerspective)
                        {
                            projScale = (float)/*AOHeight*/ GRenderView->Height / std::tan(GRenderView->ViewFovY * 0.5f) * 0.5f;
                        }
                        else
                        {
                            projScale = (float)/*AOHeight*/ GRenderView->Height * GRenderView->ProjectionMatrix[1][1] * 0.5f;
                        }

                        drawCall->Bias                   = r_HBAOBias.GetFloat();
                        drawCall->FallofFactor           = -1.0f / (r_HBAORadius.GetFloat() * r_HBAORadius.GetFloat());
                        drawCall->RadiusToScreen         = r_HBAORadius.GetFloat() * 0.5f * projScale;
                        drawCall->PowExponent            = r_HBAOPowExponent.GetFloat();
                        drawCall->Multiplier             = 1.0f / (1.0f - r_HBAOBias.GetFloat());
                        drawCall->InvFullResolution.X    = 1.0f / GRenderView->Width;  //AOWidth;
                        drawCall->InvFullResolution.Y    = 1.0f / GRenderView->Height; //AOHeight;
                        drawCall->InvQuarterResolution.X = 0;                          // don't care
                        drawCall->InvQuarterResolution.Y = 0;                          // don't care

                        rtbl->BindTexture(0, LinearDepth->Actual());
                        rtbl->BindTexture(1, NormalTexture->Actual());
                        rtbl->BindTexture(2, RandomMapTexture_R->Actual());

                        if (GRenderView->bPerspective)
                        {
                            DrawSAQ(RenderPassContext.pImmediateContext, Pipe);
                        }
                        else
                        {
                            DrawSAQ(RenderPassContext.pImmediateContext, Pipe_ORTHO);
                        }
                    });

    *ppSSAOTexture = pass.GetColorAttachments()[0].pResource;
}

void ASSAORenderer::AddAOBlurPass(AFrameGraph& FrameGraph, FGTextureProxy* SSAOTexture, FGTextureProxy* LinearDepth, FGTextureProxy** ppBluredSSAO)
{
    ARenderPass& aoBlurXPass = FrameGraph.AddTask<ARenderPass>("AO Blur X Pass");
    aoBlurXPass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    aoBlurXPass.SetColorAttachment(
        STextureAttachment("Temp SSAO Texture (Blur X)",
                           STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8)
                               .SetResolution(STextureResolution2D(AOWidth, AOHeight)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    aoBlurXPass.AddResource(SSAOTexture, FG_RESOURCE_ACCESS_READ);
    aoBlurXPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    aoBlurXPass.AddSubpass({0}, // color attachment refs
                           [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                           {
                               struct SDrawCall
                               {
                                   Float2 InvSize;
                               };

                               SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                               drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                               drawCall->InvSize.Y = 0;

                               // SSAO blur X
                               rtbl->BindTexture(0, SSAOTexture->Actual());
                               rtbl->BindTexture(1, LinearDepth->Actual());

                               DrawSAQ(RenderPassContext.pImmediateContext, BlurPipe);
                           });

    FGTextureProxy* TempSSAOTextureBlurX = aoBlurXPass.GetColorAttachments()[0].pResource;

    ARenderPass& aoBlurYPass = FrameGraph.AddTask<ARenderPass>("AO Blur Y Pass");
    aoBlurYPass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    aoBlurYPass.SetColorAttachment(
        STextureAttachment("Blured SSAO Texture",
                           STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8)
                               .SetResolution(STextureResolution2D(AOWidth, AOHeight)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    aoBlurYPass.AddResource(TempSSAOTextureBlurX, FG_RESOURCE_ACCESS_READ);
    aoBlurYPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    aoBlurYPass.AddSubpass({0}, // color attachment refs
                           [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                           {
                               struct SDrawCall
                               {
                                   Float2 InvSize;
                               };

                               SDrawCall* drawCall = MapDrawCallConstants<SDrawCall>();
                               drawCall->InvSize.X = 0;
                               drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                               // SSAO blur Y
                               rtbl->BindTexture(0, TempSSAOTextureBlurX->Actual());
                               rtbl->BindTexture(1, LinearDepth->Actual());

                               DrawSAQ(RenderPassContext.pImmediateContext, BlurPipe);
                           });

    *ppBluredSSAO = aoBlurYPass.GetColorAttachments()[0].pResource;
}

void ASSAORenderer::AddPasses(AFrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy* NormalTexture, FGTextureProxy** ppSSAOTexture)
{
    ResizeAO(GFrameData->RenderTargetMaxWidth, GFrameData->RenderTargetMaxHeight);

    if (r_HBAODeinterleaved && GRenderView->Width == GFrameData->RenderTargetMaxWidth && GRenderView->Height == GFrameData->RenderTargetMaxHeight)
    {
        FGTextureProxy *DeinterleaveDepthArray, *SSAOTextureArray;

        AddDeinterleaveDepthPass(FrameGraph, LinearDepth, &DeinterleaveDepthArray);
        AddCacheAwareAOPass(FrameGraph, DeinterleaveDepthArray, NormalTexture, &SSAOTextureArray);
        AddReinterleavePass(FrameGraph, SSAOTextureArray, ppSSAOTexture);
    }
    else
    {
        AddSimpleAOPass(FrameGraph, LinearDepth, NormalTexture, ppSSAOTexture);
    }

    if (r_HBAOBlur)
    {
        AddAOBlurPass(FrameGraph, *ppSSAOTexture, LinearDepth, ppSSAOTexture);
    }
}
