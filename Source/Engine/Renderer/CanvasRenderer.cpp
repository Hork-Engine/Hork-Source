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

#include "CanvasRenderer.h"
#include "RenderLocal.h"

#include <Runtime/Public/Runtime.h>

using namespace RenderCore;

ACanvasRenderer::ACanvasRenderer()
{
    GDevice->CreateResourceTable( &ResourceTable );

    //
    // Create pipelines
    //

    CreatePresentViewPipeline();
    CreatePipelines();
}

void ACanvasRenderer::CreatePresentViewPipeline()
{
    SPipelineDesc pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_DISABLED;
    rsd.bScissorEnable = true;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

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

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SSamplerDesc samplerCI;
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
    SPipelineDesc pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_DISABLED;
    rsd.bScissorEnable = true;

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.bDepthWrite = false;

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

    SSamplerDesc samplerCI;

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

void ACanvasRenderer::Render(AFrameGraph& FrameGraph, TPodVector<FGTextureProxy*>& pRenderViewTexture, ITexture* pBackBuffer)
{
    if ( !GFrameData->DrawListHead ) {
        return;
    }

    ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Draw HUD");

    for ( int i = 0 ; i < GFrameData->NumViews ; i++ )
    {
        pass.AddResource(pRenderViewTexture[i], FG_RESOURCE_ACCESS_READ);
    }

    FGTextureProxy* SwapChainColorBuffer = FrameGraph.AddExternalResource<FGTextureProxy>("SwapChainColorAttachment", pBackBuffer);

    pass.SetColorAttachment(STextureAttachment(SwapChainColorBuffer)
                                .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));
    pass.SetRenderArea(GFrameData->CanvasWidth, GFrameData->CanvasHeight);
    pass.AddSubpass({0},
                    [this, pRenderViewTexture](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                    {
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

                        AStreamedMemoryGPU* streamedMemory = GRuntime->GetStreamedMemoryGPU();

                        canvasBinding.Size            = sizeof(SCanvasConstants);
                        canvasBinding.Offset          = streamedMemory->AllocateConstant(canvasBinding.Size);
                        void*             pMemory     = streamedMemory->Map(canvasBinding.Offset);
                        SCanvasConstants* pCanvasCBuf = (SCanvasConstants*)pMemory;

                        pCanvasCBuf->OrthoProjection = GFrameData->CanvasOrthoProjection;

                        SRect2D scissorRect;

                        SDrawIndexedCmd drawCmd;
                        drawCmd.InstanceCount         = 1;
                        drawCmd.StartInstanceLocation = 0;
                        drawCmd.BaseVertexLocation    = 0;

                        rcmd->BindResourceTable(ResourceTable);

                        for (SHUDDrawList* drawList = GFrameData->DrawListHead; drawList; drawList = drawList->pNext)
                        {
                            // Process render commands
                            for (SHUDDrawCmd* cmd = drawList->Commands; cmd < &drawList->Commands[drawList->CommandsCount]; cmd++)
                            {
                                switch (cmd->Type)
                                {
                                    case HUD_DRAW_CMD_VIEWPORT: {
                                        // Draw just rendered scene on the canvas
                                        rcmd->BindPipeline(PresentViewPipeline[cmd->Blending]);
                                        rcmd->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                        rcmd->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

                                        // Use constant buffer from rendered view
                                        ResourceTable->BindBuffer(0,
                                                                  GStreamBuffer,
                                                                  GRenderViewContext[cmd->ViewportIndex].ViewConstantBufferBindingBindingOffset,
                                                                  GRenderViewContext[cmd->ViewportIndex].ViewConstantBufferBindingBindingSize);

                                        // Set texture
                                        ResourceTable->BindTexture(0, pRenderViewTexture[cmd->ViewportIndex]->Actual());

                                        scissorRect.X      = cmd->ClipMins.X;
                                        scissorRect.Y      = cmd->ClipMins.Y;
                                        scissorRect.Width  = cmd->ClipMaxs.X - cmd->ClipMins.X;
                                        scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
                                        rcmd->SetScissor(scissorRect);

                                        drawCmd.IndexCountPerInstance = cmd->IndexCount;
                                        drawCmd.StartIndexLocation    = cmd->StartIndexLocation;
                                        drawCmd.BaseVertexLocation    = cmd->BaseVertexLocation;
                                        rcmd->Draw(&drawCmd);
                                        break;
                                    }
                                    case HUD_DRAW_CMD_MATERIAL: {
                                        AN_ASSERT(cmd->MaterialFrameData);

                                        AMaterialGPU* pMaterial = cmd->MaterialFrameData->Material;

                                        AN_ASSERT(pMaterial->MaterialType == MATERIAL_TYPE_HUD);

                                        rcmd->BindPipeline(pMaterial->HUDPipeline);
                                        rcmd->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                        rcmd->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

                                        // Set constant buffer
                                        ResourceTable->BindBuffer(0,
                                                                  GStreamBuffer,
                                                                  canvasBinding.Offset,
                                                                  canvasBinding.Size);

                                        BindTextures(ResourceTable, cmd->MaterialFrameData, cmd->MaterialFrameData->NumTextures);

                                        // FIXME: What about material constants?

                                        scissorRect.X      = cmd->ClipMins.X;
                                        scissorRect.Y      = cmd->ClipMins.Y;
                                        scissorRect.Width  = cmd->ClipMaxs.X - cmd->ClipMins.X;
                                        scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
                                        rcmd->SetScissor(scissorRect);

                                        drawCmd.IndexCountPerInstance = cmd->IndexCount;
                                        drawCmd.StartIndexLocation    = cmd->StartIndexLocation;
                                        drawCmd.BaseVertexLocation    = cmd->BaseVertexLocation;
                                        rcmd->Draw(&drawCmd);
                                        break;
                                    }

                                    default:
                                    {
                                        IPipeline* pPipeline = Pipelines[cmd->Blending][cmd->SamplerType];

                                        rcmd->BindPipeline(pPipeline);
                                        rcmd->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                        rcmd->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

                                        // Set constant buffer
                                        ResourceTable->BindBuffer(0,
                                                                  GStreamBuffer,
                                                                  canvasBinding.Offset,
                                                                  canvasBinding.Size);

                                        // Set texture
                                        ResourceTable->BindTexture(0, cmd->Texture);

                                        scissorRect.X      = cmd->ClipMins.X;
                                        scissorRect.Y      = cmd->ClipMins.Y;
                                        scissorRect.Width  = cmd->ClipMaxs.X - cmd->ClipMins.X;
                                        scissorRect.Height = cmd->ClipMaxs.Y - cmd->ClipMins.Y;
                                        rcmd->SetScissor(scissorRect);

                                        drawCmd.IndexCountPerInstance = cmd->IndexCount;
                                        drawCmd.StartIndexLocation    = cmd->StartIndexLocation;
                                        drawCmd.BaseVertexLocation    = cmd->BaseVertexLocation;
                                        rcmd->Draw(&drawCmd);
                                        break;
                                    }
                                }
                            }
                        }
                    });

    #if 0

    std::function<void(SHUDDrawList*, SHUDDrawCmd*, FGTextureProxy*)> ProcessDrawLists = [&](SHUDDrawList* drawList, SHUDDrawCmd* cmd, FGTextureProxy* pViewTexture)
    {
        AN_ASSERT(drawList);

        FGTextureProxy* SwapChainColorBuffer = FrameGraph.AddTextureView("SwapChainColorAttachment", pBackBufferRTV);

        ARenderPass& pass = FrameGraph.AddTask<ARenderPass>("Draw HUD");

        if (pViewTexture)
            pass.AddResource(pViewTexture, FG_RESOURCE_ACCESS_READ);

        pass.SetColorAttachment(STextureAttachment(SwapChainColorBuffer)
                                 .SetLoadOp(ATTACHMENT_LOAD_OP_LOAD));
        pass.SetRenderArea(GFrameData->CanvasWidth, GFrameData->CanvasHeight);
        pass.AddSubpass( {0},
                        [this, cmd, drawList, pViewTexture, canvasBinding](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
        {
                            SRect2D scissorRect;

                            SDrawIndexedCmd drawCmd;
                            drawCmd.InstanceCount         = 1;
                            drawCmd.StartInstanceLocation = 0;
                            drawCmd.BaseVertexLocation    = 0;

                            rcmd->BindResourceTable(ResourceTable);

                            SHUDDrawCmd* curCmd = cmd;

                            if (pViewTexture)
                            {
                                // Draw just rendered scene on the canvas
                                rcmd->BindPipeline(PresentViewPipeline[curCmd->Blending]);
                                rcmd->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                rcmd->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

                                // Use constant buffer from rendered view
                                ResourceTable->BindBuffer(0,
                                                          GStreamBuffer,
                                                          GViewConstantBufferBindingBindingOffset,
                                                          GViewConstantBufferBindingBindingSize);

                                // Set texture
                                ResourceTable->BindTexture(0, pViewTexture->Actual());

                                scissorRect.X      = curCmd->ClipMins.X;
                                scissorRect.Y      = curCmd->ClipMins.Y;
                                scissorRect.Width  = curCmd->ClipMaxs.X - curCmd->ClipMins.X;
                                scissorRect.Height = curCmd->ClipMaxs.Y - curCmd->ClipMins.Y;
                                rcmd->SetScissor(scissorRect);

                                drawCmd.IndexCountPerInstance = curCmd->IndexCount;
                                drawCmd.StartIndexLocation    = curCmd->StartIndexLocation;
                                drawCmd.BaseVertexLocation    = curCmd->BaseVertexLocation;
                                rcmd->Draw(&drawCmd);

                                // Switch to next cmd
                                curCmd++;
                            }

                            SHUDDrawList* curDrawList = drawList;

                            for (; curDrawList; curDrawList = curDrawList->pNext)
                            {
                                // Process render commands
                                for (; curCmd < &curDrawList->Commands[curDrawList->CommandsCount] && curCmd->Type != HUD_DRAW_CMD_VIEWPORT; curCmd++)
                                {
                                    switch (curCmd->Type)
                                    {
                                        case HUD_DRAW_CMD_MATERIAL: {
                                            AN_ASSERT(curCmd->MaterialFrameData);

                                            AMaterialGPU* pMaterial = curCmd->MaterialFrameData->Material;

                                            AN_ASSERT(pMaterial->MaterialType == MATERIAL_TYPE_HUD);

                                            rcmd->BindPipeline(pMaterial->HUDPipeline);
                                            rcmd->BindVertexBuffer(0, GStreamBuffer, curDrawList->VertexStreamOffset);
                                            rcmd->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, curDrawList->IndexStreamOffset);

                                            // Set constant buffer
                                            ResourceTable->BindBuffer(0,
                                                                      GStreamBuffer,
                                                                      canvasBinding.Offset,
                                                                      canvasBinding.Size);

                                            BindTextures(ResourceTable, curCmd->MaterialFrameData, curCmd->MaterialFrameData->NumTextures);

                                            // FIXME: What about material constants?

                                            scissorRect.X      = curCmd->ClipMins.X;
                                            scissorRect.Y      = curCmd->ClipMins.Y;
                                            scissorRect.Width  = curCmd->ClipMaxs.X - curCmd->ClipMins.X;
                                            scissorRect.Height = curCmd->ClipMaxs.Y - curCmd->ClipMins.Y;
                                            rcmd->SetScissor(scissorRect);

                                            drawCmd.IndexCountPerInstance = curCmd->IndexCount;
                                            drawCmd.StartIndexLocation    = curCmd->StartIndexLocation;
                                            drawCmd.BaseVertexLocation    = curCmd->BaseVertexLocation;
                                            rcmd->Draw(&drawCmd);
                                            break;
                                        }

                                        default:
                                        {
                                            IPipeline* pPipeline = Pipelines[curCmd->Blending][curCmd->SamplerType];

                                            rcmd->BindPipeline(pPipeline);
                                            rcmd->BindVertexBuffer(0, GStreamBuffer, curDrawList->VertexStreamOffset);
                                            rcmd->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, curDrawList->IndexStreamOffset);

                                            // Set constant buffer
                                            ResourceTable->BindBuffer(0,
                                                                      GStreamBuffer,
                                                                      canvasBinding.Offset,
                                                                      canvasBinding.Size);

                                            // Set texture
                                            ResourceTable->BindTexture(0, curCmd->Texture);

                                            scissorRect.X      = curCmd->ClipMins.X;
                                            scissorRect.Y      = curCmd->ClipMins.Y;
                                            scissorRect.Width  = curCmd->ClipMaxs.X - curCmd->ClipMins.X;
                                            scissorRect.Height = curCmd->ClipMaxs.Y - curCmd->ClipMins.Y;
                                            rcmd->SetScissor(scissorRect);

                                            drawCmd.IndexCountPerInstance = curCmd->IndexCount;
                                            drawCmd.StartIndexLocation    = curCmd->StartIndexLocation;
                                            drawCmd.BaseVertexLocation    = curCmd->BaseVertexLocation;
                                            rcmd->Draw(&drawCmd);
                                            break;
                                        }
                                    }
                                }
                                if (curCmd < &curDrawList->Commands[curDrawList->CommandsCount] && curCmd->Type == HUD_DRAW_CMD_VIEWPORT)
                                {
                                    break;
                                }
                            }
        }
        );

        if (drawList && cmd < &drawList->Commands[drawList->CommandsCount] && cmd->Type == HUD_DRAW_CMD_VIEWPORT)
        {
            // Render scene to viewport
            AN_ASSERT(cmd->ViewportIndex >= 0 && cmd->ViewportIndex < GFrameData->NumViews);

            SRenderView* pRenderView = &GFrameData->RenderViews[cmd->ViewportIndex];

            FGTextureProxy* pTexture = nullptr;
            RenderViewCB(pRenderView, &pTexture);

            ProcessDrawLists(drawList, cmd, pTexture);
        }    
    };

    if (GFrameData->DrawListHead)
    {
        ProcessDrawLists(GFrameData->DrawListHead, GFrameData->DrawListHead->Commands, nullptr);
    }
    #endif
}
