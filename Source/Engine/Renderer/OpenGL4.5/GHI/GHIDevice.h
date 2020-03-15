/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#pragma once

#include "GHIBasic.h"
#include "GHIPipeline.h"

#include <Core/Public/Hash.h>
#include <Core/Public/PodArray.h>

namespace GHI {

class State;

using Sampler = void*;

/// Logical device
class Device final : public NonCopyable, IObjectInterface {

    friend class State;
    friend class Buffer;
    friend class Texture;
    //friend class Sampler;
    friend class ShaderModule;
    friend class CommandBuffer;
    friend class Pipeline;

public:
    Device();
    ~Device();

    void Initialize( AllocatorCallback const * _Allocator = nullptr, HashCallback _Hash = nullptr );
    void Deinitialize();

    bool IsHalfFloatVertexSupported() const { return bHalfFloatVertexSupported; }
    bool IsHalfFloatPixelSupported() const { return bHalfFloatPixelSupported; }
    bool IsTextureCompressionS3tcSupported() const { return bTextureCompressionS3tcSupported; }
    bool IsTextureAnisotropySupported() const { return bTextureAnisotropySupported; }

    AllocatorCallback const & GetAllocator() const { return Allocator; }

    Sampler const GetOrCreateSampler( struct SamplerCreateInfo const & _CreateInfo );

    unsigned int GetTextureBufferOffsetAlignment() const { return TextureBufferOffsetAlignment; }
    unsigned int GetUniformBufferOffsetAlignment() const { return UniformBufferOffsetAlignment; }

    unsigned int GetTotalStates() const { return TotalStates; }
    unsigned int GetTotalBuffers() const { return TotalBuffers; }
    unsigned int GetTotalTextures() const { return TotalTextures; }
    unsigned int GetTotalSamplers() const { return SamplerCache.Size(); }
    unsigned int GetTotalBlendingStates() const { return BlendingStateCache.Size(); }
    unsigned int GetTotalRasterizerStates() const { return RasterizerStateCache.Size(); }
    unsigned int GetTotalDepthStencilStates() const { return DepthStencilStateCache.Size(); }
    unsigned int GetTotalShaderModules() const { return TotalShaderModules; }

private:
    BlendingStateInfo const * CachedBlendingState( BlendingStateInfo const & _BlendingState );
    RasterizerStateInfo const * CachedRasterizerState( RasterizerStateInfo const & _RasterizerState );
    DepthStencilStateInfo const * CachedDepthStencilState( DepthStencilStateInfo const & _DepthStencilState );
    uint32_t GenerateUID() { return ++UIDGen; }

    bool bHalfFloatVertexSupported;
    bool bHalfFloatPixelSupported;
    bool bTextureCompressionS3tcSupported;
    bool bTextureAnisotropySupported;

    unsigned int MaxVertexBufferSlots;
    unsigned int MaxVertexAttribStride;
    unsigned int MaxVertexAttribRelativeOffset;
    unsigned int MaxCombinedTextureImageUnits;
    unsigned int MaxImageUnits;
    unsigned int MaxTextureBufferSize;
    unsigned int TextureBufferOffsetAlignment;
    unsigned int UniformBufferOffsetAlignment;
    unsigned int ShaderStorageBufferOffsetAlignment;
    unsigned int MaxBufferBindings[4]; // uniform buffer, shader storage buffer, transform feedback buffer, atomic counter buffer
    unsigned int MaxTextureAnisotropy;

    // TODO: atomics:
    unsigned int TotalStates;
    unsigned int TotalBuffers;
    unsigned int TotalTextures;
    unsigned int TotalShaderModules;
    size_t BufferMemoryAllocated;
    size_t TextureMemoryAllocated;
    uint32_t UIDGen;

    AllocatorCallback Allocator;
    HashCallback Hash;

    THash<> SamplerHash;
    TPodArray< struct SamplerInfo * > SamplerCache;

    THash<> BlendingHash;
    TPodArray< BlendingStateInfo * > BlendingStateCache;

    THash<> RasterizerHash;
    TPodArray< RasterizerStateInfo * > RasterizerStateCache;

    THash<> DepthStencilHash;
    TPodArray< DepthStencilStateInfo * > DepthStencilStateCache;
};



using ghi_sampler_t = Sampler;

typedef struct ghi_device_s {
    bool bHalfFloatVertexSupported;
    bool bHalfFloatPixelSupported;
    bool bTextureCompressionS3tcSupported;
    bool bTextureAnisotropySupported;

    unsigned int MaxVertexBufferSlots;
    unsigned int MaxVertexAttribStride;
    unsigned int MaxVertexAttribRelativeOffset;
    unsigned int MaxCombinedTextureImageUnits;
    unsigned int MaxImageUnits;
    unsigned int MaxTextureBufferSize;
    unsigned int TextureBufferOffsetAlignment;
    unsigned int UniformBufferOffsetAlignment;
    unsigned int ShaderStorageBufferOffsetAlignment;
    unsigned int MaxBufferBindings[4]; // uniform buffer, shader storage buffer, transform feedback buffer, atomic counter buffer
    unsigned int MaxTextureAnisotropy;

    // TODO: atomics:
    unsigned int TotalStates;
    unsigned int TotalBuffers;
    unsigned int TotalTextures;
    unsigned int TotalShaderModules;
    size_t BufferMemoryAllocated;
    size_t TextureMemoryAllocated;
    uint32_t UIDGen;

    AllocatorCallback Allocator;
    HashCallback Hash;

    THash<> SamplerHash;
    TPodArray< struct SamplerInfo * > SamplerCache;

    THash<> BlendingHash;
    TPodArray< BlendingStateInfo * > BlendingStateCache;

    THash<> RasterizerHash;
    TPodArray< RasterizerStateInfo * > RasterizerStateCache;

    THash<> DepthStencilHash;
    TPodArray< DepthStencilStateInfo * > DepthStencilStateCache;
} ghi_device_t;


void ghi_create_device( ghi_device_t * device, AllocatorCallback const * _Allocator = nullptr, HashCallback _Hash = nullptr );
void ghi_destroy_device( ghi_device_t * device );

ghi_sampler_t const ghi_device_get_sampler( ghi_device_t * device, struct SamplerCreateInfo const & _CreateInfo );

BlendingStateInfo const * ghi_internal_CachedBlendingState( ghi_device_t * device, BlendingStateInfo const & _BlendingState );

RasterizerStateInfo const * ghi_internal_CachedRasterizerState( ghi_device_t * device, RasterizerStateInfo const & _RasterizerState );

DepthStencilStateInfo const * ghi_internal_CachedDepthStencilState( ghi_device_t * device, DepthStencilStateInfo const & _DepthStencilState );

uint32_t ghi_internal_GenerateUID( ghi_device_t * device );

}
