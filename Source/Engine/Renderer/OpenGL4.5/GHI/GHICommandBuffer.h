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

#include "GHIDevice.h"
#include "GHIPipeline.h"
#include "GHIBuffer.h"
#include "GHISampler.h"
#include "GHITexture.h"
#include "GHIFramebuffer.h"
#include "GHIRenderPass.h"
#include "GHIQuery.h"

namespace GHI {

class State;
class TransformFeedback;

enum CLIENT_WAIT_STATUS : uint8_t
{
    CLIENT_WAIT_ALREADY_SIGNALED = 0,             /// Indicates that sync was signaled at the time that glClientWaitSync was called
    CLIENT_WAIT_TIMEOUT_EXPIRED = 1,              /// Indicates that at least timeout nanoseconds passed and sync did not become signaled.
    CLIENT_WAIT_CONDITION_SATISFIED = 2,          /// Indicates that sync was signaled before the timeout expired.
    CLIENT_WAIT_FAILED = 3                        /// Indicates that an error occurred.
};

enum BARRIER_BIT : uint32_t {
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

enum IMAGE_ACCESS_MODE : uint8_t
{
    IMAGE_ACCESS_READ,
    IMAGE_ACCESS_WRITE,
    IMAGE_ACCESS_RW
};

enum CONDITIONAL_RENDER_MODE : uint8_t {
    CONDITIONAL_RENDER_QUERY_WAIT,
    CONDITIONAL_RENDER_QUERY_NO_WAIT,
    CONDITIONAL_RENDER_QUERY_BY_REGION_WAIT,
    CONDITIONAL_RENDER_QUERY_BY_REGION_NO_WAIT,
    CONDITIONAL_RENDER_QUERY_WAIT_INVERTED,
    CONDITIONAL_RENDER_QUERY_NO_WAIT_INVERTED,
    CONDITIONAL_RENDER_QUERY_BY_REGION_WAIT_INVERTED,
    CONDITIONAL_RENDER_QUERY_BY_REGION_NO_WAIT_INVERTED
};

typedef struct _FSync * FSync;

struct BufferCopy {
    size_t    SrcOffset;
    size_t    DstOffset;
    size_t    SizeInBytes;
};

struct BufferClear {
    size_t    Offset;
    size_t    SizeInBytes;
};

struct BlitRectangle {
    uint16_t SrcX;
    uint16_t SrcY;
    uint16_t SrcWidth;
    uint16_t SrcHeight;
    uint16_t DstX;
    uint16_t DstY;
    uint16_t DstWidth;
    uint16_t DstHeight;
};

struct Viewport {
    float   X;
    float   Y;
    float   Width;
    float   Height;
    float   MinDepth;
    float   MaxDepth;
};

struct DrawCmd {
    unsigned int VertexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartVertexLocation;
    unsigned int StartInstanceLocation;
};

struct DrawIndexedCmd {
    unsigned int IndexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartIndexLocation;
    int BaseVertexLocation;
    unsigned int StartInstanceLocation;
};

struct DrawIndirectCmd {
    unsigned int VertexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartVertexLocation;
    unsigned int StartInstanceLocation;   // Since GL v4.0, ignored on older versions
};

struct DrawIndexedIndirectCmd {
    unsigned int IndexCountPerInstance;
    unsigned int InstanceCount;
    unsigned int StartIndexLocation;
    unsigned int BaseVertexLocation;
    unsigned int StartInstanceLocation;
};

struct DispatchIndirectCmd {
    unsigned int ThreadGroupCountX;
    unsigned int ThreadGroupCountY;
    unsigned int ThreadGroupCountZ;
};

struct ShaderBufferBinding {
    uint16_t SlotIndex;
    BUFFER_TYPE BufferType;
    Buffer const * pBuffer;
    size_t BindingOffset;
    size_t BindingSize;
};

struct ShaderSamplerBinding {
    uint16_t SlotIndex;
    Sampler pSampler;
};

struct ShaderTextureBinding {
    uint16_t SlotIndex;
    Texture * pTexture;
};

struct ShaderImageBinding {
    uint16_t SlotIndex;
    Texture * pTexture;
    uint16_t Lod;
    bool bLayered;
    uint16_t LayerIndex; /// Array index for texture arrays, depth for 3D textures or cube face for cubemaps
                         /// For cubemap arrays: arrayLength = floor( _LayerIndex / 6 ), face = _LayerIndex % 6
    IMAGE_ACCESS_MODE AccessMode;
    INTERNAL_PIXEL_FORMAT InternalFormat;  // FIXME: get internal format from texture?
};

struct ShaderResources {
    ShaderBufferBinding * Buffers;
    int NumBuffers;

