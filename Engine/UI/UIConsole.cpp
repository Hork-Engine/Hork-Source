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


#include "UIConsole.h"
#include "UIWidget.h"
#include "UIManager.h"

#include <Engine/GameApplication/FrameLoop.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/Core/Platform.h>

HK_NAMESPACE_BEGIN

static ConsoleVar ui_consoleDropSpeed("ui_consoleDropSpeed"_s, "5"_s);
static ConsoleVar ui_consoleHeight("ui_consoleHeight"_s, "0.8"_s);

UIConsole::UIConsole() :
    m_ConsoleBuffer(CoreApplication::sGetConsoleBuffer())
{}

void UIConsole::Clear()
{
    m_ConsoleBuffer.Clear();
}

bool UIConsole::IsActive() const
{
    return m_bDown || m_bFullscreen;
}

void UIConsole::Up()
{
    if (m_bFullscreen)
        return;

    m_bDown = false;

    m_CmdLineLength = m_CmdLinePos = 0;
    m_CurStoryLine                 = m_NumStoryLines;
}

void UIConsole::Down()
{
    m_bDown = true;
}

void UIConsole::Toggle()
{
    if (m_bDown)
        Up();
    else
        Down();
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
    Core::Memcpy(storyLine, text, sizeof(text[0]) * Math::Min(numChars, MAX_CMD_LINE_CHARS));
    if (numChars < MAX_CMD_LINE_CHARS)
    {
        storyLine[numChars] = 0;
    }
    m_CurStoryLine = m_NumStoryLines;
}

void UIConsole::InsertUTF8Text(StringView utf8)
{
    int len = Core::UTF8StrLength(utf8.Begin(), utf8.End());
    if (m_CmdLineLength + len >= MAX_CMD_LINE_CHARS)
    {
        LOG("Text is too long to be copied to command line\n");
        return;
    }

    if (len && m_CmdLinePos != m_CmdLineLength)
    {
        Core::Memmove(&m_CmdLine[m_CmdLinePos + len], &m_CmdLine[m_CmdLinePos], sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos));
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
    InsertUTF8Text(CoreApplication::sGetClipboard());
}

