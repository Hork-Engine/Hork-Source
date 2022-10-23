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


#include "UIConsole.h"
#include "UIWidget.h"

#include <Runtime/FrameLoop.h>

static AConsoleVar ui_consoleDropSpeed("ui_consoleDropSpeed"s, "5"s);
static AConsoleVar ui_consoleHeight("ui_consoleHeight"s, "0.8"s);

UIConsole::UIConsole() :
    m_pConBuffer(&Platform::GetConsoleBuffer())
{}

void UIConsole::Clear()
{
    m_pConBuffer->Clear();
}

bool UIConsole::IsActive() const
{
    return m_bDown || m_bFullscreen;
}

void UIConsole::SetFullscreen(bool bFullscreen)
{
    m_bFullscreen = bFullscreen;
}

void UIConsole::CopyStoryLine(WideChar const* storyLine)
{
    m_CmdLineLength = 0;
    while (*storyLine && m_CmdLineLength < MAX_CMD_LINE_CHARS)
    {
        m_CmdLine[m_CmdLineLength++] = *storyLine++;
    }
    m_CmdLinePos = m_CmdLineLength;
}

void UIConsole::AddStoryLine(WideChar* text, int numChars)
{
    WideChar* storyLine = m_StoryLines[m_NumStoryLines++ & (MAX_STORY_LINES - 1)];
    Platform::Memcpy(storyLine, text, sizeof(text[0]) * Math::Min(numChars, MAX_CMD_LINE_CHARS));
    if (numChars < MAX_CMD_LINE_CHARS)
    {
        storyLine[numChars] = 0;
    }
    m_CurStoryLine = m_NumStoryLines;
}

void UIConsole::InsertUTF8Text(AStringView utf8)
{
    int len = Core::UTF8StrLength(utf8.Begin(), utf8.End());
    if (m_CmdLineLength + len >= MAX_CMD_LINE_CHARS)
    {
        LOG("Text is too long to be copied to command line\n");
        return;
    }

    if (len && m_CmdLinePos != m_CmdLineLength)
    {
        Platform::Memmove(&m_CmdLine[m_CmdLinePos + len], &m_CmdLine[m_CmdLinePos], sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos));
    }

    m_CmdLineLength += len;

    const char* u8 = utf8.Begin();
    WideChar ch;
    int       byteLen;
    while (len-- > 0)
    {
        byteLen = Core::WideCharDecodeUTF8(u8, ch);
        if (!byteLen)
        {
            break;
        }
        u8 += byteLen;
        m_CmdLine[m_CmdLinePos++] = ch;
    }
}

void UIConsole::InsertClipboardText()
{
    InsertUTF8Text(Platform::GetClipboard());
}

void UIConsole::CompleteString(ACommandContext& commandCtx, AStringView _Str)
{
    AString completion;
    int     count = commandCtx.CompleteString(_Str, completion);

    if (completion.IsEmpty())
    {
        return;
    }

    if (count > 1)
    {
        commandCtx.Print(_Str.GetSubstring(0, m_CmdLinePos));
    }
    else
    {
        completion += " ";
    }

    m_CmdLinePos    = 0;
    m_CmdLineLength = 0;
    InsertUTF8Text(completion);
}

