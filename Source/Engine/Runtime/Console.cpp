/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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


#include "Console.h"
#include "Engine.h"
#include "InputDefs.h"
#include "Canvas.h"

#include <Core/Color.h>
#include <Platform/Platform.h>

static const float DropSpeed = 10;

AConsole::AConsole() :
    pConBuffer(&Platform::GetConsoleBuffer())
{}

void AConsole::Clear()
{
    pConBuffer->Clear();
}

bool AConsole::IsActive() const
{
    return bDown || bFullscreen;
}

void AConsole::SetFullscreen(bool _Fullscreen)
{
    bFullscreen = _Fullscreen;
}

void AConsole::CopyStoryLine(SWideChar const* _StoryLine)
{
    CmdLineLength = 0;
    while (*_StoryLine && CmdLineLength < MAX_CMD_LINE_CHARS)
    {
        CmdLine[CmdLineLength++] = *_StoryLine++;
    }
    CmdLinePos = CmdLineLength;
}

void AConsole::AddStoryLine(SWideChar* _Text, int _Length)
{
    SWideChar* storyLine = StoryLines[NumStoryLines++ & (MAX_STORY_LINES - 1)];
    Platform::Memcpy(storyLine, _Text, sizeof(_Text[0]) * Math::Min(_Length, MAX_CMD_LINE_CHARS));
    if (_Length < MAX_CMD_LINE_CHARS)
    {
        storyLine[_Length] = 0;
    }
    CurStoryLine = NumStoryLines;
}

void AConsole::InsertUTF8Text(const char* _Utf8)
{
    int len = Core::UTF8StrLength(_Utf8);
    if (CmdLineLength + len >= MAX_CMD_LINE_CHARS)
    {
        GLogger.Print("Text is too long to be copied to command line\n");
        return;
    }

    if (len && CmdLinePos != CmdLineLength)
    {
        Platform::Memmove(&CmdLine[CmdLinePos + len], &CmdLine[CmdLinePos], sizeof(CmdLine[0]) * (CmdLineLength - CmdLinePos));
    }

    CmdLineLength += len;

    SWideChar ch;
    int       byteLen;
    while (len-- > 0)
    {
        byteLen = Core::WideCharDecodeUTF8(_Utf8, ch);
        if (!byteLen)
        {
            break;
        }
        _Utf8 += byteLen;
        CmdLine[CmdLinePos++] = ch;
    }
}

void AConsole::InsertClipboardText()
{
    InsertUTF8Text(Platform::GetClipboard());
}

void AConsole::CompleteString(ACommandContext& _CommandCtx, const char* _Str)
{
    AString completion;
    int     count = _CommandCtx.CompleteString(_Str, strlen(_Str), completion);

    if (completion.IsEmpty())
    {
        return;
    }

    if (count > 1)
    {
        _CommandCtx.Print(_Str, CmdLinePos);
    }
    else
    {
        completion += " ";
    }

    CmdLinePos    = 0;
    CmdLineLength = 0;
    InsertUTF8Text(completion.CStr());
}

