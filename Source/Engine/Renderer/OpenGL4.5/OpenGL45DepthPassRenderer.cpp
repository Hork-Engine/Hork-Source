/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "OpenGL45DepthPassRenderer.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"

using namespace GHI;

namespace OpenGL45 {

FDepthPassRenderer GDepthPassRenderer;

void FDepthPassRenderer::Initialize() {
    RenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 0;

    AttachmentInfo depthAttachment = {};
    depthAttachment.LoadOp = ATTACHMENT_LOAD_OP_CLEAR;
    renderPassCI.pDepthStencilAttachment = &depthAttachment;

    SubpassInfo subpass = {};
    subpass.NumColorAttachments = 0;

    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpass;

    DepthPass.Initialize( renderPassCI );
}

void FDepthPassRenderer::Deinitialize() {
    DepthPass.Deinitialize();
}

bool FDepthPassRenderer::BindMaterial( FRenderInstance const * instance ) {
    FMaterialGPU * pMaterial = instance->Material;
    Pipeline * pPipeline;

    AN_Assert( pMaterial );

    bool bSkinned = instance->SkeletonSize > 0;

    // Choose pipeline
    switch ( pMaterial->MaterialType ) {
    case MATERIAL_TYPE_UNLIT:

        pPipeline = bSkinned ? &((FShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->DepthPassSkinned
                             : &((FShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->DepthPass;

        break;

    case MATERIAL_TYPE_PBR:

        pPipeline = bSkinned ? &((FShadeModelPBR*)pMaterial->ShadeModel.PBR)->DepthPassSkinned
                             : &((FShadeModelPBR*)pMaterial->ShadeModel.PBR)->DepthPass;

        break;

    default:
        return false;
    }

    // Bind pipeline
    Cmd.BindPipeline( pPipeline );

    // Bind second vertex buffer
    Buffer * pSecondVertexBuffer = bSkinned ? GPUBufferHandle( instance->WeightsBuffer ) : nullptr;
    Cmd.BindVertexBuffer( 1, pSecondVertexBuffer, 0 );

    // Set samplers
    if ( pMaterial->bVertexTextureFetch ) {
        for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
            GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
        }
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( instance );

    return true;
}

void FDepthPassRenderer::RenderInstances() {
    ClearDepthStencilValue depthStencilValue = {};
    depthStencilValue.Depth = 0;
    depthStencilValue.Stencil = 0;

    RenderPassBegin renderPassBegin = {};
    renderPassBegin.pRenderPass = &DepthPass;
    renderPassBegin.pFramebuffer = &GRenderTarget.GetFramebuffer();
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width = GRenderView->Width;
    renderPassBegin.RenderArea.Height = GRenderView->Height;
    renderPassBegin.pColorClearValues = NULL;
    renderPassBegin.pDepthStencilClearValue = &depthStencilValue;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = GRenderView->Width;
    vp.Height = GRenderView->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

    DrawIndexedCmd drawCmd;
    drawCmd.InstanceCount = 1;
    drawCmd.StartInstanceLocation = 0;

    for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
        FRenderInstance const * instance = GFrameData->Instances[ GRenderView->FirstInstance + i ];

        if ( !BindMaterial( instance ) ) {
            continue;
        }

        // Set material data (textures, uniforms)
        BindMaterialInstanceVertexOnly( instance->MaterialInstance );

        // Bind skeleton
        BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

        // Set instance uniforms
        SetInstanceUniforms( i );

        Cmd.BindShaderResources( &GFrameResources.Resources );

        drawCmd.IndexCountPerInstance = instance->IndexCount;
        drawCmd.StartIndexLocation = instance->StartIndexLocation;
        drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

        Cmd.Draw( &drawCmd );
    }

    Cmd.EndRenderPass();
}

}
