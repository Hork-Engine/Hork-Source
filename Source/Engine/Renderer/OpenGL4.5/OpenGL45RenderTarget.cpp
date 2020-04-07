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

#include "OpenGL45RenderTarget.h"

using namespace GHI;

namespace OpenGL45 {

ARenderTarget GRenderTarget;

void ARenderTarget::Initialize() {
    FramebufferWidth = 0;
    FramebufferHeight = 0;
}

void ARenderTarget::Deinitialize() {
    Framebuffer.Deinitialize();
    FramebufferTexture.Deinitialize();
    FramebufferDepth.Deinitialize();

    PostprocessFramebuffer.Deinitialize();
    PostprocessTexture.Deinitialize();

    FxaaFramebuffer.Deinitialize();
    FxaaTexture.Deinitialize();
}

void ARenderTarget::CreateFramebuffer() {
    Framebuffer.Deinitialize();
    FramebufferTexture.Deinitialize();
    FramebufferDepth.Deinitialize();

    PostprocessFramebuffer.Deinitialize();
    PostprocessTexture.Deinitialize();

    FxaaFramebuffer.Deinitialize();
    FxaaTexture.Deinitialize();

    TextureStorageCreateInfo texStorageCI = {};
    texStorageCI.Type = GHI::TEXTURE_2D;
    texStorageCI.Resolution.Tex2D.Width = FramebufferWidth;
    texStorageCI.Resolution.Tex2D.Height = FramebufferHeight;
    texStorageCI.NumLods = 1;

    texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGBA16F;
    FramebufferTexture.InitializeStorage( texStorageCI );

    texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGBA16F;
    //texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGBA8;
    PostprocessTexture.InitializeStorage( texStorageCI );

    texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGBA16F;
    //texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGBA8;
    FxaaTexture.InitializeStorage( texStorageCI );

    texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_DEPTH24_STENCIL8;
    //texStorageCI.InternalFormat = INTERNAL_PIXEL_FORMAT_DEPTH32F_STENCIL8;
    FramebufferDepth.InitializeStorage( texStorageCI );

    {
        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = FramebufferWidth;
        framebufferCI.Height = FramebufferHeight;
        framebufferCI.NumColorAttachments = 1;

        FramebufferAttachmentInfo colorAttachment = {};
        colorAttachment.pTexture = &FramebufferTexture;
        colorAttachment.bLayered = false;
        colorAttachment.LayerNum = 0;
        colorAttachment.LodNum = 0;
        framebufferCI.pColorAttachments = &colorAttachment;

        FramebufferAttachmentInfo depthAttachment = {};
        depthAttachment.pTexture = &FramebufferDepth;
        depthAttachment.bLayered = false;
        depthAttachment.LayerNum = 0;
        depthAttachment.LodNum = 0;
        framebufferCI.pDepthStencilAttachment = &depthAttachment;

        Framebuffer.Initialize( framebufferCI );
    }

    {
        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = FramebufferWidth;
        framebufferCI.Height = FramebufferHeight;
        framebufferCI.NumColorAttachments = 1;

        FramebufferAttachmentInfo colorAttachment = {};
        colorAttachment.pTexture = &PostprocessTexture;
        colorAttachment.bLayered = false;
        colorAttachment.LayerNum = 0;
        colorAttachment.LodNum = 0;
        framebufferCI.pColorAttachments = &colorAttachment;

        PostprocessFramebuffer.Initialize( framebufferCI );
    }

    {
        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.Width = FramebufferWidth;
        framebufferCI.Height = FramebufferHeight;
        framebufferCI.NumColorAttachments = 1;

        FramebufferAttachmentInfo colorAttachment = {};
        colorAttachment.pTexture = &FxaaTexture;
        colorAttachment.bLayered = false;
        colorAttachment.LayerNum = 0;
        colorAttachment.LodNum = 0;
        framebufferCI.pColorAttachments = &colorAttachment;

        FxaaFramebuffer.Initialize( framebufferCI );
    }
}

void ARenderTarget::ReallocSurface( int _AllocSurfaceWidth, int _AllocSurfaceHeight ) {
    if ( FramebufferWidth != _AllocSurfaceWidth
        || FramebufferHeight != _AllocSurfaceHeight ) {

        FramebufferWidth = _AllocSurfaceWidth;
        FramebufferHeight = _AllocSurfaceHeight;

        CreateFramebuffer();
    }
}

}
