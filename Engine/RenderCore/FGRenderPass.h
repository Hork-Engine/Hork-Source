/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "FGRenderTask.h"
#include "StaticLimits.h"
#include "Texture.h"
#include "Buffer.h"

HK_NAMESPACE_BEGIN

namespace RenderCore
{

struct Rect2D
{
    uint16_t X      = 0;
    uint16_t Y      = 0;
    uint16_t Width  = 0;
    uint16_t Height = 0;
};

enum COLOR_CLAMP : uint8_t
{
    /** Clamping is always off, no matter what the format​ or type​ parameters of the read pixels call. */
    COLOR_CLAMP_OFF,
    /** Clamping is always on, no matter what the format​ or type​ parameters of the read pixels call. */
    COLOR_CLAMP_ON,
    /** Clamping is only on if the type of the image being read is a normalized signed or unsigned value. */
    COLOR_CLAMP_FIXED_ONLY
};

enum FRAMEBUFFER_CHANNEL : uint8_t
{
    FB_CHANNEL_RED,
    FB_CHANNEL_GREEN,
    FB_CHANNEL_BLUE,
    FB_CHANNEL_RGB,
    FB_CHANNEL_BGR,
    FB_CHANNEL_RGBA,
    FB_CHANNEL_BGRA
};

enum FRAMEBUFFER_OUTPUT : uint8_t
{
    FB_UBYTE,
    FB_BYTE,
    FB_USHORT,
    FB_SHORT,
    FB_UINT,
    FB_INT,
    FB_HALF,
    FB_FLOAT
};

struct AttachmentRef
{
    uint32_t Attachment = 0;

    AttachmentRef() = default;
    AttachmentRef(uint32_t _Attachment) :
        Attachment(_Attachment)
    {}
};

enum ATTACHMENT_LOAD_OP : uint8_t
{
    ATTACHMENT_LOAD_OP_LOAD      = 0,
    ATTACHMENT_LOAD_OP_CLEAR     = 1,
    ATTACHMENT_LOAD_OP_DONT_CARE = 2,
};

enum ATTACHMENT_STORE_OP : uint8_t
{
    ATTACHMENT_STORE_OP_STORE     = 0,
    ATTACHMENT_STORE_OP_DONT_CARE = 1
};

union ClearColorValue
{
    float    Float32[4] = {0,0,0,0};
    int32_t  Int32[4];
    uint32_t UInt32[4];
};

inline ClearColorValue MakeClearColorValue(float R, float G, float B, float A)
{
    ClearColorValue v;
    v.Float32[0] = R;
    v.Float32[1] = G;
    v.Float32[2] = B;
    v.Float32[3] = A;
    return v;
}

inline ClearColorValue MakeClearColorValue(int32_t R, int32_t G, int32_t B, int32_t A)
{
    ClearColorValue v;
    v.Int32[0] = R;
    v.Int32[1] = G;
    v.Int32[2] = B;
    v.Int32[3] = A;
    return v;
}

inline ClearColorValue MakeClearColorValue(uint32_t R, uint32_t G, uint32_t B, uint32_t A)
{
    ClearColorValue v;
    v.UInt32[0] = R;
    v.UInt32[1] = G;
    v.UInt32[2] = B;
    v.UInt32[3] = A;
    return v;
}

struct ClearDepthStencilValue
{
    float    Depth   = 0;
    uint32_t Stencil = 0;

    ClearDepthStencilValue() = default;

    ClearDepthStencilValue(float Depth, uint32_t Stencil) :
        Depth(Depth), Stencil(Stencil)
    {}
};

struct TextureAttachmentClearValue
{
    ClearColorValue        Color;
    ClearDepthStencilValue DepthStencil;
};

struct TextureAttachment
{
    const char*                 Name               = "Unnamed texture attachment";
    class FGTextureProxy*       pResource          = {};
    TextureDesc                 TexDesc            = {};
    ATTACHMENT_LOAD_OP          LoadOp             = ATTACHMENT_LOAD_OP_LOAD;
    ATTACHMENT_STORE_OP         StoreOp            = ATTACHMENT_STORE_OP_STORE;
    TextureAttachmentClearValue ClearValue;
    bool                        bCreateNewResource = false;
    uint16_t                    MipLevel           = 0;
    uint16_t                    SliceNum           = 0;
    bool                        bSingleSlice       = false;

