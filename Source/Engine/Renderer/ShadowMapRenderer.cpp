#include "ShadowMapRenderer.h"
#include "Material.h"

using namespace RenderCore;

extern ARuntimeVariable RVShadowCascadeResolution;
extern ARuntimeVariable RVShadowCascadeBits;

static const float EVSM_positiveExponent = 40.0;
static const float EVSM_negativeExponent = 5.0;
static const Float2 EVSM_WarpDepth( std::exp( EVSM_positiveExponent ), -std::exp( -EVSM_negativeExponent ) );
const Float4 EVSM_ClearValue( EVSM_WarpDepth.X, EVSM_WarpDepth.Y, EVSM_WarpDepth.X*EVSM_WarpDepth.X, EVSM_WarpDepth.Y*EVSM_WarpDepth.Y );
const Float4 VSM_ClearValue( 1.0f );

AShadowMapRenderer::AShadowMapRenderer()
{
    CreatePipeline();
    CreateLightPortalPipeline();
}

void AShadowMapRenderer::CreatePipeline() {
    AString codeVS = LoadShader( "instance_shadowmap_default.vert" );
    AString codeGS = LoadShader( "instance_shadowmap_default.geom" );
    AString codeFS = LoadShader( "instance_shadowmap_default.frag" );

    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.bScissorEnable = SCISSOR_TEST;
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

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule;
    TRef< IShaderModule > geometryShaderModule;
    TRef< IShaderModule > fragmentShaderModule;

    GShaderSources.Clear();
    //GShaderSources.Add( "#define SKINNED_MESH\n" );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( codeVS.CStr() );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    pipelineCI.pVS = vertexShaderModule;

    GShaderSources.Clear();
    //GShaderSources.Add( "#define SKINNED_MESH\n" );
    GShaderSources.Add( codeGS.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShaderModule );

    pipelineCI.pGS = geometryShaderModule;

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if ( /*_ShadowMasking || */bVSM ) {
        GShaderSources.Clear();
        //GShaderSources.Add( "#define SKINNED_MESH\n" );
        GShaderSources.Add( codeFS.CStr() );
        GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

        pipelineCI.pFS = fragmentShaderModule;
    }

    GDevice->CreatePipeline( pipelineCI, &StaticShadowCasterPipeline );
}

void AShadowMapRenderer::CreateLightPortalPipeline() {
    AString codeVS = LoadShader( "instance_lightportal.vert" );
    AString codeGS = LoadShader( "instance_lightportal.geom" );
    //AString codeFS = LoadShader( "instance_lightportal.frag" );

    SPipelineCreateInfo pipelineCI;

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

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    TRef< IShaderModule > vertexShaderModule;
    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( codeVS.CStr() );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );
    pipelineCI.pVS = vertexShaderModule;

    TRef< IShaderModule > geometryShaderModule;
    GShaderSources.Clear();
    GShaderSources.Add( codeGS.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShaderModule );
    pipelineCI.pGS = geometryShaderModule;

#if 0
    TRef< IShaderModule > fragmentShaderModule;
    GShaderSources.Clear();
    GShaderSources.Add( codeFS.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );
    pipelineCI.pFS = &fragmentShaderModule;
#endif

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

        // Bind pipeline
        rcmd->BindPipeline( pPipeline );

        // Bind second vertex buffer
        if ( bSkinned ) {
            IBuffer * pSecondVertexBuffer = GPUBufferHandle( instance->WeightsBuffer );
            rcmd->BindVertexBuffer( 1, pSecondVertexBuffer, instance->WeightsBufferOffset );
        } else {
            rcmd->BindVertexBuffer( 1, nullptr, 0 );
        }

        // Set samplers
        if ( pMaterial->bShadowMapPassTextureFetch ) {
            for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
                GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
            }
        }
    } else {
        rcmd->BindPipeline( StaticShadowCasterPipeline.GetObject() );
        rcmd->BindVertexBuffer( 1, nullptr, 0 );
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( instance );

    return true;
}