void UIConsole::OnKeyEvent(SKeyEvent const& event, ACommandContext& commandCtx, ACommandProcessor& commandProcessor)
{
    if (event.Action == IA_PRESS)
    {
        if (!m_bFullscreen && event.Key == KEY_GRAVE_ACCENT)
        {
            m_bDown = !m_bDown;

            if (!m_bDown)
            {
                m_CmdLineLength = m_CmdLinePos = 0;
                m_CurStoryLine                 = m_NumStoryLines;
            }
        }
    }

    if (IsActive() && (event.Action == IA_PRESS || event.Action == IA_REPEAT))
    {
        int scrollDelta = 1;
        if (event.ModMask & MOD_MASK_CONTROL)
        {
            if (event.Key == KEY_HOME)
            {
                m_pConBuffer->ScrollStart();
            }
            else if (event.Key == KEY_END)
            {
                m_pConBuffer->ScrollEnd();
            }
            scrollDelta = 4;
        }

        switch (event.Key)
        {
            case KEY_PAGE_UP:
                m_pConBuffer->ScrollDelta(scrollDelta);
                break;
            case KEY_PAGE_DOWN:
                m_pConBuffer->ScrollDelta(-scrollDelta);
                break;
                //case KEY_ENTER:
                //    m_pConBuffer->ScrollEnd();
                //    break;
        }

        // Command line keys
        switch (event.Key)
        {
            case KEY_LEFT:
                if (event.ModMask & MOD_MASK_CONTROL)
                {
                    while (m_CmdLinePos > 0 && m_CmdLinePos <= m_CmdLineLength && m_CmdLine[m_CmdLinePos - 1] == ' ')
                    {
                        m_CmdLinePos--;
                    }
                    while (m_CmdLinePos > 0 && m_CmdLinePos <= m_CmdLineLength && m_CmdLine[m_CmdLinePos - 1] != ' ')
                    {
                        m_CmdLinePos--;
                    }
                }
                else
                {
                    m_CmdLinePos--;
                    if (m_CmdLinePos < 0)
                    {
                        m_CmdLinePos = 0;
                    }
                }
                break;
            case KEY_RIGHT:
                if (event.ModMask & MOD_MASK_CONTROL)
                {
                    while (m_CmdLinePos < m_CmdLineLength && m_CmdLine[m_CmdLinePos] != ' ')
                    {
                        m_CmdLinePos++;
                    }
                    while (m_CmdLinePos < m_CmdLineLength && m_CmdLine[m_CmdLinePos] == ' ')
                    {
                        m_CmdLinePos++;
                    }
                }
                else
                {
                    if (m_CmdLinePos < m_CmdLineLength)
                    {
                        m_CmdLinePos++;
                    }
                }
                break;
            case KEY_END:
                m_CmdLinePos = m_CmdLineLength;
                break;
            case KEY_HOME:
                m_CmdLinePos = 0;
                break;
            case KEY_BACKSPACE:
                if (m_CmdLinePos > 0)
                {
                    Platform::Memmove(m_CmdLine + m_CmdLinePos - 1, m_CmdLine + m_CmdLinePos, sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos));
                    m_CmdLineLength--;
                    m_CmdLinePos--;
                }
                break;
            case KEY_DELETE:
                if (m_CmdLinePos < m_CmdLineLength)
                {
                    Platform::Memmove(m_CmdLine + m_CmdLinePos, m_CmdLine + m_CmdLinePos + 1, sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos - 1));
                    m_CmdLineLength--;
                }
                break;
            case KEY_ENTER: {
                char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case WideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

                Core::WideStrEncodeUTF8(result, sizeof(result), m_CmdLine, m_CmdLine + m_CmdLineLength);

                if (m_CmdLineLength > 0)
                {
                    AddStoryLine(m_CmdLine, m_CmdLineLength);
                }

                LOG("{}\n", result);

                commandProcessor.Add(result);
                commandProcessor.Add("\n");

                m_CmdLineLength = 0;
                m_CmdLinePos    = 0;
                break;
            }
            case KEY_DOWN:
                m_CmdLineLength = 0;
                m_CmdLinePos    = 0;

                m_CurStoryLine++;

                if (m_CurStoryLine < m_NumStoryLines)
                {
                    CopyStoryLine(m_StoryLines[m_CurStoryLine & (MAX_STORY_LINES - 1)]);
                }
                else if (m_CurStoryLine > m_NumStoryLines)
                {
                    m_CurStoryLine = m_NumStoryLines;
                }
                break;
            case KEY_UP: {
                m_CmdLineLength = 0;
                m_CmdLinePos    = 0;

                m_CurStoryLine--;

                const int n = m_NumStoryLines - (m_NumStoryLines < MAX_STORY_LINES ? m_NumStoryLines : MAX_STORY_LINES) - 1;

                if (m_CurStoryLine > n)
                {
                    CopyStoryLine(m_StoryLines[m_CurStoryLine & (MAX_STORY_LINES - 1)]);
                }

                if (m_CurStoryLine < n)
                {
                    m_CurStoryLine = n;
                }
                break;
            }
            case KEY_V:
                if (event.ModMask & MOD_MASK_CONTROL)
                {
                    InsertClipboardText();
                }
                break;
            case KEY_TAB: {
                char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case WideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

                Core::WideStrEncodeUTF8(result, sizeof(result), m_CmdLine, m_CmdLine + m_CmdLinePos);

                CompleteString(commandCtx, result);
                break;
            }
            default:
                break;
        }
    }
}

