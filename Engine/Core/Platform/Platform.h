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

#include "Format.h"

HK_NAMESPACE_BEGIN

struct CommandLine
{
    HK_FORBID_COPY(CommandLine)

    CommandLine(const char* _CommandLine);
    CommandLine(int _Argc, char** _Argv);
    ~CommandLine();

    /** Application command line args count */
    int GetArgc() const { return m_NumArguments; }

    /** Application command line args */
    const char* const* GetArgv() const { return m_Arguments; }

    /** Check is argument exists in application command line. Return argument index or -1 if argument was not found. */
    int CheckArg(const char* _Arg) const;

    /** Check is argument exists in application command line. Return false if argument was not found. */
    bool HasArg(const char* _Arg) const;

private:
    void Validate();

    int m_NumArguments = 0;
    char** m_Arguments = nullptr;

    bool m_bNeedFree = false;
};

/** CPU features */
struct CPUInfo
{
    bool OS_AVX : 1;
    bool OS_AVX512 : 1;
    bool OS_64bit : 1;

    bool Intel : 1;
    bool AMD : 1;

    // Simd 128 bit
    bool SSE : 1;
    bool SSE2 : 1;
    bool SSE3 : 1;
    bool SSSE3 : 1;
    bool SSE41 : 1;
    bool SSE42 : 1;
    bool SSE4a : 1;
    bool AES : 1;
    bool SHA : 1;

    // Simd 256 bit
    bool AVX : 1;
    bool XOP : 1;
    bool FMA3 : 1;
    bool FMA4 : 1;
    bool AVX2 : 1;

    // Simd 512 bit
    bool AVX512_F : 1;
    bool AVX512_CD : 1;
    bool AVX512_PF : 1;
    bool AVX512_ER : 1;
    bool AVX512_VL : 1;
    bool AVX512_BW : 1;
    bool AVX512_DQ : 1;
    bool AVX512_IFMA : 1;
    bool AVX512_VBMI : 1;

    // Features
    bool x64 : 1;
    bool ABM : 1;
    bool MMX : 1;
    bool RDRAND : 1;
    bool BMI1 : 1;
    bool BMI2 : 1;
    bool ADX : 1;
    bool MPX : 1;
    bool PREFETCHWT1 : 1;
};

struct MemoryInfo
{
    size_t TotalAvailableMegabytes;
    size_t CurrentAvailableMegabytes;
    size_t PageSize;
};

enum PROCESS_ATTRIBUTE
{
    PROCESS_COULDNT_CHECK_UNIQUE = 1,
    PROCESS_ALREADY_EXISTS,
    PROCESS_UNIQUE
};

struct ProcessInfo
{
    int   ProcessAttribute = 0;
    char* Executable       = nullptr;
};

struct PlatformInitialize
{
    char**      Argv                    = nullptr;
    int         Argc                    = 0;
    const char* pCommandLine            = nullptr;
};

class ConsoleBuffer;

namespace Platform
{

/** Initialize core library */
void Initialize(PlatformInitialize const& CoreInitialize);

/** Deinitialize core library */
void Deinitialize();

/** Application command line args count */
int GetArgc();

/** Application command line args */
const char* const* GetArgv();

/** Check is argument exists in application command line. Return argument index or -1 if argument was not found. */
int CheckArg(const char* _Arg);

/** Check is argument exists in application command line. Return false if argument was not found. */
bool HasArg(const char* _Arg);

/** Get command line pointer */
CommandLine const* GetCommandLine();

/** Console buffer */
ConsoleBuffer& GetConsoleBuffer();

/** Get CPU info */
CPUInfo const* GetCPUInfo();

/** Get total/available memory status */
MemoryInfo GetPhysMemoryInfo();

/** Get process info */
ProcessInfo const& GetProcessInfo();

/** Write message to log file */
void WriteLog(const char* Message);

/** Write message to debug console */
void WriteDebugString(const char* Message);

void WriteConsole(const char* Message);

/** Print CPU features using global logger */
void PrintCPUFeatures();

/** Application start timestamp */
int64_t SysStartSeconds();

/** Application start timestamp */
int64_t SysStartMilliseconds();

/** Application start timestamp */
int64_t SysStartMicroseconds();

/** Get current time in seconds since application start */
int64_t SysSeconds();

/** Get current time in seconds since application start */
double SysSeconds_d();

/** Get current time in milliseconds since application start */
int64_t SysMilliseconds();

/** Get current time in milliseconds since application start */
double SysMilliseconds_d();

/** Get current time in microseconds since application start */
int64_t SysMicroseconds();

/** Get current time in microseconds since application start */
double SysMicroseconds_d();

/** Load dynamic library (.dll or .so) */
void* LoadDynamicLib(const char* _LibraryName);

/** Unload dynamic library (.dll or .so) */
void UnloadDynamicLib(void* _Handle);

/** Get address of procedure in dynamic library */
void* GetProcAddress(void* _Handle, const char* _ProcName);

/** Helper. Get address of procedure in dynamic library */
template <typename T>
bool GetProcAddress(void* _Handle, T** _ProcPtr, const char* _ProcName)
{
    return nullptr != ((*_ProcPtr) = (T*)GetProcAddress(_Handle, _ProcName));
}

/** Set clipboard text */
void SetClipboard(const char* _Utf8String);

/** Get clipboard text */
const char* GetClipboard();

void SetCursorEnabled(bool _Enabled);

bool IsCursorEnabled();

void GetCursorPosition(int& _X, int& _Y);

} // namespace Platform

/** Show critical error and exit */
void _CriticalError(const char* Text);

template <typename... T>
HK_FORCEINLINE void CriticalError(fmt::format_string<T...> Format, T&&... args)
{
    fmt::memory_buffer buffer;
    fmt::detail::vformat_to(buffer, fmt::string_view(Format), fmt::make_format_args(args...));
    buffer.push_back('\0');

    _CriticalError(buffer.data());
}

HK_NAMESPACE_END