void BindTexturesShadowMap( SMaterialFrameData * _Instance )
{
    if ( !_Instance || !_Instance->Material->bShadowMapPassTextureFetch ) {
        return;
    }

    BindTextures( _Instance );
}

#if defined SHADOWMAP_VSM
static void BlurDepthMoments() {
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

void AShadowMapRenderer::AddPass( AFrameGraph & FrameGraph, AFrameGraphTexture ** ppShadowMapDepth )
{
    int cascadeWidth = RVShadowCascadeResolution.GetInteger();
    int cascadeHeight = RVShadowCascadeResolution.GetInteger();
    int totalCascades = GFrameData->ShadowCascadePoolSize;

    RenderCore::TEXTURE_FORMAT depthFormat;
    if ( RVShadowCascadeBits.GetInteger() <= 16 ) {
        depthFormat = TEXTURE_FORMAT_DEPTH16;
    } else if ( RVShadowCascadeBits.GetInteger() <= 24 ) {
        depthFormat = TEXTURE_FORMAT_DEPTH24;
    } else {
        depthFormat = TEXTURE_FORMAT_DEPTH32;
    }

    if ( GFrameData->ShadowCascadePoolSize == 0
         || GRenderView->ShadowInstanceCount == 0 )
    {
        cascadeWidth = 1;
        cascadeHeight = 1;
        totalCascades = 1;
        depthFormat = TEXTURE_FORMAT_DEPTH16;
    }

    ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "ShadowMap Pass" );

    pass.SetRenderArea( cascadeWidth, cascadeHeight );

    pass.SetDepthStencilAttachment(
    {
        "Shadow Cascade Depth texture",
        MakeTexture( depthFormat, STextureResolution2DArray( cascadeWidth, cascadeHeight, totalCascades ) ),
        RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
    } );

    if ( GRenderView->LightPortalsCount > 0 ) {
        pass.SetDepthStencilClearValue( MakeClearDepthStencilValue( 0, 0 ) );
    } else {
        pass.SetDepthStencilClearValue( MakeClearDepthStencilValue( 1, 0 ) );
    }

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
            MakeTextureStorage( TexFormat, TextureResolution2DArray( cascadeWidth, cascadeHeight, totalCascades ) ),
            RenderCore::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
        },
        {
            "Shadow Cascade Color texture 2",
            MakeTextureStorage( TexFormat, TextureResolution2DArray( cascadeWidth, cascadeHeight, totalCascades ) ),
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
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        if ( !GRenderView->NumShadowMapCascades ) {
            return;
        }

        if ( !GRenderView->ShadowInstanceCount ) {
            return;
        }

        SDrawIndexedCmd drawCmd;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < GRenderView->LightPortalsCount ; i++ ) {
            SLightPortalRenderInstance const * instance = GFrameData->LightPortals[GRenderView->FirstLightPortal + i];

            rcmd->BindPipeline( LightPortalPipeline.GetObject() );

            BindVertexAndIndexBuffers( instance );

            rcmd->BindShaderResources( &GFrameResources.Resources );

            drawCmd.InstanceCount = GRenderView->NumShadowMapCascades;
            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }

        for ( int i = 0 ; i < GRenderView->ShadowInstanceCount ; i++ ) {
            SShadowRenderInstance const * instance = GFrameData->ShadowInstances[GRenderView->FirstShadowInstance + i];

            if ( !BindMaterialShadowMap( instance ) ) {
                continue;
            }

            // Set material data (textures, uniforms)
            BindTexturesShadowMap( instance->MaterialInstance );

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

            // Set instance uniforms
            SetShadowInstanceUniforms( instance );

            rcmd->BindShaderResources( &GFrameResources.Resources );

            drawCmd.InstanceCount = 1;//GRenderView->NumShadowMapCascades;
            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }
    } );

    *ppShadowMapDepth = pass.GetDepthStencilAttachment().Resource;
}
