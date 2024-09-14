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

#pragma once

#include "DeviceObject.h"
#include "SwapChain.h"

struct SDL_Window;
struct SDL_WindowEvent;

HK_NAMESPACE_BEGIN

enum class WindowMode : uint8_t
{
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen
};

struct WindowSettings
{
    /// Horizontal position on display in windowed mode
    int WindowedX;
    /// Vertical position on display in windowed mode
    int WindowedY;
    /// Horizontal display resolution
    int Width;
    /// Vertical display resolution
    int Height;
    /// Refresh rate
    float RefreshRate;
    /// Fullscreen or Windowed mode
    WindowMode Mode;
    /// Move window to center of the screen. WindowedX and WindowedY will be ignored
    bool bCentrized;
};

namespace RHI
{

class IGenericWindow : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_WINDOW;

                        IGenericWindow(IDevice* pDevice);

    void                SetTitle(StringView title);

    void                ChangeWindowSettings(WindowSettings const& windowSettings);

    float               GetWindowDPI() const;

    float               GetRefreshRate() const;

    float               GetWideScreenCorrection() const { return m_WideScreenCorrection; }

    WindowMode          GetWindowMode() const { return m_WindowMode; }

    bool                IsFullscreenMode() const { return m_FullscreenMode; }

    /// Horizontal position on display
    int                 GetX() const { return m_X; }

    /// Vertical position on display
    int                 GetY() const { return m_Y; }

    /// Horizontal position on display in windowed mode
    int                 GetWindowedX() const { return m_WindowedX; }

    /// Vertical position on display in windowed mode
    int                 GetWindowedY() const { return m_WindowedY; }

    int                 GetWidth() const { return m_Width; }
    int                 GetHeight() const { return m_Height; }

    int                 GetFramebufferWidth() const { return m_FramebufferWidth; }
    int                 GetFramebufferHeight() const { return m_FramebufferHeight; }

    void                SetOpacity(float opacity);
    float               GetOpacity() const { return m_Opacity; }

    void                SetCursorEnabled(bool bEnabled);
    bool                IsCursorEnabled() const;

    void                ParseEvent(SDL_WindowEvent const& event);

    static IGenericWindow* sGetWindowFromNativeHandle(SDL_Window* handle);

protected:
    WeakRef<ISwapChain> m_SwapChain;
    float               m_RefreshRate = 1.0f / 60;
    int                 m_FramebufferWidth = 0;
    int                 m_FramebufferHeight = 0;
    float               m_WideScreenCorrection = 1.0f;
    int                 m_X = 0;
    int                 m_Y = 0;
    int                 m_WindowedX = 0;
    int                 m_WindowedY = 0;
    int                 m_Width = 0;
    int                 m_Height = 0;
    WindowMode          m_WindowMode = WindowMode::Windowed;
    bool                m_FullscreenMode = false;
    float               m_Opacity = 1.0f;
};

} // namespace RHI

HK_NAMESPACE_END
