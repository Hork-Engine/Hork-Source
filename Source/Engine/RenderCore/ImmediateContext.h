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

#include "BufferView.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "TransformFeedback.h"
#include "Query.h"

#include <Core/Public/Logger.h>

#include <stddef.h>

namespace RenderCore {

enum CLIP_CONTROL : uint8_t
{
    CLIP_CONTROL_OPENGL,
    CLIP_CONTROL_DIRECTX
};

enum VIEWPORT_ORIGIN : uint8_t
{
    VIEWPORT_ORIGIN_TOP_LEFT,
    VIEWPORT_ORIGIN_BOTTOM_LEFT
};

struct SImmediateContextCreateInfo
{
    CLIP_CONTROL ClipControl;

    /** Viewport and Scissor origin */
    VIEWPORT_ORIGIN ViewportOrigin;

    int SwapInterval;

    SDL_Window * Window;
};

enum CLIENT_WAIT_STATUS : uint8_t
{
    CLIENT_WAIT_ALREADY_SIGNALED = 0,             /// Indicates that sync was signaled at the time that ClientWait was called
    CLIENT_WAIT_TIMEOUT_EXPIRED = 1,              /// Indicates that at least timeout nanoseconds passed and sync did not become signaled.
    CLIENT_WAIT_CONDITION_SATISFIED = 2,          /// Indicates that sync was signaled before the timeout expired.
    CLIENT_WAIT_FAILED = 3                        /// Indicates that an error occurred.
};

enum BARRIER_BIT : uint32_t
{
    VERTEX_ATTRIB_ARRAY_BARRIER_BIT     = 0x00000001,
    ELEMENT_ARRAY_BARRIER_BIT           = 0x00000002,
    UNIFORM_BARRIER_BIT                 = 0x00000004,
    TEXTURE_FETCH_BARRIER_BIT           = 0x00000008,
    SHADER_IMAGE_ACCESS_BARRIER_BIT     = 0x00000020,
    COMMAND_BARRIER_BIT                 = 0x00000040,
    PIXEL_BUFFER_BARRIER_BIT            = 0x00000080,
    TEXTURE_UPDATE_BARRIER_BIT          = 0x00000100,
    BUFFER_UPDATE_BARRIER_BIT           = 0x00000200,
    FRAMEBUFFER_BARRIER_BIT             = 0x00000400,
    TRANSFORM_FEEDBACK_BARRIER_BIT      = 0x00000800,
    ATOMIC_COUNTER_BARRIER_BIT          = 0x00001000,
    SHADER_STORAGE_BARRIER_BIT          = 0x2000,
    CLIENT_MAPPED_BUFFER_BARRIER_BIT    = 0x00004000,
    QUERY_BUFFER_BARRIER_BIT            = 0x00008000
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

typedef struct _SyncObject * SyncObject;

struct SBufferCopy
{
    size_t    SrcOffset;
    size_t    DstOffset;
    size_t    SizeInBytes;
};

struct SBufferClear
{
    size_t    Offset;
    size_t    SizeInBytes;
};

struct SBlitRectangle
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

struct SViewport
{
    float   X;
    float   Y;
    float   Width;
    float   Height;
    float   MinDepth;
    float   MaxDepth;
};

struct SDrawCmd
{
    unsigned int VertexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartVertexLocation;
    unsigned int StartInstanceLocation;
};

struct SDrawIndexedCmd
{
    unsigned int IndexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartIndexLocation;
    int BaseVertexLocation;
    unsigned int StartInstanceLocation;
};

struct SDrawIndirectCmd
{
    unsigned int VertexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartVertexLocation;
    unsigned int StartInstanceLocation;   // Since GL v4.0, ignored on older versions
};

struct SDrawIndexedIndirectCmd
{
    unsigned int IndexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartIndexLocation;
    unsigned int BaseVertexLocation;
    unsigned int StartInstanceLocation;
};

struct SDispatchIndirectCmd
{
    unsigned int ThreadGroupCountX;
    unsigned int ThreadGroupCountY;
    unsigned int ThreadGroupCountZ;
};

struct SRenderPassBegin
{
    IRenderPass const * pRenderPass;
    IFramebuffer const * pFramebuffer;
    SRect2D RenderArea;
    SClearColorValue const * pColorClearValues;
    SClearDepthStencilValue const * pDepthStencilClearValue;
};

struct SClearValue
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
    };// Data;
};

