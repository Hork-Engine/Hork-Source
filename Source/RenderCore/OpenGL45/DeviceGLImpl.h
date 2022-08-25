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

#pragma once

#include <RenderCore/Device.h>

namespace RenderCore
{

class AImmediateContextGLImpl;

class AWindowPoolGL
{
public:
    AWindowPoolGL();
    ~AWindowPoolGL();

    struct SWindowGL
    {
        SDL_Window* Handle;
        void*       GLContext;
        AImmediateContextGLImpl* ImmediateCtx;
    };

    SWindowGL Create();
    void      Destroy(SWindowGL Window);
    void      Free(SWindowGL Window);

    SWindowGL NewWindow();

    TSmallVector<SWindowGL, 8> Pool;
};

struct SVertexLayoutDescGL
{
    uint32_t           NumVertexBindings{};
    SVertexBindingInfo VertexBindings[MAX_VERTEX_BINDINGS] = {};
    uint32_t           NumVertexAttribs{};
    SVertexAttribInfo  VertexAttribs[MAX_VERTEX_ATTRIBS] = {};

    bool operator==(SVertexLayoutDescGL const& Rhs) const
    {
        if (NumVertexBindings != Rhs.NumVertexBindings)
            return false;
        if (NumVertexAttribs != Rhs.NumVertexAttribs)
            return false;
        for (uint32_t i = 0; i < NumVertexBindings; ++i)
            if (VertexBindings[i] != Rhs.VertexBindings[i])
                return false;
        for (uint32_t i = 0; i < NumVertexAttribs; ++i)
            if (VertexAttribs[i] != Rhs.VertexAttribs[i])
                return false;
        return true;
    }

    bool operator!=(SVertexLayoutDescGL const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        uint32_t h{};
        for (uint32_t i = 0; i < NumVertexBindings; ++i)
            h = HashTraits::HashCombine(h, VertexBindings[i]);
        for (uint32_t i = 0; i < NumVertexAttribs; ++i)
            h = HashTraits::HashCombine(h, VertexAttribs[i]);
        return h;
    }
};

class ADeviceGLImpl final : public IDevice
{
public:
    ADeviceGLImpl(SAllocatorCallback const* pAllocator);
    ~ADeviceGLImpl();

    IImmediateContext* GetImmediateContext() override;

    void GetOrCreateMainWindow(SVideoMode const& VideoMode, TRef<IGenericWindow>* ppWindow) override;

    void CreateGenericWindow(SVideoMode const& VideoMode, TRef<IGenericWindow>* ppWindow) override;

    void CreateSwapChain(IGenericWindow* pWindow, TRef<ISwapChain>* ppSwapChain) override;

    void CreatePipeline(SPipelineDesc const& Desc, TRef<IPipeline>* ppPipeline) override;

    void CreateShaderFromBinary(SShaderBinaryData const* _BinaryData, TRef<IShaderModule>* ppShaderModule) override;
    void CreateShaderFromCode(SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources, TRef<IShaderModule>* ppShaderModule) override;

    void CreateBuffer(SBufferDesc const& Desc, const void* _SysMem, TRef<IBuffer>* ppBuffer) override;

    void CreateTexture(STextureDesc const& Desc, TRef<ITexture>* ppTexture) override;

    void CreateSparseTexture(SSparseTextureDesc const& Desc, TRef<ISparseTexture>* ppTexture) override;

    void CreateTransformFeedback(STransformFeedbackDesc const& Desc, TRef<ITransformFeedback>* ppTransformFeedback) override;

    void CreateQueryPool(SQueryPoolDesc const& Desc, TRef<IQueryPool>* ppQueryPool) override;

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

    SAllocatorCallback const& GetAllocator() const override;

    //
    // Local
    //

    class AVertexLayoutGL* GetVertexLayout(SVertexBindingInfo const* pVertexBindings,
                                           uint32_t                  NumVertexBindings,
                                           SVertexAttribInfo const*  pVertexAttribs,
                                           uint32_t                  NumVertexAttribs);

    THashMap<SVertexLayoutDescGL, AVertexLayoutGL*> const& GetVertexLayouts() const { return VertexLayouts; }

    SBlendingStateInfo const*     CachedBlendingState(SBlendingStateInfo const& _BlendingState);
    SRasterizerStateInfo const*   CachedRasterizerState(SRasterizerStateInfo const& _RasterizerState);
    SDepthStencilStateInfo const* CachedDepthStencilState(SDepthStencilStateInfo const& _DepthStencilState);
    unsigned int                  CachedSampler(SSamplerDesc const& SamplerDesc);

    size_t BufferMemoryAllocated;
    size_t TextureMemoryAllocated;

private:
    SAllocatorCallback Allocator;

    TWeakRef<IGenericWindow> pMainWindow;

    THashMap<SVertexLayoutDescGL, AVertexLayoutGL*> VertexLayouts;

    THashMap<SSamplerDesc, struct SamplerInfo*> Samplers;

    THashMap<SBlendingStateInfo, SBlendingStateInfo*>         BlendingStates;
    THashMap<SRasterizerStateInfo, SRasterizerStateInfo*>     RasterizerStates;
    THashMap<SDepthStencilStateInfo, SDepthStencilStateInfo*> DepthStencilStates;

    AWindowPoolGL WindowPool;
    AWindowPoolGL::SWindowGL MainWindowHandle;
};

} // namespace RenderCore
