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

#include "AtmosphereRenderer.h"
#include "RenderLocal.h"

using namespace RenderCore;

static const TEXTURE_FORMAT TEX_FORMAT_SKY = TEXTURE_FORMAT_R11G11B10_FLOAT;

AAtmosphereRenderer::AAtmosphereRenderer() {
    SBufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof( SConstantData );
    GDevice->CreateBuffer( bufferCI, nullptr, &ConstantBuffer );

    Float4x4 const * cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();
    Float4x4 projMat = Float4x4::PerspectiveRevCC( Math::_HALF_PI, 1.0f, 1.0f, 0.1f, 100.0f );

    for ( int faceIndex = 0 ; faceIndex < 6 ; faceIndex++ ) {
        ConstantBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];
    }

    SPipelineDesc pipelineCI;

    SPipelineInputAssemblyInfo & ia = pipelineCI.IA;
    ia.Topology = PRIMITIVE_TRIANGLES;

    SDepthStencilStateInfo & depthStencil = pipelineCI.DSS;
    depthStencil.bDepthEnable = false;
    depthStencil.bDepthWrite = false;

    SVertexBindingInfo vertexBindings[] =
    {
        {
            0,                              // vertex buffer binding
            sizeof( Float3 ),               // vertex stride
            INPUT_RATE_PER_VERTEX,          // per vertex / per instance
        }
    };

    SVertexAttribInfo vertexAttribs[] =
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

    AShaderFactory::CreateVertexShader( "gen/atmosphere.vert", vertexAttribs, HK_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    AShaderFactory::CreateGeometryShader( "gen/atmosphere.geom", pipelineCI.pGS );
    AShaderFactory::CreateFragmentShader( "gen/atmosphere.frag", pipelineCI.pFS );

    SBufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.NumVertexBindings = HK_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = HK_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.ResourceLayout.NumBuffers = HK_ARRAY_SIZE( buffers );
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline( pipelineCI, &Pipeline );
}

void AAtmosphereRenderer::Render( int CubemapWidth, Float3 const & LightDir, TRef< RenderCore::ITexture > * ppTexture )
{
    AFrameGraph frameGraph( GDevice );
    ARenderPass & pass = frameGraph.AddTask< ARenderPass >( "Atmosphere pass" );

    pass.SetRenderArea( CubemapWidth, CubemapWidth );

    pass.SetColorAttachments(
        {
            {
                STextureAttachment("Render target texture",
                                    STextureDesc()
                                        .SetFormat( TEX_FORMAT_SKY )
                                        .SetResolution(STextureResolutionCubemap(CubemapWidth))
                )
                .SetLoadOp(ATTACHMENT_LOAD_OP_DONT_CARE)
            }
        } );

    pass.AddSubpass({0}, // color attachments
                    [&](ARenderPassContext& RenderPassContext, ACommandBuffer& CommandBuffer)
                    {
                        IImmediateContext* immediateCtx = RenderPassContext.pImmediateContext;

                        ConstantBufferData.LightDir = Float4(LightDir.Normalized(), 0.0f);

                        immediateCtx->WriteBufferRange(ConstantBuffer, 0, sizeof(ConstantBufferData), &ConstantBufferData);

                        TRef<IResourceTable> resourceTbl;
                        GDevice->CreateResourceTable(&resourceTbl);

                        resourceTbl->BindBuffer(0, ConstantBuffer);

                        immediateCtx->BindResourceTable(resourceTbl);

                        // Draw six faces in one draw call
                        DrawSphere(immediateCtx, Pipeline, 6);
                    });
    
    FGTextureProxy * pTexture = pass.GetColorAttachments()[0].pResource;
    pTexture->SetResourceCapture( true );

    frameGraph.Build();
    rcmd->ExecuteFrameGraph(&frameGraph);

    *ppTexture = pTexture->Actual();
}
