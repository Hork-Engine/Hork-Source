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

#include <Core/Public/String.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/EngineInterface.h>
#include <Runtime/Public/RenderCore.h>

class ARuntimeMain {
    AN_SINGLETON( ARuntimeMain )

public:
    int             NumArguments;
    char **         Arguments;

    AString         WorkingDir;
    char *          Executable;

    int64_t         SysStartSeconds;
    int64_t         SysStartMilliseconds;
    int64_t         SysStartMicroseconds;
    int64_t         SysFrameTimeStamp;

    SRenderFrame    FrameData;

    void *          FrameMemoryAddress;
    size_t          FrameMemorySize;
    size_t          FrameMemoryUsed;
    size_t          FrameMemoryUsedPrev;
    size_t          MaxFrameMemoryUsage;

    IEngineInterface * Engine;

    ACreateGameModuleCallback CreateGameModuleCallback;

    SCPUInfo        CPUInfo;

    bool            bTerminate;

    void Run( ACreateGameModuleCallback _CreateGameModule );

    int CheckArg( const char * _Arg );

private:
    void RuntimeMainLoop();

    void RuntimeUpdate();

    void EmergencyExit();

    void DisplayCriticalMessage( const char * _Message );

    void InitializeProcess();

    void DeinitializeProcess();

    void InitializeMemory();

    void DeinitializeMemory();

    void InitializeWorkingDirectory();

    int ProcessAttribute = 0;
};

extern ARuntimeMain & GRuntimeMain;
