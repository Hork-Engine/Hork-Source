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

#include "DeviceGLImpl.h"
#include "FramebufferGLImpl.h"

namespace RenderCore
{

class ARenderPassGLImpl;
class APipelineGLImpl;

struct SBindingStateGL
{
    uint64_t                      ReadFramebuffer;
    unsigned int                  DrawFramebuffer;
    unsigned short                DrawFramebufferWidth;
    unsigned short                DrawFramebufferHeight;
    unsigned int                  DrawInderectBuffer;
    unsigned int                  DispatchIndirectBuffer;
    SBlendingStateInfo const*     BlendState;        // current blend state binding
    SRasterizerStateInfo const*   RasterizerState;   // current rasterizer state binding
    SDepthStencilStateInfo const* DepthStencilState; // current depth-stencil state binding
};

class AResourceTableGLImpl final : public IResourceTable
{
public:
    AResourceTableGLImpl(ADeviceGLImpl* pDevice);
    ~AResourceTableGLImpl();

    void BindTexture(unsigned int Slot, ITextureView* pShaderResourceView) override;
    void BindTexture(unsigned int Slot, IBufferView* pShaderResourceView) override;
    void BindImage(unsigned int Slot, ITextureView* pUnorderedAccessView) override;

    void BindBuffer(int Slot, IBuffer const* pBuffer, size_t Offset = 0, size_t Size = 0) override;

    const unsigned int* GetTextureBindings() const { return TextureBindings; }
    const uint32_t*     GetTextureBindingUIDs() const { return TextureBindingUIDs; }

    const unsigned int* GetImageBindings() const { return ImageBindings; }
    const uint32_t*     GetImageBindingUIDs() const { return ImageBindingUIDs; }
    const uint16_t*     GetImageMipLevel() const { return ImageMipLevel; }
    const uint16_t*     GetImageLayerIndex() const { return ImageLayerIndex; }
    const bool*         GetImageLayered() const { return ImageLayered; }

    const unsigned int* GetBufferBindings() const { return BufferBindings; }
    const uint32_t*     GetBufferBindingUIDs() const { return BufferBindingUIDs; }
    const ptrdiff_t*    GetBufferBindingOffsets() const { return BufferBindingOffsets; }
    const ptrdiff_t*    GetBufferBindingSizes() const { return BufferBindingSizes; }

private:
    unsigned int TextureBindings[MAX_SAMPLER_SLOTS];
    uint32_t     TextureBindingUIDs[MAX_SAMPLER_SLOTS];

    unsigned int ImageBindings[MAX_IMAGE_SLOTS];
    uint32_t     ImageBindingUIDs[MAX_IMAGE_SLOTS];
    uint16_t     ImageMipLevel[MAX_IMAGE_SLOTS];
    uint16_t     ImageLayerIndex[MAX_IMAGE_SLOTS];
    bool         ImageLayered[MAX_IMAGE_SLOTS];

    unsigned int BufferBindings[MAX_BUFFER_SLOTS];
    uint32_t     BufferBindingUIDs[MAX_BUFFER_SLOTS];
    ptrdiff_t    BufferBindingOffsets[MAX_BUFFER_SLOTS];
    ptrdiff_t    BufferBindingSizes[MAX_BUFFER_SLOTS];
};

class AFramebufferCacheGLImpl : public ARefCounted
{
public:
    AFramebufferCacheGLImpl(ADeviceGLImpl* pDevice) :
        pDevice(pDevice)
    {}

    void CleanupOutdatedFramebuffers();

    AFramebufferGLImpl* GetFramebuffer(const char* RenderPassName, TStdVector<STextureAttachment>& ColorAttachments, STextureAttachment* DepthStencilAttachment);

private:
    TRef<ADeviceGLImpl>            pDevice;
    THash<>                        FramebufferHash;
    TStdVector<TRef<AFramebufferGLImpl>> FramebufferCache;
    TPodVector<int>                FramebufferHashes;
};

struct SRenderPassBeginGL
{
    ARenderPass*                   pRenderPass;
    AFramebufferGLImpl const*      pFramebuffer;
    SRect2D                        RenderArea;
};

class AImmediateContextGLImpl final : public IImmediateContext
{
public:
    AImmediateContextGLImpl(ADeviceGLImpl* pDevice, SImmediateContextDesc const& Desc, void* Context);
    ~AImmediateContextGLImpl();

    void MakeCurrent() override;

