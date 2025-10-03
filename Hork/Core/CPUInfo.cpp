/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "CPUInfo.h"
#include "Memory.h"

#ifdef HK_OS_LINUX

#    include <cpuid.h>

namespace
{

void CPUID(int32_t out[4], int32_t x)
{
    __cpuid_count(x, 0, out[0], out[1], out[2], out[3]);
}

uint64_t xgetbv(unsigned int index)
{
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv"
                         : "=a"(eax), "=d"(edx)
                         : "c"(index));
    return ((uint64_t)edx << 32) | eax;
}

} // namespace

#    define _XCR_XFEATURE_ENABLED_MASK 0

#endif

#ifdef HK_OS_WIN32

#include "WindowsDefs.h"

#include <immintrin.h>

namespace
{

void CPUID(int32_t out[4], int32_t x)
{
    __cpuidex(out, x, 0);
}

__int64 xgetbv(unsigned int index)
{
    return _xgetbv(index);
}

BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;

    typedef BOOL(WINAPI * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            bIsWow64 = FALSE;
        }
    }
    return bIsWow64;
}

} // namespace

#endif

HK_NAMESPACE_BEGIN

namespace Core
{

CPUInfo const* GetCPUInfo()
{
    struct CPUDetector
    {
        CPUInfo Info;

        CPUDetector()
        {
            int32_t cpuInfo[4];
            char vendor[13];

            ZeroMem(&Info, sizeof(Info));

#ifdef HK_OS_WIN32
#    ifdef _M_X64
            Info.OS_64bit = true;
#    else
            Info.OS_64bit = IsWow64() != 0;
#    endif
#else
            Info.OS_64bit = true;
#endif

            CPUID(cpuInfo, 1);

            bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
            bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

            if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
            {
                uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
                Info.OS_AVX = (xcrFeatureMask & 0x6) == 0x6;
            }

            if (Info.OS_AVX)
            {
                uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
                Info.OS_AVX512 = (xcrFeatureMask & 0xe6) == 0xe6;
            }

            CPUID(cpuInfo, 0);
            Memcpy(vendor + 0, &cpuInfo[1], 4);
            Memcpy(vendor + 4, &cpuInfo[3], 4);
            Memcpy(vendor + 8, &cpuInfo[2], 4);
            vendor[12] = '\0';

            if (!strcmp(vendor, "GenuineIntel"))
            {
                Info.Intel = true;
            }
            else if (!strcmp(vendor, "AuthenticAMD"))
            {
                Info.AMD = true;
            }

            int nIds = cpuInfo[0];

            CPUID(cpuInfo, 0x80000000);

            uint32_t nExIds = cpuInfo[0];

            if (nIds >= 0x00000001)
            {
                CPUID(cpuInfo, 0x00000001);

                Info.MMX = (cpuInfo[3] & (1 << 23)) != 0;
                Info.SSE = (cpuInfo[3] & (1 << 25)) != 0;
                Info.SSE2 = (cpuInfo[3] & (1 << 26)) != 0;
                Info.SSE3 = (cpuInfo[2] & (1 << 0)) != 0;

                Info.SSSE3 = (cpuInfo[2] & (1 << 9)) != 0;
                Info.SSE41 = (cpuInfo[2] & (1 << 19)) != 0;
                Info.SSE42 = (cpuInfo[2] & (1 << 20)) != 0;
                Info.AES = (cpuInfo[2] & (1 << 25)) != 0;

                Info.AVX = (cpuInfo[2] & (1 << 28)) != 0;
                Info.FMA3 = (cpuInfo[2] & (1 << 12)) != 0;

                Info.RDRAND = (cpuInfo[2] & (1 << 30)) != 0;
            }

            if (nIds >= 0x00000007)
            {
                CPUID(cpuInfo, 0x00000007);

                Info.AVX2 = (cpuInfo[1] & (1 << 5)) != 0;

                Info.BMI1 = (cpuInfo[1] & (1 << 3)) != 0;
                Info.BMI2 = (cpuInfo[1] & (1 << 8)) != 0;
                Info.ADX = (cpuInfo[1] & (1 << 19)) != 0;
                Info.MPX = (cpuInfo[1] & (1 << 14)) != 0;
                Info.SHA = (cpuInfo[1] & (1 << 29)) != 0;
                Info.PREFETCHWT1 = (cpuInfo[2] & (1 << 0)) != 0;

                Info.AVX512_F = (cpuInfo[1] & (1 << 16)) != 0;
                Info.AVX512_CD = (cpuInfo[1] & (1 << 28)) != 0;
                Info.AVX512_PF = (cpuInfo[1] & (1 << 26)) != 0;
                Info.AVX512_ER = (cpuInfo[1] & (1 << 27)) != 0;
                Info.AVX512_VL = (cpuInfo[1] & (1 << 31)) != 0;
                Info.AVX512_BW = (cpuInfo[1] & (1 << 30)) != 0;
                Info.AVX512_DQ = (cpuInfo[1] & (1 << 17)) != 0;
                Info.AVX512_IFMA = (cpuInfo[1] & (1 << 21)) != 0;
                Info.AVX512_VBMI = (cpuInfo[2] & (1 << 1)) != 0;
            }

            if (nExIds >= 0x80000001)
            {
                CPUID(cpuInfo, 0x80000001);
                Info.x64 = (cpuInfo[3] & (1 << 29)) != 0;
                Info.ABM = (cpuInfo[2] & (1 << 5)) != 0;
                Info.SSE4a = (cpuInfo[2] & (1 << 6)) != 0;
                Info.FMA4 = (cpuInfo[2] & (1 << 16)) != 0;
                Info.XOP = (cpuInfo[2] & (1 << 11)) != 0;
            }
        }
    };

    static CPUDetector cpuDetector;
    return &cpuDetector.Info;
}

} // namespace Core

HK_NAMESPACE_END
