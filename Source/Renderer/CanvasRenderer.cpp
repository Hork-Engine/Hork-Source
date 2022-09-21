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

static void SetBlendingFromCompositeState(SBlendingStateInfo& blend, CANVAS_COMPOSITE composite)
{
    BLEND_FUNC sfactor, dfactor;

    SRenderTargetBlendingInfo& rtblend = blend.RenderTargetSlots[0];

    rtblend.bBlendEnable = true;

    switch (composite)
    {
        case CANVAS_COMPOSITE_SOURCE_OVER:
            sfactor = BLEND_FUNC_ONE;
            dfactor = BLEND_FUNC_INV_SRC_ALPHA;
            break;
        case CANVAS_COMPOSITE_SOURCE_IN:
            sfactor = BLEND_FUNC_DST_ALPHA;
            dfactor = BLEND_FUNC_ZERO;
            break;
        case CANVAS_COMPOSITE_SOURCE_OUT:
            sfactor = BLEND_FUNC_INV_DST_ALPHA;
            dfactor = BLEND_FUNC_ZERO;
            break;
        case CANVAS_COMPOSITE_ATOP:
            sfactor = BLEND_FUNC_DST_ALPHA;
            dfactor = BLEND_FUNC_INV_SRC_ALPHA;
            break;
        case CANVAS_COMPOSITE_DESTINATION_OVER:
            sfactor = BLEND_FUNC_INV_DST_ALPHA;
            dfactor = BLEND_FUNC_ONE;
            break;
        case CANVAS_COMPOSITE_DESTINATION_IN:
            sfactor = BLEND_FUNC_ZERO;
            dfactor = BLEND_FUNC_SRC_ALPHA;
            break;
        case CANVAS_COMPOSITE_DESTINATION_OUT:
            sfactor = BLEND_FUNC_ZERO;
            dfactor = BLEND_FUNC_INV_SRC_ALPHA;
            break;
        case CANVAS_COMPOSITE_DESTINATION_ATOP:
            sfactor = BLEND_FUNC_INV_DST_ALPHA;
            dfactor = BLEND_FUNC_SRC_ALPHA;
            break;
        case CANVAS_COMPOSITE_LIGHTER:
            sfactor = BLEND_FUNC_ONE;
            dfactor = BLEND_FUNC_ONE;
            break;
        case CANVAS_COMPOSITE_COPY:
            rtblend.bBlendEnable = false;
            sfactor = BLEND_FUNC_ONE;
            dfactor = BLEND_FUNC_ZERO;
            break;
        case CANVAS_COMPOSITE_XOR:
            sfactor = BLEND_FUNC_INV_DST_ALPHA;
            dfactor = BLEND_FUNC_INV_SRC_ALPHA;
            break;
        default:
            rtblend.bBlendEnable = false;
            sfactor = BLEND_FUNC_ONE;
            dfactor = BLEND_FUNC_ZERO;
            HK_ASSERT(0);
            break;
    }

    rtblend.Func.SrcFactorRGB = rtblend.Func.SrcFactorAlpha = sfactor;
    rtblend.Func.DstFactorRGB = rtblend.Func.DstFactorAlpha = dfactor;
}