class IResourceTable : public IDeviceObject
{
public:
    virtual void BindTexture( unsigned int Slot, ITextureBase const * Texture ) = 0;

    virtual void BindImage( unsigned int Slot, ITextureBase const * Texture, uint16_t Lod = 0, bool bLayered = false, uint16_t LayerIndex = 0 ) = 0;

    virtual void BindBuffer( int Slot, IBuffer const * Buffer, size_t Offset = 0, size_t Size = 0 ) = 0;
};

/// Immediate render context
class IImmediateContext : public IObjectInterface
{
public:
    virtual void MakeCurrent() = 0;

    static IImmediateContext * GetCurrent() { return Current; }

    virtual void SetSwapChainResolution( int _Width, int _Height ) = 0;

    virtual CLIP_CONTROL GetClipControl() const = 0;

    virtual VIEWPORT_ORIGIN GetViewportOrigin() const = 0;

    virtual IFramebuffer * GetDefaultFramebuffer() = 0;

    virtual class IDevice * GetDevice() = 0;


    //
    // Pipeline
    //

    virtual void BindPipeline( IPipeline * _Pipeline, int _Subpass = 0 ) = 0;

    //
    // Vertex & Index buffers
    //

    virtual void BindVertexBuffer( unsigned int _InputSlot, /* optional */ IBuffer const * _VertexBuffer, unsigned int _Offset = 0 ) = 0;

    virtual void BindVertexBuffers( unsigned int _StartSlot, unsigned int _NumBuffers, /* optional */ IBuffer * const * _VertexBuffers, uint32_t const * _Offsets = nullptr ) = 0;

    virtual void BindIndexBuffer( IBuffer const * _IndexBuffer, INDEX_TYPE _Type, unsigned int _Offset = 0 ) = 0;

    //
    // Shader resources
    //

    virtual IResourceTable * GetRootResourceTable() = 0;

    virtual void BindResourceTable( IResourceTable * _ResourceTable ) = 0;

    //
    // Viewport
    //

    virtual void SetViewport( SViewport const & _Viewport ) = 0;

    virtual void SetViewportArray( uint32_t _NumViewports, SViewport const * _Viewports ) = 0;

    virtual void SetViewportArray( uint32_t _FirstIndex, uint32_t _NumViewports, SViewport const * _Viewports ) = 0;

    virtual void SetViewportIndexed( uint32_t _Index, SViewport const & _Viewport ) = 0;

    //
    // Scissor
    //

    virtual void SetScissor( /* optional */ SRect2D const & _Scissor ) = 0;

    virtual void SetScissorArray( uint32_t _NumScissors, SRect2D const * _Scissors ) = 0;

    virtual void SetScissorArray( uint32_t _FirstIndex, uint32_t _NumScissors, SRect2D const * _Scissors ) = 0;

    virtual void SetScissorIndexed( uint32_t _Index, SRect2D const & _Scissor ) = 0;

    //
    // Render pass
    //

    virtual void BeginRenderPass( SRenderPassBegin const & _RenderPassBegin ) = 0;

    virtual void EndRenderPass() = 0;

    //
    // Transform feedback
    //

    virtual void BindTransformFeedback( ITransformFeedback * _TransformFeedback ) = 0;

    virtual void BeginTransformFeedback( PRIMITIVE_TOPOLOGY _OutputPrimitive ) = 0;

    virtual void ResumeTransformFeedback() = 0;

    virtual void PauseTransformFeedback() = 0;

