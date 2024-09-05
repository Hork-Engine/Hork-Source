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

#pragma once

#include <Hork/Core/String.h>

HK_NAMESPACE_BEGIN

class CommandProcessor;

/// Command execution context
class ICommandContext
{
public:
    virtual ~ICommandContext() {}
    virtual void ExecuteCommand(CommandProcessor const& proc) = 0;
};

/// Command buffer parser
class CommandProcessor final : public Noncopyable
{
public:
    static constexpr int MAX_ARGS    = 256;
    static constexpr int MAX_ARG_LEN = 256;

    CommandProcessor();

    /// Clear command buffer
    void ClearBuffer();

    /// Add text to the end of command buffer
    void Add(StringView text);

    /// Insert text to current command buffer offset
    void Insert(StringView text);

    /// Execute with command context
    void Execute(ICommandContext& ctx);

    /// Get argument by index
    const char* GetArg(int i) const { return m_Args[i]; }

    /// Get arguments count
    int GetArgsCount() const { return m_ArgsCount; }

    /// Helper. Check is command name valid.
    static bool sIsValidCommandName(const char* name);

private:
    String m_Cmdbuf;
    int    m_CmdbufPos;
    char   m_Args[MAX_ARGS][MAX_ARG_LEN];
    int    m_ArgsCount;
};

HK_NAMESPACE_END