    static AImmediateContextGLImpl* GetCurrent() { return static_cast<AImmediateContextGLImpl*>(Current); }

    void ExecuteFrameGraph(AFrameGraph* pFrameGraph) override;

    //
    // Pipeline
    //

    void BindPipeline(IPipeline* _Pipeline) override;

    //
    // Vertex & Index buffers
    //

    void BindVertexBuffer(unsigned int _InputSlot, /* optional */ IBuffer const* _VertexBuffer, unsigned int _Offset = 0) override;

    void BindVertexBuffers(unsigned int _StartSlot, unsigned int _NumBuffers, /* optional */ IBuffer* const* _VertexBuffers, uint32_t const* _Offsets = nullptr) override;

    void BindIndexBuffer(IBuffer const* _IndexBuffer, INDEX_TYPE _Type, unsigned int _Offset = 0) override;

    //
    // Shader resources
    //

    IResourceTable* GetRootResourceTable() override;

    void BindResourceTable(IResourceTable* _ResourceTable) override;

    //
    // Viewport
    //

    void SetViewport(SViewport const& _Viewport) override;

    void SetViewportArray(uint32_t _NumViewports, SViewport const* _Viewports) override;

    void SetViewportArray(uint32_t _FirstIndex, uint32_t _NumViewports, SViewport const* _Viewports) override;

    void SetViewportIndexed(uint32_t _Index, SViewport const& _Viewport) override;

    //
    // Scissor
    //

    void SetScissor(/* optional */ SRect2D const& _Scissor) override;

    void SetScissorArray(uint32_t _NumScissors, SRect2D const* _Scissors) override;

    void SetScissorArray(uint32_t _FirstIndex, uint32_t _NumScissors, SRect2D const* _Scissors) override;

    void SetScissorIndexed(uint32_t _Index, SRect2D const& _Scissor) override;

    //
    // Render pass
    //

    void BeginRenderPass(SRenderPassBeginGL const& _RenderPassBegin);

    void NextSubpass();

    void EndRenderPass();

    //
    // Transform feedback
    //

    void BindTransformFeedback(ITransformFeedback* _TransformFeedback) override;

    void BeginTransformFeedback(PRIMITIVE_TOPOLOGY _OutputPrimitive) override;

    void ResumeTransformFeedback() override;

    void PauseTransformFeedback() override;

    void EndTransformFeedback() override;

    //
    // Draw
    //

    void Draw(SDrawCmd const* _Cmd) override;

    void Draw(SDrawIndexedCmd const* _Cmd) override;

    void Draw(ITransformFeedback* _TransformFeedback, unsigned int _InstanceCount = 1, unsigned int _StreamIndex = 0) override;
#if 0
    void DrawIndirect( SDrawIndirectCmd const * _Cmd ) override;

    void DrawIndirect( SDrawIndexedIndirectCmd const * _Cmd ) override;
#endif
    void DrawIndirect(IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset) override;

    void DrawIndexedIndirect(IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset) override;

    void MultiDraw(unsigned int _DrawCount, const unsigned int* _VertexCount, const unsigned int* _StartVertexLocations) override;

    void MultiDraw(unsigned int _DrawCount, const unsigned int* _IndexCount, const void* const* _IndexByteOffsets, const int* _BaseVertexLocations = nullptr) override;
#if 0
    void MultiDrawIndirect( unsigned int _DrawCount, SDrawIndirectCmd const * _Cmds, unsigned int _Stride ) override;

    void MultiDrawIndirect( unsigned int _DrawCount, SDrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride ) override;
#endif
    void MultiDrawIndirect(unsigned int _DrawCount, IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset, unsigned int _Stride) override;

    void MultiDrawIndexedIndirect(unsigned int _DrawCount, IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset, unsigned int _Stride) override;

    //
    // Dispatch compute
    //

    void DispatchCompute(unsigned int _ThreadGroupCountX,
                         unsigned int _ThreadGroupCountY,
                         unsigned int _ThreadGroupCountZ) override;

    void DispatchCompute(SDispatchIndirectCmd const* _Cmd) override;

    void DispatchComputeIndirect(IBuffer* _DispatchIndirectBuffer, unsigned int _AlignedByteOffset) override;

    //
    // Query
    //

    void BeginQuery(IQueryPool* _QueryPool, uint32_t _QueryID, uint32_t _StreamIndex = 0) override;

    void EndQuery(IQueryPool* _QueryPool, uint32_t _StreamIndex = 0) override;

