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

#include "SwapChainGLImpl.h"
#include "DeviceGLImpl.h"

#include <SDL.h>

namespace RenderCore
{

ASwapChainGLImpl::ASwapChainGLImpl(ADeviceGLImpl* pDevice, SDL_Window* pWindow) :
    ISwapChain(pDevice), pWindow(pWindow)
{
    SDL_GL_GetDrawableSize(pWindow, &Width, &Height);

    STextureDesc textureDesc;
    textureDesc.SetResolution(STextureResolution2D(Width, Height));

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_RGBA8);

    BackBuffer = MakeRef<ATextureGLImpl>(pDevice, textureDesc, true);

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_DEPTH32);

    DepthBuffer = MakeRef<ATextureGLImpl>(pDevice, textureDesc, true);
}

void ASwapChainGLImpl::Present(int SwapInterval)
{
    static int CurrentSwapInterval = 666;

    SwapInterval = Math::Clamp(SwapInterval, -1, 1);
    if (SwapInterval == -1 && !GetDevice()->IsFeatureSupported(FEATURE_SWAP_CONTROL_TEAR))
    {
        // Tearing not supported
        SwapInterval = 0;
    }

    if (CurrentSwapInterval != SwapInterval)
    {
        GLogger.Printf("Changing swap interval to %d\n", SwapInterval);

        SDL_GL_SetSwapInterval(SwapInterval);
        CurrentSwapInterval = SwapInterval;
    }

    SDL_GL_SwapWindow(pWindow);
}

void ASwapChainGLImpl::Resize(int InWidth, int InHeight)
{
    if (Width == InWidth && Height == InHeight)
    {
        return;
    }

    Width = InWidth;
    Height = InHeight;

    STextureDesc textureDesc;
    textureDesc.SetResolution(STextureResolution2D(Width, Height));

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_RGBA8);

    BackBuffer = MakeRef<ATextureGLImpl>(static_cast<ADeviceGLImpl*>(GetDevice()), textureDesc, true);

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_DEPTH32);

    DepthBuffer = MakeRef<ATextureGLImpl>(static_cast<ADeviceGLImpl*>(GetDevice()), textureDesc, true);
}

ITexture* ASwapChainGLImpl::GetBackBuffer()
{
    return BackBuffer;
}

ITexture* ASwapChainGLImpl::GetDepthBuffer()
{
    return DepthBuffer;
}

} // namespace RenderCore
