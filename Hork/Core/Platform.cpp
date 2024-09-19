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

#include "Platform.h"
#include "Memory.h"
#include "WindowsDefs.h"

#include <SDL3/SDL.h>

#ifdef HK_OS_LINUX
#    include <unistd.h>
#    include <sys/file.h>
#    include <signal.h>
#    include <dlfcn.h>
#endif

#ifdef HK_OS_ANDROID
#    include <android/log.h>
#endif

#include <chrono>

static int64_t StartMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
static int64_t StartMilliseconds = StartMicroseconds * 0.001;
static int64_t StartSeconds = StartMicroseconds * 0.000001;

static bool EnableConsoleOutput = false;

HK_NAMESPACE_BEGIN

namespace Core
{

int64_t SysStartSeconds()
{
    return StartSeconds;
}

int64_t SysStartMilliseconds()
{
    return StartMilliseconds;
}

int64_t SysStartMicroseconds()
{
    return StartMicroseconds;
}

int64_t SysSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - StartSeconds;
}

double SysSeconds_d()
{
    return SysMicroseconds() * 0.000001;
}

int64_t SysMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - StartMilliseconds;
}

double SysMilliseconds_d()
{
    return SysMicroseconds() * 0.001;
}

int64_t SysMicroseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - StartMicroseconds;
}

double SysMicroseconds_d()
{
    return static_cast<double>(SysMicroseconds());
}

void WriteDebugString(const char* message)
{
#if defined HK_DEBUG
#   if defined HK_COMPILER_MSVC
    {
        int n = MultiByteToWideChar(CP_UTF8, 0, message, -1, NULL, 0);
        if (0 != n)
        {
            // Try to alloc on stack
            if (n < 4096)
            {
                wchar_t* chars = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

                MultiByteToWideChar(CP_UTF8, 0, message, -1, chars, n);

                OutputDebugString(chars);
            }
            else
            {
                wchar_t* chars = (wchar_t*)malloc(n * sizeof(wchar_t));

                MultiByteToWideChar(CP_UTF8, 0, message, -1, chars, n);

                OutputDebugString(chars);

                free(chars);
            }
        }
    }
#   endif
#endif
}

void WriteConsoleString(const char* message)
{
    if (EnableConsoleOutput)
    {
#if defined HK_OS_WIN32
        static HWND hwndConsole = GetConsoleWindow();
        if (hwndConsole)
        {
            fprintf(stdout, "%s", message);
            fflush(stdout);
        }
#elif defined HK_OS_ANDROID
        __android_log_print(ANDROID_LOG_INFO, "Hork Engine", message);
#else
        fprintf(stdout, "%s", message);
        fflush(stdout);
#endif
    }
}

void SetEnableConsoleOutput(bool enable)
{
    EnableConsoleOutput = enable;
}

void* LoadDynamicLib(const char* libraryName)
{
    return SDL_LoadObject(libraryName);
}

void UnloadDynamicLib(void* handle)
{
    SDL_UnloadObject(handle);
}

void* GetProcAddress(void* handle, const char* procName)
{
    return handle ? (void*)SDL_LoadFunction(handle, procName) : nullptr;
}

MemoryInfo GetPhysMemoryInfo()
{
    MemoryInfo info = {};

#if defined HK_OS_WIN32
    MEMORYSTATUSEX memstat = {};
    memstat.dwLength       = sizeof(memstat);
    if (GlobalMemoryStatusEx(&memstat))
    {
        info.TotalAvailableMegabytes   = memstat.ullTotalPhys >> 20;
        info.CurrentAvailableMegabytes = memstat.ullAvailPhys >> 20;
    }
#elif defined HK_OS_LINUX
    long long TotalPages = sysconf(_SC_PHYS_PAGES);
    long long AvailPages = sysconf(_SC_AVPHYS_PAGES);
    long long PageSize = sysconf(_SC_PAGE_SIZE);
    info.TotalAvailableMegabytes = (TotalPages * PageSize) >> 20;
    info.CurrentAvailableMegabytes = (AvailPages * PageSize) >> 20;
#else
#    error "GetPhysMemoryInfo not implemented under current platform"
#endif

#ifdef HK_OS_WIN32
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    info.PageSize = systemInfo.dwPageSize;
#elif defined HK_OS_LINUX
    info.PageSize = sysconf(_SC_PAGE_SIZE);
#else
#    error "GetPageSize not implemented under current platform"
#endif

    return info;
}

void GetCursorPosition(float& x, float& y)
{
    (void)SDL_GetMouseState(&x, &y);
}

void GetGlobalCursorPosition(float& x, float& y)
{
    (void)SDL_GetGlobalMouseState(&x, &y);
}

} // namespace Core

HK_NAMESPACE_END