    ShaderSamplerBinding * Samplers;
    int NumSamplers;

    ShaderTextureBinding * Textures;
    int NumTextures;

    ShaderImageBinding * Images;
    int NumImages;
};

class CommandBuffer final : public NonCopyable, IObjectInterface {
public:
    CommandBuffer();
    ~CommandBuffer();

    void Initialize();

    void Deinitialize();

    //
    // Pipeline
    //

    void BindPipeline( Pipeline * _Pipeline );

    //
    // Vertex & Index buffers
    //

    void BindVertexBuffer( unsigned int _InputSlot, /* optional */ Buffer const * _VertexBuffer, unsigned int _Offset = 0 );

    void BindVertexBuffers( unsigned int _StartSlot, unsigned int _NumBuffers, /* optional */ Buffer * const * _VertexBuffers, unsigned int const * _Offsets = nullptr );

    void BindIndexBuffer( Buffer const * _IndexBuffer, INDEX_TYPE _Type, unsigned int _Offset = 0 );

    //
    // Shader resources
    //

    void BindShaderResources( ShaderResources const * _Resources );

    //
    // Viewport
    //

    void SetViewport( Viewport const & _Viewport );

    void SetViewportArray( uint32_t _NumViewports, Viewport const * _Viewports );

    void SetViewportArray( uint32_t _FirstIndex, uint32_t _NumViewports, Viewport const * _Viewports );

    void SetViewportIndexed( uint32_t _Index, Viewport const & _Viewport );

    //
    // Scissor
    //

    void SetScissor( /* optional */ Rect2D const & _Scissor );

    void SetScissorArray( uint32_t _NumScissors, Rect2D const * _Scissors );

    void SetScissorArray( uint32_t _FirstIndex, uint32_t _NumScissors, Rect2D const * _Scissors );

    void SetScissorIndexed( uint32_t _Index, Rect2D const & _Scissor );

    //
    // Render pass
    //

    void BeginRenderPass( RenderPassBegin const & _RenderPassBegin );

    void EndRenderPass();

    //
    // Transform feedback
    //

    void BindTransformFeedback( TransformFeedback & _TransformFeedback );

    void BeginTransformFeedback( PRIMITIVE_TOPOLOGY _OutputPrimitive );

    void ResumeTransformFeedback();

    void PauseTransformFeedback();

    void EndTransformFeedback();

    //
    // Draw
    //

    /// Draw non-indexed primitives.
    void Draw( DrawCmd const * _Cmd );

    /// Draw indexed primitives.
    void Draw( DrawIndexedCmd const * _Cmd );

    /// Draw from transform feedback
    void Draw( TransformFeedback & _TransformFeedback, unsigned int _InstanceCount = 1, unsigned int _StreamIndex = 0 );

    /// Draw non-indexed GPU-generated primitives. From client memory.
    void DrawIndirect( DrawIndirectCmd const * _Cmd );

    /// Draw indexed GPU-generated primitives. From client memory.
    void DrawIndirect( DrawIndexedIndirectCmd const * _Cmd );

    /// Draw GPU-generated primitives. From indirect buffer.
    void DrawIndirect( Buffer * _DrawIndirectBuffer, unsigned int _AlignedByteOffset, bool _Indexed );

    /// Draw non-indexed, non-instanced primitives.
    void MultiDraw( unsigned int _DrawCount, const unsigned int * _VertexCount, const unsigned int * _StartVertexLocations );

    /// Draw indexed, non-instanced primitives.
    void MultiDraw( unsigned int _DrawCount, const unsigned int * _IndexCount, const void * const * _IndexByteOffsets, const int * _BaseVertexLocations = nullptr );

    /// Draw instanced, GPU-generated primitives. From client memory.
    void MultiDrawIndirect( unsigned int _DrawCount, DrawIndirectCmd const * _Cmds, unsigned int _Stride );

    /// Draw indexed, instanced, GPU-generated primitives. From client memory.
    void MultiDrawIndirect( unsigned int _DrawCount, DrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride );

