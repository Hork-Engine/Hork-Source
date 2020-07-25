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

#include <RenderCore/Framebuffer.h>

namespace RenderCore {

class ADeviceGLImpl;

class AFramebufferGLImpl final : public IFramebuffer
{
    friend class ADeviceGLImpl;
    friend class AImmediateContextGLImpl;

public:
    AFramebufferGLImpl( ADeviceGLImpl * _Device, SFramebufferCreateInfo const & _CreateInfo, bool _Default );
    ~AFramebufferGLImpl();

    bool Read( FRAMEBUFFER_ATTACHMENT _Attachment,
               SRect2D const & _SrcRect,
               FRAMEBUFFER_CHANNEL _FramebufferChannel,
               FRAMEBUFFER_OUTPUT _FramebufferOutput,
               COLOR_CLAMP _ColorClamp,
               size_t _SizeInBytes,
               unsigned int _Alignment,               // Specifies alignment of destination data
               void * _SysMem ) override;

    bool Invalidate( uint16_t _NumFramebufferAttachments, FRAMEBUFFER_ATTACHMENT const * _Attachments ) override;

    bool InvalidateRect( uint16_t _NumFramebufferAttachments,
                         FRAMEBUFFER_ATTACHMENT const * _Attachments,
                         SRect2D const & _Rect ) override;

private:    
    bool ChooseReadBuffer( FRAMEBUFFER_ATTACHMENT _Attachment ) const;
    void BindReadFramebuffer() const;

    ADeviceGLImpl * pDevice;
    bool bDefault;
};

}