    virtual void EndTransformFeedback() = 0;

    //
    // Draw
    //

    /// Draw non-indexed primitives.
    virtual void Draw( SDrawCmd const * _Cmd ) = 0;

    /// Draw indexed primitives.
    virtual void Draw( SDrawIndexedCmd const * _Cmd ) = 0;

    /// Draw from transform feedback
    virtual void Draw( ITransformFeedback * _TransformFeedback, unsigned int _InstanceCount = 1, unsigned int _StreamIndex = 0 ) = 0;

    /// Draw non-indexed GPU-generated primitives. From client memory.
    virtual void DrawIndirect( SDrawIndirectCmd const * _Cmd ) = 0;

    /// Draw indexed GPU-generated primitives. From client memory.
    virtual void DrawIndirect( SDrawIndexedIndirectCmd const * _Cmd ) = 0;

    /// Draw GPU-generated primitives. From indirect buffer.
    virtual void DrawIndirect( IBuffer * _DrawIndirectBuffer, unsigned int _AlignedByteOffset, bool _Indexed ) = 0;

    /// Draw non-indexed, non-instanced primitives.
    virtual void MultiDraw( unsigned int _DrawCount, const unsigned int * _VertexCount, const unsigned int * _StartVertexLocations ) = 0;

    /// Draw indexed, non-instanced primitives.
    virtual void MultiDraw( unsigned int _DrawCount, const unsigned int * _IndexCount, const void * const * _IndexByteOffsets, const int * _BaseVertexLocations = nullptr ) = 0;

    /// Draw instanced, GPU-generated primitives. From client memory.
    virtual void MultiDrawIndirect( unsigned int _DrawCount, SDrawIndirectCmd const * _Cmds, unsigned int _Stride ) = 0;

    /// Draw indexed, instanced, GPU-generated primitives. From client memory.
    virtual void MultiDrawIndirect( unsigned int _DrawCount, SDrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride ) = 0;

    // TODO?
    // Draw instanced, GPU-generated primitives. From indirect buffer.
    //void MultiDrawIndirect( IBuffer * _DrawIndirectBuffer,
    //                        unsigned int _DrawCount,
    //                        const void * _Cmds,
    //                        unsigned int _Stride ) = 0;

    // TODO?
    // Draw indexed, instanced, GPU-generated primitives. From indirect buffer.
    //void MultiDrawIndirect( IBuffer * _DrawIndirectBuffer,
    //                        unsigned int _DrawCount,
    //                        unsigned int * _AlignedByteOffsets,
    //                        unsigned int _Stride ) = 0;

    //
    // Dispatch compute
    //

    /// Launch one or more compute work groups
    virtual void DispatchCompute( unsigned int _ThreadGroupCountX,
                                  unsigned int _ThreadGroupCountY,
                                  unsigned int _ThreadGroupCountZ ) = 0;

    virtual void DispatchCompute( SDispatchIndirectCmd const * _Cmd ) = 0;

    /// Launch one or more compute work groups using parameters stored in a dispatch indirect buffer
    virtual void DispatchComputeIndirect( IBuffer * _DispatchIndirectBuffer, unsigned int _AlignedByteOffset ) = 0;

    //
    // Query
    //

    virtual void BeginQuery( IQueryPool * _QueryPool, uint32_t _QueryID, uint32_t _StreamIndex = 0 ) = 0;

    virtual void EndQuery( IQueryPool * _QueryPool, uint32_t _StreamIndex = 0 ) = 0;

    virtual void RecordTimeStamp( IQueryPool * _QueryPool, uint32_t _QueryID ) = 0;

    virtual void CopyQueryPoolResultsAvailable( IQueryPool * _QueryPool,
                                                uint32_t _FirstQuery,
                                                uint32_t _QueryCount,
                                                IBuffer * _DstBuffer,
                                                size_t _DstOffst,
                                                size_t _DstStride,
                                                bool _QueryResult64Bit ) = 0;

