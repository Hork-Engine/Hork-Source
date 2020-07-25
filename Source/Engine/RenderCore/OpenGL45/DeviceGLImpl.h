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

#pragma once

#include <RenderCore/Device.h>

namespace RenderCore {

class AImmediateContextGLImpl;

class ADeviceGLImpl final : public IDevice {

    friend class AImmediateContextGLImpl;
    friend class ABufferGLImpl;
    friend class ABufferViewGLImpl;
    friend class ATextureGLImpl;
    friend class AShaderModuleGLImpl;
    friend class APipelineGLImpl;
    friend class AFramebufferGLImpl;
    friend class AQueryPoolGLImpl;
    friend class ATransformFeedbackGLImpl;
    friend class ARenderPassGLImpl;

public:
    ADeviceGLImpl( SImmediateContextCreateInfo const & _CreateInfo,
                   SAllocatorCallback const * _Allocator,
                   HashCallback _Hash );
    ~ADeviceGLImpl();

    void SwapBuffers( SDL_Window * WindowHandle ) override;

    void GetImmediateContext( IImmediateContext ** ppImmediateContext ) override;

    void CreateImmediateContext( SImmediateContextCreateInfo const & _CreateInfo, IImmediateContext ** ppImmediateContext ) override;
    void ReleaseImmediateContext( IImmediateContext * pImmediateContext ) override;

    void CreateFramebuffer( SFramebufferCreateInfo const & _CreateInfo, TRef< IFramebuffer > * ppFramebuffer ) override;

    void CreateRenderPass( SRenderPassCreateInfo const & _CreateInfo, TRef< IRenderPass > * ppRenderPass ) override;

    void CreatePipeline( SPipelineCreateInfo const & _CreateInfo, TRef< IPipeline > * ppPipeline ) override;

    void CreateShaderFromBinary( SShaderBinaryData const * _BinaryData, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule ) override;
    void CreateShaderFromCode( SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule ) override;
    void CreateShaderFromCode( SHADER_TYPE _ShaderType, const char * _Source, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule ) override;

    void CreateBuffer( SBufferCreateInfo const & _CreateInfo, const void * _SysMem, TRef< IBuffer > * ppBuffer ) override;

    void CreateBufferView( SBufferViewCreateInfo const & CreateInfo, TRef< IBuffer > pBuffer, TRef< IBufferView > * ppBufferView ) override;

    void CreateTexture( STextureCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture ) override;

    void CreateTextureView( STextureViewCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture ) override;

    void CreateTransformFeedback( STransformFeedbackCreateInfo const & _CreateInfo, TRef< ITransformFeedback > * ppTransformFeedback ) override;

    void CreateQueryPool( SQueryPoolCreateInfo const & _CreateInfo, TRef< IQueryPool > * ppQueryPool ) override;

    void CreateBindlessSampler( ITexture * pTexture, Sampler Sampler, TRef< IBindlessSampler > * ppBindlessSampler ) override;

    void GetOrCreateSampler( struct SSamplerCreateInfo const & _CreateInfo, Sampler * ppSampler ) override;

    bool CreateShaderBinaryData( SHADER_TYPE _ShaderType,
                                 unsigned int _NumSources,
                                 const char * const * _Sources,
                                 /* optional */ char ** _InfoLog,
                                 SShaderBinaryData * _BinaryData ) override;

    void DestroyShaderBinaryData( SShaderBinaryData * _BinaryData ) override;

    bool IsFeatureSupported( FEATURE_TYPE InFeatureType ) override;

    unsigned int GetDeviceCaps( DEVICE_CAPS InDeviceCaps ) override;


    SAllocatorCallback const & GetAllocator() const;

    unsigned int GetTotalImmediateContexts() const { return TotalContexts; }
    unsigned int GetTotalBuffers() const { return TotalBuffers; }
    unsigned int GetTotalTextures() const { return TotalTextures; }
    unsigned int GetTotalSamplers() const { return SamplerCache.Size(); }
    unsigned int GetTotalBlendingStates() const { return BlendingStateCache.Size(); }
    unsigned int GetTotalRasterizerStates() const { return RasterizerStateCache.Size(); }
    unsigned int GetTotalDepthStencilStates() const { return DepthStencilStateCache.Size(); }
    unsigned int GetTotalShaderModules() const { return TotalShaderModules; }
    unsigned int GetTotalPipelines() const { return TotalPipelines; }
    unsigned int GetTotalRenderPasses() const { return TotalRenderPasses; }
    unsigned int GetTotalFramebuffers() const { return TotalFramebuffers; }
    unsigned int GetTotalTransformFeedbacks() const { return TotalTransformFeedbacks; }
    unsigned int GetTotalQueryPools() const { return TotalQueryPools; }

    unsigned int CreateShaderProgram( unsigned int _Type,
                                int _NumStrings,
                                const char * const * _Strings,
                                char ** _InfoLog = nullptr );

    void DeleteShaderProgram( unsigned int _Program );

private:
    SBlendingStateInfo const * CachedBlendingState( SBlendingStateInfo const & _BlendingState );
    SRasterizerStateInfo const * CachedRasterizerState( SRasterizerStateInfo const & _RasterizerState );
    SDepthStencilStateInfo const * CachedDepthStencilState( SDepthStencilStateInfo const & _DepthStencilState );

    unsigned int DeviceCaps[DEVICE_CAPS_MAX];

    bool FeatureSupport[FEATURE_MAX];

    unsigned int MaxVertexBufferSlots;
    unsigned int MaxVertexAttribStride;
    unsigned int MaxVertexAttribRelativeOffset;
    unsigned int MaxCombinedTextureImageUnits;
    unsigned int MaxImageUnits;
    unsigned int MaxBufferBindings[4]; // uniform buffer, shader storage buffer, transform feedback buffer, atomic counter buffer
    unsigned int MaxTextureAnisotropy;

    unsigned int TotalContexts;
    unsigned int TotalBuffers;
    unsigned int TotalTextures;
    unsigned int TotalShaderModules;
    unsigned int TotalPipelines;
    unsigned int TotalRenderPasses;
    unsigned int TotalFramebuffers;
    unsigned int TotalTransformFeedbacks;
    unsigned int TotalQueryPools;

    size_t BufferMemoryAllocated;
    size_t TextureMemoryAllocated;

    AImmediateContextGLImpl * pMainContext;

    SAllocatorCallback Allocator;
    HashCallback Hash;

    THash<> SamplerHash;
    TPodArray< struct SamplerInfo * > SamplerCache;

    THash<> BlendingHash;
    TPodArray< SBlendingStateInfo * > BlendingStateCache;

    THash<> RasterizerHash;
    TPodArray< SRasterizerStateInfo * > RasterizerStateCache;

    THash<> DepthStencilHash;
    TPodArray< SDepthStencilStateInfo * > DepthStencilStateCache;
};

}
