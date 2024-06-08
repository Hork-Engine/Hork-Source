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

#include "ConsoleBuffer.h"
#include "Memory.h"

HK_NAMESPACE_BEGIN

void ConsoleBuffer::Resize(int _VidWidth)
{
    MutexGuard syncGuard(m_Mutex);

    _Resize(_VidWidth);
}

void ConsoleBuffer::_Resize(int _VidWidth)
{
    int prevMaxLines = m_MaxLines;
    int prevMaxLineChars = m_MaxLineChars;

    m_MaxLineChars = (_VidWidth - Padding * 2) / CharacterWidth;

    if (m_MaxLineChars == prevMaxLineChars)
    {
        return;
    }

    m_MaxLines = CON_IMAGE_SIZE / m_MaxLineChars;

    if (m_NumLines > m_MaxLines)
    {
        m_NumLines = m_MaxLines;
    }

    WideChar* pNewImage = (m_pImage == m_ImageData[0]) ? m_ImageData[1] : m_ImageData[0];

    Core::ZeroMem(pNewImage, sizeof(*pNewImage) * CON_IMAGE_SIZE);

    const int width = std::min(prevMaxLineChars, m_MaxLineChars);
    const int height = std::min(prevMaxLines, m_MaxLines);

    for (int i = 0; i < height; i++)
    {
        const int newOffset = (m_MaxLines - i - 1) * m_MaxLineChars;
        const int oldOffset = ((prevMaxLines + m_PrintLine - i) % prevMaxLines) * prevMaxLineChars;

        Core::Memcpy(&pNewImage[newOffset], &m_pImage[oldOffset], width * sizeof(*pNewImage));
    }

    m_pImage = pNewImage;

    m_PrintLine = m_MaxLines - 1;
    m_Scroll = 0;
}

void ConsoleBuffer::Print(const char* _Text)
{
    const char* wordStr;
    int         wordLength;
    WideChar   ch;
    int         byteLen;

    MutexGuard syncGuard(m_Mutex);

    if (!m_bInitialized)
    {
        _Resize(1024);
        m_bInitialized = true;
    }

    const char* s = _Text;

    while (*s)
    {
        byteLen = Core::WideCharDecodeUTF8(s, ch);
        if (!byteLen)
        {
            break;
        }

        switch (ch)
        {
            case ' ':
                m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                if (m_CurWidth == m_MaxLineChars)
                {
                    m_CurWidth = 0;
                    m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                    m_NumLines++;
                }
                s += byteLen;
                break;
            case '\t':
                if (m_CurWidth + 4 >= m_MaxLineChars)
                {
                    m_CurWidth = 0;
                    m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                    m_NumLines++;
                }
                else
                {
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                }
                s += byteLen;
                break;
            case '\n':
            case '\r':
                m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = '\0';
                m_CurWidth = 0;
                m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                m_NumLines++;
                s += byteLen;
                break;
            default:
                wordStr    = s;
                wordLength = 0;

                if (ch > ' ')
                {
                    do {
                        s += byteLen;
                        wordLength++;
                        byteLen = Core::WideCharDecodeUTF8(s, ch);
                    } while (byteLen > 0 && ch > ' ');
                }
                else
                {
                    s += byteLen;
                }

                if (m_CurWidth + wordLength > m_MaxLineChars)
                {
                    m_CurWidth = 0;
                    m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                    m_NumLines++;
                }

                while (wordLength-- > 0)
                {
                    byteLen = Core::WideCharDecodeUTF8(wordStr, ch);
                    wordStr += byteLen;

                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ch;
                    if (m_CurWidth == m_MaxLineChars)
                    {
                        m_CurWidth = 0;
                        m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                        m_NumLines++;
                    }
                }
                break;
        }
    }

    if (m_NumLines > m_MaxLines)
    {
        m_NumLines = m_MaxLines;
    }
}

void ConsoleBuffer::WidePrint(WideChar const* _Text)
{
    WideChar const* wordStr;
    int             wordLength;

    MutexGuard syncGuard(m_Mutex);

    if (!m_bInitialized)
    {
        _Resize(1024);
        m_bInitialized = true;
    }

    while (*_Text)
    {
        switch (*_Text)
        {
            case ' ':
                m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                if (m_CurWidth == m_MaxLineChars)
                {
                    m_CurWidth = 0;
                    m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                    m_NumLines++;
                }
                _Text++;
                break;
            case '\t':
                if (m_CurWidth + 4 >= m_MaxLineChars)
                {
                    m_CurWidth = 0;
                    m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                    m_NumLines++;
                }
                else
                {
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = ' ';
                }
                _Text++;
                break;
            case '\n':
            case '\r':
                m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = '\0';
                m_CurWidth = 0;
                m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                m_NumLines++;
                _Text++;
                break;
            default:
                wordStr    = _Text;
                wordLength = 0;

                if (*_Text > ' ')
                {
                    do {
                        _Text++;
                        wordLength++;
                    } while (*_Text > ' ');
                }
                else
                {
                    _Text++;
                }

                if (m_CurWidth + wordLength > m_MaxLineChars)
                {
                    m_CurWidth = 0;
                    m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                    m_NumLines++;
                }

                while (wordLength-- > 0)
                {
                    m_pImage[m_PrintLine * m_MaxLineChars + m_CurWidth++] = *wordStr++;
                    if (m_CurWidth == m_MaxLineChars)
                    {
                        m_CurWidth = 0;
                        m_PrintLine = (m_PrintLine + 1) % m_MaxLines;
                        m_NumLines++;
                    }
                }
                break;
        }
    }

    if (m_NumLines > m_MaxLines)
    {
        m_NumLines = m_MaxLines;
    }
}

void ConsoleBuffer::Clear()
{
    MutexGuard syncGuard(m_Mutex);

    Core::ZeroMem(m_pImage, sizeof(*m_pImage) * CON_IMAGE_SIZE);
    m_Scroll = 0;
}

void ConsoleBuffer::ScrollStart()
{
    MutexGuard syncGuard(m_Mutex);
    m_Scroll = m_NumLines - 1;
}

void ConsoleBuffer::ScrollEnd()
{
    MutexGuard syncGuard(m_Mutex);
    m_Scroll = 0;
}

void ConsoleBuffer::ScrollDelta(int Delta)
{
    MutexGuard syncGuard(m_Mutex);

    m_Scroll += Delta;
    if (m_Scroll < 0)
    {
        m_Scroll = 0;
    }
}

ConsoleBuffer::LockedData ConsoleBuffer::Lock()
{
    m_Mutex.Lock();

    LockedData lock;
    lock.pImage       = m_pImage;
    lock.Scroll       = m_Scroll;
    lock.MaxLines     = m_MaxLines;
    lock.PrintLine    = m_PrintLine;
    lock.MaxLineChars = m_MaxLineChars;

    return lock;
}

void ConsoleBuffer::Unlock()
{
    m_Mutex.Unlock();
}

HK_NAMESPACE_END
