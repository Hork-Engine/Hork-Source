/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Runtime/Public/ScopedTimeCheck.h>

extern ARuntimeVariable r_FXAA;

ARuntimeVariable r_ShowNormals( _CTS( "r_ShowNormals" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable r_ShowFeedbackVT( _CTS( "r_ShowFeedbackVT" ), _CTS( "0" ) );
ARuntimeVariable r_ShowCacheVT( _CTS( "r_ShowCacheVT" ), _CTS( "-1" ) );

using namespace RenderCore;

AFrameRenderer::AFrameRenderer()
{
    SPipelineResourceLayout resourceLayout;

    SBufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT;

    resourceLayout.NumBuffers = 1;
    resourceLayout.Buffers = &bufferInfo;

    SSamplerInfo nearestSampler;
    nearestSampler.Filter = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    SSamplerInfo linearSampler;
    linearSampler.Filter = FILTER_LINEAR;
    linearSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    linearSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    linearSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &nearestSampler;

    CreateFullscreenQuadPipeline( &LinearDepthPipe, "postprocess/linear_depth.vert", "postprocess/linear_depth.frag", &resourceLayout );
    CreateFullscreenQuadPipeline( &LinearDepthPipe_ORTHO, "postprocess/linear_depth.vert", "postprocess/linear_depth_ortho.frag", &resourceLayout );

    CreateFullscreenQuadPipeline( &ReconstructNormalPipe, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal.frag", &resourceLayout );
    CreateFullscreenQuadPipeline( &ReconstructNormalPipe_ORTHO, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal_ortho.frag", &resourceLayout );

    SSamplerInfo motionBlurSamplers[3];
    motionBlurSamplers[0] = linearSampler;
    motionBlurSamplers[1] = nearestSampler;
    motionBlurSamplers[2] = nearestSampler;

    resourceLayout.NumSamplers = AN_ARRAY_SIZE( motionBlurSamplers );
    resourceLayout.Samplers = motionBlurSamplers;

    CreateFullscreenQuadPipeline( &MotionBlurPipeline, "postprocess/motionblur.vert", "postprocess/motionblur.frag", &resourceLayout );

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &linearSampler;

    CreateFullscreenQuadPipeline( &OutlineBlurPipe, "postprocess/outlineblur.vert", "postprocess/outlineblur.frag", &resourceLayout );

    SSamplerInfo outlineApplySamplers[2];
    outlineApplySamplers[0] = linearSampler;
    outlineApplySamplers[1] = linearSampler;

    resourceLayout.NumSamplers = AN_ARRAY_SIZE( outlineApplySamplers );
    resourceLayout.Samplers = outlineApplySamplers;

    //CreateFullscreenQuadPipeline( &OutlineApplyPipe, "postprocess/outlineapply.vert", "postprocess/outlineapply.frag", &resourceLayout, RenderCore::BLENDING_COLOR_ADD );
    CreateFullscreenQuadPipeline( &OutlineApplyPipe, "postprocess/outlineapply.vert", "postprocess/outlineapply.frag", &resourceLayout, RenderCore::BLENDING_ALPHA );
}

void AFrameRenderer::AddLinearizeDepthPass( AFrameGraph & FrameGraph, AFrameGraphTexture * DepthTexture, AFrameGraphTexture ** ppLinearDepth )
{
    ARenderPass & linearizeDepthPass = FrameGraph.AddTask< ARenderPass >( "Linearize Depth Pass" );
    linearizeDepthPass.SetDynamicRenderArea( &GRenderViewArea );
    linearizeDepthPass.AddResource( DepthTexture, RESOURCE_ACCESS_READ );
    linearizeDepthPass.SetColorAttachments(
    {
        {
            "Linear depth texture",
            MakeTexture( RenderCore::TEXTURE_FORMAT_R32F, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    linearizeDepthPass.AddSubpass( { 0 }, // color attachment refs
                                   [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        rtbl->BindTexture( 0, DepthTexture->Actual() );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( LinearDepthPipe );
        }
        else {
            DrawSAQ( LinearDepthPipe_ORTHO );
        }
    } );
    *ppLinearDepth = linearizeDepthPass.GetColorAttachments()[0].Resource;
}

void AFrameRenderer::AddReconstrutNormalsPass( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth, AFrameGraphTexture ** ppNormalTexture )
{
    ARenderPass & reconstructNormalPass = FrameGraph.AddTask< ARenderPass >( "Reconstruct Normal Pass" );
    reconstructNormalPass.SetDynamicRenderArea( &GRenderViewArea );
    reconstructNormalPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    reconstructNormalPass.SetColorAttachments(
    {
        {
            "Normal texture",
            MakeTexture( RenderCore::TEXTURE_FORMAT_RGB8, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( RenderCore::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    reconstructNormalPass.AddSubpass( { 0 }, // color attachment refs
                                      [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        rtbl->BindTexture( 0, LinearDepth->Actual() );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( ReconstructNormalPipe );
        }
        else {
            DrawSAQ( ReconstructNormalPipe_ORTHO );
        }
    } );
    *ppNormalTexture = reconstructNormalPass.GetColorAttachments()[0].Resource;
}

void AFrameRenderer::AddMotionBlurPass( AFrameGraph & FrameGraph,
                                        AFrameGraphTexture * LightTexture,
                                        AFrameGraphTexture * VelocityTexture,
                                        AFrameGraphTexture * LinearDepth,
                                        AFrameGraphTexture ** ppResultTexture )
{
    ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "Motion Blur Pass" );

    renderPass.SetDynamicRenderArea( &GRenderViewArea );

    renderPass.AddResource( LightTexture, RESOURCE_ACCESS_READ );
    renderPass.AddResource( VelocityTexture, RESOURCE_ACCESS_READ );
    renderPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );

    renderPass.SetColorAttachments(
    {
        {
            "Motion blur texture",
            LightTexture->GetCreateInfo(), // Use same format as light texture
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    }
    );

    renderPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        rtbl->BindTexture( 0, LightTexture->Actual() );
        rtbl->BindTexture( 1, VelocityTexture->Actual() );
        rtbl->BindTexture( 2, LinearDepth->Actual() );

        DrawSAQ( MotionBlurPipeline );

    } );

    *ppResultTexture = renderPass.GetColorAttachments()[0].Resource;
}

static bool BindMaterialOutlinePass( SRenderInstance const * instance )
{
    AMaterialGPU * pMaterial = instance->Material;

    AN_ASSERT( pMaterial );

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline * pPipeline = pMaterial->OutlinePass[bSkinned];
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

    BindVertexAndIndexBuffers( instance );

    return true;
}

void AFrameRenderer::AddOutlinePass( AFrameGraph & FrameGraph, AFrameGraphTexture ** ppOutlineTexture )
{
    if ( GRenderView->OutlineInstanceCount == 0 ) {
        *ppOutlineTexture = nullptr;
        return;
    }

    RenderCore::TEXTURE_FORMAT pf = TEXTURE_FORMAT_RG8;

    ARenderPass & maskPass = FrameGraph.AddTask< ARenderPass >( "Outline Pass" );

    maskPass.SetDynamicRenderArea( &GRenderViewArea );

    maskPass.SetClearColors( { RenderCore::MakeClearColorValue( 0.0f, 1.0f, 0.0f, 0.0f ) } );

    maskPass.SetColorAttachments(
    {
        {
            "Outline mask",
            RenderCore::MakeTexture( pf, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
        }
    } );

    maskPass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        SDrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < GRenderView->OutlineInstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->OutlineInstances[GRenderView->FirstOutlineInstance + i];

            if ( !BindMaterialOutlinePass( instance ) ) {
                continue;
            }

            BindTextures( instance->MaterialInstance, instance->Material->DepthPassTextureCount );
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
            BindInstanceConstants( instance );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }
    } );

    *ppOutlineTexture = maskPass.GetColorAttachments()[0].Resource;
}

void AFrameRenderer::AddOutlineOverlayPass( AFrameGraph & FrameGraph, AFrameGraphTexture * RenderTarget, AFrameGraphTexture * OutlineMaskTexture )
{
    RenderCore::TEXTURE_FORMAT pf = TEXTURE_FORMAT_RG8;

    ARenderPass & blurPass = FrameGraph.AddTask< ARenderPass >( "Outline Blur Pass" );

    blurPass.SetDynamicRenderArea( &GRenderViewArea );

    blurPass.AddResource( OutlineMaskTexture, RESOURCE_ACCESS_READ );

    blurPass.SetColorAttachments(
    {
        {
            "Outline blured mask",
            RenderCore::MakeTexture( pf, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );

    blurPass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        rtbl->BindTexture( 0, OutlineMaskTexture->Actual() );

        DrawSAQ( OutlineBlurPipe );
    } );

    AFrameGraphTexture * OutlineBlurTexture = blurPass.GetColorAttachments()[0].Resource;

    ARenderPass & applyPass = FrameGraph.AddTask< ARenderPass >( "Outline Apply Pass" );

    applyPass.SetDynamicRenderArea( &GRenderViewArea );

    applyPass.AddResource( OutlineMaskTexture, RESOURCE_ACCESS_READ );
    applyPass.AddResource( OutlineBlurTexture, RESOURCE_ACCESS_READ );

    applyPass.SetColorAttachments(
    {
        {
            RenderTarget,
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        }
    }
    );

    applyPass.AddSubpass( { 0 }, // color attachment refs
                         [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace RenderCore;

        rtbl->BindTexture( 0, OutlineMaskTexture->Actual() );
        rtbl->BindTexture( 1, OutlineBlurTexture->Actual() );

        DrawSAQ( OutlineApplyPipe );
    } );
}

void AFrameRenderer::Render( AFrameGraph & FrameGraph, bool bVirtualTexturing, AVirtualTextureCache * PhysCacheVT, SFrameGraphCaptured & CapturedResources )
{
    AScopedTimeCheck TimeCheck( "Framegraph build&fill" );

    FrameGraph.Clear();

    if ( bVirtualTexturing ) {
        GRenderView->VTFeedback->AddPass( FrameGraph );
    }

    AFrameGraphTexture * ShadowMapDepth[MAX_DIRECTIONAL_LIGHTS] = {};

    for ( int lightIndex = 0 ; lightIndex < GRenderView->NumDirectionalLights ; lightIndex++ ) {
        int lightOffset = GRenderView->FirstDirectionalLight + lightIndex;

        SDirectionalLightInstance * dirLight = GFrameData->DirectionalLights[ lightOffset ];

        ShadowMapRenderer.AddPass( FrameGraph, dirLight, &ShadowMapDepth[lightIndex] );
    }
    for ( int lightIndex = GRenderView->NumDirectionalLights ; lightIndex < MAX_DIRECTIONAL_LIGHTS ; lightIndex++ ) {
        ShadowMapRenderer.AddDummyShadowMap( FrameGraph, &ShadowMapDepth[lightIndex] );
    }


    AFrameGraphTexture * DepthTexture, * VelocityTexture;
    AddDepthPass( FrameGraph, &DepthTexture, &VelocityTexture );

    AFrameGraphTexture * LinearDepth;
    AddLinearizeDepthPass( FrameGraph, DepthTexture, &LinearDepth );

    AFrameGraphTexture * NormalTexture;
    AddReconstrutNormalsPass( FrameGraph, LinearDepth, &NormalTexture );

    AFrameGraphTexture * SSAOTexture;
    if ( r_HBAO ) {
        SSAORenderer.AddPasses( FrameGraph, LinearDepth, NormalTexture, &SSAOTexture );
    }
    else {
        SSAOTexture = FrameGraph.AddExternalResource(
            "White Texture",
            RenderCore::STextureCreateInfo(),
            GWhiteTexture
        );
    }

    AFrameGraphTexture * LightTexture;
    LightRenderer.AddPass( FrameGraph, DepthTexture, SSAOTexture, ShadowMapDepth[0], ShadowMapDepth[1], ShadowMapDepth[2], ShadowMapDepth[3], LinearDepth, &LightTexture );

    if ( r_MotionBlur ) {
        AddMotionBlurPass( FrameGraph, LightTexture, VelocityTexture, LinearDepth, &LightTexture );
    }

    ABloomRenderer::STextures BloomTex;
    BloomRenderer.AddPasses( FrameGraph, LightTexture, &BloomTex );

    AFrameGraphTexture * Exposure;
    ExposureRenderer.AddPass( FrameGraph, LightTexture, &Exposure );

    AFrameGraphTexture * ColorGrading;
    ColorGradingRenderer.AddPass( FrameGraph, &ColorGrading );

    AFrameGraphTexture * PostprocessTexture;
    PostprocessRenderer.AddPass( FrameGraph, LightTexture, Exposure, ColorGrading, BloomTex, &PostprocessTexture );

    AFrameGraphTexture * OutlineTexture;
    AddOutlinePass( FrameGraph, &OutlineTexture );

    if ( OutlineTexture ) {
        AddOutlineOverlayPass( FrameGraph, PostprocessTexture, OutlineTexture );
    }

    AFrameGraphTexture * FinalTexture;
    if ( r_FXAA ) {
        FxaaRenderer.AddPass( FrameGraph, PostprocessTexture, &FinalTexture );
    }
    else {
        FinalTexture = PostprocessTexture;
    }

    if ( GRenderView->bWireframe ) {
        AddWireframePass( FrameGraph, FinalTexture );
    }

    if ( r_ShowNormals ) {
        AddNormalsPass( FrameGraph, FinalTexture );
    }

    if ( GRenderView->DebugDrawCommandCount > 0 ) {
        DebugDrawRenderer.AddPass( FrameGraph, FinalTexture, DepthTexture );
    }

    if ( bVirtualTexturing ) {
        if ( r_ShowFeedbackVT ) {
            GRenderView->VTFeedback->DrawFeedback( FrameGraph, FinalTexture );
        }

        if ( r_ShowCacheVT.GetInteger() >= 0 ) {
            PhysCacheVT->Draw( FrameGraph, FinalTexture, r_ShowCacheVT.GetInteger() );
        }
    }

    FinalTexture->SetResourceCapture( true );

    CapturedResources.FinalTexture = FinalTexture;

    FrameGraph.Build();

    //FrameGraph.ExportGraphviz( "framegraph.graphviz" );
}
