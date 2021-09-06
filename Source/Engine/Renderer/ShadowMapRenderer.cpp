#include "ShadowMapRenderer.h"
#include "RenderLocal.h"

using namespace RenderCore;

ARuntimeVariable r_ShadowCascadeBits( _CTS( "r_ShadowCascadeBits" ), _CTS( "24" ) );    // Allowed 16, 24 or 32 bits

static const float EVSM_positiveExponent = 40.0;
static const float EVSM_negativeExponent = 5.0;
static const Float2 EVSM_WarpDepth( std::exp( EVSM_positiveExponent ), -std::exp( -EVSM_negativeExponent ) );
const Float4 EVSM_ClearValue( EVSM_WarpDepth.X, EVSM_WarpDepth.Y, EVSM_WarpDepth.X*EVSM_WarpDepth.X, EVSM_WarpDepth.Y*EVSM_WarpDepth.Y );
const Float4 VSM_ClearValue( 1.0f );

AShadowMapRenderer::AShadowMapRenderer()
{
    CreatePipeline();
    CreateLightPortalPipeline();

    GDevice->CreateTexture(STextureDesc()
                               .SetFormat(TEXTURE_FORMAT_DEPTH16)
                               .SetResolution(STextureResolution2DArray(1, 1, 1))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &DummyShadowMap);

    SClearValue clearValue;
    clearValue.Float1.R = 1.0f;
    rcmd->ClearTexture( DummyShadowMap, 0, FORMAT_FLOAT1, &clearValue );
}

void AShadowMapRenderer::CreatePipeline()
{
    SPipelineDesc pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
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
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_NO_BLEND );
#endif

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_LESS;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float3 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings = vertexBinding;

    constexpr SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            0
        }
    };

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    CreateVertexShader( "instance_shadowmap_default.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS );
    CreateGeometryShader( "instance_shadowmap_default.geom", pipelineCI.pGS );

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if ( /*_ShadowMasking || */bVSM ) {
        CreateFragmentShader( "instance_shadowmap_default.frag", pipelineCI.pFS );
    }

    SBufferInfo bufferInfo[4];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants
    bufferInfo[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton
    bufferInfo[3].BufferBinding = BUFFER_BIND_CONSTANT; // cascade matrix

    pipelineCI.ResourceLayout.NumBuffers = AN_ARRAY_SIZE( bufferInfo );
    pipelineCI.ResourceLayout.Buffers = bufferInfo;

    GDevice->CreatePipeline( pipelineCI, &StaticShadowCasterPipeline );
}

void AShadowMapRenderer::CreateLightPortalPipeline()
{
    SPipelineDesc pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.bScissorEnable = false;
    rsd.CullMode = POLYGON_CULL_FRONT;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GREATER;//CMPFUNC_LESS;
    dssd.bDepthEnable = true;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float3 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings = vertexBinding;

    constexpr SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            0
        }
    };

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    CreateVertexShader( "instance_lightportal.vert", pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs, pipelineCI.pVS );
    CreateGeometryShader( "instance_lightportal.geom", pipelineCI.pGS );

#if 0
    CreateFragmentShader( "instance_lightportal.frag", pipelineCI.pFS );
#endif

    SBufferInfo bufferInfo[4];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT; // view constants (unused)
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT; // drawcall constants (unused)
    bufferInfo[2].BufferBinding = BUFFER_BIND_CONSTANT; // skeleton (unused)
    bufferInfo[3].BufferBinding = BUFFER_BIND_CONSTANT; // cascade matrix

    pipelineCI.ResourceLayout.NumBuffers = AN_ARRAY_SIZE( bufferInfo );
    pipelineCI.ResourceLayout.Buffers = bufferInfo;

    GDevice->CreatePipeline( pipelineCI, &LightPortalPipeline );
}

