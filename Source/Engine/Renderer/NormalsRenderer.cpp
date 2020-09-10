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

#include "NormalsRenderer.h"
#include "Material.h"

using namespace RenderCore;

static bool BindMaterialNormalPass( SRenderInstance const * instance )
{
    AMaterialGPU * pMaterial = instance->Material;

    AN_ASSERT( pMaterial );

    int bSkinned = instance->SkeletonSize > 0;

    IPipeline * pPipeline = pMaterial->NormalsPass[bSkinned];
    if ( !pPipeline ) {
        return false;
    }

    rcmd->BindPipeline( pPipeline );

    if ( bSkinned ) {
        IBuffer * pSecondVertexBuffer = instance->WeightsBuffer;
        rcmd->BindVertexBuffer( 1, pSecondVertexBuffer, instance->WeightsBufferOffset );
    }
    else {
        rcmd->BindVertexBuffer( 1, nullptr, 0 );
    }

    BindVertexAndIndexBuffers( instance );

    return true;
}

void AddNormalsPass( AFrameGraph & FrameGraph, AFrameGraphTexture * RenderTarget )
{
    ARenderPass & normalPass = FrameGraph.AddTask< ARenderPass >( "Normal Pass" );

    normalPass.SetDynamicRenderArea( &GRenderViewArea );

    normalPass.SetColorAttachments(
    {
        {
            RenderTarget,
            RenderCore::SAttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        }
    }
    );

    normalPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        SDrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
            SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

            if ( !BindMaterialNormalPass( instance ) ) {
                continue;
            }

            BindTextures( instance->MaterialInstance, instance->Material->NormalsPassTextureCount );
            BindSkeleton( instance->SkeletonOffset, instance->SkeletonSize );
            BindInstanceUniforms( instance );

            drawCmd.IndexCountPerInstance = instance->IndexCount;
            drawCmd.StartIndexLocation = instance->StartIndexLocation;
            drawCmd.BaseVertexLocation = instance->BaseVertexLocation;

            rcmd->Draw( &drawCmd );
        }
    } );
}
