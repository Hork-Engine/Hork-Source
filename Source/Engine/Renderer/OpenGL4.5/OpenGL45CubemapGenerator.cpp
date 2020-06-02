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

#include "OpenGL45CubemapGenerator.h"

#include <Core/Public/PodArray.h>

using namespace GHI;

namespace OpenGL45 {

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

void ACubemapGenerator::Initialize() {
    TPodArray< Float3 > vertices;
    TPodArray< unsigned short > indices;

    CreateSphere( 128, 128, vertices, indices );

    m_IndexCount = indices.Size();

    BufferCreateInfo bufferCI = {};
    bufferCI.bImmutableStorage = true;

    bufferCI.SizeInBytes = sizeof( Float3 ) * vertices.Size();
    m_VertexBuffer.Initialize( bufferCI, vertices.ToPtr() );

    bufferCI.SizeInBytes = sizeof( unsigned short ) * indices.Size();
    m_IndexBuffer.Initialize( bufferCI, indices.ToPtr() );

    bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
    bufferCI.SizeInBytes = sizeof( SCubemapGeneratorUniformBuffer );
    m_UniformBuffer.Initialize( bufferCI );

    Float4x4 const * cubeFaceMatrices = Float4x4::GetCubeFaceMatrices();
    Float4x4 projMat = Float4x4::PerspectiveRevCC( Math::_HALF_PI, 1.0f, 1.0f, 0.1f, 100.0f );

    for ( int faceIndex = 0 ; faceIndex < 6 ; faceIndex++ ) {
        m_UniformBufferData.Transform[faceIndex] = projMat * cubeFaceMatrices[faceIndex];
    }

    AttachmentInfo colorAttachment = {};
    colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;

    AttachmentRef attachmentRef = {};
    attachmentRef.Attachment = 0;

    SubpassInfo subpassInfo = {};
    subpassInfo.NumColorAttachments = 1;
    subpassInfo.pColorAttachmentRefs = &attachmentRef;

    RenderPassCreateInfo renderPassCI = {};
    renderPassCI.NumColorAttachments = 1;
    renderPassCI.pColorAttachments = &colorAttachment;
    renderPassCI.NumSubpasses = 1;
    renderPassCI.pSubpasses = &subpassInfo;
    m_RP.Initialize( renderPassCI );

    PipelineInputAssemblyInfo ia = {};
    ia.Topology = PRIMITIVE_TRIANGLES;

    BlendingStateInfo blending = {};
    blending.SetDefaults();

    RasterizerStateInfo rasterizer = {};
    rasterizer.SetDefaults();

    DepthStencilStateInfo depthStencil = {};
    depthStencil.SetDefaults();
    depthStencil.bDepthEnable = false;
    depthStencil.DepthWriteMask = DEPTH_WRITE_DISABLE;

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

    ShaderModule vertexShader, geometryShader, fragmentShader;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    AString vertexSource = LoadShader( "gen/cubemapgen.vert" );
    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSource.CStr() );
    GShaderSources.Build( VERTEX_SHADER, &vertexShader );

    AString geometrySource = LoadShader( "gen/cubemapgen.geom" );
    GShaderSources.Clear();
    GShaderSources.Add( geometrySource.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, &geometryShader );

    AString fragmentSource = LoadShader( "gen/cubemapgen.frag" );
    GShaderSources.Clear();
    GShaderSources.Add( fragmentSource.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, &fragmentShader );

    ShaderStageInfo stages[] =
    {
        {
            SHADER_STAGE_VERTEX_BIT,
            &vertexShader
        },
        {
            SHADER_STAGE_GEOMETRY_BIT,
            &geometryShader
        },
        {
            SHADER_STAGE_FRAGMENT_BIT,
            &fragmentShader
        }
    };

    PipelineCreateInfo pipelineCI;
    pipelineCI.pInputAssembly = &ia;
    pipelineCI.pBlending = &blending;
    pipelineCI.pRasterizer = &rasterizer;
    pipelineCI.pDepthStencil = &depthStencil;
    pipelineCI.NumStages = AN_ARRAY_SIZE( stages );
    pipelineCI.pStages = stages;
    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;
    //pipelineCI.pRenderPass = &m_RP;
    m_Pipeline.Initialize( pipelineCI );

    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    //samplerCI.bCubemapSeamless = true;
    m_Sampler = GDevice.GetOrCreateSampler( samplerCI );
}

void ACubemapGenerator::Deinitialize() {
    m_VertexBuffer.Deinitialize();
    m_IndexBuffer.Deinitialize();
    m_UniformBuffer.Deinitialize();
    m_Pipeline.Deinitialize();
    m_RP.Deinitialize();
}

