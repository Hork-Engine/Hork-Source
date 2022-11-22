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

#include "UIBrush.h"

#include "../CommandContext.h"
#include <Platform/ConsoleBuffer.h>

class UIConsole
{
    HK_FORBID_COPY(UIConsole)

public:
    UIConsole();

    /** Clear console text */
    void Clear();

    /** Is console active */
    bool IsActive() const;

    void Up();
    void Down();
    void Toggle();

    /** Set console to fullscreen mode */
    void SetFullscreen(bool bFullscreen);

    void Update(float timeStep);

    /** Draw console to canvas */
    void Draw(ACanvas& cv, UIBrush* background);

    /** Process key event */
    void OnKeyEvent(struct SKeyEvent const& event, ACommandContext& commandCtx, ACommandProcessor& commandProcessor);

    /** Process char event */
    void OnCharEvent(struct SCharEvent const& event);

    /** Process mouse wheel event */
    void OnMouseWheelEvent(struct SMouseWheelEvent const& event);

    /** Write command line history */
    void WriteStoryLines();

    /** Read command line history */
    void ReadStoryLines();

private:
    void CopyStoryLine(WideChar const* storyLine);
    void AddStoryLine(WideChar* text, int numChars);
    void InsertUTF8Text(AStringView utf8);
    void InsertClipboardText();
    void CompleteString(ACommandContext& commandCtx, AStringView _Str);
    void DrawCmdLine(ACanvas& cv, int x, int y, int maxLineChars);

    static const int MAX_CMD_LINE_CHARS = 256;
    static const int MAX_STORY_LINES    = 64;

    AConsoleBuffer* m_pConBuffer;

    WideChar m_CmdLine[MAX_CMD_LINE_CHARS];
    WideChar m_StoryLines[MAX_STORY_LINES][MAX_CMD_LINE_CHARS];
    float    m_ConHeight     = 0;
    int      m_CmdLineLength = 0;
    int      m_CmdLinePos    = 0;
    int      m_NumStoryLines = 0;
    int      m_CurStoryLine  = 0;
    bool     m_bDown         = false;
    bool     m_bFullscreen   = false;
};
