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

#include "DeviceObject.h"
#include "StaticLimits.h"

#include <Core/Public/Ref.h>

namespace RenderCore {

class ITexture;

enum FRAMEBUFFER_ATTACHMENT : uint16_t
{
    FB_COLOR_ATTACHMENT,                    // Use FB_COLOR_ATTACHMENT + i or i. Range i [0;MAX_COLOR_ATTACHMENTS-1]
    FB_DEPTH_ATTACHMENT          = 1024,
    FB_STENCIL_ATTACHMENT        = 1025,
    FB_DEPTH_STENCIL_ATTACHMENT  = 1026,

    // Only for default framebuffer:
    FB_FRONT_DEFAULT,
    FB_BACK_DEFAULT,
    FB_FRONT_LEFT_DEFAULT,
    FB_FRONT_RIGHT_DEFAULT,
    FB_BACK_LEFT_DEFAULT,
    FB_BACK_RIGHT_DEFAULT,
    FB_COLOR_DEFAULT,
    FB_DEPTH_DEFAULT,
    FB_STENCIL_DEFAULT
};

enum FRAMEBUFFER_MASK : uint8_t
{
    FB_MASK_COLOR         = (1<<0),
    FB_MASK_DEPTH         = (1<<1),
    FB_MASK_STENCIL       = (1<<2),
    FB_MASK_DEPTH_STENCIL = (FB_MASK_DEPTH | FB_MASK_STENCIL),
    FB_MASK_ALL           = 0xff
};

enum FRAMEBUFFER_CHANNEL : uint8_t
{
    FB_CHANNEL_RED,
    FB_CHANNEL_GREEN,
    FB_CHANNEL_BLUE,
    FB_CHANNEL_RGB,
    FB_CHANNEL_BGR,
    FB_CHANNEL_RGBA,
    FB_CHANNEL_BGRA,
    FB_CHANNEL_STENCIL,
    FB_CHANNEL_DEPTH,
    FB_CHANNEL_DEPTH_STENCIL,
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

enum FRAMEBUFFER_ATTACHMENT_TYPE : uint8_t
{
    ATTACH_TEXTURE,
    ATTACH_LAYER
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

struct SFramebufferAttachmentInfo
{
    ITexture * pTexture;
    FRAMEBUFFER_ATTACHMENT_TYPE Type;
    uint16_t LayerNum;
    uint16_t LodNum;
};

struct SFramebufferCreateInfo
{
    uint16_t                Width;
    uint16_t                Height;
    uint16_t                NumColorAttachments;
    SFramebufferAttachmentInfo * pColorAttachments;
    SFramebufferAttachmentInfo * pDepthStencilAttachment;

    SFramebufferCreateInfo() = default;
    //{
    //}

    SFramebufferCreateInfo( uint16_t InWidth,
                            uint16_t InHeight,
                            uint16_t InNumColorAttachments,
                            SFramebufferAttachmentInfo * InpColorAttachments,
                            SFramebufferAttachmentInfo * InpDepthStencilAttachment )
        : Width( InWidth )
        , Height( InHeight )
        , NumColorAttachments( InNumColorAttachments )
        , pColorAttachments( InpColorAttachments )
        , pDepthStencilAttachment( InpDepthStencilAttachment )
    {
    }
};

struct SRect2D
{
    uint16_t X;
    uint16_t Y;
    uint16_t Width;
    uint16_t Height;
};

class IFramebuffer : public IDeviceObject
{
public:
    IFramebuffer( IDevice * Device ) : IDeviceObject( Device ) {}

    uint16_t GetWidth() const { return Width; }

    uint16_t GetHeight() const { return Height; }

    uint16_t GetNumColorAttachments() const { return NumColorAttachments; }

    SFramebufferAttachmentInfo const * GetColorAttachments() const { return ColorAttachments; }

    bool HasDepthStencilAttachment() const { return bHasDepthStencilAttachment; }

    SFramebufferAttachmentInfo const & GetDepthStencilAttachment() const { return DepthStencilAttachment; }

    bool IsAttachmentsOutdated() const;

    /// Client-side call function
    virtual bool Read( FRAMEBUFFER_ATTACHMENT _Attachment,
                       SRect2D const & _SrcRect,
                       FRAMEBUFFER_CHANNEL _FramebufferChannel,
                       FRAMEBUFFER_OUTPUT _FramebufferOutput,
                       COLOR_CLAMP _ColorClamp,
                       size_t _SizeInBytes,
                       unsigned int _Alignment,               // Specifies alignment of destination data
                       void * _SysMem ) = 0;

    virtual bool Invalidate( uint16_t _NumFramebufferAttachments, FRAMEBUFFER_ATTACHMENT const * _Attachments ) = 0;

    virtual bool InvalidateRect( uint16_t _NumFramebufferAttachments,
                                 FRAMEBUFFER_ATTACHMENT const * _Attachments,
                                 SRect2D const & _Rect ) = 0;

protected:
    uint16_t Width;
    uint16_t Height;

    uint16_t NumColorAttachments;
    SFramebufferAttachmentInfo ColorAttachments[MAX_COLOR_ATTACHMENTS];
    TWeakRef< ITexture > Textures[MAX_COLOR_ATTACHMENTS];

    bool bHasDepthStencilAttachment;
    SFramebufferAttachmentInfo DepthStencilAttachment;
    TWeakRef< ITexture > DepthAttachment;
};

}
