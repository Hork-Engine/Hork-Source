/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "SwapChainGLImpl.h"
#include "DeviceGLImpl.h"
#include "GenericWindowGLImpl.h"

#include <SDL/SDL.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

SwapChainGLImpl::SwapChainGLImpl(DeviceGLImpl* pDevice, GenericWindowGLImpl* pWindow) :
    ISwapChain(pDevice), pWindow(pWindow)
{
    SDL_GL_GetDrawableSize((SDL_Window*)pWindow->GetHandle(), &Width, &Height);

    TextureDesc textureDesc;
    textureDesc.SetResolution(TextureResolution2D(Width, Height));

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_RGBA8_UNORM);

    BackBuffer = MakeRef<TextureGLImpl>(pDevice, textureDesc, true);
    BackBuffer->pContext = static_cast<ImmediateContextGLImpl*>(pWindow->GetImmediateContext());

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_D32);

    DepthBuffer = MakeRef<TextureGLImpl>(pDevice, textureDesc, true);
    DepthBuffer->pContext = static_cast<ImmediateContextGLImpl*>(pWindow->GetImmediateContext());

    pWindow->SetSwapChain(this);
}

void SwapChainGLImpl::Present(int SwapInterval)
{
    ScopedContextGL ScopedContext(pWindow->GetImmediateContext());

    SwapInterval = Math::Clamp(SwapInterval, -1, 1);
    if (SwapInterval == -1 && !GetDevice()->IsFeatureSupported(FEATURE_SWAP_CONTROL_TEAR))
    {
        // Tearing not supported
        SwapInterval = 0;
    }

    if (pWindow->CurrentSwapInterval != SwapInterval)
    {
        LOG("Changing swap interval to {}\n", SwapInterval);

        SDL_GL_SetSwapInterval(SwapInterval);
        pWindow->CurrentSwapInterval = SwapInterval;
    }

    SDL_GL_SwapWindow((SDL_Window*)pWindow->GetHandle());
}

void SwapChainGLImpl::Resize(int InWidth, int InHeight)
{
    if (Width == InWidth && Height == InHeight)
    {
        return;
    }

    Width = InWidth;
    Height = InHeight;

    TextureDesc textureDesc;
    textureDesc.SetResolution(TextureResolution2D(Width, Height));

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_RGBA8_UNORM);

    BackBuffer = MakeRef<TextureGLImpl>(static_cast<DeviceGLImpl*>(GetDevice()), textureDesc, true);
    BackBuffer->pContext = static_cast<ImmediateContextGLImpl*>(pWindow->GetImmediateContext());

    // TODO: grab format from default framebuffer
    textureDesc.SetFormat(TEXTURE_FORMAT_D32);

    DepthBuffer = MakeRef<TextureGLImpl>(static_cast<DeviceGLImpl*>(GetDevice()), textureDesc, true);
    DepthBuffer->pContext = static_cast<ImmediateContextGLImpl*>(pWindow->GetImmediateContext());
}

ITexture* SwapChainGLImpl::GetBackBuffer()
{
    return BackBuffer;
}

ITexture* SwapChainGLImpl::GetDepthBuffer()
{
    return DepthBuffer;
}

} // namespace RenderCore

HK_NAMESPACE_END
