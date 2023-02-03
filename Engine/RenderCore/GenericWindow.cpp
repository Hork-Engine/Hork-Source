/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "GenericWindow.h"

#include <Engine/Core/Platform/Platform.h>
#include <Engine/Core/BaseMath.h>

#include <SDL/SDL.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

IGenericWindow* IGenericWindow::GetWindowFromNativeHandle(SDL_Window* Handle)
{
    return (IGenericWindow*)SDL_GetWindowData(Handle, "p");
}

void IGenericWindow::ParseEvent(SDL_Event const& event)
{
    if (event.type != SDL_WINDOWEVENT)
        return;

    HK_ASSERT(SDL_GetWindowFromID(event.window.windowID) == GetHandle());

    switch (event.window.event)
    {
        // Window has been shown
        case SDL_WINDOWEVENT_SHOWN:
            break;
        // Window has been hidden
        case SDL_WINDOWEVENT_HIDDEN:
            break;
        // Window has been exposed and should be redrawn
        case SDL_WINDOWEVENT_EXPOSED:
            break;
        // Window has been moved to data1, data2
        case SDL_WINDOWEVENT_MOVED:
            VideoMode.DisplayId = SDL_GetWindowDisplayIndex((SDL_Window*)GetHandle());
            VideoMode.X         = event.window.data1;
            VideoMode.Y         = event.window.data2;
            if (!VideoMode.bFullscreen)
            {
                VideoMode.WindowedX = event.window.data1;
                VideoMode.WindowedY = event.window.data2;
            }
            break;
        // Window has been resized to data1xdata2
        case SDL_WINDOWEVENT_RESIZED:
        // The window size has changed, either as
        // a result of an API call or through the
        // system or user changing the window size.
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
            VideoMode.Width     = event.window.data1;
            VideoMode.Height    = event.window.data2;
            VideoMode.DisplayId = SDL_GetWindowDisplayIndex((SDL_Window*)GetHandle());
            SDL_GL_GetDrawableSize((SDL_Window*)GetHandle(), &VideoMode.FramebufferWidth, &VideoMode.FramebufferHeight);
            if (VideoMode.bFullscreen)
            {
                SDL_DisplayMode mode;
                SDL_GetDesktopDisplayMode(VideoMode.DisplayId, &mode);
                float sx              = (float)mode.w / VideoMode.FramebufferWidth;
                float sy              = (float)mode.h / VideoMode.FramebufferHeight;
                VideoMode.AspectScale = (sx / sy);
            }
            else
            {
                VideoMode.AspectScale = 1;
            }
            if (SwapChain)
            {
                SwapChain->Resize(VideoMode.FramebufferWidth, VideoMode.FramebufferHeight);
            }
            break;
        }
        // Window has been minimized
        case SDL_WINDOWEVENT_MINIMIZED:
            break;
        // Window has been maximized
        case SDL_WINDOWEVENT_MAXIMIZED:
            break;
        // Window has been restored to normal size and position
        case SDL_WINDOWEVENT_RESTORED:
            break;
        // Window has gained mouse focus
        case SDL_WINDOWEVENT_ENTER:
            break;
        // Window has lost mouse focus
        case SDL_WINDOWEVENT_LEAVE:
            break;
        // Window has gained keyboard focus
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            break;
        // Window has lost keyboard focus
        case SDL_WINDOWEVENT_FOCUS_LOST:
            break;
        // The window manager requests that the window be closed
        case SDL_WINDOWEVENT_CLOSE:
            break;
        // Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore)
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            break;
        // Window had a hit test that wasn't SDL_HITTEST_NORMAL.
        case SDL_WINDOWEVENT_HIT_TEST:
            break;
    }
}

} // namespace RenderCore

HK_NAMESPACE_END
