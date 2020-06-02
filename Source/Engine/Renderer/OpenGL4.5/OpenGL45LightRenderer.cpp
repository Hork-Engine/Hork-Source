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

#include "OpenGL45LightRenderer.h"
#include "OpenGL45ShadowMapRenderer.h"
#include "OpenGL45Material.h"

#include <Core/Public/Logger.h>

ARuntimeVariable RVFramebufferTextureFormat( _CTS( "FramebufferTextureFormat" ), _CTS( "0" ) );

using namespace GHI;

namespace OpenGL45 {

ALightRenderer::ALightRenderer()
{
    {
        SamplerCreateInfo samplerCI;
        samplerCI.SetDefaults();
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_WRAP;
        samplerCI.AddressV = SAMPLER_ADDRESS_WRAP;
        samplerCI.AddressW = SAMPLER_ADDRESS_WRAP;
        samplerCI.MaxAnisotropy = 0;//16;
        LightmapSampler = GDevice.GetOrCreateSampler( samplerCI );
    }

    {
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
        ShadowDepthSamplerPCF = GDevice.GetOrCreateSampler( samplerCI );
    }

    {
        SamplerCreateInfo samplerCI;

        samplerCI.SetDefaults();
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressV = SAMPLER_ADDRESS_BORDER;
        samplerCI.AddressW = SAMPLER_ADDRESS_BORDER;
        samplerCI.MipLODBias = 0;
        samplerCI.MaxAnisotropy = 0;

        samplerCI.BorderColor[0] = VSM_ClearValue.X;
        samplerCI.BorderColor[1] = VSM_ClearValue.Y;
        samplerCI.BorderColor[2] = VSM_ClearValue.Z;
        samplerCI.BorderColor[3] = VSM_ClearValue.W;

        ShadowDepthSamplerVSM = GDevice.GetOrCreateSampler( samplerCI );

        samplerCI.BorderColor[0] = EVSM_ClearValue.X;
        samplerCI.BorderColor[1] = EVSM_ClearValue.Y;
        samplerCI.BorderColor[2] = EVSM_ClearValue.Z;
        samplerCI.BorderColor[3] = EVSM_ClearValue.W;

        ShadowDepthSamplerEVSM = GDevice.GetOrCreateSampler( samplerCI );
    }

    {
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
        ShadowDepthSamplerPCSS0 = GDevice.GetOrCreateSampler( samplerCI );

        // PCF_Sampler
        samplerCI.Filter = FILTER_LINEAR; //GHI_Filter_Min_LinearMipmapLinear_Mag_Linear; // D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR  Is the same?
        samplerCI.ComparisonFunc = CMPFUNC_LESS;
        samplerCI.bCompareRefToTexture = true;
        samplerCI.BorderColor[0] = samplerCI.BorderColor[1] = samplerCI.BorderColor[2] = samplerCI.BorderColor[3] = 1.0f; // FIXME?
        ShadowDepthSamplerPCSS1 = GDevice.GetOrCreateSampler( samplerCI );
    }

    {
        GHI::SamplerCreateInfo samplerCI = {};
        samplerCI.SetDefaults();
        samplerCI.Filter = FILTER_LINEAR;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        IESSampler = GDevice.GetOrCreateSampler( samplerCI );
    }

    {
        GHI::SamplerCreateInfo samplerCI = {};
        samplerCI.SetDefaults();
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        ClusterLookupSampler = GDevice.GetOrCreateSampler( samplerCI );
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
        if ( f.OpenWrite( "brdf.png" ) ) {
            byte * b = (byte *)GHunkMemory.Alloc( sizeX*sizeY*3 );
            for ( int n = sizeX*sizeY, i = 0 ; i < n ; i++ ) {
                b[i*3] = Math::Saturate( data[i].X ) * 255;
                b[i*3+1] = Math::Saturate( data[i].Y ) * 255;
                b[i*3+2] = 0;
            }
            WritePNG( f, sizeX, sizeY, 3, b, sizeX*3 );
            GHunkMemory.ClearLastHunk();
        }

        if ( f.OpenWrite( "brdf.bin" ) ) {
            f.WriteBuffer( data, sizeInBytes );
        }
    } else {
        f.ReadBuffer( data, sizeInBytes );
    }

    GHI::TextureStorageCreateInfo createInfo = {};
    createInfo.Type = GHI::TEXTURE_2D;
    createInfo.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG16F;
    createInfo.Resolution.Tex2D.Width = sizeX;
    createInfo.Resolution.Tex2D.Height = sizeY;
    createInfo.NumLods = 1;
    LookupBRDF.InitializeStorage( createInfo );
    LookupBRDF.Write( 0, GHI::PIXEL_FORMAT_FLOAT_RG, sizeInBytes, 1, data );

    GHunkMemory.ClearLastHunk();

    GHI::SamplerCreateInfo samplerCI = {};
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
    LookupBRDFSampler = GDevice.GetOrCreateSampler( samplerCI );
}

bool ALightRenderer::BindMaterialColorPass( SRenderInstance const * Instance )
{
    AMaterialGPU * pMaterial = Instance->Material;
    Pipeline * pPipeline;
    Buffer * pSecondVertexBuffer = nullptr;
    size_t secondBufferOffset = 0;

    AN_ASSERT( pMaterial );

    bool bSkinned = Instance->SkeletonSize > 0;
    bool bLightmap = Instance->LightmapUVChannel != nullptr && Instance->Lightmap;
    bool bVertexLight = Instance->VertexLightChannel != nullptr;

    switch ( pMaterial->MaterialType ) {
    case MATERIAL_TYPE_UNLIT:

        pPipeline = bSkinned ? &((AShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->ColorPassSkinned
                             : &((AShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->ColorPassSimple;

        if ( bSkinned ) {
            pSecondVertexBuffer = GPUBufferHandle( Instance->WeightsBuffer );
            secondBufferOffset = Instance->WeightsBufferOffset;
        }

        break;

    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:

        if ( bSkinned ) {

            pPipeline = &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->ColorPassSkinned;

            pSecondVertexBuffer = GPUBufferHandle( Instance->WeightsBuffer );
            secondBufferOffset = Instance->WeightsBufferOffset;

        } else if ( bLightmap ) {

            pPipeline = &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->ColorPassLightmap;

            pSecondVertexBuffer = GPUBufferHandle( Instance->LightmapUVChannel );
            secondBufferOffset = Instance->LightmapUVOffset;

            // lightmap is in last sample
            GFrameResources.TextureBindings[pMaterial->LightmapSlot].pTexture = GPUTextureHandle( Instance->Lightmap );
            GFrameResources.SamplerBindings[pMaterial->LightmapSlot].pSampler = LightmapSampler;

        } else if ( bVertexLight ) {

            pPipeline = &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->ColorPassVertexLight;

            pSecondVertexBuffer = GPUBufferHandle( Instance->VertexLightChannel );
            secondBufferOffset = Instance->VertexLightOffset;

        } else {

            pPipeline = &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->ColorPassSimple;

            pSecondVertexBuffer = nullptr;
        }

        break;

    default:
        return false;
    }

    // Bind pipeline
    Cmd.BindPipeline( pPipeline );

    // Bind second vertex buffer
    Cmd.BindVertexBuffer( 1, pSecondVertexBuffer, secondBufferOffset );

    // Set samplers
    if ( pMaterial->bColorPassTextureFetch ) {
        for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
            GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
        }
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( Instance );

    return true;
}

void ALightRenderer::BindTexturesColorPass( SMaterialFrameData * _Instance )
{
    if ( !_Instance->Material->bColorPassTextureFetch ) {
        return;
    }

    BindTextures( _Instance );
}

AFrameGraphTextureStorage * ALightRenderer::AddPass( AFrameGraph & FrameGraph,
                                                    AFrameGraphTextureStorage * DepthTarget,
                                                    AFrameGraphTextureStorage * SSAOTexture,
                                                    AFrameGraphTextureStorage * ShadowMapDepth )
{
    AFrameGraphTextureStorage * PhotometricProfiles_R = FrameGraph.AddExternalResource(
        "Photometric Profiles",
        GHI::TextureStorageCreateInfo(),
        GPUTextureHandle( GRenderView->PhotometricProfiles )
    );

    AFrameGraphTextureStorage * LookupBRDF_R = FrameGraph.AddExternalResource(
        "Lookup BRDF",
        GHI::TextureStorageCreateInfo(),
        &LookupBRDF
    );

    AFrameGraphTextureStorage * ClusterItemTBO_R = FrameGraph.AddExternalResource(
        "Cluster Item TBO",
        GHI::TextureStorageCreateInfo(),
        &GFrameResources.ClusterItemTBO
    );

    AFrameGraphTextureStorage * ClusterLookup_R = FrameGraph.AddExternalResource(
        "Cluster lookup texture",
        GHI::TextureStorageCreateInfo(),
        &GFrameResources.ClusterLookup
    );

    GHI::INTERNAL_PIXEL_FORMAT pf;
    switch ( RVFramebufferTextureFormat ) {
    case 0:
        // Pretty good. No significant visual difference between INTERNAL_PIXEL_FORMAT_RGB16F.
        pf = INTERNAL_PIXEL_FORMAT_R11F_G11F_B10F;
        break;
    default:
        pf = INTERNAL_PIXEL_FORMAT_RGB16F;
        break;
    }

    ARenderPass & colorPass = FrameGraph.AddTask< ARenderPass >( "Color Pass" );

    colorPass.SetDynamicRenderArea( &GRenderViewArea );

    colorPass.AddResource( SSAOTexture, RESOURCE_ACCESS_READ );
    colorPass.AddResource( PhotometricProfiles_R, RESOURCE_ACCESS_READ );
    colorPass.AddResource( LookupBRDF_R, RESOURCE_ACCESS_READ );
    colorPass.AddResource( ClusterItemTBO_R, RESOURCE_ACCESS_READ );
    colorPass.AddResource( ClusterLookup_R, RESOURCE_ACCESS_READ );
    colorPass.AddResource( ShadowMapDepth, RESOURCE_ACCESS_READ );

    colorPass.SetColorAttachments(
    {
        {
            "Light texture",
            GHI::MakeTextureStorage( pf, GetFrameResoultion() ),
            GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    }
    );

    colorPass.SetDepthStencilAttachment(
    {
        DepthTarget,
        GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
    } );

    colorPass.AddSubpass( { 0 }, // color attachment refs
                          [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        // Clearing don't work properly with dynamic resolution scale :(
#if 0
        if ( GRenderView->bClearBackground || RVRenderSnapshot ) {
            unsigned int attachment = 0;
            Cmd.ClearFramebufferAttachments( GRenderTarget.GetFramebuffer(), &attachment, 1, &clearValue, nullptr, nullptr );
        }
#endif

        DrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        GFrameResources.TextureBindings[10].pTexture = PhotometricProfiles_R->Actual();
        GFrameResources.SamplerBindings[10].pSampler = IESSampler;

        GFrameResources.TextureBindings[11].pTexture = LookupBRDF_R->Actual();
        GFrameResources.SamplerBindings[11].pSampler = LookupBRDFSampler;

        // Bind ambient occlusion
        GFrameResources.TextureBindings[12].pTexture = SSAOTexture->Actual();
        //GFrameResources.SamplerBindings[12].pSampler = AOSampler;

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
            if ( !BindMaterialColorPass( instance ) ) {
                continue;
            }

            // Set material data (textures, uniforms)
            BindTexturesColorPass( instance->MaterialInstance );

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

            // Set instance uniforms
            SetInstanceUniforms( instance, i );

            Cmd.BindShaderResources( &GFrameResources.Resources );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            Cmd.Draw( &drawCmd );

            //if ( RVRenderSnapshot ) {
            //    SaveSnapshot();
            //}
        }

        int translucentUniformOffset = GRenderView->InstanceCount;

        for ( int i = 0 ; i < GRenderView->TranslucentInstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->TranslucentInstances[GRenderView->FirstTranslucentInstance + i];

            // Choose pipeline and second vertex buffer
            if ( !BindMaterialColorPass( instance ) ) {
                continue;
            }

            // Set material data (textures, uniforms)
            BindTexturesColorPass( instance->MaterialInstance );

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

            // Set instance uniforms
            SetInstanceUniforms( instance, translucentUniformOffset + i );

            Cmd.BindShaderResources( &GFrameResources.Resources );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            Cmd.Draw( &drawCmd );

            //if ( RVRenderSnapshot ) {
            //    SaveSnapshot();
            //}
        }

    } );

    return colorPass.GetColorAttachments()[0].Resource;
}

}