bool AShadowMapRenderer::BindMaterialShadowMap( SShadowRenderInstance const * instance )
{
    AMaterialGPU * pMaterial = instance->Material;

    if ( pMaterial ) {
        int bSkinned = instance->SkeletonSize > 0;

        IPipeline * pPipeline = pMaterial->ShadowPass[bSkinned];
        if ( !pPipeline ) {
            return false;
        }

        rcmd->BindPipeline( pPipeline );

        if ( bSkinned ) {
            rcmd->BindVertexBuffer( 1, instance->WeightsBuffer, instance->WeightsBufferOffset );
        }
        else {
            rcmd->BindVertexBuffer( 1, nullptr, 0 );
        }

        BindTextures( instance->MaterialInstance, pMaterial->ShadowMapPassTextureCount );
    }
    else {
        rcmd->BindPipeline( StaticShadowCasterPipeline.GetObject() );
        rcmd->BindVertexBuffer( 1, nullptr, 0 );
    }

    BindVertexAndIndexBuffers( instance );

    return true;
}

#if defined SHADOWMAP_VSM
static void BlurDepthMoments()
{
    GHI_Framebuffer_t * FB = RenderFrame.Statement->GetFramebuffer< SHADOWMAP_FRAMEBUFFER >();

    GHI_SetDepthStencilTarget( RenderFrame.State, FB, NULL );
    GHI_SetDepthStencilState( RenderFrame.State, FDepthStencilState< false, GHI_DepthWrite_Disable >().HardwareState() );
    GHI_SetRasterizerState( RenderFrame.State, FRasterizerState< GHI_FillSolid, GHI_CullFront >().HardwareState() );
    GHI_SetSampler( RenderFrame.State, 0, ShadowDepthSampler1 );

    GHI_SetInputAssembler( RenderFrame.State, RenderFrame.SaqIA );
    GHI_SetPrimitiveTopology( RenderFrame.State, GHI_Prim_TriangleStrip );

    GHI_Viewport_t Viewport;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.MinDepth = 0;
    Viewport.MaxDepth = 1;

    // Render horizontal blur

    GHI_Texture_t * DepthMomentsTextureTmp = ShadowPool_DepthMomentsVSMBlur();

    GHI_SetFramebufferSize( RenderFrame.State, FB, DepthMomentsTextureTmp->GetDesc().Width, DepthMomentsTextureTmp->GetDesc().Height );
    Viewport.Width = DepthMomentsTextureTmp->GetDesc().Width;
    Viewport.Height = DepthMomentsTextureTmp->GetDesc().Height;
    GHI_SetViewport( RenderFrame.State, &Viewport );

    RenderTargetVSM.Texture = DepthMomentsTextureTmp;
    GHI_SetRenderTargets( RenderFrame.State, FB, 1, &RenderTargetVSM );
    GHI_SetProgramPipeline( RenderFrame.State, RenderFrame.Statement->GetProgramAndAttach< FGaussianBlur9HProgram, FSaqVertex >() );
    GHI_AssignTextureUnit( RenderFrame.State, 0, ShadowPool_DepthMomentsVSM() );
    GHI_DrawInstanced( RenderFrame.State, 4, NumShadowMapCascades, 0 );

    // Render vertical blur

    GHI_SetFramebufferSize( RenderFrame.State, FB, r_ShadowCascadeResolution.GetInteger(), r_ShadowCascadeResolution.GetInteger() );
    Viewport.Width = r_ShadowCascadeResolution.GetInteger();
    Viewport.Height = r_ShadowCascadeResolution.GetInteger();
    GHI_SetViewport( RenderFrame.State, &Viewport );

    RenderTargetVSM.Texture = ShadowPool_DepthMomentsVSM();
    GHI_SetRenderTargets( RenderFrame.State, FB, 1, &RenderTargetVSM );
    GHI_SetProgramPipeline( RenderFrame.State, RenderFrame.Statement->GetProgramAndAttach< FGaussianBlur9VProgram, FSaqVertex >() );
    GHI_AssignTextureUnit( RenderFrame.State, 0, DepthMomentsTextureTmp );
    GHI_DrawInstanced( RenderFrame.State, 4, NumShadowMapCascades, 0 );
}
#endif

