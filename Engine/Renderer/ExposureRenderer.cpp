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

#include "ExposureRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

ConsoleVar r_ShowDefaultExposure("r_ShowDefaultExposure"_s, "0"_s);

using namespace RenderCore;

ExposureRenderer::ExposureRenderer()
{
    // Create textures
    TextureDesc textureDesc;
    textureDesc.SetMipLevels(1);
    textureDesc.SetBindFlags(BIND_SHADER_RESOURCE);

    textureDesc.SetFormat(TEXTURE_FORMAT_RG16_FLOAT);

    textureDesc.SetResolution(TextureResolution2D(64, 64));
    GDevice->CreateTexture(textureDesc, &Luminance64);

    textureDesc.SetResolution(TextureResolution2D(32, 32));
    GDevice->CreateTexture(textureDesc, &Luminance32);

    textureDesc.SetResolution(TextureResolution2D(16, 16));
    GDevice->CreateTexture(textureDesc, &Luminance16);

    textureDesc.SetResolution(TextureResolution2D(8, 8));
    GDevice->CreateTexture(textureDesc, &Luminance8);

    textureDesc.SetResolution(TextureResolution2D(4, 4));
    GDevice->CreateTexture(textureDesc, &Luminance4);

    textureDesc.SetResolution(TextureResolution2D(2, 2));
    GDevice->CreateTexture(textureDesc, &Luminance2);

    byte defaultLum[2] = {30, 30}; // TODO: choose appropriate value

    textureDesc.SetResolution(TextureResolution2D(1, 1));
    textureDesc.SetFormat(TEXTURE_FORMAT_RG8_UNORM);
    GDevice->CreateTexture(textureDesc, &DefaultLuminance);
    DefaultLuminance->Write(0, sizeof(defaultLum), 1, defaultLum);

    SamplerDesc samplerCI;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    samplerCI.Filter = FILTER_LINEAR;

    BufferInfo bufferInfo[1];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT;

    PipelineResourceLayout resourceLayout;
    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &samplerCI;

    resourceLayout.NumBuffers = HK_ARRAY_SIZE(bufferInfo);
    resourceLayout.Buffers = bufferInfo;

    ShaderFactory::CreateFullscreenQuadPipeline(&MakeLuminanceMapPipe, "postprocess/exposure/make_luminance.vert", "postprocess/exposure/make_luminance.frag", &resourceLayout);
    ShaderFactory::CreateFullscreenQuadPipeline(&DynamicExposurePipe, "postprocess/exposure/dynamic_exposure.vert", "postprocess/exposure/dynamic_exposure.frag", &resourceLayout, BLENDING_ALPHA);

    resourceLayout.NumBuffers = 0;

    ShaderFactory::CreateFullscreenQuadPipeline(&SumLuminanceMapPipe, "postprocess/exposure/sum_luminance.vert", "postprocess/exposure/sum_luminance.frag", &resourceLayout);
}

void ExposureRenderer::AddPass(FrameGraph& FrameGraph, FGTextureProxy* SourceTexture, FGTextureProxy** ppExposure)
{
    ITexture* exposureTexture = GRenderView->CurrentExposure;

    if (!exposureTexture || r_ShowDefaultExposure)
    {
        *ppExposure = FrameGraph.AddExternalResource<FGTextureProxy>("Fallback exposure texture", DefaultLuminance);
        return;
    }

    auto Exposure_R = FrameGraph.AddExternalResource<FGTextureProxy>("Exposure texture", exposureTexture);
    auto Luminance64_R = FrameGraph.AddExternalResource<FGTextureProxy>("Luminance64", Luminance64);
    auto Luminance32_R = FrameGraph.AddExternalResource<FGTextureProxy>("Luminance32", Luminance32);
    auto Luminance16_R = FrameGraph.AddExternalResource<FGTextureProxy>("Luminance16", Luminance16);
    auto Luminance8_R = FrameGraph.AddExternalResource<FGTextureProxy>("Luminance8", Luminance8);
    auto Luminance4_R = FrameGraph.AddExternalResource<FGTextureProxy>("Luminance4", Luminance4);
    auto Luminance2_R = FrameGraph.AddExternalResource<FGTextureProxy>("Luminance2", Luminance2);

    // Make luminance map 64x64
    FrameGraph.AddTask<RenderPass>("Make luminance map 64x64")
        .SetRenderArea(64, 64)
        .SetColorAttachment(TextureAttachment(Luminance64_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(SourceTexture, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, SourceTexture->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, MakeLuminanceMapPipe);
                    });

    // Downscale luminance to 32x32
    FrameGraph.AddTask<RenderPass>("Downscale luminance to 32x32")
        .SetRenderArea(32, 32)
        .SetColorAttachment(TextureAttachment(Luminance32_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(Luminance64_R, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, Luminance64_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, SumLuminanceMapPipe);
                    });

    // Downscale luminance to 16x16
    FrameGraph.AddTask<RenderPass>("Downscale luminance to 16x16")
        .SetRenderArea(16, 16)
        .SetColorAttachment(TextureAttachment(Luminance16_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(Luminance32_R, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, Luminance32_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, SumLuminanceMapPipe);
                    });

    // Downscale luminance to 8x8
    FrameGraph.AddTask<RenderPass>("Downscale luminance to 8x8")
        .SetRenderArea(8, 8)
        .SetColorAttachment(TextureAttachment(Luminance8_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(Luminance16_R, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, Luminance16_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, SumLuminanceMapPipe);
                    });

    // Downscale luminance to 4x4
    FrameGraph.AddTask<RenderPass>("Downscale luminance to 4x4")
        .SetRenderArea(4, 4)
        .SetColorAttachment(TextureAttachment(Luminance4_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(Luminance8_R, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, Luminance8_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, SumLuminanceMapPipe);
                    });

    // Downscale luminance to 2x2
    FrameGraph.AddTask<RenderPass>("Downscale luminance to 2x2")
        .SetRenderArea(2, 2)
        .SetColorAttachment(TextureAttachment(Luminance2_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(Luminance4_R, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, Luminance4_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, SumLuminanceMapPipe);
                    });

    // Render final exposure
    FrameGraph.AddTask<RenderPass>("Render final exposure")
        .SetRenderArea(1, 1)
        .SetColorAttachment(TextureAttachment(Exposure_R)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE))
        .AddResource(Luminance2_R, FG_RESOURCE_ACCESS_READ)
        .AddSubpass({0},
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        rtbl->BindTexture(0, Luminance2_R->Actual());

                        DrawSAQ(RenderPassContext.pImmediateContext, DynamicExposurePipe);
                    });

    *ppExposure = Exposure_R;
}

HK_NAMESPACE_END
