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

#include "OpenGL45Common.h"

namespace OpenGL45 {

struct SBloomTexture
{
    GHI::Framebuffer Framebuffer;
    GHI::Framebuffer Framebuffer_2;
    GHI::Framebuffer Framebuffer_4;
    GHI::Framebuffer Framebuffer_6;
    GHI::Texture Texture[2];
    GHI::Texture Textures_2[2];
    GHI::Texture Textures_4[2];
    GHI::Texture Textures_6[2];
    GHI::Texture * TexturePtrs[8];
    int Width;
    int Height;
};

class ARenderTarget {
public:
    void Initialize();
    void Deinitialize();

    void ReallocSurface( int _AllocSurfaceWidth, int _AllocSurfaceHeight );

    GHI::Framebuffer & GetFramebuffer() { return Framebuffer; }
    GHI::Texture & GetFramebufferTexture() { return FramebufferTexture; }

    GHI::Framebuffer & GetPostprocessFramebuffer() { return PostprocessFramebuffer; }
    GHI::Texture & GetPostprocessTexture() { return PostprocessTexture; }

    GHI::Framebuffer & GetFxaaFramebuffer() { return FxaaFramebuffer; }
    GHI::Texture & GetFxaaTexture() { return FxaaTexture; }

    SBloomTexture & GetBloomTexture() { return Bloom; }

    GHI::Texture Luminance64;
    GHI::Texture Luminance32;
    GHI::Texture Luminance16;
    GHI::Texture Luminance8;
    GHI::Texture Luminance4;
    GHI::Texture Luminance2;
    GHI::Framebuffer FramebufferLum64;
    GHI::Framebuffer FramebufferLum32;
    GHI::Framebuffer FramebufferLum16;
    GHI::Framebuffer FramebufferLum8;
    GHI::Framebuffer FramebufferLum4;
    GHI::Framebuffer FramebufferLum2;
    GHI::Framebuffer FramebufferLum1;
    GHI::Texture AdaptiveLuminance;
private:
    void CreateFramebuffer();
    void CreateBloomTextures();

    GHI::Framebuffer Framebuffer;
    GHI::Framebuffer PostprocessFramebuffer;
    GHI::Framebuffer FxaaFramebuffer;
    int FramebufferWidth;
    int FramebufferHeight;
    GHI::Texture FramebufferTexture;
    GHI::Texture FramebufferDepth;
    GHI::Texture PostprocessTexture;
    GHI::Texture FxaaTexture;
    SBloomTexture Bloom;
};

extern ARenderTarget GRenderTarget;

}
