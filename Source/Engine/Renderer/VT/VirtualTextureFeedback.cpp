/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "VirtualTextureFeedback.h"
#include "../RenderLocal.h"
#include "../Material.h"

/*

Usage:

AVirtualTextureFeedback * feedbackBuffer = GRenderView->FeedbackBuffer;

feedbackBuffer->Begin( GRenderView->Width, GRenderView->Height );

RenderFeedback( feedbackBuffer->GetFeedbackTexture(), feedbackBuffer->GetFeedbackDepth(), feedbackBuffer->GetResolutionRatio() );

feedbackBuffer->End( Allocator, &FeedbackSize, &FeedbackData );

*/

using namespace RenderCore;

ARuntimeVariable r_FeedbackResolutionFactorVT( _CTS( "r_FeedbackResolutionFactorVT" ), _CTS( "16" ) );
ARuntimeVariable r_RenderFeedback( _CTS( "r_RenderFeedback" ), _CTS( "1" ) );

// TODO: Move to project settings?
//static const INTERNAL_PIXEL_FORMAT FEEDBACK_DEPTH_FORMAT = TEXTURE_FORMAT_DEPTH16;
static const TEXTURE_FORMAT FEEDBACK_DEPTH_FORMAT = TEXTURE_FORMAT_DEPTH24;
//static const INTERNAL_PIXEL_FORMAT FEEDBACK_DEPTH_FORMAT = TEXTURE_FORMAT_DEPTH32;

AVirtualTextureFeedback::AVirtualTextureFeedback()
    : SwapIndex( 0 )
    , ResolutionRatio( 0.0f )
{
    FeedbackSize[0] = FeedbackSize[1] = 0;
    MappedData[0] = MappedData[1] = 0;

    SPipelineResourceLayout resourceLayout;

    SSamplerDesc nearestSampler;
    nearestSampler.Filter = FILTER_NEAREST;
    nearestSampler.AddressU = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressV = SAMPLER_ADDRESS_CLAMP;
    nearestSampler.AddressW = SAMPLER_ADDRESS_CLAMP;

    resourceLayout.NumSamplers = 1;
    resourceLayout.Samplers = &nearestSampler;

    CreateFullscreenQuadPipeline( &DrawFeedbackPipeline, "drawfeedback.vert", "drawfeedback.frag", &resourceLayout );
}

AVirtualTextureFeedback::~AVirtualTextureFeedback()
{
    if ( MappedData[SwapIndex] ) {
        rcmd->UnmapBuffer(PixelBufferObject[SwapIndex]);
    }
}

void AVirtualTextureFeedback::Begin( int Width, int Height )
{
    float resolutionScale = Math::Max( r_FeedbackResolutionFactorVT.GetFloat(), 1.0f );
    resolutionScale = 1.0f / resolutionScale;

    uint32_t feedbackWidth = Width * resolutionScale;
    uint32_t feedbackHeight = Height * resolutionScale;

    // Choose acceptable size of feedback buffer
    // Max feedback buffer size is 0xffff
    if ( feedbackWidth * feedbackHeight >= 0xffff ) {
        float aspect = (float)feedbackWidth / Width;
        float w = Math::Floor( Math::Sqrt( float( 0xffff-1 )*aspect ) );
        feedbackWidth = w;
        feedbackHeight = Math::Floor( w / aspect );
        AN_ASSERT( feedbackWidth*feedbackHeight < 0xffff );
    }

    FeedbackSize[SwapIndex] = feedbackWidth * feedbackHeight;

    ResolutionRatio.X = (float)feedbackWidth / Width;
    ResolutionRatio.Y = (float)feedbackHeight / Height;

    if ( MappedData[SwapIndex] ) {
        MappedData[SwapIndex] = nullptr;
        rcmd->UnmapBuffer(PixelBufferObject[SwapIndex]);
    }

    if ( !FeedbackTexture || FeedbackTexture->GetWidth() != feedbackWidth || FeedbackTexture->GetHeight() != feedbackHeight )
    {
        GDevice->CreateTexture(STextureDesc()
                                   .SetFormat(TEXTURE_FORMAT_RGBA8)
                                   .SetResolution(STextureResolution2D(feedbackWidth, feedbackHeight)),
                               &FeedbackTexture);
        GDevice->CreateTexture(STextureDesc()
                                   .SetFormat(FEEDBACK_DEPTH_FORMAT)
                                   .SetResolution(STextureResolution2D(feedbackWidth, feedbackHeight)),
                               &FeedbackDepth);
    }

    size_t feedbackSizeInBytes = FeedbackSize[SwapIndex] * 4;

    if (!PixelBufferObject[SwapIndex] || PixelBufferObject[SwapIndex]->GetDesc().SizeInBytes != feedbackSizeInBytes)
    {
        SBufferDesc bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)(
            IMMUTABLE_MAP_READ
            //| IMMUTABLE_MAP_CLIENT_STORAGE
            | IMMUTABLE_MAP_PERSISTENT
            | IMMUTABLE_MAP_COHERENT
            );
        bufferCI.SizeInBytes = feedbackSizeInBytes;

        GDevice->CreateBuffer( bufferCI, nullptr, &PixelBufferObject[SwapIndex] );
        PixelBufferObject[SwapIndex]->SetDebugName( "Virtual texture feedback PBO" );
    }
}

