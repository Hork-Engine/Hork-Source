#include "ShadowMapRenderer.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

ConsoleVar r_ShadowCascadeBits("r_ShadowCascadeBits"s, "32"s); // Allowed 16, 32 bits

static const float  EVSM_positiveExponent = 40.0;
static const float  EVSM_negativeExponent = 5.0;
static const Float2 EVSM_WarpDepth(std::exp(EVSM_positiveExponent), -std::exp(-EVSM_negativeExponent));
const Float4        EVSM_ClearValue(EVSM_WarpDepth.X, EVSM_WarpDepth.Y, EVSM_WarpDepth.X* EVSM_WarpDepth.X, EVSM_WarpDepth.Y* EVSM_WarpDepth.Y);
const Float4        VSM_ClearValue(1.0f);

ShadowMapRenderer::ShadowMapRenderer()
{
    CreatePipeline();
    CreateLightPortalPipeline();

    GDevice->CreateTexture(TextureDesc()
                               .SetFormat(TEXTURE_FORMAT_D16)
                               .SetResolution(TextureResolution2DArray(1, 1, 1))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &DummyShadowMap);
    DummyShadowMap->SetDebugName("Dummy Shadow Map");

    ClearValue clearValue;
    clearValue.Float1.R = 1.0f;
    rcmd->ClearTexture(DummyShadowMap, 0, FORMAT_FLOAT1, &clearValue);
}

