/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Core/Platform.h>
#include <Hork/Core/BaseMath.h>

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

namespace RHI
{

IGenericWindow::IGenericWindow(IDevice* pDevice) :
    IDeviceObject(pDevice, PROXY_TYPE)
{}

IGenericWindow* IGenericWindow::sGetWindowFromNativeHandle(SDL_Window* handle)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(handle);
    return (IGenericWindow*)SDL_GetPointerProperty(props, "p", nullptr);
}

void IGenericWindow::SetTitle(StringView title)
{
    if (title.IsNullTerminated())
        SDL_SetWindowTitle((SDL_Window*)GetHandle(), title.ToPtr());
    else
        SDL_SetWindowTitle((SDL_Window*)GetHandle(), String(title).CStr());
}

void IGenericWindow::ChangeWindowSettings(WindowSettings const& windowSettings)
{
    SDL_Window* handle = (SDL_Window*)GetHandle();

    SDL_ShowWindow(handle);

    m_RefreshRate = 0;
    switch (windowSettings.Mode)
    {
        case WindowMode::Windowed:
            SDL_SetWindowFullscreen(handle, SDL_FALSE);
            SDL_SetWindowSize(handle, windowSettings.Width, windowSettings.Height);
            if (windowSettings.bCentrized)
                SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            else
                SDL_SetWindowPosition(handle, windowSettings.WindowedX, windowSettings.WindowedY);
            break;
        case WindowMode::BorderlessFullscreen:
            SDL_SetWindowFullscreenMode(handle, nullptr);
            SDL_SetWindowFullscreen(handle, SDL_TRUE);
            break;
        case WindowMode::ExclusiveFullscreen: {
            SDL_DisplayMode mode = {};
            if (SDL_GetClosestFullscreenDisplayMode(SDL_GetDisplayForWindow(handle), windowSettings.Width, windowSettings.Height, windowSettings.RefreshRate, SDL_TRUE, &mode))
            {
                SDL_SetWindowFullscreenMode(handle, &mode);
                m_RefreshRate = mode.refresh_rate;
            }
            else
            {
                SDL_SetWindowFullscreenMode(handle, nullptr);
            }
            SDL_SetWindowFullscreen(handle, SDL_TRUE);
            break;
        }
    }
    
    if (m_RefreshRate == 0.0f)
    {
        if (SDL_DisplayMode const* mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(handle)))
            m_RefreshRate = mode->refresh_rate;
    }

    m_WindowMode = windowSettings.Mode;
}

void IGenericWindow::ParseEvent(SDL_WindowEvent const& event)
{
    if (SDL_GetWindowFromID(event.windowID) != GetHandle())
        return;

    bool updateAspectScale = false;

    switch (event.type)
    {
        // Window has been shown
        case SDL_EVENT_WINDOW_SHOWN:
            break;
        // Window has been hidden
        case SDL_EVENT_WINDOW_HIDDEN:
            break;
        // Window has been exposed and should be redrawn
        case SDL_EVENT_WINDOW_EXPOSED:
            break;
        // Window has been moved to data1, data2
        case SDL_EVENT_WINDOW_MOVED:
            m_X = event.data1;
            m_Y = event.data2;
            if (!m_FullscreenMode)
            {
                m_WindowedX = event.data1;
                m_WindowedY = event.data2;
            }
            break;
        // Window has been resized to data1xdata2
        case SDL_EVENT_WINDOW_RESIZED:
            m_Width  = event.data1;
            m_Height = event.data2;
            break;
        // The window size has changed, either as
        // a result of an API call or through the
        // system or user changing the window size.
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            m_FramebufferWidth  = event.data1;
            m_FramebufferHeight = event.data2;
            if (m_SwapChain)
                m_SwapChain->Resize(m_FramebufferWidth, m_FramebufferHeight);
            updateAspectScale = true;
            break;
        // The pixel size of a Metal view associated with the window has changed
        case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
            break;
        // Window has been minimized
        case SDL_EVENT_WINDOW_MINIMIZED:
            break;
        // Window has been maximized
        case SDL_EVENT_WINDOW_MAXIMIZED:
            break;
        // Window has been restored to normal size and position
        case SDL_EVENT_WINDOW_RESTORED:
            break;
        // Window has gained mouse focus
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            break;
        // Window has lost mouse focus
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            break;
        // Window has gained keyboard focus
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            break;
        // Window has lost keyboard focus
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            break;
        // The window manager requests that the window be closed
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            break;
        // Window had a hit test that wasn't SDL_HITTEST_NORMAL.
        case SDL_EVENT_WINDOW_HIT_TEST:
            break;
        // The ICC profile of the window's display has changed
        case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
            break;
        // The window safe area has been changed
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
            break;
        // The window has been occluded
        case SDL_EVENT_WINDOW_OCCLUDED:
            break;
        // The window has entered fullscreen mode
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            m_FullscreenMode = true;
            updateAspectScale = true;
            break;
        // The window has left fullscreen mode
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            m_FullscreenMode = false;
            updateAspectScale = true;
            break;
        // Window has been moved to display data1
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            updateAspectScale = true;
            break;
        // Window display scale has been changed
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            break;
        // Window HDR properties have changed
        case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
            break;
    }

    if (updateAspectScale)
    {
        m_WideScreenCorrection = 1;

        if (m_FullscreenMode && m_FramebufferWidth != 0 && m_FramebufferHeight != 0)
        {
            if (SDL_DisplayMode const* mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow((SDL_Window*)GetHandle())))
            {
                float sx = (float)mode->w / m_FramebufferWidth;
                float sy = (float)mode->h / m_FramebufferHeight;
                m_WideScreenCorrection = sx / sy;
            }
        }
    }
}

float IGenericWindow::GetWindowDPI() const
{
    float scale = SDL_GetWindowDisplayScale((SDL_Window*)GetHandle());
    if (scale == 0.0f)
        scale = 1.0f;

#if defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_IOS)
    return scale * 160.0f;
#else
    return scale * 96.0f;
#endif
}

float IGenericWindow::GetRefreshRate() const
{
    return m_RefreshRate;
}

void IGenericWindow::SetOpacity(float opacity)
{
    opacity = Math::Clamp(opacity, 0.0f, 1.0f);

    if (m_Opacity != opacity)
    {
        SDL_SetWindowOpacity((SDL_Window*)GetHandle(), opacity);
        m_Opacity = opacity;
    }
}

void IGenericWindow::SetCursorEnabled(bool bEnabled)
{
    //SDL_SetWindowMouseGrab((SDL_Window*)GetHandle(), (SDL_bool)!bEnabled);
    //SDL_SetWindowKeyboardGrab((SDL_Window*)GetHandle(), (SDL_bool)!bEnabled);
    SDL_SetWindowRelativeMouseMode((SDL_Window*)GetHandle(), (SDL_bool)!bEnabled);
}

bool IGenericWindow::IsCursorEnabled() const
{
    return !SDL_GetWindowRelativeMouseMode((SDL_Window*)GetHandle());
}

} // namespace RHI

HK_NAMESPACE_END