void AConsole::KeyEvent(SKeyEvent const& _Event, ACommandContext& _CommandCtx, ARuntimeCommandProcessor& _CommandProcessor)
{
    if (_Event.Action == IA_PRESS)
    {
        if (!bFullscreen && _Event.Key == KEY_GRAVE_ACCENT)
        {
            bDown = !bDown;

            if (!bDown)
            {
                CmdLineLength = CmdLinePos = 0;
                CurStoryLine               = NumStoryLines;
            }
        }
    }

    if (IsActive() && (_Event.Action == IA_PRESS || _Event.Action == IA_REPEAT))
    {
        int scrollDelta = 1;
        if (_Event.ModMask & KMOD_MASK_CONTROL)
        {
            if (_Event.Key == KEY_HOME)
            {
                pConBuffer->ScrollStart();
            }
            else if (_Event.Key == KEY_END)
            {
                pConBuffer->ScrollEnd();
            }
            scrollDelta = 4;
        }

        switch (_Event.Key)
        {
            case KEY_PAGE_UP:
                pConBuffer->ScrollDelta(scrollDelta);
                break;
            case KEY_PAGE_DOWN:
                pConBuffer->ScrollDelta(-scrollDelta);
                break;
                //case KEY_ENTER:
                //    pConBuffer->ScrollEnd();
                //    break;
        }

        // Command line keys
        switch (_Event.Key)
        {
            case KEY_LEFT:
                if (_Event.ModMask & KMOD_MASK_CONTROL)
                {
                    while (CmdLinePos > 0 && CmdLinePos <= CmdLineLength && CmdLine[CmdLinePos - 1] == ' ')
                    {
                        CmdLinePos--;
                    }
                    while (CmdLinePos > 0 && CmdLinePos <= CmdLineLength && CmdLine[CmdLinePos - 1] != ' ')
                    {
                        CmdLinePos--;
                    }
                }
                else
                {
                    CmdLinePos--;
                    if (CmdLinePos < 0)
                    {
                        CmdLinePos = 0;
                    }
                }
                break;
            case KEY_RIGHT:
                if (_Event.ModMask & KMOD_MASK_CONTROL)
                {
                    while (CmdLinePos < CmdLineLength && CmdLine[CmdLinePos] != ' ')
                    {
                        CmdLinePos++;
                    }
                    while (CmdLinePos < CmdLineLength && CmdLine[CmdLinePos] == ' ')
                    {
                        CmdLinePos++;
                    }
                }
                else
                {
                    if (CmdLinePos < CmdLineLength)
                    {
                        CmdLinePos++;
                    }
                }
                break;
            case KEY_END:
                CmdLinePos = CmdLineLength;
                break;
            case KEY_HOME:
                CmdLinePos = 0;
                break;
            case KEY_BACKSPACE:
                if (CmdLinePos > 0)
                {
                    Platform::Memmove(CmdLine + CmdLinePos - 1, CmdLine + CmdLinePos, sizeof(CmdLine[0]) * (CmdLineLength - CmdLinePos));
                    CmdLineLength--;
                    CmdLinePos--;
                }
                break;
            case KEY_DELETE:
                if (CmdLinePos < CmdLineLength)
                {
                    Platform::Memmove(CmdLine + CmdLinePos, CmdLine + CmdLinePos + 1, sizeof(CmdLine[0]) * (CmdLineLength - CmdLinePos - 1));
                    CmdLineLength--;
                }
                break;
            case KEY_ENTER: {
                char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case SWideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

                Core::WideStrEncodeUTF8(result, sizeof(result), CmdLine, CmdLine + CmdLineLength);

                if (CmdLineLength > 0)
                {
                    AddStoryLine(CmdLine, CmdLineLength);
                }

                GLogger.Printf("%s\n", result);

                _CommandProcessor.Add(result);
                _CommandProcessor.Add("\n");

                CmdLineLength = 0;
                CmdLinePos    = 0;
                break;
            }
            case KEY_DOWN:
                CmdLineLength = 0;
                CmdLinePos    = 0;

                CurStoryLine++;

                if (CurStoryLine < NumStoryLines)
                {
                    CopyStoryLine(StoryLines[CurStoryLine & (MAX_STORY_LINES - 1)]);
                }
                else if (CurStoryLine > NumStoryLines)
                {
                    CurStoryLine = NumStoryLines;
                }
                break;
            case KEY_UP: {
                CmdLineLength = 0;
                CmdLinePos    = 0;

                CurStoryLine--;

                const int n = NumStoryLines - (NumStoryLines < MAX_STORY_LINES ? NumStoryLines : MAX_STORY_LINES) - 1;

                if (CurStoryLine > n)
                {
                    CopyStoryLine(StoryLines[CurStoryLine & (MAX_STORY_LINES - 1)]);
                }

                if (CurStoryLine < n)
                {
                    CurStoryLine = n;
                }
                break;
            }
            case KEY_V:
                if (_Event.ModMask & KMOD_MASK_CONTROL)
                {
                    InsertClipboardText();
                }
                break;
            case KEY_TAB: {
                char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case SWideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

                Core::WideStrEncodeUTF8(result, sizeof(result), CmdLine, CmdLine + CmdLinePos);

                CompleteString(_CommandCtx, result);
                break;
            }
            default:
                break;
        }
    }
}

void AConsole::CharEvent(SCharEvent const& _Event)
{
    if (!IsActive())
    {
        return;
    }

    if (_Event.UnicodeCharacter == '`')
    {
        return;
    }

    if (CmdLineLength < MAX_CMD_LINE_CHARS)
    {
        if (CmdLinePos != CmdLineLength)
        {
            Platform::Memmove(&CmdLine[CmdLinePos + 1], &CmdLine[CmdLinePos], sizeof(CmdLine[0]) * (CmdLineLength - CmdLinePos));
        }
        CmdLine[CmdLinePos] = _Event.UnicodeCharacter;
        CmdLineLength++;
        CmdLinePos++;
    }
}

void AConsole::MouseWheelEvent(SMouseWheelEvent const& _Event)
{
    if (!IsActive())
    {
        return;
    }

    if (_Event.WheelY < 0.0)
    {
        pConBuffer->ScrollDelta(-1);
    }
    else if (_Event.WheelY > 0.0)
    {
        pConBuffer->ScrollDelta(1);
    }
}

void AConsole::DrawCmdLine(ACanvas* _Canvas, int x, int y, int MaxLineChars)
{
    Color4 const& charColor = Color4::White();

    //AFont * font = _Canvas->GetCurrentFont();

    const float scale = 1; //(float)CharacterHeight / font->GetFontSize();

    int cx = x;

    int offset = CmdLinePos + 1 - MaxLineChars;
    if (offset < 0)
    {
        offset = 0;
    }
    int numDrawChars = CmdLineLength;
    if (numDrawChars > MaxLineChars)
    {
        numDrawChars = MaxLineChars;
    }
    for (int j = 0; j < numDrawChars; j++)
    {
        int n = j + offset;

        if (n >= CmdLineLength)
        {
            break;
        }

        SWideChar ch = CmdLine[n];

        if (ch <= ' ')
        {
            cx += AConsoleBuffer::CharacterWidth;
            continue;
        }

        _Canvas->DrawWChar(ch, cx, y, scale, charColor);

        cx += AConsoleBuffer::CharacterWidth;
    }

    if ((Platform::SysMicroseconds() >> 18) & 1)
    {
        cx = x + (CmdLinePos - offset) * AConsoleBuffer::CharacterWidth;

        _Canvas->DrawWChar('_', cx, y, scale, charColor);
    }
}

