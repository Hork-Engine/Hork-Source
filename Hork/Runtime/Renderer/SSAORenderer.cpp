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

#include "RenderLocal.h"
#include "SSAORenderer.h"

#include <Hork/Core/Random.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_HBAODeinterleaved("r_HBAODeinterleaved"_s, "1"_s);
ConsoleVar r_HBAOBlur("r_HBAOBlur"_s, "1"_s);
ConsoleVar r_HBAORadius("r_HBAORadius"_s, "2"_s);
ConsoleVar r_HBAOBias("r_HBAOBias"_s, "0.1"_s);
ConsoleVar r_HBAOPowExponent("r_HBAOPowExponent"_s, "1.5"_s);

using namespace RHI;

SSAORenderer::SSAORenderer()
{
    PipelineResourceLayout resourceLayout;

    SamplerDesc nearestSampler;

    nearestSampler.Filter   = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SamplerDesc pipeSamplers[3];

    // Linear depth sampler
    pipeSamplers[0] = nearestSampler;

    // Normal texture sampler
    pipeSamplers[1] = nearestSampler;

    // Random map sampler
    pipeSamplers[2].Filter   = FILTER_NEAREST;
    pipeSamplers[2].AddressU = SAMPLER_ADDRESS_WRAP;
    pipeSamplers[2].AddressV = SAMPLER_ADDRESS_WRAP;
    pipeSamplers[2].AddressW = SAMPLER_ADDRESS_WRAP;

    BufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    resourceLayout.NumBuffers = 2;
    resourceLayout.Buffers    = bufferInfo;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(pipeSamplers);
    resourceLayout.Samplers    = pipeSamplers;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &Pipe, "postprocess/ssao/ssao.vert", "postprocess/ssao/simple.frag", &resourceLayout);
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &Pipe_ORTHO, "postprocess/ssao/ssao.vert", "postprocess/ssao/simple_ortho.frag", &resourceLayout);

    SamplerDesc cacheAwareSamplers[2];

    // Deinterleave depth array sampler
    cacheAwareSamplers[0] = nearestSampler;
    // Normal texture sampler
    cacheAwareSamplers[1] = nearestSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(cacheAwareSamplers);
    resourceLayout.Samplers    = cacheAwareSamplers;

    ShaderUtils::CreateFullscreenQuadPipelineGS(GDevice, &CacheAwarePipe, "postprocess/ssao/ssao.vert", "postprocess/ssao/deinterleaved.frag", "postprocess/ssao/deinterleaved.geom", &resourceLayout);
    ShaderUtils::CreateFullscreenQuadPipelineGS(GDevice, &CacheAwarePipe_ORTHO, "postprocess/ssao/ssao.vert", "postprocess/ssao/deinterleaved_ortho.frag", "postprocess/ssao/deinterleaved.geom", &resourceLayout);

    SamplerDesc blurSamplers[2];

    // SSAO texture sampler
    blurSamplers[0].Filter   = FILTER_LINEAR;
    blurSamplers[0].AddressU = SAMPLER_ADDRESS_CLAMP;
    blurSamplers[0].AddressV = SAMPLER_ADDRESS_CLAMP;
    blurSamplers[0].AddressW = SAMPLER_ADDRESS_CLAMP;

    // Linear depth sampler
    blurSamplers[1] = nearestSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(blurSamplers);
    resourceLayout.Samplers    = blurSamplers;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &BlurPipe, "postprocess/ssao/blur.vert", "postprocess/ssao/blur.frag", &resourceLayout);

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &nearestSampler;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &DeinterleavePipe, "postprocess/ssao/deinterleave.vert", "postprocess/ssao/deinterleave.frag", &resourceLayout);

    resourceLayout.NumBuffers = 0;
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &ReinterleavePipe, "postprocess/ssao/reinterleave.vert", "postprocess/ssao/reinterleave.frag", &resourceLayout);

    MersenneTwisterRand rng(0u);

    const float NUM_DIRECTIONS = 8;

    Half   hbaoRandom_Half[HBAO_RANDOM_ELEMENTS][4];

    for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
    {
        const float r1 = rng.GetFloat();
        const float r2 = rng.GetFloat();

        // Random rotation angles in [0,2PI/NUM_DIRECTIONS)
        const float angle = Math::_2PI * r1 / NUM_DIRECTIONS;

        float s, c;

        Math::SinCos(angle, s, c);

        //std::swap(hbaoRandom[i].X, hbaoRandom[i].Z); // Swap to BGR

        hbaoRandom_Half[i][0] = c;
        hbaoRandom_Half[i][1] = s;
        hbaoRandom_Half[i][2] = r2;
        hbaoRandom_Half[i][3] = 1;
    }

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    LOG( "vec2( {}, {} ),\n", float( i % 4 ) + 0.5f, float( i / 4 ) + 0.5f );
    //}

    //for ( int i = 0; i < HBAO_RANDOM_ELEMENTS; i++ ) {
    //    LOG( "vec4( {}, {}, {}, 0.0 ),\n", hbaoRandom[i].X, hbaoRandom[i].Y, hbaoRandom[i].Z );
    //}

    GDevice->CreateTexture(TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RGBA16_FLOAT)
                               .SetResolution(TextureResolution2D(HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &RandomMap);
    RandomMap->SetDebugName("SSAO Random Map");

    RandomMap->Write(0, sizeof(hbaoRandom_Half), 1, hbaoRandom_Half);
}

