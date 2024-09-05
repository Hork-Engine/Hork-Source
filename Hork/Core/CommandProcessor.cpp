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

#include "CommandProcessor.h"
#include "Logger.h"

HK_NAMESPACE_BEGIN

CommandProcessor::CommandProcessor()
{
    ClearBuffer();
}

void CommandProcessor::ClearBuffer()
{
    m_Cmdbuf.Clear();
    m_CmdbufPos = 0;
    m_ArgsCount = 0;
}

void CommandProcessor::Add(StringView text)
{
    m_Cmdbuf += text;
}

void CommandProcessor::Insert(StringView text)
{
    m_Cmdbuf.InsertAt(m_CmdbufPos, text);
}

void CommandProcessor::Execute(ICommandContext& ctx)
{
    if (m_Cmdbuf.IsEmpty())
    {
        return;
    }

    m_ArgsCount = 0;

    HK_ASSERT(m_CmdbufPos == 0);

    while (m_CmdbufPos < m_Cmdbuf.Length())
    {
        // "//" comments
        if (m_Cmdbuf[m_CmdbufPos] == '/' && m_Cmdbuf[m_CmdbufPos + 1] == '/')
        {
            m_CmdbufPos += 2; // skip "//"
            while (m_Cmdbuf[m_CmdbufPos] != '\n' && m_CmdbufPos < m_Cmdbuf.Length())
            {
                m_CmdbufPos++;
            }
            continue;
        }

        // "/* */" comments
        if (m_Cmdbuf[m_CmdbufPos] == '/' && m_Cmdbuf[m_CmdbufPos + 1] == '*')
        {
            m_CmdbufPos += 2; // skip "/*"
            for (;;)
            {
                if (m_CmdbufPos >= m_Cmdbuf.Length())
                {
                    LOG("CommandProcessor::Execute: expected '*/'\n");
                    break;
                }

                if (m_Cmdbuf[m_CmdbufPos] == '*' && m_Cmdbuf[m_CmdbufPos + 1] == '/')
                {
                    m_CmdbufPos += 2; // skip "*/"
                    break;
                }

                m_CmdbufPos++;
            }
            continue;
        }

        if (m_Cmdbuf[m_CmdbufPos] == '\n' || m_Cmdbuf[m_CmdbufPos] == ';')
        {
            m_CmdbufPos++;
            if (m_ArgsCount > 0)
            {
                ctx.ExecuteCommand(*this);
                m_ArgsCount = 0;
            }
            continue;
        }

        // skip whitespaces
        if ((m_Cmdbuf[m_CmdbufPos] < 32 && m_Cmdbuf[m_CmdbufPos] > 0) || m_Cmdbuf[m_CmdbufPos] == ' ' || m_Cmdbuf[m_CmdbufPos] == '\t')
        {
            m_CmdbufPos++;
            continue;
        }

        if (m_ArgsCount < MAX_ARGS)
        {
            bool bQuoted = false;

            if (m_Cmdbuf[m_CmdbufPos] == '\"')
            { /* quoted token */
                bQuoted = true;
                m_CmdbufPos++;
                if (m_Cmdbuf[m_CmdbufPos] == '\"')
                {
                    // empty token
                    bQuoted = false;
                    m_CmdbufPos++;
                    continue;
                }
            }

            if (m_CmdbufPos >= m_Cmdbuf.Length())
            {
                if (bQuoted)
                {
                    LOG("CommandProcessor::Execute: no closed quote\n");
                }
                continue;
            }

            int c = 0;
            for (;;)
            {
                if ((m_Cmdbuf[m_CmdbufPos] < 32 && m_Cmdbuf[m_CmdbufPos] > 0) || m_Cmdbuf[m_CmdbufPos] == '\"' || m_CmdbufPos >= m_Cmdbuf.Length() || c >= (MAX_ARG_LEN - 1))
                {
                    if (bQuoted)
                    {
                        LOG("CommandProcessor::Execute: no closed quote\n");
                    }
                    break;
                }

                if (!bQuoted)
                {
                    if ((m_Cmdbuf[m_CmdbufPos] == '/' && m_Cmdbuf[m_CmdbufPos + 1] == '/') || (m_Cmdbuf[m_CmdbufPos] == '/' && m_Cmdbuf[m_CmdbufPos + 1] == '*') || m_Cmdbuf[m_CmdbufPos] == ' ' || m_Cmdbuf[m_CmdbufPos] == '\t' || m_Cmdbuf[m_CmdbufPos] == ';')
                    {
                        break;
                    }
                }

                m_Args[m_ArgsCount][c++] = m_Cmdbuf[m_CmdbufPos++];

                if (bQuoted && m_Cmdbuf[m_CmdbufPos] == '\"')
                {
                    bQuoted = false;
                    m_CmdbufPos++;
                    break;
                }
            }
            m_Args[m_ArgsCount][c] = 0;
            m_ArgsCount++;
        }
        else
        {
            LOG("CommandProcessor::Execute: MAX_ARGS hit\n");
            m_CmdbufPos++;
        }
    }

    if (m_ArgsCount > 0)
    {
        ctx.ExecuteCommand(*this);
        m_ArgsCount = 0;
    }

    // clear command buffer
    m_CmdbufPos = 0;
    m_Cmdbuf.Clear();
}

bool CommandProcessor::sIsValidCommandName(const char* name)
{
    if (!name || !*name)
    {
        return false;
    }

    while (*name)
    {
        const char c = *name++;

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

HK_NAMESPACE_END
