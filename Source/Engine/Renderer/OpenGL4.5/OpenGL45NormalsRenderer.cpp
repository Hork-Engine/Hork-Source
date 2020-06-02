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

#include "OpenGL45NormalsRenderer.h"
#include "OpenGL45Material.h"

using namespace GHI;

namespace OpenGL45 {

static bool BindMaterialNormalPass( SRenderInstance const * instance ) {
    AMaterialGPU * pMaterial = instance->Material;
    Pipeline * pPipeline;

    AN_ASSERT( pMaterial );

    bool bSkinned = instance->SkeletonSize > 0;

    // Choose pipeline
    switch ( pMaterial->MaterialType ) {
    case MATERIAL_TYPE_UNLIT:

        pPipeline = bSkinned ? &((AShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->NormalsPassSkinned
                             : &((AShadeModelUnlit*)pMaterial->ShadeModel.Unlit)->NormalsPass;

        break;

    case MATERIAL_TYPE_PBR:
    case MATERIAL_TYPE_BASELIGHT:

        pPipeline = bSkinned ? &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->NormalsPassSkinned
                             : &((AShadeModelLit*)pMaterial->ShadeModel.Lit)->NormalsPass;

        break;

    default:
        return false;
    }

    // Bind pipeline
    Cmd.BindPipeline( pPipeline );

    // Bind second vertex buffer
    if ( bSkinned ) {
        Buffer * pSecondVertexBuffer = GPUBufferHandle( instance->WeightsBuffer );
        Cmd.BindVertexBuffer( 1, pSecondVertexBuffer, instance->WeightsBufferOffset );
    } else {
        Cmd.BindVertexBuffer( 1, nullptr, 0 );
    }

    // Set samplers
    if ( pMaterial->bNormalsPassTextureFetch ) {
        for ( int i = 0 ; i < pMaterial->NumSamplers ; i++ ) {
            GFrameResources.SamplerBindings[i].pSampler = pMaterial->pSampler[i];
        }
    }

    // Bind vertex and index buffers
    BindVertexAndIndexBuffers( instance );

    return true;
}

static void BindTexturesNormalsPass( SMaterialFrameData * _Instance ) {
    if ( !_Instance->Material->bNormalsPassTextureFetch ) {
        return;
    }

    BindTextures( _Instance );
}

void ANormalsRenderer::AddPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * RenderTarget )
{
    ARenderPass & normalPass = FrameGraph.AddTask< ARenderPass >( "Normal Pass" );

    normalPass.SetDynamicRenderArea( &GRenderViewArea );

    normalPass.SetColorAttachments(
    {
        {
            RenderTarget,
            GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        }
    }
    );

    //normalPass.SetCondition( []() { return RVDrawNormals.GetBool(); } );

    normalPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        DrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

            if ( !BindMaterialNormalPass( instance ) ) {
                continue;
            }

            // Set material data (textures, uniforms)
            BindTexturesNormalsPass( instance->MaterialInstance );

            // Bind skeleton
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );

            // Set instance uniforms
            SetInstanceUniforms( instance, i );

            Cmd.BindShaderResources( &GFrameResources.Resources );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            Cmd.Draw( &drawCmd );
        }
    } );
}

}
