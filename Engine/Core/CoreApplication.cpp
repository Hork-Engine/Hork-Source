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

#include "CoreApplication.h"
#include "Platform.h"
#include "WindowsDefs.h"
#include "CPUInfo.h"
#include "Profiler.h"
#include "Logger.h"
#include "ConsoleVar.h"
#include "HashFunc.h"

#include <SDL/SDL.h>

HK_NAMESPACE_BEGIN

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


ApplicationArguments::ApplicationArguments(ArgumentPack const& args) :
    m_NumArguments(args.Argc),
    m_Arguments(const_cast<char**>(args.Argv))
{
#ifdef HK_OS_WIN32
    if (!m_Arguments)
    {
        m_Arguments = CommandLineToArgvA(GetCommandLineA(), &m_NumArguments);
        m_bNeedFree = true;
    }
#endif

    if (m_NumArguments <= 0 || !m_Arguments)
    {
        static char* dummyArgs[1] = {""};
        m_NumArguments = 1;
        m_Arguments = dummyArgs;
        m_bNeedFree = false;
    }
}

ApplicationArguments::~ApplicationArguments()
{
    if (m_bNeedFree)
        free(m_Arguments);
}

bool ApplicationArguments::Has(StringView name) const
{
    return Find(name) != -1;
}

int ApplicationArguments::Find(StringView name) const
{
    for (int i = 0; i < m_NumArguments; i++)
    {
        if (name.Icompare(m_Arguments[i]))
            return i;
    }
    return -1;
}

namespace {

void PrintCPUFeatures()
{
    CPUInfo const* pCPUInfo = Core::GetCPUInfo();

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
}

}

enum PROCESS_ATTRIBUTE
{
    PROCESS_COULDNT_CHECK_UNIQUE = 1,
    PROCESS_ALREADY_EXISTS,
    PROCESS_UNIQUE
};

CoreApplication* CoreApplication::s_Instance = {};

CoreApplication::CoreApplication(ArgumentPack const& args) :
    m_Arguments(args)
{
    HK_ASSERT(!s_Instance);
    s_Instance = this;

    setlocale(LC_ALL, "C");
    srand((unsigned)time(NULL));

#if defined(HK_OS_WIN32)
    SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

#ifdef HK_OS_WIN32
    int curLen = 256;
    int len = 0;

    m_Executable = nullptr;
    while (1)
    {
        m_Executable = (char*)realloc(m_Executable, curLen + 1);

        len = GetModuleFileNameA(NULL, m_Executable, curLen);
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
            TerminateWithError("InitializeProcess: Failed on GetModuleFileName\n");
            break;
        }
    }
    m_Executable[len] = 0;

    Core::FixSeparatorInplace(m_Executable);

    uint32_t appHash = HashTraits::SDBMHash(m_Executable, len);

    char pid[32];
    Core::Sprintf(pid, sizeof(pid), "hork_%x", appHash);
    m_ProcessMutex = CreateMutexA(NULL, FALSE, pid);
    if (!m_ProcessMutex)
    {
        m_ProcessAttribute = PROCESS_COULDNT_CHECK_UNIQUE;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        m_ProcessAttribute = PROCESS_ALREADY_EXISTS;
    }
    else
    {
        m_ProcessAttribute = PROCESS_UNIQUE;
    }
