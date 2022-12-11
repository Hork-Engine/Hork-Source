/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include <Platform/Logger.h>

ConsoleVar r_LightTextureFormat("r_LightTextureFormat"s, "0"s, 0, "0 - R11F_G11F_B10F, 1 - RGBA16F"s);

using namespace RenderCore;

LightRenderer::LightRenderer()
{
}

#if 0
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

void LightRenderer::CreateLookupBRDF()
{
    int sizeX = 512;
    int sizeY = 256; // enough for roughness
    int sizeInBytes = sizeX*sizeY*sizeof( Float2 );

    Float2 * data = (Float2 *)GHunkMemory.Alloc( sizeInBytes );

    File f = File::OpenRead( "brdf.bin" );
    if ( !f ) {
        Float2 * pdata = data;
        for ( int y = 1 ; y <= sizeY ; y++ ) {
            float v = float( y ) / (sizeY);
            for ( int x = 1 ; x <= sizeX ; x++ ) {
                float u = float( x ) / (sizeX);
                *pdata++ = IntegrateBRDF( u, v );
            }
        }

        // Debug image
        //f = File::OpenWrite( "brdf.png" );
        //if ( f ) {
        //    byte * b = (byte *)GHunkMemory.Alloc( sizeX*sizeY*3 );
        //    for ( int n = sizeX*sizeY, i = 0 ; i < n ; i++ ) {
        //        b[i*3] = Math::Saturate( data[i].X ) * 255;
        //        b[i*3+1] = Math::Saturate( data[i].Y ) * 255;
        //        b[i*3+2] = 0;
        //    }
        //    WritePNG( f, sizeX, sizeY, 3, b, sizeX*3 );
        //    GHunkMemory.ClearLastHunk();
        //}

        f = File::OpenWrite( "brdf.bin" );
        if ( f ) {
            f.WriteBuffer( data, sizeInBytes );
        }
    }
    else {
        f.ReadBuffer( data, sizeInBytes );
    }

    GDevice->CreateTexture(RenderCore::TextureDesc{}
                               .SetFormat(RenderCore::TEXTURE_FORMAT_RG16_FLOAT)
                               .SetResolution(TextureResolution2D(sizeX, sizeY))
                               .SetBindFlags(BIND_SHADER_RESOURCE),
                           &LookupBRDF);
    LookupBRDF->Write(0, RenderCore::FORMAT_FLOAT2, sizeInBytes, 1, data);

    GHunkMemory.ClearLastHunk();
}
#endif
bool LightRenderer::BindMaterialLightPass(IImmediateContext* immediateCtx, RenderInstance const* Instance)
{
    MaterialGPU* pMaterial = Instance->Material;
    IPipeline*    pPipeline;
    IBuffer*      pSecondVertexBuffer = nullptr;
    size_t        secondBufferOffset  = 0;

    HK_ASSERT(pMaterial);

    bool bSkinned     = Instance->SkeletonSize > 0;
    bool bLightmap    = Instance->LightmapUVChannel != nullptr && Instance->Lightmap;
    bool bVertexLight = Instance->VertexLightChannel != nullptr;

    switch (pMaterial->MaterialType)
    {
        case MATERIAL_TYPE_UNLIT:
            pPipeline = pMaterial->LightPass[bSkinned];
            if (bSkinned)
            {
                pSecondVertexBuffer = Instance->WeightsBuffer;
                secondBufferOffset  = Instance->WeightsBufferOffset;
            }
            break;

        case MATERIAL_TYPE_PBR:
        case MATERIAL_TYPE_BASELIGHT:
            if (bSkinned)
            {
                pPipeline = pMaterial->LightPass[1];

                pSecondVertexBuffer = Instance->WeightsBuffer;
                secondBufferOffset  = Instance->WeightsBufferOffset;
            }
            else if (bLightmap)
            {
                pPipeline = pMaterial->LightPassLightmap;

                pSecondVertexBuffer = Instance->LightmapUVChannel;
                secondBufferOffset  = Instance->LightmapUVOffset;

                // lightmap is in last sample
                rtbl->BindTexture(pMaterial->LightmapSlot, Instance->Lightmap);
            }
            else if (bVertexLight)
            {
                pPipeline = pMaterial->LightPassVertexLight;

                pSecondVertexBuffer = Instance->VertexLightChannel;
                secondBufferOffset  = Instance->VertexLightOffset;
            }
            else
            {
                pPipeline = pMaterial->LightPass[0];

                pSecondVertexBuffer = nullptr;
            }
            break;

        default:
            return false;
    }

    immediateCtx->BindPipeline(pPipeline);
    immediateCtx->BindVertexBuffer(1, pSecondVertexBuffer, secondBufferOffset);

    BindVertexAndIndexBuffers(immediateCtx, Instance);

    //if ( Instance->bUseVT ) // TODO
    {
        int              textureUnit = 0; // TODO: Instance->VTUnit;
        VirtualTexture* pVirtualTex = GFeedbackAnalyzerVT->GetTexture(textureUnit);
        //HK_ASSERT( pVirtualTex != nullptr );

        if (GPhysCacheVT)
            rtbl->BindTexture(6, GPhysCacheVT->GetLayers()[0]);

        if (pVirtualTex)
        {
            rtbl->BindTexture(7, pVirtualTex->GetIndirectionTexture());
        }
    }

    return true;
}

