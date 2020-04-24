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
    Bloom.Width = 0;
    Bloom.Height = 0;

    TextureStorageCreateInfo texStorageCI = {};
    texStorageCI.Type = GHI::TEXTURE_2D;
    texStorageCI.NumLods = 1;
    texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG16F;

    texStorageCI.Resolution.Tex2D.Width = 64;
    texStorageCI.Resolution.Tex2D.Height = 64;
    Luminance64.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 32;
    texStorageCI.Resolution.Tex2D.Height = 32;
    Luminance32.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 16;
    texStorageCI.Resolution.Tex2D.Height = 16;
    Luminance16.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 8;
    texStorageCI.Resolution.Tex2D.Height = 8;
    Luminance8.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 4;
    texStorageCI.Resolution.Tex2D.Height = 4;
    Luminance4.InitializeStorage( texStorageCI );

    texStorageCI.Resolution.Tex2D.Width = 2;
    texStorageCI.Resolution.Tex2D.Height = 2;
    Luminance2.InitializeStorage( texStorageCI );

    FramebufferCreateInfo framebufferCI = {};    
    framebufferCI.NumColorAttachments = 1;
    FramebufferAttachmentInfo colorAttachment = {};
    colorAttachment.bLayered = false;
    colorAttachment.LayerNum = 0;
    colorAttachment.LodNum = 0;
    framebufferCI.pColorAttachments = &colorAttachment;

    framebufferCI.Width = 64;
    framebufferCI.Height = 64;
    colorAttachment.pTexture = &Luminance64;
    FramebufferLum64.Initialize( framebufferCI );

    framebufferCI.Width = 32;
    framebufferCI.Height = 32;
    colorAttachment.pTexture = &Luminance32;
    FramebufferLum32.Initialize( framebufferCI );

    framebufferCI.Width = 16;
    framebufferCI.Height = 16;
    colorAttachment.pTexture = &Luminance16;
    FramebufferLum16.Initialize( framebufferCI );

    framebufferCI.Width = 8;
    framebufferCI.Height = 8;
    colorAttachment.pTexture = &Luminance8;
    FramebufferLum8.Initialize( framebufferCI );

    framebufferCI.Width = 4;
    framebufferCI.Height = 4;
    colorAttachment.pTexture = &Luminance4;
    FramebufferLum4.Initialize( framebufferCI );

    framebufferCI.Width = 2;
    framebufferCI.Height = 2;
    colorAttachment.pTexture = &Luminance2;
    FramebufferLum2.Initialize( framebufferCI );
}

void ARenderTarget::Deinitialize() {
    Luminance64.Deinitialize();
    Luminance32.Deinitialize();
    Luminance16.Deinitialize();
    Luminance8.Deinitialize();
    Luminance4.Deinitialize();
    Luminance2.Deinitialize();
    FramebufferLum64.Deinitialize();
    FramebufferLum32.Deinitialize();
    FramebufferLum16.Deinitialize();
    FramebufferLum8.Deinitialize();
    FramebufferLum4.Deinitialize();
    FramebufferLum2.Deinitialize();

    Framebuffer.Deinitialize();
    FramebufferTexture.Deinitialize();
    FramebufferDepth.Deinitialize();

    PostprocessFramebuffer.Deinitialize();
    PostprocessTexture.Deinitialize();

    FxaaFramebuffer.Deinitialize();
    FxaaTexture.Deinitialize();

    for ( int i = 0 ; i < 2 ; i++ ) {
        Bloom.Texture[i].Deinitialize();
        Bloom.Textures_2[i].Deinitialize();
        Bloom.Textures_4[i].Deinitialize();
        Bloom.Textures_6[i].Deinitialize();
    }
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

    CreateBloomTextures();
}