    virtual void CopyQueryPoolResults( IQueryPool * _QueryPool,
                                       uint32_t _FirstQuery,
                                       uint32_t _QueryCount,
                                       IBuffer * _DstBuffer,
                                       size_t _DstOffst,
                                       size_t _DstStride,
                                       QUERY_RESULT_FLAGS _Flags ) = 0;

    //
    // Conditional render
    //

    virtual void BeginConditionalRender( IQueryPool * _QueryPool, uint32_t _QueryID, CONDITIONAL_RENDER_MODE _Mode ) = 0;

    virtual void EndConditionalRender() = 0;

    //
    // Synchronization
    //

    virtual SyncObject FenceSync() = 0;

    virtual void RemoveSync( SyncObject _Sync ) = 0;

    virtual CLIENT_WAIT_STATUS ClientWait( SyncObject _Sync, /* optional */ uint64_t _TimeOutNanoseconds = 0xFFFFFFFFFFFFFFFF ) = 0;

    virtual void ServerWait( SyncObject _Sync ) = 0;

    virtual bool IsSignaled( SyncObject _Sync ) = 0;

    virtual void Flush() = 0;

    virtual void Barrier( int _BarrierBits ) = 0;

    virtual void BarrierByRegion( int _BarrierBits ) = 0;

    virtual void TextureBarrier() = 0;

    //
    // DYNAMIC STATE
    //

    virtual void DynamicState_BlendingColor( /* optional */ const float _ConstantColor[4] ) = 0;

    virtual void DynamicState_SampleMask( /* optional */ const uint32_t _SampleMask[4] ) = 0;

    virtual void DynamicState_StencilRef( uint32_t _StencilRef ) = 0;

    virtual void SetLineWidth( float _Width ) = 0;

    //
    // Copy
    //

    virtual void CopyBuffer( IBuffer * _SrcBuffer, IBuffer * _DstBuffer ) = 0;

    virtual void CopyBufferRange( IBuffer * _SrcBuffer, IBuffer * _DstBuffer, uint32_t _NumRanges, SBufferCopy const * _Ranges ) = 0;

    /// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP
    virtual bool CopyBufferToTexture( IBuffer const * _SrcBuffer,
                                      ITexture * _DstTexture,
                                      STextureRect const & _Rectangle,
                                      DATA_FORMAT _Format,
                                      size_t _CompressedDataByteLength,       /// only for compressed images
                                      size_t _SourceByteOffset,               /// offset in source buffer
                                      unsigned int _Alignment ) = 0;              /// Specifies alignment of source data

    /// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP, TEXTURE_CUBE_MAP_ARRAY or TEXTURE_RECT_GL
    /// Texture cannot be multisample
    virtual void CopyTextureToBuffer( ITexture const * _SrcTexture,
                                      IBuffer * _DstBuffer,
                                      STextureRect const & _Rectangle,
                                      DATA_FORMAT _Format,   /// how texture will be store in destination buffer
                                      size_t _SizeInBytes,       /// copying data byte length
                                      size_t _DstByteOffset,        /// offset in destination buffer
                                      unsigned int _Alignment ) = 0;       /// Specifies alignment of destination data

    virtual void CopyTextureRect( ITexture const * _SrcTexture,
                                  ITexture * _DstTexture,
                                  uint32_t _NumCopies,
                                  TextureCopy const * Copies ) = 0;

    /// Only for TEXTURE_1D TEXTURE_1D_ARRAY TEXTURE_2D TEXTURE_2D_ARRAY TEXTURE_3D TEXTURE_CUBE_MAP TEXTURE_RECT_GL
    /// Выборка элемента массива из TEXTURE_1D_ARRAY осуществляется через _OffsetY
    /// Выборка элемента массива из TEXTURE_2D_ARRAY осуществляется через _OffsetZ
    /// Выборка слоя из TEXTURE_3D осуществляется через _OffsetZ
    /// Выборка грани кубической текстуры из TEXTURE_CUBE_MAP осуществляется через _OffsetZ
    virtual bool CopyFramebufferToTexture( IFramebuffer const * _SrcFramebuffer,
                                           ITexture * _DstTexture,
                                           FRAMEBUFFER_ATTACHMENT _Attachment,
                                           STextureOffset const & _Offset,
                                           SRect2D const & _SrcRect,
                                           unsigned int _Alignment ) = 0;