void SSAORenderer::AddDeinterleaveDepthPass(FrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy** ppDeinterleaveDepthArray)
{
    ITexture* deinterleavedDepthMaps = GRenderView->HBAOMaps;
    HK_ASSERT(deinterleavedDepthMaps);

    uint32_t quarterWidth  = deinterleavedDepthMaps->GetDesc().Resolution.Width;
    uint32_t quarterHeight = deinterleavedDepthMaps->GetDesc().Resolution.Height;

    Float2 invFullResolution = {1.0f / GRenderView->Width, 1.0f / GRenderView->Height};

    FGTextureProxy* SSAODeinterleaveDepthArray_R = FrameGraph.AddExternalResource<FGTextureProxy>("SSAODeinterleaveDepthArray", deinterleavedDepthMaps);

    RenderPass& deinterleavePass = FrameGraph.AddTask<RenderPass>("Deinterleave Depth Pass");
    deinterleavePass.SetRenderArea(quarterWidth, quarterHeight);
    deinterleavePass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    deinterleavePass.SetColorAttachments(
        {
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(0)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(1)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(2)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(3)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(4)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(5)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(6)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(7)},
        });
    deinterleavePass.AddSubpass({0, 1, 2, 3, 4, 5, 6, 7}, // color attachment refs
                                [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                                {
                                    struct DrawCall
                                    {
                                        Float2 UVOffset;
                                        Float2 InvFullResolution;
                                    };

                                    DrawCall* drawCall           = MapDrawCallConstants<DrawCall>();
                                    drawCall->UVOffset.X          = 0.5f;
                                    drawCall->UVOffset.Y          = 0.5f;
                                    drawCall->InvFullResolution = invFullResolution;

                                    rtbl->BindTexture(0, LinearDepth->Actual());

                                    DrawSAQ(RenderPassContext.pImmediateContext, DeinterleavePipe);
                                });

    RenderPass& deinterleavePass2 = FrameGraph.AddTask<RenderPass>("Deinterleave Depth Pass 2");
    deinterleavePass2.SetRenderArea(quarterWidth, quarterHeight);
    deinterleavePass2.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    deinterleavePass2.SetColorAttachments(
        {
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(8)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(9)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(10)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(11)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(12)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(13)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(14)},
            {TextureAttachment(SSAODeinterleaveDepthArray_R).SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE).SetSlice(15)},
        });
    deinterleavePass2.AddSubpass({0, 1, 2, 3, 4, 5, 6, 7}, // color attachment refs
                                 [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                                 {
                                     struct DrawCall
                                     {
                                         Float2 UVOffset;
                                         Float2 InvFullResolution;
                                     };

                                     DrawCall* drawCall           = MapDrawCallConstants<DrawCall>();
                                     drawCall->UVOffset.X          = float(8 % 4) + 0.5f;
                                     drawCall->UVOffset.Y          = float(8 / 4) + 0.5f;
                                     drawCall->InvFullResolution   = invFullResolution;

                                     rtbl->BindTexture(0, LinearDepth->Actual());

                                     DrawSAQ(RenderPassContext.pImmediateContext, DeinterleavePipe);
                                 });

    FGTextureProxy* DeinterleaveDepthArray_R = FrameGraph.AddExternalResource<FGTextureProxy>("Deinterleave Depth Array", deinterleavedDepthMaps);

    *ppDeinterleaveDepthArray = DeinterleaveDepthArray_R;
}

