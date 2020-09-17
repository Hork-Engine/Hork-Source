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

#include "ImmediateContext.h"
#include "BufferView.h"
#include "Texture.h"
#include "SparseTexture.h"
#include "Sampler.h"
#include "Query.h"
#include "Pipeline.h"
#include "ShaderModule.h"
#include "TransformFeedback.h"
#include "Framebuffer.h"
#include "RenderPass.h"

#include <Core/Public/Hash.h>
#include <Core/Public/PodArray.h>
#include <Core/Public/Ref.h>

namespace RenderCore {

enum FEATURE_TYPE
{
    FEATURE_HALF_FLOAT_VERTEX,
    FEATURE_HALF_FLOAT_PIXEL,
    FEATURE_TEXTURE_ANISOTROPY,
    FEATURE_SPARSE_TEXTURES,
    FEATURE_BINDLESS_TEXTURE,
    FEATURE_SWAP_CONTROL,
    FEATURE_SWAP_CONTROL_TEAR,
    FEATURE_GPU_MEMORY_INFO,

    FEATURE_MAX
};

enum DEVICE_CAPS
{
    DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE,
    DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT,

    DEVICE_CAPS_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
    DEVICE_CAPS_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT,

    DEVICE_CAPS_MAX_TEXTURE_SIZE,
    DEVICE_CAPS_MAX_TEXTURE_LAYERS,
    DEVICE_CAPS_MAX_SPARSE_TEXTURE_LAYERS,
    DEVICE_CAPS_MAX_TEXTURE_ANISOTROPY,
    DEVICE_CAPS_MAX_PATCH_VERTICES,
    DEVICE_CAPS_MAX_VERTEX_BUFFER_SLOTS,
    DEVICE_CAPS_MAX_VERTEX_ATTRIB_STRIDE,
    DEVICE_CAPS_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET,
    DEVICE_CAPS_MAX_UNIFORM_BUFFER_BINDINGS,
    DEVICE_CAPS_MAX_SHADER_STORAGE_BUFFER_BINDINGS,
    DEVICE_CAPS_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,
    DEVICE_CAPS_MAX_TRANSFORM_FEEDBACK_BUFFERS,

    DEVICE_CAPS_MAX
};

#define MAX_ERROR_LOG_LENGTH  2048

class IDevice : public IObjectInterface
{
public:
    virtual void SwapBuffers( SDL_Window * WindowHandle, int SwapInterval ) = 0;

    virtual void GetImmediateContext( IImmediateContext ** ppImmediateContext ) = 0;

    virtual void CreateImmediateContext( SImmediateContextCreateInfo const & _CreateInfo, IImmediateContext ** ppImmediateContext ) = 0;
    virtual void ReleaseImmediateContext( IImmediateContext * pImmediateContext ) = 0;

    virtual void CreateFramebuffer( SFramebufferCreateInfo const & _CreateInfo, TRef< IFramebuffer > * ppFramebuffer ) = 0;

    virtual void CreateRenderPass( SRenderPassCreateInfo const & _CreateInfo, TRef< IRenderPass > * ppRenderPass ) = 0;

    virtual void CreatePipeline( SPipelineCreateInfo const & _CreateInfo, TRef< IPipeline > * ppPipeline ) = 0;

    virtual void CreateShaderFromBinary( SShaderBinaryData const * _BinaryData, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule ) = 0;
    virtual void CreateShaderFromCode( SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule ) = 0;
    virtual void CreateShaderFromCode( SHADER_TYPE _ShaderType, const char * _Source, char ** _InfoLog, TRef< IShaderModule > * ppShaderModule ) = 0;

    virtual void CreateBuffer( SBufferCreateInfo const & _CreateInfo, const void * _SysMem, TRef< IBuffer > * ppBuffer ) = 0;

    virtual void CreateBufferView( SBufferViewCreateInfo const & CreateInfo, TRef< IBuffer > pBuffer, TRef< IBufferView > * ppBufferView ) = 0;

    virtual void CreateTexture( STextureCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture ) = 0;

