/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "GenericWindowGLImpl.h"
#include "DeviceGLImpl.h"

#include <Platform/Platform.h>
#include <Platform/WindowsDefs.h>
#include <Core/BaseMath.h>

#include "GL/glew.h"
#ifdef AN_OS_WIN32
#    include "GL/wglew.h"
#endif
#ifdef AN_OS_LINUX
#    include "GL/glxew.h"
#endif

#include <SDL.h>

namespace RenderCore
{

AGenericWindowGLImpl::AGenericWindowGLImpl(ADeviceGLImpl* pDevice, SVideoMode const& InVideoMode, AWindowPoolGL& WindowPool, AWindowPoolGL::SWindowGL WindowHandle) :
    IGenericWindow(pDevice),
    WindowPool(WindowPool),
    bUseExternalHandle(WindowHandle.Handle != nullptr)
{
    if (bUseExternalHandle)
    {
        WindowGL = WindowHandle;
    }
    else
    {
        WindowGL = WindowPool.Create();
        if (!WindowGL.ImmediateCtx)
        {
            WindowGL.ImmediateCtx = new AImmediateContextGLImpl(pDevice, WindowGL, false);
        }
    }

    SetHandle(WindowGL.Handle);

    SDL_SetWindowData(WindowGL.Handle, "p", this);

    SetVideoMode(InVideoMode);
}

AGenericWindowGLImpl::~AGenericWindowGLImpl()
{
    SDL_SetWindowData((SDL_Window*)GetHandle(), "p", nullptr);

    if (bUseExternalHandle)
    {
        SDL_HideWindow(WindowGL.Handle);
    }
    else
    {
        if (WindowGL.ImmediateCtx)
        {
            WindowGL.ImmediateCtx->RemoveRef();
            WindowGL.ImmediateCtx = nullptr;
        }

        WindowPool.Destroy(WindowGL);
    }
}

void AGenericWindowGLImpl::SetSwapChain(ISwapChain* InSwapChain)
{
    SwapChain = InSwapChain;
}

void AGenericWindowGLImpl::SetVideoMode(SVideoMode const& DesiredMode)
{
    SDL_Window* handle = (SDL_Window*)GetHandle();

    Platform::Memcpy(&VideoMode, &DesiredMode, sizeof(VideoMode));

    SDL_SetWindowTitle(WindowGL.Handle, DesiredMode.Title);
    SDL_ShowWindow(handle);
    SDL_SetWindowFullscreen(handle, DesiredMode.bFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    SDL_SetWindowSize(handle, DesiredMode.Width, DesiredMode.Height);
    if (!DesiredMode.bFullscreen)
    {
        if (DesiredMode.bCentrized)
        {
            SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
        else
        {
            SDL_SetWindowPosition(handle, DesiredMode.WindowedX, DesiredMode.WindowedY);
        }
    }
    //SDL_SetWindowDisplayMode( handle, &mode );
    SDL_GL_GetDrawableSize(handle, &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight);

    VideoMode.bFullscreen = !!(SDL_GetWindowFlags(handle) & SDL_WINDOW_FULLSCREEN);

    VideoMode.Opacity = Math::Clamp(VideoMode.Opacity, 0.0f, 1.0f);

    float opacity = 1.0f;
    SDL_GetWindowOpacity(handle, &opacity);
    if (Math::Abs(VideoMode.Opacity - opacity) > 1.0f / 255.0f)
    {
        SDL_SetWindowOpacity(handle, VideoMode.Opacity);
    }

    VideoMode.DisplayId = SDL_GetWindowDisplayIndex(handle);
    /*if ( VideoMode.bFullscreen ) {
        const float MM_To_Inch = 0.0393701f;
        VideoMode.DPI_X = (float)VideoMode.Width / (monitor->PhysicalWidthMM*MM_To_Inch);
        VideoMode.DPI_Y = (float)VideoMode.Height / (monitor->PhysicalHeightMM*MM_To_Inch);

    } else */
    {
        SDL_GetDisplayDPI(VideoMode.DisplayId, NULL, &VideoMode.DPI_X, &VideoMode.DPI_Y);
    }

    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(handle, &mode);
    VideoMode.RefreshRate = mode.refresh_rate;
}

} // namespace RenderCore
