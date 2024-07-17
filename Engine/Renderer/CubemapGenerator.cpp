/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "CubemapGenerator.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

CubemapGenerator::CubemapGenerator()
{
    BufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;

    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof(ConstantData);
    GDevice->CreateBuffer(bufferCI, nullptr, &ConstantBuffer);

    Float4x4 const * cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();

    Float4x4::PerspectiveMatrixDesc desc = {};
    desc.AspectRatio = 1;
    desc.FieldOfView = 90;
    desc.ZNear = 0.1f;
    desc.ZFar = 100.0f;
    Float4x4 projMat = Float4x4::GetPerspectiveMatrix(desc);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        ConstantBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];

    PipelineDesc pipelineCI;

    PipelineInputAssemblyInfo & ia = pipelineCI.IA;
    ia.Topology = PRIMITIVE_TRIANGLES;

    DepthStencilStateInfo & depthStencil = pipelineCI.DSS;
    depthStencil.bDepthEnable = false;
    depthStencil.bDepthWrite = false;

    VertexBindingInfo vertexBindings[] =
    {
        {
            0,                              // vertex buffer binding
            sizeof( Float3 ),               // vertex stride
            INPUT_RATE_PER_VERTEX,          // per vertex / per instance
        }
    };

    VertexAttribInfo vertexAttribs[] =
    {
        {
            "InPosition",
            0,
            0,          // vertex buffer binding
            VAT_FLOAT3,
            VAM_FLOAT,
            0,
            0
        }
    };

    ShaderFactory::CreateVertexShader("gen/cubemapgen.vert", vertexAttribs, HK_ARRAY_SIZE(vertexAttribs), pipelineCI.pVS);
    ShaderFactory::CreateGeometryShader("gen/cubemapgen.geom", pipelineCI.pGS);
    ShaderFactory::CreateFragmentShader("gen/cubemapgen.frag", pipelineCI.pFS);

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE(vertexBindings);
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(vertexAttribs);
    pipelineCI.pVertexAttribs = vertexAttribs;

    SamplerDesc samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    //samplerCI.bCubemapSeamless = true;

    BufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.ResourceLayout.Samplers = &samplerCI;
    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE( buffers );
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline( pipelineCI, &Pipeline );
}

void CubemapGenerator::GenerateArray(TEXTURE_FORMAT _Format, int _Resolution, int _SourcesCount, ITexture** _Sources, Ref<RenderCore::ITexture>* ppTextureArray)
{
    GDevice->CreateTexture(RenderCore::TextureDesc()
                               .SetFormat(_Format)
                               .SetResolution(TextureResolutionCubemapArray(_Resolution, _SourcesCount)),
                           ppTextureArray);

    FrameGraph frameGraph(GDevice);

    FGTextureProxy* pCubemapArrayProxy = frameGraph.AddExternalResource<FGTextureProxy>("CubemapArray", *ppTextureArray);

    Ref<IResourceTable> resourceTbl;
    GDevice->CreateResourceTable(&resourceTbl);

    resourceTbl->BindBuffer(0, ConstantBuffer);

    RenderPass& pass = frameGraph.AddTask<RenderPass>("Irradiance gen pass");

    pass.SetRenderArea(_Resolution, _Resolution);

    pass.SetColorAttachment(
        TextureAttachment(pCubemapArrayProxy)
        .SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
    );

    pass.AddSubpass({0}, // color attachments
                    [&](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        immediateCtx->BindResourceTable(resourceTbl);

                        for (int sourceIndex = 0; sourceIndex < _SourcesCount; sourceIndex++)
                        {
                            ConstantBufferData.Index.X = sourceIndex * 6; // Offset for cubemap array layer

                            immediateCtx->WriteBufferRange(ConstantBuffer, 0, sizeof(ConstantBufferData), &ConstantBufferData);

                            resourceTbl->BindTexture(0, _Sources[sourceIndex]);

                            // Draw six faces in one draw call
                            DrawSphere(immediateCtx, Pipeline, 6);
                        }
                    });

    frameGraph.Build();
    rcmd->ExecuteFrameGraph(&frameGraph);
}

void CubemapGenerator::Generate(TEXTURE_FORMAT _Format, int _Resolution, ITexture* _Source, Ref<RenderCore::ITexture>* ppTexture)
{
    GDevice->CreateTexture(TextureDesc()
                               .SetFormat(_Format)
                               .SetResolution(TextureResolutionCubemap(_Resolution)),
                           ppTexture);

    FrameGraph frameGraph( GDevice );

    FGTextureProxy* pCubemapProxy = frameGraph.AddExternalResource<FGTextureProxy>("Cubemap", *ppTexture);

    Ref< IResourceTable > resourceTbl;
    GDevice->CreateResourceTable( &resourceTbl );

    resourceTbl->BindBuffer( 0, ConstantBuffer );

    RenderPass & pass = frameGraph.AddTask< RenderPass >( "Irradiance gen pass" );

    pass.SetRenderArea( _Resolution, _Resolution );

    pass.SetColorAttachment(
        TextureAttachment(pCubemapProxy)
        .SetLoadOp( ATTACHMENT_LOAD_OP_DONT_CARE )
    );

    pass.AddSubpass({0}, // color attachments
                    [&](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        ConstantBufferData.Index.X = 0;

                        immediateCtx->WriteBufferRange(ConstantBuffer, 0, sizeof(ConstantBufferData), &ConstantBufferData);

                        resourceTbl->BindTexture(0, _Source);

                        immediateCtx->BindResourceTable(resourceTbl);

                        // Draw six faces in one draw call
                        DrawSphere(immediateCtx, Pipeline, 6);
                    });

    frameGraph.Build();
    rcmd->ExecuteFrameGraph(&frameGraph);
}

HK_NAMESPACE_END