    // TODO?
    // Draw instanced, GPU-generated primitives. From indirect buffer.
    //void MultiDrawIndirect( Buffer * _DrawIndirectBuffer,
    //                        unsigned int _DrawCount,
    //                        const void * _Cmds,
    //                        unsigned int _Stride );

    // TODO?
    // Draw indexed, instanced, GPU-generated primitives. From indirect buffer.
    //void MultiDrawIndirect( Buffer * _DrawIndirectBuffer,
    //                        unsigned int _DrawCount,
    //                        unsigned int * _AlignedByteOffsets,
    //                        unsigned int _Stride );

    //
    // Dispatch compute
    //

    /// Launch one or more compute work groups
    void DispatchCompute( unsigned int _ThreadGroupCountX,
                          unsigned int _ThreadGroupCountY,
                          unsigned int _ThreadGroupCountZ );

    void DispatchCompute( DispatchIndirectCmd const * _Cmd );

    /// Launch one or more compute work groups using parameters stored in a dispatch indirect buffer
    void DispatchComputeIndirect( Buffer * _DispatchIndirectBuffer, unsigned int _AlignedByteOffset );

    //
    // Query
    //

    void BeginQuery( QueryPool * _QueryPool, uint32_t _QueryID, uint32_t _StreamIndex = 0 );

    void EndQuery( QueryPool * _QueryPool, uint32_t _StreamIndex = 0 );

    void CopyQueryPoolResultsAvailable( QueryPool * _QueryPool,
                                        uint32_t _FirstQuery,
                                        uint32_t _QueryCount,
                                        Buffer * _DstBuffer,
                                        size_t _DstOffst,
                                        size_t _DstStride,
                                        bool _QueryResult64Bit );

    void CopyQueryPoolResults( QueryPool * _QueryPool,
                               uint32_t _FirstQuery,
                               uint32_t _QueryCount,
                               Buffer * _DstBuffer,
                               size_t _DstOffst,
                               size_t _DstStride,
                               QUERY_RESULT_FLAGS _Flags );

    //
    // Conditional render
    //

    void BeginConditionalRender( QueryPool * _QueryPool, uint32_t _QueryID, CONDITIONAL_RENDER_MODE _Mode );

    void EndConditionalRender();

    //
    // Synchronization
    //

    FSync FenceSync();

    void RemoveSync( FSync _Sync );

    CLIENT_WAIT_STATUS ClientWait( FSync _Sync, /* optional */ uint64_t _TimeOutNanoseconds = 0xFFFFFFFFFFFFFFFF );

    void ServerWait( FSync _Sync );

    bool IsSignaled( FSync _Sync );

    void Flush();

    void Barrier( int _BarrierBits );

    void BarrierByRegion( int _BarrierBits );

    void TextureBarrier();

    //
    // DYNAMIC STATE
    //

    void DynamicState_BlendingColor( /* optional */ const float _ConstantColor[4] );

    void DynamicState_SampleMask( /* optional */ const uint32_t _SampleMask[4] );

    void DynamicState_StencilRef( uint32_t _StencilRef );

    void SetLineWidth( float _Width );

    //
    // Copy
    //

    void CopyBuffer( Buffer & _SrcBuffer, Buffer & _DstBuffer );

    void CopyBufferRange( Buffer & _SrcBuffer, Buffer & _DstBuffer, uint32_t _NumRanges, BufferCopy const * _Ranges );

    /// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP
    bool CopyBufferToTexture( Buffer & _SrcBuffer,
                              Texture & _DstTexture,
                              TextureRect const & _Rectangle,
                              BUFFER_DATA_TYPE _DataType,             /// source buffer format
                              size_t _CompressedDataByteLength,       /// only for compressed images
                              size_t _SourceByteOffset,               /// offset in source buffer
                              unsigned int _Alignment                 /// Specifies alignment of source data
                              );

    /// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP, TEXTURE_CUBE_MAP_ARRAY or TEXTURE_RECT
    /// Texture cannot be multisample
    void CopyTextureToBuffer( Texture & _SrcTexture,
                              Buffer & _DstBuffer,
                              TextureRect const & _Rectangle,
                              BUFFER_DATA_TYPE _DataType,   /// how texture will be store in destination buffer
                              size_t _SizeInBytes,       /// copying data byte length
                              size_t _DstByteOffset,        /// offset in destination buffer
                              unsigned int _Alignment       /// Specifies alignment of destination data
                              );

