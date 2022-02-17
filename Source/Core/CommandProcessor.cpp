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

#include "CommandProcessor.h"
#include <Platform/Logger.h>

ACommandProcessor::ACommandProcessor()
{
    ClearBuffer();
}

void ACommandProcessor::ClearBuffer()
{
    Cmdbuf.Clear();
    CmdbufPos = 0;
    ArgsCount = 0;
}

void ACommandProcessor::Add(const char* _Text)
{
    Cmdbuf += _Text;
}

void ACommandProcessor::Insert(const char* _Text)
{
    Cmdbuf.Insert(_Text, CmdbufPos);
}

void ACommandProcessor::Execute(ICommandContext& _Ctx)
{
    if (Cmdbuf.IsEmpty())
    {
        return;
    }

    ArgsCount = 0;

    HK_ASSERT(CmdbufPos == 0);

    while (CmdbufPos < Cmdbuf.Length())
    {
        // "//" comments
        if (Cmdbuf[CmdbufPos] == '/' && Cmdbuf[CmdbufPos + 1] == '/')
        {
            CmdbufPos += 2; // skip "//"
            while (Cmdbuf[CmdbufPos] != '\n' && CmdbufPos < Cmdbuf.Length())
            {
                CmdbufPos++;
            }
            continue;
        }

        // "/* */" comments
        if (Cmdbuf[CmdbufPos] == '/' && Cmdbuf[CmdbufPos + 1] == '*')
        {
            CmdbufPos += 2; // skip "/*"
            for (;;)
            {
                if (CmdbufPos >= Cmdbuf.Length())
                {
                    LOG("ACommandProcessor::Execute: expected '*/'\n");
                    break;
                }

                if (Cmdbuf[CmdbufPos] == '*' && Cmdbuf[CmdbufPos + 1] == '/')
                {
                    CmdbufPos += 2; // skip "*/"
                    break;
                }

                CmdbufPos++;
            }
            continue;
        }

        if (Cmdbuf[CmdbufPos] == '\n' || Cmdbuf[CmdbufPos] == ';')
        {
            CmdbufPos++;
            if (ArgsCount > 0)
            {
                _Ctx.ExecuteCommand(*this);
                ArgsCount = 0;
            }
            continue;
        }

        // skip whitespaces
        if ((Cmdbuf[CmdbufPos] < 32 && Cmdbuf[CmdbufPos] > 0) || Cmdbuf[CmdbufPos] == ' ' || Cmdbuf[CmdbufPos] == '\t')
        {
            CmdbufPos++;
            continue;
        }

        if (ArgsCount < MAX_ARGS)
        {
            bool bQuoted = false;

            if (Cmdbuf[CmdbufPos] == '\"')
            { /* quoted token */
                bQuoted = true;
                CmdbufPos++;
                if (Cmdbuf[CmdbufPos] == '\"')
                {
                    // empty token
                    bQuoted = false;
                    CmdbufPos++;
                    continue;
                }
            }

            if (CmdbufPos >= Cmdbuf.Length())
            {
                if (bQuoted)
                {
                    LOG("ACommandProcessor::Execute: no closed quote\n");
                }
                continue;
            }

            int c = 0;
            for (;;)
            {
                if ((Cmdbuf[CmdbufPos] < 32 && Cmdbuf[CmdbufPos] > 0) || Cmdbuf[CmdbufPos] == '\"' || CmdbufPos >= Cmdbuf.Length() || c >= (MAX_ARG_LEN - 1))
                {
                    if (bQuoted)
                    {
                        LOG("ACommandProcessor::Execute: no closed quote\n");
                    }
                    break;
                }

                if (!bQuoted)
                {
                    if ((Cmdbuf[CmdbufPos] == '/' && Cmdbuf[CmdbufPos + 1] == '/') || (Cmdbuf[CmdbufPos] == '/' && Cmdbuf[CmdbufPos + 1] == '*') || Cmdbuf[CmdbufPos] == ' ' || Cmdbuf[CmdbufPos] == '\t' || Cmdbuf[CmdbufPos] == ';')
                    {
                        break;
                    }
                }

                Args[ArgsCount][c++] = Cmdbuf[CmdbufPos++];

                if (bQuoted && Cmdbuf[CmdbufPos] == '\"')
                {
                    bQuoted = false;
                    CmdbufPos++;
                    break;
                }
            }
            Args[ArgsCount][c] = 0;
            ArgsCount++;
        }
        else
        {
            LOG("ACommandProcessor::Execute: MAX_ARGS hit\n");
            CmdbufPos++;
        }
    }

    if (ArgsCount > 0)
    {
        _Ctx.ExecuteCommand(*this);
        ArgsCount = 0;
    }

    // clear command buffer
    CmdbufPos = 0;
    Cmdbuf.Clear();
}

bool ACommandProcessor::IsValidCommandName(const char* _Name)
{
    if (!_Name || !*_Name)
    {
        return false;
    }

    while (*_Name)
    {
        const char c = *_Name++;

        const bool bLiteral = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        const bool bNumeric = (c >= '0' && c <= '9');
        const bool bMisc    = (c == '_');

        if (!bLiteral && !bNumeric && !bMisc)
        {
            return false;
        }
    }
    return true;
}