    void RecordTimeStamp(IQueryPool* _QueryPool, uint32_t _QueryID) override;

    void CopyQueryPoolResultsAvailable(IQueryPool* _QueryPool,
                                       uint32_t    _FirstQuery,
                                       uint32_t    _QueryCount,
                                       IBuffer*    _DstBuffer,
                                       size_t      _DstOffst,
                                       size_t      _DstStride,
                                       bool        _QueryResult64Bit) override;

    void CopyQueryPoolResults(IQueryPool*        _QueryPool,
                              uint32_t           _FirstQuery,
                              uint32_t           _QueryCount,
                              IBuffer*           _DstBuffer,
                              size_t             _DstOffst,
                              size_t             _DstStride,
                              QUERY_RESULT_FLAGS _Flags) override;

    //
    // Conditional render
    //

    void BeginConditionalRender(IQueryPool* _QueryPool, uint32_t _QueryID, CONDITIONAL_RENDER_MODE _Mode) override;

    void EndConditionalRender() override;

    //
    // Synchronization
    //

    SyncObject FenceSync() override;

    void RemoveSync(SyncObject _Sync) override;

    CLIENT_WAIT_STATUS ClientWait(SyncObject _Sync, /* optional */ uint64_t _TimeOutNanoseconds = 0xFFFFFFFFFFFFFFFF) override;

    void ServerWait(SyncObject _Sync) override;

    bool IsSignaled(SyncObject _Sync) override;

    void Flush() override;

    void Barrier(int _BarrierBits) override;

    void BarrierByRegion(int _BarrierBits) override;

    void TextureBarrier() override;

    //
    // Dynamic state
    //

    void DynamicState_BlendingColor(/* optional */ const float _ConstantColor[4]) override;

    void DynamicState_SampleMask(/* optional */ const uint32_t _SampleMask[4]) override;

    void DynamicState_StencilRef(uint32_t _StencilRef) override;

    //
    // Copy
    //

    void CopyBuffer(IBuffer* _SrcBuffer, IBuffer* _DstBuffer) override;

    void CopyBufferRange(IBuffer* _SrcBuffer, IBuffer* _DstBuffer, uint32_t _NumRanges, SBufferCopy const* _Ranges) override;

    bool CopyBufferToTexture(IBuffer const*      pSrcBuffer,
                             ITexture*           DstTexture,
                             STextureRect const& Rectangle,
                             DATA_FORMAT         Format,
                             size_t              CompressedDataSizeInBytes,
                             size_t              SourceByteOffset,
                             unsigned int        Alignment) override;

    void CopyTextureToBuffer(ITexture const*     pSrcTexture,
                             IBuffer*            DstBuffer,
                             STextureRect const& Rectangle,
                             DATA_FORMAT         Format,
                             size_t              SizeInBytes,
                             size_t              DstByteOffset,
                             unsigned int        Alignment) override;

    void CopyTextureRect(ITexture const*    pSrcTexture,
                         ITexture*          pDstTexture,
                         uint32_t           NumCopies,
                         TextureCopy const* Copies) override;


    //
    // Clear
    //

    void ClearBuffer(IBuffer* pBuffer, BUFFER_VIEW_PIXEL_FORMAT InternalFormat, DATA_FORMAT Format, const SClearValue* ClearValue) override;

    void ClearBufferRange(IBuffer* pBuffer, BUFFER_VIEW_PIXEL_FORMAT InternalFormat, uint32_t NumRanges, SBufferClear const* Ranges, DATA_FORMAT Format, const SClearValue* ClearValue) override;

    void ClearTexture(ITexture* pTexture, uint16_t MipLevel, DATA_FORMAT Format, const SClearValue* ClearValue) override;

    void ClearTextureRect(ITexture*           pTexture,
                          uint32_t            NumRectangles,
                          STextureRect const* Rectangles,
                          DATA_FORMAT         Format,
                          const SClearValue*  ClearValue) override;

    //
    // Read
    //

    void ReadTexture(ITexture*    pTexture,
                     uint16_t     MipLevel,
                     DATA_FORMAT  Format,
                     size_t       SizeInBytes,
                     unsigned int Alignment,
                     void*        pSysMem) override;

    void ReadTextureRect(ITexture*           pTexture,
                         STextureRect const& Rectangle,
                         DATA_FORMAT         Format,
                         size_t              SizeInBytes,
                         unsigned int        Alignment,
                         void*               pSysMem) override;

