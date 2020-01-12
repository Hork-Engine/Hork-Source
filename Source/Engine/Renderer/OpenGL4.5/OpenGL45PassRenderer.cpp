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

#include "OpenGL45PassRenderer.h"
#include "OpenGL45FrameResources.h"

using namespace GHI;

namespace OpenGL45 {

void APassRenderer::BindTextures( SMaterialFrameData * _MaterialInstance ) {
    AN_ASSERT( _MaterialInstance );

    ATextureGPU ** texture = _MaterialInstance->Textures;

    int n = _MaterialInstance->NumTextures;
    if ( n > _MaterialInstance->Material->NumSamplers ) {
        n = _MaterialInstance->Material->NumSamplers;
    }

    for (int t = 0 ; t < n ; t++, texture++ ) {
        if ( *texture ) {
            ATextureGPU * texProxy = *texture;
            GFrameResources.TextureBindings[t].pTexture = GPUTextureHandle( texProxy );
        } else {
            GFrameResources.TextureBindings[t].pTexture = nullptr;
        }
    }

    //for (int t = n ; t<MAX_MATERIAL_TEXTURES ; t++ ) {
    //    TextureBindings[t].pTexture = nullptr;
    //}
}

void APassRenderer::BindVertexAndIndexBuffers( SRenderInstance const * _Instance ) {
    Buffer * pVertexBuffer = GPUBufferHandle( _Instance->VertexBuffer );
    Buffer * pIndexBuffer = GPUBufferHandle( _Instance->IndexBuffer );

    AN_ASSERT( pVertexBuffer );
    AN_ASSERT( pIndexBuffer );

    Cmd.BindVertexBuffer( 0, pVertexBuffer, 0 );
    Cmd.BindIndexBuffer( pIndexBuffer, INDEX_TYPE_UINT32, 0 );
}

void APassRenderer::BindVertexAndIndexBuffers( SShadowRenderInstance const * _Instance ) {
    Buffer * pVertexBuffer = GPUBufferHandle( _Instance->VertexBuffer );
    Buffer * pIndexBuffer = GPUBufferHandle( _Instance->IndexBuffer );

    AN_ASSERT( pVertexBuffer );
    AN_ASSERT( pIndexBuffer );

    Cmd.BindVertexBuffer( 0, pVertexBuffer, 0 );
    Cmd.BindIndexBuffer( pIndexBuffer, INDEX_TYPE_UINT32, 0 );
}

void APassRenderer::BindSkeleton( size_t _Offset, size_t _Size ) {
    GFrameResources.SkeletonBufferBinding->BindingOffset = _Offset;
    GFrameResources.SkeletonBufferBinding->BindingSize = _Size;
}

void APassRenderer::SetInstanceUniforms( int _Index ) {
    GFrameResources.InstanceUniformBufferBinding->pBuffer = &GFrameResources.InstanceUniformBuffer;
    GFrameResources.InstanceUniformBufferBinding->BindingOffset = _Index * InstanceUniformBufferSizeof;
    GFrameResources.InstanceUniformBufferBinding->BindingSize = sizeof( SInstanceUniformBuffer );
}

void APassRenderer::SetShadowInstanceUniforms( int _Index ) {
    GFrameResources.InstanceUniformBufferBinding->pBuffer = &GFrameResources.ShadowInstanceUniformBuffer;
    GFrameResources.InstanceUniformBufferBinding->BindingOffset = _Index * ShadowInstanceUniformBufferSizeof;
    GFrameResources.InstanceUniformBufferBinding->BindingSize = sizeof( SShadowInstanceUniformBuffer );
}

}
