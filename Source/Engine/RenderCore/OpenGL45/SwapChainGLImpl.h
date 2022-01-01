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

#pragma once

#include <RenderCore/SwapChain.h>
#include "TextureGLImpl.h"

struct SDL_Window;

namespace RenderCore
{

class ADeviceGLImpl;

class ASwapChainGLImpl : public ISwapChain
{
public:
    ASwapChainGLImpl(ADeviceGLImpl* pDevice, SDL_Window* pWindow);

    void Present(int SwapInterval = 1) override;

    void Resize(int Width, int Height) override;

    ITexture* GetBackBuffer() override;
    ITexture* GetDepthBuffer() override;

private:
    SDL_Window*          pWindow; // TODO: strong or weak ptr
    TRef<ATextureGLImpl> BackBuffer;
    TRef<ATextureGLImpl> DepthBuffer;
};

} // namespace RenderCore