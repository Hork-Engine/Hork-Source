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

#include "BufferView.h"
#include "Texture.h"
#include "SparseTexture.h"
#include "Pipeline.h"
#include "TransformFeedback.h"
#include "Query.h"
#include "FGRenderPass.h"

#include <Platform/Logger.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

enum CLIENT_WAIT_STATUS : uint8_t
{
    CLIENT_WAIT_ALREADY_SIGNALED    = 0, /// Indicates that sync was signaled at the time that ClientWait was called
    CLIENT_WAIT_TIMEOUT_EXPIRED     = 1, /// Indicates that at least timeout nanoseconds passed and sync did not become signaled.
    CLIENT_WAIT_CONDITION_SATISFIED = 2, /// Indicates that sync was signaled before the timeout expired.
    CLIENT_WAIT_FAILED              = 3  /// Indicates that an error occurred.
};

enum BARRIER_BIT : uint32_t
{
    VERTEX_ATTRIB_ARRAY_BARRIER_BIT  = 0x00000001,
    ELEMENT_ARRAY_BARRIER_BIT        = 0x00000002,
    UNIFORM_BARRIER_BIT              = 0x00000004,
    TEXTURE_FETCH_BARRIER_BIT        = 0x00000008,
    SHADER_IMAGE_ACCESS_BARRIER_BIT  = 0x00000020,
    COMMAND_BARRIER_BIT              = 0x00000040,
    PIXEL_BUFFER_BARRIER_BIT         = 0x00000080,
    TEXTURE_UPDATE_BARRIER_BIT       = 0x00000100,
    BUFFER_UPDATE_BARRIER_BIT        = 0x00000200,
    FRAMEBUFFER_BARRIER_BIT          = 0x00000400,
    TRANSFORM_FEEDBACK_BARRIER_BIT   = 0x00000800,
    ATOMIC_COUNTER_BARRIER_BIT       = 0x00001000,
    SHADER_STORAGE_BARRIER_BIT       = 0x2000,
    CLIENT_MAPPED_BUFFER_BARRIER_BIT = 0x00004000,
    QUERY_BUFFER_BARRIER_BIT         = 0x00008000
};

enum INDEX_TYPE : uint8_t
{
    INDEX_TYPE_UINT16 = 0,
    INDEX_TYPE_UINT32 = 1
};

enum CONDITIONAL_RENDER_MODE : uint8_t
{
    CONDITIONAL_RENDER_QUERY_WAIT,
    CONDITIONAL_RENDER_QUERY_NO_WAIT,
    CONDITIONAL_RENDER_QUERY_BY_REGION_WAIT,
    CONDITIONAL_RENDER_QUERY_BY_REGION_NO_WAIT,
    CONDITIONAL_RENDER_QUERY_WAIT_INVERTED,
    CONDITIONAL_RENDER_QUERY_NO_WAIT_INVERTED,
    CONDITIONAL_RENDER_QUERY_BY_REGION_WAIT_INVERTED,
    CONDITIONAL_RENDER_QUERY_BY_REGION_NO_WAIT_INVERTED
};

enum FRAMEBUFFER_BLIT_MASK : uint8_t
{
    FB_MASK_COLOR         = (1 << 0),
    FB_MASK_DEPTH         = (1 << 1),
    FB_MASK_STENCIL       = (1 << 2),
    FB_MASK_DEPTH_STENCIL = (FB_MASK_DEPTH | FB_MASK_STENCIL),
    FB_MASK_ALL           = 0xff
};

typedef struct _SyncObject* SyncObject;

struct BufferCopy
{
    size_t SrcOffset;
    size_t DstOffset;
    size_t SizeInBytes;
};

struct BufferClear
{
    size_t Offset;
    size_t SizeInBytes;
};

struct BlitRectangle
{
    uint16_t SrcX;
    uint16_t SrcY;
    uint16_t SrcWidth;
    uint16_t SrcHeight;
    uint16_t DstX;
    uint16_t DstY;
    uint16_t DstWidth;
    uint16_t DstHeight;
};