void AShadowMapRenderer::AddDummyShadowMap( AFrameGraph & FrameGraph, FGTextureProxy ** ppShadowMapDepth )
{
    *ppShadowMapDepth = FrameGraph.AddExternalResource<FGTextureProxy>("Dummy Shadow Map", DummyShadowMap);
}

void AShadowMapRenderer::AddPass( AFrameGraph & FrameGraph, SDirectionalLightInstance const * Light, FGTextureProxy ** ppShadowMapDepth )
{
    if ( Light->ShadowmapIndex < 0 ) {
        AddDummyShadowMap( FrameGraph, ppShadowMapDepth );
        return;
    }

    SLightShadowmap const * shadowMap = &GFrameData->LightShadowmaps[ Light->ShadowmapIndex ];
    if ( shadowMap->ShadowInstanceCount == 0 ) {
        AddDummyShadowMap( FrameGraph, ppShadowMapDepth );
        return;
    }

    int cascadeResolution = Light->ShadowCascadeResolution;
    int totalCascades = Light->NumCascades;

    RenderCore::TEXTURE_FORMAT depthFormat;
    if ( r_ShadowCascadeBits.GetInteger() <= 16 ) {
        depthFormat = TEXTURE_FORMAT_DEPTH16;
    }
    else if ( r_ShadowCascadeBits.GetInteger() <= 24 ) {
        depthFormat = TEXTURE_FORMAT_DEPTH24;
    }
    else {
        depthFormat = TEXTURE_FORMAT_DEPTH32;
    }

    ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "ShadowMap Pass" );

    pass.SetRenderArea( cascadeResolution, cascadeResolution );

    pass.SetDepthStencilAttachment(
        STextureAttachment("Shadow Cascade Depth texture",
                           STextureDesc()
                               .SetFormat(depthFormat)
                               .SetResolution(STextureResolution2DArray(cascadeResolution, cascadeResolution, totalCascades))
                               .SetBindFlags(BIND_SHADER_RESOURCE))
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
            .SetClearValue(shadowMap->LightPortalsCount > 0 ? SClearDepthStencilValue(0, 0) : SClearDepthStencilValue(1, 0)));

#if defined SHADOWMAP_EVSM || defined SHADOWMAP_VSM
#ifdef SHADOWMAP_EVSM
    TexFormat = TEXTURE_FORMAT_RGBA32F,
#else
    TexFormat = TEXTURE_FORMAT_RG32F,
#endif
    pass.SetColorAttachments(
    {
        {
            "Shadow Cascade Color texture",
            MakeTextureStorage( TexFormat, TextureResolution2DArray( cascadeResolution, cascadeResolution, totalCascades ) ),
            RenderCore::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
        },
        {
            "Shadow Cascade Color texture 2",
            MakeTextureStorage( TexFormat, TextureResolution2DArray( cascadeResolution, cascadeResolution, totalCascades ) ),
            RenderCore::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        },
    } );

    // TODO: очищать только нужные слои!
    pass.SetClearColors(
    {
#if defined SHADOWMAP_EVSM
        MakeClearColorValue( EVSM_ClearValue )
#elif defined SHADOWMAP_VSM
        MakeClearColorValue( VSM_ClearValue )
#endif
    } );
#endif

    pass.AddSubpass( {}, // no color attachments
                    [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
    {
        BindShadowCascades( Light->ViewProjStreamHandle );

        SDrawIndexedCmd drawCmd;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < shadowMap->LightPortalsCount ; i++ ) {
            SLightPortalRenderInstance const * instance = GFrameData->LightPortals[shadowMap->FirstLightPortal + i];

            rcmd->BindPipeline( LightPortalPipeline.GetObject() );

            BindVertexAndIndexBuffers( instance );

            drawCmd.InstanceCount = Light->NumCascades;
            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }

        drawCmd.InstanceCount = 1;

        for ( int i = 0 ; i < shadowMap->ShadowInstanceCount ; i++ ) {
            SShadowRenderInstance const * instance = GFrameData->ShadowInstances[shadowMap->FirstShadowInstance + i];

            if ( !BindMaterialShadowMap( instance ) ) {
                continue;
            }

            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
            BindShadowInstanceConstants( instance );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }
    } );

    *ppShadowMapDepth = pass.GetDepthStencilAttachment().pResource;
}


















