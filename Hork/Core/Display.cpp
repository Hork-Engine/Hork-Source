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

#include "Display.h"
#include "Logger.h"

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

namespace Core
{

void GetDisplays(Vector<DisplayInfo>& displays)
{
    displays.Clear();

    int displayCount = 0;
    if (SDL_DisplayID *displayIDs = SDL_GetDisplays(&displayCount))
    {
        SDL_Rect rect;

        displays.Reserve(displayCount);

        for (int i = 0; i < displayCount; ++i)
        {
            SDL_DisplayID instanceID = displayIDs[i];

            DisplayInfo& d = displays.Add();

            d.Id   = instanceID;
            d.Name = SDL_GetDisplayName(instanceID);

            SDL_GetDisplayBounds(instanceID, &rect);

            d.DisplayX = rect.x;
            d.DisplayY = rect.y;
            d.DisplayW = rect.w;
            d.DisplayH = rect.h;

            SDL_GetDisplayUsableBounds(instanceID, &rect);

            d.DisplayUsableX = rect.x;
            d.DisplayUsableY = rect.y;
            d.DisplayUsableW = rect.w;
            d.DisplayUsableH = rect.h;

            d.Orientation = (DISPLAY_ORIENTATION)SDL_GetCurrentDisplayOrientation(instanceID);
        }
        SDL_free(displayIDs);
    }
}

void GetDisplayModes(DisplayInfo const& display, Vector<DisplayMode>& modes)
{
    modes.Clear();

    int numModes = 0;
    if (SDL_DisplayMode** dispModes = SDL_GetFullscreenDisplayModes(display.Id, &numModes))
    {
        modes.Reserve(numModes);
        for (int i = 0; i < numModes; ++i)
        {
            SDL_DisplayMode const *modeSDL = dispModes[i];

            //if (modeSDL->format == SDL_PIXELFORMAT_XRGB8888)
            {
                DisplayMode& mode = modes.Add();
                mode.Width       = modeSDL->w;
                mode.Height      = modeSDL->h;
                mode.RefreshRate = modeSDL->refresh_rate;
            }
        }
        SDL_free(dispModes);
    }
}

void GetDesktopDisplayMode(DisplayInfo const& display, DisplayMode& mode)
{
    SDL_DisplayMode const* modeSDL = SDL_GetDesktopDisplayMode(display.Id);
    HK_ASSERT(modeSDL);

    if (modeSDL)
    {
        mode.Width       = modeSDL->w;
        mode.Height      = modeSDL->h;
        mode.RefreshRate = modeSDL->refresh_rate;
    }
    else
    {
        Core::ZeroMem(&mode, sizeof(mode));
    }
}

void GetCurrentDisplayMode(DisplayInfo const& display, DisplayMode& mode)
{
    SDL_DisplayMode const* modeSDL = SDL_GetCurrentDisplayMode(display.Id);
    HK_ASSERT(modeSDL);

    if (modeSDL)
    {
        mode.Width       = modeSDL->w;
        mode.Height      = modeSDL->h;
        mode.RefreshRate = modeSDL->refresh_rate;
    }
    else
    {
        Core::ZeroMem(&mode, sizeof(mode));
    }
}

bool GetClosestDisplayMode(DisplayInfo const& display, int width, int height, float refreshRate, bool includeHightDensityModes, DisplayMode& mode)
{
    SDL_DisplayMode modeSDL = {};

    if (!SDL_GetClosestFullscreenDisplayMode(display.Id, width, height, refreshRate, includeHightDensityModes, &modeSDL))
    {
        LOG("Couldn't find closest display mode to {} x {} {:.2f}Hz\n", width, height, refreshRate);
        Core::ZeroMem(&mode, sizeof(mode));
        return false;
    }

    mode.Width       = modeSDL.w;
    mode.Height      = modeSDL.h;
    mode.RefreshRate = modeSDL.refresh_rate;

    return true;
}

} // namespace Core

HK_NAMESPACE_END
