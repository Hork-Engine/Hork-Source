/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#pragma once

#include <Engine/Core/Public/String.h>
#include <Engine/Runtime/Public/Runtime.h>

extern int rt_NumArguments;
extern char **rt_Arguments;
extern FString rt_WorkingDir;
extern char * rt_Executable;
extern int64_t rt_SysStartSeconds;
extern int64_t rt_SysStartMilliseconds;
extern int64_t rt_SysStartMicroseconds;
extern int64_t rt_SysFrameTimeStamp;
extern FRenderFrame rt_FrameData;
extern FEventQueue rt_Events;
extern FEventQueue rt_GameEvents;
extern void * rt_FrameMemoryAddress;
extern size_t rt_FrameMemorySize;
extern size_t rt_FrameMemoryUsed;
extern size_t rt_FrameMemoryUsedPrev;

int rt_CheckArg( const char * _Arg );