    bool WriteTexture(ITexture*    pTexture,
                      uint16_t     MipLevel,
                      DATA_FORMAT  Format,
                      size_t       SizeInBytes,
                      unsigned int Alignment,
                      const void*  pSysMem) override;

    bool WriteTextureRect(ITexture*           pTexture,
                          STextureRect const& Rectangle,
                          DATA_FORMAT         Format,
                          size_t              SizeInBytes,
                          unsigned int        Alignment,
                          const void*         pSysMem) override;

    //
    // Sparse texture
    // 

    void SparseTextureCommitPage(ISparseTexture* pTexture,
                                 int             MipLevel,
                                 int             PageX,
                                 int             PageY,
                                 int             PageZ,
                                 DATA_FORMAT     Format, // Specifies a pixel format for the input data
                                 size_t          SizeInBytes,
                                 unsigned int    Alignment, // Specifies alignment of source data
                                 const void*     pSysMem) override;

    void SparseTextureCommitRect(ISparseTexture*     pTexture,
                                 STextureRect const& Rectangle,
                                 DATA_FORMAT         Format, // Specifies a pixel format for the input data
                                 size_t              SizeInBytes,
                                 unsigned int        Alignment, // Specifies alignment of source data
                                 const void*         pSysMem) override;

    void SparseTextureUncommitPage(ISparseTexture* pTexture, int MipLevel, int PageX, int PageY, int PageZ) override;

    void SparseTextureUncommitRect(ISparseTexture* pTexture, STextureRect const& Rectangle) override;

    //
    // Buffer
    //

    /// Client-side call function
    void ReadBuffer(IBuffer* pBuffer, void* pSysMem) override;

    /// Client-side call function
    void ReadBufferRange(IBuffer* pBuffer, size_t ByteOffset, size_t SizeInBytes, void* pSysMem) override;

    /// Client-side call function
    void WriteBuffer(IBuffer* pBuffer, const void* pSysMem) override;

    /// Client-side call function
    void WriteBufferRange(IBuffer* pBuffer, size_t ByteOffset, size_t SizeInBytes, const void* pSysMem) override;

    /// Returns pointer to the buffer range data.
    /// _RangeOffset
    ///     Specifies the start within the buffer of the range to be mapped.
    /// _RangeSize
    ///     Specifies the byte length of the range to be mapped.
    /// _ClientServerTransfer
    ///     Indicates how user will communicate with mapped data. See EMapTransfer for details.
    /// _Invalidate
    ///     Indicates will the previous contents of the specified range or entire buffer discarded, or not.
    ///     See EMapInvalidate for details.
    /// _Persistence
    ///     Indicates persistency of mapped buffer data. See EMapPersistence for details.
    /// _FlushExplicit
    ///     Indicates that one or more discrete subranges of the mapping may be modified.
    ///     When this flag is set, modifications to each subrange must be explicitly flushed by
    ///     calling FlushMappedRange. No error is set if a subrange of the mapping is modified
    ///     and not flushed, but data within the corresponding subrange of the buffer are undefined.
    ///     This flag may only be used in conjunction with MAP_TRANSFER_WRITE.
    ///     When this option is selected, flushing is strictly limited to regions that are explicitly
    ///     indicated with calls to FlushMappedRange prior to unmap;
    ///     if this option is not selected Unmap will automatically flush the entire mapped range when called.
    /// _Unsynchronized
    ///     Indicates that the hardware should not attempt to synchronize pending operations on the buffer prior to returning
    ///     from MapRange. No error is generated if pending operations which source
    ///     or modify the buffer overlap the mapped region, but the result of such previous and any subsequent
    ///     operations is undefined.
    void* MapBufferRange(IBuffer* pBuffer, size_t _RangeOffset, size_t _RangeSize, MAP_TRANSFER _ClientServerTransfer, MAP_INVALIDATE _Invalidate = MAP_NO_INVALIDATE, MAP_PERSISTENCE _Persistence = MAP_NON_PERSISTENT, bool _FlushExplicit = false, bool _Unsynchronized = false) override;

    /// Returns pointer to the entire buffer data.
    void* MapBuffer(IBuffer* pBuffer, MAP_TRANSFER _ClientServerTransfer, MAP_INVALIDATE _Invalidate = MAP_NO_INVALIDATE, MAP_PERSISTENCE _Persistence = MAP_NON_PERSISTENT, bool _FlushExplicit = false, bool _Unsynchronized = false) override;

    /// After calling this function, you should not use
    /// the pointer returned by Map or MapRange again.
    void UnmapBuffer(IBuffer* pBuffer) override;

