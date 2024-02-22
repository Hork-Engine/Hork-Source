/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Core/String.h>

#include "ConsoleBuffer.h"

HK_NAMESPACE_BEGIN

class ArgumentPack
{
public:
    ArgumentPack() = default;

    ArgumentPack(int argc, const char* argv[]) :
        Argc(argc), Argv(argv)
    {}

    int          Argc{};
    const char** Argv{};
};

class ApplicationArguments
{
public:
    ApplicationArguments(ArgumentPack const& args);
    ~ApplicationArguments();

    /** Application command line args count */
    int Count() const { return m_NumArguments; }

    /** Get arg at specified position */
    const char* At(int index) const
    {
        return m_Arguments[index];
    }

    bool Has(StringView name) const;
    int Find(StringView name) const;

private:
    int    m_NumArguments{};
    char** m_Arguments{};
    bool   m_bNeedFree{};
};

class CoreApplication
{
public:
    CoreApplication(ArgumentPack const& args);

    virtual ~CoreApplication();

    int ExitCode() const
    {
        return 0;
    }

    static ApplicationArguments const& Args()
    {
        return s_Instance->m_Arguments;
    }

    static StringView GetExecutable()
    {
        return s_Instance->m_Executable;
    }

    static StringView GetWorkingDir()
    {
        return s_Instance->m_WorkingDir;
    }
    
    static StringView GetRootPath()
    {
        return s_Instance->m_RootPath;
    }

    static ConsoleBuffer& GetConsoleBuffer()
    {
        return s_Instance->m_ConsoleBuffer;
    }

    static void WriteMessage(const char* message)
    {
        s_Instance->_WriteMessage(message);
    }

    static void SetClipboard(StringView text);
    static void SetClipboard(String const& text);

    static const char* GetClipboard();

    template <typename... T>
    static HK_FORCEINLINE void TerminateWithError(fmt::format_string<T...> Format, T&&... args)
    {
        fmt::memory_buffer buffer;
        fmt::detail::vformat_to(buffer, fmt::string_view(Format), fmt::make_format_args(args...));
        buffer.push_back('\0');

        s_Instance->_TerminateWithError(buffer.data());
    }

private:
    void _WriteMessage(const char* message);
    void _TerminateWithError(const char* message);

    void Cleanup();

protected:
    static CoreApplication* Instance()
    {
        return s_Instance;
    }

private:
    static CoreApplication* s_Instance;
    ApplicationArguments m_Arguments;
    char*                m_Executable{};
    String               m_WorkingDir;
    const char*          m_RootPath;
#ifdef HK_OS_WIN32
    void*                m_ProcessMutex{};
#endif
    int                  m_ProcessAttribute{};
    FILE*                m_LogFile{};
    Hk::Mutex            m_LogWriterSync;
    char*                m_Clipboard{};
    ConsoleBuffer        m_ConsoleBuffer;
};

HK_NAMESPACE_END
