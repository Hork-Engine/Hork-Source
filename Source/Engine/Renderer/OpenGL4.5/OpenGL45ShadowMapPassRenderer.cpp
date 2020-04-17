#include "OpenGL45ShadowMapPassRenderer.h"
#include "OpenGL45Material.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45ShadowMapRT.h"
#include "OpenGL45ShaderSource.h"

#include <Core/Public/CriticalError.h>

using namespace GHI;

namespace OpenGL45 {

AShadowMapPassRenderer GShadowMapPassRenderer;

static const float EVSM_positiveExponent = 40.0;
static const float EVSM_negativeExponent = 5.0;
static const Float2 EVSM_WarpDepth( std::exp( EVSM_positiveExponent ), -std::exp( -EVSM_negativeExponent ) );
static const Float4 EVSM_ClearValue( EVSM_WarpDepth.X, EVSM_WarpDepth.Y, EVSM_WarpDepth.X*EVSM_WarpDepth.X, EVSM_WarpDepth.Y*EVSM_WarpDepth.Y );
static const Float4 VSM_ClearValue( 1.0f );

void AShadowMapPassRenderer::Initialize() {
    CreateShadowDepthSamplers();
    CreateRenderPass();
    CreatePipeline();
}

void AShadowMapPassRenderer::Deinitialize() {
    DepthPass.Deinitialize();
    StaticShadowCasterPipeline.Deinitialize();
}

void AShadowMapPassRenderer::CreateRenderPass() {
    RenderPassCreateInfo renderPassCI = {};

    SubpassInfo subpass = {};

    #if defined SHADOWMAP_EVSM || defined SHADOWMAP_VSM

    AttachmentInfo colorAttachment[2] = {};
    //colorAttachment[0].LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment[0].LoadOp = ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment[1].LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassCI.pColorAttachments = &colorAttachment;
    renderPassCI.NumColorAttachments = 2;

    AttachmentRef colorAttachmentRef[1] = {};
    colorAttachmentRef[0].Attachment = 0;

    subpass.NumColorAttachments = 1; // FIXME!!!
    subpass.pColorAttachmentRefs = &colorAttachmentRef;
    #else
    renderPassCI.NumColorAttachments = 0;
    subpass.NumColorAttachments = 0;
    #endif

    AttachmentInfo depthAttachment = {};
    depthAttachment.LoadOp = ATTACHMENT_LOAD_OP_CLEAR;
    renderPassCI.pDepthStencilAttachment = &depthAttachment;

    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpass;

    DepthPass.Initialize( renderPassCI );
}

#if defined SHADOWMAP_PCSS
void AShadowMapPassRenderer::CreateShadowDepthSamplers() {
    SamplerCreateInfo samplerCI;

    samplerCI.SetDefaults();

    samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
    samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
    samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
    samplerCI.MipLODBias = 0;
    samplerCI.MaxAnisotropy = 0;
    //samplerCI.BorderColor[0] = samplerCI.BorderColor[1] = samplerCI.BorderColor[2] = samplerCI.BorderColor[3] = 1.0f;

    // Find blocker point sampler
    samplerCI.Filter = FILTER_NEAREST;//FILTER_LINEAR;
    //samplerCI.ComparisonFunc = CMPFUNC_GREATER;//CMPFUNC_GEQUAL;
    //samplerCI.CompareRefToTexture = true;
    ShadowDepthSampler0 = GDevice.GetOrCreateSampler( samplerCI );

    // PCF_Sampler
    samplerCI.Filter = FILTER_LINEAR; //GHI_Filter_Min_LinearMipmapLinear_Mag_Linear; // D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR  Is the same?
    samplerCI.ComparisonFunc = CMPFUNC_LESS;
    samplerCI.CompareRefToTexture = true;
    samplerCI.BorderColor[0] = samplerCI.BorderColor[1] = samplerCI.BorderColor[2] = samplerCI.BorderColor[3] = 1.0f; // FIXME?
    ShadowDepthSampler1 = GDevice.GetOrCreateSampler( samplerCI );
}
#elif defined SHADOWMAP_PCF
void AShadowMapPassRenderer::CreateShadowDepthSamplers() {
    SamplerCreateInfo samplerCI;

    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
    samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
    samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
    samplerCI.MipLODBias = 0;
    samplerCI.MaxAnisotropy = 0;
    //samplerCI.ComparisonFunc = CMPFUNC_LEQUAL;
    samplerCI.ComparisonFunc = CMPFUNC_LESS;
    samplerCI.bCompareRefToTexture = true;

    ShadowDepthSampler0 = NULL;
    ShadowDepthSampler1 = GDevice.GetOrCreateSampler( samplerCI );
}
#elif defined SHADOWMAP_VSM
void AShadowMapPassRenderer::CreateShadowDepthSamplers() {
    SamplerCreateInfo samplerCI;

    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
    samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
    samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
    samplerCI.MipLODBias = 0;
    samplerCI.MaxAnisotropy = 0;
#ifdef SHADOWMAP_EVSM
    samplerCI.BorderColor[0] = EVSM_ClearValue.X;
    samplerCI.BorderColor[1] = EVSM_ClearValue.Y;
    samplerCI.BorderColor[2] = EVSM_ClearValue.Z;
    samplerCI.BorderColor[3] = EVSM_ClearValue.W;
#else
    samplerCI.BorderColor[0] = VSM_ClearValue.X;
    samplerCI.BorderColor[1] = VSM_ClearValue.Y;
    samplerCI.BorderColor[2] = VSM_ClearValue.Z;
    samplerCI.BorderColor[3] = VSM_ClearValue.W;
#endif
    ShadowDepthSampler0 = NULL;
    ShadowDepthSampler1 = GDevice.GetOrCreateSampler( samplerCI );
}
#endif

void AShadowMapPassRenderer::CreatePipeline() {
    AString codeVS = LoadShader( "shadowcast.vert" );
    AString codeGS = LoadShader( "instance_shadowmap.geom" );
    AString codeFS = LoadShader( "shadowcast.frag" );

    PipelineCreateInfo pipelineCI = {};

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.bScissorEnable = SCISSOR_TEST;
#if defined SHADOWMAP_VSM
    //Desc.CullMode = POLYGON_CULL_FRONT; // Less light bleeding
    Desc.CullMode = POLYGON_CULL_DISABLED;
#else
    //rsd.CullMode = POLYGON_CULL_BACK;
    //rsd.CullMode = POLYGON_CULL_DISABLED; // Less light bleeding
    rsd.CullMode = POLYGON_CULL_FRONT;
#endif
    //rsd.CullMode = POLYGON_CULL_DISABLED;

    BlendingStateInfo bsd;
    bsd.SetDefaults();
    //bsd.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;  // FIXME: there is no fragment shader, so we realy need to disable color mask?
#if defined SHADOWMAP_VSM
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_NO_BLEND );
#endif

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();
    dssd.DepthFunc = CMPFUNC_LESS;

    VertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float3 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings = vertexBinding;

    constexpr VertexAttribInfo vertexAttribs[] = {
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

    PipelineInputAssemblyInfo inputAssembly = {};
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo stages[3] = {};

    pipelineCI.NumStages = 0;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( pipelineCI.pVertexAttribs, pipelineCI.NumVertexAttribs );

    ShaderModule vertexShaderModule;
    ShaderModule geometryShaderModule;
    ShaderModule fragmentShaderModule;

    GShaderSources.Clear();
    //GShaderSources.Add( "#define SKINNED_MESH\n" );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( codeVS.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    ShaderStageInfo & vs = stages[pipelineCI.NumStages++];
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    GShaderSources.Clear();
    //GShaderSources.Add( "#define SKINNED_MESH\n" );
    GShaderSources.Add( codeGS.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, &geometryShaderModule );

    ShaderStageInfo & gs = stages[pipelineCI.NumStages++];
    gs.Stage = SHADER_STAGE_GEOMETRY_BIT;
    gs.pModule = &geometryShaderModule;

    bool bVSM = false;

#if defined SHADOWMAP_VSM || defined SHADOWMAP_EVSM
    bVSM = true;
#endif

    if ( /*_ShadowMasking || */bVSM ) {
        GShaderSources.Clear();
        //GShaderSources.Add( "#define SHADOW_MASKING\n" );
        //GShaderSources.Add( "#define SKINNED_MESH\n" );
        GShaderSources.Add( codeFS.CStr() );
        GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

        ShaderStageInfo & fs = stages[pipelineCI.NumStages++];
        fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
        fs.pModule = &fragmentShaderModule;
    }

    pipelineCI.pStages = stages;
    pipelineCI.pRenderPass = GetRenderPass();
    pipelineCI.Subpass = 0;

    StaticShadowCasterPipeline.Initialize( pipelineCI );
}

bool AShadowMapPassRenderer::BindMaterial( SShadowRenderInstance const * instance ) {
    AMaterialGPU * pMaterial = instance->Material;

    if ( pMaterial ) {
        bool bSkinned = instance->SkeletonSize > 0;
        Pipeline * pPipeline;

        // Choose pipeline
        switch ( pMaterial->MaterialType ) {
        case MATERIAL_TYPE_PBR:
        case MATERIAL_TYPE_BASELIGHT:
            pPipeline = bSkinned ? &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->ShadowPassSkinned
                                 : &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->ShadowPass;
            break;
        case MATERIAL_TYPE_UNLIT:
            pPipeline = bSkinned ? &((AShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->ShadowPassSkinned
                                 : &((AShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->ShadowPass;
            break;
        default:
            return false;
        }

        // Bind pipeline
        Cmd.BindPipeline( pPipeline );

        // Bind second vertex buffer
        if ( bSkinned ) {
            Buffer * pSecondVertexBuffer = GPUBufferHandle( instance->WeightsBuffer );
            Cmd.BindVertexBuffer( 1, pSecondVertexBuffer, instance->WeightsBufferOffset );
        } else {
            Cmd.BindVertexBuffer( 1, nullptr, 0 );
        }

        // Set samplers
        if ( pMaterial->bShadowMapPassTextureFetch ) {
            for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
                GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
            }
        }
    } else {
        Cmd.BindPipeline( &StaticShadowCasterPipeline );
        Cmd.BindVertexBuffer( 1, nullptr, 0 );
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( instance );

    return true;
}

void AShadowMapPassRenderer::BindTexturesShadowMapPass( SMaterialFrameData * _Instance ) {
    if ( !_Instance || !_Instance->Material->bShadowMapPassTextureFetch ) {
        return;
    }

    BindTextures( _Instance );
}

void AShadowMapPassRenderer::RenderInstances() {

    if ( !GRenderView->NumShadowMapCascades ) {
        return;
    }

    if ( !GRenderView->ShadowInstanceCount ) {
        return;
    }

    GShadowMapRT.Realloc( GFrameData->ShadowCascadePoolSize );

    // TODO: Bind uniform buffer:
    //Cmd.SetBuffers( UNIFORM_BUFFER, SHADOW_MATRIX_BUFFER_UNIFORM_BINDING, 1, &CascadeViewProjectionBuffer );

    // TODO: очищать только нужные слои!
    ClearColorValue colorValues[2];
    #if defined SHADOWMAP_EVSM
    colorValues[0].Float32[0] = EVSM_ClearValue[0];
    colorValues[0].Float32[1] = EVSM_ClearValue[1];
    colorValues[0].Float32[2] = EVSM_ClearValue[2];
    colorValues[0].Float32[3] = EVSM_ClearValue[3];
    #elif defined SHADOWMAP_VSM
    colorValues[0].Float32[0] = VSM_ClearValue[0];
    colorValues[0].Float32[1] = VSM_ClearValue[1];
    colorValues[0].Float32[2] = VSM_ClearValue[2];
    colorValues[0].Float32[3] = VSM_ClearValue[3];
    #endif
    colorValues[1] = colorValues[0]; // TODO: вторую текстуру можно вообще не очищать!!!

    // TODO: тоже самое касается глубины
    ClearDepthStencilValue depthStencilValue = {};
    depthStencilValue.Depth = 1;
    depthStencilValue.Stencil = 0;

    RenderPassBegin renderPassBegin = {};
    renderPassBegin.pRenderPass = &DepthPass;
    renderPassBegin.pFramebuffer = &GShadowMapRT.GetFramebuffer();
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width = GShadowMapRT.GetFramebuffer().GetWidth();
    renderPassBegin.RenderArea.Height = GShadowMapRT.GetFramebuffer().GetHeight();
    renderPassBegin.pColorClearValues = colorValues;
    renderPassBegin.pDepthStencilClearValue = &depthStencilValue;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = renderPassBegin.RenderArea.Width;
    vp.Height = renderPassBegin.RenderArea.Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawIndexedCmd drawCmd;
    drawCmd.StartInstanceLocation = 0;

    for ( int i = 0 ; i < GRenderView->ShadowInstanceCount ; i++ ) {
        SShadowRenderInstance const * instance = GFrameData->ShadowInstances[ GRenderView->FirstShadowInstance + i ];

        if ( !BindMaterial( instance ) ) {
            continue;
        }

        // Set material data (textures, uniforms)
        BindTexturesShadowMapPass( instance->MaterialInstance );

        // Bind skeleton
        BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

        // Set instance uniforms
        SetShadowInstanceUniforms( i );

        Cmd.BindShaderResources( &GFrameResources.Resources );

        // TODO: должна быть предварительная проверка видимости (рассчитано закранее: instanceFirstCascade, instanceNumCascades)
        // instanceFirstCascade нужно записать в uniformBuffer, принадлежащий инстансу,
        // а drawCmd.InstanceCount = instanceNumCascades;
        // Далее в геометрическом шейдере gl_Layer = gl_InstanceId + instanceFirstCascade;
        // TODO: добавить к юниформам инстанса битовое поле uint16_t CascadeMask, для того чтобы в геометрическом шедере можно было сделать фильтр:
        // if ( CascadeMask & gl_LayerId ) {
        //      ... EmitPrimitive
        // } else {
        //      nothing
        // }
        drawCmd.InstanceCount = GRenderView->NumShadowMapCascades;
        drawCmd.IndexCountPerInstance = instance->IndexCount;
        drawCmd.StartIndexLocation = instance->StartIndexLocation;
        drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

        Cmd.Draw( &drawCmd );
    }

    Cmd.EndRenderPass();

#if defined SHADOWMAP_VSM
    BlurDepthMoments();
#endif
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

}