struct Viewport
{
    float X;
    float Y;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
};

struct DrawCmd
{
    unsigned int VertexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartVertexLocation;
    unsigned int StartInstanceLocation;
};

struct DrawIndexedCmd
{
    unsigned int IndexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartIndexLocation;
    int          BaseVertexLocation;
    unsigned int StartInstanceLocation;
};

struct DrawIndirectCmd
{
    unsigned int VertexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartVertexLocation;
    unsigned int StartInstanceLocation; // Since GL v4.0, ignored on older versions
};

struct DrawIndexedIndirectCmd
{
    unsigned int IndexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartIndexLocation;
    unsigned int BaseVertexLocation;
    unsigned int StartInstanceLocation;
};

struct DispatchIndirectCmd
{
    unsigned int ThreadGroupCountX;
    unsigned int ThreadGroupCountY;
    unsigned int ThreadGroupCountZ;
};

struct ClearValue
{
    union
    {
        struct
        {
            int8_t R;
        } Byte1;

        struct
        {
            int8_t R;
            int8_t G;
        } Byte2;

        struct
        {
            int8_t R;
            int8_t G;
            int8_t B;
        } Byte3;

        struct
        {
            int8_t R;
            int8_t G;
            int8_t B;
            int8_t A;
        } Byte4;

        struct
        {
            uint8_t R;
        } UByte1;

        struct
        {
            uint8_t R;
            uint8_t G;
        } UByte2;

        struct
        {
            uint8_t R;
            uint8_t G;
            uint8_t B;
        } UByte3;

        struct
        {
            uint8_t R;
            uint8_t G;
            uint8_t B;
            uint8_t A;
        } UByte4;

        struct
        {
            int16_t R;
        } Short1;

        struct
        {
            int16_t R;
            int16_t G;
        } Short2;

        struct
        {
            int16_t R;
            int16_t G;
            int16_t B;
        } Short3;

        struct
        {
            int16_t R;
            int16_t G;
            int16_t B;
            int16_t A;
        } Short4;

        struct
        {
            uint16_t R;
        } UShort1;

        struct
        {
            uint16_t R;
            uint16_t G;
        } UShort2;

        struct
        {
            uint16_t R;
            uint16_t G;
            uint16_t B;
        } UShort3;

        struct
        {
            uint16_t R;
            uint16_t G;
            uint16_t B;
            uint16_t A;
        } UShort4;

        struct
        {
            int32_t R;
        } Int1;

        struct
        {
            int32_t R;
            int32_t G;
        } Int2;

        struct
        {
            int32_t R;
            int32_t G;
            int32_t B;
        } Int3;

        struct
        {
            int32_t R;
            int32_t G;
            int32_t B;
            int32_t A;
        } Int4;

        struct
        {
            uint32_t R;
        } UInt1;

        struct
        {
            uint32_t R;
            uint32_t G;
        } UInt2;

        struct
        {
            uint32_t R;
            uint32_t G;
            uint32_t B;
        } UInt3;

        struct
        {
            uint32_t R;
            uint32_t G;
            uint32_t B;
            uint32_t A;
        } UInt4;

        struct
        {
            uint16_t R;
        } Half1;

        struct
        {
            uint16_t R;
            uint16_t G;
        } Half2;

        struct
        {
            uint16_t R;
            uint16_t G;
            uint16_t B;
        } Half3;

        struct
        {
            uint16_t R;
            uint16_t G;
            uint16_t B;
            uint16_t A;
        } Half4;

        struct
        {
            float R;
        } Float1;

        struct
        {
            float R;
            float G;
        } Float2;

        struct
        {
            float R;
            float G;
            float B;
        } Float3;

        struct
        {
            float R;
            float G;
            float B;
            float A;
        } Float4;
    }; // Data;
};

class IResourceTable : public IDeviceObject
{
public:
    IResourceTable(IDevice* pDevice, bool bIsRoot) :
        IDeviceObject(pDevice, DEVICE_OBJECT_TYPE_RESOURCE_TABLE, bIsRoot)
    {}

