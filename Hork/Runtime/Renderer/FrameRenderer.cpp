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
#include "FrameRenderer.h"
#include "VT/VirtualTextureFeedback.h"

#include <Hork/Core/Profiler.h>

HK_NAMESPACE_BEGIN

ConsoleVar r_ShowNormals("r_ShowNormals"_s, "0"_s, CVAR_CHEAT);
ConsoleVar r_ShowFeedbackVT("r_ShowFeedbackVT"_s, "0"_s);
ConsoleVar r_ShowCacheVT("r_ShowCacheVT"_s, "-1"_s);

using namespace RHI;

FrameRenderer::FrameRenderer()
{
    PipelineResourceLayout resourceLayout;

    BufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT;

    resourceLayout.NumBuffers = 1;
    resourceLayout.Buffers    = &bufferInfo;

    SamplerDesc nearestSampler;
    nearestSampler.Filter   = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SamplerDesc linearSampler;
    linearSampler.Filter   = FILTER_LINEAR;
    linearSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    linearSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    linearSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &nearestSampler;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_LinearDepthPipe, "postprocess/linear_depth.vert", "postprocess/linear_depth.frag", &resourceLayout);
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_LinearDepthPipe_ORTHO, "postprocess/linear_depth.vert", "postprocess/linear_depth_ortho.frag", &resourceLayout);

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_ReconstructNormalPipe, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal.frag", &resourceLayout);
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_ReconstructNormalPipe_ORTHO, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal_ortho.frag", &resourceLayout);

    SamplerDesc motionBlurSamplers[3];
    motionBlurSamplers[0] = linearSampler;
    motionBlurSamplers[1] = nearestSampler;
    motionBlurSamplers[2] = nearestSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(motionBlurSamplers);
    resourceLayout.Samplers    = motionBlurSamplers;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_MotionBlurPipeline, "postprocess/motionblur.vert", "postprocess/motionblur.frag", &resourceLayout);

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &linearSampler;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_OutlineBlurPipe, "postprocess/outlineblur.vert", "postprocess/outlineblur.frag", &resourceLayout);

    SamplerDesc outlineApplySamplers[2];
    outlineApplySamplers[0] = linearSampler;
    outlineApplySamplers[1] = linearSampler;

    resourceLayout.NumSamplers = HK_ARRAY_SIZE(outlineApplySamplers);
    resourceLayout.Samplers    = outlineApplySamplers;

    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_OutlineApplyPipe, "postprocess/outlineapply.vert", "postprocess/outlineapply.frag", &resourceLayout, RHI::BLENDING_ALPHA);

    resourceLayout = PipelineResourceLayout{};
    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers    = &nearestSampler;
    ShaderUtils::CreateFullscreenQuadPipeline(GDevice, &m_CopyPipeline, "postprocess/copy.vert", "postprocess/copy.frag", &resourceLayout);
    
}