#elif defined HK_OS_LINUX
    int curLen = 256;
    int len = 0;

    m_Executable = nullptr;
    while (1)
    {
        m_Executable = (char*)realloc(m_Executable, curLen + 1);
        len = readlink("/proc/self/exe", m_Executable, curLen);
        if (len == -1)
        {
            TerminateWithError("InitializeProcess: Failed on readlink\n");
            len = 0;
            break;
        }
        if (len < curLen)
        {
            break;
        }
        curLen <<= 1;
    }
    m_Executable[len] = 0;

    uint32_t appHash = HashTraits::SDBMHash(m_Executable, len);
    char pid[32];
    Core::Sprintf(pid, sizeof(pid), "/tmp/hork_%x.pid", appHash);
    int f = open(pid, O_RDWR | O_CREAT, 0666);
    int locked = flock(f, LOCK_EX | LOCK_NB);
    if (locked)
    {
        if (errno == EWOULDBLOCK)
        {
            m_ProcessAttribute = PROCESS_ALREADY_EXISTS;
        }
        else
        {
            m_ProcessAttribute = PROCESS_COULDNT_CHECK_UNIQUE;
        }
    }
    else
    {
        m_ProcessAttribute = PROCESS_UNIQUE;
    }
#else
#    error "Not implemented under current platform"
#endif

    m_LogFile = nullptr;
    if (m_Arguments.Has("-bEnableLog"))
    {
        // TODO: Set correct path for log file
        m_LogFile = fopen("log.txt", "ab");
    }

    if (!m_Arguments.Has("-bAllowMultipleInstances"))
    {
        switch (m_ProcessAttribute)
        {
            case PROCESS_COULDNT_CHECK_UNIQUE:
                TerminateWithError("Couldn't check unique instance\n");
                break;
            case PROCESS_ALREADY_EXISTS:
                TerminateWithError("Application already runned\n");
                break;
            case PROCESS_UNIQUE:
                break;
        }
    }

#ifdef HK_DEBUG
    LOG("Compiler: " HK_COMPILER_STRING "\n");
#endif
    LOG("Endian: " HK_ENDIAN_STRING "\n");

    PrintCPUFeatures();

    LOG("Num hardware threads: {}\n", Thread::NumHardwareThreads);

#ifdef HK_OS_WIN32
    size_t ProcessWorkingSetSizeMin = 192ull << 20;
    size_t ProcessWorkingSetSizeMax = 1024ull << 20;

    int n = m_Arguments.Find("-ProcessWorkingSetSize");
    if (n != -1 && (n + 2) < m_Arguments.Count())
    {
        ProcessWorkingSetSizeMin = std::max(0, atoi(m_Arguments.At(n + 1)));
        ProcessWorkingSetSizeMax = std::max(0, atoi(m_Arguments.At(n + 2)));
    }

    if (ProcessWorkingSetSizeMin && ProcessWorkingSetSizeMax)
    {
        if (!SetProcessWorkingSetSize(GetCurrentProcess(), ProcessWorkingSetSizeMin, ProcessWorkingSetSizeMax))
        {
            LOG("Failed on SetProcessWorkingSetSize\n");
        }
    }
#endif

    MemoryInfo physMemoryInfo = Core::GetPhysMemoryInfo();
    LOG("Memory page size: {} bytes\n", physMemoryInfo.PageSize);
    if (physMemoryInfo.TotalAvailableMegabytes > 0 && physMemoryInfo.CurrentAvailableMegabytes > 0)
    {
        LOG("Total available phys memory: {} Megs\n", physMemoryInfo.TotalAvailableMegabytes);
        LOG("Current available phys memory: {} Megs\n", physMemoryInfo.CurrentAvailableMegabytes);
    }

    SDL_SetMemoryFunctions(
        [](size_t size) -> void*
        {
            return Core::GetHeapAllocator<HEAP_MISC>().Alloc(size, 0);
        },
        [](size_t nmemb, size_t size) -> void*
        {
            return Core::GetHeapAllocator<HEAP_MISC>().Alloc(nmemb * size, 0, MALLOC_ZERO);
        },
        [](void* mem, size_t size) -> void*
        {
            return Core::GetHeapAllocator<HEAP_MISC>().Realloc(mem, size, 0);
        },
        [](void* mem)
        {
            Core::GetHeapAllocator<HEAP_MISC>().Free(mem);
        });

    SDL_LogSetOutputFunction(
        [](void* userdata, int category, SDL_LogPriority priority, const char* message)
        {
            LOG("SDL: {} : {}\n", category, message);
        },
        NULL);

    ConsoleVar::AllocateVariables();

    Core::InitializeProfiler();

    m_WorkingDir = PathUtils::GetFilePath(m_Executable);