    void BindTexture(unsigned int Slot, ITexture* pTexture)
    {
        BindTexture(Slot, pTexture->GetShaderResourceView());
    }

    virtual void BindTexture(unsigned int Slot, ITextureView* pShaderResourceView)          = 0;
    virtual void BindTexture(unsigned int Slot, IBufferView* pShaderResourceView)           = 0;
    virtual void BindImage(unsigned int Slot, ITextureView* pUnorderedAccessView)                 = 0;
    virtual void BindBuffer(int Slot, IBuffer const* pBuffer, size_t Offset = 0, size_t Size = 0) = 0;
};

class IImmediateContext : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_IMMEDIATE_CONTEXT;

    IImmediateContext(IDevice* pDevice) :
        IDeviceObject(pDevice, PROXY_TYPE, true)
    {}

    virtual void ExecuteFrameGraph(class FrameGraph* pFrameGraph) = 0;

    //
    // Pipeline
    //

    virtual void BindPipeline(IPipeline* pPipeline) = 0;

    //
    // Vertex & Index buffers
    //

    virtual void BindVertexBuffer(unsigned int InputSlot, /* optional */ IBuffer const* pVertexBuffer, unsigned int Offset = 0) = 0;

    virtual void BindVertexBuffers(unsigned int StartSlot, unsigned int NumBuffers, /* optional */ IBuffer* const* ppVertexBuffers, uint32_t const* pOffsets = nullptr) = 0;

    virtual void BindIndexBuffer(IBuffer const* pIndexBuffer, INDEX_TYPE Type, unsigned int Offset = 0) = 0;

    //
    // Shader resources
    //

    virtual IResourceTable* GetRootResourceTable() = 0;

    virtual void BindResourceTable(IResourceTable* pResourceTable) = 0;

    //
    // Viewport
    //

    virtual void SetViewport(Viewport const& Viewport) = 0;

    virtual void SetViewportArray(uint32_t NumViewports, Viewport const* pViewports) = 0;

    virtual void SetViewportArray(uint32_t FirstIndex, uint32_t NumViewports, Viewport const* pViewports) = 0;

    virtual void SetViewportIndexed(uint32_t Index, Viewport const& Viewport) = 0;

    //
    // Scissor
    //

    virtual void SetScissor(Rect2D const& Scissor) = 0;

    virtual void SetScissorArray(uint32_t NumScissors, Rect2D const* pScissors) = 0;

    virtual void SetScissorArray(uint32_t FirstIndex, uint32_t NumScissors, Rect2D const* pScissors) = 0;

    virtual void SetScissorIndexed(uint32_t Index, Rect2D const& Scissor) = 0;

    //
    // Transform feedback
    //

    virtual void BindTransformFeedback(ITransformFeedback* pTransformFeedback) = 0;

    virtual void BeginTransformFeedback(PRIMITIVE_TOPOLOGY OutputPrimitive) = 0;

    virtual void ResumeTransformFeedback() = 0;

    virtual void PauseTransformFeedback() = 0;

    virtual void EndTransformFeedback() = 0;

    //
    // Draw
    //

    /// Draw non-indexed primitives.
    virtual void Draw(DrawCmd const* pCmd) = 0;

    /// Draw indexed primitives.
    virtual void Draw(DrawIndexedCmd const* pCmd) = 0;

    /// Draw from transform feedback
    virtual void Draw(ITransformFeedback* pTransformFeedback, unsigned int InstanceCount = 1, unsigned int StreamIndex = 0) = 0;

#if 0
    /// Draw non-indexed GPU-generated primitives. From client memory.
    virtual void DrawIndirect( DrawIndirectCmd const * _Cmd ) = 0;

    /// Draw indexed GPU-generated primitives. From client memory.
    virtual void DrawIndirect( DrawIndexedIndirectCmd const * _Cmd ) = 0;
