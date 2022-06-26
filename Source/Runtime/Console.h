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

#include "CommandContext.h"

#include <Platform/ConsoleBuffer.h>

struct SKeyEvent;
struct SCharEvent;
struct SMouseWheelEvent;
class ACanvas;

class AConsole
{
    HK_FORBID_COPY(AConsole)

public:
    AConsole();

    /** Clear console text */
    void Clear();

    /** Is console active */
    bool IsActive() const;

    /** Set console to fullscreen mode */
    void SetFullscreen(bool _Fullscreen);

    /** Process key event */
    void KeyEvent(SKeyEvent const& _Event, ACommandContext& _CommandCtx, ACommandProcessor& _CommandProcessor);

    /** Process char event */
    void CharEvent(SCharEvent const& _Event);

    /** Process mouse wheel event */
    void MouseWheelEvent(SMouseWheelEvent const& _Event);

    /** Draw console to canvas */
    void Draw(ACanvas* _Canvas, float _TimeStep);

    /** Write command line history */
    void WriteStoryLines();

    /** Read command line history */
    void ReadStoryLines();

private:
    void CopyStoryLine(WideChar const* _StoryLine);
    void AddStoryLine(WideChar* _Text, int _Length);
    void InsertUTF8Text(const char* _Utf8);
    void InsertClipboardText();
    void CompleteString(ACommandContext& _CommandCtx, AStringView _Str);
    void DrawCmdLine(ACanvas* _Canvas, int x, int y, int MaxLineChars);

    static const int MAX_CMD_LINE_CHARS = 256;
    static const int MAX_STORY_LINES    = 64;

    AConsoleBuffer* pConBuffer;

    WideChar CmdLine[MAX_CMD_LINE_CHARS];
    WideChar StoryLines[MAX_STORY_LINES][MAX_CMD_LINE_CHARS];
    float     ConHeight     = 0;
    int       CmdLineLength = 0;
    int       CmdLinePos    = 0;
    int       NumStoryLines = 0;
    int       CurStoryLine  = 0;
    bool      bDown         = false;
    bool      bFullscreen   = false;
};