    void CopyTextureRect( Texture & _SrcTexture,
                          Texture & _DstTexture,
                          uint32_t _NumCopies,
                          TextureCopy const * Copies );

    /// Only for TEXTURE_1D TEXTURE_1D_ARRAY TEXTURE_2D TEXTURE_2D_ARRAY TEXTURE_3D TEXTURE_CUBE_MAP TEXTURE_RECT
    /// Выборка элемента массива из TEXTURE_1D_ARRAY осуществляется через _OffsetY
    /// Выборка элемента массива из TEXTURE_2D_ARRAY осуществляется через _OffsetZ
    /// Выборка слоя из TEXTURE_3D осуществляется через _OffsetZ
    /// Выборка грани кубической текстуры из TEXTURE_CUBE_MAP осуществляется через _OffsetZ
    bool CopyFramebufferToTexture( Framebuffer & _SrcFramebuffer,
                                   Texture & _DstTexture,
                                   FRAMEBUFFER_ATTACHMENT _Attachment,
                                   TextureOffset const & _Offset,
                                   Rect2D const & _SrcRect,
                                   unsigned int _Alignment );

    void CopyFramebufferToBuffer( Framebuffer & _SrcFramebuffer,
                                  Buffer & _DstBuffer,
                                  FRAMEBUFFER_ATTACHMENT _Attachment,
                                  Rect2D const & _SrcRect,
                                  FRAMEBUFFER_CHANNEL _FramebufferChannel,
                                  FRAMEBUFFER_OUTPUT _FramebufferOutput,
                                  COLOR_CLAMP _ColorClamp,
                                  size_t _SizeInBytes,
                                  size_t _DstByteOffset,
                                  unsigned int _Alignment );            /// Specifies alignment of destination data


    /// Copy source framebuffer to current
    /// NOTE: Next things can affect:
    /// SCISSOR
    /// Pixel Owner Ship (for default framebuffer only)
    /// Conditional Rendering
    bool BlitFramebuffer( Framebuffer & _SrcFramebuffer,
                          FRAMEBUFFER_ATTACHMENT _SrcAttachment,
                          uint32_t _NumRectangles,
                          BlitRectangle const * _Rectangles,
                          FRAMEBUFFER_MASK _Mask,
                          bool _LinearFilter );

    //
    // Clear
    //

    /// Fill all of buffer object's data store with a fixed value.
    /// If _ClearValue is NULL, then the buffer's data store is filled with zeros.
    void ClearBuffer( Buffer & _Buffer, BUFFER_DATA_TYPE _DataType, const void * _ClearValue = nullptr );

    /// Fill all or part of buffer object's data store with a fixed value.
    /// If _ClearValue is NULL, then the subrange of the buffer's data store is filled with zeros.
    void ClearBufferRange( Buffer & _Buffer, BUFFER_DATA_TYPE _DataType, uint32_t _NumRanges, BufferClear const * _Ranges, const void * _ClearValue = nullptr );

    /// Fill texture image with a fixed value.
    /// If _ClearValue is NULL, then the texture image is filled with zeros.
    void ClearTexture( Texture & _Texture, uint16_t _Lod, TEXTURE_PIXEL_FORMAT _PixelFormat, const void * _ClearValue = nullptr );

    /// Fill all or part of texture image with a fixed value.
    /// If _ClearValue is NULL, then the rect of the texture image is filled with zeros.
    void ClearTextureRect( Texture & _Texture,
                           uint32_t _NumRectangles,
                           TextureRect const * _Rectangles,
                           TEXTURE_PIXEL_FORMAT _PixelFormat,
                           const void * _ClearValue = nullptr );

    void ClearFramebufferAttachments( Framebuffer & _Framebuffer,
                                      /* optional */ unsigned int * _ColorAttachments,
                                      /* optional */ unsigned int _NumColorAttachments,
                                      /* optional */ ClearColorValue * _ColorClearValues,
                                      /* optional */ ClearDepthStencilValue * _DepthStencilClearValue,
                                      /* optional */ Rect2D const * _Rect );

private:
    void BindRenderPassSubPass( State * _State, RenderPass * _RenderPass, int _Subpass );
    void BeginRenderPassDefaultFramebuffer( RenderPassBegin const & _RenderPassBegin );