void ARenderTarget::CreateBloomTextures() {
    int NewWidth = FramebufferWidth >> 1;
    int NewHeight = FramebufferHeight >> 1;

    if ( Bloom.Width != NewWidth || Bloom.Height != NewHeight ) {
        Bloom.Width = NewWidth;
        Bloom.Height = NewHeight;

        Bloom.Framebuffer.Deinitialize();
        Bloom.Framebuffer_2.Deinitialize();
        Bloom.Framebuffer_4.Deinitialize();
        Bloom.Framebuffer_6.Deinitialize();

        for ( int i = 0 ; i < 2 ; i++ ) {
            Bloom.Texture[i].Deinitialize();
            Bloom.Textures_2[i].Deinitialize();
            Bloom.Textures_4[i].Deinitialize();
            Bloom.Textures_6[i].Deinitialize();
        }

        TextureStorageCreateInfo texStorageCI = {};
        texStorageCI.Type = GHI::TEXTURE_2D;
        texStorageCI.NumLods = 1;
        texStorageCI.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RGB16F;

        texStorageCI.Resolution.Tex2D.Width = Bloom.Width;
        texStorageCI.Resolution.Tex2D.Height = Bloom.Height;
        Bloom.Texture[0].InitializeStorage( texStorageCI );
        Bloom.Texture[1].InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = Bloom.Width>>2;
        texStorageCI.Resolution.Tex2D.Height = Bloom.Height>>2;
        Bloom.Textures_2[0].InitializeStorage( texStorageCI );
        Bloom.Textures_2[1].InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = Bloom.Width>>4;
        texStorageCI.Resolution.Tex2D.Height = Bloom.Height>>4;
        Bloom.Textures_4[0].InitializeStorage( texStorageCI );
        Bloom.Textures_4[1].InitializeStorage( texStorageCI );

        texStorageCI.Resolution.Tex2D.Width = Bloom.Width>>6;
        texStorageCI.Resolution.Tex2D.Height = Bloom.Height>>6;
        Bloom.Textures_6[0].InitializeStorage( texStorageCI );
        Bloom.Textures_6[1].InitializeStorage( texStorageCI );

        Bloom.TexturePtrs[0] = &Bloom.Texture[0];
        Bloom.TexturePtrs[1] = &Bloom.Texture[1];
        Bloom.TexturePtrs[2] = &Bloom.Textures_2[0];
        Bloom.TexturePtrs[3] = &Bloom.Textures_2[1];
        Bloom.TexturePtrs[4] = &Bloom.Textures_4[0];
        Bloom.TexturePtrs[5] = &Bloom.Textures_4[1];
        Bloom.TexturePtrs[6] = &Bloom.Textures_6[0];
        Bloom.TexturePtrs[7] = &Bloom.Textures_6[1];

        FramebufferAttachmentInfo colorAttachments[2] = {};

        FramebufferCreateInfo framebufferCI = {};
        framebufferCI.NumColorAttachments = 2;
        framebufferCI.pColorAttachments = colorAttachments;

        framebufferCI.Width = Bloom.Width;
        framebufferCI.Height = Bloom.Height;
        colorAttachments[0].pTexture = &Bloom.Texture[0];
        colorAttachments[1].pTexture = &Bloom.Texture[1];
        Bloom.Framebuffer.Initialize( framebufferCI );

        framebufferCI.Width = Bloom.Width>>2;
        framebufferCI.Height = Bloom.Height>>2;
        colorAttachments[0].pTexture = &Bloom.Textures_2[0];
        colorAttachments[1].pTexture = &Bloom.Textures_2[1];
        Bloom.Framebuffer_2.Initialize( framebufferCI );

        framebufferCI.Width = Bloom.Width>>4;
        framebufferCI.Height = Bloom.Height>>4;
        colorAttachments[0].pTexture = &Bloom.Textures_4[0];
        colorAttachments[1].pTexture = &Bloom.Textures_4[1];
        Bloom.Framebuffer_4.Initialize( framebufferCI );

        framebufferCI.Width = Bloom.Width>>6;
        framebufferCI.Height = Bloom.Height>>6;
        colorAttachments[0].pTexture = &Bloom.Textures_6[0];
        colorAttachments[1].pTexture = &Bloom.Textures_6[1];
        Bloom.Framebuffer_6.Initialize( framebufferCI );
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