void UIConsole::OnCharEvent(SCharEvent const& event)
{
    if (!IsActive())
    {
        return;
    }

    if (event.UnicodeCharacter == '`')
    {
        return;
    }

    if (m_CmdLineLength < MAX_CMD_LINE_CHARS)
    {
        if (m_CmdLinePos != m_CmdLineLength)
        {
            Platform::Memmove(&m_CmdLine[m_CmdLinePos + 1], &m_CmdLine[m_CmdLinePos], sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos));
        }
        m_CmdLine[m_CmdLinePos] = event.UnicodeCharacter;
        m_CmdLineLength++;
        m_CmdLinePos++;
    }
}

void UIConsole::OnMouseWheelEvent(SMouseWheelEvent const& event)
{
    if (!IsActive())
    {
        return;
    }

    if (event.WheelY < 0.0)
    {
        m_pConBuffer->ScrollDelta(-1);
    }
    else if (event.WheelY > 0.0)
    {
        m_pConBuffer->ScrollDelta(1);
    }
}

void UIConsole::DrawCmdLine(ACanvas& cv, int x, int y, int maxLineChars)
{
    Color4 const& charColor = Color4::White();

    FontStyle fontStyle;
    fontStyle.FontSize = AConsoleBuffer::CharacterWidth;

    int cx = x;

    int offset = m_CmdLinePos + 1 - maxLineChars;
    if (offset < 0)
    {
        offset = 0;
    }
    int numDrawChars = m_CmdLineLength;
    if (numDrawChars > maxLineChars)
    {
        numDrawChars = maxLineChars;
    }
    for (int j = 0; j < numDrawChars; j++)
    {
        int n = j + offset;

        if (n >= m_CmdLineLength)
        {
            break;
        }

        WideChar ch = m_CmdLine[n];

        if (ch <= ' ')
        {
            cx += AConsoleBuffer::CharacterWidth;
            continue;
        }

        cv.DrawWChar(fontStyle, ch, cx, y, charColor);

        cx += AConsoleBuffer::CharacterWidth;
    }

    if ((Platform::SysMicroseconds() >> 18) & 1)
    {
        cx = x + (m_CmdLinePos - offset) * AConsoleBuffer::CharacterWidth;

        cv.DrawWChar(fontStyle, '_', cx, y, charColor);
    }
}

void UIConsole::Update(float timeStep)
{
    if (m_bFullscreen)
    {
        m_ConHeight = 1;
        return;
    }

    float conHeight = Math::Clamp(ui_consoleHeight.GetFloat(), 0.0f, 1.0f);
    float speed     = Math::Max(0.1f, ui_consoleDropSpeed.GetFloat()) * timeStep;

    if (m_bDown)
    {
        if (m_ConHeight < conHeight)
        {
            m_ConHeight += speed;
            if (m_ConHeight > conHeight)
                m_ConHeight = conHeight;
        }
        else if (m_ConHeight > conHeight)
        {
            m_ConHeight -= speed;
            if (m_ConHeight < conHeight)
                m_ConHeight = conHeight;
        }
    }
    else
    {
        m_ConHeight -= speed;
        if (m_ConHeight < 0)
            m_ConHeight = 0;
    }
}

