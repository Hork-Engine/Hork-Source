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

#include "LightRenderer.h"
#include "ShadowMapRenderer.h"
#include "Material.h"
#include "RenderBackend.h"

#include <Core/Public/Logger.h>

ARuntimeVariable RVFramebufferTextureFormat( _CTS( "FramebufferTextureFormat" ), _CTS( "0" ) );

using namespace RenderCore;

ALightRenderer::ALightRenderer()
{
    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_WRAP;
        samplerCI.AddressV = SAMPLER_ADDRESS_WRAP;
        samplerCI.AddressW = SAMPLER_ADDRESS_WRAP;
        GDevice->GetOrCreateSampler( samplerCI, &LightmapSampler );
    }

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_MIPMAP_BILINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
        GDevice->GetOrCreateSampler( samplerCI, &ReflectSampler );
    }

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &ReflectDepthSampler );
    }

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &VirtualTextureSampler );
    }

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_MIPMAP_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &VirtualTextureIndirectionSampler );
    }

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
        samplerCI.MipLODBias = 0;
        //samplerCI.ComparisonFunc = CMPFUNC_LEQUAL;
        samplerCI.ComparisonFunc = CMPFUNC_LESS;
        samplerCI.bCompareRefToTexture = true;
        GDevice->GetOrCreateSampler( samplerCI, &ShadowDepthSamplerPCF );
    }

    {
        SSamplerCreateInfo samplerCI;
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
        samplerCI.MipLODBias = 0;

        samplerCI.BorderColor[0] = VSM_ClearValue.X;
        samplerCI.BorderColor[1] = VSM_ClearValue.Y;
        samplerCI.BorderColor[2] = VSM_ClearValue.Z;
        samplerCI.BorderColor[3] = VSM_ClearValue.W;

        GDevice->GetOrCreateSampler( samplerCI, &ShadowDepthSamplerVSM );

        samplerCI.BorderColor[0] = EVSM_ClearValue.X;
        samplerCI.BorderColor[1] = EVSM_ClearValue.Y;
        samplerCI.BorderColor[2] = EVSM_ClearValue.Z;
        samplerCI.BorderColor[3] = EVSM_ClearValue.W;

        GDevice->GetOrCreateSampler( samplerCI, &ShadowDepthSamplerEVSM );
    }

    {
        SSamplerCreateInfo samplerCI;

        samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
        samplerCI.MipLODBias = 0;
        //samplerCI.BorderColor[0] = samplerCI.BorderColor[1] = samplerCI.BorderColor[2] = samplerCI.BorderColor[3] = 1.0f;

        // Find blocker point sampler
        samplerCI.Filter = FILTER_NEAREST;//FILTER_LINEAR;
                                          //samplerCI.ComparisonFunc = CMPFUNC_GREATER;//CMPFUNC_GEQUAL;
                                          //samplerCI.CompareRefToTexture = true;
        GDevice->GetOrCreateSampler( samplerCI, &ShadowDepthSamplerPCSS0 );

        // PCF_Sampler
        samplerCI.Filter = FILTER_LINEAR; //GHI_Filter_Min_LinearMipmapLinear_Mag_Linear; // D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR  Is the same?
        samplerCI.ComparisonFunc = CMPFUNC_LESS;
        samplerCI.bCompareRefToTexture = true;
        samplerCI.BorderColor[0] = samplerCI.BorderColor[1] = samplerCI.BorderColor[2] = samplerCI.BorderColor[3] = 1.0f; // FIXME?
        GDevice->GetOrCreateSampler( samplerCI, &ShadowDepthSamplerPCSS1 );
    }

    {
        RenderCore::SSamplerCreateInfo samplerCI = {};
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &IESSampler );
    }

    {
        RenderCore::SSamplerCreateInfo samplerCI = {};
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &ClusterLookupSampler );
    }

    {
        RenderCore::SSamplerCreateInfo samplerCI = {};
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        GDevice->GetOrCreateSampler( samplerCI, &SSAOSampler );
    }

    CreateLookupBRDF();
}