void AConsole::Draw(ACanvas* _Canvas, float _TimeStep)
{
    if (!bFullscreen)
    {
        if (bDown)
        {
            ConHeight += DropSpeed * _TimeStep;
        }
        else
        {
            ConHeight -= DropSpeed * _TimeStep;
        }

        if (ConHeight <= 0.0f)
        {
            ConHeight = 0.0f;
            return;
        }

        if (ConHeight > 1.0f)
        {
            ConHeight = 1.0f;
        }
    }
    else
    {
        ConHeight = 2;
    }

    AFont const* font = _Canvas->GetCurrentFont();

    const int fontVStride = font->GetFontSize() + 4;
    //const int fontVStride = CharacterHeight + 4;
    const int   cmdLineH      = fontVStride;
    const float halfVidHeight = (_Canvas->GetHeight() >> 1) * ConHeight;
    const int   numVisLines   = Math::Ceil((halfVidHeight - cmdLineH) / fontVStride);

    const Color4 c1(0, 0, 0, 1.0f);
    const Color4 c2(0, 0, 0, 0.0f);
    const Color4 charColor(1, 1, 1, 1);

    if (bFullscreen)
    {
        _Canvas->DrawRectFilled(Float2(0, 0), Float2(_Canvas->GetWidth(), _Canvas->GetHeight()), Color4::Black());
    }
    else
    {
        _Canvas->DrawRectFilledMultiColor(Float2(0, 0), Float2(_Canvas->GetWidth(), halfVidHeight), c1, c2, c2, c1);
    }
    _Canvas->DrawLine(Float2(0, halfVidHeight), Float2(_Canvas->GetWidth(), halfVidHeight), Color4::White(), 2.0f);

    int x = AConsoleBuffer::Padding;
    int y = halfVidHeight - fontVStride;

    const float scale = 1; //(float)CharacterHeight / font->GetFontSize();

    AConsoleBuffer::SLock lock = pConBuffer->Lock();

    DrawCmdLine(_Canvas, x, y, lock.MaxLineChars);

    y -= cmdLineH;

    for (int i = 0; i < numVisLines; i++)
    {
        int n = i + lock.Scroll;
        if (n >= lock.MaxLines)
        {
            break;
        }

        const int  offset = ((lock.MaxLines + lock.PrintLine - n - 1) % lock.MaxLines) * lock.MaxLineChars;
        SWideChar* line   = &lock.pImage[offset];

        for (int j = 0; j < lock.MaxLineChars && *line; j++)
        {
            _Canvas->DrawWChar(*line++, x, y, scale, charColor);

            x += AConsoleBuffer::CharacterWidth;
        }
        x = AConsoleBuffer::Padding;
        y -= fontVStride;
    }

    pConBuffer->Unlock();
}

void AConsole::WriteStoryLines()
{
    if (!NumStoryLines)
    {
        return;
    }

    AFileStream f;
    if (!f.OpenWrite("console_story.txt"))
    {
        GLogger.Printf("Failed to write console story\n");
        return;
    }

    char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case SWideChar transforms to 4 bytes,
                                             // one additional byte is reserved for trailing '\0'

    int numLines = Math::Min(MAX_STORY_LINES, NumStoryLines);

    for (int i = 0; i < numLines; i++)
    {
        int n = (NumStoryLines - numLines + i) & (MAX_STORY_LINES - 1);

        Core::WideStrEncodeUTF8(result, sizeof(result), StoryLines[n], StoryLines[n] + MAX_CMD_LINE_CHARS);

        f.Printf("%s\n", result);
    }
}

void AConsole::ReadStoryLines()
{
    SWideChar wideStr[MAX_CMD_LINE_CHARS];
    int       wideStrLength;
    char      buf[MAX_CMD_LINE_CHARS * 3 + 2]; // In worst case SWideChar transforms to 3 bytes,
                                               // two additional bytes are reserved for trailing '\n\0'

    AFileStream f;
    if (!f.OpenRead("console_story.txt"))
    {
        return;
    }

    NumStoryLines = 0;
    while (NumStoryLines < MAX_STORY_LINES && f.Gets(buf, sizeof(buf)))
    {
        wideStrLength = 0;

        const char* s = buf;
        while (*s && *s != '\n' && wideStrLength < MAX_CMD_LINE_CHARS)
        {
            int byteLen = Core::WideCharDecodeUTF8(s, wideStr[wideStrLength]);
            if (!byteLen)
            {
                break;
            }
            s += byteLen;
            wideStrLength++;
        }

        if (wideStrLength > 0)
        {
            AddStoryLine(wideStr, wideStrLength);
        }
    }
}
