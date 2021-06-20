/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "String.h"

struct SCommandLine
{
    AN_FORBID_COPY(SCommandLine)

    SCommandLine( const char * _CommandLine );
    SCommandLine( int _Argc, char ** _Argv );
    ~SCommandLine();

    /** Application command line args count */
    int GetArgc() const { return NumArguments; }

    /** Application command line args */
    const char * const *GetArgv() const { return Arguments; }

    /** Check is argument exists in application command line. Return argument index or -1 if argument was not found. */
    int CheckArg( AStringView _Arg ) const;

    /** Check is argument exists in application command line. Return false if argument was not found. */
    bool HasArg( AStringView _Arg ) const;

private:
    void Validate();

    int     NumArguments = 0;
    char ** Arguments = nullptr;

    bool    bNeedFree = false;
};

/** CPU features */
struct SCPUInfo
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

struct SMemoryInfo
{
    int TotalAvailableMegabytes;
    int CurrentAvailableMegabytes;
    int PageSize;
};

enum PROCESS_ATTRIBUTE
{
    PROCESS_COULDNT_CHECK_UNIQUE = 1,
    PROCESS_ALREADY_EXISTS,
    PROCESS_UNIQUE
};

struct SProcessInfo
{
    SProcessInfo()
        : ProcessAttribute(0)
        , Executable(nullptr)
    {
    }

    int ProcessAttribute;
    char * Executable;
};

struct SCoreInitialize
{
    SCoreInitialize()
        : Argv(nullptr)
        , Argc(0)
        , pCommandLine(nullptr)
        , bAllowMultipleInstances(true)
    {
    }

    char ** Argv;
    int Argc;
    const char * pCommandLine;
    bool bAllowMultipleInstances;
    size_t ZoneSizeInMegabytes = 256;
    size_t HunkSizeInMegabytes = 32;
    size_t FrameMemorySizeInMegabytes = 16;
};

namespace Core
{

void Initialize( SCoreInitialize const & CoreInitialize );

void Deinitialize();

std::string & GetMessageBuffer();

/** Application command line args count */
int GetArgc();

/** Application command line args */
const char * const *GetArgv();

/** Check is argument exists in application command line. Return argument index or -1 if argument was not found. */
int CheckArg( AStringView _Arg );

/** Check is argument exists in application command line. Return false if argument was not found. */
bool HasArg( AStringView _Arg );

SCommandLine const * GetCommandLine();

/** Get CPU info */
SCPUInfo const * CPUInfo();

/** Get process info */
SProcessInfo const & GetProcessInfo();

/** Write message to log file */
void WriteLog( const char * _Message );

/** Write message to debug console */
void WriteDebugString( const char * _Message );

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
void * LoadDynamicLib( AStringView _LibraryName );

/** Unload dynamic library (.dll or .so) */
void UnloadDynamicLib( void * _Handle );

/** Get address of procedure in dynamic library */
void * GetProcAddress( void * _Handle, const char * _ProcName );

/** Helper. Get address of procedure in dynamic library */
template< typename T >
bool GetProcAddress( void * _Handle, T ** _ProcPtr, const char * _ProcName ) {
    return nullptr != ( (*_ProcPtr) = (T *)GetProcAddress( _Handle, _ProcName ) );
}

/** Set clipboard text */
void SetClipboard( const char * _Utf8String );

/** Set clipboard text */
AN_FORCEINLINE void SetClipboard( AString const & _Clipboard ) { SetClipboard( _Clipboard.CStr() ); }

/** Get clipboard text */
const char * GetClipboard();

SMemoryInfo GetPhysMemoryInfo();

void * GetFrameMemoryAddress();

size_t GetFrameMemorySize();

}

/** Show critical error and exit */
void CriticalError( const char * _Format, ... );