void ShadowMapRenderer::CreatePipeline()
{
    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
#if defined SHADOWMAP_VSM
    //Desc.CullMode = POLYGON_CULL_FRONT; // Less light bleeding
    Desc.CullMode = POLYGON_CULL_DISABLED;
#else
    //rsd.CullMode = POLYGON_CULL_BACK;
    rsd.CullMode = POLYGON_CULL_DISABLED; // Less light bleeding
                                          //rsd.CullMode = POLYGON_CULL_FRONT;
#endif
    //rsd.CullMode = POLYGON_CULL_DISABLED;

    //BlendingStateInfo & bsd = pipelineCI.BS;
    //bsd.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;  // FIXME: there is no fragment shader, so we realy need to disable color mask?
#if defined SHADOWMAP_VSM
    bsd.RenderTargetSlots[0].SetBlendingPreset(BLENDING_NO_BLEND);
#endif

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc               = CMPFUNC_LESS;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride    = sizeof(Float3);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings   = vertexBinding;

    constexpr VertexAttribInfo vertexAttribs[] = {
        {"InPosition",
         0, // location
         0, // buffer input slot
         VAT_FLOAT3,
         VAM_FLOAT,
         0, // InstanceDataStepRate
         0}};

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(vertexAttribs);
    pipelineCI.pVertexAttribs   = vertexAttribs;

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology                    = PRIMITIVE_TRIANGLES;

    ShaderFactory::CreateVertexShader("instance_shadowmap_default.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS);
    ShaderFactory::CreateGeometryShader("instance_shadowmap_default.geom", pipelineCI.pGS);

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if (/*_ShadowMasking || */ bVSM)
    {
        ShaderFactory::CreateFragmentShader("instance_shadowmap_default.frag", pipelineCI.pFS);
    }

    BufferInfo bufferInfo[4];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    bufferInfo[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    bufferInfo[3].BufferBinding = BUFFER_BIND_CONSTANT; // cascade matrix

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(bufferInfo);
    pipelineCI.ResourceLayout.Buffers    = bufferInfo;

    GDevice->CreatePipeline(pipelineCI, &StaticShadowCasterPipeline);
}

void ShadowMapRenderer::CreateLightPortalPipeline()
{
    PipelineDesc pipelineCI;

    RasterizerStateInfo& rsd = pipelineCI.RS;
    rsd.bScissorEnable        = false;
    rsd.CullMode              = POLYGON_CULL_FRONT;

    DepthStencilStateInfo& dssd = pipelineCI.DSS;
    dssd.DepthFunc               = CMPFUNC_GREATER; //CMPFUNC_LESS;
    dssd.bDepthEnable            = true;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride    = sizeof(Float3);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings   = vertexBinding;

    constexpr VertexAttribInfo vertexAttribs[] = {
        {"InPosition",
         0, // location
         0, // buffer input slot
         VAT_FLOAT3,
         VAM_FLOAT,
         0, // InstanceDataStepRate
         0}};

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(vertexAttribs);
    pipelineCI.pVertexAttribs   = vertexAttribs;

    PipelineInputAssemblyInfo& inputAssembly = pipelineCI.IA;
    inputAssembly.Topology                    = PRIMITIVE_TRIANGLES;

    ShaderFactory::CreateVertexShader("instance_lightportal.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS);
    ShaderFactory::CreateGeometryShader("instance_lightportal.geom", pipelineCI.pGS);

#if 0
    CreateFragmentShader( "instance_lightportal.frag", pipelineCI.pFS );
#endif

    BufferInfo bufferInfo[4];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants (unused)
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants (unused)
    bufferInfo[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton (unused)
    bufferInfo[3].BufferBinding = BUFFER_BIND_CONSTANT; // cascade matrix

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(bufferInfo);
    pipelineCI.ResourceLayout.Buffers    = bufferInfo;

    GDevice->CreatePipeline(pipelineCI, &LightPortalPipeline);
}

bool ShadowMapRenderer::BindMaterialShadowMap(IImmediateContext* immediateCtx, ShadowRenderInstance const* instance)
{
    MaterialGPU* pMaterial = instance->Material;

    if (pMaterial)
    {
        int bSkinned = instance->SkeletonSize > 0;

        IPipeline* pPipeline = pMaterial->ShadowPass[bSkinned];
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

        BindTextures(instance->MaterialInstance, pMaterial->ShadowMapPassTextureCount);
    }
    else
    {
        immediateCtx->BindPipeline(StaticShadowCasterPipeline);
        immediateCtx->BindVertexBuffer(1, nullptr, 0);
    }

    BindVertexAndIndexBuffers(immediateCtx, instance);

    return true;
}

bool ShadowMapRenderer::BindMaterialOmniShadowMap(IImmediateContext* immediateCtx, ShadowRenderInstance const* instance)
{
    MaterialGPU* pMaterial = instance->Material;

    if (pMaterial)
    {
        int bSkinned = instance->SkeletonSize > 0;

        IPipeline* pPipeline = pMaterial->OmniShadowPass[bSkinned];
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

        BindTextures(instance->MaterialInstance, pMaterial->ShadowMapPassTextureCount);
    }
    else
    {
        immediateCtx->BindPipeline(StaticShadowCasterPipeline);
        immediateCtx->BindVertexBuffer(1, nullptr, 0);
    }

    BindVertexAndIndexBuffers(immediateCtx, instance);

    return true;
}

#if defined SHADOWMAP_VSM
static void BlurDepthMoments()
{
    GHI_Framebuffer_t* FB = RenderFrame.Statement->GetFramebuffer<SHADOWMAP_FRAMEBUFFER>();

    GHI_SetDepthStencilTarget(RenderFrame.State, FB, NULL);
    GHI_SetDepthStencilState(RenderFrame.State, FDepthStencilState<false, GHI_DepthWrite_Disable>().HardwareState());
    GHI_SetRasterizerState(RenderFrame.State, FRasterizerState<GHI_FillSolid, GHI_CullFront>().HardwareState());
    GHI_SetSampler(RenderFrame.State, 0, ShadowDepthSampler1);

    GHI_SetInputAssembler(RenderFrame.State, RenderFrame.SaqIA);
    GHI_SetPrimitiveTopology(RenderFrame.State, GHI_Prim_TriangleStrip);

    GHI_Viewport_t Viewport;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.MinDepth = 0;
    Viewport.MaxDepth = 1;

    // Render horizontal blur

    GHI_Texture_t* DepthMomentsTextureTmp = ShadowPool_DepthMomentsVSMBlur();

    GHI_SetFramebufferSize(RenderFrame.State, FB, DepthMomentsTextureTmp->GetDesc().Width, DepthMomentsTextureTmp->GetDesc().Height);
    Viewport.Width  = DepthMomentsTextureTmp->GetDesc().Width;
    Viewport.Height = DepthMomentsTextureTmp->GetDesc().Height;
    GHI_SetViewport(RenderFrame.State, &Viewport);

    RenderTargetVSM.Texture = DepthMomentsTextureTmp;
    GHI_SetRenderTargets(RenderFrame.State, FB, 1, &RenderTargetVSM);
    GHI_SetProgramPipeline(RenderFrame.State, RenderFrame.Statement->GetProgramAndAttach<FGaussianBlur9HProgram, FSaqVertex>());
    GHI_AssignTextureUnit(RenderFrame.State, 0, ShadowPool_DepthMomentsVSM());
    GHI_DrawInstanced(RenderFrame.State, 4, NumShadowMapCascades, 0);

    // Render vertical blur

    GHI_SetFramebufferSize(RenderFrame.State, FB, r_ShadowCascadeResolution.GetInteger(), r_ShadowCascadeResolution.GetInteger());
    Viewport.Width  = r_ShadowCascadeResolution.GetInteger();
    Viewport.Height = r_ShadowCascadeResolution.GetInteger();
    GHI_SetViewport(RenderFrame.State, &Viewport);

    RenderTargetVSM.Texture = ShadowPool_DepthMomentsVSM();
    GHI_SetRenderTargets(RenderFrame.State, FB, 1, &RenderTargetVSM);
    GHI_SetProgramPipeline(RenderFrame.State, RenderFrame.Statement->GetProgramAndAttach<FGaussianBlur9VProgram, FSaqVertex>());
    GHI_AssignTextureUnit(RenderFrame.State, 0, DepthMomentsTextureTmp);
    GHI_DrawInstanced(RenderFrame.State, 4, NumShadowMapCascades, 0);
}
#endif

void ShadowMapRenderer::AddDummyShadowMap(FrameGraph& FrameGraph, FGTextureProxy** ppShadowMapDepth)
{
    *ppShadowMapDepth = FrameGraph.AddExternalResource<FGTextureProxy>("Dummy Shadow Map", DummyShadowMap);
}

void ShadowMapRenderer::AddPass(FrameGraph& FrameGraph, DirectionalLightInstance const* Light, FGTextureProxy** ppShadowMapDepth)
{
    if (Light->ShadowmapIndex < 0)
    {
        AddDummyShadowMap(FrameGraph, ppShadowMapDepth);
        return;
    }

    LightShadowmap const* shadowMap = &GFrameData->LightShadowmaps[Light->ShadowmapIndex];
    if (shadowMap->ShadowInstanceCount == 0)
    {
        AddDummyShadowMap(FrameGraph, ppShadowMapDepth);
        return;
    }

    int cascadeResolution = Light->ShadowCascadeResolution;
    int totalCascades     = Light->NumCascades;

    TEXTURE_FORMAT depthFormat;
    if (r_ShadowCascadeBits.GetInteger() <= 16)
    {
        depthFormat = TEXTURE_FORMAT_D16;
    }
    else
    {
        depthFormat = TEXTURE_FORMAT_D32;
    }

    RenderPass& pass = FrameGraph.AddTask<RenderPass>("ShadowMap Pass");

    pass.SetRenderArea(cascadeResolution, cascadeResolution);

    pass.SetDepthStencilAttachment(
        TextureAttachment("Shadow Cascade Depth texture",
                           TextureDesc()
                               .SetFormat(depthFormat)
                               .SetResolution(TextureResolution2DArray(cascadeResolution, cascadeResolution, totalCascades))
                               .SetBindFlags(BIND_SHADER_RESOURCE))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
            .SetClearValue(shadowMap->LightPortalsCount > 0 ? ClearDepthStencilValue(0, 0) : ClearDepthStencilValue(1, 0)));

#if defined SHADOWMAP_EVSM || defined SHADOWMAP_VSM
#    ifdef SHADOWMAP_EVSM
    TexFormat = TEXTURE_FORMAT_RGBA32_FLOAT,
#    else
    TexFormat = TEXTURE_FORMAT_RG32_FLOAT,
#    endif
    pass.SetColorAttachments(
        {
            {"Shadow Cascade Color texture",
             MakeTextureStorage(TexFormat, TextureResolution2DArray(cascadeResolution, cascadeResolution, totalCascades)),
             RenderCore::AttachmentInfo().SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)},
            {"Shadow Cascade Color texture 2",
             MakeTextureStorage(TexFormat, TextureResolution2DArray(cascadeResolution, cascadeResolution, totalCascades)),
             RenderCore::AttachmentInfo().SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE)},
        });

    // TODO: очищать только нужные слои!
    pass.SetClearColors(
        {
#    if defined SHADOWMAP_EVSM
            MakeClearColorValue(EVSM_ClearValue)
#    elif defined SHADOWMAP_VSM
            MakeClearColorValue(VSM_ClearValue)
#    endif
        });
#endif

    pass.AddSubpass({}, // no color attachments
                    [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        BindShadowCascades(Light->ViewProjStreamHandle);

                        DrawIndexedCmd drawCmd;
                        drawCmd.StartInstanceLocation = 0;

                        for (int i = 0; i < shadowMap->LightPortalsCount; i++)
                        {
                            LightPortalRenderInstance const* instance = GFrameData->LightPortals[shadowMap->FirstLightPortal + i];

                            immediateCtx->BindPipeline(LightPortalPipeline);

                            BindVertexAndIndexBuffers(immediateCtx, instance);

                            drawCmd.InstanceCount         = Light->NumCascades;
                            drawCmd.IndexCountPerInstance = instance->IndexCount;
                            drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                            drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                            immediateCtx->Draw(&drawCmd);
                        }

                        drawCmd.InstanceCount = 1;

                        for (int i = 0; i < shadowMap->ShadowInstanceCount; i++)
                        {
                            ShadowRenderInstance const* instance = GFrameData->ShadowInstances[shadowMap->FirstShadowInstance + i];

                            if (!BindMaterialShadowMap(immediateCtx, instance))
                            {
                                continue;
                            }

                            BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                            BindShadowInstanceConstants(instance);

                            drawCmd.IndexCountPerInstance = instance->IndexCount;
                            drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                            drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                            immediateCtx->Draw(&drawCmd);
                        }
                    });

    *ppShadowMapDepth = pass.GetDepthStencilAttachment().pResource;
}

void ShadowMapRenderer::AddPass(FrameGraph& FrameGraph, LightShadowmap const* ShadowMaps, int NumOmnidirectionalShadowMaps, OmnidirectionalShadowMapPool& Pool, FGTextureProxy** ppOmnidirectionalShadowMapArray)
{
    FGTextureProxy* OmnidirectionalShadowMapArray_R = FrameGraph.AddExternalResource<FGTextureProxy>("OmnidirectionalShadowMapArray", Pool.GetTexture());

    if (!NumOmnidirectionalShadowMaps)
    {
        *ppOmnidirectionalShadowMapArray = OmnidirectionalShadowMapArray_R;
        return;
    }

    int faceResolution = Pool.GetResolution();

    if (NumOmnidirectionalShadowMaps > Pool.GetSize())
    {
        NumOmnidirectionalShadowMaps = Pool.GetSize();
        WARNING("Max omnidirectional shadow maps hit\n");
    }

    for (int omniShadowMap = 0; omniShadowMap < NumOmnidirectionalShadowMaps; omniShadowMap++)
    {
        int shadowMapIndex = omniShadowMap * 6;

        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            int                    sliceIndex = shadowMapIndex + faceIndex;
            LightShadowmap const* shadowMap  = &ShadowMaps[sliceIndex];

            RenderPass& pass = FrameGraph.AddTask<RenderPass>("Omnidirectional Shadow Map Pass");

            pass.SetRenderArea(faceResolution, faceResolution);

            // Attach view
            pass.SetDepthStencilAttachment(
                {
                    TextureAttachment(OmnidirectionalShadowMapArray_R)
                        .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
                        .SetSlice(sliceIndex)
                        .SetClearValue(ClearDepthStencilValue(0, 0))
                });

            pass.AddSubpass({}, // no color attachments
                            [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                            {
                                IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                                DrawIndexedCmd drawCmd;
                                drawCmd.StartInstanceLocation = 0;
                                drawCmd.InstanceCount         = 1;

                                BindOmniShadowProjection(faceIndex);

                                for (int i = 0; i < shadowMap->ShadowInstanceCount; i++)
                                {
                                    ShadowRenderInstance const* instance = GFrameData->ShadowInstances[shadowMap->FirstShadowInstance + i];

                                    if (!BindMaterialOmniShadowMap(immediateCtx, instance))
                                    {
                                        continue;
                                    }

                                    BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                                    BindShadowInstanceConstants(instance, faceIndex, shadowMap->LightPosition);

                                    drawCmd.IndexCountPerInstance = instance->IndexCount;
                                    drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                                    drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                                    immediateCtx->Draw(&drawCmd);
                                }
                            });
        }
    }

    *ppOmnidirectionalShadowMapArray = OmnidirectionalShadowMapArray_R;
}

HK_NAMESPACE_END
