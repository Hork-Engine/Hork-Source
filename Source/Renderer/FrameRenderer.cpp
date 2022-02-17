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
#include "FrameRenderer.h"
#include "VT/VirtualTextureFeedback.h"

#include <Core/ScopedTimer.h>

extern AConsoleVar r_FXAA;

AConsoleVar r_ShowNormals(_CTS("r_ShowNormals"), _CTS("0"), CVAR_CHEAT);
AConsoleVar r_ShowFeedbackVT(_CTS("r_ShowFeedbackVT"), _CTS("0"));
AConsoleVar r_ShowCacheVT(_CTS("r_ShowCacheVT"), _CTS("-1"));

using namespace RenderCore;

AFrameRenderer::AFrameRenderer()
{
    SPipelineResourceLayout resourceLayout;

    SBufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT;

    resourceLayout.NumBuffers = 1;
    resourceLayout.Buffers    = &bufferInfo;

    SSamplerDesc nearestSampler;
    nearestSampler.Filter   = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SSamplerDesc linearSampler;
    linearSampler.Filter   = FILTER_LINEAR;
    linearSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    linearSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    linearSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &nearestSampler;

    CreateFullscreenQuadPipeline(&LinearDepthPipe, "postprocess/linear_depth.vert", "postprocess/linear_depth.frag", &resourceLayout);
    CreateFullscreenQuadPipeline(&LinearDepthPipe_ORTHO, "postprocess/linear_depth.vert", "postprocess/linear_depth_ortho.frag", &resourceLayout);

    CreateFullscreenQuadPipeline(&ReconstructNormalPipe, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal.frag", &resourceLayout);
    CreateFullscreenQuadPipeline(&ReconstructNormalPipe_ORTHO, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal_ortho.frag", &resourceLayout);

    SSamplerDesc motionBlurSamplers[3];
    motionBlurSamplers[0] = linearSampler;
    motionBlurSamplers[1] = nearestSampler;
    motionBlurSamplers[2] = nearestSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(motionBlurSamplers);
    resourceLayout.Samplers    = motionBlurSamplers;

    CreateFullscreenQuadPipeline(&MotionBlurPipeline, "postprocess/motionblur.vert", "postprocess/motionblur.frag", &resourceLayout);

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &linearSampler;

    CreateFullscreenQuadPipeline(&OutlineBlurPipe, "postprocess/outlineblur.vert", "postprocess/outlineblur.frag", &resourceLayout);

    SSamplerDesc outlineApplySamplers[2];
    outlineApplySamplers[0] = linearSampler;
    outlineApplySamplers[1] = linearSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(outlineApplySamplers);
    resourceLayout.Samplers    = outlineApplySamplers;

    //CreateFullscreenQuadPipeline( &OutlineApplyPipe, "postprocess/outlineapply.vert", "postprocess/outlineapply.frag", &resourceLayout, RenderCore::BLENDING_COLOR_ADD );
    CreateFullscreenQuadPipeline(&OutlineApplyPipe, "postprocess/outlineapply.vert", "postprocess/outlineapply.frag", &resourceLayout, RenderCore::BLENDING_ALPHA);
}

void AFrameRenderer::AddLinearizeDepthPass(AFrameGraph& FrameGraph, FGTextureProxy* DepthTexture, FGTextureProxy** ppLinearDepth)
{
    ARenderPass& linearizeDepthPass = FrameGraph.AddTask<ARenderPass>("Linearize Depth Pass");
    linearizeDepthPass.SetRenderArea(GRenderViewArea);
    linearizeDepthPass.AddResource(DepthTexture, FG_RESOURCE_ACCESS_READ);
    linearizeDepthPass.SetColorAttachment(
        STextureAttachment("Linear depth texture",
                           STextureDesc()
                               .SetFormat(RenderCore::TEXTURE_FORMAT_R32F)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
    linearizeDepthPass.AddSubpass({0}, // color attachment refs
                                  [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                                  {
                                      using namespace RenderCore;

                                      rtbl->BindTexture(0, DepthTexture->Actual());

                                      if (GRenderView->bPerspective)
                                      {
                                          DrawSAQ(RenderPassContext.pImmediateContext, LinearDepthPipe);
                                      }
                                      else
                                      {
                                          DrawSAQ(RenderPassContext.pImmediateContext, LinearDepthPipe_ORTHO);
                                      }
                                  });
    *ppLinearDepth = linearizeDepthPass.GetColorAttachments()[0].pResource;
}

void AFrameRenderer::AddReconstrutNormalsPass(AFrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy** ppNormalTexture)
{
    ARenderPass& reconstructNormalPass = FrameGraph.AddTask<ARenderPass>("Reconstruct Normal Pass");
    reconstructNormalPass.SetRenderArea(GRenderViewArea);
    reconstructNormalPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    reconstructNormalPass.SetColorAttachment(
        STextureAttachment("Normal texture",
                           STextureDesc()
                               .SetFormat(RenderCore::TEXTURE_FORMAT_RGB8)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE));
    reconstructNormalPass.AddSubpass({0}, // color attachment refs
                                     [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                                     {
                                         using namespace RenderCore;

                                         rtbl->BindTexture(0, LinearDepth->Actual());

                                         if (GRenderView->bPerspective)
                                         {
                                             DrawSAQ(RenderPassContext.pImmediateContext, ReconstructNormalPipe);
                                         }
                                         else
                                         {
                                             DrawSAQ(RenderPassContext.pImmediateContext, ReconstructNormalPipe_ORTHO);
                                         }
                                     });
    *ppNormalTexture = reconstructNormalPass.GetColorAttachments()[0].pResource;
}

void AFrameRenderer::AddMotionBlurPass(AFrameGraph&     FrameGraph,
                                       FGTextureProxy*  LightTexture,
                                       FGTextureProxy*  VelocityTexture,
                                       FGTextureProxy*  LinearDepth,
                                       FGTextureProxy** ppResultTexture)
{
    ARenderPass& renderPass = FrameGraph.AddTask<ARenderPass>("Motion Blur Pass");

    renderPass.SetRenderArea(GRenderViewArea);

    renderPass.AddResource(LightTexture, FG_RESOURCE_ACCESS_READ);
    renderPass.AddResource(VelocityTexture, FG_RESOURCE_ACCESS_READ);
    renderPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);

    renderPass.SetColorAttachment(
        STextureAttachment("Motion blur texture",
                           LightTexture->GetResourceDesc()) // Use same format as light texture
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    renderPass.AddSubpass({0}, // color attachment refs
                          [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)

                          {
                              rtbl->BindTexture(0, LightTexture->Actual());
                              rtbl->BindTexture(1, VelocityTexture->Actual());
                              rtbl->BindTexture(2, LinearDepth->Actual());

                              DrawSAQ(RenderPassContext.pImmediateContext, MotionBlurPipeline);
                          });

    *ppResultTexture = renderPass.GetColorAttachments()[0].pResource;
}

static bool BindMaterialOutlinePass(IImmediateContext* immediateCtx, SRenderInstance const* instance)
{
    AMaterialGPU* pMaterial = instance->Material;

    HK_ASSERT(pMaterial);

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline* pPipeline = pMaterial->OutlinePass[bSkinned];
    if (!pPipeline)
    {
        return false;
    }

    immediateCtx->BindPipeline(pPipeline);

    if (bSkinned)
    {
        immediateCtx->BindVertexBuffer(1, instance->WeightsBuffer, instance->WeightsBufferOffset);
    }
    else
    {
        immediateCtx->BindVertexBuffer(1, nullptr, 0);
    }

    BindVertexAndIndexBuffers(immediateCtx, instance);

    return true;
}

void AFrameRenderer::AddOutlinePass(AFrameGraph& FrameGraph, FGTextureProxy** ppOutlineTexture)
{
    if (GRenderView->OutlineInstanceCount == 0)
    {
        *ppOutlineTexture = nullptr;
        return;
    }

    RenderCore::TEXTURE_FORMAT pf = TEXTURE_FORMAT_RG8;

    ARenderPass& maskPass = FrameGraph.AddTask<ARenderPass>("Outline Pass");

    maskPass.SetRenderArea(GRenderViewArea);

    maskPass.SetColorAttachment(
        STextureAttachment("Outline mask",
                           STextureDesc()
                               .SetFormat(pf)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
            .SetClearValue(RenderCore::MakeClearColorValue(0.0f, 1.0f, 0.0f, 0.0f)));

    maskPass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            //for ( int i = 0 ; i < GRenderView->TerrainInstanceCount ; i++ ) {
                            //    STerrainRenderInstance const * instance = GFrameData->TerrainInstances[GRenderView->FirstTerrainInstance + i];

                            //    STerrainInstanceConstantBuffer * drawCall = MapDrawCallConstants< STerrainInstanceConstantBuffer >();
                            //    drawCall->LocalViewProjection = instance->LocalViewProjection;
                            //    StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, drawCall->ModelNormalToViewSpace );
                            //    drawCall->ViewPositionAndHeight = instance->ViewPositionAndHeight;
                            //    drawCall->TerrainClipMin = instance->ClipMin;
                            //    drawCall->TerrainClipMax = instance->ClipMax;

                            //    rtbl->BindTexture( 0, instance->Clipmaps );
                            //    immediateCtx->BindPipeline( GTerrainOutlinePipeline );
                            //    immediateCtx->BindVertexBuffer( 0, instance->VertexBuffer );
                            //    immediateCtx->BindVertexBuffer( 1, GStreamBuffer, instance->InstanceBufferStreamHandle );
                            //    immediateCtx->BindIndexBuffer( instance->IndexBuffer, INDEX_TYPE_UINT16 );
                            //    immediateCtx->MultiDrawIndexedIndirect( instance->IndirectBufferDrawCount,
                            //                                    GStreamBuffer,
                            //                                    instance->IndirectBufferStreamHandle,
                            //                                    sizeof( SDrawIndexedIndirectCmd ) );
                            //}

                            IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                            SDrawIndexedCmd drawCmd;
                            drawCmd.InstanceCount         = 1;
                            drawCmd.StartInstanceLocation = 0;

                            for (int i = 0; i < GRenderView->OutlineInstanceCount; i++)
                            {
                                SRenderInstance const* instance = GFrameData->OutlineInstances[GRenderView->FirstOutlineInstance + i];

                                if (!BindMaterialOutlinePass(immediateCtx, instance))
                                {
                                    continue;
                                }

                                BindTextures(instance->MaterialInstance, instance->Material->DepthPassTextureCount);
                                BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                                BindInstanceConstants(instance);

                                drawCmd.IndexCountPerInstance = instance->IndexCount;
                                drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                                drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                                immediateCtx->Draw(&drawCmd);
                            }
                        });

    *ppOutlineTexture = maskPass.GetColorAttachments()[0].pResource;
}

void AFrameRenderer::AddOutlineOverlayPass(AFrameGraph& FrameGraph, FGTextureProxy* RenderTarget, FGTextureProxy* OutlineMaskTexture)
{
    RenderCore::TEXTURE_FORMAT pf = TEXTURE_FORMAT_RG8;

    ARenderPass& blurPass = FrameGraph.AddTask<ARenderPass>("Outline Blur Pass");

    blurPass.SetRenderArea(GRenderViewArea);

    blurPass.AddResource(OutlineMaskTexture, FG_RESOURCE_ACCESS_READ);

    blurPass.SetColorAttachment(
        STextureAttachment("Outline blured mask",
                           STextureDesc()
                               .SetFormat(pf)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    blurPass.AddSubpass({0}, // color attachment refs
                        [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                        {
                            using namespace RenderCore;

                            rtbl->BindTexture(0, OutlineMaskTexture->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, OutlineBlurPipe);
                        });

    FGTextureProxy* OutlineBlurTexture = blurPass.GetColorAttachments()[0].pResource;

    ARenderPass& applyPass = FrameGraph.AddTask<ARenderPass>("Outline Apply Pass");

    applyPass.SetRenderArea(GRenderViewArea);

    applyPass.AddResource(OutlineMaskTexture, FG_RESOURCE_ACCESS_READ);
    applyPass.AddResource(OutlineBlurTexture, FG_RESOURCE_ACCESS_READ);

    applyPass.SetColorAttachment(
        STextureAttachment(RenderTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    applyPass.AddSubpass({0}, // color attachment refs
                         [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                         {
                             using namespace RenderCore;

                             rtbl->BindTexture(0, OutlineMaskTexture->Actual());
                             rtbl->BindTexture(1, OutlineBlurTexture->Actual());

                             DrawSAQ(RenderPassContext.pImmediateContext, OutlineApplyPipe);
                         });
}

void AFrameRenderer::Render(AFrameGraph& FrameGraph, bool bVirtualTexturing, AVirtualTextureCache* PhysCacheVT, FGTextureProxy** ppFinalTexture)
{
    AScopedTimer TimeCheck("Framegraph build&fill");

    if (bVirtualTexturing)
    {
        GRenderView->VTFeedback->AddPass(FrameGraph);
    }

    FGTextureProxy* ShadowMapDepth[MAX_DIRECTIONAL_LIGHTS] = {};

    int numDirLights = GRenderView->NumDirectionalLights;
    if (numDirLights > MAX_DIRECTIONAL_LIGHTS)
    {
        LOG("GRenderView->NumDirectionalLights > MAX_DIRECTIONAL_LIGHTS\n");

        numDirLights = MAX_DIRECTIONAL_LIGHTS;
    }

    for (int lightIndex = 0; lightIndex < numDirLights; lightIndex++)
    {
        int lightOffset = GRenderView->FirstDirectionalLight + lightIndex;

        SDirectionalLightInstance* dirLight = GFrameData->DirectionalLights[lightOffset];

        ShadowMapRenderer.AddPass(FrameGraph, dirLight, &ShadowMapDepth[lightIndex]);
    }
    for (int lightIndex = numDirLights; lightIndex < MAX_DIRECTIONAL_LIGHTS; lightIndex++)
    {
        ShadowMapRenderer.AddDummyShadowMap(FrameGraph, &ShadowMapDepth[lightIndex]);
    }


    FGTextureProxy *DepthTexture, *VelocityTexture;
    AddDepthPass(FrameGraph, &DepthTexture, &VelocityTexture);

    FGTextureProxy* LinearDepth;
    AddLinearizeDepthPass(FrameGraph, DepthTexture, &LinearDepth);

    FGTextureProxy* NormalTexture;
    AddReconstrutNormalsPass(FrameGraph, LinearDepth, &NormalTexture);

    FGTextureProxy* SSAOTexture;
    if (r_HBAO)
    {
        SSAORenderer.AddPasses(FrameGraph, LinearDepth, NormalTexture, &SSAOTexture);
    }
    else
    {
        SSAOTexture = FrameGraph.AddExternalResource<FGTextureProxy>("White Texture", GWhiteTexture);
    }

    FGTextureProxy* LightTexture;
    LightRenderer.AddPass(FrameGraph, DepthTexture, SSAOTexture, ShadowMapDepth[0], ShadowMapDepth[1], ShadowMapDepth[2], ShadowMapDepth[3], LinearDepth, &LightTexture);

    if (r_MotionBlur)
    {
        AddMotionBlurPass(FrameGraph, LightTexture, VelocityTexture, LinearDepth, &LightTexture);
    }

    ABloomRenderer::STextures BloomTex;
    BloomRenderer.AddPasses(FrameGraph, LightTexture, &BloomTex);

    FGTextureProxy* Exposure;
    ExposureRenderer.AddPass(FrameGraph, LightTexture, &Exposure);

    FGTextureProxy* ColorGrading;
    ColorGradingRenderer.AddPass(FrameGraph, &ColorGrading);

    FGTextureProxy* PostprocessTexture;
    PostprocessRenderer.AddPass(FrameGraph, LightTexture, Exposure, ColorGrading, BloomTex, &PostprocessTexture);

    FGTextureProxy* OutlineTexture;
    AddOutlinePass(FrameGraph, &OutlineTexture);

    if (OutlineTexture)
    {
        AddOutlineOverlayPass(FrameGraph, PostprocessTexture, OutlineTexture);
    }

    FGTextureProxy* FinalTexture;
    if (r_FXAA)
    {
        FxaaRenderer.AddPass(FrameGraph, PostprocessTexture, &FinalTexture);
    }
    else
    {
        FinalTexture = PostprocessTexture;
    }

    if (GRenderView->bWireframe)
    {
        AddWireframePass(FrameGraph, FinalTexture);
    }

    if (r_ShowNormals)
    {
        AddNormalsPass(FrameGraph, FinalTexture);
    }

    if (GRenderView->DebugDrawCommandCount > 0)
    {
        DebugDrawRenderer.AddPass(FrameGraph, FinalTexture, DepthTexture);
    }

    if (bVirtualTexturing)
    {
        if (r_ShowFeedbackVT)
        {
            GRenderView->VTFeedback->DrawFeedback(FrameGraph, FinalTexture);
        }

        if (r_ShowCacheVT.GetInteger() >= 0)
        {
            PhysCacheVT->Draw(FrameGraph, FinalTexture, r_ShowCacheVT.GetInteger());
        }
    }

    *ppFinalTexture = FinalTexture;
}