ACanvasRenderer::ACanvasRenderer()
{
    m_bEdgeAntialias = true; // TODO: set from config?

    static const SVertexAttribInfo vertexAttribs[] = {
        {"InPosition",
         0, // location
         0, // buffer input slot
         VAT_FLOAT2,
         VAM_FLOAT,
         0, // InstanceDataStepRate
         HK_OFS(CanvasVertex, x)},
        {"InTexCoord",
         1, // location
         0, // buffer input slot
         VAT_FLOAT2,
         VAM_FLOAT,
         0, // InstanceDataStepRate
         HK_OFS(CanvasVertex, u)}};

    AShaderFactory::CreateVertexShader("canvas/canvas.vert", vertexAttribs, HK_ARRAY_SIZE(vertexAttribs), m_VertexShader);
    if (m_bEdgeAntialias)
    {
        AShaderFactory::CreateFragmentShader("canvas/canvas_aa.frag", m_FragmentShader);
    }
    else
    {
        AShaderFactory::CreateFragmentShader("canvas/canvas.frag", m_FragmentShader);
    }

    PRIMITIVE_TOPOLOGY primitiveTopology[TOPOLOGY_MAX] = {PRIMITIVE_TRIANGLES, PRIMITIVE_TRIANGLE_STRIP};

    SRasterizerStateInfo rasterState[RASTER_STATE_MAX];
    {
        SRasterizerStateInfo& rasterStateCull = rasterState[RASTER_STATE_CULL];

        rasterStateCull.CullMode        = POLYGON_CULL_BACK;
        rasterStateCull.bFrontClockwise = true;
    }

    SBlendingStateInfo blendState[CANVAS_COMPOSITE_LAST + 1];
    for (int i = 0; i <= CANVAS_COMPOSITE_LAST ; i++)
    {
        SBlendingStateInfo& blendAlpha = blendState[i];

        SetBlendingFromCompositeState(blendAlpha, CANVAS_COMPOSITE(i));
    }

    SDepthStencilStateInfo depthStencil[DEPTH_STENCIL_MAX];
    {
        SDepthStencilStateInfo& drawShapes = depthStencil[DEPTH_STENCIL_DRAW_AA];

        drawShapes.bDepthEnable   = false;
        drawShapes.bDepthWrite    = false;
        drawShapes.bStencilEnable = true;

        drawShapes.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
        drawShapes.FrontFace.DepthFailOp   = STENCIL_OP_KEEP;
        drawShapes.FrontFace.DepthPassOp   = STENCIL_OP_KEEP;
        drawShapes.FrontFace.StencilFunc   = CMPFUNC_EQUAL;

        drawShapes.BackFace.StencilFailOp = STENCIL_OP_KEEP;
        drawShapes.BackFace.DepthFailOp   = STENCIL_OP_KEEP;
        drawShapes.BackFace.DepthPassOp   = STENCIL_OP_KEEP;
        drawShapes.BackFace.StencilFunc   = CMPFUNC_EQUAL;
    }
    {
        SDepthStencilStateInfo& drawShapes = depthStencil[DEPTH_STENCIL_FILL];

        drawShapes.bDepthEnable   = false;
        drawShapes.bDepthWrite    = false;
        drawShapes.bStencilEnable = true;

        drawShapes.FrontFace.StencilFailOp = STENCIL_OP_ZERO;
        drawShapes.FrontFace.DepthFailOp   = STENCIL_OP_ZERO;
        drawShapes.FrontFace.DepthPassOp   = STENCIL_OP_ZERO;
        drawShapes.FrontFace.StencilFunc   = CMPFUNC_NOT_EQUAL;

        drawShapes.BackFace.StencilFailOp = STENCIL_OP_ZERO;
        drawShapes.BackFace.DepthFailOp   = STENCIL_OP_ZERO;
        drawShapes.BackFace.DepthPassOp   = STENCIL_OP_ZERO;
        drawShapes.BackFace.StencilFunc   = CMPFUNC_NOT_EQUAL;
    }
    {
        SDepthStencilStateInfo& drawShapes = depthStencil[DEPTH_STENCIL_DEFAULT];

        drawShapes.bDepthEnable   = false;
        drawShapes.bDepthWrite    = false;
        drawShapes.bStencilEnable = false;

        drawShapes.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
        drawShapes.FrontFace.DepthFailOp   = STENCIL_OP_KEEP;
        drawShapes.FrontFace.DepthPassOp   = STENCIL_OP_KEEP;
        drawShapes.FrontFace.StencilFunc   = CMPFUNC_ALWAYS;

        drawShapes.BackFace.StencilFailOp = STENCIL_OP_KEEP;
        drawShapes.BackFace.DepthFailOp   = STENCIL_OP_KEEP;
        drawShapes.BackFace.DepthPassOp   = STENCIL_OP_KEEP;
        drawShapes.BackFace.StencilFunc   = CMPFUNC_ALWAYS;
    }
    {
        SDepthStencilStateInfo& drawShapes = depthStencil[DEPTH_STENCIL_STROKE_FILL];

        drawShapes.bDepthEnable = false;
        drawShapes.bDepthWrite = false;
        drawShapes.bStencilEnable = true;

        drawShapes.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
        drawShapes.FrontFace.DepthFailOp = STENCIL_OP_KEEP;
        drawShapes.FrontFace.DepthPassOp = STENCIL_OP_INCR;
        drawShapes.FrontFace.StencilFunc = CMPFUNC_EQUAL;

        drawShapes.BackFace.StencilFailOp = STENCIL_OP_KEEP;
        drawShapes.BackFace.DepthFailOp = STENCIL_OP_KEEP;
        drawShapes.BackFace.DepthPassOp = STENCIL_OP_INCR;
        drawShapes.BackFace.StencilFunc = CMPFUNC_EQUAL;
    }

    SSamplerDesc sampler[SAMPLER_STATE_MAX];
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_CLAMP_CLAMP_LINEAR];        

        smp.Filter         = FILTER_LINEAR;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_CLAMP;
        smp.AddressV       = SAMPLER_ADDRESS_CLAMP;

    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_WRAP_CLAMP_LINEAR];

        smp.Filter         = FILTER_LINEAR;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_WRAP;
        smp.AddressV       = SAMPLER_ADDRESS_CLAMP;
    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_CLAMP_WRAP_LINEAR];

        smp.Filter         = FILTER_LINEAR;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_CLAMP;
        smp.AddressV       = SAMPLER_ADDRESS_WRAP;
    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_WRAP_WRAP_LINEAR];

        smp.Filter         = FILTER_LINEAR;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_WRAP;
        smp.AddressV       = SAMPLER_ADDRESS_WRAP;
    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_CLAMP_CLAMP_NEAREST];

        smp.Filter         = FILTER_NEAREST;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_CLAMP;
        smp.AddressV       = SAMPLER_ADDRESS_CLAMP;
    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_WRAP_CLAMP_NEAREST];

        smp.Filter         = FILTER_NEAREST;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_WRAP;
        smp.AddressV       = SAMPLER_ADDRESS_CLAMP;
    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_CLAMP_WRAP_NEAREST];

        smp.Filter         = FILTER_NEAREST;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_CLAMP;
        smp.AddressV       = SAMPLER_ADDRESS_WRAP;
    }
    {
        SSamplerDesc& smp = sampler[SAMPLER_STATE_WRAP_WRAP_NEAREST];

        smp.Filter         = FILTER_NEAREST;
        smp.ComparisonFunc = CMPFUNC_NEVER;
        smp.AddressU       = SAMPLER_ADDRESS_WRAP;
        smp.AddressV       = SAMPLER_ADDRESS_WRAP;
    }

    SPipelineDesc pipelineCI;

    pipelineCI.pVS = m_VertexShader;
    pipelineCI.pFS = m_FragmentShader;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride    = sizeof(CanvasVertex);
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE(vertexBinding);
    pipelineCI.pVertexBindings   = vertexBinding;

    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(vertexAttribs);
    pipelineCI.pVertexAttribs   = vertexAttribs;

    pipelineCI.ResourceLayout.NumSamplers = 1;

    SBufferInfo bufferInfo[2];
    bufferInfo[0].BufferBinding = BUFFER_BIND_CONSTANT;
    bufferInfo[1].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(bufferInfo);
    pipelineCI.ResourceLayout.Buffers    = bufferInfo;

    for (int top = 0; top < TOPOLOGY_MAX; ++top)
    {
        pipelineCI.IA.Topology = primitiveTopology[top];
        for (int rs = 0; rs < RASTER_STATE_MAX; ++rs)
        {
            pipelineCI.RS = rasterState[rs];
            for (int bs = 0; bs <= CANVAS_COMPOSITE_LAST; ++bs)
            {
                pipelineCI.BS = blendState[bs];
                for (int ds = 0; ds < DEPTH_STENCIL_MAX; ++ds)
                {
                    pipelineCI.DSS = depthStencil[ds];
                    for (int smp = 0; smp < SAMPLER_STATE_MAX; ++smp)
                    {
                        pipelineCI.ResourceLayout.Samplers = &sampler[smp];

                        GDevice->CreatePipeline(pipelineCI, &m_PipelinePermut[top][rs][bs][ds][smp]);
                    }
                }
            }
        }
    }

    {
        pipelineCI.IA.Topology = PRIMITIVE_TRIANGLES;

        SRasterizerStateInfo rasterStateNoCull;
        rasterStateNoCull.CullMode        = POLYGON_CULL_DISABLED;
        rasterStateNoCull.bFrontClockwise = true;

        pipelineCI.RS = rasterStateNoCull;
            
        SBlendingStateInfo noWrite;
        noWrite.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

        pipelineCI.BS = noWrite;
        
        SDepthStencilStateInfo ds;

        ds.bDepthEnable   = false;
        ds.bDepthWrite    = false;
        ds.bStencilEnable = true;

        ds.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
        ds.FrontFace.DepthFailOp   = STENCIL_OP_KEEP;
        ds.FrontFace.DepthPassOp   = STENCIL_OP_INCR;
        ds.FrontFace.StencilFunc   = CMPFUNC_ALWAYS;

        ds.BackFace.StencilFailOp = STENCIL_OP_KEEP;
        ds.BackFace.DepthFailOp   = STENCIL_OP_KEEP;
        ds.BackFace.DepthPassOp   = STENCIL_OP_DECR;
        ds.BackFace.StencilFunc   = CMPFUNC_ALWAYS;

        pipelineCI.DSS = ds;

        pipelineCI.ResourceLayout.NumSamplers = 0;

        GDevice->CreatePipeline(pipelineCI, &m_PipelineShapes);
    }

    {
        pipelineCI.IA.Topology = PRIMITIVE_TRIANGLE_STRIP;

        SRasterizerStateInfo rs;
        rs.CullMode = POLYGON_CULL_BACK;
        rs.bFrontClockwise = true;

        pipelineCI.RS = rs;

        SBlendingStateInfo noWrite;
        noWrite.RenderTargetSlots[0].ColorWriteMask = COLOR_WRITE_DISABLED;

        pipelineCI.BS = noWrite;

        SDepthStencilStateInfo ds;

        ds.bDepthEnable = false;
        ds.bDepthWrite = false;
        ds.bStencilEnable = true;

        ds.FrontFace.StencilFailOp = STENCIL_OP_ZERO;
        ds.FrontFace.DepthFailOp = STENCIL_OP_ZERO;
        ds.FrontFace.DepthPassOp = STENCIL_OP_ZERO;
        ds.FrontFace.StencilFunc = CMPFUNC_ALWAYS;

        ds.BackFace.StencilFailOp = STENCIL_OP_ZERO;
        ds.BackFace.DepthFailOp = STENCIL_OP_ZERO;
        ds.BackFace.DepthPassOp = STENCIL_OP_ZERO;
        ds.BackFace.StencilFunc = CMPFUNC_ALWAYS;

        pipelineCI.DSS = ds;

        pipelineCI.ResourceLayout.NumSamplers = 0;

        GDevice->CreatePipeline(pipelineCI, &m_PipelineClearStencil);
    }

    BuildFanIndices(3, false);
}