void FrameRenderer::AddLinearizeDepthPass(FrameGraph& FrameGraph, FGTextureProxy* DepthTexture, FGTextureProxy** ppLinearDepth)
{
    RenderPass& linearizeDepthPass = FrameGraph.AddTask<RenderPass>("Linearize Depth Pass");
    linearizeDepthPass.SetRenderArea(GRenderViewArea);
    linearizeDepthPass.AddResource(DepthTexture, FG_RESOURCE_ACCESS_READ);
    linearizeDepthPass.SetColorAttachment(
        TextureAttachment("Linear depth texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_R32_FLOAT)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(RHI::ATTACHMENT_LOAD_OP_DONT_CARE));
    linearizeDepthPass.AddSubpass({0}, // color attachment refs
                                  [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                                  {
                                      using namespace RHI;

                                      rtbl->BindTexture(0, DepthTexture->Actual());

                                      if (GRenderView->bPerspective)
                                      {
                                          DrawSAQ(RenderPassContext.pImmediateContext, m_LinearDepthPipe);
                                      }
                                      else
                                      {
                                          DrawSAQ(RenderPassContext.pImmediateContext, m_LinearDepthPipe_ORTHO);
                                      }
                                  });
    *ppLinearDepth = linearizeDepthPass.GetColorAttachments()[0].pResource;
}

void FrameRenderer::AddReconstrutNormalsPass(FrameGraph& FrameGraph, FGTextureProxy* LinearDepth, FGTextureProxy** ppNormalTexture)
{
    RenderPass& reconstructNormalPass = FrameGraph.AddTask<RenderPass>("Reconstruct Normal Pass");
    reconstructNormalPass.SetRenderArea(GRenderViewArea);
    reconstructNormalPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
    reconstructNormalPass.SetColorAttachment(
        TextureAttachment("Normal texture",
                           TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_RGBA8_UNORM)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(RHI::ATTACHMENT_LOAD_OP_DONT_CARE));
    reconstructNormalPass.AddSubpass({0}, // color attachment refs
                                     [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                                     {
                                         using namespace RHI;

                                         rtbl->BindTexture(0, LinearDepth->Actual());

                                         if (GRenderView->bPerspective)
                                         {
                                             DrawSAQ(RenderPassContext.pImmediateContext, m_ReconstructNormalPipe);
                                         }
                                         else
                                         {
                                             DrawSAQ(RenderPassContext.pImmediateContext, m_ReconstructNormalPipe_ORTHO);
                                         }
                                     });
    *ppNormalTexture = reconstructNormalPass.GetColorAttachments()[0].pResource;
}

void FrameRenderer::AddMotionBlurPass(FrameGraph&     FrameGraph,
                                       FGTextureProxy*  LightTexture,
                                       FGTextureProxy*  VelocityTexture,
                                       FGTextureProxy*  LinearDepth,
                                       FGTextureProxy** ppResultTexture)
{
    RenderPass& renderPass = FrameGraph.AddTask<RenderPass>("Motion Blur Pass");

    renderPass.SetRenderArea(GRenderViewArea);

    renderPass.AddResource(LightTexture, FG_RESOURCE_ACCESS_READ);
    renderPass.AddResource(VelocityTexture, FG_RESOURCE_ACCESS_READ);
    renderPass.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);

    renderPass.SetColorAttachment(
        TextureAttachment("Motion blur texture",
                           LightTexture->GetResourceDesc()) // Use same format as light texture
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    renderPass.AddSubpass({0}, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)

                          {
                              rtbl->BindTexture(0, LightTexture->Actual());
                              rtbl->BindTexture(1, VelocityTexture->Actual());
                              rtbl->BindTexture(2, LinearDepth->Actual());

                              DrawSAQ(RenderPassContext.pImmediateContext, m_MotionBlurPipeline);
                          });

    *ppResultTexture = renderPass.GetColorAttachments()[0].pResource;
}

static bool BindMaterialOutlinePass(IImmediateContext* immediateCtx, RenderInstance const* instance)
{
    MaterialGPU* pMaterial = instance->Material;

    HK_ASSERT(pMaterial);

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline* pPipeline = pMaterial->Passes[bSkinned ? MaterialPass::OutlinePass_Skin : MaterialPass::OutlinePass];
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

void FrameRenderer::AddOutlinePass(FrameGraph& FrameGraph, FGTextureProxy** ppOutlineTexture)
{
    if (GRenderView->OutlineInstanceCount == 0)
    {
        *ppOutlineTexture = nullptr;
        return;
    }

    TEXTURE_FORMAT pf = TEXTURE_FORMAT_RG8_UNORM;

    RenderPass& maskPass = FrameGraph.AddTask<RenderPass>("Outline Pass");

    maskPass.SetRenderArea(GRenderViewArea);

    maskPass.SetColorAttachment(
        TextureAttachment("Outline mask",
                           TextureDesc()
                               .SetFormat(pf)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
            .SetClearValue(RHI::MakeClearColorValue(0.0f, 1.0f, 0.0f, 0.0f)));

    maskPass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            //for ( int i = 0 ; i < GRenderView->TerrainInstanceCount ; i++ ) {
                            //    TerrainRenderInstance const * instance = GFrameData->TerrainInstances[GRenderView->FirstTerrainInstance + i];

                            //    TerrainInstanceConstantBuffer * drawCall = MapDrawCallConstants< TerrainInstanceConstantBuffer >();
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
                            //                                    sizeof( DrawIndexedIndirectCmd ) );
                            //}

                            IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                            DrawIndexedCmd drawCmd;
                            drawCmd.InstanceCount         = 1;
                            drawCmd.StartInstanceLocation = 0;

                            for (int i = 0; i < GRenderView->OutlineInstanceCount; i++)
                            {
                                RenderInstance const* instance = GFrameData->OutlineInstances[GRenderView->FirstOutlineInstance + i];

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

void FrameRenderer::AddOutlineOverlayPass(FrameGraph& FrameGraph, FGTextureProxy* RenderTarget, FGTextureProxy* OutlineMaskTexture)
{
    TEXTURE_FORMAT pf = TEXTURE_FORMAT_RG8_UNORM;

    RenderPass& blurPass = FrameGraph.AddTask<RenderPass>("Outline Blur Pass");

    blurPass.SetRenderArea(GRenderViewArea);

    blurPass.AddResource(OutlineMaskTexture, FG_RESOURCE_ACCESS_READ);

    blurPass.SetColorAttachment(
        TextureAttachment("Outline blured mask",
                           TextureDesc()
                               .SetFormat(pf)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    blurPass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            using namespace RHI;

                            rtbl->BindTexture(0, OutlineMaskTexture->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, m_OutlineBlurPipe);
                        });

    FGTextureProxy* OutlineBlurTexture = blurPass.GetColorAttachments()[0].pResource;

    RenderPass& applyPass = FrameGraph.AddTask<RenderPass>("Outline Apply Pass");

    applyPass.SetRenderArea(GRenderViewArea);

    applyPass.AddResource(OutlineMaskTexture, FG_RESOURCE_ACCESS_READ);
    applyPass.AddResource(OutlineBlurTexture, FG_RESOURCE_ACCESS_READ);

    applyPass.SetColorAttachment(
        TextureAttachment(RenderTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    applyPass.AddSubpass({0}, // color attachment refs
                         [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                         {
                             using namespace RHI;

                             rtbl->BindTexture(0, OutlineMaskTexture->Actual());
                             rtbl->BindTexture(1, OutlineBlurTexture->Actual());

                             DrawSAQ(RenderPassContext.pImmediateContext, m_OutlineApplyPipe);
                         });
}

void FrameRenderer::AddCopyPass(FrameGraph& FrameGraph, FGTextureProxy* Source, FGTextureProxy* Dest)
{
    RenderPass& pass = FrameGraph.AddTask<RenderPass>("Copy Pass");

    pass.SetRenderArea(GRenderViewArea);

    pass.AddResource(Source, FG_RESOURCE_ACCESS_READ);

    pass.SetColorAttachment(
        TextureAttachment(Dest)
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    pass.AddSubpass({0}, // color attachment refs
                        [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            using namespace RHI;

                            rtbl->BindTexture(0, Source->Actual());

                            DrawSAQ(RenderPassContext.pImmediateContext, m_CopyPipeline);
                        });
}

void FrameRenderer::Render(FrameGraph& FrameGraph, bool bVirtualTexturing, VirtualTextureCache* PhysCacheVT)
{
    HK_PROFILER_EVENT("Framegraph build&fill");

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

        DirectionalLightInstance* dirLight = GFrameData->DirectionalLights[lightOffset];

        m_ShadowMapRenderer.AddPass(FrameGraph, dirLight, &ShadowMapDepth[lightIndex]);
    }
    for (int lightIndex = numDirLights; lightIndex < MAX_DIRECTIONAL_LIGHTS; lightIndex++)
    {
        m_ShadowMapRenderer.AddDummyShadowMap(FrameGraph, &ShadowMapDepth[lightIndex]);
    }

    FGTextureProxy* OmnidirectionalShadowMapArray;
    if (GRenderView->NumOmnidirectionalShadowMaps > 0)
    {
        m_ShadowMapRenderer.AddPass(FrameGraph, &GFrameData->LightShadowmaps[GRenderView->FirstOmnidirectionalShadowMap], GRenderView->NumOmnidirectionalShadowMaps, m_OmniShadowMapPool, &OmnidirectionalShadowMapArray);
    }
    else
    {
        m_ShadowMapRenderer.AddPass(FrameGraph, nullptr, 0, m_OmniShadowMapPool, &OmnidirectionalShadowMapArray);
    }

    FGTextureProxy *DepthTexture, *VelocityTexture;
    AddDepthPass(FrameGraph, &DepthTexture, &VelocityTexture);

    FGTextureProxy* LinearDepth;
    AddLinearizeDepthPass(FrameGraph, DepthTexture, &LinearDepth);

    FGTextureProxy* NormalTexture;
    AddReconstrutNormalsPass(FrameGraph, LinearDepth, &NormalTexture);

    FGTextureProxy* SSAOTexture;
    if (r_HBAO && GRenderView->bAllowHBAO)
    {
        m_SSAORenderer.AddPasses(FrameGraph, LinearDepth, NormalTexture, &SSAOTexture);
    }
    else
    {
        SSAOTexture = FrameGraph.AddExternalResource<FGTextureProxy>("White Texture", GWhiteTexture);
    }

    FGTextureProxy* LightTexture;
    m_LightRenderer.AddPass(FrameGraph, DepthTexture, SSAOTexture, ShadowMapDepth[0], ShadowMapDepth[1], ShadowMapDepth[2], ShadowMapDepth[3], OmnidirectionalShadowMapArray, LinearDepth, &LightTexture);

    if (GRenderView->AntialiasingType == ANTIALIASING_SMAA)
    {
        FGTextureProxy* AntialiasedTexture;
        m_SmaaRenderer.AddPass(FrameGraph, LightTexture, &AntialiasedTexture);
        LightTexture = AntialiasedTexture;
    }

    if (GRenderView->bAllowMotionBlur && GRenderView->FrameNumber > 0)
    {
        AddMotionBlurPass(FrameGraph, LightTexture, VelocityTexture, LinearDepth, &LightTexture);
    }

    BloomRenderer::Textures BloomTex;
    m_BloomRenderer.AddPasses(FrameGraph, LightTexture, &BloomTex);

    FGTextureProxy* Exposure;
    m_ExposureRenderer.AddPass(FrameGraph, LightTexture, &Exposure);

    FGTextureProxy* ColorGrading;
    m_ColorGradingRenderer.AddPass(FrameGraph, &ColorGrading);

    bool bFxaaPassRequired = GRenderView->AntialiasingType == ANTIALIASING_FXAA;

    FGTextureProxy* FinalTexture;

    if (bFxaaPassRequired)
    {
        m_PostprocessRenderer.AddPass(FrameGraph, LightTexture, Exposure, ColorGrading, BloomTex, TEXTURE_FORMAT_RGBA16_FLOAT, &FinalTexture);
    }
    else
    {
        FinalTexture = FrameGraph.AddExternalResource<FGTextureProxy>("RenderTarget", GRenderView->RenderTarget);
        m_PostprocessRenderer.AddPass(FrameGraph, LightTexture, Exposure, ColorGrading, BloomTex, FinalTexture);
    }

    // Apply outline
    FGTextureProxy* OutlineTexture;
    AddOutlinePass(FrameGraph, &OutlineTexture);
    if (OutlineTexture)
    {
        AddOutlineOverlayPass(FrameGraph, FinalTexture, OutlineTexture);
    }

    if (bFxaaPassRequired)
    {
        FGTextureProxy* FxaaTexture;
        m_FxaaRenderer.AddPass(FrameGraph, FinalTexture, &FxaaTexture);

        FinalTexture = FrameGraph.AddExternalResource<FGTextureProxy>("RenderTarget", GRenderView->RenderTarget);
        AddCopyPass(FrameGraph, FxaaTexture, FinalTexture);
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
        m_DebugDrawRenderer.AddPass(FrameGraph, FinalTexture, DepthTexture);
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
}

HK_NAMESPACE_END
