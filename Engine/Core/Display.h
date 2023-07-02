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

#pragma once

#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

enum DISPLAY_ORIENTATION
{
    /** The display orientation can't be determined */
    DISPLAY_ORIENTATION_UNKNOWN,
    /** The display is in landscape mode, with the right side up, relative to portrait mode */
    DISPLAY_ORIENTATION_LANDSCAPE,
    /** The display is in landscape mode, with the left side up, relative to portrait mode */
    DISPLAY_ORIENTATION_LANDSCAPE_FLIPPED,
    /** The display is in portrait mode */
    DISPLAY_ORIENTATION_PORTRAIT,
    /** The display is in portrait mode, upside down */
    DISPLAY_ORIENTATION_PORTRAIT_FLIPPED
};

struct DisplayMode
{
    /** Width, in screen coordinates */
    int Width;
    /** Height, in screen coordinates */
    int Height;
    /** Refresh rate */
    int RefreshRate;
};

struct DisplayInfo
{
    /** Internal identifier */
    int Id;
    /** Display name */
    const char* Name;
    /** Display bounds */
    int DisplayX;
    /** Display bounds */
    int DisplayY;
    /** Display bounds */
    int DisplayW;
    /** Display bounds */
    int DisplayH;
    /** Display usable bounds */
    int DisplayUsableX;
    /** Display usable bounds */
    int DisplayUsableY;
    /** Display usable bounds */
    int DisplayUsableW;
    /** Display usable bounds */
    int DisplayUsableH;
    /** Display orientation */
    DISPLAY_ORIENTATION Orientation;
    /** Diagonal DPI */
    float ddpi;
    /** Horizontal DPI */
    float hdpi;
    /** Vertical DPI */
    float vdpi;
};

namespace Core
{

/** Get list of displays */
void GetDisplays(TVector<DisplayInfo>& Displays);

/** Get list of display modes */
void GetDisplayModes(DisplayInfo const& Display, TVector<DisplayMode>& Modes);

/** Get information about the desktop display mode */
void GetDesktopDisplayMode(DisplayInfo const& Display, DisplayMode& Mode);

/** Get information about the current display mode */
void GetCurrentDisplayMode(DisplayInfo const& Display, DisplayMode& Mode);

/** Get the closest match to the requested display mode */
bool GetClosestDisplayMode(DisplayInfo const& Display, int Width, int Height, int RefreshRate, DisplayMode& Mode);

} // namespace Core

HK_NAMESPACE_END
