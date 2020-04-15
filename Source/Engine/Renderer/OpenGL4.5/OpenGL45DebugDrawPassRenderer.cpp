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

#include "OpenGL45DebugDrawPassRenderer.h"
#include "OpenGL45FrameResources.h"
#include "OpenGL45RenderTarget.h"
#include "OpenGL45Material.h"
#include "OpenGL45ShaderSource.h"

using namespace GHI;

namespace OpenGL45 {

ADebugDrawPassRenderer GDebugDrawPassRenderer;

void ADebugDrawPassRenderer::Initialize() {
    RenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 1;

    AttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_LOAD;
    renderPassCI.pColorAttachments = &colorAttachment;

    AttachmentInfo depthAttachment = {};
    depthAttachment.LoadOp = ATTACHMENT_LOAD_OP_LOAD;
    renderPassCI.pDepthStencilAttachment = &depthAttachment;

    AttachmentRef colorAttachmentRef = {};
    colorAttachmentRef.Attachment = 0;

    SubpassInfo subpass = {};
    subpass.NumColorAttachments = 1;
    subpass.pColorAttachmentRefs = &colorAttachmentRef;

    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpass;

    DebugDrawPass.Initialize( renderPassCI );

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

    const char * vertesSourceCode = R"(

            out gl_PerVertex
            {
                vec4 gl_Position;
//                float gl_PointSize;
//                float gl_ClipDistance[];
            };

            layout( location = 0 ) out vec4 VS_Color;

            void main() {
                gl_Position = ModelviewProjection * vec4( InPosition, 1.0 );
                VS_Color = InColor;
                //gl_PointSize = 10;
            }

            )";

    const char * fragmentSourceCode = R"(

            layout( location = 0 ) in vec4 VS_Color;
            layout( location = 0 ) out vec4 FS_FragColor;

            void main() {
                FS_FragColor = VS_Color;
            }

            )";

    ShaderModule vertexShaderModule, fragmentShaderModule;

    GShaderSources.Clear();
    GShaderSources.Add( UniformStr );
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertesSourceCode );
    GShaderSources.Build( VERTEX_SHADER, &vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentSourceCode );
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

    pipelineCI.pRenderPass = &DebugDrawPass;
    pipelineCI.Subpass = 0;

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

void ADebugDrawPassRenderer::Deinitialize() {
    DebugDrawPass.Deinitialize();

    for ( int i = 0 ; i < DBG_DRAW_CMD_MAX ; i++ ) {
        Pipelines[i].Deinitialize();
    }
}

void ADebugDrawPassRenderer::RenderInstances( GHI::Framebuffer * _Framebuffer ) {
    RenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = &DebugDrawPass;
    renderPassBegin.pFramebuffer = _Framebuffer;
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width =  GRenderView->Width;
    renderPassBegin.RenderArea.Height = GRenderView->Height;
    renderPassBegin.pColorClearValues = NULL;
    renderPassBegin.pDepthStencilClearValue = NULL;

    Cmd.BeginRenderPass( renderPassBegin );

    Viewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = GRenderView->Width;
    vp.Height = GRenderView->Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    Cmd.SetViewport( vp );

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

        if ( RVRenderSnapshot ) {
            SaveSnapshot(GRenderTarget.GetFramebufferTexture());
        }
    }

    Cmd.EndRenderPass();
}

}
