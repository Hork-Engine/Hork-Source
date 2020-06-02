/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#include "GHIBasic.h"

namespace GHI {

class Device;
class Framebuffer;
class RenderPass;

//enum IMAGE_LAYOUT {
////    IMAGE_LAYOUT_UNDEFINED = 0,
////    IMAGE_LAYOUT_GENERAL = 1,
//    IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
//    IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
////    IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
////    IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
////    IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
////    IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
////    IMAGE_LAYOUT_PREINITIALIZED = 8,
////    IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
////    IMAGE_LAYOUT_BEGIN_RANGE = IMAGE_LAYOUT_UNDEFINED,
////    IMAGE_LAYOUT_END_RANGE = IMAGE_LAYOUT_PREINITIALIZED,
////    IMAGE_LAYOUT_RANGE_SIZE = (IMAGE_LAYOUT_PREINITIALIZED - IMAGE_LAYOUT_UNDEFINED + 1),
////    IMAGE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
//}

struct AttachmentRef {
    uint32_t            Attachment;
    //IMAGE_LAYOUT   Layout; // TODO

    AttachmentRef() {}
    AttachmentRef( uint32_t _Attachment ) : Attachment( _Attachment ) {}
};

struct SubpassInfo {
//    VkSubpassDescriptionFlags       flags;
//    VkPipelineBindPoint             pipelineBindPoint;
//    uint32_t                        inputAttachmentCount;
//    AttachmentRef const *     pInputAttachments;
    uint32_t                  NumColorAttachments;
    AttachmentRef const *     pColorAttachmentRefs;
//    AttachmentRef const *     pResolveAttachments;
//    AttachmentRef const *     pDepthStencilAttachment;
//    uint32_t                        preserveAttachmentCount;
//    const uint32_t*                 pPreserveAttachments;
};

enum ATTACHMENT_LOAD_OP : uint8_t
{
    ATTACHMENT_LOAD_OP_LOAD = 0,
    ATTACHMENT_LOAD_OP_CLEAR = 1,
    ATTACHMENT_LOAD_OP_DONT_CARE = 2,
//    VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE = VK_ATTACHMENT_LOAD_OP_LOAD,
//    VK_ATTACHMENT_LOAD_OP_END_RANGE = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//    VK_ATTACHMENT_LOAD_OP_RANGE_SIZE = (VK_ATTACHMENT_LOAD_OP_DONT_CARE - VK_ATTACHMENT_LOAD_OP_LOAD + 1),
//    VK_ATTACHMENT_LOAD_OP_MAX_ENUM = 0x7FFFFFFF
};

struct AttachmentInfo {
//    VkAttachmentDescriptionFlags    flags;
//    VkFormat                        format;
//    VkSampleCountFlagBits           samples;
    ATTACHMENT_LOAD_OP          LoadOp;
//    VkAttachmentStoreOp             storeOp;
//    VkAttachmentLoadOp              stencilLoadOp;
//    VkAttachmentStoreOp             stencilStoreOp;
//    VkImageLayout                   initialLayout;
//    VkImageLayout                   finalLayout;

    AttachmentInfo()
        : LoadOp( ATTACHMENT_LOAD_OP_LOAD )
    {
    }

    AttachmentInfo & SetLoadOp( ATTACHMENT_LOAD_OP Op ) {
        LoadOp = Op;
        return *this;
    }
};

struct RenderPassCreateInfo {
    int               NumColorAttachments;
    AttachmentInfo *  pColorAttachments;

    AttachmentInfo *  pDepthStencilAttachment;

    int               NumSubpasses;
    SubpassInfo *     pSubpasses;

    //uint32_t        NumDependencies;
    //VkSubpassDependency const * pDependencies;
};

struct RenderSubpass {
    uint32_t          NumColorAttachments;
    AttachmentRef     ColorAttachmentRefs[ MAX_COLOR_ATTACHMENTS ];
};

typedef union ClearColorValue {
    float       Float32[4];
    int32_t     Int32[4];
    uint32_t    UInt32[4];
} ClearColorValue;

inline ClearColorValue MakeClearColorValue( float R, float G, float B, float A ) {
    ClearColorValue v;
    v.Float32[0] = R;
    v.Float32[1] = G;
    v.Float32[2] = B;
    v.Float32[3] = A;
    return v;
}

inline ClearColorValue MakeClearColorValue( int32_t R, int32_t G, int32_t B, int32_t A ) {
    ClearColorValue v;
    v.Int32[0] = R;
    v.Int32[1] = G;
    v.Int32[2] = B;
    v.Int32[3] = A;
    return v;
}

inline ClearColorValue MakeClearColorValue( uint32_t R, uint32_t G, uint32_t B, uint32_t A ) {
    ClearColorValue v;
    v.UInt32[0] = R;
    v.UInt32[1] = G;
    v.UInt32[2] = B;
    v.UInt32[3] = A;
    return v;
}

typedef struct ClearDepthStencilValue {
    float       Depth;
    uint32_t    Stencil;
} ClearDepthStencilValue;

inline ClearDepthStencilValue MakeClearDepthStencilValue( float Depth, uint32_t Stencil ) {
    ClearDepthStencilValue v;
    v.Depth = Depth;
    v.Stencil = Stencil;
    return v;
}

struct RenderPassBegin {
    RenderPass const * pRenderPass;
    Framebuffer const * pFramebuffer;
    Rect2D            RenderArea;
    ClearColorValue const * pColorClearValues;
    ClearDepthStencilValue const * pDepthStencilClearValue;
};

class RenderPass final : public NonCopyable, IObjectInterface {

    friend class CommandBuffer;

public:
    RenderPass();
    ~RenderPass();

    void Initialize( RenderPassCreateInfo const & _CreateInfo );
    void Deinitialize();

    void * GetHandle() const { return Handle; }

private:
    Device *       pDevice;
    void *         Handle;

    unsigned int   NumColorAttachments;
    AttachmentInfo ColorAttachments[ MAX_COLOR_ATTACHMENTS ];

    bool           bHasDepthStencilAttachment;
    AttachmentInfo DepthStencilAttachment;

    int            NumSubpasses;
    RenderSubpass  Subpasses[ MAX_SUBPASS_COUNT ];
};

}