    virtual void CopyFramebufferToBuffer( IFramebuffer const * _SrcFramebuffer,
                                          IBuffer * _DstBuffer,
                                          FRAMEBUFFER_ATTACHMENT _Attachment,
                                          SRect2D const & _SrcRect,
                                          FRAMEBUFFER_CHANNEL _FramebufferChannel,
                                          FRAMEBUFFER_OUTPUT _FramebufferOutput,
                                          COLOR_CLAMP _ColorClamp,
                                          size_t _SizeInBytes,
                                          size_t _DstByteOffset,
                                          unsigned int _Alignment ) = 0;            /// Specifies alignment of destination data


    /// Copy source framebuffer to current
    /// NOTE: Next things can affect:
    /// SCISSOR
    /// Pixel Owner Ship (for default framebuffer only)
    /// Conditional Rendering
    virtual bool BlitFramebuffer( IFramebuffer const * _SrcFramebuffer,
                                  FRAMEBUFFER_ATTACHMENT _SrcAttachment,
                                  uint32_t _NumRectangles,
                                  SBlitRectangle const * _Rectangles,
                                  FRAMEBUFFER_MASK _Mask,
                                  bool _LinearFilter ) = 0;

    //
    // Clear
    //

    /// Fill all of buffer object's data store with a fixed value.
    /// If _ClearValue is NULL, then the buffer's data store is filled with zeros.
    virtual void ClearBuffer( IBuffer * _Buffer, BUFFER_VIEW_PIXEL_FORMAT _InternalFormat, DATA_FORMAT _Format, const SClearValue * _ClearValue ) = 0;

    /// Fill all or part of buffer object's data store with a fixed value.
    /// If _ClearValue is NULL, then the subrange of the buffer's data store is filled with zeros.
    virtual void ClearBufferRange( IBuffer * _Buffer, BUFFER_VIEW_PIXEL_FORMAT _InternalFormat, uint32_t _NumRanges, SBufferClear const * _Ranges, DATA_FORMAT _Format, const SClearValue * _ClearValue ) = 0;

    /// Fill texture image with a fixed value.
    /// If _ClearValue is NULL, then the texture image is filled with zeros.
    virtual void ClearTexture( ITexture * _Texture, uint16_t _Lod, DATA_FORMAT _Format, const SClearValue * _ClearValue ) = 0;

    /// Fill all or part of texture image with a fixed value.
    /// If _ClearValue is NULL, then the rect of the texture image is filled with zeros.
    virtual void ClearTextureRect( ITexture * _Texture,
                                   uint32_t _NumRectangles,
                                   STextureRect const * _Rectangles,
                                   DATA_FORMAT _Format,
                                   const SClearValue * _ClearValue ) = 0;

    virtual void ClearFramebufferAttachments( IFramebuffer * _Framebuffer,
                                              /* optional */ unsigned int * _ColorAttachments,
                                              /* optional */ unsigned int _NumColorAttachments,
                                              /* optional */ SClearColorValue const * _ColorClearValues,
                                              /* optional */ SClearDepthStencilValue const * _DepthStencilClearValue,
                                              /* optional */ SRect2D const * _Rect ) = 0;

protected:
    static IImmediateContext * Current;
};

/*


Objects:
    Device
    ImmediateContext
    Buffer
    Texture
    ShaderModule
    Framebuffer
    Pipeline
    RenderPass


This objects can be shared:
    Buffer, Texture, ShaderModule


*/

}
