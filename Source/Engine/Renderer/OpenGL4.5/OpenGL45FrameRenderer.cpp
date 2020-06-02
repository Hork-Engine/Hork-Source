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

#include "OpenGL45FrameRenderer.h"

#include <Runtime/Public/ScopedTimeCheck.h>

ARuntimeVariable RVFxaa( _CTS( "FXAA" ), _CTS( "1" ) );
ARuntimeVariable RVDrawNormals( _CTS( "DrawNormals" ), _CTS( "0" ), VAR_CHEAT );

using namespace GHI;

namespace OpenGL45 {

AFrameRenderer::AFrameRenderer()
{
    CreateFullscreenQuadPipeline( LinearDepthPipe, "postprocess/linear_depth.vert", "postprocess/linear_depth.frag" );
    CreateFullscreenQuadPipeline( LinearDepthPipe_ORTHO, "postprocess/linear_depth.vert", "postprocess/linear_depth_ortho.frag" );
    CreateFullscreenQuadPipeline( ReconstructNormalPipe, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal.frag" );
    CreateFullscreenQuadPipeline( ReconstructNormalPipe_ORTHO, "postprocess/reconstruct_normal.vert", "postprocess/reconstruct_normal_ortho.frag" );

    {
        SamplerCreateInfo samplerCI;
        samplerCI.SetDefaults();
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        NearestSampler = GDevice.GetOrCreateSampler( samplerCI );
    }
}

AFrameGraphTextureStorage * AFrameRenderer::AddLinearizeDepthPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * DepthTexture )
{
    ARenderPass & linearizeDepthPass = FrameGraph.AddTask< ARenderPass >( "Linearize Depth Pass" );
    linearizeDepthPass.SetDynamicRenderArea( &GRenderViewArea );
    linearizeDepthPass.AddResource( DepthTexture, RESOURCE_ACCESS_READ );
    linearizeDepthPass.SetColorAttachments(
    {
        {
            "Linear depth texture",
            MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_R32F, GetFrameResoultion() ),
            GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    linearizeDepthPass.AddSubpass( { 0 }, // color attachment refs
                                   [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        GFrameResources.TextureBindings[0].pTexture = DepthTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( &LinearDepthPipe );
        } else {
            DrawSAQ( &LinearDepthPipe_ORTHO );
        }
    } );
    return linearizeDepthPass.GetColorAttachments()[0].Resource;
}

AFrameGraphTextureStorage * AFrameRenderer::AddReconstrutNormalsPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * LinearDepth )
{
    ARenderPass & reconstructNormalPass = FrameGraph.AddTask< ARenderPass >( "Reconstruct Normal Pass" );
    reconstructNormalPass.SetDynamicRenderArea( &GRenderViewArea );
    reconstructNormalPass.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
    reconstructNormalPass.SetColorAttachments(
    {
        {
            "Normal texture",
            MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_RGB8, GetFrameResoultion() ),
            GHI::AttachmentInfo().SetLoadOp( GHI::ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
    reconstructNormalPass.AddSubpass( { 0 }, // color attachment refs
                                      [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        using namespace GHI;

        GFrameResources.TextureBindings[0].pTexture = LinearDepth->Actual();
        GFrameResources.SamplerBindings[0].pSampler = NearestSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        if ( GRenderView->bPerspective ) {
            DrawSAQ( &ReconstructNormalPipe );
        } else {
            DrawSAQ( &ReconstructNormalPipe_ORTHO );
        }
    } );
    return reconstructNormalPass.GetColorAttachments()[0].Resource;
}

void AFrameRenderer::Render( AFrameGraph & FrameGraph, SFrameGraphCaptured & CapturedResources )
{
    AScopedTimeCheck TimeCheck( "Framegraph build&fill" );

    FrameGraph.Clear();

    AFrameGraphTextureStorage * ShadowMapDepth = ShadowMapRenderer.AddPass( FrameGraph );

    AFrameGraphTextureStorage * DepthTexture = DepthRenderer.AddPass( FrameGraph );

    AFrameGraphTextureStorage * LinearDepth = AddLinearizeDepthPass( FrameGraph, DepthTexture );

    AFrameGraphTextureStorage * NormalTexture = AddReconstrutNormalsPass( FrameGraph, LinearDepth );

    AFrameGraphTextureStorage * SSAOTexture = SSAORenderer.AddPasses( FrameGraph, LinearDepth, NormalTexture );

    AFrameGraphTextureStorage * LightTexture = LightRenderer.AddPass( FrameGraph, DepthTexture, SSAOTexture, ShadowMapDepth );

    ABloomRenderer::STextures BloomTex = BloomRenderer.AddPasses( FrameGraph, LightTexture );

    AFrameGraphTextureStorage * Exposure = ExposureRenderer.AddPass( FrameGraph, LightTexture );

    AFrameGraphTextureStorage * ColorGrading = ColorGradingRenderer.AddPass( FrameGraph );

    AFrameGraphTextureStorage * PostprocessTexture = PostprocessRenderer.AddPass( FrameGraph, LightTexture, Exposure, ColorGrading, BloomTex );

    AFrameGraphTextureStorage * FinalTexture = 
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

    FinalTexture->SetResourceCapture( true );

    CapturedResources.FinalTexture = FinalTexture;

    FrameGraph.Build();

    //FrameGraph.ExportGraphviz( "framegraph.graphviz" );
}

}