    virtual void CreateTextureView( STextureViewCreateInfo const & _CreateInfo, TRef< ITexture > * ppTexture ) = 0;

    /** FEATURE_SPARSE_TEXTURES must be supported */
    virtual void CreateSparseTexture( SSparseTextureCreateInfo const & _CreateInfo, TRef< ISparseTexture > * ppTexture ) = 0;

    virtual void CreateTransformFeedback( STransformFeedbackCreateInfo const & _CreateInfo, TRef< ITransformFeedback > * ppTransformFeedback ) = 0;

    virtual void CreateQueryPool( SQueryPoolCreateInfo const & _CreateInfo, TRef< IQueryPool > * ppQueryPool ) = 0;

    /** FEATURE_BINDLESS_TEXTURE must be supported */
    virtual void GetBindlessSampler( ITexture * pTexture, SSamplerInfo const & _CreateInfo, TRef< IBindlessSampler > * ppBindlessSampler ) = 0;

    virtual void CreateResourceTable( TRef< IResourceTable > * ppResourceTable ) = 0;

    virtual bool CreateShaderBinaryData( SHADER_TYPE _ShaderType,
                                         unsigned int _NumSources,
                                         const char * const * _Sources,
                                         /* optional */ char ** _InfoLog,
                                         SShaderBinaryData * _BinaryData ) = 0;

    virtual void DestroyShaderBinaryData( SShaderBinaryData * _BinaryData ) = 0;

    virtual bool IsFeatureSupported( FEATURE_TYPE InFeatureType ) = 0;

    virtual unsigned int GetDeviceCaps( DEVICE_CAPS InDeviceCaps ) = 0;

    /** Get total available GPU memory in kB. FEATURE_GPU_MEMORY_INFO must be supported */
    virtual int32_t GetGPUMemoryTotalAvailable() = 0;

    /** Get current available GPU memory in kB. FEATURE_GPU_MEMORY_INFO must be supported */
    virtual int32_t GetGPUMemoryCurrentAvailable() = 0;

    virtual bool EnumerateSparseTexturePageSize( SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int * NumPageSizes, int * PageSizesX, int * PageSizesY, int * PageSizesZ ) = 0;

    virtual bool ChooseAppropriateSparseTexturePageSize( SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int * PageSizeIndex, int * PageSizeX = nullptr, int * PageSizeY = nullptr, int * PageSizeZ = nullptr ) = 0;

    virtual bool LookupImageFormat( const char * _FormatQualifier, TEXTURE_FORMAT * _Format ) = 0;

    virtual const char * LookupImageFormatQualifier( TEXTURE_FORMAT _Format ) = 0;

    //unsigned int GetTotalImmediateContexts() const { return TotalContexts; }
    //unsigned int GetTotalBuffers() const { return TotalBuffers; }
    //unsigned int GetTotalTextures() const { return TotalTextures; }
    //unsigned int GetTotalSamplers() const { return SamplerCache.Size(); }
    //unsigned int GetTotalBlendingStates() const { return BlendingStateCache.Size(); }
    //unsigned int GetTotalRasterizerStates() const { return RasterizerStateCache.Size(); }
    //unsigned int GetTotalDepthStencilStates() const { return DepthStencilStateCache.Size(); }
    //unsigned int GetTotalShaderModules() const { return TotalShaderModules; }
    //unsigned int GetTotalPipelines() const { return TotalPipelines; }
    //unsigned int GetTotalRenderPasses() const { return TotalRenderPasses; }
    //unsigned int GetTotalFramebuffers() const { return TotalFramebuffers; }
    //unsigned int GetTotalTransformFeedbacks() const { return TotalTransformFeedbacks; }
    //unsigned int GetTotalQueryPools() const { return TotalQueryPools; }
};

void CreateLogicalDevice( SImmediateContextCreateInfo const & _CreateInfo,
                          SAllocatorCallback const * _Allocator,
                          HashCallback _Hash,
                          TRef< IDevice > * ppDevice );

}
