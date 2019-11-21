/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "OpenGL45EnvProbeGenerator.h"

#include <Core/Public/PodArray.h>

using namespace GHI;

namespace OpenGL45 {

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
        Math::RadSinCos( a1, r, y );
        for ( j = 0, a2 = 0; j < _VDiv; j++ ) {
            float s, c;
            Math::RadSinCos( a2, s, c );
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

struct SRoughnessMapVertex {
    Float3 Position;
};

static const INTERNAL_PIXEL_FORMAT ENVPROBE_IPF = INTERNAL_PIXEL_FORMAT_RGB32F; // FIXME: is RGB16F enough? // TODO: try RGB16F and compression!!

void AEnvProbeGenerator::Initialize() {
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
    bufferCI.SizeInBytes = sizeof( SRoughnessUniformBuffer );
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
            sizeof( SRoughnessMapVertex ),  // vertex stride
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

    char * infoLog = nullptr;

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    AString vertexSource =
        "#version 450\n"
        +vertexAttribsShaderString+
        "out gl_PerVertex\n"
        "{\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "layout( location = 0 ) out vec3 VS_Normal;\n"
        "layout( location = 1 ) out flat int VS_InstanceID;\n"
        "layout( binding = 0, std140 ) uniform UniformBlock\n"
        "{\n"
        "    mat4 uTransform[6];\n"
        "    vec4 uRoughness;\n"
        "};\n"
        "void main() {\n"
        "    gl_Position = uTransform[gl_InstanceID % 6] * vec4( InPosition, 1.0 );\n"
        "    VS_Normal = InPosition;\n"
        "    VS_InstanceID = gl_InstanceID + int(uRoughness.y);\n"
        "}\n";

    const char * geometrySource =
        "#version 450\n"
        "in gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "} gl_in[];\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "layout(triangles) in;\n"
        "layout(triangle_strip, max_vertices = 3) out;\n"
        "layout( location = 0 ) out vec3 GS_Normal;\n"
        "layout( location = 0 ) in vec3 VS_Normal[];\n"
        "layout( location = 1 ) in flat int VS_InstanceID[];\n"
        "void main() {\n"
        "    gl_Layer = VS_InstanceID[0];\n"  // FIXME: check gl_InvocationID
        "    gl_Position = gl_in[ 0 ].gl_Position;\n"
        "    GS_Normal = VS_Normal[ 0 ];\n"
        "    EmitVertex();\n"
        "    gl_Position = gl_in[ 1 ].gl_Position;\n"
        "    GS_Normal = VS_Normal[ 1 ];\n"
        "    EmitVertex();\n"
        "    gl_Position = gl_in[ 2 ].gl_Position;\n"
        "    GS_Normal = VS_Normal[ 2 ];\n"
        "    EmitVertex();\n"
        "    EndPrimitive();\n"
        "}\n";

    const char * fragmentSource =
        "#version 450\n"
        "layout( location = 0 ) in vec3 GS_Normal;\n"
        "layout( location = 0 ) out vec4 FS_FragColor; \n"
        "layout( binding = 0 ) uniform samplerCube Envmap;\n" // Envmap must be HDRI in linear space
        "layout( binding = 0, std140 ) uniform UniformBlock\n"
        "{\n"
        "    mat4 uTransform[6];\n"
        "    vec4 uRoughness;\n"
        "};\n"

        "vec2 Hammersley( int k, int n ) {\n"
        "    float u = 0;\n"
        "    float p = 0.5;\n"
        "    for ( int kk = k; bool( kk ); p *= 0.5f, kk /= 2 ) {\n"
        "       if ( bool( kk & 1 ) ) {\n"
        "           u += p;\n"
        "       }\n"
        "    }\n"
        "    float x = u;\n"
        "    float y = (k + 0.5f) / n;\n"
        "    return vec2( x, y );\n"
        "}\n"

        "float radicalInverse_VdC( uint bits ) {\n"
        "    bits = (bits << 16u) | (bits >> 16u);\n"
        "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
        "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
        "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
        "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
        "    return float( bits ) * 2.3283064365386963e-10; // / 0x100000000\n"
        "}\n"

        "vec2 hammersley2d( uint i, uint N ) {\n"
        "    return vec2( float( i )/float( N ), radicalInverse_VdC( i ) );\n"
        "}\n"

        "const float PI = 3.1415926;\n"

        "vec3 ImportanceSampleGGX( vec2 Xi, float Roughness, vec3 N ) {\n"
        "    float a = Roughness * Roughness;\n"
        "    float Phi = 2 * PI * Xi.x;\n"
        "    float CosTheta = sqrt( (1 - Xi.y) / (1 + (a*a - 1) * Xi.y) );\n"
        "    float SinTheta = sqrt( 1 - CosTheta * CosTheta );\n"
        "    vec3 H;\n"
        "    H.x = SinTheta * cos( Phi );\n"
        "    H.y = SinTheta * sin( Phi );\n"
        "    H.z = CosTheta;\n"
        "    vec3 UpVector = abs( N.z ) < 0.99 ? vec3( 0, 0, 1 ) : vec3( 1, 0, 0 );\n"
        "    //vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);\n"
        "    vec3 TangentX = normalize( cross( UpVector, N ) );\n"
        "    vec3 TangentY = cross( N, TangentX );\n"
        // Tangent to world space
        "    return TangentX * H.x + TangentY * H.y + N * H.z;\n"
        "}\n"

        "vec3 PrefilterEnvMap( float Roughness, vec3 R ) {\n"
        "    vec3 PrefilteredColor = vec3( 0.0 );\n"
        "    float TotalWeight = 0.0;\n"
        "    const int NumSamples = 1024;\n"
        "    for ( int i = 0; i < NumSamples; i++ ) {\n"
        "        vec2 Xi = Hammersley( i, NumSamples );\n"
        "        vec3 H = ImportanceSampleGGX( Xi, Roughness, R );\n"
        "        vec3 L = 2 * dot( R, H ) * H - R;\n"
        "        float NoL = clamp( dot( R, L ), 0.0, 1.0 );\n"
        "        if ( NoL > 0 ) {\n"
        "            PrefilteredColor += textureLod( Envmap, L, 0 ).rgb * NoL;\n"
        "            TotalWeight += NoL;\n"
        "        }\n"
        "    }\n"
        "    return PrefilteredColor / TotalWeight;\n"
        "}\n"

        "void main() {\n"
        "    FS_FragColor = vec4( PrefilterEnvMap( uRoughness.x, normalize( GS_Normal ) ), 1.0 );\n"
        "}\n";

    ShaderModule vertexShader, geometryShader, fragmentShader;
    vertexShader.InitializeFromCode( VERTEX_SHADER, vertexSource.CStr(), &infoLog );
    geometryShader.InitializeFromCode( GEOMETRY_SHADER, geometrySource, &infoLog );
    fragmentShader.InitializeFromCode( FRAGMENT_SHADER, fragmentSource, &infoLog );

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
    pipelineCI.pRenderPass = &m_RP;
    pipelineCI.Subpass = 0;
    m_Pipeline.Initialize( pipelineCI );

    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_LINEAR;
    samplerCI.bCubemapSeamless = true;
    m_Sampler = GDevice.GetOrCreateSampler( samplerCI );
}

void AEnvProbeGenerator::Deinitialize() {
    m_VertexBuffer.Deinitialize();
    m_IndexBuffer.Deinitialize();
    m_UniformBuffer.Deinitialize();
    m_Pipeline.Deinitialize();
    m_RP.Deinitialize();
}

void AEnvProbeGenerator::GenerateArray( Texture & _CubemapArray, int _MaxLod, int _CubemapsCount, Texture ** _Cubemaps ) {
    int size = 1 << _MaxLod;

    TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_CUBE_MAP_ARRAY;
    textureCI.InternalFormat = ENVPROBE_IPF;
    textureCI.Resolution.TexCubemapArray.Width = size;
    textureCI.Resolution.TexCubemapArray.NumLayers = _CubemapsCount;
    textureCI.NumLods = _MaxLod + 1;
    _CubemapArray.InitializeStorage( textureCI );

    TPodArray< GHI::Framebuffer > framebuffers;
    framebuffers.Resize( textureCI.NumLods );
    for ( int i = 0 ; i < framebuffers.Size() ; i++ ) {
        int lodWidth = size >> i;

        FramebufferAttachmentInfo attachment = {};
        attachment.pTexture = &_CubemapArray;
        attachment.LodNum = i;

        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = lodWidth;
        framebufferCI.Height = lodWidth;
        framebufferCI.NumColorAttachments = 1;
        framebufferCI.pColorAttachments = &attachment;

        framebuffers[i].Initialize( framebufferCI );
    }

    ShaderSamplerBinding samplerBinding;
    samplerBinding.SlotIndex = 0;
    samplerBinding.pSampler = &m_Sampler;

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

    int lodWidth = size;

    for ( int Lod = 0 ; lodWidth >= 1 ; Lod++, lodWidth >>= 1 ) {

        RenderPassBegin renderPassBegin = {};
        renderPassBegin.pFramebuffer = &framebuffers[Lod];
        renderPassBegin.pRenderPass = &m_RP;
        renderPassBegin.RenderArea.Width = lodWidth;
        renderPassBegin.RenderArea.Height = lodWidth;

        Cmd.BeginRenderPass( renderPassBegin );
        Cmd.BindPipeline( &m_Pipeline );
        Cmd.BindVertexBuffer( 0, &m_VertexBuffer );
        Cmd.BindIndexBuffer( &m_IndexBuffer, GHI::INDEX_TYPE_UINT16 );

        viewport.Width = lodWidth;
        viewport.Height = lodWidth;

        Cmd.SetViewport( viewport );

        m_UniformBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

        for ( int cubemapIndex = 0 ; cubemapIndex < _CubemapsCount ; cubemapIndex++ ) {

            m_UniformBufferData.Roughness.Y = cubemapIndex * 6; // Offset for cubemap array layer

            m_UniformBuffer.Write( &m_UniformBufferData );

            textureBinding.pTexture = _Cubemaps[cubemapIndex];

            Cmd.BindShaderResources( &resources );

            // Draw six faces in one draw call
            Cmd.Draw( &drawCmd );
        }

        Cmd.EndRenderPass();
    }
}

void AEnvProbeGenerator::Generate( Texture & _Cubemap, int _MaxLod, Texture * _SourceCubemap ) {
    int size = 1 << _MaxLod;

    TextureStorageCreateInfo textureCI = {};
    textureCI.Type = GHI::TEXTURE_CUBE_MAP;
    textureCI.InternalFormat = ENVPROBE_IPF;
    textureCI.Resolution.TexCubemap.Width = size;
    textureCI.NumLods = _MaxLod + 1;
    _Cubemap.InitializeStorage( textureCI );

    TPodArray< GHI::Framebuffer > framebuffers;
    framebuffers.Resize( textureCI.NumLods );
    for ( int i = 0 ; i < framebuffers.Size() ; i++ ) {
        int lodWidth = size >> i;

        FramebufferAttachmentInfo attachment = {};
        attachment.pTexture = &_Cubemap;
        attachment.LodNum = i;

        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = lodWidth;
        framebufferCI.Height = lodWidth;
        framebufferCI.NumColorAttachments = 1;
        framebufferCI.pColorAttachments = &attachment;

        framebuffers[i].Initialize( framebufferCI );
    }

    ShaderSamplerBinding samplerBinding;
    samplerBinding.SlotIndex = 0;
    samplerBinding.pSampler = &m_Sampler;

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

    int lodWidth = size;

    m_UniformBufferData.Roughness.Y = 0; // Offset for cubemap array layer

    for ( int Lod = 0 ; lodWidth >= 1 ; Lod++, lodWidth >>= 1 ) {

        RenderPassBegin renderPassBegin = {};
        renderPassBegin.pFramebuffer = &framebuffers[Lod];
        renderPassBegin.pRenderPass = &m_RP;
        renderPassBegin.RenderArea.Width = lodWidth;
        renderPassBegin.RenderArea.Height = lodWidth;

        Cmd.BeginRenderPass( renderPassBegin );
        Cmd.BindPipeline( &m_Pipeline );
        Cmd.BindVertexBuffer( 0, &m_VertexBuffer );
        Cmd.BindIndexBuffer( &m_IndexBuffer, GHI::INDEX_TYPE_UINT16 );

        viewport.Width = lodWidth;
        viewport.Height = lodWidth;

        Cmd.SetViewport( viewport );

        m_UniformBufferData.Roughness.X = static_cast<float>(Lod) / _MaxLod;

        m_UniformBuffer.Write( &m_UniformBufferData );

        textureBinding.pTexture = _SourceCubemap;

        Cmd.BindShaderResources( &resources );

        // Draw six faces in one draw call
        Cmd.Draw( &drawCmd );

        Cmd.EndRenderPass();
    }
}

}