static Float2 Hammersley( int k, int n ) {
    float u = 0;
    float p = 0.5;
    for ( int kk = k; kk; p *= 0.5f, kk /= 2 ) {
        if ( kk & 1 ) {
            u += p;
        }
    }
    float x = u;
    float y = (k + 0.5f) / n;
    return Float2( x, y );
}

static Float3 ImportanceSampleGGX( Float2 const & Xi, float Roughness, Float3 const & N ) {
    float a = Roughness * Roughness;
    float Phi = 2 * Math::_PI * Xi.X;
    float CosTheta = sqrt( (1 - Xi.Y) / (1 + (a*a - 1) * Xi.Y) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );

    // Sphere to cart
    Float3 H;
    H.X = SinTheta * cos( Phi );
    H.Y = SinTheta * sin( Phi );
    H.Z = CosTheta;

    // Tangent to world space    
    Float3 UpVector = Math::Abs( N.Z ) < 0.99 ? Float3( 0, 0, 1 ) : Float3( 1, 0, 0 );
    Float3 Tangent = Math::Cross( UpVector, N ).Normalized();
    Float3 Bitangent = Math::Cross( N, Tangent );

    return (Tangent * H.X + Bitangent * H.Y + N * H.Z).Normalized();
}

static float GeometrySchlickGGX( float NdotV, float roughness )
{
    float a = roughness;
    float k = (a * a) / 2.0; // for IBL

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

static float GeometrySmith( Float3 N, Float3 V, Float3 L, float roughness )
{
    float NdotV = Math::Max( Math::Dot( N, V ), 0.0f );
    float NdotL = Math::Max( Math::Dot( N, L ), 0.0f );
    float ggx2 = GeometrySchlickGGX( NdotV, roughness );
    float ggx1 = GeometrySchlickGGX( NdotL, roughness );

    return ggx1 * ggx2;
}

static Float2 IntegrateBRDF( float NdotV, float roughness )
{
    Float3 V;
    V.X = sqrt( 1.0 - NdotV*NdotV );
    V.Y = 0.0;
    V.Z = NdotV;

    float A = 0.0;
    float B = 0.0;

    Float3 N = Float3( 0.0, 0.0, 1.0 );

    const int SAMPLE_COUNT = 1024;
    for ( int i = 0; i < SAMPLE_COUNT; ++i )
    {
        Float2 Xi = Hammersley( i, SAMPLE_COUNT );
        Float3 H = ImportanceSampleGGX( Xi, roughness, N );
        Float3 L = (2.0f * Math::Dot( V, H ) * H - V).Normalized();

        float NdotL = Math::Max( L.Z, 0.0f );
        float NdotH = Math::Max( H.Z, 0.0f );
        float VdotH = Math::Max( Math::Dot( V, H ), 0.0f );

        if ( NdotL > 0.0 )
        {
            float G = GeometrySmith( N, V, L, roughness );
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow( 1.0 - VdotH, 5.0 );

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float( SAMPLE_COUNT );
    B /= float( SAMPLE_COUNT );
    return Float2( A, B );
}

void ALightRenderer::CreateLookupBRDF() {
    AFileStream f;

    int sizeX = 512;
    int sizeY = 256; // enough for roughness
    int sizeInBytes = sizeX*sizeY*sizeof( Float2 );

    Float2 * data = (Float2 *)GHunkMemory.Alloc( sizeInBytes );

    if ( !f.OpenRead( "brdf.bin" ) ) {
        Float2 * pdata = data;
        for ( int y = 1 ; y <= sizeY ; y++ ) {
            float v = float( y ) / (sizeY);
            for ( int x = 1 ; x <= sizeX ; x++ ) {
                float u = float( x ) / (sizeX);
                *pdata++ = IntegrateBRDF( u, v );
            }
        }

        // Debug image
        //if ( f.OpenWrite( "brdf.png" ) ) {
        //    byte * b = (byte *)GHunkMemory.Alloc( sizeX*sizeY*3 );
        //    for ( int n = sizeX*sizeY, i = 0 ; i < n ; i++ ) {
        //        b[i*3] = Math::Saturate( data[i].X ) * 255;
        //        b[i*3+1] = Math::Saturate( data[i].Y ) * 255;
        //        b[i*3+2] = 0;
        //    }
        //    WritePNG( f, sizeX, sizeY, 3, b, sizeX*3 );
        //    GHunkMemory.ClearLastHunk();
        //}

        if ( f.OpenWrite( "brdf.bin" ) ) {
            f.WriteBuffer( data, sizeInBytes );
        }
    } else {
        f.ReadBuffer( data, sizeInBytes );
    }

    RenderCore::STextureCreateInfo createInfo = {};
    createInfo.Type = RenderCore::TEXTURE_2D;
    createInfo.Format = RenderCore::TEXTURE_FORMAT_RG16F;
    createInfo.Resolution.Tex2D.Width = sizeX;
    createInfo.Resolution.Tex2D.Height = sizeY;
    createInfo.NumLods = 1;
    GDevice->CreateTexture( createInfo, &LookupBRDF );
    LookupBRDF->Write( 0, RenderCore::FORMAT_FLOAT2, sizeInBytes, 1, data );

    GHunkMemory.ClearLastHunk();

    RenderCore::SSamplerCreateInfo samplerCI = {};
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    GDevice->GetOrCreateSampler( samplerCI, &LookupBRDFSampler );
}

bool ALightRenderer::BindMaterialLightPass( SRenderInstance const * Instance )
{
    AMaterialGPU * pMaterial = Instance->Material;
    IPipeline * pPipeline;
    IBuffer * pSecondVertexBuffer = nullptr;
    size_t secondBufferOffset = 0;

    AN_ASSERT( pMaterial );

    bool bSkinned = Instance->SkeletonSize > 0;
    bool bLightmap = Instance->LightmapUVChannel != nullptr && Instance->Lightmap;
    bool bVertexLight = Instance->VertexLightChannel != nullptr;

    switch ( pMaterial->MaterialType ) {
    case MATERIAL_TYPE_UNLIT:

        pPipeline = pMaterial->LightPass[bSkinned];

        if ( bSkinned ) {
            pSecondVertexBuffer = GPUBufferHandle( Instance->WeightsBuffer );
            secondBufferOffset = Instance->WeightsBufferOffset;
        }

        break;

    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:

        if ( bSkinned ) {

            pPipeline = pMaterial->LightPass[1];

            pSecondVertexBuffer = GPUBufferHandle( Instance->WeightsBuffer );
            secondBufferOffset = Instance->WeightsBufferOffset;

        } else if ( bLightmap ) {

            pPipeline = pMaterial->LightPassLightmap;

            pSecondVertexBuffer = GPUBufferHandle( Instance->LightmapUVChannel );
            secondBufferOffset = Instance->LightmapUVOffset;

            // lightmap is in last sample
            GFrameResources.TextureBindings[pMaterial->LightmapSlot].pTexture = GPUTextureHandle( Instance->Lightmap );
            GFrameResources.SamplerBindings[pMaterial->LightmapSlot].pSampler = LightmapSampler;

        } else if ( bVertexLight ) {

            pPipeline = pMaterial->LightPassVertexLight;

            pSecondVertexBuffer = GPUBufferHandle( Instance->VertexLightChannel );
            secondBufferOffset = Instance->VertexLightOffset;

        } else {

            pPipeline = pMaterial->LightPass[0];

            pSecondVertexBuffer = nullptr;
        }

        break;

    default:
        return false;
    }

    // Bind pipeline
    rcmd->BindPipeline( pPipeline );

    // Bind second vertex buffer
    rcmd->BindVertexBuffer( 1, pSecondVertexBuffer, secondBufferOffset );

    // Set samplers
    if ( pMaterial->bLightPassTextureFetch ) {
        for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
            GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
        }
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( Instance );

    //if ( Instance->bUseVT ) // TODO
    {
        int textureUnit = 0; // TODO: Instance->VTUnit;
        AVirtualTexture * pVirtualTex = GOpenGL45RenderBackend.VTWorkflow->FeedbackAnalyzer.GetTexture( textureUnit );
        //AN_ASSERT( pVirtualTex != nullptr );

        GFrameResources.TextureBindings[6].pTexture = GOpenGL45RenderBackend.VTWorkflow->PhysCache.GetLayers()[0];
        GFrameResources.SamplerBindings[6].pSampler = VirtualTextureSampler;

        if ( pVirtualTex ) {
            GFrameResources.TextureBindings[7].pTexture = pVirtualTex->GetIndirectionTexture();
            GFrameResources.SamplerBindings[7].pSampler = VirtualTextureIndirectionSampler;
        }
    }

    return true;
}

void ALightRenderer::BindTexturesLightPass( SMaterialFrameData * _Instance )
{
    if ( !_Instance->Material->bLightPassTextureFetch ) {
        return;
    }

    BindTextures( _Instance );
}

void ALightRenderer::AddPass( AFrameGraph & FrameGraph,
                              AFrameGraphTexture * DepthTarget,
                              AFrameGraphTexture * SSAOTexture,
                              AFrameGraphTexture * ShadowMapDepth,
                              AFrameGraphTexture * LinearDepth,
                              AFrameGraphTexture ** ppLight,
                              AFrameGraphTexture ** ppVelocity )
{
    AFrameGraphTexture * PhotometricProfiles_R = FrameGraph.AddExternalResource(
        "Photometric Profiles",
        RenderCore::STextureCreateInfo(),
        GPUTextureHandle( GRenderView->PhotometricProfiles )
    );

    AFrameGraphTexture * LookupBRDF_R = FrameGraph.AddExternalResource(
        "Lookup BRDF",
        RenderCore::STextureCreateInfo(),
        LookupBRDF
    );

    AFrameGraphBufferView * ClusterItemTBO_R = FrameGraph.AddExternalResource(
        "Cluster Item Buffer View",
        RenderCore::SBufferViewCreateInfo(),
        GFrameResources.ClusterItemTBO
    );

    AFrameGraphTexture * ClusterLookup_R = FrameGraph.AddExternalResource(
        "Cluster lookup texture",
        RenderCore::STextureCreateInfo(),
        GFrameResources.ClusterLookup
    );

    AFrameGraphTexture * ReflectionColor_R = FrameGraph.AddExternalResource(
        "Reflection color texture",
        RenderCore::STextureCreateInfo(),
        GPUTextureHandle( GRenderView->LightTexture )
    );

    AFrameGraphTexture * ReflectionDepth_R = FrameGraph.AddExternalResource(
        "Reflection depth texture",
        RenderCore::STextureCreateInfo(),
        GPUTextureHandle( GRenderView->DepthTexture )
    );

    RenderCore::TEXTURE_FORMAT pf;
    switch ( RVFramebufferTextureFormat ) {
    case 0:
        // Pretty good. No significant visual difference between TEXTURE_FORMAT_RGB16F.
        pf = TEXTURE_FORMAT_R11F_G11F_B10F;
        break;
    default:
        pf = TEXTURE_FORMAT_RGB16F;
        break;
    }

    ARenderPass & opaquePass = FrameGraph.AddTask< ARenderPass >( "Opaque Pass" );

    opaquePass.SetDynamicRenderArea( &GRenderViewArea );

    opaquePass.AddResource( SSAOTexture, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( PhotometricProfiles_R, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( LookupBRDF_R, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( ClusterItemTBO_R, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( ClusterLookup_R, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( ShadowMapDepth, RESOURCE_ACCESS_READ );

    if ( RVSSLR ) {
        opaquePass.AddResource( ReflectionColor_R, RESOURCE_ACCESS_READ );
        opaquePass.AddResource( ReflectionDepth_R, RESOURCE_ACCESS_READ );
    }

    if ( RVMotionBlur ) {
        opaquePass.SetColorAttachments(
        {
            {
                "Light texture",
                RenderCore::MakeTexture( pf, GetFrameResoultion() ),
                RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
            },
            {
                "Velocity texture",
                RenderCore::MakeTexture( TEXTURE_FORMAT_RG8, GetFrameResoultion() ),
                RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        }
        );
    }
    else {
        opaquePass.SetColorAttachments(
        {
            {
                "Light texture",
                RenderCore::MakeTexture( pf, GetFrameResoultion() ),
                RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
            }
        }
        );
    }

    opaquePass.SetDepthStencilAttachment(
    {
        DepthTarget,
        RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
    } );

    opaquePass.AddSubpass( { 0, 1 }, // color attachment refs
                          [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        // Clearing don't work properly with dynamic resolution scale :(
#if 0
        if ( GRenderView->bClearBackground || RVRenderSnapshot ) {
            unsigned int attachment = 0;
            Cmd.ClearFramebufferAttachments( GRenderTarget.GetFramebuffer(), &attachment, 1, &clearValue, nullptr, nullptr );
        }
#endif

        SDrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        if ( RVSSLR ) {
            GFrameResources.TextureBindings[8].pTexture = ReflectionDepth_R->Actual();
            GFrameResources.SamplerBindings[8].pSampler = ReflectDepthSampler;

            GFrameResources.TextureBindings[9].pTexture = ReflectionColor_R->Actual();
            GFrameResources.SamplerBindings[9].pSampler = ReflectSampler;
        }

        GFrameResources.TextureBindings[10].pTexture = PhotometricProfiles_R->Actual();
        GFrameResources.SamplerBindings[10].pSampler = IESSampler;

        GFrameResources.TextureBindings[11].pTexture = LookupBRDF_R->Actual();
        GFrameResources.SamplerBindings[11].pSampler = LookupBRDFSampler;

        // Bind ambient occlusion
        GFrameResources.TextureBindings[12].pTexture = SSAOTexture->Actual();
        GFrameResources.SamplerBindings[12].pSampler = SSAOSampler;

        // Bind cluster index buffer
        GFrameResources.TextureBindings[13].pTexture = ClusterItemTBO_R->Actual();
        GFrameResources.SamplerBindings[13].pSampler = ClusterLookupSampler;

        // Bind cluster lookup
        GFrameResources.TextureBindings[14].pTexture = ClusterLookup_R->Actual();
        GFrameResources.SamplerBindings[14].pSampler = ClusterLookupSampler;

        // Bind shadow map
        GFrameResources.TextureBindings[15].pTexture = ShadowMapDepth->Actual();
        GFrameResources.SamplerBindings[15].pSampler = ShadowDepthSamplerPCF;

        for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

            // Choose pipeline and second vertex buffer
            if ( !BindMaterialLightPass( instance ) ) {
                continue;
            }

            // Set material data (textures, uniforms)
            BindTexturesLightPass( instance->MaterialInstance );

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
            BindSkeletonMotionBlur( instance->SkeletonOffsetMB, instance->SkeletonSize );

            // Set instance uniforms
            SetInstanceUniforms( instance );

            rcmd->BindShaderResources( &GFrameResources.Resources );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );

            //if ( RVRenderSnapshot ) {
            //    SaveSnapshot();
            //}
        }
    } );

    AFrameGraphTexture * LightTexture = opaquePass.GetColorAttachments()[0].Resource;

    if ( GRenderView->TranslucentInstanceCount ) {
        ARenderPass & translucentPass = FrameGraph.AddTask< ARenderPass >( "Translucent Pass" );

        translucentPass.SetDynamicRenderArea( &GRenderViewArea );

        translucentPass.AddResource( SSAOTexture, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( PhotometricProfiles_R, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( LookupBRDF_R, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( ClusterItemTBO_R, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( ClusterLookup_R, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( ShadowMapDepth, RESOURCE_ACCESS_READ );

        if ( RVSSLR ) {
            translucentPass.AddResource( ReflectionColor_R, RESOURCE_ACCESS_READ );
            translucentPass.AddResource( ReflectionDepth_R, RESOURCE_ACCESS_READ );
        }

        translucentPass.SetColorAttachments(
        {
            {
                LightTexture,
                RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
            }
        } );

        translucentPass.SetDepthStencilAttachment(
        {
            DepthTarget,
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        } );

        translucentPass.AddSubpass( { 0 }, // color attachment refs
                                    [=]( ARenderPass const & RenderPass, int SubpassIndex )
        {
            SDrawIndexedCmd drawCmd;
            drawCmd.InstanceCount = 1;
            drawCmd.StartInstanceLocation = 0;

            if ( RVSSLR ) {
                GFrameResources.TextureBindings[8].pTexture = ReflectionDepth_R->Actual();
                GFrameResources.SamplerBindings[8].pSampler = ReflectDepthSampler;

                GFrameResources.TextureBindings[9].pTexture = ReflectionColor_R->Actual();
                GFrameResources.SamplerBindings[9].pSampler = ReflectSampler;
            }

            GFrameResources.TextureBindings[10].pTexture = PhotometricProfiles_R->Actual();
            GFrameResources.SamplerBindings[10].pSampler = IESSampler;

            GFrameResources.TextureBindings[11].pTexture = LookupBRDF_R->Actual();
            GFrameResources.SamplerBindings[11].pSampler = LookupBRDFSampler;

            // Bind ambient occlusion
            GFrameResources.TextureBindings[12].pTexture = SSAOTexture->Actual();
            GFrameResources.SamplerBindings[12].pSampler = SSAOSampler;

            // Bind cluster index buffer
            GFrameResources.TextureBindings[13].pTexture = ClusterItemTBO_R->Actual();
            GFrameResources.SamplerBindings[13].pSampler = ClusterLookupSampler;

            // Bind cluster lookup
            GFrameResources.TextureBindings[14].pTexture = ClusterLookup_R->Actual();
            GFrameResources.SamplerBindings[14].pSampler = ClusterLookupSampler;

            // Bind shadow map
            GFrameResources.TextureBindings[15].pTexture = ShadowMapDepth->Actual();
            GFrameResources.SamplerBindings[15].pSampler = ShadowDepthSamplerPCF;

            for ( int i = 0 ; i < GRenderView->TranslucentInstanceCount ; i++ ) {
                SRenderInstance const * instance = GFrameData->TranslucentInstances[GRenderView->FirstTranslucentInstance + i];

                // Choose pipeline and second vertex buffer
                if ( !BindMaterialLightPass( instance ) ) {
                    continue;
                }

                // Set material data (textures, uniforms)
                BindTexturesLightPass( instance->MaterialInstance );

                // Bind skeleton
                BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
                BindSkeletonMotionBlur( instance->SkeletonOffsetMB, instance->SkeletonSize );

                // Set instance uniforms
                SetInstanceUniforms( instance );

                rcmd->BindShaderResources( &GFrameResources.Resources );

                drawCmd.IndexCountPerInstance = instance->IndexCount;
                drawCmd.StartIndexLocation = instance->StartIndexLocation;
                drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

                rcmd->Draw( &drawCmd );

                //if ( RVRenderSnapshot ) {
                //    SaveSnapshot();
                //}
            }

        } );

        LightTexture = translucentPass.GetColorAttachments()[0].Resource;
    }

    if ( RVSSLR ) {
        // TODO: We can store reflection color and depth in one texture
        ACustomTask & task = FrameGraph.AddTask< ACustomTask >( "Copy Light Pass" );
        task.AddResource( LightTexture, RESOURCE_ACCESS_READ );
        task.AddResource( LinearDepth, RESOURCE_ACCESS_READ );
        task.AddResource( ReflectionColor_R, RESOURCE_ACCESS_WRITE );
        task.AddResource( ReflectionDepth_R, RESOURCE_ACCESS_WRITE );
        task.SetFunction( [=]( ACustomTask const & RenderTask )
        {
            RenderCore::TextureCopy Copy = {};
            Copy.SrcRect.Dimension.X = GRenderView->Width;
            Copy.SrcRect.Dimension.Y = GRenderView->Height;
            Copy.SrcRect.Dimension.Z = 1;

            {
            RenderCore::ITexture * pSource = LightTexture->Actual();
            RenderCore::ITexture * pDest = ReflectionColor_R->Actual();
            rcmd->CopyTextureRect( pSource, pDest, 1, &Copy );
            }

            {
            RenderCore::ITexture * pSource = LinearDepth->Actual();
            RenderCore::ITexture * pDest = ReflectionDepth_R->Actual();
            rcmd->CopyTextureRect( pSource, pDest, 1, &Copy );
            }
        } );
    }

    *ppLight = LightTexture;

    if ( RVMotionBlur ) {
        *ppVelocity = opaquePass.GetColorAttachments()[1].Resource;
    }
    else {
        *ppVelocity = nullptr;
    }
}
