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

#include "CubemapGenerator.h"
#include "RenderLocal.h"

#include <Core/Public/PodArray.h>

using namespace RenderCore;

ACubemapGenerator::ACubemapGenerator()
{
    SBufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;

    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof( SConstantData );
    GDevice->CreateBuffer( bufferCI, nullptr, &ConstantBuffer );

    Float4x4 const * cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();
    Float4x4 projMat = Float4x4::PerspectiveRevCC( Math::_HALF_PI, 1.0f, 1.0f, 0.1f, 100.0f );

    for ( int faceIndex = 0 ; faceIndex < 6 ; faceIndex++ ) {
        ConstantBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];
    }

    SAttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;

    SAttachmentRef attachmentRef = {};
    attachmentRef.Attachment = 0;

    SSubpassInfo subpassInfo = {};
    subpassInfo.NumColorAttachments = 1;
    subpassInfo.pColorAttachmentRefs = &attachmentRef;

    SRenderPassCreateInfo renderPassCI = {};
    renderPassCI.NumColorAttachments = 1;
    renderPassCI.pColorAttachments = &colorAttachment;
    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpassInfo;
    GDevice->CreateRenderPass( renderPassCI, &RP );

    SPipelineCreateInfo pipelineCI;

    SPipelineInputAssemblyInfo & ia = pipelineCI.IA;
    ia.Topology = PRIMITIVE_TRIANGLES;

    SDepthStencilStateInfo & depthStencil = pipelineCI.DSS;
    depthStencil.bDepthEnable = false;
    depthStencil.DepthWriteMask = DEPTH_WRITE_DISABLE;

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

    CreateVertexShader( "gen/cubemapgen.vert", vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateGeometryShader( "gen/cubemapgen.geom", pipelineCI.pGS );
    CreateFragmentShader( "gen/cubemapgen.frag", pipelineCI.pFS );

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SSamplerInfo samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    //samplerCI.bCubemapSeamless = true;

    SBufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.ResourceLayout.Samplers = &samplerCI;
    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.NumBuffers = AN_ARRAY_SIZE( buffers );
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline( pipelineCI, &Pipeline );
}

void ACubemapGenerator::GenerateArray( RenderCore::TEXTURE_FORMAT _Format, int _Resolution, int _SourcesCount, ITexture ** _Sources, TRef< RenderCore::ITexture > * ppTextureArray )
{
    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Format = _Format;
    textureCI.Resolution.TexCubemapArray.Width = _Resolution;
    textureCI.Resolution.TexCubemapArray.NumLayers = _SourcesCount;
    textureCI.NumLods = 1;
    GDevice->CreateTexture( textureCI, ppTextureArray );

    TRef< IResourceTable > resourceTbl;
    GDevice->CreateResourceTable( &resourceTbl );

    resourceTbl->BindBuffer( 0, ConstantBuffer );

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    SFramebufferAttachmentInfo attachment = {};
    attachment.pTexture = *ppTextureArray;
    attachment.LodNum = 0;

    SFramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = _Resolution;
    framebufferCI.Height = _Resolution;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    TRef< RenderCore::IFramebuffer > framebuffer;
    GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.pRenderPass = RP;
    renderPassBegin.RenderArea.Width = _Resolution;
    renderPassBegin.RenderArea.Height = _Resolution;

    rcmd->BeginRenderPass( renderPassBegin );

    viewport.Width = _Resolution;
    viewport.Height = _Resolution;

    rcmd->SetViewport( viewport );
    rcmd->BindResourceTable( resourceTbl );

    for ( int sourceIndex = 0 ; sourceIndex < _SourcesCount ; sourceIndex++ ) {
        ConstantBufferData.Index.X = sourceIndex * 6; // Offset for cubemap array layer

        ConstantBuffer->Write( &ConstantBufferData );

        resourceTbl->BindTexture( 0, _Sources[sourceIndex] );

        // Draw six faces in one draw call
        DrawSphere( Pipeline, 6 );
    }

    rcmd->EndRenderPass();
}

void ACubemapGenerator::Generate( RenderCore::TEXTURE_FORMAT _Format, int _Resolution, ITexture * _Source, TRef< RenderCore::ITexture > * ppTexture )
{
    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP;
    textureCI.Format = _Format;
    textureCI.Resolution.TexCubemap.Width = _Resolution;
    textureCI.NumLods = 1;
    GDevice->CreateTexture( textureCI, ppTexture );

    TRef< IResourceTable > resourceTbl;
    GDevice->CreateResourceTable( &resourceTbl );

    resourceTbl->BindBuffer( 0, ConstantBuffer );

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    SFramebufferAttachmentInfo attachment = {};
    attachment.pTexture = *ppTexture;
    attachment.LodNum = 0;

    SFramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = _Resolution;
    framebufferCI.Height = _Resolution;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    TRef< RenderCore::IFramebuffer > framebuffer;
    GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.pRenderPass = RP;
    renderPassBegin.RenderArea.Width = _Resolution;
    renderPassBegin.RenderArea.Height = _Resolution;

    rcmd->BeginRenderPass( renderPassBegin );

    viewport.Width = _Resolution;
    viewport.Height = _Resolution;

    rcmd->SetViewport( viewport );

    ConstantBufferData.Index.X = 0;

    ConstantBuffer->Write( &ConstantBufferData );

    resourceTbl->BindTexture( 0, _Source );

    rcmd->BindResourceTable( resourceTbl );

    // Draw six faces in one draw call
    DrawSphere( Pipeline, 6 );

    rcmd->EndRenderPass();
}