void SSAORenderer::AddCacheAwareAOPass(FrameGraph& FrameGraph, FGTextureProxy* DeinterleaveDepthArray, FGTextureProxy* NormalTexture, FGTextureProxy** ppSSAOTextureArray)
{
    ITexture* deinterleavedDepthMaps = GRenderView->HBAOMaps;
    HK_ASSERT(deinterleavedDepthMaps);

    uint32_t quarterWidth  = deinterleavedDepthMaps->GetDesc().Resolution.Width;
    uint32_t quarterHeight = deinterleavedDepthMaps->GetDesc().Resolution.Height;

    Float2 invFullResolution    = {1.0f / GRenderView->Width, 1.0f / GRenderView->Height};
    Float2 invQuarterResolution = {1.0f / quarterWidth, 1.0f / quarterHeight};
    float  aoHeight             = (float)GRenderView->Height;

    RenderPass& cacheAwareAO = FrameGraph.AddTask<RenderPass>("Cache Aware AO Pass");
    cacheAwareAO.SetRenderArea(quarterWidth, quarterHeight);
    cacheAwareAO.AddResource(DeinterleaveDepthArray, FG_RESOURCE_ACCESS_READ);
    cacheAwareAO.AddResource(NormalTexture, FG_RESOURCE_ACCESS_READ);
    cacheAwareAO.SetColorAttachment(
        TextureAttachment("SSAO Texture Array",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetResolution(TextureResolution2DArray(quarterWidth, quarterHeight, HBAO_RANDOM_ELEMENTS)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    cacheAwareAO.AddSubpass({0}, // color attachment refs
                            [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                            {
                                struct DrawCall
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

                                DrawCall* drawCall = MapDrawCallConstants<DrawCall>();

                                float projScale;

                                if (GRenderView->bPerspective)
                                {
                                    projScale = aoHeight / std::tan(GRenderView->ViewFovY * 0.5f) * 0.5f;
                                }
                                else
                                {
                                    projScale = aoHeight * GRenderView->ProjectionMatrix[1][1] * 0.5f;
                                }

                                drawCall->Bias                   = r_HBAOBias.GetFloat();
                                drawCall->FallofFactor           = -1.0f / (r_HBAORadius.GetFloat() * r_HBAORadius.GetFloat());
                                drawCall->RadiusToScreen         = r_HBAORadius.GetFloat() * 0.5f * projScale;
                                drawCall->PowExponent            = r_HBAOPowExponent.GetFloat();
                                drawCall->Multiplier             = 1.0f / (1.0f - r_HBAOBias.GetFloat());
                                drawCall->InvFullResolution      = invFullResolution;
                                drawCall->InvQuarterResolution   = invQuarterResolution;

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

void SSAORenderer::AddReinterleavePass(FrameGraph& FrameGraph, FGTextureProxy* SSAOTextureArray, FGTextureProxy** ppSSAOTexture)
{
    RenderPass& reinterleavePass = FrameGraph.AddTask<RenderPass>("Reinterleave Pass");
    reinterleavePass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    reinterleavePass.AddResource(SSAOTextureArray, FG_RESOURCE_ACCESS_READ);
    reinterleavePass.SetColorAttachment(
        TextureAttachment("SSAO Texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetResolution(TextureResolution2D(GRenderView->Width, GRenderView->Height))
                               .SetBindFlags(BIND_SHADER_RESOURCE))                               
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    reinterleavePass.AddSubpass({0}, // color attachment refs
                                [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                                {
                                    rtbl->BindTexture(0, SSAOTextureArray->Actual());

                                    DrawSAQ(RenderPassContext.pImmediateContext, ReinterleavePipe);
                                });

    *ppSSAOTexture = reinterleavePass.GetColorAttachments()[0].pResource;
}

void SSAORenderer::AddSimpleAOPass(FrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy* NormalTexture, FGTextureProxy** ppSSAOTexture)
{
    FGTextureProxy* RandomMapTexture_R = FrameGraph.AddExternalResource<FGTextureProxy>("SSAO Random Map", RandomMap);

    RenderPass& pass = FrameGraph.AddTask<RenderPass>("Simple AO Pass");
    pass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    pass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    pass.AddResource(NormalTexture, FG_RESOURCE_ACCESS_READ);
    pass.AddResource(RandomMapTexture_R, FG_RESOURCE_ACCESS_READ);
    pass.SetColorAttachment(
        TextureAttachment("SSAO Texture (Interleaved)",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetResolution(TextureResolution2D(GRenderView->Width, GRenderView->Height)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    pass.AddSubpass({0}, // color attachment refs
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        struct DrawCall
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

                        DrawCall* drawCall = MapDrawCallConstants<DrawCall>();

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
                        drawCall->InvFullResolution.X    = 1.0f / GRenderView->Width;
                        drawCall->InvFullResolution.Y    = 1.0f / GRenderView->Height;
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

void SSAORenderer::AddAOBlurPass(FrameGraph& FrameGraph, FGTextureProxy* SSAOTexture, FGTextureProxy* LinearDepth, FGTextureProxy** ppBluredSSAO)
{
    RenderPass& aoBlurXPass = FrameGraph.AddTask<RenderPass>("AO Blur X Pass");
    aoBlurXPass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    aoBlurXPass.SetColorAttachment(
        TextureAttachment("Temp SSAO Texture (Blur X)",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetResolution(TextureResolution2D(GRenderView->Width, GRenderView->Height)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    aoBlurXPass.AddResource(SSAOTexture, FG_RESOURCE_ACCESS_READ);
    aoBlurXPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    aoBlurXPass.AddSubpass({0}, // color attachment refs
                           [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                           {
                               struct DrawCall
                               {
                                   Float2 InvSize;
                               };

                               DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                               drawCall->InvSize.X = 1.0f / RenderPassContext.RenderArea.Width;
                               drawCall->InvSize.Y = 0;

                               // SSAO blur X
                               rtbl->BindTexture(0, SSAOTexture->Actual());
                               rtbl->BindTexture(1, LinearDepth->Actual());

                               DrawSAQ(RenderPassContext.pImmediateContext, BlurPipe);
                           });

    FGTextureProxy* TempSSAOTextureBlurX = aoBlurXPass.GetColorAttachments()[0].pResource;

    RenderPass& aoBlurYPass = FrameGraph.AddTask<RenderPass>("AO Blur Y Pass");
    aoBlurYPass.SetRenderArea(GRenderView->Width, GRenderView->Height);
    aoBlurYPass.SetColorAttachment(
        TextureAttachment("Blured SSAO Texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                               .SetResolution(TextureResolution2D(GRenderView->Width, GRenderView->Height)))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));
    aoBlurYPass.AddResource(TempSSAOTextureBlurX, FG_RESOURCE_ACCESS_READ);
    aoBlurYPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    aoBlurYPass.AddSubpass({0}, // color attachment refs
                           [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                           {
                               struct DrawCall
                               {
                                   Float2 InvSize;
                               };

                               DrawCall* drawCall = MapDrawCallConstants<DrawCall>();
                               drawCall->InvSize.X = 0;
                               drawCall->InvSize.Y = 1.0f / RenderPassContext.RenderArea.Height;

                               // SSAO blur Y
                               rtbl->BindTexture(0, TempSSAOTextureBlurX->Actual());
                               rtbl->BindTexture(1, LinearDepth->Actual());

                               DrawSAQ(RenderPassContext.pImmediateContext, BlurPipe);
                           });

    *ppBluredSSAO = aoBlurYPass.GetColorAttachments()[0].pResource;
}

void SSAORenderer::AddPasses(FrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy* NormalTexture, FGTextureProxy** ppSSAOTexture)
{
    if (r_HBAODeinterleaved && GRenderView->HBAOMaps)
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

HK_NAMESPACE_END
