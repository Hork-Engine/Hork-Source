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

#include "DepthRenderer.h"
#include "Material.h"

using namespace RenderCore;

static bool BindMaterialDepthPass( SRenderInstance const * instance ) {
    AMaterialGPU * pMaterial = instance->Material;
    IPipeline * pPipeline;

    AN_ASSERT( pMaterial );

    bool bSkinned = instance->SkeletonSize > 0;

    // Choose pipeline
    switch ( pMaterial->MaterialType ) {
    case MATERIAL_TYPE_UNLIT:
    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:
        pPipeline = bSkinned ? pMaterial->DepthPassSkinned
            : pMaterial->DepthPass;

        break;
    default:
        return false;
    }

    // Bind pipeline
    rcmd->BindPipeline( pPipeline );

    // Bind second vertex buffer
    if ( bSkinned ) {
        rcmd->BindVertexBuffer( 1, GPUBufferHandle( instance->WeightsBuffer ), instance->WeightsBufferOffset );
    } else {
        rcmd->BindVertexBuffer( 1, nullptr, 0 );
    }

    // Set samplers
    if ( pMaterial->bDepthPassTextureFetch ) {
        for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
            GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
        }
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( instance );

    return true;
}

static void BindTexturesDepthPass( SMaterialFrameData * _Instance ) {
    if ( !_Instance->Material->bDepthPassTextureFetch ) {
        return;
    }

    BindTextures( _Instance );
}

void AddDepthPass( AFrameGraph & FrameGraph, AFrameGraphTexture ** ppDepthTexture ) {
    ARenderPass & depthPass = FrameGraph.AddTask< ARenderPass >( "Depth Pre-Pass" );

    depthPass.SetDynamicRenderArea( &GRenderViewArea );

    depthPass.SetDepthStencilAttachment(
    {
        "Depth texture",
        MakeTexture( RenderCore::TEXTURE_FORMAT_DEPTH24_STENCIL8, GetFrameResoultion() ),
        RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_CLEAR )
    } );

    depthPass.AddSubpass( {}, // no color attachments
                          [=]( ARenderPass const & RenderPass, int SubpassIndex )
    {
        SDrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

            if ( !BindMaterialDepthPass( instance ) ) {
                continue;
            }

            // Set material data (textures, uniforms)
            BindTexturesDepthPass( instance->MaterialInstance );

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

            // Set instance uniforms
            SetInstanceUniforms( instance );

            rcmd->BindShaderResources( &GFrameResources.Resources );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }

    } );

    *ppDepthTexture = depthPass.GetDepthStencilAttachment().Resource;
}