ACanvasRenderer::~ACanvasRenderer()
{
}

void ACanvasRenderer::Render(AFrameGraph& FrameGraph, TSmallVector<FGTextureProxy*, 32>& pRenderViewTexture, ITexture* pBackBuffer)
{
    if (GFrameData->CanvasDrawData->NumDrawCommands == 0)
    {
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
                    [this, &pRenderViewTexture](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                    {
                        struct SCanvasConstants
                        {
                            Float4x4 OrthoProjection;
                            Float4   ViewSize;
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
                        pCanvasCBuf->ViewSize.X      = GFrameData->CanvasWidth;
                        pCanvasCBuf->ViewSize.Y      = GFrameData->CanvasHeight;

                        immediateCtx->BindResourceTable(rtbl);
                        rtbl->BindBuffer(0, GStreamBuffer, canvasBinding.Offset, canvasBinding.Size);
                        RenderVG(immediateCtx, GFrameData->CanvasDrawData, pRenderViewTexture);
                    });
}

void ACanvasRenderer::RenderVG(IImmediateContext* immediateCtx, CanvasDrawData const* pDrawData, TSmallVector<FGTextureProxy*, 32>& pRenderViewTexture)
{
    m_pDrawData = pDrawData;
    m_ImmediateCtx = immediateCtx;

    //m_ImmediateCtx->DynamicState_StencilRef(0);

    if (pDrawData->NumDrawCommands > 0)
    {
        SetBuffers();

        // Draw shapes

        for (CanvasDrawCmd const* drawCommand = pDrawData->DrawCommands; drawCommand < &pDrawData->DrawCommands[pDrawData->NumDrawCommands]; ++drawCommand)
        {
            ITexture* pTexture = nullptr;

            if (drawCommand->pTexture)
            {
                m_SamplerState = (drawCommand->TextureFlags & CANVAS_IMAGE_REPEATX ? 1 : 0) + (drawCommand->TextureFlags & CANVAS_IMAGE_REPEATY ? 2 : 0);

                if (drawCommand->TextureFlags & CANVAS_IMAGE_NEAREST)
                    m_SamplerState += 4;

                if (drawCommand->pTexture && (drawCommand->TextureFlags & _CANVAS_IMAGE_VIEWPORT_INDEX))
                {
                    size_t index = (size_t)drawCommand->pTexture - 1;
                    pTexture     = pRenderViewTexture[index]->Actual();
                }
                else
                {
                    pTexture = drawCommand->pTexture;
                }
            }

            m_BlendState = drawCommand->Composite;
            m_DepthStencil = DEPTH_STENCIL_DEFAULT;
            m_RasterState  = RASTER_STATE_CULL;

            switch (drawCommand->Type)
            {
                case CANVAS_DRAW_COMMAND_FILL:
                    DrawFill(drawCommand);
                    break;
                case CANVAS_DRAW_COMMAND_CONVEXFILL:
                    DrawConvexFill(drawCommand, pTexture);
                    break;
                case CANVAS_DRAW_COMMAND_STROKE:
                    DrawStroke(drawCommand);
                    break;
                case CANVAS_DRAW_COMMAND_STENCIL_STROKE:
                    DrawStroke(drawCommand, true);
                    break;
                case CANVAS_DRAW_COMMAND_TRIANGLES:
                    DrawTriangles(drawCommand);
                    break;
            }
        }
    }
}

void ACanvasRenderer::DrawFill(CanvasDrawCmd const* drawCommand)
{
    CanvasPath const* paths = &m_pDrawData->Paths[drawCommand->FirstPath];
    int               i, npaths = drawCommand->PathCount;

    // Draw shapes

    SetUniforms(drawCommand->UniformOffset, nullptr);

    m_ImmediateCtx->BindPipeline(m_PipelineShapes);

    for (i = 0; i < npaths; i++)
    {
        unsigned int numIndices = ((paths[i].FillCount - 2) * 3);

        BuildFanIndices(numIndices);

        SDrawIndexedCmd cmd;

        cmd.IndexCountPerInstance = numIndices;
        cmd.InstanceCount = 1;
        cmd.StartIndexLocation = 0;
        cmd.BaseVertexLocation = paths[i].FillOffset;
        cmd.StartInstanceLocation = 0;

        m_ImmediateCtx->Draw(&cmd);
    }

    // Draw anti-aliased pixels
    m_RasterState = RASTER_STATE_CULL;

    SetUniforms(drawCommand->UniformOffset + sizeof(CanvasUniforms), drawCommand->pTexture);

    if (m_bEdgeAntialias)
    {
        m_DepthStencil = DEPTH_STENCIL_DRAW_AA;

        BindPipeline(TOPOLOGY_TRIANGLE_STRIP);

        // Draw fringes
        for (i = 0; i < npaths; i++)
        {
            SDrawCmd cmd;

            cmd.VertexCountPerInstance = paths[i].StrokeCount;
            cmd.InstanceCount = 1;
            cmd.StartVertexLocation = paths[i].StrokeOffset;
            cmd.StartInstanceLocation = 0;

            m_ImmediateCtx->Draw(&cmd);
        }
    }

    // Draw fill
    //m_RasterState = RASTER_STATE_NOCULL;
    m_DepthStencil = DEPTH_STENCIL_FILL;

    BindPipeline(TOPOLOGY_TRIANGLE_STRIP);

    SDrawCmd cmd;
    cmd.VertexCountPerInstance = drawCommand->VertexCount;
    cmd.InstanceCount          = 1;
    cmd.StartVertexLocation    = drawCommand->FirstVertex;
    cmd.StartInstanceLocation  = 0;
    m_ImmediateCtx->Draw(&cmd);
}

void ACanvasRenderer::DrawConvexFill(CanvasDrawCmd const* drawCommand, ITexture* pTexture)
{
    CanvasPath const* paths = &m_pDrawData->Paths[drawCommand->FirstPath];
    int               i, npaths = drawCommand->PathCount;

    SetUniforms(drawCommand->UniformOffset, pTexture);

    BindPipeline(TOPOLOGY_TRIANGLE_LIST);
    for (i = 0; i < npaths; i++)
    {
        // Emulate triangle fan with index buffer
        if (paths[i].FillCount > 2)
        {
            unsigned int numIndices = ((paths[i].FillCount - 2) * 3);

            BuildFanIndices(numIndices);

            SDrawIndexedCmd cmd;
            cmd.IndexCountPerInstance = numIndices;
            cmd.InstanceCount = 1;
            cmd.StartIndexLocation = 0;
            cmd.BaseVertexLocation = paths[i].FillOffset;
            cmd.StartInstanceLocation = 0;

            m_ImmediateCtx->Draw(&cmd);
        }
    }

    // Draw fringes
    BindPipeline(TOPOLOGY_TRIANGLE_STRIP);
    for (i = 0; i < npaths; i++)
    {
        if (paths[i].StrokeCount > 0)
        {
            SDrawCmd cmd;

            cmd.VertexCountPerInstance = paths[i].StrokeCount;
            cmd.InstanceCount          = 1;
            cmd.StartVertexLocation    = paths[i].StrokeOffset;
            cmd.StartInstanceLocation  = 0;

            m_ImmediateCtx->Draw(&cmd);
        }
    }
}

void ACanvasRenderer::DrawStroke(CanvasDrawCmd const* drawCommand, bool bStencilStroke)
{
    CanvasPath const* paths  = &m_pDrawData->Paths[drawCommand->FirstPath];
    int               npaths = drawCommand->PathCount, i;

    if (bStencilStroke)
    {
        // Fill the stroke base without overlap
        m_DepthStencil = DEPTH_STENCIL_STROKE_FILL;

        SetUniforms(drawCommand->UniformOffset + sizeof(CanvasUniforms), drawCommand->pTexture);
        BindPipeline(TOPOLOGY_TRIANGLE_STRIP);
        for (i = 0; i < npaths; i++)
        {
            SDrawCmd cmd;

            cmd.VertexCountPerInstance = paths[i].StrokeCount;
            cmd.InstanceCount          = 1;
            cmd.StartVertexLocation    = paths[i].StrokeOffset;
            cmd.StartInstanceLocation  = 0;

            m_ImmediateCtx->Draw(&cmd);
        }

        // Draw anti-aliased pixels.
        m_DepthStencil = DEPTH_STENCIL_DRAW_AA;

        SetUniforms(drawCommand->UniformOffset, drawCommand->pTexture);
        BindPipeline(TOPOLOGY_TRIANGLE_STRIP);
        for (i = 0; i < npaths; i++)
        {
            SDrawCmd cmd;

            cmd.VertexCountPerInstance = paths[i].StrokeCount;
            cmd.InstanceCount          = 1;
            cmd.StartVertexLocation    = paths[i].StrokeOffset;
            cmd.StartInstanceLocation  = 0;

            m_ImmediateCtx->Draw(&cmd);
        }

        // Clear stencil buffer.
        m_ImmediateCtx->BindPipeline(m_PipelineClearStencil);
        for (i = 0; i < npaths; i++)
        {
            SDrawCmd cmd;

            cmd.VertexCountPerInstance = paths[i].StrokeCount;
            cmd.InstanceCount          = 1;
            cmd.StartVertexLocation    = paths[i].StrokeOffset;
            cmd.StartInstanceLocation  = 0;

            m_ImmediateCtx->Draw(&cmd);
        }
    }
    else
    {
        SetUniforms(drawCommand->UniformOffset, drawCommand->pTexture);

        m_DepthStencil = DEPTH_STENCIL_DEFAULT;

        BindPipeline(TOPOLOGY_TRIANGLE_STRIP);

        // Draw Strokes
        for (i = 0; i < npaths; i++)
        {
            SDrawCmd cmd;

            cmd.VertexCountPerInstance = paths[i].StrokeCount;
            cmd.InstanceCount          = 1;
            cmd.StartVertexLocation    = paths[i].StrokeOffset;
            cmd.StartInstanceLocation  = 0;

            m_ImmediateCtx->Draw(&cmd);
        }
    }
}

void ACanvasRenderer::DrawTriangles(CanvasDrawCmd const* drawCommand)
{
    SetUniforms(drawCommand->UniformOffset, drawCommand->pTexture);
    BindPipeline(TOPOLOGY_TRIANGLE_LIST);

    SDrawCmd cmd;
    cmd.VertexCountPerInstance = drawCommand->VertexCount;
    cmd.InstanceCount          = 1;
    cmd.StartVertexLocation    = drawCommand->FirstVertex;
    cmd.StartInstanceLocation  = 0;
    m_ImmediateCtx->Draw(&cmd);
}

void ACanvasRenderer::SetUniforms(int uniformOffset, RenderCore::ITexture* pTexture)
{
    CanvasUniforms* frag = (CanvasUniforms*)&m_pDrawData->Uniforms[uniformOffset];

    CanvasUniforms* uniforms = MapDrawCallConstants<CanvasUniforms>();
    Platform::Memcpy(uniforms, frag, sizeof(CanvasUniforms));

    if (pTexture)
    {
        rtbl->BindTexture(0, pTexture);
    }
}

void ACanvasRenderer::SetBuffers()
{
    m_ImmediateCtx->BindVertexBuffer(0, GStreamBuffer, GFrameData->CanvasVertexData);
    m_ImmediateCtx->BindIndexBuffer(m_pFanIndexBuffer, INDEX_TYPE_UINT32, 0);
}

void ACanvasRenderer::BuildFanIndices(int numIndices, bool bRebind)
{
    if (numIndices <= m_FanIndices.Size())
        return;

    if (m_pFanIndexBuffer)
    {
        m_pFanIndexBuffer.Reset();
    }

    int numTriangles = (numIndices + 2) / 3;

    const int Granularity = 256;
    int mod = numTriangles % Granularity;
    numTriangles = mod ? numTriangles + Granularity - mod : numTriangles;

    numIndices = numTriangles * 3;

    uint32_t current = m_FanIndices.Size();
    uint32_t index = (current / 3) + 1;

    m_FanIndices.Resize(numIndices);
    uint32_t* pData = m_FanIndices.ToPtr();

    while (current + 3 <= m_FanIndices.Size())
    {
        pData[current++] = 0;
        pData[current++] = index++;
        pData[current++] = index;
    }

    RenderCore::SBufferDesc bufferDesc;
    bufferDesc.bImmutableStorage = true;
    bufferDesc.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
    bufferDesc.SizeInBytes = sizeof(uint32_t) * m_FanIndices.Size();
    GDevice->CreateBuffer(bufferDesc, m_FanIndices.ToPtr(), &m_pFanIndexBuffer);

    if (bRebind)
        m_ImmediateCtx->BindIndexBuffer(m_pFanIndexBuffer, INDEX_TYPE_UINT32, 0);
}

void ACanvasRenderer::BindPipeline(int Topology)
{
    m_ImmediateCtx->BindPipeline(m_PipelinePermut[Topology][m_RasterState][m_BlendState][m_DepthStencil][m_SamplerState]);
}
