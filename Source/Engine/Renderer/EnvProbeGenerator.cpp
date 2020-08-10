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

// TODO: Replace with Cube
static void CreateSphere( int _HDiv, int _VDiv, TPodArray< Float3 > & _Vertices, TPodArray< unsigned short > & _Indices ) {
    const int numVerts = _VDiv * (_HDiv - 1) + 2;
    const int numIndices = (_HDiv - 1) * (_VDiv - 1) * 6;
    int i, j;
    float a1, a2;

    AN_ASSERT_( numVerts < 65536, "Too many vertices" );

    _Vertices.Clear();
    _Indices.Clear();

    _Vertices.Resize( numVerts );
    _Indices.Resize( numIndices );

    for ( i = 0, a1 = Math::_PI / _HDiv; i < (_HDiv - 1); i++ ) {
        float y, r;
        Math::SinCos( a1, r, y );
        for ( j = 0, a2 = 0; j < _VDiv; j++ ) {
            float s, c;
            Math::SinCos( a2, s, c );
            _Vertices[i*_VDiv + j] = Float3( r*c, -y, r*s );
            a2 += Math::_2PI / (_VDiv - 1);
        }
        a1 += Math::_PI / _HDiv;
    }
    _Vertices[(_HDiv - 1)*_VDiv + 0] = Float3( 0, -1, 0 );
    _Vertices[(_HDiv - 1)*_VDiv + 1] = Float3( 0, 1, 0 );

    // generate indices
    unsigned short *indices = _Indices.ToPtr();
    for ( i = 0; i < _HDiv; i++ ) {
        for ( j = 0; j < _VDiv - 1; j++ ) {
            unsigned short i2 = i + 1;
            unsigned short j2 = (j == _VDiv - 1) ? 0 : j + 1;
            if ( i == (_HDiv - 2) ) {
                *indices++ = (i*_VDiv + j2);
                *indices++ = (i*_VDiv + j);
                *indices++ = ((_HDiv - 1)*_VDiv + 1);

            } else if ( i == (_HDiv - 1) ) {
                *indices++ = (0 * _VDiv + j);
                *indices++ = (0 * _VDiv + j2);
                *indices++ = ((_HDiv - 1)*_VDiv + 0);

            } else {
                int quad[4] = { i*_VDiv + j, i*_VDiv + j2, i2*_VDiv + j2, i2*_VDiv + j };
                *indices++ = quad[3];
                *indices++ = quad[2];
                *indices++ = quad[1];
                *indices++ = quad[1];
                *indices++ = quad[0];
                *indices++ = quad[3];
            }
        }
    }
}

static const TEXTURE_FORMAT TEX_FORMAT_ENVPROBE = TEXTURE_FORMAT_RGB16F; // TODO: try compression

AEnvProbeGenerator::AEnvProbeGenerator() {
    TPodArray< Float3 > vertices;
    TPodArray< unsigned short > indices;

    CreateSphere( 128, 128, vertices, indices );

    m_IndexCount = indices.Size();

    SBufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;

    bufferCI.SizeInBytes = sizeof( Float3 ) * vertices.Size();
    GDevice->CreateBuffer( bufferCI, vertices.ToPtr(), &m_VertexBuffer );

    bufferCI.SizeInBytes = sizeof( unsigned short ) * indices.Size();
    GDevice->CreateBuffer( bufferCI, indices.ToPtr(), &m_IndexBuffer );

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

    TRef< IShaderModule > vertexShader, geometryShader, fragmentShader;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    AString vertexSource = LoadShader( "gen/envprobegen.vert" );
    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSource.CStr() );
    GShaderSources.Build( VERTEX_SHADER, vertexShader );

    AString geometrySource = LoadShader( "gen/envprobegen.geom" );
    GShaderSources.Clear();
    GShaderSources.Add( geometrySource.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShader );

    AString fragmentSource = LoadShader( "gen/envprobegen.frag" );
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
    GDevice->CreatePipeline( pipelineCI, &m_Pipeline );

    SSamplerCreateInfo samplerCI;
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.bCubemapSeamless = true;
    GDevice->GetOrCreateSampler( samplerCI, &m_Sampler );
}

