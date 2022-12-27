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

#include <Platform/Platform.h>
#include <Platform/Memory/Memory.h>
#include <Platform/Logger.h>
#include <Platform/WindowsDefs.h>
#include <Platform/Path.h>
#include <Platform/String.h>
#include <Platform/ConsoleBuffer.h>

#include <SDL.h>

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

//////////////////////////////////////////////////////////////////////////////////////////
//
// Command line
//
//////////////////////////////////////////////////////////////////////////////////////////

namespace
{

/*************************************************************************
 * CommandLineToArgvA            [SHELL32.@]
 * 
 * MODIFIED FROM https://www.winehq.org/project
 * We must interpret the quotes in the command line to rebuild the argv
 * array correctly:
 * - arguments are separated by spaces or tabs
 * - quotes serve as optional argument delimiters
 *   '"a b"'   -> 'a b'
 * - escaped quotes must be converted back to '"'
 *   '\"'      -> '"'
 * - consecutive backslashes preceding a quote see their number halved with
 *   the remainder escaping the quote:
 *   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
 *   2n+1 backslashes + quote -> n backslashes + literal quote
 * - backslashes that are not followed by a quote are copied literally:
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 * - in quoted strings, consecutive quotes see their number divided by three
 *   with the remainder modulo 3 deciding whether to close the string or not.
 *   Note that the opening quote must be counted in the consecutive quotes,
 *   that's the (1+) below:
 *   (1+) 3n   quotes -> n quotes
 *   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
 *   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
 * - in unquoted strings, the first quote opens the quoted string and the
 *   remaining consecutive quotes follow the above rule.
 */

static char** CommandLineToArgvA(const char* lpCmdline, int* numargs)
{
    int         argc;
    char**      argv;
    const char* s;
    char*       d;
    char*       cmdline;
    int         qcount, bcount;

    if (!numargs || *lpCmdline == 0)
    {
        return nullptr;
    }

    /* --- First count the arguments */
    argc = 1;
    s    = lpCmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t')
            s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s)
                argc++;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s == '"')
        {
            /* '"' */
            if ((bcount & 1) == 0)
                qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"')
            {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv = (char**)malloc((argc + 1) * sizeof(char*) + (strlen(lpCmdline) + 1) * sizeof(char));
    if (!argv)
        return NULL;
    cmdline = (char*)(argv + argc + 1);
    strcpy(cmdline, lpCmdline);

    /* --- Then split and copy the arguments */
    argv[0] = d = cmdline;
    argc        = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s)
        {
            if (*s == '"')
            {
                s++;
                break;
            }
            *d++ = *s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t')
            d++;
        s = d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs   = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* close the argument */
            *d++   = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s)
                argv[argc++] = d;
        }
        else if (*s == '\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d    = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"')
            {
                if (++qcount == 3)
                {
                    *d++   = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            *d++   = *s++;
            bcount = 0;
        }
    }
    *d         = '\0';
    argv[argc] = NULL;
    *numargs   = argc;

    return argv;
}

} // namespace

HK_NAMESPACE_BEGIN

CommandLine::CommandLine(const char* _CommandLine)
{
    m_Arguments = CommandLineToArgvA(_CommandLine, &m_NumArguments);
    m_bNeedFree = true;

    Validate();
}

CommandLine::CommandLine(int _Argc, char** _Argv) :
    m_NumArguments(_Argc), m_Arguments(_Argv)
{
    Validate();
}

CommandLine::~CommandLine()
{
    if (m_bNeedFree)
    {
        free(m_Arguments);
    }
}

void CommandLine::Validate()
{
    if (m_NumArguments < 1)
    {
        HK_ASSERT(0);
        return;
    }
    // Fix executable path separator
    Platform::FixSeparator(m_Arguments[0]);
}

