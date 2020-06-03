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

#include "OpenGL45ExposureRenderer.h"
#include "OpenGL45Common.h"

#include <Runtime/Public/RuntimeVariable.h>

ARuntimeVariable RVShowDefaultExposure( _CTS( "ShowDefaultExposure" ), _CTS( "0" ) );

using namespace GHI;

namespace OpenGL45 {

AExposureRenderer::AExposureRenderer()
{
    // Create textures
    TextureStorageCreateInfo texStorageCI = {};
    texStorageCI.Type = GHI::TEXTURE_2D;
    texStorageCI.NumLods = 1;
    texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG16F;

    texStorageCI.Resolution.Tex2D.Width = 64;
    texStorageCI.Resolution.Tex2D.Height = 64;
    Luminance64.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 32;
    texStorageCI.Resolution.Tex2D.Height = 32;
    Luminance32.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 16;
    texStorageCI.Resolution.Tex2D.Height = 16;
    Luminance16.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 8;
    texStorageCI.Resolution.Tex2D.Height = 8;
    Luminance8.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 4;
    texStorageCI.Resolution.Tex2D.Height = 4;
    Luminance4.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 2;
    texStorageCI.Resolution.Tex2D.Height = 2;
    Luminance2.InitializeStorage( texStorageCI );

    byte defaultLum[2] = { 30, 30 }; // TODO: choose appropriate value
    texStorageCI.Resolution.Tex2D.Width = 1;
    texStorageCI.Resolution.Tex2D.Height = 1;
    texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG8;
    DefaultLuminance.InitializeStorage( texStorageCI );
    DefaultLuminance.Write( 0, GHI::PIXEL_FORMAT_UBYTE_RG, sizeof( defaultLum ), 1, defaultLum );

    CreateFullscreenQuadPipeline( MakeLuminanceMapPipe, "postprocess/exposure/make_luminance.vert", "postprocess/exposure/make_luminance.frag" );
    CreateFullscreenQuadPipeline( SumLuminanceMapPipe, "postprocess/exposure/sum_luminance.vert", "postprocess/exposure/sum_luminance.frag" );
    CreateFullscreenQuadPipeline( DynamicExposurePipe, "postprocess/exposure/dynamic_exposure.vert", "postprocess/exposure/dynamic_exposure.frag", GHI::BLENDING_ALPHA );

    CreateSampler();
}

void AExposureRenderer::CreateSampler() {
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    samplerCI.Filter = FILTER_LINEAR;
    LuminanceSampler = GDevice.GetOrCreateSampler( samplerCI );
}

AFrameGraphTextureStorage * AExposureRenderer::AddPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * SourceTexture ) {
    ATextureGPU * exposureTexture = GRenderView->CurrentExposure;

    if ( !exposureTexture || RVShowDefaultExposure ) {
        return FrameGraph.AddExternalResource< GHI::TextureStorageCreateInfo, GHI::Texture >(
            "Fallback exposure texture",
            MakeTextureStorage( GHI::INTERNAL_PIXEL_FORMAT_RG16F, GHI::TextureResolution2D( 1, 1 ) ),
            &DefaultLuminance );
    }

    auto Exposure_R = FrameGraph.AddExternalResource(
        "Exposure texture",
        GHI::TextureStorageCreateInfo(),
        GPUTextureHandle( exposureTexture )
    );
    auto Luminance64_R = FrameGraph.AddExternalResource(
        "Luminance64",
        GHI::TextureStorageCreateInfo(),
        &Luminance64
    );
    auto Luminance32_R = FrameGraph.AddExternalResource(
        "Luminance32",
        GHI::TextureStorageCreateInfo(),
        &Luminance32
    );
    auto Luminance16_R = FrameGraph.AddExternalResource(
        "Luminance16",
        GHI::TextureStorageCreateInfo(),
        &Luminance16
    );
    auto Luminance8_R = FrameGraph.AddExternalResource(
        "Luminance8",
        GHI::TextureStorageCreateInfo(),
        &Luminance8
    );
    auto Luminance4_R = FrameGraph.AddExternalResource(
        "Luminance4",
        GHI::TextureStorageCreateInfo(),
        &Luminance4
    );
    auto Luminance2_R = FrameGraph.AddExternalResource(
        "Luminance2",
        GHI::TextureStorageCreateInfo(),
        &Luminance2
    );

    // Make luminance map 64x64
    FrameGraph.AddTask< ARenderPass >( "Make luminance map 64x64" )
        .SetRenderArea( 64, 64 )
        .SetColorAttachments( { { Luminance64_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( SourceTexture, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = SourceTexture->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &MakeLuminanceMapPipe );
    });

    // Downscale luminance to 32x32
    FrameGraph.AddTask< ARenderPass >( "Downscale luminance to 32x32" )
        .SetRenderArea( 32, 32 )
        .SetColorAttachments( { { Luminance32_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( Luminance64_R, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = Luminance64_R->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &SumLuminanceMapPipe );
    } );

    // Downscale luminance to 16x16
    FrameGraph.AddTask< ARenderPass >( "Downscale luminance to 16x16" )
        .SetRenderArea( 16, 16 )
        .SetColorAttachments( { { Luminance16_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( Luminance32_R, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = Luminance32_R->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &SumLuminanceMapPipe );
    } );

    // Downscale luminance to 8x8
    FrameGraph.AddTask< ARenderPass >( "Downscale luminance to 8x8" )
        .SetRenderArea( 8, 8 )
        .SetColorAttachments( { { Luminance8_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( Luminance16_R, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = Luminance16_R->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &SumLuminanceMapPipe );
    } );

    // Downscale luminance to 4x4
    FrameGraph.AddTask< ARenderPass >( "Downscale luminance to 4x4" )
        .SetRenderArea( 4, 4 )
        .SetColorAttachments( { { Luminance4_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( Luminance8_R, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = Luminance8_R->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &SumLuminanceMapPipe );
    } );

    // Downscale luminance to 2x2
    FrameGraph.AddTask< ARenderPass >( "Downscale luminance to 2x2" )
        .SetRenderArea( 2, 2 )
        .SetColorAttachments( { { Luminance2_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( Luminance4_R, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = Luminance4_R->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &SumLuminanceMapPipe );
    } );

    // Render final exposure
    FrameGraph.AddTask< ARenderPass >( "Render final exposure" )
        .SetRenderArea( 1, 1 )
        .SetColorAttachments( { { Exposure_R, AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE ) } } )
        .AddResource( Luminance2_R, RESOURCE_ACCESS_READ )
        .AddSubpass( { 0 },
                     [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        GFrameResources.TextureBindings[0].pTexture = Luminance2_R->Actual();
        GFrameResources.SamplerBindings[0].pSampler = LuminanceSampler;

        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawSAQ( &DynamicExposurePipe );
    } );

    return Exposure_R;
}

}