#if 0
ARuntimeVariable r_ShadowmapResolution( _CTS( "r_ShadowmapResolution" ), _CTS( "512" ) );
ARuntimeVariable r_ShadowmapBits( _CTS( "r_ShadowmapBits" ), _CTS( "24" ) );
#endif

void AShadowMapRenderer::AddPass( AFrameGraph & FrameGraph, SLightParameters const * Light, FGTextureProxy ** ppShadowMapDepth )
{
#if 0
    if ( Light->ShadowmapIndex < 0 ) {
        AddDummyCubeShadowMap( FrameGraph, ppShadowMapDepth );
        return;
    }

    int totalInstanceCount = 0;
    for ( int faceIndex = 0 ; faceIndex < 6 ; faceIndex++ ) {
        SLightShadowmap const * shadowMap = &GFrameData->LightShadowmaps[ Light->ShadowmapIndex ];
        totalInstanceCount += shadowMap->ShadowInstanceCount;
    }

    if ( totalInstanceCount == 0 ) {
        AddDummyShadowMapCube( FrameGraph, ppShadowMapDepth );
        return;
    }

    RenderCore::TEXTURE_FORMAT depthFormat;
    if ( r_ShadowmapBits.GetInteger() <= 16 ) {
        depthFormat = TEXTURE_FORMAT_DEPTH16;
    }
    else if ( r_ShadowmapBits.GetInteger() <= 24 ) {
        depthFormat = TEXTURE_FORMAT_DEPTH24;
    }
    else {
        depthFormat = TEXTURE_FORMAT_DEPTH32;
    }

    int faceResolution = Math::ToClosestPowerOfTwo( r_ShadowmapResolution.GetInteger() );

    for ( int faceIndex = 0 ; faceIndex < 6 ; faceIndex++ ) {
        SLightShadowmap const * shadowMap = &GFrameData->LightShadowmaps[ Light->ShadowmapIndex + faceIndex ];

        ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "Point Shadow Map Pass" );

        pass.SetRenderArea( faceResolution, faceResolution );

        pass.SetDepthStencilAttachment(
        {
            "Shadow Face Depth texture",
            MakeTexture( depthFormat, STextureResolution2DArray( faceResolution, faceResolution, totalCascades ) ),
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
        } );

        pass.SetDepthStencilClearValue( MakeClearDepthStencilValue( 1, 0 ) );

        pass.AddSubpass( {}, // no color attachments
                         [=]( ARenderPass const & RenderPass, ACommandBuffer const& CommandBuffer )
        {
            SetShadowFaceBinding( faceIndex );

            SDrawIndexedCmd drawCmd;
            drawCmd.StartInstanceLocation = 0;
            drawCmd.InstanceCount = 1;

            for ( int i = 0 ; i < shadowMap->ShadowInstanceCount ; i++ ) {
                SShadowRenderInstance const * instance = GFrameData->ShadowInstances[shadowMap->FirstShadowInstance + i];

                if ( !BindMaterialShadowMap( instance ) ) {
                    continue;
                }

                // Bind textures
                BindTextures( instance->MaterialInstance, instance->Material->ShadowMapPassTextureCount );

                // Bind skeleton
                BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

                // Set instance constants
                SetShadowInstanceConstants( instance );

                drawCmd.IndexCountPerInstance = instance->IndexCount;
                drawCmd.StartIndexLocation = instance->StartIndexLocation;
                drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

                rcmd->Draw( &drawCmd );
            }
        } );

        *ppShadowMapDepth = pass.GetDepthStencilAttachment().Resource;
    }
#endif
}