int CommandLine::CheckArg(const char* _Arg) const
{
    for (int i = 0; i < m_NumArguments; i++)
    {
#ifdef WIN32
        if (!stricmp(_Arg, m_Arguments[i]))
            return i;
#else
        if (!strcasecmp(_Arg, m_Arguments[i]))
            return i;
#endif
    }
    return -1;
}

bool CommandLine::HasArg(const char* _Arg) const
{
    return CheckArg(_Arg) != -1;
}

HK_NAMESPACE_END

//////////////////////////////////////////////////////////////////////////////////////////
//
// Main process
//
//////////////////////////////////////////////////////////////////////////////////////////

namespace
{

#ifdef HK_OS_WIN32
static HANDLE ProcessMutex = nullptr;
#endif
static FILE*       ProcessLogFile = nullptr;
static Hk::Mutex       ProcessLogWriterSync;
static Hk::ProcessInfo Process;

static void InitializeProcess()
{
    setlocale(LC_ALL, "C");
    srand((unsigned)time(NULL));

#if defined(HK_OS_WIN32)
    SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

    auto SDBMHash = [](const char* s, int len) -> uint32_t
    {
        uint32_t hash{0};
        for (int i = 0; i < len; i++)
            hash = s[i] + (hash << 6) + (hash << 16) - hash;
        return hash;
    };

#ifdef HK_OS_WIN32
    int curLen = 256;
    int len    = 0;

    Process.Executable = nullptr;
    while (1)
    {
        Process.Executable = (char*)realloc(Process.Executable, curLen + 1);

        len = GetModuleFileNameA(NULL, Process.Executable, curLen);
        if (len < curLen && len != 0)
        {
            break;
        }
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            curLen <<= 1;
        }
        else
        {
            Hk::CriticalError("InitializeProcess: Failed on GetModuleFileName\n");
            break;
        }
    }
    Process.Executable[len] = 0;

    Hk::Platform::FixSeparator(Process.Executable);

    uint32_t appHash = SDBMHash(Process.Executable, len);

    char pid[32];
    Hk::Platform::Sprintf(pid, sizeof(pid), "hork_%x", appHash);
    ProcessMutex = CreateMutexA(NULL, FALSE, pid);
    if (!ProcessMutex)
    {
        Process.ProcessAttribute = Hk::PROCESS_COULDNT_CHECK_UNIQUE;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        Process.ProcessAttribute = Hk::PROCESS_ALREADY_EXISTS;
    }
    else
    {
        Process.ProcessAttribute = Hk::PROCESS_UNIQUE;
    }
#elif defined HK_OS_LINUX
    int curLen = 256;
    int len = 0;

    Process.Executable = nullptr;
    while (1)
    {
        Process.Executable = (char*)realloc(Process.Executable, curLen + 1);
        len = readlink("/proc/self/exe", Process.Executable, curLen);
        if (len == -1)
        {
            CriticalError("InitializeProcess: Failed on readlink\n");
            len = 0;
            break;
        }
        if (len < curLen)
        {
            break;
        }
        curLen <<= 1;
    }
    Process.Executable[len] = 0;

    uint32_t appHash = SDBMHash(Process.Executable, len);
    char pid[32];
    Platform::Sprintf(pid, sizeof(pid), "/tmp/hork_%x.pid", appHash);
    int f = open(pid, O_RDWR | O_CREAT, 0666);
    int locked = flock(f, LOCK_EX | LOCK_NB);
    if (locked)
    {
        if (errno == EWOULDBLOCK)
        {
            Process.ProcessAttribute = PROCESS_ALREADY_EXISTS;
        }
        else
        {
            Process.ProcessAttribute = PROCESS_COULDNT_CHECK_UNIQUE;
        }
    }
    else
    {
        Process.ProcessAttribute = PROCESS_UNIQUE;
    }
#else
#    error "Not implemented under current platform"
#endif

    ProcessLogFile = nullptr;
    if (Hk::Platform::HasArg("-bEnableLog"))
    {
        // TODO: Set correct path for log file
        ProcessLogFile = fopen("log.txt", "ab");
    }
}