#if defined HK_OS_WIN32
    SetCurrentDirectoryA(m_WorkingDir.CStr());
#elif defined HK_OS_LINUX
    int r = chdir(m_WorkingDir.CStr());
    if (r != 0)
    {
        LOG("Cannot set working directory\n");
    }
#else
#    error "InitializeDirectories not implemented under current platform"
#endif

#if 0
    m_RootPath = m_pModuleDecl->RootPath;
    if (m_RootPath.IsEmpty())
    {
        m_RootPath = "Data/";
    }
    else
    {
        PathUtils::FixSeparatorInplace(m_RootPath);
        if (m_RootPath[m_RootPath.Length() - 1] != '/')
        {
            m_RootPath += '/';
        }
    }
#else
    m_RootPath = "Data/";
#endif

    LOG("Working directory: {}\n", m_WorkingDir);
    LOG("Root path: {}\n", m_RootPath);
    LOG("Executable: {}\n", m_Executable);
}

CoreApplication::~CoreApplication()
{
    Cleanup();
}

void CoreApplication::Cleanup()
{
    Core::ShutdownProfiler();

    m_WorkingDir.Free();

    ConsoleVar::FreeVariables();

    if (m_LogFile)
    {
        fclose(m_LogFile);
        m_LogFile = nullptr;
    }

    if (m_Executable)
    {
        free(m_Executable);
        m_Executable = nullptr;
    }

#ifdef HK_OS_WIN32
    if (m_ProcessMutex)
    {
        ReleaseMutex(m_ProcessMutex);
        CloseHandle(m_ProcessMutex);
        m_ProcessMutex = NULL;
    }
#endif

    if (m_Clipboard)
    {
        SDL_free(m_Clipboard);
    }

    SDL_Quit();
}

void CoreApplication::_WriteMessage(const char* message)
{
    Core::WriteDebugString(message);

    m_ConsoleBuffer.Print(message);

    if (m_LogFile)
    {
        MutexGuard lock(m_LogWriterSync);
        fprintf(m_LogFile, "%s", message);
        fflush(m_LogFile);
    }
}

void CoreApplication::SetClipboard(StringView text)
{
    if (text.IsNullTerminated())
    {
        SDL_SetClipboardText(text.ToPtr());
    }
    else
    {
        // Create null-terminated copy of string
        String str(text);
        SDL_SetClipboardText(str.CStr());
    }
}

void CoreApplication::SetClipboard(String const& text)
{
    SDL_SetClipboardText(text.CStr());
}

const char* CoreApplication::GetClipboard()
{
    if (s_Instance->m_Clipboard)
        SDL_free(s_Instance->m_Clipboard);
    return (s_Instance->m_Clipboard = SDL_GetClipboardText());
}

namespace {

void DisplayCriticalMessage(const char* message)
{
#if defined HK_OS_WIN32
    wchar_t wstr[1024];
    MultiByteToWideChar(CP_UTF8, 0, message, -1, wstr, HK_ARRAY_SIZE(wstr));
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
    data.message = message;
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

void CoreApplication::_TerminateWithError(const char* message)
{
    DisplayCriticalMessage(message);

    MemoryHeap::MemoryCleanup();

    Cleanup();

    std::quick_exit(0);
}

#ifdef HK_ALLOW_ASSERTS

void AssertFunction(const char* _File, int _Line, const char* _Function, const char* _Assertion, const char* _Comment)
{
    static thread_local bool bRecursive = false;

    if (bRecursive)
        return;

    bRecursive = true;

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

    bRecursive = false;
}

#endif

HK_NAMESPACE_END