    //
    // Query
    //

    void GetQueryPoolResults(IQueryPool*        QueryPool,
                             uint32_t           FirstQuery,
                             uint32_t           QueryCount,
                             size_t             DataSize,
                             void*              pSysMem,
                             size_t             DstStride,
                             QUERY_RESULT_FLAGS Flags) override;
    
    //
    // Misc
    //

    void GenerateTextureMipLevels(ITexture* pTexture) override;

    //
    // Render pass
    //

    bool CopyFramebufferToTexture(ARenderPassContext&   RenderPassContext,
                                  ITexture*             pDstTexture,
                                  int                   ColorAttachment,
                                  STextureOffset const& Offset,
                                  SRect2D const&        SrcRect,
                                  unsigned int          Alignment) override;

    void CopyColorAttachmentToBuffer(ARenderPassContext& RenderPassContext,
                                     IBuffer*            pDstBuffer,
                                     int                 SubpassAttachmentRef,
                                     SRect2D const&      SrcRect,
                                     FRAMEBUFFER_CHANNEL FramebufferChannel,
                                     FRAMEBUFFER_OUTPUT  FramebufferOutput,
                                     COLOR_CLAMP         ColorClamp,
                                     size_t              SizeInBytes,
                                     size_t              DstByteOffset,
                                     unsigned int        Alignment) override;

    void CopyDepthAttachmentToBuffer(ARenderPassContext& RenderPassContext,
                                     IBuffer*            pDstBuffer,
                                     SRect2D const&      SrcRect,
                                     size_t              SizeInBytes,
                                     size_t              DstByteOffset,
                                     unsigned int        Alignment) override;

    bool BlitFramebuffer(ARenderPassContext&   RenderPassContext,
                         int                   ColorAttachment,
                         uint32_t              NumRectangles,
                         SBlitRectangle const* Rectangles,
                         FRAMEBUFFER_BLIT_MASK Mask,
                         bool                  LinearFilter) override;

    void ClearAttachments(ARenderPassContext&            RenderPassContext,
                          unsigned int*                  ColorAttachments,
                          unsigned int                   NumColorAttachments,
                          SClearColorValue const*        ColorClearValues,
                          SClearDepthStencilValue const* DepthStencilClearValue,
                          SRect2D const*                 Rect) override;

    bool ReadFramebufferAttachment(ARenderPassContext& RenderPassContext,
                                   int                 ColorAttachment,
                                   SRect2D const&      SrcRect,
                                   FRAMEBUFFER_CHANNEL FramebufferChannel,
                                   FRAMEBUFFER_OUTPUT  FramebufferOutput,
                                   COLOR_CLAMP         ColorClamp,
                                   size_t              SizeInBytes,
                                   unsigned int        Alignment, // Specifies alignment of destination data
                                   void*               pSysMem) override;

    bool ReadFramebufferDepthStencilAttachment(ARenderPassContext& RenderPassContext,
                                               SRect2D const&      SrcRect,
                                               size_t              SizeInBytes,
                                               unsigned int        Alignment,
                                               void*               pSysMem) override;

    //
    // Local
    //

    bool IsMainContext() const { return bMainContext; }

private:
    void PolygonOffsetClampSafe(float _Slope, int _Bias, float _Clamp);

    bool CopyBufferToTexture1D(IBuffer const* pSrcBuffer,
                               ITexture*      pDstTexture,
                               uint16_t       MipLevel,
                               uint16_t       OffsetX,
                               uint16_t       DimensionX,
                               size_t         CompressedDataSizeInBytes, // Only for compressed images
                               DATA_FORMAT    Format,
                               size_t         SourceByteOffset,
                               unsigned int   Alignment);

    bool CopyBufferToTexture2D(IBuffer const* pSrcBuffer,
                               ITexture*      pDstTexture,
                               uint16_t       MipLevel,
                               uint16_t       OffsetX,
                               uint16_t       OffsetY,
                               uint16_t       DimensionX,
                               uint16_t       DimensionY,
                               uint16_t       CubeFaceIndex,             // only for TEXTURE_CUBE_MAP
                               uint16_t       NumCubeFaces,              // only for TEXTURE_CUBE_MAP
                               size_t         CompressedDataSizeInBytes, // Only for compressed images
                               DATA_FORMAT    Format,
                               size_t         SourceByteOffset,
                               unsigned int   Alignment);