    explicit TextureAttachment(FGTextureProxy* pResource) :
        pResource(pResource), bCreateNewResource(false)
    {
    }

    explicit TextureAttachment(const char* Name, TextureDesc const& TexDesc) :
        Name(Name), TexDesc(TexDesc), bCreateNewResource(true)
    {
        if (IsDepthStencilFormat(TexDesc.Format))
        {
            this->TexDesc.BindFlags |= BIND_DEPTH_STENCIL;
        }
        else
        {
            this->TexDesc.BindFlags |= BIND_RENDER_TARGET;
        }

        // FIXME: Set BIND_SHADER_RESOURCE as default for TextureAttachment or not?
        this->TexDesc.BindFlags |= BIND_SHADER_RESOURCE;
        //this->TexDesc.BindFlags |= BIND_UNORDERED_ACCESS;
    }

    TextureAttachment& SetLoadOp(ATTACHMENT_LOAD_OP InLoadOp)
    {
        LoadOp = InLoadOp;
        return *this;
    }

    TextureAttachment& SetStoreOp(ATTACHMENT_STORE_OP InStoreOp)
    {
        StoreOp = InStoreOp;
        return *this;
    }

    TextureAttachment& SetClearValue(ClearColorValue const& InClearValue)
    {
        ClearValue.Color = InClearValue;
        return *this;
    }

    TextureAttachment& SetClearValue(ClearDepthStencilValue const& InClearValue)
    {
        ClearValue.DepthStencil = InClearValue;
        return *this;
    }

    TextureAttachment& SetMipLevel(uint16_t InMipLevel)
    {
        MipLevel = InMipLevel;
        return *this;
    }

    TextureAttachment& SetSlice(uint16_t InSlice)
    {
        SliceNum     = InSlice;
        bSingleSlice = true;
        return *this;
    }

    ITexture* GetTexture()
    {
        HK_ASSERT(pResource);
        return pResource->Actual();
    }
};

class FGCommandBuffer
{
public:
    // TODO...
};

class RenderPass;

class FGRenderPassContext
{
public:
    RenderPass* pRenderPass;
    int         SubpassIndex;
    Rect2D      RenderArea;
    class IImmediateContext* pImmediateContext;

    int GetSubpassIndex() const { return SubpassIndex; }
};

// fixed_function is used to prevent memory allocations during frame rendering.
using FGRecordFunction = eastl::fixed_function<97, void(FGRenderPassContext&, FGCommandBuffer&)>;

class FGSubpassInfo
{
public:
    template <typename Fn>
    FGSubpassInfo(std::initializer_list<AttachmentRef> ColorAttachmentRefs, Fn RecordFunction) :
        Refs(ColorAttachmentRefs), Function(RecordFunction)
    {
    }

    StaticVector<AttachmentRef, MAX_COLOR_ATTACHMENTS> Refs;
    FGRecordFunction                                      Function;
};

class RenderPass : public FGRenderTask<RenderPass>
{
    using Super = FGRenderTask<RenderPass>;

public:
    using ColorAttachments = StaticVector<TextureAttachment, MAX_COLOR_ATTACHMENTS>;
    using SubpassArray = SmallVector<FGSubpassInfo, 1, Allocators::FrameMemoryAllocator>;

    RenderPass(FrameGraph* pFrameGraph, const char* Name) :
        FGRenderTask(pFrameGraph, Name, FG_RENDER_TASK_PROXY_TYPE_RENDER_PASS)
    {}

    RenderPass(RenderPass const&) = delete;
    RenderPass(RenderPass&&)      = default;
    RenderPass& operator=(RenderPass const&) = delete;
    RenderPass& operator=(RenderPass&&) = default;

