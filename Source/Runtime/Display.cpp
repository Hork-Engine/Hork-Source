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

#include "Display.h"
#include <Platform/Logger.h>

#include <SDL.h>

HK_NAMESPACE_BEGIN

namespace Runtime
{

void GetDisplays(TPodVector<DisplayInfo>& Displays)
{
    SDL_Rect rect;
    int      displayCount = SDL_GetNumVideoDisplays();

    Displays.ResizeInvalidate(displayCount);

    for (int i = 0; i < displayCount; i++)
    {
        DisplayInfo& d = Displays[i];

        d.Id   = i;
        d.Name = SDL_GetDisplayName(i);

        SDL_GetDisplayBounds(i, &rect);

        d.DisplayX = rect.x;
        d.DisplayY = rect.y;
        d.DisplayW = rect.w;
        d.DisplayH = rect.h;

        SDL_GetDisplayUsableBounds(i, &rect);

        d.DisplayUsableX = rect.x;
        d.DisplayUsableY = rect.y;
        d.DisplayUsableW = rect.w;
        d.DisplayUsableH = rect.h;

        d.Orientation = (DISPLAY_ORIENTATION)SDL_GetDisplayOrientation(i);

        SDL_GetDisplayDPI(i, &d.ddpi, &d.hdpi, &d.vdpi);
    }
}

void GetDisplayModes(DisplayInfo const& Display, TPodVector<DisplayMode>& Modes)
{
    SDL_DisplayMode modeSDL;

    int numModes = SDL_GetNumDisplayModes(Display.Id);

    Modes.Clear();
    for (int i = 0; i < numModes; i++)
    {
        SDL_GetDisplayMode(Display.Id, i, &modeSDL);

        if (modeSDL.format == SDL_PIXELFORMAT_RGB888)
        {
            DisplayMode& mode = Modes.Add();

            mode.Width       = modeSDL.w;
            mode.Height      = modeSDL.h;
            mode.RefreshRate = modeSDL.refresh_rate;
        }
    }
}

void GetDesktopDisplayMode(DisplayInfo const& Display, DisplayMode& Mode)
{
    SDL_DisplayMode modeSDL;
    SDL_GetDesktopDisplayMode(Display.Id, &modeSDL);

    Mode.Width       = modeSDL.w;
    Mode.Height      = modeSDL.h;
    Mode.RefreshRate = modeSDL.refresh_rate;
}

void GetCurrentDisplayMode(DisplayInfo const& Display, DisplayMode& Mode)
{
    SDL_DisplayMode modeSDL;
    SDL_GetCurrentDisplayMode(Display.Id, &modeSDL);

    Mode.Width       = modeSDL.w;
    Mode.Height      = modeSDL.h;
    Mode.RefreshRate = modeSDL.refresh_rate;
}

bool GetClosestDisplayMode(DisplayInfo const& Display, int Width, int Height, int RefreshRate, DisplayMode& Mode)
{
    SDL_DisplayMode modeSDL, closestSDL;

    modeSDL.w            = Width;
    modeSDL.h            = Height;
    modeSDL.refresh_rate = RefreshRate;
    modeSDL.format       = SDL_PIXELFORMAT_RGB888;
    modeSDL.driverdata   = nullptr;
    if (!SDL_GetClosestDisplayMode(Display.Id, &modeSDL, &closestSDL) || closestSDL.format != modeSDL.format)
    {
        LOG("Couldn't find closest display mode to {} x {} {}Hz\n", Width, Height, RefreshRate);
        Platform::ZeroMem(&Mode, sizeof(Mode));
        return false;
    }

    Mode.Width       = closestSDL.w;
    Mode.Height      = closestSDL.h;
    Mode.RefreshRate = closestSDL.refresh_rate;

    return true;
}

} // namespace Runtime

HK_NAMESPACE_END