static void DeinitializeProcess()
{
    if (ProcessLogFile)
    {
        fclose(ProcessLogFile);
        ProcessLogFile = nullptr;
    }

    if (Process.Executable)
    {
        free(Process.Executable);
        Process.Executable = nullptr;
    }

#ifdef HK_OS_WIN32
    if (ProcessMutex)
    {
        ReleaseMutex(ProcessMutex);
        CloseHandle(ProcessMutex);
        ProcessMutex = NULL;
    }
#endif
}

} // namespace

//////////////////////////////////////////////////////////////////////////////////////////
//
// CPU Info
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef HK_OS_LINUX

#    include <cpuid.h>

namespace
{

static void CPUID(int32_t out[4], int32_t x)
{
    __cpuid_count(x, 0, out[0], out[1], out[2], out[3]);
}

static uint64_t xgetbv(unsigned int index)
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

#    include <immintrin.h>

namespace
{

static void CPUID(int32_t out[4], int32_t x)
{
    __cpuidex(out, x, 0);
}

static __int64 xgetbv(unsigned int index)
{
    return _xgetbv(index);
}

static BOOL IsWow64()
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


static Hk::CommandLine const* pCommandLine;

static int64_t StartSeconds;
static int64_t StartMilliseconds;
static int64_t StartMicroseconds;

static char* Clipboard = nullptr;
static Hk::ConsoleBuffer ConBuffer;

void* operator new(std::size_t sz)
{
    if (void* ptr = Hk::Platform::GetHeapAllocator<Hk::HEAP_MISC>().Alloc(std::max<size_t>(1, sz)))
        return ptr;

    // By default, exceptions are disabled in Hork.
#ifdef __cpp_exceptions
    throw std::bad_alloc{};
#else
    // Calling abort() to prevent compiler warnings.
    std::abort();
#endif
}

void operator delete(void* ptr) noexcept
{
    Hk::Platform::GetHeapAllocator<Hk::HEAP_MISC>().Free(ptr);
}

HK_NAMESPACE_BEGIN

namespace Platform
{

void Initialize(PlatformInitialize const& CoreInitialize)
{
#if defined(HK_DEBUG) && defined(HK_COMPILER_MSVC)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (CoreInitialize.pCommandLine)
    {
        static CommandLine cmdLine(CoreInitialize.pCommandLine);
        pCommandLine = &cmdLine;
    }
    else
    {
        static CommandLine cmdLine(CoreInitialize.Argc, CoreInitialize.Argv);
        pCommandLine = &cmdLine;
    }

    InitializeProcess();

    ProcessInfo const& processInfo = Platform::GetProcessInfo();

    if (!pCommandLine->HasArg("-bAllowMultipleInstances"))
    {
        switch (processInfo.ProcessAttribute)
        {
            case PROCESS_COULDNT_CHECK_UNIQUE:
                CriticalError("Couldn't check unique instance\n");
                break;
            case PROCESS_ALREADY_EXISTS:
                CriticalError("Application already runned\n");
                break;
            case PROCESS_UNIQUE:
                break;
        }
    }

    PrintCPUFeatures();

#ifdef HK_OS_WIN32
    size_t ProcessWorkingSetSizeMin = 192ull << 20;
    size_t ProcessWorkingSetSizeMax = 1024ull << 20;

    int n = Platform::CheckArg("-ProcessWorkingSetSize");
    if (n != -1 && (n + 1) < Platform::GetArgc())
    {
        ProcessWorkingSetSizeMin = std::max(0, atoi(Platform::GetArgv()[n]));
        ProcessWorkingSetSizeMax = std::max(0, atoi(Platform::GetArgv()[n + 1]));
    }

    if (ProcessWorkingSetSizeMin && ProcessWorkingSetSizeMax)
    {
        if (!SetProcessWorkingSetSize(GetCurrentProcess(), ProcessWorkingSetSizeMin, ProcessWorkingSetSizeMax))
        {
            LOG("Failed on SetProcessWorkingSetSize\n");
        }
    }
#endif

    MemoryInfo physMemoryInfo = Platform::GetPhysMemoryInfo();
    LOG("Memory page size: {} bytes\n", physMemoryInfo.PageSize);
    if (physMemoryInfo.TotalAvailableMegabytes > 0 && physMemoryInfo.CurrentAvailableMegabytes > 0)
    {
        LOG("Total available phys memory: {} Megs\n", physMemoryInfo.TotalAvailableMegabytes);
        LOG("Current available phys memory: {} Megs\n", physMemoryInfo.CurrentAvailableMegabytes);
    }

    SDL_SetMemoryFunctions(
        [](size_t size) -> void*
        {
            return GetHeapAllocator<HEAP_MISC>().Alloc(size, 0);
        },
        [](size_t nmemb, size_t size) -> void*
        {
            return GetHeapAllocator<HEAP_MISC>().Alloc(nmemb * size, 0, MALLOC_ZERO);
        },
        [](void* mem, size_t size) -> void*
        {
            return GetHeapAllocator<HEAP_MISC>().Realloc(mem, size, 0);
        },
        [](void* mem)
        {
            GetHeapAllocator<HEAP_MISC>().Free(mem);
        });


    SDL_LogSetOutputFunction(
        [](void* userdata, int category, SDL_LogPriority priority, const char* message)
        {
            LOG("SDL: {} : {}\n", category, message);
        },
        NULL);

    // Synchronize SDL ticks with our start time
    (void)SDL_GetTicks();

    StartMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    StartMilliseconds = StartMicroseconds * 0.001;
    StartSeconds      = StartMicroseconds * 0.000001;
}

void Deinitialize()
{
    DeinitializeProcess();

    if (Clipboard)
    {
        SDL_free(Clipboard);
    }

    SDL_Quit();
}

int GetArgc()
{
    return pCommandLine->GetArgc();
}

const char* const* GetArgv()
{
    return pCommandLine->GetArgv();
}

int CheckArg(const char* _Arg)
{
    return pCommandLine->CheckArg(_Arg);
}

bool HasArg(const char* _Arg)
{
    return pCommandLine->HasArg(_Arg);
}

CommandLine const* GetCommandLine()
{
    return pCommandLine;
}

ConsoleBuffer& GetConsoleBuffer()
{
    return ConBuffer;
}

CPUInfo const* GetCPUInfo()
{
    static CPUInfo* pCPUInfo = nullptr;
    static CPUInfo  info;

    if (pCPUInfo)
    {
        return pCPUInfo;
    }

    pCPUInfo = &info;

    int32_t cpuInfo[4];
    char    vendor[13];

    Platform::ZeroMem(&info, sizeof(info));

#ifdef HK_OS_WIN32
#    ifdef _M_X64
    info.OS_64bit = true;
#    else
    info.OS_64bit = IsWow64() != 0;
#    endif
#else
    info.OS_64bit = true;
#endif

    CPUID(cpuInfo, 1);

    bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    bool cpuAVXSuport        = (cpuInfo[2] & (1 << 28)) != 0;

    if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
    {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        info.OS_AVX             = (xcrFeatureMask & 0x6) == 0x6;
    }

    if (info.OS_AVX)
    {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        info.OS_AVX512          = (xcrFeatureMask & 0xe6) == 0xe6;
    }

    CPUID(cpuInfo, 0);
    Platform::Memcpy(vendor + 0, &cpuInfo[1], 4);
    Platform::Memcpy(vendor + 4, &cpuInfo[3], 4);
    Platform::Memcpy(vendor + 8, &cpuInfo[2], 4);
    vendor[12] = '\0';

    if (!strcmp(vendor, "GenuineIntel"))
    {
        info.Intel = true;
    }
    else if (!strcmp(vendor, "AuthenticAMD"))
    {
        info.AMD = true;
    }

    int nIds = cpuInfo[0];

    CPUID(cpuInfo, 0x80000000);

    uint32_t nExIds = cpuInfo[0];

    if (nIds >= 0x00000001)
    {
        CPUID(cpuInfo, 0x00000001);

        info.MMX  = (cpuInfo[3] & (1 << 23)) != 0;
        info.SSE  = (cpuInfo[3] & (1 << 25)) != 0;
        info.SSE2 = (cpuInfo[3] & (1 << 26)) != 0;
        info.SSE3 = (cpuInfo[2] & (1 << 0)) != 0;

        info.SSSE3 = (cpuInfo[2] & (1 << 9)) != 0;
        info.SSE41 = (cpuInfo[2] & (1 << 19)) != 0;
        info.SSE42 = (cpuInfo[2] & (1 << 20)) != 0;
        info.AES   = (cpuInfo[2] & (1 << 25)) != 0;

        info.AVX  = (cpuInfo[2] & (1 << 28)) != 0;
        info.FMA3 = (cpuInfo[2] & (1 << 12)) != 0;

        info.RDRAND = (cpuInfo[2] & (1 << 30)) != 0;
    }

    if (nIds >= 0x00000007)
    {
        CPUID(cpuInfo, 0x00000007);

        info.AVX2 = (cpuInfo[1] & (1 << 5)) != 0;

        info.BMI1        = (cpuInfo[1] & (1 << 3)) != 0;
        info.BMI2        = (cpuInfo[1] & (1 << 8)) != 0;
        info.ADX         = (cpuInfo[1] & (1 << 19)) != 0;
        info.MPX         = (cpuInfo[1] & (1 << 14)) != 0;
        info.SHA         = (cpuInfo[1] & (1 << 29)) != 0;
        info.PREFETCHWT1 = (cpuInfo[2] & (1 << 0)) != 0;

        info.AVX512_F    = (cpuInfo[1] & (1 << 16)) != 0;
        info.AVX512_CD   = (cpuInfo[1] & (1 << 28)) != 0;
        info.AVX512_PF   = (cpuInfo[1] & (1 << 26)) != 0;
        info.AVX512_ER   = (cpuInfo[1] & (1 << 27)) != 0;
        info.AVX512_VL   = (cpuInfo[1] & (1 << 31)) != 0;
        info.AVX512_BW   = (cpuInfo[1] & (1 << 30)) != 0;
        info.AVX512_DQ   = (cpuInfo[1] & (1 << 17)) != 0;
        info.AVX512_IFMA = (cpuInfo[1] & (1 << 21)) != 0;
        info.AVX512_VBMI = (cpuInfo[2] & (1 << 1)) != 0;
    }

    if (nExIds >= 0x80000001)
    {
        CPUID(cpuInfo, 0x80000001);
        info.x64   = (cpuInfo[3] & (1 << 29)) != 0;
        info.ABM   = (cpuInfo[2] & (1 << 5)) != 0;
        info.SSE4a = (cpuInfo[2] & (1 << 6)) != 0;
        info.FMA4  = (cpuInfo[2] & (1 << 16)) != 0;
        info.XOP   = (cpuInfo[2] & (1 << 11)) != 0;
    }

    return pCPUInfo;
}

ProcessInfo const& GetProcessInfo()
{
    return Process;
}

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

void PrintCPUFeatures()
{
    CPUInfo const* pCPUInfo = Platform::GetCPUInfo();

    LOG("CPU: {}\n", pCPUInfo->Intel ? "Intel" : "AMD");
    LOG("CPU Features:");
    if (pCPUInfo->MMX) LOG(" MMX");
    if (pCPUInfo->x64) LOG(" x64");
    if (pCPUInfo->ABM) LOG(" ABM");
    if (pCPUInfo->RDRAND) LOG(" RDRAND");
    if (pCPUInfo->BMI1) LOG(" BMI1");
    if (pCPUInfo->BMI2) LOG(" BMI2");
    if (pCPUInfo->ADX) LOG(" ADX");
    if (pCPUInfo->MPX) LOG(" MPX");
    if (pCPUInfo->PREFETCHWT1) LOG(" PREFETCHWT1");
    LOG("\n");
    LOG("Simd 128 bit:");
    if (pCPUInfo->SSE) LOG(" SSE");
    if (pCPUInfo->SSE2) LOG(" SSE2");
    if (pCPUInfo->SSE3) LOG(" SSE3");
    if (pCPUInfo->SSSE3) LOG(" SSSE3");
    if (pCPUInfo->SSE4a) LOG(" SSE4a");
    if (pCPUInfo->SSE41) LOG(" SSE4.1");
    if (pCPUInfo->SSE42) LOG(" SSE4.2");
    if (pCPUInfo->AES) LOG(" AES-NI");
    if (pCPUInfo->SHA) LOG(" SHA");
    LOG("\n");
    LOG("Simd 256 bit:");
    if (pCPUInfo->AVX) LOG(" AVX");
    if (pCPUInfo->XOP) LOG(" XOP");
    if (pCPUInfo->FMA3) LOG(" FMA3");
    if (pCPUInfo->FMA4) LOG(" FMA4");
    if (pCPUInfo->AVX2) LOG(" AVX2");
    LOG("\n");
    LOG("Simd 512 bit:");
    if (pCPUInfo->AVX512_F) LOG(" AVX512-F");
    if (pCPUInfo->AVX512_CD) LOG(" AVX512-CD");
    if (pCPUInfo->AVX512_PF) LOG(" AVX512-PF");
    if (pCPUInfo->AVX512_ER) LOG(" AVX512-ER");
    if (pCPUInfo->AVX512_VL) LOG(" AVX512-VL");
    if (pCPUInfo->AVX512_BW) LOG(" AVX512-BW");
    if (pCPUInfo->AVX512_DQ) LOG(" AVX512-DQ");
    if (pCPUInfo->AVX512_IFMA) LOG(" AVX512-IFMA");
    if (pCPUInfo->AVX512_VBMI) LOG(" AVX512-VBMI");
    LOG("\n");
    LOG("OS: " HK_OS_STRING "\n");
    LOG("OS Features:");
    if (pCPUInfo->OS_64bit) LOG(" 64bit");
    if (pCPUInfo->OS_AVX) LOG(" AVX");
    if (pCPUInfo->OS_AVX512) LOG(" AVX512");
    LOG("\n");
    LOG("Endian: " HK_ENDIAN_STRING "\n");
#ifdef HK_DEBUG
    LOG("Compiler: " HK_COMPILER_STRING "\n");
#endif
}

void WriteLog(const char* _Message)
{
    if (ProcessLogFile)
    {
        MutexGurad syncGuard(ProcessLogWriterSync);
        fprintf(ProcessLogFile, "%s", _Message);
        fflush(ProcessLogFile);
    }
}

void WriteDebugString(const char* _Message)
{
#if defined HK_DEBUG
#    if defined HK_COMPILER_MSVC
    {
        int n = MultiByteToWideChar(CP_UTF8, 0, _Message, -1, NULL, 0);
        if (0 != n)
        {
            // Try to alloc on stack
            if (n < 4096)
            {
                wchar_t* chars = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

                MultiByteToWideChar(CP_UTF8, 0, _Message, -1, chars, n);

                OutputDebugString(chars);
            }
            else
            {
                wchar_t* chars = (wchar_t*)malloc(n * sizeof(wchar_t));

                MultiByteToWideChar(CP_UTF8, 0, _Message, -1, chars, n);

                OutputDebugString(chars);

                free(chars);
            }
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

void WriteConsole(const char* Message)
{
    ConBuffer.Print(Message);
}

void* LoadDynamicLib(const char* _LibraryName)
{
    return SDL_LoadObject(_LibraryName);
}

void UnloadDynamicLib(void* _Handle)
{
    SDL_UnloadObject(_Handle);
}

void* GetProcAddress(void* _Handle, const char* _ProcName)
{
    return _Handle ? SDL_LoadFunction(_Handle, _ProcName) : nullptr;
}

void SetClipboard(const char* _Utf8String)
{
    SDL_SetClipboardText(_Utf8String);
}

const char* GetClipboard()
{
    if (Clipboard)
    {
        SDL_free(Clipboard);
    }
    Clipboard = SDL_GetClipboardText();
    return Clipboard;
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

void SetCursorEnabled(bool _Enabled)
{
    //SDL_SetWindowGrab( Wnd, (SDL_bool)!_Enabled ); // FIXME: Always grab in fullscreen?
    SDL_SetRelativeMouseMode((SDL_bool)!_Enabled);
}

bool IsCursorEnabled()
{
    return !SDL_GetRelativeMouseMode();
}

void GetCursorPosition(int& _X, int& _Y)
{
    (void)SDL_GetMouseState(&_X, &_Y);
}

} // namespace Platform

HK_NAMESPACE_END

namespace
{

static void DisplayCriticalMessage(const char* _Message)
{
#if defined HK_OS_WIN32
    wchar_t wstr[1024];
    MultiByteToWideChar(CP_UTF8, 0, _Message, -1, wstr, HK_ARRAY_SIZE(wstr));
    MessageBox(NULL, wstr, L"Critical Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
#else
    SDL_MessageBoxData data = {};
    SDL_MessageBoxButtonData button = {};
    const SDL_MessageBoxColorScheme scheme =
        {
            {
                {56, 54, 53},    /* SDL_MESSAGEBOX_COLOR_BACKGROUND, */
                {209, 207, 205}, /* SDL_MESSAGEBOX_COLOR_TEXT, */
                {140, 135, 129}, /* SDL_MESSAGEBOX_COLOR_BUTTON_BORDER, */
                {105, 102, 99},  /* SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND, */
                {205, 202, 53},  /* SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED, */
            }};

    data.flags = SDL_MESSAGEBOX_ERROR;
    data.title = "Critical Error";
    data.message = _Message;
    data.numbuttons = 1;
    data.buttons = &button;
    data.window = NULL;
    data.colorScheme = &scheme;

    button.flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    button.flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    button.text = "OK";

    SDL_ShowMessageBox(&data, NULL);
#endif
}

} // namespace

HK_NAMESPACE_BEGIN

void _CriticalError(const char* Text)
{
    DisplayCriticalMessage(Text);

    SDL_Quit();

    MemoryHeap::MemoryCleanup();

    DeinitializeProcess();

    std::quick_exit(0);
}

#ifdef HK_ALLOW_ASSERTS

// Define global assert function
void AssertFunction(const char* _File, int _Line, const char* _Function, const char* _Assertion, const char* _Comment)
{
    static thread_local bool bNestedFunctionCall = false;

    if (bNestedFunctionCall)
    {
        // Assertion occurs in logger's print function
        return;
    }

    bNestedFunctionCall = true;

    // LOG is thread-safe function so we don't need to wrap it by a critical section
    LOG("===== Assertion failed =====\n"
        "At file {}, line {}\n"
        "Function: {}\n"
        "Assertion: {}\n"
        "{}{}"
        "============================\n",
        _File, _Line, _Function, _Assertion, _Comment ? _Comment : "", _Comment ? "\n" : "");

    SDL_SetRelativeMouseMode(SDL_FALSE); // FIXME: Is it threadsafe?

#    ifdef HK_OS_WIN32
    DebugBreak();
#    else
    //__asm__( "int $3" );
    raise(SIGTRAP);
#    endif

    bNestedFunctionCall = false;
}

#endif

HK_NAMESPACE_END
