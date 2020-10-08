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
#include "RenderLocal.h"

#include <Core/Public/Logger.h>

ARuntimeVariable r_LightTextureFormat( _CTS( "r_LightTextureFormat" ), _CTS( "0" ), 0, _CTS( "0 - R11F_G11F_B10F, 1 - RGB16F" ) );

using namespace RenderCore;

ALightRenderer::ALightRenderer()
{
    CreateLookupBRDF();
}

static Float2 Hammersley( int k, int n )
{
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

static Float3 ImportanceSampleGGX( Float2 const & Xi, float Roughness, Float3 const & N )
{
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

void ALightRenderer::CreateLookupBRDF()
{
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
    }
    else {
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
            pSecondVertexBuffer = Instance->WeightsBuffer;
            secondBufferOffset = Instance->WeightsBufferOffset;
        }
        break;

    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:
        if ( bSkinned ) {
            pPipeline = pMaterial->LightPass[1];

            pSecondVertexBuffer = Instance->WeightsBuffer;
            secondBufferOffset = Instance->WeightsBufferOffset;
        }
        else if ( bLightmap ) {
            pPipeline = pMaterial->LightPassLightmap;

            pSecondVertexBuffer = Instance->LightmapUVChannel;
            secondBufferOffset = Instance->LightmapUVOffset;

            // lightmap is in last sample
            rtbl->BindTexture( pMaterial->LightmapSlot, Instance->Lightmap );
        }
        else if ( bVertexLight ) {
            pPipeline = pMaterial->LightPassVertexLight;

            pSecondVertexBuffer = Instance->VertexLightChannel;
            secondBufferOffset = Instance->VertexLightOffset;
        }
        else {
            pPipeline = pMaterial->LightPass[0];

            pSecondVertexBuffer = nullptr;
        }
        break;

    default:
        return false;
    }

    rcmd->BindPipeline( pPipeline );
    rcmd->BindVertexBuffer( 1, pSecondVertexBuffer, secondBufferOffset );

    BindVertexAndIndexBuffers( Instance );

    //if ( Instance->bUseVT ) // TODO
    {
        int textureUnit = 0; // TODO: Instance->VTUnit;
        AVirtualTexture * pVirtualTex = GRenderBackend.FeedbackAnalyzerVT->GetTexture( textureUnit );
        //AN_ASSERT( pVirtualTex != nullptr );

        rtbl->BindTexture( 6, GRenderBackend.PhysCacheVT->GetLayers()[0] );

        if ( pVirtualTex ) {
            rtbl->BindTexture( 7, pVirtualTex->GetIndirectionTexture() );
        }
    }

    return true;
}