void ACubemapGenerator::GenerateArray( Texture & _CubemapArray, GHI::INTERNAL_PIXEL_FORMAT _Format, int _Resolution, int _SourcesCount, Texture ** _Sources ) {
    TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.InternalFormat = _Format;
    textureCI.Resolution.TexCubemapArray.Width = _Resolution;
    textureCI.Resolution.TexCubemapArray.NumLayers = _SourcesCount;
    textureCI.NumLods = 1;
    _CubemapArray.InitializeStorage( textureCI );

    ShaderSamplerBinding samplerBinding;
    samplerBinding.SlotIndex = 0;
    samplerBinding.pSampler = m_Sampler;

    ShaderTextureBinding textureBinding;
    textureBinding.SlotIndex = 0;

    ShaderBufferBinding uniformBufferBinding = {};
    uniformBufferBinding.SlotIndex = 0;
    uniformBufferBinding.BufferType = UNIFORM_BUFFER;
    uniformBufferBinding.pBuffer = &m_UniformBuffer;

    ShaderResources resources = {};
    resources.Buffers = &uniformBufferBinding;
    resources.NumBuffers = 1;

    resources.Samplers = &samplerBinding;
    resources.NumSamplers = 1;

    resources.Textures = &textureBinding;
    resources.NumTextures = 1;

    Viewport viewport = {};
    viewport.MaxDepth = 1;

    DrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = m_IndexCount;
    drawCmd.InstanceCount = 6;

    GHI::Framebuffer framebuffer;

    FramebufferAttachmentInfo attachment = {};
    attachment.pTexture = &_CubemapArray;
    attachment.LodNum = 0;

    FramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = _Resolution;
    framebufferCI.Height = _Resolution;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    framebuffer.Initialize( framebufferCI );

    RenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = &framebuffer;
    renderPassBegin.pRenderPass = &m_RP;
    renderPassBegin.RenderArea.Width = _Resolution;
    renderPassBegin.RenderArea.Height = _Resolution;

    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.BindPipeline( &m_Pipeline );
    Cmd.BindVertexBuffer( 0, &m_VertexBuffer );
    Cmd.BindIndexBuffer( &m_IndexBuffer, GHI::INDEX_TYPE_UINT16 );

    viewport.Width = _Resolution;
    viewport.Height = _Resolution;

    Cmd.SetViewport( viewport );

    for ( int sourceIndex = 0 ; sourceIndex < _SourcesCount ; sourceIndex++ ) {

        m_UniformBufferData.Index.X = sourceIndex * 6; // Offset for cubemap array layer

        m_UniformBuffer.Write( &m_UniformBufferData );

        textureBinding.pTexture = _Sources[sourceIndex];

        Cmd.BindShaderResources( &resources );

        // Draw six faces in one draw call
        Cmd.Draw( &drawCmd );
    }

    Cmd.EndRenderPass();
}

void ACubemapGenerator::Generate( Texture & _Cubemap, GHI::INTERNAL_PIXEL_FORMAT _Format, int _Resolution, Texture * _Source ) {
    TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_CUBE_MAP;
    textureCI.InternalFormat = _Format;
    textureCI.Resolution.TexCubemap.Width = _Resolution;
    textureCI.NumLods = 1;
    _Cubemap.InitializeStorage( textureCI );

    ShaderSamplerBinding samplerBinding;
    samplerBinding.SlotIndex = 0;
    samplerBinding.pSampler = m_Sampler;

    ShaderTextureBinding textureBinding;
    textureBinding.SlotIndex = 0;

    ShaderBufferBinding uniformBufferBinding = {};
    uniformBufferBinding.SlotIndex = 0;
    uniformBufferBinding.BufferType = UNIFORM_BUFFER;
    uniformBufferBinding.pBuffer = &m_UniformBuffer;

    ShaderResources resources = {};
    resources.Buffers = &uniformBufferBinding;
    resources.NumBuffers = 1;

    resources.Samplers = &samplerBinding;
    resources.NumSamplers = 1;

    resources.Textures = &textureBinding;
    resources.NumTextures = 1;

    Viewport viewport = {};
    viewport.MaxDepth = 1;

    DrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = m_IndexCount;
    drawCmd.InstanceCount = 6;

    GHI::Framebuffer framebuffer;

    FramebufferAttachmentInfo attachment = {};
    attachment.pTexture = &_Cubemap;
    attachment.LodNum = 0;

    FramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = _Resolution;
    framebufferCI.Height = _Resolution;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    framebuffer.Initialize( framebufferCI );

    RenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = &framebuffer;
    renderPassBegin.pRenderPass = &m_RP;
    renderPassBegin.RenderArea.Width = _Resolution;
    renderPassBegin.RenderArea.Height = _Resolution;

    Cmd.BeginRenderPass( renderPassBegin );
    Cmd.BindPipeline( &m_Pipeline );
    Cmd.BindVertexBuffer( 0, &m_VertexBuffer );
    Cmd.BindIndexBuffer( &m_IndexBuffer, GHI::INDEX_TYPE_UINT16 );

    viewport.Width = _Resolution;
    viewport.Height = _Resolution;

    Cmd.SetViewport( viewport );

    m_UniformBufferData.Index.X = 0;

    m_UniformBuffer.Write( &m_UniformBufferData );

    textureBinding.pTexture = _Source;

    Cmd.BindShaderResources( &resources );

    // Draw six faces in one draw call
    Cmd.Draw( &drawCmd );

    Cmd.EndRenderPass();
}

}