    bool CopyBufferToTexture3D(IBuffer const* pSrcBuffer,
                               ITexture*      pDstTexture,
                               uint16_t       MipLevel,
                               uint16_t       OffsetX,
                               uint16_t       OffsetY,
                               uint16_t       OffsetZ,
                               uint16_t       DimensionX,
                               uint16_t       DimensionY,
                               uint16_t       DimensionZ,
                               size_t         CompressedDataSizeInBytes, // Only for compressed images
                               DATA_FORMAT    Format,
                               size_t         SourceByteOffset,
                               unsigned int   Alignment);

    void UpdateVertexBuffers();
    void UpdateVertexAndIndexBuffers();
    void UpdateShaderBindings();

    void BeginSubpass();
    void EndSubpass();
    void UpdateDrawBuffers();

    void PackAlignment(unsigned int _Alignment);
    void UnpackAlignment(unsigned int _Alignment);

    void ClampReadColor(COLOR_CLAMP _ColorClamp);

    void BindReadFramebuffer(AFramebufferGLImpl const* Framebuffer);

    bool ChooseReadBuffer(AFramebufferGLImpl const* pFramebuffer, int _ColorAttachment) const;

    void ExecuteRenderPass(class ARenderPass* pRenderPass);
    void ExecuteCustomTask(class ACustomTask* pCustomTask);

    struct SDL_Window* pWindow;
    void*              pContextGL;
    bool               bMainContext;

    TRef<AFramebufferGLImpl> DefaultFramebuffer;

    SBindingStateGL Binding;

    uint32_t  BufferBindingUIDs[MAX_BUFFER_SLOTS];
    ptrdiff_t BufferBindingOffsets[MAX_BUFFER_SLOTS];
    ptrdiff_t BufferBindingSizes[MAX_BUFFER_SLOTS];

    TRef<IResourceTable>       RootResourceTable;
    TRef<AResourceTableGLImpl> CurrentResourceTable;
    APipelineGLImpl*           CurrentPipeline;
    class AVertexLayoutGL*     CurrentVertexLayout;
    class AVertexArrayObjectGL* CurrentVAO;
    uint8_t                    NumPatchVertices;      // count of patch vertices to set by glPatchParameteri
    unsigned int               IndexBufferType;       // type of current binded index buffer (uin16 or uint32_t)
    size_t                     IndexBufferTypeSizeOf; // size of one index
    unsigned int               IndexBufferOffset;     // offset in current binded index buffer
    uint32_t                   IndexBufferUID;
    unsigned int               IndexBufferHandle;
    uint32_t                   VertexBufferUIDs[MAX_VERTEX_BUFFER_SLOTS];
    unsigned int               VertexBufferHandles[MAX_VERTEX_BUFFER_SLOTS];
    ptrdiff_t                  VertexBufferOffsets[MAX_VERTEX_BUFFER_SLOTS];

    uint32_t CurrentQueryUID[QUERY_TYPE_MAX];

    struct
    {
        unsigned int PackAlignment;
        unsigned int UnpackAlignment;
    } PixelStore;

    COLOR_CLAMP ColorClamp;

    SBlendingStateInfo BlendState; // current blend state
    float              BlendColor[4];
    unsigned int       SampleMask[4];
    bool               bSampleMaskEnabled;
    bool               bLogicOpEnabled;

    SRasterizerStateInfo RasterizerState; // current rasterizer state
    bool                 bPolygonOffsetEnabled;
    unsigned int         CullFace;

    SDepthStencilStateInfo DepthStencilState; // current depth-stencil state
    unsigned int           StencilRef;

    ARenderPass*              CurrentRenderPass;
    int                      CurrentSubpass;
    SRect2D                  CurrentRenderPassRenderArea;
    AFramebufferGLImpl const* CurrentFramebuffer;

    TArray<SClearColorValue, MAX_COLOR_ATTACHMENTS> ColorAttachmentClearValues;
    SClearDepthStencilValue                         DepthStencilAttachmentClearValue;

    struct SAttachmentUse
    {
        int FirstSubpass;
        int LastSubpass;
    };
    TArray<SAttachmentUse, MAX_COLOR_ATTACHMENTS> ColorAttachmentSubpassUse;
    SAttachmentUse                                DepthStencilAttachmentSubpassUse;

    float CurrentViewport[4];
    float CurrentDepthRange[2];

    SRect2D CurrentScissor;

    TRef<AFramebufferCacheGLImpl> pFramebufferCache;
};

} // namespace RenderCore
