/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "FramebufferGL.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <SDL.h>

namespace RenderCore
{

AFramebufferGL::AFramebufferGL(SFramebufferDescGL const& Desc, int Hash) :
    Hash(Hash), Width(Desc.Width), Height(Desc.Height), NumColorAttachments(Desc.NumColorAttachments), bHasDepthStencilAttachment(Desc.pDepthStencilAttachment != nullptr)
{
    AN_ASSERT(Desc.Width != 0);
    AN_ASSERT(Desc.Height != 0);
    AN_ASSERT(Desc.NumColorAttachments <= MAX_COLOR_ATTACHMENTS);

    bool bDefault = false;

    // Check if this is the default framebuffer
    // Iterate all color attachments. The default framebuffer can only have one color attachment.
    for (uint16_t i = 0; i < NumColorAttachments; i++)
    {
        ITextureView* rtv = Desc.pColorAttachments[i];

        AN_ASSERT(rtv->GetDesc().ViewType == TEXTURE_VIEW_RENDER_TARGET);

        if (rtv->GetHandleNativeGL() == 0)
        {
            if (i == 0)
            {
                ITexture const* backBuffer = rtv->GetTexture();

                AN_ASSERT(backBuffer->GetHandleNativeGL() == 0);
                AN_ASSERT(Width == backBuffer->GetWidth());
                AN_ASSERT(Height == backBuffer->GetHeight());

                RTVs[0] = rtv;

                bDefault = true;
            }
            else
            {
                AN_ASSERT_(0, "Attempting to combine the swap chain's back buffer with other color attachments");
            }
        }
    }

    // If there is a default depth stencil buffer, it can only be combined with the default back buffer, or not to have color attachments.
    if (Desc.pDepthStencilAttachment)
    {
        ITextureView* dsv = Desc.pDepthStencilAttachment;

        AN_ASSERT(dsv->GetDesc().ViewType == TEXTURE_VIEW_DEPTH_STENCIL);

        if (bDefault && dsv->GetHandleNativeGL() != 0)
        {
            AN_ASSERT_(0, "Expected default depth-stencil buffer");
        }

        if (dsv->GetHandleNativeGL() == 0)
        {
            if (!bDefault && Desc.NumColorAttachments > 0)
            {
                AN_ASSERT_(0, "The swap chain's depth-stencil buffer can only be combined with default back buffer.");
            }
            else
            {                
                ITexture const* depthBuffer = dsv->GetTexture();

                AN_ASSERT(depthBuffer->GetHandleNativeGL() == 0);
                AN_ASSERT(Width == depthBuffer->GetWidth());
                AN_ASSERT(Height == depthBuffer->GetHeight());

                pDSV = dsv;

                bDefault = true;
            }
        }
    }

    if (bDefault)
    {
        return;
    }

    glCreateFramebuffers(1, &FramebufferId);

    // TODO: Check GL_MAX_FRAMEBUFFER_WIDTH, GL_MAX_FRAMEBUFFER_HEIGHT, GL_MAX_FRAMEBUFFER_LAYERS

    // From OpenGL specification: GL_FRAMEBUFFER_DEFAULT_WIDTH/GL_FRAMEBUFFER_DEFAULT_HEIGHT/etc specifies the assumed with for a framebuffer object with no attachments.
    // If a framebuffer has attachments then the params of those attachments is used.
    // Therefore, we do not need to set this:
    //glNamedFramebufferParameteri(FramebufferId, GL_FRAMEBUFFER_DEFAULT_WIDTH, Width);
    //glNamedFramebufferParameteri(FramebufferId, GL_FRAMEBUFFER_DEFAULT_HEIGHT, Height);
    //glNamedFramebufferParameteri(FramebufferId, GL_FRAMEBUFFER_DEFAULT_LAYERS, NumLayers);
    //glNamedFramebufferParameteri(FramebufferId, GL_FRAMEBUFFER_DEFAULT_SAMPLES, SamplesCount);
    //glNamedFramebufferParameteri(FramebufferId, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, FixedSampleLocations);

    glNamedFramebufferDrawBuffer(FramebufferId, GL_NONE);

    for (uint16_t i = 0; i < NumColorAttachments; i++)
    {
        ITextureView*           textureView    = Desc.pColorAttachments[i];
        ITexture*               texture        = textureView->GetTexture();
        GLuint                  textureId      = textureView->GetHandleNativeGL();
        STextureViewDesc const& viewDesc       = textureView->GetDesc();
        GLenum                  attachmentName = GL_COLOR_ATTACHMENT0 + i;

        AN_ASSERT(Width == textureView->GetWidth());
        AN_ASSERT(Height == textureView->GetHeight());

        if (viewDesc.NumSlices == texture->GetSliceCount(viewDesc.FirstMipLevel))
        {
            glNamedFramebufferTexture(FramebufferId, attachmentName, textureId, viewDesc.FirstMipLevel);
        }
        else if (viewDesc.NumSlices == 1)
        {
            glNamedFramebufferTextureLayer(FramebufferId, attachmentName, textureId, viewDesc.FirstMipLevel, viewDesc.FirstSlice);
        }
        else
        {
            AN_ASSERT_(0, "Only one layer or an entire texture can be attached to a framebuffer");
        }

        RTVs[i] = textureView;
    }

    if (bHasDepthStencilAttachment)
    {
        ITextureView*           textureView = Desc.pDepthStencilAttachment;
        ITexture*               texture     = textureView->GetTexture();
        GLuint                  textureId   = textureView->GetHandleNativeGL();
        STextureViewDesc const& viewDesc    = textureView->GetDesc();
        GLenum                  attachmentName;

        AN_ASSERT(Width == textureView->GetWidth());
        AN_ASSERT(Height == textureView->GetHeight());

        // TODO: table
        switch (textureView->GetDesc().Format)
        {
            case TEXTURE_FORMAT_STENCIL1:
            case TEXTURE_FORMAT_STENCIL4:
            case TEXTURE_FORMAT_STENCIL8:
            case TEXTURE_FORMAT_STENCIL16:
                attachmentName = GL_STENCIL_ATTACHMENT;
                break;
            case TEXTURE_FORMAT_DEPTH16:
            case TEXTURE_FORMAT_DEPTH24:
            case TEXTURE_FORMAT_DEPTH32:
                attachmentName = GL_DEPTH_ATTACHMENT;
                break;
            case TEXTURE_FORMAT_DEPTH24_STENCIL8:
            case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
                attachmentName = GL_DEPTH_STENCIL_ATTACHMENT;
                break;
            default:
                AN_ASSERT(0);
                attachmentName = GL_DEPTH_STENCIL_ATTACHMENT;
        }

        if (viewDesc.NumSlices == texture->GetSliceCount(viewDesc.FirstMipLevel))
        {
            glNamedFramebufferTexture(FramebufferId, attachmentName, textureId, viewDesc.FirstMipLevel);
        }
        else if (viewDesc.NumSlices == 1)
        {
            glNamedFramebufferTextureLayer(FramebufferId, attachmentName, textureId, viewDesc.FirstMipLevel, viewDesc.FirstSlice);
        }
        else
        {
            AN_ASSERT_(0, "Only one layer or an entire texture can be attached to a framebuffer");
        }

        pDSV = textureView;
    }

    AN_ASSERT(glCheckNamedFramebufferStatus(FramebufferId, GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // Reminder: These framebuffer parameters can be obtained using the following functions:
    // glGetBufferParameteri64v        glGetNamedBufferParameteri64v
    // glGetBufferParameteriv          glGetNamedBufferParameteriv
}

AFramebufferGL::~AFramebufferGL()
{
    if (FramebufferId)
    {
        glDeleteFramebuffers(1, &FramebufferId);
    }
}

bool AFramebufferGL::IsAttachmentsOutdated() const
{
    for (int i = 0; i < NumColorAttachments; i++)
    {
        if (RTVs[i].IsExpired())
        {
            return true;
        }
    }

    return bHasDepthStencilAttachment && pDSV.IsExpired();
}

} // namespace RenderCore
