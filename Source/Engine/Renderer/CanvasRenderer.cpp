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

#include "CanvasRenderer.h"
#include "RenderLocal.h"

using namespace RenderCore;

ACanvasRenderer::ACanvasRenderer()
{
    SRenderPassCreateInfo renderPassCI = {};

    renderPassCI.NumColorAttachments = 1;

    SAttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_LOAD;
    renderPassCI.pColorAttachments = &colorAttachment;
    renderPassCI.pDepthStencilAttachment = NULL;

    renderPassCI.NumSubpasses = 0;
    renderPassCI.pSubpasses = NULL;

    GDevice->CreateRenderPass( renderPassCI, &CanvasPass );

    GDevice->CreateResourceTable( &ResourceTable );

    //
    // Create pipelines
    //

    CreatePresentViewPipeline();
    CreatePipelines();
}

void ACanvasRenderer::CreatePresentViewPipeline()
{
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_DISABLED;
    rsd.bScissorEnable = true;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, TexCoord )
        },
        {
            "InColor",
            2,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, Color )
        }
    };

    CreateVertexShader( "canvas/presentview.vert", vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateFragmentShader( "canvas/presentview.frag", pipelineCI.pFS );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SSamplerInfo samplerCI;
    samplerCI.Filter = FILTER_NEAREST;//FILTER_LINEAR;//FILTER_NEAREST; // linear is better for dynamic resolution
    samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
    samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;

    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.Samplers = &samplerCI;

    SBufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.ResourceLayout.NumBuffers = 1;
    pipelineCI.ResourceLayout.Buffers = &bufferInfo;

    for ( int i = 0 ; i < COLOR_BLENDING_MAX ; i++ ) {
        if ( i == COLOR_BLENDING_DISABLED ) {
            pipelineCI.BS.RenderTargetSlots[0].SetBlendingPreset( BLENDING_NO_BLEND );
        }
        else if ( i == COLOR_BLENDING_ALPHA ) {
            pipelineCI.BS.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );
        }
        else {
            pipelineCI.BS.RenderTargetSlots[0].SetBlendingPreset( (BLENDING_PRESET)(BLENDING_NO_BLEND + i) );
        }

        GDevice->CreatePipeline( pipelineCI, &PresentViewPipeline[i] );
    }
}

void ACanvasRenderer::CreatePipelines()
{
    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_DISABLED;
    rsd.bScissorEnable = true;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, TexCoord )
        },
        {
            "InColor",
            2,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            AN_OFS( SHUDDrawVert, Color )
        }
    };


    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    CreateVertexShader( "canvas/canvas.vert", vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), vertexShaderModule );
    CreateFragmentShader( "canvas/canvas.frag", fragmentShaderModule );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SSamplerInfo samplerCI;

    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.Samplers = &samplerCI;

    SBufferInfo bufferInfo;
    bufferInfo.BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.ResourceLayout.NumBuffers = 1;
    pipelineCI.ResourceLayout.Buffers = &bufferInfo;

    for ( int i = 0 ; i < COLOR_BLENDING_MAX ; i++ ) {
        if ( i == COLOR_BLENDING_DISABLED ) {
            pipelineCI.BS.RenderTargetSlots[0].SetBlendingPreset( BLENDING_NO_BLEND );
        }
        else if ( i == COLOR_BLENDING_ALPHA ) {
            pipelineCI.BS.RenderTargetSlots[0].SetBlendingPreset( BLENDING_ALPHA );
        }
        else {
            pipelineCI.BS.RenderTargetSlots[0].SetBlendingPreset( (BLENDING_PRESET)(BLENDING_NO_BLEND + i) );
        }

        for ( int j = 0 ; j < HUD_SAMPLER_MAX ; j++ ) {
            samplerCI.Filter = (j & 1) ? FILTER_NEAREST : FILTER_LINEAR;
            samplerCI.AddressU = samplerCI.AddressV = samplerCI.AddressW = (SAMPLER_ADDRESS_MODE)(j >> 1);

            GDevice->CreatePipeline( pipelineCI, &Pipelines[i][j] );
        }
    }
}

void ACanvasRenderer::BeginCanvasPass()
{
    SRenderPassBegin renderPassBegin = {};

    renderPassBegin.pRenderPass = CanvasPass;
    renderPassBegin.pFramebuffer = rcmd->GetDefaultFramebuffer();
    renderPassBegin.RenderArea.X = 0;
    renderPassBegin.RenderArea.Y = 0;
    renderPassBegin.RenderArea.Width = GFrameData->CanvasWidth;
    renderPassBegin.RenderArea.Height = GFrameData->CanvasHeight;
    renderPassBegin.pColorClearValues = NULL;
    renderPassBegin.pDepthStencilClearValue = NULL;

    rcmd->BeginRenderPass( renderPassBegin );

    SViewport vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = GFrameData->CanvasWidth;
    vp.Height = GFrameData->CanvasHeight;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    rcmd->SetViewport( vp );
}

