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

#include <Platform/Logger.h>
#include <Platform/Memory/Memory.h>
#include <Platform/String.h>

#if defined HK_COMPILER_MSVC && defined HK_DEBUG
#    include <Platform/WindowsDefs.h>
#endif

#ifdef HK_OS_ANDROID
#    include <android/log.h>
#endif

ALogger GLogger;

thread_local char LogBuffer[16384];

void ALogger::Critical(const char* _Format, ...)
{
    va_list VaList;
    va_start(VaList, _Format);
    Platform::VSprintf(LogBuffer, sizeof(LogBuffer), _Format, VaList);
    va_end(VaList);
    MessageCallback(LOGGER_LEVEL_CRITICAL, LogBuffer, UserData);
}

void ALogger::Error(const char* _Format, ...)
{
    va_list VaList;
    va_start(VaList, _Format);
    Platform::VSprintf(LogBuffer, sizeof(LogBuffer), _Format, VaList);
    va_end(VaList);
    MessageCallback(LOGGER_LEVEL_ERROR, LogBuffer, UserData);
}

void ALogger::Warning(const char* _Format, ...)
{
    va_list VaList;
    va_start(VaList, _Format);
    Platform::VSprintf(LogBuffer, sizeof(LogBuffer), _Format, VaList);
    va_end(VaList);
    MessageCallback(LOGGER_LEVEL_WARNING, LogBuffer, UserData);
}

void ALogger::DebugMessage(const char* _Format, ...)
{
#ifdef HK_DEBUG
    va_list VaList;
    va_start(VaList, _Format);
    Platform::VSprintf(LogBuffer, sizeof(LogBuffer), _Format, VaList);
    va_end(VaList);
    MessageCallback(LOGGER_LEVEL_MESSAGE, LogBuffer, UserData);
#endif
}

void ALogger::Printf(const char* _Format, ...)
{
    va_list VaList;
    va_start(VaList, _Format);
    Platform::VSprintf(LogBuffer, sizeof(LogBuffer), _Format, VaList);
    va_end(VaList);
    MessageCallback(LOGGER_LEVEL_MESSAGE, LogBuffer, UserData);
}

void ALogger::Print(const char* _Message)
{
    MessageCallback(LOGGER_LEVEL_MESSAGE, _Message, UserData);
}

void ALogger::_Printf(int _Level, const char* _Format, ...)
{
    va_list VaList;
    va_start(VaList, _Format);
    Platform::VSprintf(LogBuffer, sizeof(LogBuffer), _Format, VaList);
    va_end(VaList);
    MessageCallback(_Level, LogBuffer, UserData);
}

void ALogger::SetMessageCallback(void (*_MessageCallback)(int, const char*, void*), void *_UserData)
{
    MessageCallback = _MessageCallback;
    UserData        = _UserData;
}

void ALogger::DefaultMessageCallback(int, const char* _Message, void*)
{
#if defined HK_DEBUG
#    if defined HK_COMPILER_MSVC
    {
        int n = MultiByteToWideChar(CP_UTF8, 0, _Message, -1, NULL, 0);
        if (0 != n)
        {
            wchar_t* chars = (wchar_t*)StackAlloc(n * sizeof(wchar_t));

            MultiByteToWideChar(CP_UTF8, 0, _Message, -1, chars, n);

            OutputDebugString(chars);
        }
    }
#    else
#        ifdef HK_OS_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "Hork Engine", _Message);
#        else
    fprintf(stdout, "%s", _Message);
    fflush(stdout);
#        endif
#    endif
#endif
}