void ALightRenderer::AddPass( AFrameGraph & FrameGraph,
                              AFrameGraphTexture * DepthTarget,
                              AFrameGraphTexture * SSAOTexture,
                              AFrameGraphTexture * ShadowMapDepth0,
                              AFrameGraphTexture * ShadowMapDepth1,
                              AFrameGraphTexture * ShadowMapDepth2,
                              AFrameGraphTexture * ShadowMapDepth3,
                              AFrameGraphTexture * LinearDepth,
                              AFrameGraphTexture ** ppLight/*,
                              AFrameGraphTexture ** ppVelocity*/ )
{
    AFrameGraphTexture * PhotometricProfiles_R = FrameGraph.AddExternalResource(
        "Photometric Profiles",
        RenderCore::STextureCreateInfo(),
        GRenderView->PhotometricProfiles
    );

    AFrameGraphTexture * LookupBRDF_R = FrameGraph.AddExternalResource(
        "Lookup BRDF",
        RenderCore::STextureCreateInfo(),
        LookupBRDF
    );

    AFrameGraphBufferView * ClusterItemTBO_R = FrameGraph.AddExternalResource(
        "Cluster Item Buffer View",
        RenderCore::SBufferViewCreateInfo(),
        GClusterItemTBO
    );

    AFrameGraphTexture * ClusterLookup_R = FrameGraph.AddExternalResource(
        "Cluster lookup texture",
        RenderCore::STextureCreateInfo(),
        GClusterLookup
    );

    AFrameGraphTexture * ReflectionColor_R = FrameGraph.AddExternalResource(
        "Reflection color texture",
        RenderCore::STextureCreateInfo(),
        GRenderView->LightTexture
    );

    AFrameGraphTexture * ReflectionDepth_R = FrameGraph.AddExternalResource(
        "Reflection depth texture",
        RenderCore::STextureCreateInfo(),
        GRenderView->DepthTexture
    );

    RenderCore::TEXTURE_FORMAT pf;
    switch ( r_LightTextureFormat ) {
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
    opaquePass.AddResource( ShadowMapDepth0, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( ShadowMapDepth1, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( ShadowMapDepth2, RESOURCE_ACCESS_READ );
    opaquePass.AddResource( ShadowMapDepth3, RESOURCE_ACCESS_READ );

    if ( r_SSLR ) {
        opaquePass.AddResource( ReflectionColor_R, RESOURCE_ACCESS_READ );
        opaquePass.AddResource( ReflectionDepth_R, RESOURCE_ACCESS_READ );
    }

    opaquePass.SetColorAttachments(
    {
        {
            "Light texture",
            RenderCore::MakeTexture( pf, GetFrameResoultion() ),
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
        }
    } );
 
    opaquePass.SetDepthStencilAttachment(
    {
        DepthTarget,
        RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
    } );

    opaquePass.AddSubpass( { 0 }, // color attachment refs
                          [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        // Clearing don't work properly with dynamic resolution scale :(
#if 0
        if ( GRenderView->bClearBackground || r_RenderSnapshot ) {
            unsigned int attachment = 0;
            Cmd.ClearFramebufferAttachments( GRenderTarget.GetFramebuffer(), &attachment, 1, &clearValue, nullptr, nullptr );
        }
#endif

        SDrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        BindShadowMatrix();

        if ( r_SSLR ) {
            rtbl->BindTexture( 8, ReflectionDepth_R->Actual() );
            rtbl->BindTexture( 9, ReflectionColor_R->Actual() );
        }

        rtbl->BindTexture( 10, PhotometricProfiles_R->Actual() );
        rtbl->BindTexture( 11, LookupBRDF_R->Actual() );

        // Bind ambient occlusion
        rtbl->BindTexture( 12, SSAOTexture->Actual() );

        // Bind cluster index buffer
        rtbl->BindTexture( 13, ClusterItemTBO_R->Actual() );

        // Bind cluster lookup
        rtbl->BindTexture( 14, ClusterLookup_R->Actual() );

        // Bind shadow map
        rtbl->BindTexture( 15, ShadowMapDepth0->Actual() );
        rtbl->BindTexture( 16, ShadowMapDepth1->Actual() );
        rtbl->BindTexture( 17, ShadowMapDepth2->Actual() );
        rtbl->BindTexture( 18, ShadowMapDepth3->Actual() );

        for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

            if ( !BindMaterialLightPass( instance ) ) {
                continue;
            }

            BindTextures( instance->MaterialInstance, instance->Material->LightPassTextureCount );
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
            BindInstanceConstants( instance );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );

            //if ( r_RenderSnapshot ) {
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
        translucentPass.AddResource( ShadowMapDepth0, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( ShadowMapDepth1, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( ShadowMapDepth2, RESOURCE_ACCESS_READ );
        translucentPass.AddResource( ShadowMapDepth3, RESOURCE_ACCESS_READ );

        if ( r_SSLR ) {
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

            BindShadowMatrix();

            if ( r_SSLR ) {
                rtbl->BindTexture( 8, ReflectionDepth_R->Actual() );
                rtbl->BindTexture( 9, ReflectionColor_R->Actual() );
            }

            rtbl->BindTexture( 10, PhotometricProfiles_R->Actual() );
            rtbl->BindTexture( 11, LookupBRDF_R->Actual() );

            // Bind ambient occlusion
            rtbl->BindTexture( 12, SSAOTexture->Actual() );

            // Bind cluster index buffer
            rtbl->BindTexture( 13, ClusterItemTBO_R->Actual() );

            // Bind cluster lookup
            rtbl->BindTexture( 14, ClusterLookup_R->Actual() );

            // Bind shadow map
            rtbl->BindTexture( 15, ShadowMapDepth0->Actual() );
            rtbl->BindTexture( 16, ShadowMapDepth1->Actual() );
            rtbl->BindTexture( 17, ShadowMapDepth2->Actual() );
            rtbl->BindTexture( 18, ShadowMapDepth3->Actual() );

            for ( int i = 0 ; i < GRenderView->TranslucentInstanceCount ; i++ ) {
                SRenderInstance const * instance = GFrameData->TranslucentInstances[GRenderView->FirstTranslucentInstance + i];

                if ( !BindMaterialLightPass( instance ) ) {
                    continue;
                }

                BindTextures( instance->MaterialInstance, instance->Material->LightPassTextureCount );
                BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
                BindInstanceConstants( instance );

                drawCmd.IndexCountPerInstance = instance->IndexCount;
                drawCmd.StartIndexLocation = instance->StartIndexLocation;
                drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

                rcmd->Draw( &drawCmd );

                //if ( r_RenderSnapshot ) {
                //    SaveSnapshot();
                //}
            }

        } );

        LightTexture = translucentPass.GetColorAttachments()[0].Resource;
    }

    if ( r_SSLR ) {
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
            Copy.SrcRect.Offset.Y = Copy.DstOffset.Y = GetFrameResoultion().Height - GRenderView->Height;

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
}