void ACanvasRenderer::Render( std::function<void(SRenderView *, AFrameGraphTexture**)> RenderViewCB )
{
    if ( !GFrameData->DrawListHead ) {
        return;
    }

    SRect2D scissorRect;

    // Begin render pass
    BeginCanvasPass();

    SDrawIndexedCmd drawCmd;
    drawCmd.InstanceCount = 1;
    drawCmd.StartInstanceLocation = 0;
    drawCmd.BaseVertexLocation = 0;

    struct SCanvasConstants
    {
        Float4x4 OrthoProjection;
    };

    struct SCanvasBinding
    {
        size_t Offset;
        size_t Size;
    };

    SCanvasBinding canvasBinding;

    AStreamedMemoryGPU * streamedMemory = GRenderBackend.GetStreamedMemoryGPU();

    canvasBinding.Size = sizeof( SCanvasConstants );
    canvasBinding.Offset = streamedMemory->AllocateConstant( canvasBinding.Size );
    void * pMemory = streamedMemory->Map( canvasBinding.Offset );
    SCanvasConstants * pCanvasCBuf = (SCanvasConstants *)pMemory;

    pCanvasCBuf->OrthoProjection = GFrameData->CanvasOrthoProjection;

    rcmd->BindResourceTable( ResourceTable );

    for ( SHUDDrawList * drawList = GFrameData->DrawListHead ; drawList ; drawList = drawList->pNext ) {

        // Process render commands
        for ( SHUDDrawCmd * cmd = drawList->Commands ; cmd < &drawList->Commands[drawList->CommandsCount] ; cmd++ ) {

            switch ( cmd->Type )
            {
                case HUD_DRAW_CMD_VIEWPORT:
                {
                    // End canvas pass
                    rcmd->EndRenderPass();

                    // Render scene to viewport
                    AN_ASSERT( cmd->ViewportIndex >= 0 && cmd->ViewportIndex < GFrameData->NumViews );

                    SRenderView * renderView = &GFrameData->RenderViews[cmd->ViewportIndex];
                    AFrameGraphTexture * viewTexture;

                    RenderViewCB( renderView, &viewTexture );

                    // Restore canvas pass
                    BeginCanvasPass();

                    // Restore resource table
                    rcmd->BindResourceTable( ResourceTable );

                    // Draw just rendered scene on the canvas
                    rcmd->BindPipeline( PresentViewPipeline[cmd->Blending].GetObject() );
                    rcmd->BindVertexBuffer( 0, GStreamBuffer, drawList->VertexStreamOffset );
                    rcmd->BindIndexBuffer( GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset );

                    // Use constant buffer from rendered view
                    ResourceTable->BindBuffer( 0,
                                               GStreamBuffer,
                                               GViewConstantBufferBindingBindingOffset,
                                               GViewConstantBufferBindingBindingSize );

                    // Set texture
                    ResourceTable->BindTexture( 0, viewTexture->Actual() );

                    scissorRect.X = cmd->ClipMins.X;
                    scissorRect.Y = cmd->ClipMins.Y;
                    scissorRect.Width = cmd->ClipMaxs.X - cmd->ClipMins.X;
                    scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
                    rcmd->SetScissor( scissorRect );

                    drawCmd.IndexCountPerInstance = cmd->IndexCount;
                    drawCmd.StartIndexLocation = cmd->StartIndexLocation;
                    drawCmd.BaseVertexLocation = cmd->BaseVertexLocation;
                    rcmd->Draw( &drawCmd );
                    break;
                }

                case HUD_DRAW_CMD_MATERIAL:
                {
                    AN_ASSERT( cmd->MaterialFrameData );

                    AMaterialGPU * pMaterial = cmd->MaterialFrameData->Material;

                    AN_ASSERT( pMaterial->MaterialType == MATERIAL_TYPE_HUD );

                    rcmd->BindPipeline( pMaterial->HUDPipeline );
                    rcmd->BindVertexBuffer( 0, GStreamBuffer, drawList->VertexStreamOffset );
                    rcmd->BindIndexBuffer( GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset );

                    // Set constant buffer
                    ResourceTable->BindBuffer( 0,
                                               GStreamBuffer,
                                               canvasBinding.Offset,
                                               canvasBinding.Size );

                    BindTextures( ResourceTable, cmd->MaterialFrameData, cmd->MaterialFrameData->NumTextures );

                    // FIXME: What about material constants?

                    scissorRect.X = cmd->ClipMins.X;
                    scissorRect.Y = cmd->ClipMins.Y;
                    scissorRect.Width = cmd->ClipMaxs.X - cmd->ClipMins.X;
                    scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
                    rcmd->SetScissor( scissorRect );

                    drawCmd.IndexCountPerInstance = cmd->IndexCount;
                    drawCmd.StartIndexLocation = cmd->StartIndexLocation;
                    drawCmd.BaseVertexLocation = cmd->BaseVertexLocation;
                    rcmd->Draw( &drawCmd );
                    break;
                }

                default:
                {
                    IPipeline * pPipeline = Pipelines[cmd->Blending][cmd->SamplerType].GetObject();

                    rcmd->BindPipeline( pPipeline );
                    rcmd->BindVertexBuffer( 0, GStreamBuffer, drawList->VertexStreamOffset );
                    rcmd->BindIndexBuffer( GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset );

                    // Set constant buffer
                    ResourceTable->BindBuffer( 0,
                                               GStreamBuffer,
                                               canvasBinding.Offset,
                                               canvasBinding.Size );

                    // Set texture
                    ResourceTable->BindTexture( 0, cmd->Texture );

                    scissorRect.X = cmd->ClipMins.X;
                    scissorRect.Y = cmd->ClipMins.Y;
                    scissorRect.Width = cmd->ClipMaxs.X - cmd->ClipMins.X;
                    scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
                    rcmd->SetScissor( scissorRect );

                    drawCmd.IndexCountPerInstance = cmd->IndexCount;
                    drawCmd.StartIndexLocation = cmd->StartIndexLocation;
                    drawCmd.BaseVertexLocation = cmd->BaseVertexLocation;
                    rcmd->Draw( &drawCmd );
                    break;
                }
            }
        }
    }

    rcmd->EndRenderPass();
}
