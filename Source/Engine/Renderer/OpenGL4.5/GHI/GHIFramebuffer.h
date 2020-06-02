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
class Texture;

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
    FB_RED_CHANNEL,
    FB_GREEN_CHANNEL,
    FB_BLUE_CHANNEL,
    FB_RGB_CHANNEL,
    FB_BGR_CHANNEL,
    FB_RGBA_CHANNEL,
    FB_BGRA_CHANNEL,
    FB_STENCIL_CHANNEL,
    FB_DEPTH_CHANNEL,
    FB_DEPTH_STENCIL_CHANNEL,
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

struct FramebufferAttachmentInfo {
    Texture * pTexture;
    FRAMEBUFFER_ATTACHMENT_TYPE Type;
    uint16_t LayerNum;
    uint16_t LodNum;
};

struct FramebufferCreateInfo {
    uint16_t                Width;
    uint16_t                Height;
    uint16_t                NumColorAttachments;
    FramebufferAttachmentInfo * pColorAttachments;
    FramebufferAttachmentInfo * pDepthStencilAttachment;

    FramebufferCreateInfo() = default;
    //{
    //}

    FramebufferCreateInfo( uint16_t InWidth,
                           uint16_t InHeight,
                           uint16_t InNumColorAttachments,
                           FramebufferAttachmentInfo * InpColorAttachments,
                           FramebufferAttachmentInfo * InpDepthStencilAttachment )
        : Width( InWidth )
        , Height( InHeight )
        , NumColorAttachments( InNumColorAttachments )
        , pColorAttachments( InpColorAttachments )
        , pDepthStencilAttachment( InpDepthStencilAttachment )
    {
    }
};

class Framebuffer final : public NonCopyable, IObjectInterface {

    friend class CommandBuffer;
    friend class State;

public:
    Framebuffer();
    ~Framebuffer();

    void Initialize( FramebufferCreateInfo const & _CreateInfo );
    void Deinitialize();

    uint16_t GetWidth() const { return Width; }

    uint16_t GetHeight() const { return Height; }

    uint16_t GetNumColorAttachments() const { return NumColorAttachments; }

    FramebufferAttachmentInfo const * GetColorAttachments() const { return ColorAttachments; }

    bool HasDepthStencilAttachment() const { return bHasDepthStencilAttachment; }

    FramebufferAttachmentInfo const & GetDepthStencilAttachment() const { return DepthStencilAttachment; }

    /// Client-side call function
    bool Read( FRAMEBUFFER_ATTACHMENT _Attachment,
               Rect2D const & _SrcRect,
               FRAMEBUFFER_CHANNEL _FramebufferChannel,
               FRAMEBUFFER_OUTPUT _FramebufferOutput,
               COLOR_CLAMP _ColorClamp,
               size_t _SizeInBytes,
               unsigned int _Alignment,               // Specifies alignment of destination data
               void * _SysMem );

    bool Invalidate( uint16_t _NumFramebufferAttachments, FRAMEBUFFER_ATTACHMENT const * _Attachments );

    bool InvalidateRect( uint16_t _NumFramebufferAttachments,
                         FRAMEBUFFER_ATTACHMENT const * _Attachments,
                         Rect2D const & _Rect );

    void * GetHandle() const { return Handle; }

private:    
    bool ChooseReadBuffer( FRAMEBUFFER_ATTACHMENT _Attachment );
    void BindReadFramebuffer();

    Device * pDevice;
    void * Handle;

    uint16_t Width;
    uint16_t Height;

    uint16_t NumColorAttachments;
    FramebufferAttachmentInfo ColorAttachments[ MAX_COLOR_ATTACHMENTS ];

    bool bHasDepthStencilAttachment;
    FramebufferAttachmentInfo DepthStencilAttachment;

    bool bDefault;
};

}
