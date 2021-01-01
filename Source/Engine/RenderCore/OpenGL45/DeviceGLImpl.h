/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

class ADeviceGLImpl final : public IDevice
{
public:
    ADeviceGLImpl( SImmediateContextCreateInfo const & _CreateInfo,
                   SAllocatorCallback const * _Allocator,
                   HashCallback _Hash );
    ~ADeviceGLImpl();

    //
    // IDevice interface
    //

    void SwapBuffers( SDL_Window * WindowHandle, int SwapInterval ) override;

    void GetImmediateContext( IImmediateContext ** ppImmediateContext ) override;

    void CreateImmediateContext( SImmediateContextCreateInfo const & _CreateInfo, IImmediateContext ** ppImmediateContext ) override;
    void ReleaseImmediateContext( IImmediateContext * pImmediateContext ) override;

    void CreateFramebuffer( SFramebufferCreateInfo const & _CreateInfo, TRef< IFramebuffer > * ppFramebuffer ) override;

    void CreateRenderPass( SRenderPassCreateInfo const & _CreateInfo, TRef< IRenderPass > * ppRenderPass ) override;

    void CreatePipeline( SPipelineCreateInfo const & _CreateInfo, TRef< IPipeline > * ppPipeline ) override;

    void CreateShaderFromBinary( SShaderBinaryData const * _BinaryData, TRef< IShaderModule > * ppShaderModule ) override;
    void CreateShaderFromCode( SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, TRef< IShaderModule > * ppShaderModule ) override;

    void CreateBuffer( SBufferCreateInfo const & _CreateInfo, const void * _SysMem, TRef< IBuffer > * ppBuffer ) override;

    void CreateBufferView( SBufferViewCreateInfo const & CreateInfo, TRef< IBuffer > pBuffer, TRef< IBufferView > * ppBufferView ) override;

    void CreateTexture( STextureCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture ) override;

    void CreateTextureView( STextureViewCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture ) override;

    void CreateSparseTexture( SSparseTextureCreateInfo const & _CreateInfo, TRef< ISparseTexture > * ppTexture ) override;

    void CreateTransformFeedback( STransformFeedbackCreateInfo const & _CreateInfo, TRef< ITransformFeedback > * ppTransformFeedback ) override;

    void CreateQueryPool( SQueryPoolCreateInfo const & _CreateInfo, TRef< IQueryPool > * ppQueryPool ) override;

    void GetBindlessSampler( ITexture * pTexture, SSamplerInfo const & _CreateInfo, TRef< IBindlessSampler > * ppBindlessSampler ) override;

    void CreateResourceTable( TRef< IResourceTable > * ppResourceTable ) override;

    bool CreateShaderBinaryData( SHADER_TYPE _ShaderType,
                                 unsigned int _NumSources,
                                 const char * const * _Sources,
                                 SShaderBinaryData * _BinaryData ) override;

    void DestroyShaderBinaryData( SShaderBinaryData * _BinaryData ) override;

    bool IsFeatureSupported( FEATURE_TYPE InFeatureType ) override;

    unsigned int GetDeviceCaps( DEVICE_CAPS InDeviceCaps ) override;

    int32_t GetGPUMemoryTotalAvailable() override;
    int32_t GetGPUMemoryCurrentAvailable() override;

    bool EnumerateSparseTexturePageSize( SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int * NumPageSizes, int * PageSizesX, int * PageSizesY, int * PageSizesZ ) override;

    bool ChooseAppropriateSparseTexturePageSize( SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int * PageSizeIndex, int * PageSizeX = nullptr, int * PageSizeY = nullptr, int * PageSizeZ = nullptr ) override;

    bool LookupImageFormat( const char * _FormatQualifier, TEXTURE_FORMAT * _Format ) override;

    const char * LookupImageFormatQualifier( TEXTURE_FORMAT _Format ) override;

    //
    // Local
    //

    SAllocatorCallback const & GetAllocator() const;

    int Hash( const unsigned char * _Data, int _Size ) const
    {
        return HashCB( _Data, _Size );
    }

    SBlendingStateInfo const * CachedBlendingState( SBlendingStateInfo const & _BlendingState );
    SRasterizerStateInfo const * CachedRasterizerState( SRasterizerStateInfo const & _RasterizerState );
    SDepthStencilStateInfo const * CachedDepthStencilState( SDepthStencilStateInfo const & _DepthStencilState );
    unsigned int CachedSampler( SSamplerInfo const & _CreateInfo );

    // Statistic
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

private:
    AImmediateContextGLImpl * pMainContext;

    SAllocatorCallback Allocator;
    HashCallback HashCB;

    THash<> SamplerHash;
    TPodArray< struct SamplerInfo * > SamplerCache;

    THash<> BlendingHash;
    TPodArray< SBlendingStateInfo * > BlendingStateCache;

    THash<> RasterizerHash;
    TPodArray< SRasterizerStateInfo * > RasterizerStateCache;

    THash<> DepthStencilHash;
    TPodArray< SDepthStencilStateInfo * > DepthStencilStateCache;

    unsigned int DeviceCaps[DEVICE_CAPS_MAX];

    bool FeatureSupport[FEATURE_MAX];

    int SwapInterval;
};

}