    bool CopyBufferToTexture1D( Buffer & _SrcBuffer,
                                Texture & _DstTexture,
                                uint16_t _Lod,
                                uint16_t _OffsetX,
                                uint16_t _DimensionX,
                                size_t _CompressedDataByteLength, // Only for compressed images
                                BUFFER_DATA_TYPE _DataType,
                                size_t _SourceByteOffset,
                                unsigned int _Alignment );

    bool CopyBufferToTexture2D( Buffer & _SrcBuffer,
                                Texture & _DstTexture,
                                uint16_t _Lod,
                                uint16_t _OffsetX,
                                uint16_t _OffsetY,
                                uint16_t _DimensionX,
                                uint16_t _DimensionY,
                                uint16_t _CubeFaceIndex, // only for TEXTURE_CUBE_MAP
                                uint16_t _NumCubeFaces, // only for TEXTURE_CUBE_MAP
                                size_t _CompressedDataByteLength, // Only for compressed images
                                BUFFER_DATA_TYPE _DataType,
                                size_t _SourceByteOffset,
                                unsigned int _Alignment );

    bool CopyBufferToTexture3D( Buffer & _SrcBuffer,
                                Texture & _DstTexture,
                                uint16_t _Lod,
                                uint16_t _OffsetX,
                                uint16_t _OffsetY,
                                uint16_t _OffsetZ,
                                uint16_t _DimensionX,
                                uint16_t _DimensionY,
                                uint16_t _DimensionZ,
                                size_t _CompressedDataByteLength, // Only for compressed images
                                BUFFER_DATA_TYPE _DataType,
                                size_t _SourceByteOffset,
                                unsigned int _Alignment );
};

//typedef void (VKAPI_PTR *PFN_vkCmdSetDepthBias)(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
//typedef void (VKAPI_PTR *PFN_vkCmdSetDepthBounds)(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds);
//typedef void (VKAPI_PTR *PFN_vkCmdSetStencilCompareMask)(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask);
//typedef void (VKAPI_PTR *PFN_vkCmdSetStencilWriteMask)(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask);
//typedef void (VKAPI_PTR *PFN_vkCmdBindDescriptorSets)(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
//typedef void (VKAPI_PTR *PFN_vkCmdCopyImage)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
//typedef void (VKAPI_PTR *PFN_vkCmdBlitImage)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
//typedef void (VKAPI_PTR *PFN_vkCmdCopyBufferToImage)(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
//typedef void (VKAPI_PTR *PFN_vkCmdCopyImageToBuffer)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
//typedef void (VKAPI_PTR *PFN_vkCmdUpdateBuffer)(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
//typedef void (VKAPI_PTR *PFN_vkCmdFillBuffer)(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
//typedef void (VKAPI_PTR *PFN_vkCmdClearColorImage)(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
//typedef void (VKAPI_PTR *PFN_vkCmdClearDepthStencilImage)(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
//typedef void (VKAPI_PTR *PFN_vkCmdClearAttachments)(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
//typedef void (VKAPI_PTR *PFN_vkCmdResolveImage)(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
//typedef void (VKAPI_PTR *PFN_vkCmdSetEvent)(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);
//typedef void (VKAPI_PTR *PFN_vkCmdResetEvent)(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);
//typedef void (VKAPI_PTR *PFN_vkCmdWaitEvents)(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
//typedef void (VKAPI_PTR *PFN_vkCmdPipelineBarrier)(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
//typedef void (VKAPI_PTR *PFN_vkCmdBeginQuery)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
//typedef void (VKAPI_PTR *PFN_vkCmdEndQuery)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query);
//typedef void (VKAPI_PTR *PFN_vkCmdResetQueryPool)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
//typedef void (VKAPI_PTR *PFN_vkCmdWriteTimestamp)(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
//typedef void (VKAPI_PTR *PFN_vkCmdCopyQueryPoolResults)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
//typedef void (VKAPI_PTR *PFN_vkCmdPushConstants)(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
//typedef void (VKAPI_PTR *PFN_vkCmdNextSubpass)(VkCommandBuffer commandBuffer, VkSubpassContents contents);
//typedef void (VKAPI_PTR *PFN_vkCmdExecuteCommands)(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

}
