/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

namespace RenderCore
{

class ImmediateContextGLImpl;

class WindowPoolGL
{
public:
    WindowPoolGL();
    ~WindowPoolGL();

    struct WindowGL
    {
        SDL_Window* Handle;
        void*       GLContext;
        ImmediateContextGLImpl* ImmediateCtx;
    };

    WindowGL Create();
    void     Destroy(WindowGL Window);
    void     Free(WindowGL Window);

    WindowGL NewWindow();

    TSmallVector<WindowGL, 8> Pool;
};

struct VertexLayoutDescGL
{
    uint32_t           NumVertexBindings{};
    VertexBindingInfo VertexBindings[MAX_VERTEX_BINDINGS] = {};
    uint32_t           NumVertexAttribs{};
    VertexAttribInfo  VertexAttribs[MAX_VERTEX_ATTRIBS] = {};

    bool operator==(VertexLayoutDescGL const& Rhs) const
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

    bool operator!=(VertexLayoutDescGL const& Rhs) const
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

class DeviceGLImpl final : public IDevice
{
public:
    DeviceGLImpl(AllocatorCallback const* pAllocator);
    ~DeviceGLImpl();

    IImmediateContext* GetImmediateContext() override;

    void GetOrCreateMainWindow(DisplayVideoMode const& VideoMode, TRef<IGenericWindow>* ppWindow) override;

    void CreateGenericWindow(DisplayVideoMode const& VideoMode, TRef<IGenericWindow>* ppWindow) override;

    void CreateSwapChain(IGenericWindow* pWindow, TRef<ISwapChain>* ppSwapChain) override;

    void CreatePipeline(PipelineDesc const& Desc, TRef<IPipeline>* ppPipeline) override;

    void CreateShaderFromBinary(ShaderBinaryData const* _BinaryData, TRef<IShaderModule>* ppShaderModule) override;
    void CreateShaderFromCode(SHADER_TYPE _ShaderType, unsigned int _NumSources, const char* const* _Sources, TRef<IShaderModule>* ppShaderModule) override;

    void CreateBuffer(BufferDesc const& Desc, const void* _SysMem, TRef<IBuffer>* ppBuffer) override;

    void CreateTexture(TextureDesc const& Desc, TRef<ITexture>* ppTexture) override;

    void CreateSparseTexture(SparseTextureDesc const& Desc, TRef<ISparseTexture>* ppTexture) override;

    void CreateTransformFeedback(TransformFeedbackDesc const& Desc, TRef<ITransformFeedback>* ppTransformFeedback) override;

    void CreateQueryPool(QueryPoolDesc const& Desc, TRef<IQueryPool>* ppQueryPool) override;

    void CreateResourceTable(TRef<IResourceTable>* ppResourceTable) override;

    bool CreateShaderBinaryData(SHADER_TYPE        _ShaderType,
                                unsigned int       _NumSources,
                                const char* const* _Sources,
                                ShaderBinaryData* _BinaryData) override;

    void DestroyShaderBinaryData(ShaderBinaryData* _BinaryData) override;

    int32_t GetGPUMemoryTotalAvailable() override;
    int32_t GetGPUMemoryCurrentAvailable() override;

    bool EnumerateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int* NumPageSizes, int* PageSizesX, int* PageSizesY, int* PageSizesZ) override;

    bool ChooseAppropriateSparseTexturePageSize(SPARSE_TEXTURE_TYPE Type, TEXTURE_FORMAT Format, int Width, int Height, int Depth, int* PageSizeIndex, int* PageSizeX = nullptr, int* PageSizeY = nullptr, int* PageSizeZ = nullptr) override;

    AllocatorCallback const& GetAllocator() const override;

    //
    // Local
    //

    class VertexLayoutGL* GetVertexLayout(VertexBindingInfo const* pVertexBindings,
                                           uint32_t                NumVertexBindings,
                                           VertexAttribInfo const* pVertexAttribs,
                                           uint32_t                NumVertexAttribs);

    THashMap<VertexLayoutDescGL, VertexLayoutGL*> const& GetVertexLayouts() const { return VertexLayouts; }

    BlendingStateInfo const*     CachedBlendingState(BlendingStateInfo const& _BlendingState);
    RasterizerStateInfo const*   CachedRasterizerState(RasterizerStateInfo const& _RasterizerState);
    DepthStencilStateInfo const* CachedDepthStencilState(DepthStencilStateInfo const& _DepthStencilState);
    unsigned int                 CachedSampler(SamplerDesc const& SamplerDesc);

    size_t BufferMemoryAllocated;
    size_t TextureMemoryAllocated;

private:
    AllocatorCallback Allocator;

    TWeakRef<IGenericWindow> pMainWindow;

    THashMap<VertexLayoutDescGL, VertexLayoutGL*> VertexLayouts;

    THashMap<SamplerDesc, struct SamplerInfo*> Samplers;

    THashMap<BlendingStateInfo, BlendingStateInfo*>         BlendingStates;
    THashMap<RasterizerStateInfo, RasterizerStateInfo*>     RasterizerStates;
    THashMap<DepthStencilStateInfo, DepthStencilStateInfo*> DepthStencilStates;

    WindowPoolGL WindowPool;
    WindowPoolGL::WindowGL MainWindowHandle;
};

} // namespace RenderCore

HK_NAMESPACE_END