void AEnvProbeGenerator::GenerateArray( int _MaxLod, int _CubemapsCount, ITexture ** _Cubemaps, TRef< RenderCore::ITexture > * ppTextureArray ) {
    int size = 1 << _MaxLod;

    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.Format = TEX_FORMAT_ENVPROBE;
    textureCI.Resolution.TexCubemapArray.Width = size;
    textureCI.Resolution.TexCubemapArray.NumLayers = _CubemapsCount;
    textureCI.NumLods = _MaxLod + 1;

    GDevice->CreateTexture( textureCI, ppTextureArray );

    SShaderSamplerBinding samplerBinding;
    samplerBinding.SlotIndex = 0;
    samplerBinding.pSampler = m_Sampler;

    SShaderTextureBinding textureBinding;
    textureBinding.SlotIndex = 0;

    SShaderBufferBinding uniformBufferBinding = {};
    uniformBufferBinding.SlotIndex = 0;
    uniformBufferBinding.BufferType = UNIFORM_BUFFER;
    uniformBufferBinding.pBuffer = m_UniformBuffer;

    SShaderResources resources = {};
    resources.Buffers = &uniformBufferBinding;
    resources.NumBuffers = 1;

    resources.Samplers = &samplerBinding;
    resources.NumSamplers = 1;

    resources.Textures = &textureBinding;
    resources.NumTextures = 1;

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    SDrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = m_IndexCount;
    drawCmd.InstanceCount = 6;

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
        rcmd->BindPipeline( m_Pipeline );
        rcmd->BindVertexBuffer( 0, m_VertexBuffer );
        rcmd->BindIndexBuffer( m_IndexBuffer, RenderCore::INDEX_TYPE_UINT16 );

        viewport.Width = lodWidth;
        viewport.Height = lodWidth;

        rcmd->SetViewport( viewport );

        m_UniformBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

        for ( int cubemapIndex = 0 ; cubemapIndex < _CubemapsCount ; cubemapIndex++ ) {

            m_UniformBufferData.Roughness.Y = cubemapIndex * 6; // Offset for cubemap array layer

            m_UniformBuffer->Write( &m_UniformBufferData );

            textureBinding.pTexture = _Cubemaps[cubemapIndex];

            rcmd->BindShaderResources( &resources );

            // Draw six faces in one draw call
            rcmd->Draw( &drawCmd );
        }

        rcmd->EndRenderPass();
    }
}

void AEnvProbeGenerator::Generate( int _MaxLod, ITexture * _SourceCubemap, TRef< RenderCore::ITexture > * ppTexture ) {
    int size = 1 << _MaxLod;

    STextureCreateInfo textureCI = {};
    textureCI.Type = RenderCore::TEXTURE_CUBE_MAP;
    textureCI.Format = TEX_FORMAT_ENVPROBE;
    textureCI.Resolution.TexCubemap.Width = size;
    textureCI.NumLods = _MaxLod + 1;

    GDevice->CreateTexture( textureCI, ppTexture );

    SShaderSamplerBinding samplerBinding;
    samplerBinding.SlotIndex = 0;
    samplerBinding.pSampler = m_Sampler;

    SShaderTextureBinding textureBinding;
    textureBinding.SlotIndex = 0;

    SShaderBufferBinding uniformBufferBinding = {};
    uniformBufferBinding.SlotIndex = 0;
    uniformBufferBinding.BufferType = UNIFORM_BUFFER;
    uniformBufferBinding.pBuffer = m_UniformBuffer;

    SShaderResources resources = {};
    resources.Buffers = &uniformBufferBinding;
    resources.NumBuffers = 1;

    resources.Samplers = &samplerBinding;
    resources.NumSamplers = 1;

    resources.Textures = &textureBinding;
    resources.NumTextures = 1;

    SViewport viewport = {};
    viewport.MaxDepth = 1;

    SDrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = m_IndexCount;
    drawCmd.InstanceCount = 6;

    int lodWidth = size;

    m_UniformBufferData.Roughness.Y = 0; // Offset for cubemap array layer

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
        rcmd->BindPipeline( m_Pipeline );
        rcmd->BindVertexBuffer( 0, m_VertexBuffer );
        rcmd->BindIndexBuffer( m_IndexBuffer, RenderCore::INDEX_TYPE_UINT16 );

        viewport.Width = lodWidth;
        viewport.Height = lodWidth;

        rcmd->SetViewport( viewport );

        m_UniformBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

        m_UniformBuffer->Write( &m_UniformBufferData );

        textureBinding.pTexture = _SourceCubemap;

        rcmd->BindShaderResources( &resources );

        // Draw six faces in one draw call
        rcmd->Draw( &drawCmd );

        rcmd->EndRenderPass();
    }
}
