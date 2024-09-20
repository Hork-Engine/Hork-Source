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
#include "DrawUtils.h"

#include <Hork/ShaderUtils/ShaderUtils.h>
#include <Hork/RHI/Common/FrameGraph.h>

HK_NAMESPACE_BEGIN

using namespace RHI;

static const TEXTURE_FORMAT TEX_FORMAT_ENVPROBE = TEXTURE_FORMAT_R11G11B10_FLOAT; //TEXTURE_FORMAT_RGBA16_FLOAT;

EnvProbeGenerator::EnvProbeGenerator(IDevice* device, RenderUtils::SphereMesh* sphereMesh) :
    m_Device(device),
    m_SphereMesh(sphereMesh)
{
    BufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof(ConstantData);
    m_Device->CreateBuffer(bufferCI, nullptr, &m_ConstantBuffer);

    Float4x4 const* cubeFaceMatrices = Float4x4::sGetCubeFaceMatrices();

    Float4x4::PerspectiveMatrixDesc desc = {};
    desc.AspectRatio = 1;
    desc.FieldOfView = 90;
    desc.ZNear = 0.1f;
    desc.ZFar = 100.0f;
    Float4x4 projMat = Float4x4::sGetPerspectiveMatrix(desc);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        m_ConstantBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];
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

    ShaderUtils::CreateVertexShader(m_Device, "gen/envprobegen.vert", vertexAttribs, HK_ARRAY_SIZE(vertexAttribs), pipelineCI.pVS);
    ShaderUtils::CreateGeometryShader(m_Device, "gen/envprobegen.geom", pipelineCI.pGS);
    ShaderUtils::CreateFragmentShader(m_Device, "gen/envprobegen.frag", pipelineCI.pFS);

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

    m_Device->CreatePipeline(pipelineCI, &m_Pipeline);
}

void EnvProbeGenerator::GenerateArray(int maxLod, int cubemapsCount, ITexture** cubemaps, Ref<RHI::ITexture>* ppTextureArray)
{
    int size = 1 << maxLod;

    m_Device->CreateTexture(TextureDesc()
                               .SetFormat(TEX_FORMAT_ENVPROBE)
                               .SetResolution(TextureResolutionCubemapArray(size, cubemapsCount))
                               .SetMipLevels(maxLod + 1),
                           ppTextureArray);

    FrameGraph frameGraph(m_Device);

    FGTextureProxy* pCubemapArrayProxy = frameGraph.AddExternalResource<FGTextureProxy>("CubemapArray", *ppTextureArray);
    Ref<IResourceTable> resourceTbl;
    m_Device->CreateResourceTable(&resourceTbl);

    resourceTbl->BindBuffer(0, m_ConstantBuffer);

    int lodWidth = size;

    Vector<String> strs(maxLod + 1);

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

                            m_ConstantBufferData.Roughness.X = static_cast<float>(Lod) / maxLod;

                            for (int cubemapIndex = 0; cubemapIndex < cubemapsCount; cubemapIndex++)
                            {
                                m_ConstantBufferData.Roughness.Y = cubemapIndex * 6; // Offset for cubemap array layer

                                immediateCtx->WriteBufferRange(m_ConstantBuffer, 0, sizeof(m_ConstantBufferData), &m_ConstantBufferData);

                                resourceTbl->BindTexture(0, cubemaps[cubemapIndex]);

                                // Draw six faces in one draw call
                                m_SphereMesh->Draw(immediateCtx, m_Pipeline, 6);
                            }
                        });
    }

    frameGraph.Build();
    //frameGraph.ExportGraphviz( "framegraph.graphviz" );
    m_Device->GetImmediateContext()->ExecuteFrameGraph(&frameGraph);
}

void EnvProbeGenerator::Generate(int maxLod, ITexture* sourceCubemap, Ref<RHI::ITexture>* ppTexture)
{
    int size = 1 << maxLod;

    m_Device->CreateTexture(TextureDesc()
                               .SetFormat(TEX_FORMAT_ENVPROBE)
                               .SetResolution(TextureResolutionCubemap(size))
                               .SetMipLevels(maxLod + 1),
                           ppTexture);

    FrameGraph frameGraph(m_Device);

    FGTextureProxy* pCubemapProxy = frameGraph.AddExternalResource<FGTextureProxy>("Cubemap", *ppTexture);
    Ref<IResourceTable> resourceTbl;
    m_Device->CreateResourceTable(&resourceTbl);

    resourceTbl->BindBuffer(0, m_ConstantBuffer);

    m_ConstantBufferData.Roughness.Y = 0; // Offset for cubemap array layer

    int lodWidth = size;

    Vector<String> strs(maxLod + 1);

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

                            m_ConstantBufferData.Roughness.X = static_cast<float>(Lod) / maxLod;

                            immediateCtx->WriteBufferRange(m_ConstantBuffer, 0, sizeof(m_ConstantBufferData), &m_ConstantBufferData);

                            resourceTbl->BindTexture(0, sourceCubemap);

                            // Draw six faces in one draw call
                            m_SphereMesh->Draw(immediateCtx, m_Pipeline, 6);
                        });
    }

    frameGraph.Build();
    //frameGraph.ExportGraphviz( "framegraph.graphviz" );
    m_Device->GetImmediateContext()->ExecuteFrameGraph(&frameGraph);
}

HK_NAMESPACE_END