void UIConsole::CompleteString(CommandContext& commandCtx, StringView _Str)
{
    String completion;
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

void UIConsole::OnKeyEvent(KeyEvent const& event, CommandContext& commandCtx, CommandProcessor& commandProcessor)
{
    if (event.Action == InputAction::Pressed || event.Action == InputAction::Repeat)
    {
        int scrollDelta = 1;
        if (event.ModMask.Control)
        {
            if (event.Key == VirtualKey::Home)
            {
                m_ConsoleBuffer.ScrollStart();
            }
            else if (event.Key == VirtualKey::End)
            {
                m_ConsoleBuffer.ScrollEnd();
            }
            scrollDelta = 4;
        }

        switch (event.Key)
        {
            case VirtualKey::PageUp:
                m_ConsoleBuffer.ScrollDelta(scrollDelta);
                break;
            case VirtualKey::PageDown:
                m_ConsoleBuffer.ScrollDelta(-scrollDelta);
                break;
                //case VirtualKey::ENTER:
                //    m_ConsoleBuffer.ScrollEnd();
                //    break;
        }

        // Command line keys
        switch (event.Key)
        {
            case VirtualKey::Left:
                if (event.ModMask.Control)
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
            case VirtualKey::Right:
                if (event.ModMask.Control)
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
            case VirtualKey::End:
                m_CmdLinePos = m_CmdLineLength;
                break;
            case VirtualKey::Home:
                m_CmdLinePos = 0;
                break;
            case VirtualKey::Backspace:
                if (m_CmdLinePos > 0)
                {
                    Core::Memmove(m_CmdLine + m_CmdLinePos - 1, m_CmdLine + m_CmdLinePos, sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos));
                    m_CmdLineLength--;
                    m_CmdLinePos--;
                }
                break;
            case VirtualKey::Delete:
                if (m_CmdLinePos < m_CmdLineLength)
                {
                    Core::Memmove(m_CmdLine + m_CmdLinePos, m_CmdLine + m_CmdLinePos + 1, sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos - 1));
                    m_CmdLineLength--;
                }
                break;
            case VirtualKey::Enter: {
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
            case VirtualKey::Down:
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
            case VirtualKey::Up: {
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
            case VirtualKey::V:
                if (event.ModMask.Control)
                {
                    InsertClipboardText();
                }
                break;
            case VirtualKey::Tab: {
                char result[MAX_CMD_LINE_CHARS * 4 + 1]; // In worst case WideChar transforms to 4 bytes,
                                                         // one additional byte is reserved for trailing '\0'

                Core::WideStrEncodeUTF8(result, sizeof(result), m_CmdLine, m_CmdLine + m_CmdLinePos);

                CompleteString(commandCtx, result);
                break;
            }
            case VirtualKey::Insert:
                if (event.ModMask.IsEmpty())
                {
                    GUIManager->SetInsertMode(!GUIManager->IsInsertMode());
                }
                break;
            default:
                break;
        }
    }
}

void UIConsole::OnCharEvent(CharEvent const& event)
{
    if (event.UnicodeCharacter == '`')
    {
        return;
    }

    if (!GUIManager->IsInsertMode() || m_CmdLinePos == m_CmdLineLength)
    {
        if (m_CmdLineLength < MAX_CMD_LINE_CHARS)
        {
            if (m_CmdLinePos != m_CmdLineLength)
            {
                Core::Memmove(&m_CmdLine[m_CmdLinePos + 1], &m_CmdLine[m_CmdLinePos], sizeof(m_CmdLine[0]) * (m_CmdLineLength - m_CmdLinePos));
            }
            m_CmdLine[m_CmdLinePos] = event.UnicodeCharacter;
            m_CmdLinePos++;
            m_CmdLineLength++;
        }
    }
    else
    {
        if (m_CmdLinePos < MAX_CMD_LINE_CHARS)
        {
            m_CmdLine[m_CmdLinePos] = event.UnicodeCharacter;
            m_CmdLinePos++;
        }
    }
}

void UIConsole::OnMouseWheelEvent(MouseWheelEvent const& event)
{
    if (event.WheelY < 0.0)
    {
        m_ConsoleBuffer.ScrollDelta(-1);
    }
    else if (event.WheelY > 0.0)
    {
        m_ConsoleBuffer.ScrollDelta(1);
    }
}

void UIConsole::DrawCmdLine(Canvas& cv, int x, int y, int maxLineChars)
{
    FontStyle fontStyle;
    fontStyle.FontSize = ConsoleBuffer::CharacterWidth;

    int offset = m_CmdLinePos + 1 - maxLineChars;
    if (offset < 0)
        offset = 0;

    int numDrawChars = m_CmdLineLength;
    if (numDrawChars > maxLineChars)
        numDrawChars = maxLineChars;

    cv.Text(fontStyle, x, y, TEXT_ALIGNMENT_LEFT, WideStringView(&m_CmdLine[offset], &m_CmdLine[Math::Min(m_CmdLineLength, offset + numDrawChars)]));

    if ((Core::SysMicroseconds() >> 18) & 1)
    {
        FontResource* font = GameApplication::sGetDefaultFont();
        for (int i = 0; i < m_CmdLinePos - offset; i++)
            x += font->GetCharAdvance(fontStyle, m_CmdLine[i]);

        if (GUIManager->IsInsertMode())
        {
            cv.DrawRectFilled(Float2(x, y), Float2(x + ConsoleBuffer::CharacterWidth * 0.7f, y + ConsoleBuffer::CharacterWidth), Color4::sWhite());
        }
        else
        {
            WideChar buf[2] = {'_', 0};

            cv.Text(fontStyle, x, y, TEXT_ALIGNMENT_LEFT | TEXT_ALIGNMENT_TOP, WideStringView(buf, &buf[1]));
        }
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

void UIConsole::Draw(Canvas& cv, UIBrush* background, float width, float height)
{
    if (m_ConHeight <= 0.0f)
        return;

    const float fontSize = ConsoleBuffer::CharacterWidth;

    cv.ResetScissor();
    cv.FontFace(GameApplication::sGetDefaultFontHandle());

    FontStyle fontStyle;
    fontStyle.FontSize = fontSize;

    const int   verticalSpace  = 4;
    const int   verticalStride = fontSize + verticalSpace;
    const int   cmdLineH       = verticalStride;
    const float halfVidHeight  = height * m_ConHeight;
    const int   numVisLines    = Math::Ceil((halfVidHeight - cmdLineH) / verticalStride);

    UIWidgetGeometry geometry;

    geometry.Mins = Float2(0, height * (m_ConHeight - 1.0f));
    geometry.Maxs = geometry.Mins + Float2(width, height);

    if (background)
        DrawBrush(cv, geometry.Mins, geometry.Maxs, {}, background);
    else
        cv.DrawRectFilled(geometry.Mins, geometry.Maxs, Color4::sBlack());

    cv.DrawLine(Float2(0, halfVidHeight), Float2(width, halfVidHeight), Color4::sWhite(), 2.0f);

    int x = ConsoleBuffer::Padding;
    int y = halfVidHeight - verticalStride;

    cv.FillColor(Color4::sWhite());

    ConsoleBuffer::LockedData lock = m_ConsoleBuffer.Lock();

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
        int len = StringLength(line);

        len = Math::Min(len, lock.MaxLineChars);

        cv.Text(fontStyle, x, y, TEXT_ALIGNMENT_LEFT, WideStringView(line, len));

        y -= verticalStride;
    }

    m_ConsoleBuffer.Unlock();
}

void UIConsole::WriteStoryLines()
{
    if (!m_NumStoryLines)
    {
        return;
    }

    File f = File::sOpenWrite("console_story.txt");
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

    File f = File::sOpenRead("console_story.txt");
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

HK_NAMESPACE_END
