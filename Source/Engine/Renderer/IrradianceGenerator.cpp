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

#include "IrradianceGenerator.h"

#include <Core/Public/PodArray.h>

using namespace RenderCore;

static const TEXTURE_FORMAT TEX_FORMAT_IRRADIANCE = TEXTURE_FORMAT_RGB16F; // TODO: try compression

AIrradianceGenerator::AIrradianceGenerator() {
    SBufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof( SIrradianceGeneratorUniformBuffer );
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

    TRef< IShaderModule > vertexShader, geometryShader, fragmentShader;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    AString vertexSource = LoadShader( "gen/irradiancegen.vert" );
    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSource.CStr() );
    GShaderSources.Build( VERTEX_SHADER, vertexShader );

    AString geometrySource = LoadShader( "gen/irradiancegen.geom" );
    GShaderSources.Clear();
    GShaderSources.Add( geometrySource.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShader );

    AString fragmentSource = LoadShader( "gen/irradiancegen.frag" );
    GShaderSources.Clear();
    GShaderSources.Add( fragmentSource.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShader );

    pipelineCI.pVS = vertexShader;
    pipelineCI.pGS = geometryShader;
    pipelineCI.pFS = fragmentShader;
    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    SSamplerInfo samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.bCubemapSeamless = true;

    pipelineCI.SS.Samplers = &samplerCI;
    pipelineCI.SS.NumSamplers = 1;

    GDevice->CreatePipeline( pipelineCI, &m_Pipeline );
}

void AIrradianceGenerator::GenerateArray( int _CubemapsCount, ITexture ** _Cubemaps, TRef< RenderCore::ITexture > * ppTextureArray ) {
    int size = 32;

    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Format = TEX_FORMAT_IRRADIANCE;
    textureCI.Resolution.TexCubemapArray.Width = size;
    textureCI.Resolution.TexCubemapArray.NumLayers = _CubemapsCount;
    textureCI.NumLods = 1;

    GDevice->CreateTexture( textureCI, ppTextureArray );

    SResourceTable resourceTable;

    SResourceBufferBinding * uniformBufferBinding = resourceTable.AddBuffer( UNIFORM_BUFFER );
    uniformBufferBinding->pBuffer = m_UniformBuffer;

    SResourceTextureBinding * textureBinding = resourceTable.AddTexture();

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    SFramebufferAttachmentInfo attachment = {};
    attachment.pTexture = *ppTextureArray;
    attachment.LodNum = 0;

    SFramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = size;
    framebufferCI.Height = size;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    TRef< RenderCore::IFramebuffer > framebuffer;
    GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.pRenderPass = m_RP;
    renderPassBegin.RenderArea.Width = size;
    renderPassBegin.RenderArea.Height = size;

    rcmd->BeginRenderPass( renderPassBegin );

    viewport.Width = size;
    viewport.Height = size;

    rcmd->SetViewport( viewport );

    for ( int cubemapIndex = 0 ; cubemapIndex < _CubemapsCount ; cubemapIndex++ ) {

        m_UniformBufferData.Index.X = cubemapIndex * 6; // Offset for cubemap array layer

        m_UniformBuffer->Write( &m_UniformBufferData );

        textureBinding->pTexture = _Cubemaps[cubemapIndex];

        rcmd->BindResourceTable( &resourceTable );

        // Draw six faces in one draw call
        DrawSphere( m_Pipeline, 6 );
    }

    rcmd->EndRenderPass();
}

void AIrradianceGenerator::Generate( ITexture * _SourceCubemap, TRef< RenderCore::ITexture > * ppTexture ) {
    int size = 32;

    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP;
    textureCI.Format = TEX_FORMAT_IRRADIANCE;
    textureCI.Resolution.TexCubemap.Width = size;
    textureCI.NumLods = 1;

    GDevice->CreateTexture( textureCI, ppTexture );

    SResourceTable resourceTable;

    SResourceBufferBinding * uniformBufferBinding = resourceTable.AddBuffer( UNIFORM_BUFFER );
    uniformBufferBinding->pBuffer = m_UniformBuffer;

    SResourceTextureBinding * textureBinding = resourceTable.AddTexture();

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    SFramebufferAttachmentInfo attachment = {};
    attachment.pTexture = *ppTexture;
    attachment.LodNum = 0;

    SFramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = size;
    framebufferCI.Height = size;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    TRef< RenderCore::IFramebuffer > framebuffer;
    GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.pRenderPass = m_RP;
    renderPassBegin.RenderArea.Width = size;
    renderPassBegin.RenderArea.Height = size;

    rcmd->BeginRenderPass( renderPassBegin );

    viewport.Width = size;
    viewport.Height = size;

    rcmd->SetViewport( viewport );

    m_UniformBufferData.Index.X = 0;

    m_UniformBuffer->Write( &m_UniformBufferData );

    textureBinding->pTexture = _SourceCubemap;

    rcmd->BindResourceTable( &resourceTable );

    // Draw six faces in one draw call
    DrawSphere( m_Pipeline, 6 );

    rcmd->EndRenderPass();
}
