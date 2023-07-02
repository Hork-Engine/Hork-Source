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

#include "BaseTypes.h"

HK_NAMESPACE_BEGIN

struct MemoryInfo
{
    size_t TotalAvailableMegabytes;
    size_t CurrentAvailableMegabytes;
    size_t PageSize;
};

namespace Core
{

/** Get total/available memory status */
MemoryInfo GetPhysMemoryInfo();

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

void WriteDebugString(const char* message);

/** Load dynamic library (.dll or .so) */
void* LoadDynamicLib(const char* libraryName);

/** Unload dynamic library (.dll or .so) */
void UnloadDynamicLib(void* handle);

/** Get address of procedure in dynamic library */
void* GetProcAddress(void* handle, const char* procName);

/** Helper. Get address of procedure in dynamic library */
template <typename T>
bool GetProcAddress(void* handle, T** procPtr, const char* procName)
{
    return nullptr != ((*procPtr) = (T*)GetProcAddress(handle, procName));
}

void SetCursorEnabled(bool bEnabled);

bool IsCursorEnabled();

void GetCursorPosition(int& _X, int& _Y);

} // namespace Core



HK_NAMESPACE_END
