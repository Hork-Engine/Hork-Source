/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#pragma once

#include "GHIBasic.h"
#include "GHIPipeline.h"

#include <Engine/Core/Public/Hash.h>
#include <Engine/Core/Public/PodArray.h>

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
    unsigned int MaxBufferBindings[4]; // uniform buffer, shader storage buffer, transform feedback buffer, atomic counter buffer
    unsigned int MaxTextureAnisotropy;

    // TODO: atomics:
    unsigned int TotalStates;
    unsigned int TotalBuffers;
    unsigned int TotalTextures;
    unsigned int TotalSamplers;
    unsigned int TotalShaderModules;
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

}
