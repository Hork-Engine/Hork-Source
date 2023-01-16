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

#include <Platform/Thread.h>

#ifdef HK_OS_WIN32
#    include <Platform/WindowsDefs.h>
#    include <process.h>
#endif

#include <thread>
#include <chrono>

HK_NAMESPACE_BEGIN

const int Thread::NumHardwareThreads = std::thread::hardware_concurrency();

void Thread::CreateThread(InvokerReturnType (*ThreadProc)(void*), void* pThreadData)
{
#ifdef HK_OS_WIN32
    unsigned threadId;
    m_Internal = (HANDLE)_beginthreadex(
        NULL,        // security
        0,           // stack size
        ThreadProc,  // proc
        pThreadData, // arg
        0,           // init flag
        &threadId);  // thread address
#else
    pthread_create(&m_Internal, nullptr, ThreadProc, threadProc);
    HK_UNUSED(IdealProcessor);
#endif
}

void Thread::EndThread()
{
#ifdef HK_OS_WIN32
    _endthreadex(0);
#endif
}

void Thread::Join()
{
    if (!m_Internal)
    {
        return;
    }

#ifdef HK_OS_WIN32
    WaitForSingleObject(m_Internal, INFINITE);
    CloseHandle(m_Internal);
    m_Internal = nullptr;
#else
    pthread_join(m_Internal, NULL);
    m_Internal = 0;
#endif
}

size_t Thread::ThisThreadId()
{
#ifdef HK_OS_WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

#ifdef HK_OS_WIN32

struct WaitableTimer
{
    HANDLE Handle = nullptr;
    ~WaitableTimer()
    {
        if (Handle)
        {
            CloseHandle(Handle);
        }
    }
};

static thread_local WaitableTimer WaitableTimer;

static void WaitMicrosecondsWIN32(int _Microseconds)
{
#    if 0
    std::this_thread::sleep_for( std::chrono::microseconds( _Microseconds ) );
#    else
    LARGE_INTEGER WaitTime;

    WaitTime.QuadPart = -10 * _Microseconds;

    if (!WaitableTimer.Handle)
    {
        WaitableTimer.Handle = CreateWaitableTimer(NULL, TRUE, NULL);
    }

    SetWaitableTimer(WaitableTimer.Handle, &WaitTime, 0, NULL, NULL, FALSE);
    WaitForSingleObject(WaitableTimer.Handle, INFINITE);
#    endif
}

#endif

void Thread::WaitSeconds(int _Seconds)
{
#ifdef HK_OS_WIN32
    //std::this_thread::sleep_for( std::chrono::seconds( _Seconds ) );
    WaitMicrosecondsWIN32(_Seconds * 1000000);
#else
    struct timespec ts = {_Seconds, 0};
    while (::nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
#endif
}

void Thread::WaitMilliseconds(int _Milliseconds)
{
#ifdef HK_OS_WIN32
    //std::this_thread::sleep_for( std::chrono::milliseconds( _Milliseconds ) );
    WaitMicrosecondsWIN32(_Milliseconds * 1000);
#else
    int64_t         seconds     = _Milliseconds / 1000;
    int64_t         nanoseconds = (_Milliseconds - seconds * 1000) * 1000000;
    struct timespec ts          = {
        seconds,
        nanoseconds};
    while (::nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
#endif
}

void Thread::WaitMicroseconds(int _Microseconds)
{
#ifdef HK_OS_WIN32
    //std::this_thread::sleep_for( std::chrono::microseconds( _Microseconds ) );
    WaitMicrosecondsWIN32(_Microseconds);
#else
    int64_t         seconds     = _Microseconds / 1000000;
    int64_t         nanoseconds = (_Microseconds - seconds * 1000000) * 1000;
    struct timespec ts          = {
        seconds,
        nanoseconds};
    while (::nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
#endif
}

#ifdef HK_OS_WIN32
constexpr int INTERNAL_SIZEOF = sizeof(CRITICAL_SECTION);
#endif

Mutex::Mutex()
{
#ifdef HK_OS_WIN32
    HK_VALIDATE_TYPE_SIZE(m_Internal, INTERNAL_SIZEOF);
    InitializeCriticalSection((CRITICAL_SECTION*)&m_Internal[0]);
#endif
}

Mutex::~Mutex()
{
#ifdef HK_OS_WIN32
    DeleteCriticalSection((CRITICAL_SECTION*)&m_Internal[0]);
#endif
}

void Mutex::Lock()
{
#ifdef HK_OS_WIN32
    EnterCriticalSection((CRITICAL_SECTION*)&m_Internal[0]);
#else
    pthread_mutex_lock(&m_Internal);
#endif
}

bool Mutex::TryLock()
{
#ifdef HK_OS_WIN32
    return TryEnterCriticalSection((CRITICAL_SECTION*)&m_Internal[0]) != FALSE;
#else
    return pthread_mutex_trylock(&m_Internal) == 0;
#endif
}

void Mutex::Unlock()
{
#ifdef HK_OS_WIN32
    LeaveCriticalSection((CRITICAL_SECTION*)&m_Internal[0]);
#else
    pthread_mutex_unlock(&m_Internal);
#endif
}

#ifdef HK_OS_WIN32
void SyncEvent::CreateEventWIN32()
{
    m_Internal = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void SyncEvent::DestroyEventWIN32()
{
    CloseHandle(m_Internal);
}

void SyncEvent::WaitWIN32()
{
    WaitForSingleObject(m_Internal, INFINITE);
}

void SyncEvent::SingalWIN32()
{
    SetEvent(m_Internal);
}
#endif

void SyncEvent::WaitTimeout(int _Milliseconds, bool& _TimedOut)
{
    _TimedOut = false;

#ifdef HK_OS_WIN32
    if (WaitForSingleObject(m_Internal, _Milliseconds) == WAIT_TIMEOUT)
    {
        _TimedOut = true;
    }
#else
    int64_t timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() + _Milliseconds;

    int64_t seconds     = timeStamp / 1000;
    int64_t nanoseconds = (timeStamp - seconds * 1000) * 1000000;

    struct timespec ts = {
        seconds,
        nanoseconds};

    MutexGurad syncGuard(m_Sync);
    while (!m_bSignaled)
    {
        if (pthread_cond_timedwait(&m_Internal, &m_Sync.m_Internal, &ts) == ETIMEDOUT)
        {
            _TimedOut = true;
            return;
        }
    }
    m_bSignaled = false;
#endif
}

HK_NAMESPACE_END
