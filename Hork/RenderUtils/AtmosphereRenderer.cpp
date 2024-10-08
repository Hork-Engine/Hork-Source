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

#include "AtmosphereRenderer.h"
#include "DrawUtils.h"

#include <Hork/ShaderUtils/ShaderUtils.h>
#include <Hork/RHI/Common/FrameGraph.h>

HK_NAMESPACE_BEGIN

using namespace RHI;

//static const TEXTURE_FORMAT TEX_FORMAT_SKY = TEXTURE_FORMAT_RGBA32_FLOAT;
static const TEXTURE_FORMAT TEX_FORMAT_SKY = TEXTURE_FORMAT_R11G11B10_FLOAT;

AtmosphereRenderer::AtmosphereRenderer(IDevice* device, RenderUtils::SphereMesh* sphereMesh) :
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

    ShaderUtils::CreateVertexShader(m_Device, "gen/atmosphere.vert", vertexAttribs, HK_ARRAY_SIZE(vertexAttribs), pipelineCI.pVS);
    ShaderUtils::CreateGeometryShader(m_Device, "gen/atmosphere.geom", pipelineCI.pGS);
    ShaderUtils::CreateFragmentShader(m_Device, "gen/atmosphere.frag", pipelineCI.pFS);

    BufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE(vertexBindings);
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE(vertexAttribs);
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE(buffers);
    pipelineCI.ResourceLayout.Buffers = buffers;

    m_Device->CreatePipeline(pipelineCI, &m_Pipeline);
}

void AtmosphereRenderer::Render(TEXTURE_FORMAT format, int cubemapWidth, Float3 const& lightDir, Ref<RHI::ITexture>* ppTexture)
{
    FrameGraph frameGraph(m_Device);
    RenderPass& pass = frameGraph.AddTask<RenderPass>("Atmosphere pass");

    pass.SetRenderArea(cubemapWidth, cubemapWidth);

    pass.SetColorAttachments(
        {{TextureAttachment("Render target texture",
                            TextureDesc()
                                .SetFormat(format)
                                .SetResolution(TextureResolutionCubemap(cubemapWidth)))
              .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE)}});

    pass.AddSubpass({0}, // color attachments
                    [&](FGRenderPassContext& RenderPassContext, FGCommandBuffer& CommandBuffer)
                    {
                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        m_ConstantBufferData.LightDir = Float4(lightDir.Normalized(), 0.0f);

                        immediateCtx->WriteBufferRange(m_ConstantBuffer, 0, sizeof(m_ConstantBufferData), &m_ConstantBufferData);

                        Ref<IResourceTable> resourceTbl;
                        m_Device->CreateResourceTable(&resourceTbl);

                        resourceTbl->BindBuffer(0, m_ConstantBuffer);

                        immediateCtx->BindResourceTable(resourceTbl);

                        // Draw six faces in one draw call
                        m_SphereMesh->Draw(immediateCtx, m_Pipeline, 6);
                    });

    FGTextureProxy* pTexture = pass.GetColorAttachments()[0].pResource;
    pTexture->SetResourceCapture(true);

    frameGraph.Build();

    m_Device->GetImmediateContext()->ExecuteFrameGraph(&frameGraph);

    *ppTexture = pTexture->Actual();
}

HK_NAMESPACE_END
