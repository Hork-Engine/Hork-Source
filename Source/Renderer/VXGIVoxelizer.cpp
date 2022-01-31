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

#include "VXGIVoxelizer.h"
#include "RenderLocal.h"

using namespace RenderCore;

static const TEXTURE_FORMAT TEX_FORMAT_SKY = TEXTURE_FORMAT_RGB16F; // TODO: try compression

#define MAX_MIP_MAP_LEVELS 9
#define MAX_VOXEL_RES 512
// 512 ^ 3
#define MAX_SPARSE_BUFFER_SIZE 134217728
static const int VoxGridSize = 256;
static const int numMipLevels = std::log2( VoxGridSize );

VXGIVoxelizer::VXGIVoxelizer() {
    #if 0
    SFramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = VoxGridSize;
    framebufferCI.Height = VoxGridSize;
    GDevice->CreateFramebuffer( framebufferCI, &voxelFBO );

    GDevice->CreateTexture( MakeTexture( TEXTURE_FORMAT_R32UI, STextureResolution2DArray( VoxGridSize, VoxGridSize, 3 ) ), &voxel2DTex );
    // TODO: Sampler GL_TEXTURE_MIN_FILTER=GL_NEAREST, GL_TEXTURE_MAG_FILTER=GL_NEAREST, GL_TEXTURE_WRAP_S=GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T=GL_CLAMP_TO_EDGE

    GDevice->CreateTexture( MakeTexture( TEXTURE_FORMAT_R32UI, STextureResolution3D( VoxGridSize, VoxGridSize, VoxGridSize ), STextureSwizzle(), numMipLevels + 1 ), &voxelTex );
    // TODO: Sampler GL_TEXTURE_MIN_FILTER=GL_NEAREST_MIPMAP_NEAREST, GL_TEXTURE_MAG_FILTER=GL_NEAREST, GL_TEXTURE_WRAP_S=GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T=GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_R=GL_CLAMP_TO_EDGE
    
    
    DrawElementsIndirectCommand drawIndCmd[10];
    // Initialize the indirect drawing buffer
    for ( int i = MAX_MIP_MAP_LEVELS, j = 0; i >= 0; i--, j++ ) {
        drawIndCmd[i].vertexCount = 32;//(uint32_t)voxelModel->GetNumIndices();
        drawIndCmd[i].instanceCount = 0;
        drawIndCmd[i].firstVertex = 0;
        drawIndCmd[i].baseVertex = 0;

        if ( i == 0 ) {
            drawIndCmd[i].baseInstance = 0;
        } else if ( i == MAX_MIP_MAP_LEVELS ) {
            drawIndCmd[i].baseInstance = MAX_SPARSE_BUFFER_SIZE - 1;
        } else {
            drawIndCmd[i].baseInstance = drawIndCmd[i + 1].baseInstance - (1 << (3 * j));
        }
    }

    // Draw Indirect Command buffer for drawing voxels
    {
    SBufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
    bufferCI.SizeInBytes = sizeof( drawIndCmd );
    GDevice->CreateBuffer( bufferCI, drawIndCmd, &drawIndBuffer );
    //glBindBuffer( GL_DRAW_INDIRECT_BUFFER, drawIndBuffer );
    //glBindBufferBase( GL_SHADER_STORAGE_BUFFER, DRAW_IND, drawIndBuffer );
    }

    // Compute indirect buffer and struct
    ComputeIndirectCommand compIndCmd[10];

    // Initialize the indirect compute buffer
    for ( size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++ ) {
        compIndCmd[i].workGroupSizeX = 0;
        compIndCmd[i].workGroupSizeY = 1;
        compIndCmd[i].workGroupSizeZ = 1;
    }

    {
    SBufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;
    bufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
    bufferCI.SizeInBytes = sizeof( compIndCmd );
    GDevice->CreateBuffer( bufferCI, compIndCmd, &compIndBuffer );
    
    //glBindBuffer( GL_DISPATCH_INDIRECT_BUFFER, compIndBuffer );
    //glBindBufferBase( GL_SHADER_STORAGE_BUFFER, COMPUTE_IND, compIndBuffer );
    }


    //SAttachmentInfo colorAttachment = {};
    //colorAttachment.LoadOp = ATTACHMENT_LOAD_OP_DONT_CARE;

    //SAttachmentRef attachmentRef = {};
    //attachmentRef.Attachment = 0;

    //SSubpassInfo subpassInfo = {};
    //subpassInfo.NumColorAttachments = 1;
    //subpassInfo.pColorAttachmentRefs = &attachmentRef;

    SRenderPassCreateInfo renderPassCI = {};
    //renderPassCI.NumColorAttachments = 1;
    //renderPassCI.pColorAttachments = &colorAttachment;
    //renderPassCI.NumSubpasses = 1;
    //renderPassCI.pSubpasses = &subpassInfo;
    GDevice->CreateRenderPass( renderPassCI, &RP );


    // TODO: Add stage to material
    CreatePipeline();

#if 0
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

    CreateVertexShader( "gen/atmosphere.vert", vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateGeometryShader( "gen/atmosphere.geom", pipelineCI.pGS );
    CreateFragmentShader( "gen/atmosphere.frag", pipelineCI.pFS );

    SBufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.ResourceLayout.NumBuffers = AN_ARRAY_SIZE( buffers );
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline( pipelineCI, &Pipeline );
#endif
    #endif
}

void VXGIVoxelizer::CreatePipeline()
{
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

    CreateVertexShader( "gen/atmosphere.vert", vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateGeometryShader( "gen/atmosphere.geom", pipelineCI.pGS );
    CreateFragmentShader( "gen/atmosphere.frag", pipelineCI.pFS );

    SBufferInfo buffers[1];
    buffers[0].BufferBinding = BUFFER_BIND_CONSTANT;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBindings );
    pipelineCI.pVertexBindings = vertexBindings;
    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.ResourceLayout.NumBuffers = AN_ARRAY_SIZE( buffers );
    pipelineCI.ResourceLayout.Buffers = buffers;

    GDevice->CreatePipeline( pipelineCI, &Pipeline );
}

void VXGIVoxelizer::Render()
{
    #if 0
    RenderCore::IResourceTable * resourceTable = rtbl; // FIXME: create separate resource table?

    // Clear the last voxelization data
    rcmd->ClearTexture( voxel2DTex, 0, FORMAT_UINT1, nullptr ); // FIXME: format must be GL_RED_INTEGER, GL_UNSIGNED_INT
    for ( size_t i = 0; i <= numMipLevels; i++ ) {
        rcmd->ClearTexture( voxelTex, i, FORMAT_UINT1, nullptr ); // FIXME: format must be GL_RED_INTEGER, GL_UNSIGNED_INT
    }

    // Reset the sparse voxel count
    //TODO glBindBuffer( GL_SHADER_STORAGE_BUFFER, drawIndBuffer );
    SBufferClear ranges[MAX_MIP_MAP_LEVELS+1];
    for ( size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++ ) {
        ranges[i].Offset = i * sizeof( DrawElementsIndirectCommand ) + sizeof( uint32_t );
        ranges[i].SizeInBytes = sizeof( uint32_t );
    }
    // Clear data before since data is used when drawing
    rcmd->ClearBufferRange( drawIndBuffer, BUFFER_VIEW_PIXEL_FORMAT_R32UI, AN_ARRAY_SIZE( ranges ), ranges, FORMAT_UINT1, nullptr ); // FIXME: format must be GL_RED, GL_UNSIGNED_INT

    // Reset the sparse voxel count for compute shader
    //TODO glBindBuffer( GL_SHADER_STORAGE_BUFFER, compIndBuffer );
    for ( size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++ ) {
        ranges[i].Offset = i * sizeof( ComputeIndirectCommand );
        ranges[i].SizeInBytes = sizeof( uint32_t );
    }
    // Clear data before since data is used when drawing
    rcmd->ClearBufferRange( compIndBuffer, BUFFER_VIEW_PIXEL_FORMAT_R32UI, AN_ARRAY_SIZE( ranges ), ranges, FORMAT_UINT1, nullptr ); // FIXME: format must be GL_RED, GL_UNSIGNED_INT



    // Bind the textures used to hold the voxelization data
    resourceTable->BindImage( 2, voxel2DTex, 0, true, 0 ); // TODO: set in pipeline AccesMode=GL_READ_WRITE
    resourceTable->BindImage( 3, voxelTex, 0, false, 0 ); // TODO: set in pipeline AccesMode=GL_READ_WRITE


    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = voxelFBO;
    renderPassBegin.pRenderPass = RP;
    renderPassBegin.RenderArea.Width = VoxGridSize;
    renderPassBegin.RenderArea.Height = VoxGridSize;

    rcmd->BeginRenderPass( renderPassBegin );

    SViewport viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = VoxGridSize;
    viewport.Height = VoxGridSize;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    rcmd->SetViewport( viewport );

    rcmd->BindResourceTable( resourceTable );
#if 0
    // All faces must be rendered
    glDisable( GL_CULL_FACE );

    glUseProgram( shaders->voxelize );

    for ( auto model = models->begin(); model != models->end(); model++ ) {

        (*model)->Voxelize();

        rcmd->Barrier( SHADER_IMAGE_ACCESS_BARRIER_BIT );
    }
    rcmd->Barrier( SHADER_STORAGE_BARRIER_BIT );
#endif
    rcmd->EndRenderPass();

#if 0

    GDevice->CreateTexture( MakeTexture( TEX_FORMAT_SKY, STextureResolutionCubemap( CubemapWidth ) ), ppTexture );

    ConstantBufferData.LightDir = Float4( LightDir.Normalized(), 0.0f );

    ConstantBuffer->Write( &ConstantBufferData );

    TRef< IResourceTable > resourceTbl;
    GDevice->CreateResourceTable( &resourceTbl );

    resourceTbl->BindBuffer( 0, ConstantBuffer );

    SViewport viewport = {};
    viewport.Width = CubemapWidth;
    viewport.Height = CubemapWidth;
    viewport.MaxDepth = 1;

    SFramebufferAttachmentInfo attachment = {};
    attachment.pTexture = *ppTexture;
    attachment.LodNum = 0;

    SFramebufferCreateInfo framebufferCI = {};
    framebufferCI.Width = CubemapWidth;
    framebufferCI.Height = CubemapWidth;
    framebufferCI.NumColorAttachments = 1;
    framebufferCI.pColorAttachments = &attachment;

    TRef< RenderCore::IFramebuffer > framebuffer;
    GDevice->CreateFramebuffer( framebufferCI, &framebuffer );

    SRenderPassBegin renderPassBegin = {};
    renderPassBegin.pFramebuffer = framebuffer;
    renderPassBegin.pRenderPass = RP;
    renderPassBegin.RenderArea.Width = CubemapWidth;
    renderPassBegin.RenderArea.Height = CubemapWidth;

    rcmd->BeginRenderPass( renderPassBegin );
    rcmd->SetViewport( viewport );
    rcmd->BindResourceTable( resourceTbl );

    // Draw six faces in one draw call
    DrawSphere( Pipeline, 6 );

    rcmd->EndRenderPass();
#endif
    #endif
}