void LightRenderer::AddPass(FrameGraph&     FrameGraph,
                             FGTextureProxy*  DepthTarget,
                             FGTextureProxy*  SSAOTexture,
                             FGTextureProxy*  ShadowMapDepth0,
                             FGTextureProxy*  ShadowMapDepth1,
                             FGTextureProxy*  ShadowMapDepth2,
                             FGTextureProxy*  ShadowMapDepth3,
                             FGTextureProxy*  OmnidirectionalShadowMapArray,
                             FGTextureProxy*  LinearDepth,
                             FGTextureProxy** ppLight /*,
                              FGTextureProxy ** ppVelocity*/
)
{
    FGTextureProxy*    PhotometricProfiles_R = FrameGraph.AddExternalResource<FGTextureProxy>("Photometric Profiles", GRenderView->PhotometricProfiles);
    FGTextureProxy*    LookupBRDF_R          = FrameGraph.AddExternalResource<FGTextureProxy>("Lookup BRDF", GLookupBRDF);
    FGBufferViewProxy* ClusterItemTBO_R      = FrameGraph.AddExternalResource<FGBufferViewProxy>("Cluster Item Buffer View", GClusterItemTBO);
    FGTextureProxy*    ClusterLookup_R       = FrameGraph.AddExternalResource<FGTextureProxy>("Cluster lookup texture", GClusterLookup);
    FGTextureProxy*    ReflectionColor_R     = FrameGraph.AddExternalResource<FGTextureProxy>("Reflection color texture", GRenderView->LightTexture);
    FGTextureProxy*    ReflectionDepth_R     = FrameGraph.AddExternalResource<FGTextureProxy>("Reflection depth texture", GRenderView->DepthTexture);

    TEXTURE_FORMAT pf;
    switch (r_LightTextureFormat)
    {
        case 0:
            // Pretty good. No significant visual difference between TEXTURE_FORMAT_RGBA16_FLOAT.
            pf = TEXTURE_FORMAT_R11G11B10_FLOAT;
            break;
        default:
            pf = TEXTURE_FORMAT_RGBA16_FLOAT;
            break;
    }

    RenderPass& opaquePass = FrameGraph.AddTask<RenderPass>("Opaque Pass");

    opaquePass.SetRenderArea(GRenderViewArea);

    opaquePass.AddResource(SSAOTexture, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(PhotometricProfiles_R, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(LookupBRDF_R, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(ClusterItemTBO_R, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(ClusterLookup_R, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(ShadowMapDepth0, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(ShadowMapDepth1, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(ShadowMapDepth2, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(ShadowMapDepth3, FG_RESOURCE_ACCESS_READ);
    opaquePass.AddResource(OmnidirectionalShadowMapArray, FG_RESOURCE_ACCESS_READ);

    if (r_SSLR)
    {
        opaquePass.AddResource(ReflectionColor_R, FG_RESOURCE_ACCESS_READ);
        opaquePass.AddResource(ReflectionDepth_R, FG_RESOURCE_ACCESS_READ);
    }

    opaquePass.SetColorAttachment(
        TextureAttachment("Light texture",
                           TextureDesc()
                               .SetFormat(pf)
                               .SetResolution(GetFrameResoultion()))
            .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE));

    opaquePass.SetDepthStencilAttachment(
        TextureAttachment(DepthTarget)
            .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

    opaquePass.AddSubpass({0}, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                          {
                              // Clearing don't work properly with dynamic resolution scale :(
                              IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

#if 1
                              if (GRenderView->bClearBackground)
                              {
                                  unsigned int attachment = 0;

                                  ClearColorValue clearValue;
                                  clearValue.Float32[0] = GRenderView->BackgroundColor.X;
                                  clearValue.Float32[1] = GRenderView->BackgroundColor.Y;
                                  clearValue.Float32[2] = GRenderView->BackgroundColor.Z;
                                  clearValue.Float32[3] = 0;
                                  immediateCtx->ClearAttachments(RenderPassContext, &attachment, 1, &clearValue, nullptr, nullptr);
                              }
#endif
                              BindShadowMatrix();

                              if (r_SSLR)
                              {
                                  rtbl->BindTexture(8, ReflectionDepth_R->Actual());
                                  rtbl->BindTexture(9, ReflectionColor_R->Actual());
                              }

                              rtbl->BindTexture(10, PhotometricProfiles_R->Actual());
                              rtbl->BindTexture(11, LookupBRDF_R->Actual());

                              // Bind ambient occlusion
                              rtbl->BindTexture(12, SSAOTexture->Actual());

                              // Bind cluster index buffer
                              rtbl->BindTexture(13, ClusterItemTBO_R->Actual());

                              // Bind cluster lookup
                              rtbl->BindTexture(14, ClusterLookup_R->Actual());

                              // Bind shadow map
                              rtbl->BindTexture(15, ShadowMapDepth0->Actual());
                              rtbl->BindTexture(16, ShadowMapDepth1->Actual());
                              rtbl->BindTexture(17, ShadowMapDepth2->Actual());
                              rtbl->BindTexture(18, ShadowMapDepth3->Actual());
                              rtbl->BindTexture(19, OmnidirectionalShadowMapArray->Actual());

                              for (int i = 0; i < GRenderView->TerrainInstanceCount; i++)
                              {
                                  TerrainRenderInstance const* instance = GFrameData->TerrainInstances[GRenderView->FirstTerrainInstance + i];

                                  TerrainInstanceConstantBuffer* drawCall = MapDrawCallConstants<TerrainInstanceConstantBuffer>();
                                  drawCall->LocalViewProjection            = instance->LocalViewProjection;
                                  StoreFloat3x3AsFloat3x4Transposed(instance->ModelNormalToViewSpace, drawCall->ModelNormalToViewSpace);
                                  drawCall->ViewPositionAndHeight = instance->ViewPositionAndHeight;
                                  drawCall->TerrainClipMin        = instance->ClipMin;
                                  drawCall->TerrainClipMax        = instance->ClipMax;

                                  rtbl->BindTexture(0, instance->Clipmaps);
                                  rtbl->BindTexture(1, instance->Normals);
                                  immediateCtx->BindPipeline(GTerrainLightPipeline);
                                  immediateCtx->BindVertexBuffer(0, instance->VertexBuffer);
                                  immediateCtx->BindVertexBuffer(1, GStreamBuffer, instance->InstanceBufferStreamHandle);
                                  immediateCtx->BindIndexBuffer(instance->IndexBuffer, INDEX_TYPE_UINT16);
                                  immediateCtx->MultiDrawIndexedIndirect(instance->IndirectBufferDrawCount,
                                                                         GStreamBuffer,
                                                                         instance->IndirectBufferStreamHandle,
                                                                         sizeof(DrawIndexedIndirectCmd));
                              }

                              DrawIndexedCmd drawCmd;
                              drawCmd.InstanceCount         = 1;
                              drawCmd.StartInstanceLocation = 0;

                              for (int i = 0; i < GRenderView->InstanceCount; i++)
                              {
                                  RenderInstance const* instance = GFrameData->Instances[GRenderView->FirstInstance + i];

                                  if (!BindMaterialLightPass(immediateCtx, instance))
                                  {
                                      continue;
                                  }

                                  BindTextures(instance->MaterialInstance, instance->Material->LightPassTextureCount);
                                  BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                                  BindInstanceConstants(instance);

                                  drawCmd.IndexCountPerInstance = instance->IndexCount;
                                  drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                                  drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                                  immediateCtx->Draw(&drawCmd);

                                  //if ( r_RenderSnapshot ) {
                                  //    SaveSnapshot();
                                  //}
                              }
                          });

    FGTextureProxy* LightTexture = opaquePass.GetColorAttachments()[0].pResource;

    if (GRenderView->TranslucentInstanceCount)
    {
        RenderPass& translucentPass = FrameGraph.AddTask<RenderPass>("Translucent Pass");

        translucentPass.SetRenderArea(GRenderViewArea);

        translucentPass.AddResource(SSAOTexture, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(PhotometricProfiles_R, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(LookupBRDF_R, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(ClusterItemTBO_R, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(ClusterLookup_R, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(ShadowMapDepth0, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(ShadowMapDepth1, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(ShadowMapDepth2, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(ShadowMapDepth3, FG_RESOURCE_ACCESS_READ);
        translucentPass.AddResource(OmnidirectionalShadowMapArray, FG_RESOURCE_ACCESS_READ);

        if (r_SSLR)
        {
            translucentPass.AddResource(ReflectionColor_R, FG_RESOURCE_ACCESS_READ);
            translucentPass.AddResource(ReflectionDepth_R, FG_RESOURCE_ACCESS_READ);
        }

        translucentPass.SetColorAttachment(
            TextureAttachment(LightTexture)
                .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

        translucentPass.SetDepthStencilAttachment(
            TextureAttachment(DepthTarget)
                .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));

        translucentPass.AddSubpass({0}, // color attachment refs
                                   [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                                   {
                                       IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                                       DrawIndexedCmd drawCmd;
                                       drawCmd.InstanceCount         = 1;
                                       drawCmd.StartInstanceLocation = 0;

                                       BindShadowMatrix();

                                       if (r_SSLR)
                                       {
                                           rtbl->BindTexture(8, ReflectionDepth_R->Actual());
                                           rtbl->BindTexture(9, ReflectionColor_R->Actual());
                                       }

                                       rtbl->BindTexture(10, PhotometricProfiles_R->Actual());
                                       rtbl->BindTexture(11, LookupBRDF_R->Actual());

                                       // Bind ambient occlusion
                                       rtbl->BindTexture(12, SSAOTexture->Actual());

                                       // Bind cluster index buffer
                                       rtbl->BindTexture(13, ClusterItemTBO_R->Actual());

                                       // Bind cluster lookup
                                       rtbl->BindTexture(14, ClusterLookup_R->Actual());

                                       // Bind shadow map
                                       rtbl->BindTexture(15, ShadowMapDepth0->Actual());
                                       rtbl->BindTexture(16, ShadowMapDepth1->Actual());
                                       rtbl->BindTexture(17, ShadowMapDepth2->Actual());
                                       rtbl->BindTexture(18, ShadowMapDepth3->Actual());
                                       rtbl->BindTexture(19, OmnidirectionalShadowMapArray->Actual());

                                       for (int i = 0; i < GRenderView->TranslucentInstanceCount; i++)
                                       {
                                           RenderInstance const* instance = GFrameData->TranslucentInstances[GRenderView->FirstTranslucentInstance + i];

                                           if (!BindMaterialLightPass(immediateCtx, instance))
                                           {
                                               continue;
                                           }

                                           BindTextures(instance->MaterialInstance, instance->Material->LightPassTextureCount);
                                           BindSkeleton(instance->SkeletonOffset, instance->SkeletonSize);
                                           BindInstanceConstants(instance);

                                           drawCmd.IndexCountPerInstance = instance->IndexCount;
                                           drawCmd.StartIndexLocation    = instance->StartIndexLocation;
                                           drawCmd.BaseVertexLocation    = instance->BaseVertexLocation;

                                           immediateCtx->Draw(&drawCmd);

                                           //if ( r_RenderSnapshot ) {
                                           //    SaveSnapshot();
                                           //}
                                       }
                                   });

        LightTexture = translucentPass.GetColorAttachments()[0].pResource;
    }

    if (r_SSLR)
    {
        // TODO: We can store reflection color and depth in one texture
        FGCustomTask& task = FrameGraph.AddTask<FGCustomTask>("Copy Light Pass");
        task.AddResource(LightTexture, FG_RESOURCE_ACCESS_READ);
        task.AddResource(LinearDepth, FG_RESOURCE_ACCESS_READ);
        task.AddResource(ReflectionColor_R, FG_RESOURCE_ACCESS_WRITE);
        task.AddResource(ReflectionDepth_R, FG_RESOURCE_ACCESS_WRITE);
        task.SetFunction([=](FGCustomTaskContext const& Task)
                         {
                             IImmediateContext* immediateCtx = Task.pImmediateContext;

                             RenderCore::TextureCopy Copy = {};
                             Copy.SrcRect.Dimension.X     = GRenderView->Width;
                             Copy.SrcRect.Dimension.Y     = GRenderView->Height;
                             Copy.SrcRect.Dimension.Z     = 1;
                             Copy.SrcRect.Offset.Y = Copy.DstOffset.Y = GetFrameResoultion().Height - GRenderView->Height;

                             {
                                 RenderCore::ITexture* pSource = LightTexture->Actual();
                                 RenderCore::ITexture* pDest   = ReflectionColor_R->Actual();
                                 immediateCtx->CopyTextureRect(pSource, pDest, 1, &Copy);

                                 immediateCtx->GenerateTextureMipLevels(pDest);
                             }

                             {
                                 RenderCore::ITexture* pSource = LinearDepth->Actual();
                                 RenderCore::ITexture* pDest   = ReflectionDepth_R->Actual();
                                 immediateCtx->CopyTextureRect(pSource, pDest, 1, &Copy);
                             }
                         });
    }

    *ppLight = LightTexture;
}
