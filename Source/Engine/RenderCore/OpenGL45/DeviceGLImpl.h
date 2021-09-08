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

namespace RenderCore
{

class AImmediateContextGLImpl;

class ADeviceGLImpl final : public IDevice
{
public:
    ADeviceGLImpl(SImmediateContextDesc const& Desc,
                  SAllocatorCallback const*    pAllocator,
                  TRef<IImmediateContext>*     ppImmediateContext);
    ~ADeviceGLImpl();

    void CreateImmediateContext(SImmediateContextDesc const& Desc, TRef<IImmediateContext>* ppImmediateContext) override;

    void CreateSwapChain(SDL_Window* pWindow, TRef<ISwapChain>* ppSwapChain) override;

    void CreatePipeline(SPipelineDesc const& Desc, TRef<IPipeline>* ppPipeline) override;

    void CreateShaderFromBinary(SShaderBinaryData const* _BinaryData, TRef<IShaderModule>* ppShaderModule) override;
    void CreateShaderFromCode(SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources, TRef<IShaderModule>* ppShaderModule) override;

    void CreateBuffer(SBufferDesc const& Desc, const void* _SysMem, TRef<IBuffer>* ppBuffer) override;

    void CreateTexture(STextureDesc const& Desc, TRef<ITexture>* ppTexture) override;

    void CreateSparseTexture(SSparseTextureDesc const& Desc, TRef<ISparseTexture>* ppTexture) override;

    void CreateTransformFeedback(STransformFeedbackDesc const& Desc, TRef<ITransformFeedback>* ppTransformFeedback) override;

    void CreateQueryPool(SQueryPoolDesc const& Desc, TRef<IQueryPool>* ppQueryPool) override;

    void GetBindlessSampler(ITexture* pTexture, SSamplerDesc const& Desc, TRef<IBindlessSampler>* ppBindlessSampler) override;

    void CreateResourceTable(TRef<IResourceTable>* ppResourceTable) override;

    bool CreateShaderBinaryData(SHADER_TYPE        _ShaderType,
                                unsigned int       _NumSources,
                                const char* const* _Sources,
                                SShaderBinaryData* _BinaryData) override;

    void DestroyShaderBinaryData(SShaderBinaryData* _BinaryData) override;

    int32_t GetGPUMemoryTotalAvailable() override;
    int32_t GetGPUMemoryCurrentAvailable() override;

    bool EnumerateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int* NumPageSizes, int* PageSizesX, int* PageSizesY, int* PageSizesZ) override;

    bool ChooseAppropriateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int* PageSizeIndex, int* PageSizeX = nullptr, int* PageSizeY = nullptr, int* PageSizeZ = nullptr) override;

    bool LookupImageFormat(const char* _FormatQualifier, TEXTURE_FORMAT* _Format) override;

    const char* LookupImageFormatQualifier(TEXTURE_FORMAT _Format) override;

    SAllocatorCallback const& GetAllocator() const override;

    //
    // Local
    //

    class AVertexLayoutGL* GetVertexLayout(SVertexBindingInfo const* pVertexBindings,
                                           uint32_t                  NumVertexBindings,
                                           SVertexAttribInfo const*  pVertexAttribs,
                                           uint32_t                  NumVertexAttribs);

    TPodVector<AVertexLayoutGL*> const& GetVertexLayouts() const { return VertexLayouts; }

    SBlendingStateInfo const*     CachedBlendingState(SBlendingStateInfo const& _BlendingState);
    SRasterizerStateInfo const*   CachedRasterizerState(SRasterizerStateInfo const& _RasterizerState);
    SDepthStencilStateInfo const* CachedDepthStencilState(SDepthStencilStateInfo const& _DepthStencilState);
    unsigned int                  CachedSampler(SSamplerDesc const& SamplerDesc);

    size_t BufferMemoryAllocated;
    size_t TextureMemoryAllocated;

private:
    SAllocatorCallback Allocator;

    THash<>                      VertexLayoutsHash;
    TPodVector<AVertexLayoutGL*> VertexLayouts;

    THash<>                         SamplerHash;
    TPodVector<struct SamplerInfo*> SamplerCache;

    THash<>                         BlendingHash;
    TPodVector<SBlendingStateInfo*> BlendingStateCache;

    THash<>                           RasterizerHash;
    TPodVector<SRasterizerStateInfo*> RasterizerStateCache;

    THash<>                             DepthStencilHash;
    TPodVector<SDepthStencilStateInfo*> DepthStencilStateCache;
};

} // namespace RenderCore
