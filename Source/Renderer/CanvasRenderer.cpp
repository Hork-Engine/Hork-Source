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

#include "CanvasRenderer.h"
#include "RenderLocal.h"

using namespace RenderCore;

ACanvasRenderer::ACanvasRenderer()
{
    GDevice->CreateResourceTable( &ResourceTable );
    ResourceTable->SetDebugName("Canvas");

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
            HK_OFS( SHUDDrawVert, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            HK_OFS( SHUDDrawVert, TexCoord )
        },
        {
            "InColor",
            2,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            HK_OFS( SHUDDrawVert, Color )
        }
    };

    AShaderFactory::CreateVertexShader( "canvas/presentview.vert", vertexAttribs, HK_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    AShaderFactory::CreateFragmentShader( "canvas/presentview.frag", pipelineCI.pFS );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE( vertexAttribs );
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
            HK_OFS( SHUDDrawVert, Position )
        },
        {
            "InTexCoord",
            1,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            HK_OFS( SHUDDrawVert, TexCoord )
        },
        {
            "InColor",
            2,              // location
            0,              // buffer input slot
            VAT_UBYTE4N,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            HK_OFS( SHUDDrawVert, Color )
        }
    };


    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    AShaderFactory::CreateVertexShader( "canvas/canvas.vert", vertexAttribs, HK_ARRAY_SIZE( vertexAttribs ), vertexShaderModule );
    AShaderFactory::CreateFragmentShader( "canvas/canvas.frag", fragmentShaderModule );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLES;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( SHUDDrawVert );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE( vertexAttribs );
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

                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        SCanvasBinding canvasBinding;

                        canvasBinding.Size            = sizeof(SCanvasConstants);
                        canvasBinding.Offset          = GStreamedMemory->AllocateConstant(canvasBinding.Size);
                        void*             pMemory     = GStreamedMemory->Map(canvasBinding.Offset);
                        SCanvasConstants* pCanvasCBuf = (SCanvasConstants*)pMemory;

                        pCanvasCBuf->OrthoProjection = GFrameData->CanvasOrthoProjection;

                        SRect2D scissorRect;

                        SDrawIndexedCmd drawCmd;
                        drawCmd.InstanceCount         = 1;
                        drawCmd.StartInstanceLocation = 0;
                        drawCmd.BaseVertexLocation    = 0;

                        immediateCtx->BindResourceTable(ResourceTable);

                        for (SHUDDrawList* drawList = GFrameData->DrawListHead; drawList; drawList = drawList->pNext)
                        {
                            // Process render commands
                            for (SHUDDrawCmd* cmd = drawList->Commands; cmd < &drawList->Commands[drawList->CommandsCount]; cmd++)
                            {
                                switch (cmd->Type)
                                {
                                    case HUD_DRAW_CMD_VIEWPORT: {
                                        // Draw just rendered scene on the canvas
                                        immediateCtx->BindPipeline(PresentViewPipeline[cmd->Blending]);
                                        immediateCtx->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                        immediateCtx->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

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
                                        immediateCtx->SetScissor(scissorRect);

                                        drawCmd.IndexCountPerInstance = cmd->IndexCount;
                                        drawCmd.StartIndexLocation    = cmd->StartIndexLocation;
                                        drawCmd.BaseVertexLocation    = cmd->BaseVertexLocation;
                                        immediateCtx->Draw(&drawCmd);
                                        break;
                                    }
                                    case HUD_DRAW_CMD_MATERIAL: {
                                        HK_ASSERT(cmd->MaterialFrameData);

                                        AMaterialGPU* pMaterial = cmd->MaterialFrameData->Material;

                                        HK_ASSERT(pMaterial->MaterialType == MATERIAL_TYPE_HUD);

                                        immediateCtx->BindPipeline(pMaterial->HUDPipeline);
                                        immediateCtx->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                        immediateCtx->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

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
                                        immediateCtx->SetScissor(scissorRect);

                                        drawCmd.IndexCountPerInstance = cmd->IndexCount;
                                        drawCmd.StartIndexLocation    = cmd->StartIndexLocation;
                                        drawCmd.BaseVertexLocation    = cmd->BaseVertexLocation;
                                        immediateCtx->Draw(&drawCmd);
                                        break;
                                    }

                                    default:
                                    {
                                        IPipeline* pPipeline = Pipelines[cmd->Blending][cmd->SamplerType];

                                        immediateCtx->BindPipeline(pPipeline);
                                        immediateCtx->BindVertexBuffer(0, GStreamBuffer, drawList->VertexStreamOffset);
                                        immediateCtx->BindIndexBuffer(GStreamBuffer, INDEX_TYPE_UINT16, drawList->IndexStreamOffset);

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
                                        immediateCtx->SetScissor(scissorRect);

                                        drawCmd.IndexCountPerInstance = cmd->IndexCount;
                                        drawCmd.StartIndexLocation    = cmd->StartIndexLocation;
                                        drawCmd.BaseVertexLocation    = cmd->BaseVertexLocation;
                                        immediateCtx->Draw(&drawCmd);
                                        break;
                                    }
                                }
                            }
                        }
                    });
}
