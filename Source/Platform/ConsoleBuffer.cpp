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

#include <Platform/ConsoleBuffer.h>
#include <Platform/Memory/Memory.h>

void AConsoleBuffer::Resize(int _VidWidth)
{
    AMutexGurad syncGuard(Mutex);

    _Resize(_VidWidth);
}

void AConsoleBuffer::_Resize(int _VidWidth)
{
    int prevMaxLines     = MaxLines;
    int prevMaxLineChars = MaxLineChars;

    MaxLineChars = (_VidWidth - Padding * 2) / CharacterWidth;

    if (MaxLineChars == prevMaxLineChars)
    {
        return;
    }

    MaxLines = CON_IMAGE_SIZE / MaxLineChars;

    if (NumLines > MaxLines)
    {
        NumLines = MaxLines;
    }

    SWideChar* pNewImage = (pImage == ImageData[0]) ? ImageData[1] : ImageData[0];

    Platform::ZeroMem(pNewImage, sizeof(*pNewImage) * CON_IMAGE_SIZE);

    const int width  = std::min(prevMaxLineChars, MaxLineChars);
    const int height = std::min(prevMaxLines, MaxLines);

    for (int i = 0; i < height; i++)
    {
        const int newOffset = (MaxLines - i - 1) * MaxLineChars;
        const int oldOffset = ((prevMaxLines + PrintLine - i) % prevMaxLines) * prevMaxLineChars;

        Platform::Memcpy(&pNewImage[newOffset], &pImage[oldOffset], width * sizeof(*pNewImage));
    }

    pImage = pNewImage;

    PrintLine = MaxLines - 1;
    Scroll    = 0;
}

void AConsoleBuffer::Print(const char* _Text)
{
    const char* wordStr;
    int         wordLength;
    SWideChar   ch;
    int         byteLen;

    AMutexGurad syncGuard(Mutex);

    if (!bInitialized)
    {
        _Resize(1024);
        bInitialized = true;
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
                pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                if (CurWidth == MaxLineChars)
                {
                    CurWidth  = 0;
                    PrintLine = (PrintLine + 1) % MaxLines;
                    NumLines++;
                }
                s += byteLen;
                break;
            case '\t':
                if (CurWidth + 4 >= MaxLineChars)
                {
                    CurWidth  = 0;
                    PrintLine = (PrintLine + 1) % MaxLines;
                    NumLines++;
                }
                else
                {
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                }
                s += byteLen;
                break;
            case '\n':
            case '\r':
                pImage[PrintLine * MaxLineChars + CurWidth++] = '\0';
                CurWidth                                      = 0;
                PrintLine                                     = (PrintLine + 1) % MaxLines;
                NumLines++;
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

                if (CurWidth + wordLength > MaxLineChars)
                {
                    CurWidth  = 0;
                    PrintLine = (PrintLine + 1) % MaxLines;
                    NumLines++;
                }

                while (wordLength-- > 0)
                {
                    byteLen = Core::WideCharDecodeUTF8(wordStr, ch);
                    wordStr += byteLen;

                    pImage[PrintLine * MaxLineChars + CurWidth++] = ch;
                    if (CurWidth == MaxLineChars)
                    {
                        CurWidth  = 0;
                        PrintLine = (PrintLine + 1) % MaxLines;
                        NumLines++;
                    }
                }
                break;
        }
    }

    if (NumLines > MaxLines)
    {
        NumLines = MaxLines;
    }
}

void AConsoleBuffer::WidePrint(SWideChar const* _Text)
{
    SWideChar const* wordStr;
    int              wordLength;

    AMutexGurad syncGuard(Mutex);

    if (!bInitialized)
    {
        _Resize(640);
        bInitialized = true;
    }

    while (*_Text)
    {
        switch (*_Text)
        {
            case ' ':
                pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                if (CurWidth == MaxLineChars)
                {
                    CurWidth  = 0;
                    PrintLine = (PrintLine + 1) % MaxLines;
                    NumLines++;
                }
                _Text++;
                break;
            case '\t':
                if (CurWidth + 4 >= MaxLineChars)
                {
                    CurWidth  = 0;
                    PrintLine = (PrintLine + 1) % MaxLines;
                    NumLines++;
                }
                else
                {
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                    pImage[PrintLine * MaxLineChars + CurWidth++] = ' ';
                }
                _Text++;
                break;
            case '\n':
            case '\r':
                pImage[PrintLine * MaxLineChars + CurWidth++] = '\0';
                CurWidth                                      = 0;
                PrintLine                                     = (PrintLine + 1) % MaxLines;
                NumLines++;
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

                if (CurWidth + wordLength > MaxLineChars)
                {
                    CurWidth  = 0;
                    PrintLine = (PrintLine + 1) % MaxLines;
                    NumLines++;
                }

                while (wordLength-- > 0)
                {
                    pImage[PrintLine * MaxLineChars + CurWidth++] = *wordStr++;
                    if (CurWidth == MaxLineChars)
                    {
                        CurWidth  = 0;
                        PrintLine = (PrintLine + 1) % MaxLines;
                        NumLines++;
                    }
                }
                break;
        }
    }

    if (NumLines > MaxLines)
    {
        NumLines = MaxLines;
    }
}

void AConsoleBuffer::Clear()
{
    AMutexGurad syncGuard(Mutex);

    Platform::ZeroMem(pImage, sizeof(*pImage) * CON_IMAGE_SIZE);
    Scroll = 0;
}

void AConsoleBuffer::ScrollStart()
{
    AMutexGurad syncGuard(Mutex);
    Scroll = NumLines - 1;
}

void AConsoleBuffer::ScrollEnd()
{
    AMutexGurad syncGuard(Mutex);
    Scroll = 0;
}

void AConsoleBuffer::ScrollDelta(int Delta)
{
    AMutexGurad syncGuard(Mutex);

    Scroll += Delta;
    if (Scroll < 0)
    {
        Scroll = 0;
    }
}

AConsoleBuffer::SLock AConsoleBuffer::Lock()
{
    Mutex.Lock();

    SLock lock;
    lock.pImage       = pImage;
    lock.Scroll       = Scroll;
    lock.MaxLines     = MaxLines;
    lock.PrintLine    = PrintLine;
    lock.MaxLineChars = MaxLineChars;

    return lock;
}

void AConsoleBuffer::Unlock()
{
    Mutex.Unlock();
}
