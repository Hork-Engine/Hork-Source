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

#include "FrameRenderer.h"
#include "RenderBackend.h"
#include "VT/VirtualTextureFeedback.h"

#include <Runtime/Public/ScopedTimeCheck.h>

ARuntimeVariable RVFxaa( _CTS( "FXAA" ), _CTS( "1" ) );
ARuntimeVariable RVDrawNormals( _CTS( "DrawNormals" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVVTDrawFeedback( _CTS( "VTDrawFeedback" ), _CTS( "0" ) );
ARuntimeVariable RVVTDrawCache( _CTS( "VTDrawCache" ), _CTS( "-1" ) );

using namespace RenderCore;

AFrameRenderer::AFrameRenderer()
{
    CreateFullscreenQuadPipeline( &LinearDepthPipe, "postprocess/linear_depth.vert", "postprocess/linear_depth.frag" );
    CreateFullscreenQuadPipeline( &LinearDepthPipe_ORTHO, "postprocess/linear_depth.vert", "postprocess/linear_depth_ortho.frag" );
    CreateFullscreenQuadPipeline( &ReconstructNormalPipe, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal.frag" );
    CreateFullscreenQuadPipeline( &ReconstructNormalPipe_ORTHO, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal_ortho.frag" );
    CreateFullscreenQuadPipeline( &MotionBlurPipeline, "postprocess/motionblur.vert", "postprocess/motionblur.frag" );

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &NearestSampler );
    }
    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &LinearSampler );
    }
}

AFrameGraphTexture * AFrameRenderer::AddLinearizeDepthPass( AFrameGraph & FrameGraph, AFrameGraphTexture * DepthTexture )
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

        GFrameResources.TextureBindings[0].pTexture = DepthTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        rcmd->BindShaderResources( &GFrameResources.Resources );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( LinearDepthPipe );
        } else {
            DrawSAQ( LinearDepthPipe_ORTHO );
        }
    } );
    return linearizeDepthPass.GetColorAttachments()[0].Resource;
}

AFrameGraphTexture * AFrameRenderer::AddReconstrutNormalsPass( AFrameGraph & FrameGraph, AFrameGraphTexture * LinearDepth )
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

        GFrameResources.TextureBindings[0].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        rcmd->BindShaderResources( &GFrameResources.Resources );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( ReconstructNormalPipe );
        } else {
            DrawSAQ( ReconstructNormalPipe_ORTHO );
        }
    } );
    return reconstructNormalPass.GetColorAttachments()[0].Resource;
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
        GFrameResources.TextureBindings[0].pTexture = LightTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LinearSampler;

        GFrameResources.TextureBindings[1].pTexture = VelocityTexture->Actual();
        GFrameResources.SamplerBindings[1].pSampler = NearestSampler;

        GFrameResources.TextureBindings[2].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[2].pSampler = NearestSampler;

        rcmd->BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( MotionBlurPipeline );

    } );

    *ppResultTexture = renderPass.GetColorAttachments()[0].Resource;
}

void AFrameRenderer::Render( AFrameGraph & FrameGraph, SVirtualTextureWorkflow * VTWorkflow, SFrameGraphCaptured & CapturedResources )
{
    AScopedTimeCheck TimeCheck( "Framegraph build&fill" );

    FrameGraph.Clear();

    if ( VTWorkflow ) {
        GRenderView->VTFeedback->AddPass( FrameGraph );
    }

    AFrameGraphTexture * ShadowMapDepth = ShadowMapRenderer.AddPass( FrameGraph );

    AFrameGraphTexture * DepthTexture = DepthRenderer.AddPass( FrameGraph );

    AFrameGraphTexture * LinearDepth = AddLinearizeDepthPass( FrameGraph, DepthTexture );

    AFrameGraphTexture * NormalTexture = AddReconstrutNormalsPass( FrameGraph, LinearDepth );

    AFrameGraphTexture * SSAOTexture = SSAORenderer.AddPasses( FrameGraph, LinearDepth, NormalTexture );

    AFrameGraphTexture * LightTexture;
    AFrameGraphTexture * VelocityTexture;
    
    LightRenderer.AddPass( FrameGraph, DepthTexture, SSAOTexture, ShadowMapDepth, LinearDepth, &LightTexture, &VelocityTexture );

    AddMotionBlurPass( FrameGraph, LightTexture, VelocityTexture, LinearDepth, &LightTexture );

    ABloomRenderer::STextures BloomTex = BloomRenderer.AddPasses( FrameGraph, LightTexture );

    AFrameGraphTexture * Exposure = ExposureRenderer.AddPass( FrameGraph, LightTexture );

    AFrameGraphTexture * ColorGrading = ColorGradingRenderer.AddPass( FrameGraph );

    AFrameGraphTexture * PostprocessTexture = PostprocessRenderer.AddPass( FrameGraph, LightTexture, Exposure, ColorGrading, BloomTex );

    AFrameGraphTexture * FinalTexture = 
        RVFxaa ? FxaaRenderer.AddPass( FrameGraph, PostprocessTexture ) : PostprocessTexture;

    if ( GRenderView->bWireframe ) {
        WireframeRenderer.AddPass( FrameGraph, FinalTexture );
    }

    if ( RVDrawNormals ) {
        NormalsRenderer.AddPass( FrameGraph, FinalTexture );
    }

    if ( GRenderView->DebugDrawCommandCount > 0 ) {
        DebugDrawRenderer.AddPass( FrameGraph, FinalTexture, DepthTexture );
    }

    if ( VTWorkflow ) {
        if ( RVVTDrawFeedback ) {
            GRenderView->VTFeedback->DrawFeedback( FrameGraph, FinalTexture );
        }

        if ( RVVTDrawCache.GetInteger() >= 0 ) {
            VTWorkflow->PhysCache.Draw( FrameGraph, FinalTexture, RVVTDrawCache.GetInteger() );
        }
    }

    FinalTexture->SetResourceCapture( true );

    CapturedResources.FinalTexture = FinalTexture;

    FrameGraph.Build();

    //FrameGraph.ExportGraphviz( "framegraph.graphviz" );
}