#endif

    /// Draw non-indexed GPU-generated primitives. From indirect buffer.
    virtual void DrawIndirect(IBuffer* pDrawIndirectBuffer, unsigned int AlignedByteOffset) = 0;

    /// Draw indexed GPU-generated primitives. From indirect buffer.
    virtual void DrawIndexedIndirect(IBuffer* pDrawIndirectBuffer, unsigned int AlignedByteOffset) = 0;

    /// Draw non-indexed, non-instanced primitives.
    virtual void MultiDraw(unsigned int DrawCount, const unsigned int* VertexCount, const unsigned int* StartVertexLocations) = 0;

    /// Draw indexed, non-instanced primitives.
    virtual void MultiDraw(unsigned int DrawCount, const unsigned int* IndexCount, const void* const* IndexByteOffsets, const int* BaseVertexLocations = nullptr) = 0;

#if 0
    /// Draw instanced, GPU-generated primitives. From client memory.
    virtual void MultiDrawIndirect( unsigned int DrawCount, DrawIndirectCmd const * _Cmds, unsigned int _Stride ) = 0;

    /// Draw indexed, instanced, GPU-generated primitives. From client memory.
    virtual void MultiDrawIndirect( unsigned int DrawCount, DrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride ) = 0;
#endif

    /// Draw non-indexed GPU-generated primitives. From indirect buffer.
    virtual void MultiDrawIndirect(unsigned int DrawCount, IBuffer* pDrawIndirectBuffer, unsigned int AlignedByteOffset, unsigned int Stride) = 0;

    /// Draw indexed GPU-generated primitives. From indirect buffer.
    virtual void MultiDrawIndexedIndirect(unsigned int DrawCount, IBuffer* pDrawIndirectBuffer, unsigned int AlignedByteOffset, unsigned int Stride) = 0;

    //
    // Dispatch compute
    //

    /// Launch one or more compute work groups
    virtual void DispatchCompute(unsigned int ThreadGroupCountX,
                                 unsigned int ThreadGroupCountY,
                                 unsigned int ThreadGroupCountZ) = 0;

    virtual void DispatchCompute(DispatchIndirectCmd const* pCmd) = 0;

    /// Launch one or more compute work groups using parameters stored in a dispatch indirect buffer
    virtual void DispatchComputeIndirect(IBuffer* pDispatchIndirectBuffer, unsigned int AlignedByteOffset) = 0;

    //
    // Query
    //

    virtual void BeginQuery(IQueryPool* QueryPool, uint32_t QueryID, uint32_t StreamIndex = 0) = 0;

    virtual void EndQuery(IQueryPool* QueryPool, uint32_t StreamIndex = 0) = 0;

    virtual void RecordTimeStamp(IQueryPool* QueryPool, uint32_t QueryID) = 0;

    virtual void CopyQueryPoolResultsAvailable(IQueryPool* QueryPool,
                                               uint32_t    FirstQuery,
                                               uint32_t    QueryCount,
                                               IBuffer*    pDstBuffer,
                                               size_t      DstOffst,
                                               size_t      DstStride,
                                               bool        QueryResult64Bit) = 0;

    virtual void CopyQueryPoolResults(IQueryPool*        QueryPool,
                                      uint32_t           FirstQuery,
                                      uint32_t           QueryCount,
                                      IBuffer*           pDstBuffer,
                                      size_t             DstOffst,
                                      size_t             DstStride,
                                      QUERY_RESULT_FLAGS Flags) = 0;

    //
    // Conditional render
    //

    virtual void BeginConditionalRender(IQueryPool* QueryPool, uint32_t QueryID, CONDITIONAL_RENDER_MODE Mode) = 0;

    virtual void EndConditionalRender() = 0;

    //
    // Synchronization
    //

    virtual SyncObject FenceSync() = 0;

    virtual void RemoveSync(SyncObject Sync) = 0;

    virtual CLIENT_WAIT_STATUS ClientWait(SyncObject Sync, uint64_t TimeOutNanoseconds = 0xFFFFFFFFFFFFFFFF) = 0;

    virtual void ServerWait(SyncObject Sync) = 0;

    virtual bool IsSignaled(SyncObject Sync) = 0;

    virtual void Flush() = 0;

    virtual void Barrier(int BarrierBits) = 0;

    virtual void BarrierByRegion(int BarrierBits) = 0;

    virtual void TextureBarrier() = 0;

    //
    // DYNAMIC STATE
    //

    virtual void DynamicState_BlendingColor(const float ConstantColor[4]) = 0;

    virtual void DynamicState_SampleMask(const uint32_t SampleMask[4]) = 0;

    virtual void DynamicState_StencilRef(uint32_t StencilRef) = 0;

    //
    // Copy
    //

    virtual void CopyBuffer(IBuffer* pSrcBuffer, IBuffer* pDstBuffer) = 0;

    virtual void CopyBufferRange(IBuffer* pSrcBuffer, IBuffer* pDstBuffer, uint32_t NumRanges, BufferCopy const* Ranges) = 0;

    /// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP
    virtual bool CopyBufferToTexture(IBuffer const*      pSrcBuffer,
                                     ITexture*           pDstTexture,
                                     TextureRect const& Rectangle,
                                     DATA_FORMAT         Format,
                                     size_t              CompressedDataSizeInBytes, /// only for compressed images
                                     size_t              SourceByteOffset,          /// offset in source buffer
                                     unsigned int        Alignment) = 0;                    /// Specifies alignment of source data

    /// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP, TEXTURE_CUBE_MAP_ARRAY
    /// Texture cannot be multisample
    virtual void CopyTextureToBuffer(ITexture const*     pSrcTexture,
                                     IBuffer*            pDstBuffer,
                                     TextureRect const& Rectangle,
                                     DATA_FORMAT         Format,         /// how texture will be store in destination buffer
                                     size_t              SizeInBytes,    /// copying data byte length
                                     size_t              DstByteOffset, /// offset in destination buffer
                                     unsigned int        Alignment) = 0;        /// Specifies alignment of destination data

    virtual void CopyTextureRect(ITexture const*    pSrcTexture,
                                 ITexture*          pDstTexture,
                                 uint32_t           NumCopies,
                                 TextureCopy const* Copies) = 0;

    //
    // Clear
    //

    /// Fill all of buffer object's data store with a fixed value.
    /// If _ClearValue is NULL, then the buffer's data store is filled with zeros.
    virtual void ClearBuffer(IBuffer* pBuffer, BUFFER_VIEW_PIXEL_FORMAT InternalFormat, DATA_FORMAT Format, const ClearValue* ClearValue) = 0;

    /// Fill all or part of buffer object's data store with a fixed value.
    /// If _ClearValue is NULL, then the subrange of the buffer's data store is filled with zeros.
    virtual void ClearBufferRange(IBuffer* pBuffer, BUFFER_VIEW_PIXEL_FORMAT InternalFormat, uint32_t NumRanges, BufferClear const* Ranges, DATA_FORMAT Format, const ClearValue* ClearValue) = 0;

    /// Fill texture image with a fixed value.
    /// If _ClearValue is NULL, then the texture image is filled with zeros.
    virtual void ClearTexture(ITexture* pTexture, uint16_t MipLevel, DATA_FORMAT Format, const ClearValue* ClearValue) = 0;

    /// Fill all or part of texture image with a fixed value.
    /// If _ClearValue is NULL, then the rect of the texture image is filled with zeros.
    virtual void ClearTextureRect(ITexture*           pTexture,
                                  uint32_t            NumRectangles,
                                  TextureRect const* Rectangles,
                                  DATA_FORMAT         Format,
                                  const ClearValue*  ClearValue) = 0;

    //
    // Read
    //

    /// Client-side call function. Read data to client memory.
    virtual void ReadTexture(ITexture*    pTexture,
                             uint16_t     MipLevel,
                             size_t       SizeInBytes,
                             unsigned int Alignment,
                             void*        pSysMem) = 0;

    /// Client-side call function. Read data to client memory.
    virtual void ReadTextureRect(ITexture*           pTexture,
                                 TextureRect const& Rectangle,
                                 size_t              SizeInBytes,
                                 unsigned int        Alignment,
                                 void*               pSysMem) = 0;

    /// Client-side call function. Write data from client memory.
    virtual bool WriteTexture(ITexture*    pTexture,
                              uint16_t     MipLevel,
                              size_t       SizeInBytes,
                              unsigned int Alignment,
                              const void*  pSysMem) = 0;

    /// Only for TEXTURE_1D TEXTURE_1D_ARRAY TEXTURE_2D TEXTURE_2D_ARRAY TEXTURE_3D
    /// Client-side call function. Write data from client memory.
    virtual bool WriteTextureRect(ITexture*           pTexture,
                                  TextureRect const& Rectangle,
                                  size_t              SizeInBytes,
                                  unsigned int        Alignment,
                                  const void*         pSysMem,
                                  size_t              RowPitch   = 0,
                                  size_t              DepthPitch = 0) = 0;

    //
    // Buffer
    //

    /// Client-side call function
    virtual void ReadBufferRange(IBuffer* pBuffer, size_t ByteOffset, size_t SizeInBytes, void* pSysMem) = 0;

    /// Client-side call function
    virtual void WriteBufferRange(IBuffer* pBuffer, size_t ByteOffset, size_t SizeInBytes, const void* pSysMem) = 0;

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
    virtual void* MapBufferRange(IBuffer*        pBuffer,
                                 size_t          RangeOffset,
                                 size_t          RangeSize,
                                 MAP_TRANSFER    ClientServerTransfer,
                                 MAP_INVALIDATE  Invalidate     = MAP_NO_INVALIDATE,
                                 MAP_PERSISTENCE Persistence    = MAP_NON_PERSISTENT,
                                 bool            FlushExplicit  = false,
                                 bool            Unsynchronized = false) = 0;

    /// Returns pointer to the entire buffer data.
    virtual void* MapBuffer(IBuffer*        pBuffer,
                            MAP_TRANSFER    ClientServerTransfer,
                            MAP_INVALIDATE  Invalidate     = MAP_NO_INVALIDATE,
                            MAP_PERSISTENCE Persistence    = MAP_NON_PERSISTENT,
                            bool            FlushExplicit  = false,
                            bool            Unsynchronized = false) = 0;

    /// After calling this function, you should not use
    /// the pointer returned by Map or MapRange again.
    virtual void UnmapBuffer(IBuffer* pBuffer) = 0;

    //
    // Sparse texture
    //

    virtual void SparseTextureCommitPage(ISparseTexture* pTexture,
                                         int             MipLevel,
                                         int             PageX,
                                         int             PageY,
                                         int             PageZ,
                                         DATA_FORMAT     Format, // Specifies a pixel format for the input data
                                         size_t          SizeInBytes,
                                         unsigned int    Alignment, // Specifies alignment of source data
                                         const void*     pSysMem) = 0;

    virtual void SparseTextureCommitRect(ISparseTexture*     pTexture,
                                         TextureRect const& Rectangle,
                                         DATA_FORMAT         Format, // Specifies a pixel format for the input data
                                         size_t              SizeInBytes,
                                         unsigned int        Alignment, // Specifies alignment of source data
                                         const void*         pSysMem) = 0;

    virtual void SparseTextureUncommitPage(ISparseTexture* pTexture, int MipLevel, int PageX, int PageY, int PageZ) = 0;

    virtual void SparseTextureUncommitRect(ISparseTexture* pTexture, TextureRect const& Rectangle) = 0;

    //
    // Query
    //

    virtual void GetQueryPoolResults(IQueryPool*        QueryPool,
                                     uint32_t           FirstQuery,
                                     uint32_t           QueryCount,
                                     size_t             DataSize,
                                     void*              pSysMem,
                                     size_t             DstStride,
                                     QUERY_RESULT_FLAGS Flags) = 0;

    void GetQueryPoolResult32(IQueryPool*        QueryPool,
                              uint32_t           QueryId,
                              uint32_t*          pResult,
                              QUERY_RESULT_FLAGS Flags)
    {
        auto flags = Flags & ~QUERY_RESULT_64_BIT;
        GetQueryPoolResults(QueryPool, QueryId, 1, sizeof(*pResult), pResult, sizeof(*pResult), (QUERY_RESULT_FLAGS)flags);
    }

    void GetQueryPoolResult64(IQueryPool*        QueryPool,
                              uint32_t           QueryId,
                              uint64_t*          pResult,
                              QUERY_RESULT_FLAGS Flags)
    {
        auto flags = Flags | QUERY_RESULT_64_BIT;
        GetQueryPoolResults(QueryPool, QueryId, 1, sizeof(*pResult), pResult, sizeof(*pResult), (QUERY_RESULT_FLAGS)flags);
    }

    //
    // Misc
    //

    /// Only for TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP, or TEXTURE_CUBE_MAP_ARRAY.
    virtual void GenerateTextureMipLevels(ITexture* pTexture) = 0;

    //
    // Render pass
    //

    /// Only for TEXTURE_1D TEXTURE_1D_ARRAY TEXTURE_2D TEXTURE_2D_ARRAY TEXTURE_3D TEXTURE_CUBE_MAP
    virtual bool CopyFramebufferToTexture(FGRenderPassContext&   RenderPassContext,
                                          ITexture*             pDstTexture,
                                          int                   ColorAttachment,
                                          TextureOffset const& Offset,
                                          Rect2D const&        SrcRect,
                                          unsigned int          Alignment) = 0;

    virtual void CopyColorAttachmentToBuffer(FGRenderPassContext& RenderPassContext,
                                             IBuffer*            pDstBuffer,
                                             int                 SubpassAttachmentRef,
                                             Rect2D const&      SrcRect,
                                             FRAMEBUFFER_CHANNEL FramebufferChannel,
                                             FRAMEBUFFER_OUTPUT  FramebufferOutput,
                                             COLOR_CLAMP         ColorClamp,
                                             size_t              SizeInBytes,
                                             size_t              DstByteOffset,
                                             unsigned int        Alignment) = 0;

    virtual void CopyDepthAttachmentToBuffer(FGRenderPassContext& RenderPassContext,
                                             IBuffer*            pDstBuffer,
                                             Rect2D const&      SrcRect,
                                             size_t              SizeInBytes,
                                             size_t              DstByteOffset,
                                             unsigned int        Alignment) = 0;


    /// Copy source framebuffer to current
    /// NOTE: Next things can affect:
    /// SCISSOR
    /// Pixel Owner Ship (for default framebuffer only)
    /// Conditional Rendering
    virtual bool BlitFramebuffer(FGRenderPassContext&   RenderPassContext,
                                 int                   ColorAttachment,
                                 uint32_t              NumRectangles,
                                 BlitRectangle const* Rectangles,
                                 FRAMEBUFFER_BLIT_MASK Mask,
                                 bool                  LinearFilter) = 0;

    virtual void ClearAttachments(FGRenderPassContext&                           RenderPassContext,
                                  /* optional */ unsigned int*                  ColorAttachments,
                                  /* optional */ unsigned int                   NumColorAttachments,
                                  /* optional */ ClearColorValue const*        ColorClearValues,
                                  /* optional */ ClearDepthStencilValue const* DepthStencilClearValue,
                                  /* optional */ Rect2D const*                 Rect) = 0;

    /// Client-side call function
    virtual bool ReadFramebufferAttachment(FGRenderPassContext& RenderPassContext,
                                           int                 ColorAttachment,
                                           Rect2D const&      SrcRect,
                                           FRAMEBUFFER_CHANNEL FramebufferChannel,
                                           FRAMEBUFFER_OUTPUT  FramebufferOutput,
                                           COLOR_CLAMP         ColorClamp,
                                           size_t              SizeInBytes,
                                           unsigned int        Alignment, // Specifies alignment of destination data
                                           void*               pSysMem) = 0;

    virtual bool ReadFramebufferDepthStencilAttachment(FGRenderPassContext& RenderPassContext,
                                                       Rect2D const&      SrcRect,
                                                       size_t              SizeInBytes,
                                                       unsigned int        Alignment,
                                                       void*               pSysMem) = 0;
};

} // namespace RenderCore

HK_NAMESPACE_END