void UIConsole::Draw(ACanvas& cv, UIBrush* background)
{
    if (m_ConHeight <= 0.0f)
        return;

    AFont* font = cv.GetDefaultFont();

    const float fontSize = AConsoleBuffer::CharacterWidth;

    cv.ResetScissor();
    cv.FontFace(font);

    FontStyle fontStyle;
    fontStyle.FontSize = fontSize;

    const int   verticalSpace  = 4;
    const int   verticalStride = fontSize + verticalSpace;
    const int   cmdLineH       = verticalStride;
    const float halfVidHeight  = cv.GetHeight() * m_ConHeight;
    const int   numVisLines    = Math::Ceil((halfVidHeight - cmdLineH) / verticalStride);

    const Color4 c1(0, 0, 0, 1.0f);
    const Color4 c2(0, 0, 0, 0.0f);
    const Color4 charColor(1, 1, 1, 1);

    UIWidgetGeometry geometry;

    geometry.Mins = Float2(0, cv.GetHeight() * (m_ConHeight - 1.0f));
    geometry.Maxs = geometry.Mins + Float2(cv.GetWidth(), cv.GetHeight());

    if (background)
        DrawBrush(cv, geometry.Mins, geometry.Maxs, {}, background);
    else
        cv.DrawRectFilled(geometry.Mins, geometry.Maxs, Color4::Black());

    cv.DrawLine(Float2(0, halfVidHeight), Float2(cv.GetWidth(), halfVidHeight), Color4::White(), 2.0f);

    int x = AConsoleBuffer::Padding;
    int y = halfVidHeight - verticalStride;

    AConsoleBuffer::SLock lock = m_pConBuffer->Lock();

    DrawCmdLine(cv, x, y, lock.MaxLineChars);

    y -= verticalStride;

    for (int i = 0; i < numVisLines; i++)
    {
        int n = i + lock.Scroll;
        if (n >= lock.MaxLines)
        {
            break;
        }

        const int  offset = ((lock.MaxLines + lock.PrintLine - n - 1) % lock.MaxLines) * lock.MaxLineChars;
        WideChar* line   = &lock.pImage[offset];

        for (int j = 0; j < lock.MaxLineChars && *line; j++)
        {
            cv.DrawWChar(fontStyle, *line++, x, y, charColor);

            x += AConsoleBuffer::CharacterWidth;
        }
        x = AConsoleBuffer::Padding;
        y -= verticalStride;
    }

    m_pConBuffer->Unlock();
}

void UIConsole::WriteStoryLines()
{
    if (!m_NumStoryLines)
    {
        return;
    }

    AFile f = AFile::OpenWrite("console_story.txt");
    if (!f)
    {
        LOG("Failed to write console story\n");
        return;
    }

    char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case WideChar transforms to 4 bytes,
                                             // one additional byte is reserved for trailing '\0'

    int numLines = Math::Min(MAX_STORY_LINES, m_NumStoryLines);

    for (int i = 0; i < numLines; i++)
    {
        int n = (m_NumStoryLines - numLines + i) & (MAX_STORY_LINES - 1);

        Core::WideStrEncodeUTF8(result, sizeof(result), m_StoryLines[n], m_StoryLines[n] + MAX_CMD_LINE_CHARS);

        f.FormattedPrint("{}\n", result);
    }
}

void UIConsole::ReadStoryLines()
{
    WideChar wideStr[MAX_CMD_LINE_CHARS];
    int      wideStrLength;
    char     buf[MAX_CMD_LINE_CHARS * 3 + 2]; // In worst case WideChar transforms to 3 bytes,
                                              // two additional bytes are reserved for trailing '\n\0'

    AFile f = AFile::OpenRead("console_story.txt");
    if (!f)
    {
        return;
    }

    m_NumStoryLines = 0;
    while (m_NumStoryLines < MAX_STORY_LINES && f.Gets(buf, sizeof(buf)))
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
