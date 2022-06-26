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

#include "FGRenderTask.h"
#include "StaticLimits.h"
#include "Texture.h"
#include "Buffer.h"

namespace RenderCore
{

struct SRect2D
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

struct SAttachmentRef
{
    uint32_t Attachment = 0;

    SAttachmentRef() = default;
    SAttachmentRef(uint32_t _Attachment) :
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

union SClearColorValue
{
    float    Float32[4] = {0,0,0,0};
    int32_t  Int32[4];
    uint32_t UInt32[4];
};

inline SClearColorValue MakeClearColorValue(float R, float G, float B, float A)
{
    SClearColorValue v;
    v.Float32[0] = R;
    v.Float32[1] = G;
    v.Float32[2] = B;
    v.Float32[3] = A;
    return v;
}

inline SClearColorValue MakeClearColorValue(int32_t R, int32_t G, int32_t B, int32_t A)
{
    SClearColorValue v;
    v.Int32[0] = R;
    v.Int32[1] = G;
    v.Int32[2] = B;
    v.Int32[3] = A;
    return v;
}

inline SClearColorValue MakeClearColorValue(uint32_t R, uint32_t G, uint32_t B, uint32_t A)
{
    SClearColorValue v;
    v.UInt32[0] = R;
    v.UInt32[1] = G;
    v.UInt32[2] = B;
    v.UInt32[3] = A;
    return v;
}

struct SClearDepthStencilValue
{
    float    Depth   = 0;
    uint32_t Stencil = 0;

    SClearDepthStencilValue() = default;

    SClearDepthStencilValue(float Depth, uint32_t Stencil) :
        Depth(Depth), Stencil(Stencil)
    {}
};

struct STextureAttachmentClearValue
{
    SClearColorValue        Color;
    SClearDepthStencilValue DepthStencil;
};

struct STextureAttachment
{
    const char*                  Name               = "Unnamed texture attachment";
    class FGTextureProxy*        pResource          = {};
    STextureDesc                 TextureDesc        = {};
    ATTACHMENT_LOAD_OP           LoadOp             = ATTACHMENT_LOAD_OP_LOAD;
    ATTACHMENT_STORE_OP          StoreOp            = ATTACHMENT_STORE_OP_STORE;
    STextureAttachmentClearValue ClearValue;
    bool                         bCreateNewResource = false;
    uint16_t                     MipLevel           = 0;
    uint16_t                     SliceNum           = 0;
    bool                         bSingleSlice       = false;

    explicit STextureAttachment(FGTextureProxy* pResource) :
        pResource(pResource), bCreateNewResource(false)
    {
    }

    explicit STextureAttachment(const char* Name, STextureDesc const& TextureDesc) :
        Name(Name), TextureDesc(TextureDesc), bCreateNewResource(true)
    {
        if (IsDepthStencilFormat(TextureDesc.Format))
        {
            this->TextureDesc.BindFlags |= BIND_DEPTH_STENCIL;
        }
        else
        {
            this->TextureDesc.BindFlags |= BIND_RENDER_TARGET;
        }

        // FIXME: Set BIND_SHADER_RESOURCE as default for STextureAttachment or not?
        this->TextureDesc.BindFlags |= BIND_SHADER_RESOURCE;
        //this->TextureDesc.BindFlags |= BIND_UNORDERED_ACCESS;
    }

    STextureAttachment& SetLoadOp(ATTACHMENT_LOAD_OP InLoadOp)
    {
        LoadOp = InLoadOp;
        return *this;
    }

    STextureAttachment& SetStoreOp(ATTACHMENT_STORE_OP InStoreOp)
    {
        StoreOp = InStoreOp;
        return *this;
    }

    STextureAttachment& SetClearValue(SClearColorValue const& InClearValue)
    {
        ClearValue.Color = InClearValue;
        return *this;
    }

    STextureAttachment& SetClearValue(SClearDepthStencilValue const& InClearValue)
    {
        ClearValue.DepthStencil = InClearValue;
        return *this;
    }

    STextureAttachment& SetMipLevel(uint16_t InMipLevel)
    {
        MipLevel = InMipLevel;
        return *this;
    }

    STextureAttachment& SetSlice(uint16_t InSlice)
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

class ACommandBuffer
{
public:
    // TODO...
};

class ARenderPass;

class ARenderPassContext
{
public:
    ARenderPass* pRenderPass;
    int          SubpassIndex;
    SRect2D      RenderArea;
    class IImmediateContext* pImmediateContext;

