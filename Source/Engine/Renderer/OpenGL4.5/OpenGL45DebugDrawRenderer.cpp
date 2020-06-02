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

#include "OpenGL45DebugDrawRenderer.h"

using namespace GHI;

namespace OpenGL45 {

ADebugDrawRenderer::ADebugDrawRenderer()
{
    //
    // Create pipeline
    //

    RasterizerStateInfo rsd;
    rsd.SetDefaults();
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = SCISSOR_TEST;
    //rsd.FillMode = POLYGON_FILL_WIRE;

    BlendingStateInfo bsd;
    bsd.SetDefaults();
    bsd.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );

    DepthStencilStateInfo dssd;
    dssd.SetDefaults();
    dssd.DepthFunc = CMPFUNC_GREATER;

    VertexAttribInfo vertexAttribs[2] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT3,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( SDebugVertex, Position )
        },
        {
            "InColor",
            1,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            GHI_STRUCT_OFS( SDebugVertex, Color )
        }
    };

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    AString vertesSourceCode = LoadShader( "debugdraw.vert" );
    AString fragmentSourceCode = LoadShader( "debugdraw.frag" );

    ShaderModule vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertesSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentSourceCode.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShaderModule );

    PipelineCreateInfo pipelineCI = {};

    PipelineInputAssemblyInfo inputAssembly = {};

    pipelineCI.pInputAssembly = &inputAssembly;
    pipelineCI.pBlending = &bsd;
    pipelineCI.pRasterizer = &rsd;
    pipelineCI.pDepthStencil = &dssd;

    ShaderStageInfo vs = {};
    vs.Stage = SHADER_STAGE_VERTEX_BIT;
    vs.pModule = &vertexShaderModule;

    ShaderStageInfo fs = {};
    fs.Stage = SHADER_STAGE_FRAGMENT_BIT;
    fs.pModule = &fragmentShaderModule;

    ShaderStageInfo stages[2] = { vs, fs };

    pipelineCI.NumStages = 2;
    pipelineCI.pStages = stages;

    VertexBindingInfo vertexBinding = {};
    vertexBinding.InputSlot = 0;
    vertexBinding.Stride = sizeof( SDebugVertex );
    vertexBinding.InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = 1;
    pipelineCI.pVertexBindings = &vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    for ( int i = 0 ; i < DBG_DRAW_CMD_MAX ; i++ ) {

        rsd.bAntialiasedLineEnable = false;

        switch ( i ) {
        case DBG_DRAW_CMD_POINTS:
            inputAssembly.Topology = PRIMITIVE_POINTS;
            inputAssembly.bPrimitiveRestart = false;
            dssd.bDepthEnable = false;
            dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
            break;
        case DBG_DRAW_CMD_POINTS_DEPTH_TEST:
            inputAssembly.Topology = PRIMITIVE_POINTS;
            inputAssembly.bPrimitiveRestart = false;
            dssd.bDepthEnable = true;
            dssd.DepthWriteMask = DEPTH_WRITE_ENABLE;
            break;
        case DBG_DRAW_CMD_LINES:
            inputAssembly.Topology = PRIMITIVE_LINE_STRIP;
            inputAssembly.bPrimitiveRestart = true;
            dssd.bDepthEnable = false;
            dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
            rsd.bAntialiasedLineEnable = true;
            break;
        case DBG_DRAW_CMD_LINES_DEPTH_TEST:
            inputAssembly.Topology = PRIMITIVE_LINE_STRIP;
            inputAssembly.bPrimitiveRestart = true;
            dssd.bDepthEnable = true;
            dssd.DepthWriteMask = DEPTH_WRITE_ENABLE;
            rsd.bAntialiasedLineEnable = true;
            break;
        case DBG_DRAW_CMD_TRIANGLE_SOUP:
            inputAssembly.Topology = PRIMITIVE_TRIANGLES;
            inputAssembly.bPrimitiveRestart = false;
            dssd.bDepthEnable = false;
            dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;
            break;
        case DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST:
            inputAssembly.Topology = PRIMITIVE_TRIANGLES;
            inputAssembly.bPrimitiveRestart = false;
            dssd.bDepthEnable = true;
            dssd.DepthWriteMask = DEPTH_WRITE_ENABLE;
            break;
        }

        Pipelines[i].Initialize( pipelineCI );
    }
}

void ADebugDrawRenderer::AddPass( AFrameGraph & FrameGraph, AFrameGraphTextureStorage * RenderTarget, AFrameGraphTextureStorage * DepthTexture ) {
    ARenderPass & renderPass = FrameGraph.AddTask< ARenderPass >( "Debug Draw Pass" );

    renderPass.SetDynamicRenderArea( &GRenderViewArea );

    renderPass.SetColorAttachments(
    {
        {
            RenderTarget,
            GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD )
        }
    }
    );

    renderPass.SetDepthStencilAttachment(
        { DepthTexture, GHI::AttachmentInfo().SetLoadOp( ATTACHMENT_LOAD_OP_LOAD ) }
    );

    renderPass.SetCondition( []() { return GRenderView->DebugDrawCommandCount > 0; } );

    renderPass.AddSubpass( { 0 }, // color attachment refs
                           [=]( ARenderPass const & RenderPass, int SubpassIndex )

    {
        Cmd.BindShaderResources( &GFrameResources.Resources );

        DrawIndexedCmd drawCmd;
        drawCmd.InstanceCount = 1;
        drawCmd.StartInstanceLocation = 0;

        Buffer * streamBuffer = GPUBufferHandle( GFrameData->StreamBuffer );

        for ( int i = 0 ; i < GRenderView->DebugDrawCommandCount ; i++ ) {
            SDebugDrawCmd const * cmd = &GFrameData->DbgCmds[GRenderView->FirstDebugDrawCommand + i];

            Cmd.BindPipeline( &Pipelines[cmd->Type] );
            Cmd.BindVertexBuffer( 0, streamBuffer, GFrameData->DbgVertexStreamOffset );
            Cmd.BindIndexBuffer( streamBuffer, INDEX_TYPE_UINT16, GFrameData->DbgIndexStreamOffset );

            drawCmd.IndexCountPerInstance = cmd->NumIndices;
            drawCmd.StartIndexLocation = cmd->FirstIndex;
            drawCmd.BaseVertexLocation = cmd->FirstVertex;

            Cmd.Draw( &drawCmd );
        }

    } );
}

}
