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

#include "EnvProbeGenerator.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

static const TEXTURE_FORMAT TEX_FORMAT_ENVPROBE = TEXTURE_FORMAT_R11G11B10_FLOAT; //TEXTURE_FORMAT_RGBA16_FLOAT;

EnvProbeGenerator::EnvProbeGenerator()
{
    BufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof(ConstantData);
    GDevice->CreateBuffer(bufferCI, nullptr, &ConstantBuffer);

    Float4x4 const* cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();

    Float4x4::PerspectiveMatrixDesc desc = {};
    desc.AspectRatio = 1;
    desc.FieldOfView = 90;
    desc.ZNear = 0.1f;
    desc.ZFar = 100.0f;
    Float4x4 projMat = Float4x4::GetPerspectiveMatrix(desc);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        ConstantBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];
    }

    PipelineDesc pipelineCI;

    PipelineInputAssemblyInfo& ia = pipelineCI.IA;
    ia.Topology = PRIMITIVE_TRIANGLES;

    DepthStencilStateInfo& depthStencil = pipelineCI.DSS;
    depthStencil.bDepthEnable = false;
    depthStencil.bDepthWrite = false;

    VertexBindingInfo vertexBindings[] =
        {
            {
                0,                     // vertex buffer binding
                sizeof(Float3),        // vertex stride
                INPUT_RATE_PER_VERTEX, // per vertex / per instance
            }};

    VertexAttribInfo vertexAttribs[] =
        {
            {"InPosition",
             0,
             0, // vertex buffer binding
             VAT_FLOAT3,
             VAM_FLOAT,
             0,
             0}};

    ShaderFactory::CreateVertexShader("gen/envprobegen.vert", vertexAttribs, HK_ARRAY_SIZE(vertexAttribs), pipelineCI.pVS);
    ShaderFactory::CreateGeometryShader("gen/envprobegen.geom", pipelineCI.pGS);
    ShaderFactory::CreateFragmentShader("gen/envprobegen.frag", pipelineCI.pFS);

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE(vertexBindings);
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(vertexAttribs);
    pipelineCI.pVertexAttribs = vertexAttribs;

    SamplerDesc samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.bCubemapSeamless = true;

    BufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.ResourceLayout.Samplers = &samplerCI;
    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline(pipelineCI, &Pipeline);
}

void EnvProbeGenerator::GenerateArray(int _MaxLod, int _CubemapsCount, ITexture** _Cubemaps, Ref<RenderCore::ITexture>* ppTextureArray)
{
    int size = 1 << _MaxLod;

    GDevice->CreateTexture(TextureDesc()
                               .SetFormat(TEX_FORMAT_ENVPROBE)
                               .SetResolution(TextureResolutionCubemapArray(size, _CubemapsCount))
                               .SetMipLevels(_MaxLod + 1),
                           ppTextureArray);

    FrameGraph frameGraph(GDevice);

    FGTextureProxy* pCubemapArrayProxy = frameGraph.AddExternalResource<FGTextureProxy>("CubemapArray", *ppTextureArray);
    Ref<IResourceTable> resourceTbl;
    GDevice->CreateResourceTable(&resourceTbl);

    resourceTbl->BindBuffer(0, ConstantBuffer);

    int lodWidth = size;

    Vector<String> strs(_MaxLod + 1);

    for (int Lod = 0; lodWidth >= 1; Lod++, lodWidth >>= 1)
    {

        strs[Lod] = HK_FORMAT("Envprobe LOD {} pass", Lod);

        RenderPass& pass = frameGraph.AddTask<RenderPass>(strs[Lod].CStr());

        //pass.SetRenderArea( lodWidth, lodWidth );

        pass.SetColorAttachments(
            {{TextureAttachment(pCubemapArrayProxy)
                  .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE)
                  .SetMipLevel(Lod)}});

        pass.AddSubpass({0}, // color attachments
                        [&, Lod](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                            immediateCtx->BindResourceTable(resourceTbl);

                            ConstantBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

                            for (int cubemapIndex = 0; cubemapIndex < _CubemapsCount; cubemapIndex++)
                            {
                                ConstantBufferData.Roughness.Y = cubemapIndex * 6; // Offset for cubemap array layer

                                immediateCtx->WriteBufferRange(ConstantBuffer, 0, sizeof(ConstantBufferData), &ConstantBufferData);

                                resourceTbl->BindTexture(0, _Cubemaps[cubemapIndex]);

                                // Draw six faces in one draw call
                                DrawSphere(immediateCtx, Pipeline, 6);
                            }
                        });
    }

    frameGraph.Build();
    //frameGraph.ExportGraphviz( "framegraph.graphviz" );
    rcmd->ExecuteFrameGraph(&frameGraph);
}

void EnvProbeGenerator::Generate(int _MaxLod, ITexture* _SourceCubemap, Ref<RenderCore::ITexture>* ppTexture)
{
    int size = 1 << _MaxLod;

    GDevice->CreateTexture(TextureDesc()
                               .SetFormat(TEX_FORMAT_ENVPROBE)
                               .SetResolution(TextureResolutionCubemap(size))
                               .SetMipLevels(_MaxLod + 1),
                           ppTexture);

    FrameGraph frameGraph(GDevice);

    FGTextureProxy* pCubemapProxy = frameGraph.AddExternalResource<FGTextureProxy>("Cubemap", *ppTexture);
    Ref<IResourceTable> resourceTbl;
    GDevice->CreateResourceTable(&resourceTbl);

    resourceTbl->BindBuffer(0, ConstantBuffer);

    ConstantBufferData.Roughness.Y = 0; // Offset for cubemap array layer

    int lodWidth = size;

    Vector<String> strs(_MaxLod + 1);

    for (int Lod = 0; lodWidth >= 1; Lod++, lodWidth >>= 1)
    {

        strs[Lod] = HK_FORMAT("Envprobe LOD {} pass", Lod);

        RenderPass& pass = frameGraph.AddTask<RenderPass>(strs[Lod].CStr());

        pass.SetRenderArea(lodWidth, lodWidth);

        pass.SetColorAttachment(
            TextureAttachment(pCubemapProxy)
                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE)
                .SetMipLevel(Lod));

        pass.AddSubpass({0}, // color attachments
                        [&, Lod](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                        {
                            IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                            immediateCtx->BindResourceTable(resourceTbl);

                            ConstantBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

                            immediateCtx->WriteBufferRange(ConstantBuffer, 0, sizeof(ConstantBufferData), &ConstantBufferData);

                            resourceTbl->BindTexture(0, _SourceCubemap);

                            // Draw six faces in one draw call
                            DrawSphere(immediateCtx, Pipeline, 6);
                        });
    }

    frameGraph.Build();
    //frameGraph.ExportGraphviz( "framegraph.graphviz" );
    rcmd->ExecuteFrameGraph(&frameGraph);
}

HK_NAMESPACE_END