    int GetSubpassIndex() const { return SubpassIndex; }
};

using ARecordFunction    = std::function<void(ARenderPassContext&, ACommandBuffer&)>;

class ASubpassInfo
{
public:
    ASubpassInfo(std::initializer_list<SAttachmentRef> const& ColorAttachmentRefs, ARecordFunction RecordFunction) :
        Refs(ColorAttachmentRefs), Function(RecordFunction)
    {
    }

    TVector<SAttachmentRef> Refs;
    ARecordFunction            Function;
};

class ARenderPass : public FGRenderTask<ARenderPass>
{
    using Super = FGRenderTask<ARenderPass>;

public:
    ARenderPass(AFrameGraph* pFrameGraph, const char* Name) :
        FGRenderTask(pFrameGraph, Name, FG_RENDER_TASK_PROXY_TYPE_RENDER_PASS)
    {}

    ARenderPass(ARenderPass const&) = delete;
    ARenderPass(ARenderPass&&)      = default;
    ARenderPass& operator=(ARenderPass const&) = delete;
    ARenderPass& operator=(ARenderPass&&) = default;

    ARenderPass& SetColorAttachment(STextureAttachment const& InColorAttachment)
    {
        HK_ASSERT_(ColorAttachments.IsEmpty(), "Overwriting color attachments");

        ColorAttachments.Add(InColorAttachment);

        AddAttachmentResources();
        return *this;
    }

    ARenderPass& SetColorAttachments(TVector<STextureAttachment> InColorAttachments)
    {
        HK_ASSERT_(ColorAttachments.IsEmpty(), "Overwriting color attachments");

        ColorAttachments = std::move(InColorAttachments);

        AddAttachmentResources();
        return *this;
    }

    ARenderPass& SetDepthStencilAttachment(STextureAttachment const& InDepthStencilAttachment)
    {
        HK_ASSERT_(!bHasDepthStencilAttachment, "Overwriting depth stencil attachment");

        DepthStencilAttachment     = InDepthStencilAttachment;
        bHasDepthStencilAttachment = true;

        AddAttachmentResource(DepthStencilAttachment);
        return *this;
    }

    ARenderPass& SetRenderArea(int X, int Y, int W, int H)
    {
        RenderArea.X         = X;
        RenderArea.Y         = Y;
        RenderArea.Width     = W;
        RenderArea.Height    = H;
        bRenderAreaSpecified = true;
        return *this;
    }

    ARenderPass& SetRenderArea(int W, int H)
    {
        RenderArea.X         = 0;
        RenderArea.Y         = 0;
        RenderArea.Width     = W;
        RenderArea.Height    = H;
        bRenderAreaSpecified = true;
        return *this;
    }

    ARenderPass& SetRenderArea(SRect2D const& Area)
    {
        RenderArea           = Area;
        bRenderAreaSpecified = true;
        return *this;
    }

    ARenderPass& AddSubpass(std::initializer_list<SAttachmentRef> const& ColorAttachmentRefs, ARecordFunction RecordFunction)
    {
        Subpasses.EmplaceBack(ColorAttachmentRefs, RecordFunction);
        return *this;
    }

    // Getters

    SRect2D const& GetRenderArea() const
    {
        return RenderArea;
    }

    TVector<ASubpassInfo> const& GetSubpasses() const
    {
        return Subpasses;
    }

    TVector<STextureAttachment> const& GetColorAttachments() const
    {
        return ColorAttachments;
    }

    STextureAttachment const& GetDepthStencilAttachment() const
    {
        return DepthStencilAttachment;
    }

    bool HasDepthStencilAttachment() const
    {
        return bHasDepthStencilAttachment;
    }

    bool IsRenderAreaSpecified() const
    {
        return bRenderAreaSpecified;
    }

private:
    void AddAttachmentResources()
    {
        for (STextureAttachment& attachment : ColorAttachments)
        {
            AddAttachmentResource(attachment);
        }
    }
    void AddAttachmentResource(STextureAttachment& attachment)
    {
        if (attachment.bCreateNewResource)
        {
            AddNewResource(attachment.Name, attachment.TextureDesc, &attachment.pResource);
        }
        else
        {
            AddResource(attachment.pResource, FG_RESOURCE_ACCESS_WRITE);
        }
    }

    TVector<STextureAttachment> ColorAttachments;
    STextureAttachment             DepthStencilAttachment{nullptr};
    bool                           bHasDepthStencilAttachment = false;
    bool                           bRenderAreaSpecified       = false;
    SRect2D                        RenderArea;
    TVector<ASubpassInfo>       Subpasses;
};

} // namespace RenderCore
