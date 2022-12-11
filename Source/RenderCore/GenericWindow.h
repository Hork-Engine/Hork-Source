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

#pragma once

#include "DeviceObject.h"
#include "SwapChain.h"

struct SDL_Window;
union SDL_Event;

struct DisplayVideoMode
{
    /** Horizontal position on display (read only) */
    int X;
    /** Vertical position on display (read only) */
    int Y;
    /** Horizontal position on display in windowed mode. */
    int WindowedX;
    /** Vertical position on display in windowed mode. */
    int WindowedY;
    /** Horizontal display resolution */
    int Width;
    /** Vertical display resolution */
    int Height;
    /** Video mode framebuffer width (for Retina displays, read only) */
    int FramebufferWidth;
    /** Video mode framebuffer width (for Retina displays, read only) */
    int FramebufferHeight;
    /** Physical monitor (read only) */
    int DisplayId;
    /** Display refresh rate (read only) */
    int RefreshRate;
    /** Display dots per inch (read only) */
    float DPI_X;
    /** Display dots per inch (read only) */
    float DPI_Y;
    /** Viewport aspect ratio scale (read only) */
    float AspectScale;
    /** Window opacity */
    float Opacity;
    /** Fullscreen or Windowed mode */
    bool bFullscreen;
    /** Move window to center of the screen. WindowedX and WindowedY will be ignored. */
    bool bCentrized;
    /** Render backend name */
    char Backend[32];
    /** Window title */
    char Title[128];
};

namespace RenderCore
{

class IGenericWindow : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_WINDOW;

    IGenericWindow(IDevice* pDevice) :
        IDeviceObject(pDevice, PROXY_TYPE)
    {}

    virtual void      SetVideoMode(DisplayVideoMode const& DesiredMode) = 0;
    DisplayVideoMode const& GetVideoMode() const { return VideoMode; }

    void ParseEvent(SDL_Event const& Event);

    static IGenericWindow* GetWindowFromNativeHandle(SDL_Window* Handle);

protected:
    DisplayVideoMode  VideoMode;
    TWeakRef<ISwapChain> SwapChain;
};

} // namespace RenderCore
