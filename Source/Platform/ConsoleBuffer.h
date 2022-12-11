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

#include <Platform/Utf8.h>
#include <Platform/Thread.h>

class ConsoleBuffer
{
public:
    static constexpr int Padding        = 8;
    static constexpr int CharacterWidth = 14;

    /** Set console width */
    void Resize(int _VidWidth);

    /** Print Utf8 text */
    void Print(const char* _Text);

    /** Print WideChar text */
    void WidePrint(WideChar const* _Text);

    /** Clear text and reset scrolling */
    void Clear();

    /** Scroll to top */
    void ScrollStart();

    /** Scroll to bottom */
    void ScrollEnd();

    /** Scroll by delta */
    void ScrollDelta(int Delta);

    struct LockedData
    {
        WideChar* pImage;
        int       Scroll;
        int       MaxLines;
        int       PrintLine;
        int       MaxLineChars;
    };

    /** Lock console mutex before draw */
    LockedData Lock();

    /** Unlock console mutex */
    void Unlock();

private:
    void _Resize(int _VidWidth);

    static constexpr int CON_IMAGE_SIZE = 1024 * 1024;

    WideChar  ImageData[2][CON_IMAGE_SIZE];
    WideChar* pImage = ImageData[0];
    Mutex     Mutex;
    int       PrintLine    = 0;
    int       CurWidth     = 0;
    int       MaxLines     = 0;
    int       NumLines     = 0;
    int       MaxLineChars = 0;
    int       Scroll       = 0;
    bool      bInitialized = false;
};
