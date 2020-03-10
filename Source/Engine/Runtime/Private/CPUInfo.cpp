/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "CPUInfo.h"

#include <Core/Public/WindowsDefs.h>

#ifdef AN_OS_LINUX

#include <cpuid.h>

static void CPUID( int32_t out[4], int32_t x ) {
    __cpuid_count( x, 0, out[0], out[1], out[2], out[3] );
}

static uint64_t xgetbv( unsigned int index ) {
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
    return ((uint64_t)edx << 32) | eax;
}

#define _XCR_XFEATURE_ENABLED_MASK 0

#endif

#ifdef AN_OS_WIN32

static void CPUID( int32_t out[4], int32_t x ) {
    __cpuidex( out, x, 0 );
}

static __int64 xgetbv( unsigned int index ) {
    return _xgetbv( index );
}

static BOOL IsWow64() {
    BOOL bIsWow64 = FALSE;

    typedef BOOL ( WINAPI *LPFN_ISWOW64PROCESS ) ( HANDLE, PBOOL );
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if ( NULL != fnIsWow64Process ) {
        if ( !fnIsWow64Process( GetCurrentProcess(), &bIsWow64 ) ) {
            bIsWow64 = FALSE;
        }
    }
    return bIsWow64;
}

#endif

void GetCPUInfo( SCPUInfo & _Info ) {
    int32_t cpuInfo[4];
    char vendor[13];

    Core::ZeroMem( &_Info, sizeof( _Info ) );

#ifdef AN_OS_WIN32
#ifdef _M_X64
    _Info.OS_64bit = true;
#else
    _Info.OS_64bit = IsWow64() != 0;
#endif
#else
    _Info.OS_64bit = true;
#endif

    CPUID( cpuInfo, 1 );

    bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

    if ( osUsesXSAVE_XRSTORE && cpuAVXSuport ) {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        _Info.OS_AVX = (xcrFeatureMask & 0x6) == 0x6;
    }

    if ( _Info.OS_AVX ) {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        _Info.OS_AVX512 = (xcrFeatureMask & 0xe6) == 0xe6;
    }

    CPUID( cpuInfo, 0 );
    Core::Memcpy( vendor + 0, &cpuInfo[1], 4 );
    Core::Memcpy( vendor + 4, &cpuInfo[3], 4 );
    Core::Memcpy( vendor + 8, &cpuInfo[2], 4 );
    vendor[12] = '\0';

    if ( !Core::Strcmp( vendor, "GenuineIntel" ) ){
        _Info.Intel = true;
    } else if ( !Core::Strcmp( vendor, "AuthenticAMD" ) ){
        _Info.AMD = true;
    }

    int nIds = cpuInfo[0];

    CPUID( cpuInfo, 0x80000000 );

    uint32_t nExIds = cpuInfo[0];

    if ( nIds >= 0x00000001 ) {
        CPUID( cpuInfo, 0x00000001 );

        _Info.MMX    = (cpuInfo[3] & (1 << 23)) != 0;
        _Info.SSE    = (cpuInfo[3] & (1 << 25)) != 0;
        _Info.SSE2   = (cpuInfo[3] & (1 << 26)) != 0;
        _Info.SSE3   = (cpuInfo[2] & (1 <<  0)) != 0;

        _Info.SSSE3  = (cpuInfo[2] & (1 <<  9)) != 0;
        _Info.SSE41  = (cpuInfo[2] & (1 << 19)) != 0;
        _Info.SSE42  = (cpuInfo[2] & (1 << 20)) != 0;
        _Info.AES    = (cpuInfo[2] & (1 << 25)) != 0;

        _Info.AVX    = (cpuInfo[2] & (1 << 28)) != 0;
        _Info.FMA3   = (cpuInfo[2] & (1 << 12)) != 0;

        _Info.RDRAND = (cpuInfo[2] & (1 << 30)) != 0;
    }

    if ( nIds >= 0x00000007 ) {
        CPUID( cpuInfo, 0x00000007 );

        _Info.AVX2         = (cpuInfo[1] & (1 <<  5)) != 0;

        _Info.BMI1         = (cpuInfo[1] & (1 <<  3)) != 0;
        _Info.BMI2         = (cpuInfo[1] & (1 <<  8)) != 0;
        _Info.ADX          = (cpuInfo[1] & (1 << 19)) != 0;
        _Info.MPX          = (cpuInfo[1] & (1 << 14)) != 0;
        _Info.SHA          = (cpuInfo[1] & (1 << 29)) != 0;
        _Info.PREFETCHWT1  = (cpuInfo[2] & (1 <<  0)) != 0;

        _Info.AVX512_F     = (cpuInfo[1] & (1 << 16)) != 0;
        _Info.AVX512_CD    = (cpuInfo[1] & (1 << 28)) != 0;
        _Info.AVX512_PF    = (cpuInfo[1] & (1 << 26)) != 0;
        _Info.AVX512_ER    = (cpuInfo[1] & (1 << 27)) != 0;
        _Info.AVX512_VL    = (cpuInfo[1] & (1 << 31)) != 0;
        _Info.AVX512_BW    = (cpuInfo[1] & (1 << 30)) != 0;
        _Info.AVX512_DQ    = (cpuInfo[1] & (1 << 17)) != 0;
        _Info.AVX512_IFMA  = (cpuInfo[1] & (1 << 21)) != 0;
        _Info.AVX512_VBMI  = (cpuInfo[2] & (1 <<  1)) != 0;
    }

    if ( nExIds >= 0x80000001 ) {
        CPUID( cpuInfo, 0x80000001 );
        _Info.x64   = (cpuInfo[3] & (1 << 29)) != 0;
        _Info.ABM   = (cpuInfo[2] & (1 <<  5)) != 0;
        _Info.SSE4a = (cpuInfo[2] & (1 <<  6)) != 0;
        _Info.FMA4  = (cpuInfo[2] & (1 << 16)) != 0;
        _Info.XOP   = (cpuInfo[2] & (1 << 11)) != 0;
    }
}
