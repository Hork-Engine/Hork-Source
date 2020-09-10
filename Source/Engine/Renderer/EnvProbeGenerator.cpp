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

#include "EnvProbeGenerator.h"

#include <Core/Public/PodArray.h>

using namespace RenderCore;

static const TEXTURE_FORMAT TEX_FORMAT_ENVPROBE = TEXTURE_FORMAT_RGB16F; // TODO: try compression

AEnvProbeGenerator::AEnvProbeGenerator()
{
    SBufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof( SRoughnessUniformBuffer );
    GDevice->CreateBuffer( bufferCI, nullptr, &m_UniformBuffer );

    Float4x4 const * cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();
    Float4x4 projMat = Float4x4::PerspectiveRevCC( Math::_HALF_PI, 1.0f, 1.0f, 0.1f, 100.0f );

    for ( int faceIndex = 0 ; faceIndex < 6 ; faceIndex++ ) {
        m_UniformBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];
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
    GDevice->CreateRenderPass( renderPassCI, &m_RP );

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

    CreateVertexShader( "gen/envprobegen.vert", vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateGeometryShader( "gen/envprobegen.geom", pipelineCI.pGS );
    CreateFragmentShader( "gen/envprobegen.frag", pipelineCI.pFS );

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SSamplerInfo samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.bCubemapSeamless = true;

    SBufferInfo buffers[1];
    buffers[0].BufferType = UNIFORM_BUFFER;

    pipelineCI.ResourceLayout.Samplers = &samplerCI;
    pipelineCI.ResourceLayout.NumSamplers = 1;
    pipelineCI.ResourceLayout.NumBuffers = AN_ARRAY_SIZE( buffers );
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline( pipelineCI, &m_Pipeline );    
}

void AEnvProbeGenerator::GenerateArray( int _MaxLod, int _CubemapsCount, ITexture ** _Cubemaps, TRef< RenderCore::ITexture > * ppTextureArray )
{
    int size = 1 << _MaxLod;

    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Format = TEX_FORMAT_ENVPROBE;
    textureCI.Resolution.TexCubemapArray.Width = size;
    textureCI.Resolution.TexCubemapArray.NumLayers = _CubemapsCount;
    textureCI.NumLods = _MaxLod + 1;
    GDevice->CreateTexture( textureCI, ppTextureArray );

    TRef< IResourceTable > resourceTbl;
    GDevice->CreateResourceTable( &resourceTbl );

    resourceTbl->BindBuffer( 0, m_UniformBuffer );

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    int lodWidth = size;

    for ( int Lod = 0 ; lodWidth >= 1 ; Lod++, lodWidth >>= 1 ) {
        SFramebufferAttachmentInfo attachment = {};
        attachment.pTexture = *ppTextureArray;
        attachment.LodNum = Lod;

        SFramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = lodWidth;
        framebufferCI.Height = lodWidth;
        framebufferCI.NumColorAttachments = 1;
        framebufferCI.pColorAttachments = &attachment;

        TRef< RenderCore::IFramebuffer > framebuffer;
        GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

        SRenderPassBegin renderPassBegin = {};
        renderPassBegin.pFramebuffer = framebuffer;
        renderPassBegin.pRenderPass = m_RP;
        renderPassBegin.RenderArea.Width = lodWidth;
        renderPassBegin.RenderArea.Height = lodWidth;

        rcmd->BeginRenderPass( renderPassBegin );

        viewport.Width = lodWidth;
        viewport.Height = lodWidth;

        rcmd->SetViewport( viewport );
        rcmd->BindResourceTable( resourceTbl );

        m_UniformBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

        for ( int cubemapIndex = 0 ; cubemapIndex < _CubemapsCount ; cubemapIndex++ ) {
            m_UniformBufferData.Roughness.Y = cubemapIndex * 6; // Offset for cubemap array layer

            m_UniformBuffer->Write( &m_UniformBufferData );

            resourceTbl->BindTexture( 0, _Cubemaps[cubemapIndex] );

            // Draw six faces in one draw call
            DrawSphere( m_Pipeline, 6 );
        }

        rcmd->EndRenderPass();
    }
}

void AEnvProbeGenerator::Generate( int _MaxLod, ITexture * _SourceCubemap, TRef< RenderCore::ITexture > * ppTexture )
{
    int size = 1 << _MaxLod;

    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP;
    textureCI.Format = TEX_FORMAT_ENVPROBE;
    textureCI.Resolution.TexCubemap.Width = size;
    textureCI.NumLods = _MaxLod + 1;
    GDevice->CreateTexture( textureCI, ppTexture );

    TRef< IResourceTable > resourceTbl;
    GDevice->CreateResourceTable( &resourceTbl );

    resourceTbl->BindBuffer( 0, m_UniformBuffer );

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    int lodWidth = size;

    m_UniformBufferData.Roughness.Y = 0; // Offset for cubemap array layer

    rcmd->BindResourceTable( resourceTbl );

    for ( int Lod = 0 ; lodWidth >= 1 ; Lod++, lodWidth >>= 1 ) {
        SFramebufferAttachmentInfo attachment = {};
        attachment.pTexture = *ppTexture;
        attachment.LodNum = Lod;

        SFramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = lodWidth;
        framebufferCI.Height = lodWidth;
        framebufferCI.NumColorAttachments = 1;
        framebufferCI.pColorAttachments = &attachment;

        TRef< RenderCore::IFramebuffer > framebuffer;
        GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

        SRenderPassBegin renderPassBegin = {};
        renderPassBegin.pFramebuffer = framebuffer;
        renderPassBegin.pRenderPass = m_RP;
        renderPassBegin.RenderArea.Width = lodWidth;
        renderPassBegin.RenderArea.Height = lodWidth;

        rcmd->BeginRenderPass( renderPassBegin );

        viewport.Width = lodWidth;
        viewport.Height = lodWidth;

        rcmd->SetViewport( viewport );

        m_UniformBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

        m_UniformBuffer->Write( &m_UniformBufferData );

        resourceTbl->BindTexture( 0, _SourceCubemap );

        // Draw six faces in one draw call
        DrawSphere( m_Pipeline, 6 );

        rcmd->EndRenderPass();
    }
}