    RenderPass& SetColorAttachment(TextureAttachment const& InColorAttachment)
    {
        HK_ASSERT_(m_ColorAttachments.IsEmpty(), "Overwriting color attachments");

        m_ColorAttachments.Add(InColorAttachment);

        AddAttachmentResources();
        return *this;
    }

    RenderPass& SetColorAttachments(std::initializer_list<TextureAttachment> InColorAttachments)
    {
        HK_ASSERT_(m_ColorAttachments.IsEmpty(), "Overwriting color attachments");
        
        for (auto& attachment : InColorAttachments)
            m_ColorAttachments.Add(attachment);

        AddAttachmentResources();
        return *this;
    }

    RenderPass& SetDepthStencilAttachment(TextureAttachment const& InDepthStencilAttachment)
    {
        HK_ASSERT_(!m_bHasDepthStencilAttachment, "Overwriting depth stencil attachment");

        m_DepthStencilAttachment = InDepthStencilAttachment;
        m_bHasDepthStencilAttachment = true;

        AddAttachmentResource(m_DepthStencilAttachment);
        return *this;
    }

    RenderPass& SetRenderArea(int X, int Y, int W, int H)
    {
        m_RenderArea.X = X;
        m_RenderArea.Y = Y;
        m_RenderArea.Width = W;
        m_RenderArea.Height = H;
        m_bRenderAreaSpecified = true;
        return *this;
    }

    RenderPass& SetRenderArea(int W, int H)
    {
        m_RenderArea.X = 0;
        m_RenderArea.Y = 0;
        m_RenderArea.Width = W;
        m_RenderArea.Height = H;
        m_bRenderAreaSpecified = true;
        return *this;
    }

    RenderPass& SetRenderArea(Rect2D const& Area)
    {
        m_RenderArea = Area;
        m_bRenderAreaSpecified = true;
        return *this;
    }

    template <typename Fn>
    RenderPass& AddSubpass(std::initializer_list<AttachmentRef> ColorAttachmentRefs, Fn RecordFunction)
    {
        m_Subpasses.EmplaceBack(ColorAttachmentRefs, RecordFunction);
        return *this;
    }

    // Getters

    Rect2D const& GetRenderArea() const
    {
        return m_RenderArea;
    }

    SubpassArray const& GetSubpasses() const
    {
        return m_Subpasses;
    }

    ColorAttachments const& GetColorAttachments() const
    {
        return m_ColorAttachments;
    }

    TextureAttachment const& GetDepthStencilAttachment() const
    {
        return m_DepthStencilAttachment;
    }

    bool HasDepthStencilAttachment() const
    {
        return m_bHasDepthStencilAttachment;
    }

    bool IsRenderAreaSpecified() const
    {
        return m_bRenderAreaSpecified;
    }

private:
    void AddAttachmentResources()
    {
        for (TextureAttachment& attachment : m_ColorAttachments)
        {
            AddAttachmentResource(attachment);
        }
    }
    void AddAttachmentResource(TextureAttachment& attachment)
    {
        if (attachment.bCreateNewResource)
        {
            AddNewResource(attachment.Name, attachment.TexDesc, &attachment.pResource);
        }
        else
        {
            bool read  = attachment.LoadOp == ATTACHMENT_LOAD_OP_LOAD;
            bool write = attachment.StoreOp == ATTACHMENT_STORE_OP_STORE;

            if (read && write)
                AddResource(attachment.pResource, FG_RESOURCE_ACCESS_READ_WRITE);
            else if (read)
                AddResource(attachment.pResource, FG_RESOURCE_ACCESS_READ);
            else if (write)
                AddResource(attachment.pResource, FG_RESOURCE_ACCESS_WRITE);
            else
                HK_ASSERT(0);
        }
    }

    ColorAttachments  m_ColorAttachments;
    TextureAttachment m_DepthStencilAttachment{nullptr};
    bool              m_bHasDepthStencilAttachment = false;
    bool              m_bRenderAreaSpecified       = false;
    Rect2D            m_RenderArea;
    SubpassArray      m_Subpasses;
};

} // namespace RenderCore

HK_NAMESPACE_END
