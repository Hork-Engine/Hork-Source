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

#include "RenderLocal.h"
#include "DebugDrawRenderer.h"

using namespace RenderCore;

DebugDrawRenderer::DebugDrawRenderer()
{
    //
    // Create pipeline
    //

    PipelineDesc pipelineCI;

    RasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;
    //rsd.FillMode = POLYGON_FILL_WIRE;

    BlendingStateInfo & bsd = pipelineCI.BS;
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    DepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.DepthFunc = CMPFUNC_GREATER;

    VertexAttribInfo vertexAttribs[2] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            HK_OFS( DebugVertex, Position )
        },
        {
            "InColor",
            1,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            HK_OFS( DebugVertex, Color )
        }
    };

    ShaderFactory::CreateVertexShader( "debugdraw.vert", vertexAttribs, HK_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    ShaderFactory::CreateFragmentShader( "debugdraw.frag", pipelineCI.pFS );

    PipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;

    VertexBindingInfo vertexBinding = {};
    vertexBinding.InputSlot = 0;
    vertexBinding.Stride = sizeof( DebugVertex );
    vertexBinding.InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings = &vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    BufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT; // view constants

    pipelineCI.ResourceLayout.NumBuffers = 1;
    pipelineCI.ResourceLayout.Buffers = &bufferInfo;

    for ( int i = 0 ; i < DBG_DRAW_CMD_MAX ; i++ ) {

        rsd.bAntialiasedLineEnable = false;

        switch ( i ) {
        case DBG_DRAW_CMD_POINTS:
            inputAssembly.Topology = PRIMITIVE_POINTS;
            dssd.bDepthEnable = false;
            dssd.bDepthWrite = false;
            break;
        case DBG_DRAW_CMD_POINTS_DEPTH_TEST:
            inputAssembly.Topology = PRIMITIVE_POINTS;
            dssd.bDepthEnable = true;
            dssd.bDepthWrite = true;
            break;
        case DBG_DRAW_CMD_LINES:
            inputAssembly.Topology = PRIMITIVE_LINE_STRIP;
            dssd.bDepthEnable = false;
            dssd.bDepthWrite = false;
            rsd.bAntialiasedLineEnable = true;
            break;
        case DBG_DRAW_CMD_LINES_DEPTH_TEST:
            inputAssembly.Topology = PRIMITIVE_LINE_STRIP;
            dssd.bDepthEnable = true;
            dssd.bDepthWrite = true;
            rsd.bAntialiasedLineEnable = true;
            break;
        case DBG_DRAW_CMD_TRIANGLE_SOUP:
            inputAssembly.Topology = PRIMITIVE_TRIANGLES;
            dssd.bDepthEnable = false;
            dssd.bDepthWrite = false;
            break;
        case DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST:
            inputAssembly.Topology = PRIMITIVE_TRIANGLES;
            dssd.bDepthEnable = true;
            dssd.bDepthWrite = true;
            break;
        }

        GDevice->CreatePipeline( pipelineCI, &Pipelines[i] );
    }
}

void DebugDrawRenderer::AddPass( FrameGraph & FrameGraph, FGTextureProxy * RenderTarget, FGTextureProxy * DepthTexture )
{
    if (GRenderView->DebugDrawCommandCount == 0)
    {
        return;
    }

    RenderPass & renderPass = FrameGraph.AddTask< RenderPass >( "Debug Draw Pass" );

    renderPass.SetRenderArea(GRenderViewArea);

    renderPass.SetColorAttachments(
    {
        {
            TextureAttachment(RenderTarget)
            .SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        }
    }
    );

    renderPass.SetDepthStencilAttachment(
        {
            TextureAttachment(DepthTexture)
            .SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        }
    );

    renderPass.AddSubpass( { 0 }, // color attachment refs
                          [=](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)

    {
        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

        DrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        for ( int i = 0 ; i < GRenderView->DebugDrawCommandCount ; i++ ) {
            DebugDrawCmd const * cmd = &GFrameData->DbgCmds[GRenderView->FirstDebugDrawCommand + i];

            immediateCtx->BindPipeline(Pipelines[cmd->Type]);
            immediateCtx->BindVertexBuffer(0, GStreamBuffer, GFrameData->DbgVertexStreamOffset);
            immediateCtx->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, GFrameData->DbgIndexStreamOffset);

            drawCmd.IndexCountPerInstance = cmd->NumIndices;
            drawCmd.StartIndexLocation = cmd->FirstIndex;
            drawCmd.BaseVertexLocation = cmd->FirstVertex;

            immediateCtx->Draw(&drawCmd);
        }

    } );
}