void AVirtualTextureFeedback::End( int * pFeedbackSize, const void ** ppData )
{
    SwapIndex = (SwapIndex + 1) & 1;

    size_t sizeInBytes = FeedbackSize[SwapIndex] * 4; // 4 bytes per pixel

    *ppData = nullptr;
    *pFeedbackSize = 0;

    if ( sizeInBytes > 0 ) {
        if ( PixelBufferObject[SwapIndex] ) {
            MappedData[SwapIndex] = rcmd->MapBuffer(PixelBufferObject[SwapIndex],
                                                    MAP_TRANSFER_READ,
                                                    MAP_NO_INVALIDATE,
                                                    MAP_PERSISTENT_COHERENT,
                                                    false,
                                                    false);
            if (MappedData[SwapIndex])
            {
                *ppData = MappedData[SwapIndex];
                *pFeedbackSize = FeedbackSize[SwapIndex];
            }
        }
    }
}

static bool BindMaterialFeedbackPass(IImmediateContext* immediateCtx, SRenderInstance const * Instance )
{
    AMaterialGPU * pMaterial = Instance->Material;
    IBuffer * pSecondVertexBuffer = nullptr;
    size_t secondBufferOffset = 0;

    AN_ASSERT( pMaterial );

    int bSkinned = Instance->SkeletonSize > 0;

    IPipeline * pPipeline = pMaterial->FeedbackPass[bSkinned];
    if ( !pPipeline ) {
        return false;
    }

    if ( bSkinned ) {
        pSecondVertexBuffer = Instance->WeightsBuffer;
        secondBufferOffset = Instance->WeightsBufferOffset;
    }

    // Bind pipeline
    immediateCtx->BindPipeline( pPipeline );

    // Bind second vertex buffer
    immediateCtx->BindVertexBuffer(1, pSecondVertexBuffer, secondBufferOffset);

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers(immediateCtx, Instance);

    return true;
}

void AVirtualTextureFeedback::AddPass( AFrameGraph & FrameGraph )
{
    if ( !r_RenderFeedback )
        return;

    FGTextureProxy* FeedbackDepth_R   = FrameGraph.AddExternalResource<FGTextureProxy>("VT Feedback depth", GetFeedbackDepth());
    FGTextureProxy* FeedbackTexture_R = FrameGraph.AddExternalResource<FGTextureProxy>("VT Feedback texture", GetFeedbackTexture());

    ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "VT Feedback Pass" );

    pass.SetRenderArea( GetFeedbackTexture()->GetWidth(), GetFeedbackTexture()->GetHeight() );

    pass.SetColorAttachment(
        STextureAttachment(FeedbackTexture_R)
            .SetLoadOp(ATTACHMENT_LOAD_OP_CLEAR)
            .SetClearValue(MakeClearColorValue(0.0f, 0.0f, 0.0f, 0.0f)));

    pass.SetDepthStencilAttachment(
        STextureAttachment(FeedbackDepth_R)
        .SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
        //.SetStoreOp( ATTACHMENT_STORE_OP_DONT_CARE )  // TODO: Check
    );

    pass.AddSubpass( { 0 }, // color attachment refs
                    [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
    {
        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

        SDrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        // NOTE:
        // 1. Meshes with one material and same virtual texture can be batched to one mesh/drawcall
        // 2. We can draw geometry only with virtual texturing

        for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

            // Choose pipeline and second vertex buffer
            if (!BindMaterialFeedbackPass(immediateCtx, instance))
            {
                continue;
            }

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

            // Set instance constants
            BindInstanceConstantsFB( instance );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            immediateCtx->Draw(&drawCmd);
        }

        SRect2D r;
        r.X = 0;
        r.Y = 0;
        r.Width  = RenderPassContext.RenderArea.Width;
        r.Height = RenderPassContext.RenderArea.Height;

        immediateCtx->CopyColorAttachmentToBuffer(RenderPassContext,
                                                                  GetPixelBuffer(),
                                                                  0,
                                                                  r,
                                                                  FB_CHANNEL_BGRA,
                                                                  FB_UBYTE,
                                                                  COLOR_CLAMP_OFF,
                                                                  r.Width * r.Height * 4,
                                                                  0,
                                                                  4);
    } );
}

void AVirtualTextureFeedback::DrawFeedback( AFrameGraph & FrameGraph, FGTextureProxy * RenderTarget )
{
    FGTextureProxy* FeedbackTexture_R = FrameGraph.AddExternalResource<FGTextureProxy>("VT Feedback texture", GetFeedbackTexture());

    ARenderPass & pass = FrameGraph.AddTask< ARenderPass >( "VT Draw Feedback Pass" );

    pass.SetRenderArea(
                GRenderView->Width * 0.25f,
                GRenderView->Height * 0.25f,
                GRenderView->Width * 0.5f,
                GRenderView->Height * 0.5f
                );

    pass.AddResource( FeedbackTexture_R, FG_RESOURCE_ACCESS_READ );

    pass.SetColorAttachment(
        STextureAttachment(RenderTarget)
        .SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
    );

    pass.AddSubpass( { 0 }, // color attachment refs
                    [=](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
    {
        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

        rtbl->BindTexture( 0, FeedbackTexture_R->Actual() );

        DrawSAQ(immediateCtx, DrawFeedbackPipeline);
    } );
}
