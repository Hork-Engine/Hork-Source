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

#include "ColorGradingRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

ColorGradingRenderer::ColorGradingRenderer()
{
    SamplerDesc samplerCI;
    samplerCI.Filter = FILTER_NEAREST; // FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;

    PipelineResourceLayout resourceLayout;
    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &samplerCI;

    BufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants

    resourceLayout.Buffers = bufferInfo;

    resourceLayout.NumBuffers = 1;
    ShaderFactory::sCreateFullscreenQuadPipelineGS(&PipelineLUT,
                                                   "postprocess/colorgrading.vert",
                                                   "postprocess/colorgrading.frag",
                                                   "postprocess/colorgrading.geom",
                                                   &resourceLayout,
                                                   RenderCore::BLENDING_ALPHA);

    resourceLayout.NumBuffers = 2;
    ShaderFactory::sCreateFullscreenQuadPipelineGS(&PipelineProcedural,
                                                   "postprocess/colorgrading.vert",
                                                   "postprocess/colorgrading_procedural.frag",
                                                   "postprocess/colorgrading.geom",
                                                   &resourceLayout,
                                                   RenderCore::BLENDING_ALPHA);
}

void ColorGradingRenderer::AddPass(FrameGraph& FrameGraph, FGTextureProxy** ppColorGrading)
{
    if (!GRenderView->CurrentColorGradingLUT)
    {
        *ppColorGrading = nullptr;
        return;
    }

    auto ColorGrading_R = FrameGraph.AddExternalResource<FGTextureProxy>("CurrentColorGradingLUT", GRenderView->CurrentColorGradingLUT);

    if (GRenderView->ColorGradingLUT)
    {
        auto source = FrameGraph.AddExternalResource<FGTextureProxy>("ColorGradingLUT", GRenderView->ColorGradingLUT);

        RenderPass& renderPass = FrameGraph.AddTask<RenderPass>("Color Grading Pass");

        renderPass.SetRenderArea(16, 16);
        renderPass.SetColorAttachments(
            {{TextureAttachment(ColorGrading_R)
                  .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD)}});
        renderPass.AddResource(source, FG_RESOURCE_ACCESS_READ);
        renderPass.AddSubpass({0},
                              [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer) {
                                  rtbl->BindTexture(0, source->Actual());

                                  DrawSAQ(RenderPassContext.pImmediateContext, PipelineLUT);
                              });
    }
    else
    {
        RenderPass& renderPass = FrameGraph.AddTask<RenderPass>("Color Grading Procedural Pass");

        renderPass.SetRenderArea(16, 16);
        renderPass.SetColorAttachments(
            {{TextureAttachment(ColorGrading_R)
                  .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD)}});
        renderPass.AddSubpass({0},
                              [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer) {
                                  struct DrawCall
                                  {
                                      Float4 TemperatureScale;
                                      Float4 TemperatureStrength;
                                      Float4 Grain;
                                      Float4 Gamma;
                                      Float4 Lift;
                                      Float4 Presaturation;
                                      Float4 LuminanceNormalization;
                                  };

                                  DrawCall* drawCall = MapDrawCallConstants<DrawCall>();

                                  drawCall->TemperatureScale.X = GRenderView->ColorGradingTemperatureScale.X;
                                  drawCall->TemperatureScale.Y = GRenderView->ColorGradingTemperatureScale.Y;
                                  drawCall->TemperatureScale.Z = GRenderView->ColorGradingTemperatureScale.Z;
                                  drawCall->TemperatureScale.W = 0.0f;

                                  drawCall->TemperatureStrength.X = GRenderView->ColorGradingTemperatureStrength.X;
                                  drawCall->TemperatureStrength.Y = GRenderView->ColorGradingTemperatureStrength.Y;
                                  drawCall->TemperatureStrength.Z = GRenderView->ColorGradingTemperatureStrength.Z;
                                  drawCall->TemperatureStrength.W = 0.0f;

                                  drawCall->Grain.X = GRenderView->ColorGradingGrain.X * 2.0f;
                                  drawCall->Grain.Y = GRenderView->ColorGradingGrain.Y * 2.0f;
                                  drawCall->Grain.Z = GRenderView->ColorGradingGrain.Z * 2.0f;
                                  drawCall->Grain.W = 0.0f;

                                  drawCall->Gamma.X = 0.5f / Math::Max(GRenderView->ColorGradingGamma.X, 0.0001f);
                                  drawCall->Gamma.Y = 0.5f / Math::Max(GRenderView->ColorGradingGamma.Y, 0.0001f);
                                  drawCall->Gamma.Z = 0.5f / Math::Max(GRenderView->ColorGradingGamma.Z, 0.0001f);
                                  drawCall->Gamma.W = 0.0f;

                                  drawCall->Lift.X = GRenderView->ColorGradingLift.X * 2.0f - 1.0f;
                                  drawCall->Lift.Y = GRenderView->ColorGradingLift.Y * 2.0f - 1.0f;
                                  drawCall->Lift.Z = GRenderView->ColorGradingLift.Z * 2.0f - 1.0f;
                                  drawCall->Lift.W = 0.0f;

                                  drawCall->Presaturation.X = GRenderView->ColorGradingPresaturation.X;
                                  drawCall->Presaturation.Y = GRenderView->ColorGradingPresaturation.Y;
                                  drawCall->Presaturation.Z = GRenderView->ColorGradingPresaturation.Z;
                                  drawCall->Presaturation.W = 0.0f;

                                  drawCall->LuminanceNormalization.X = GRenderView->ColorGradingBrightnessNormalization;
                                  drawCall->LuminanceNormalization.Y = 0.0f;
                                  drawCall->LuminanceNormalization.Z = 0.0f;
                                  drawCall->LuminanceNormalization.W = 0.0f;

                                  DrawSAQ(RenderPassContext.pImmediateContext, PipelineProcedural);
                              });
    }

    *ppColorGrading = ColorGrading_R;
}

HK_NAMESPACE_END
