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

#include <Core/Public/Thread.h>
#include <Core/Public/Std.h>

#ifdef AN_OS_WIN32
#include <Core/Public/WindowsDefs.h>
#include <process.h>
#endif

#include <thread>
#include <chrono>

const int AThread::NumHardwareThreads = AStdThread::hardware_concurrency();

void AThread::Start() {
#ifdef AN_OS_WIN32
    unsigned threadId;
    Internal = (HANDLE)_beginthreadex(
        NULL,            // security
        0,               // stack size
        &StartRoutine,   // callback address
        this,            // arg
        0,               // init flag
        &threadId );     // thread address

    //SetThreadPriority( Internal, THREAD_PRIORITY_HIGHEST );
#else
    pthread_create( &Internal, NULL, &StartRoutine, this );
#endif
}

void AThread::Join() {
    if ( !Internal ) {
        return;
    }

#ifdef AN_OS_WIN32
    WaitForSingleObject( Internal, INFINITE );
    CloseHandle( Internal );
    Internal = nullptr;
#else
    pthread_join( Internal, NULL );
    Internal = 0;
#endif
}

size_t AThread::ThisThreadId() {
#ifdef AN_OS_WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

#ifdef AN_OS_WIN32
unsigned __stdcall AThread::StartRoutine( void * _Thread ) {
    static_cast< AThread * >( _Thread )->Routine( static_cast< AThread * >( _Thread )->Data );
    _endthreadex( 0 );
    return 0;
}
#else
void * AThread::StartRoutine( void * _Thread ) {
    static_cast< AThread * >( _Thread )->Routine( static_cast< AThread * >( _Thread )->Data );
    return 0;
}
#endif

#ifdef AN_OS_WIN32
constexpr int INTERNAL_SIZEOF = sizeof( CRITICAL_SECTION );
#endif

AMutex::AMutex() {
#ifdef AN_OS_WIN32
    AN_SIZEOF_STATIC_CHECK( Internal, INTERNAL_SIZEOF );
    InitializeCriticalSection( ( CRITICAL_SECTION * )&Internal[0] );
#endif
}

AMutex::~AMutex() {
#ifdef AN_OS_WIN32
    DeleteCriticalSection( (CRITICAL_SECTION *)&Internal[0] );
#endif
}

void AMutex::Lock() {
#ifdef AN_OS_WIN32
    EnterCriticalSection( (CRITICAL_SECTION *)&Internal[0] );
#else
    pthread_mutex_lock( &Internal );
#endif
}

bool AMutex::TryLock() {
#ifdef AN_OS_WIN32
    return TryEnterCriticalSection( (CRITICAL_SECTION *)&Internal[0] ) != FALSE;
#else
    return !pthread_mutex_trylock( &Internal );
#endif
}

void AMutex::Unlock() {
#ifdef AN_OS_WIN32
    LeaveCriticalSection( (CRITICAL_SECTION *)&Internal[0] );
#else
    pthread_mutex_unlock( &Internal );
#endif
}

#ifdef AN_OS_WIN32
void ASyncEvent::CreateEventWIN32() {
    Internal = CreateEvent( NULL, FALSE, FALSE, NULL );
}

void ASyncEvent::DestroyEventWIN32() {
    CloseHandle( Internal );
}

void ASyncEvent::WaitWIN32() {
    WaitForSingleObject( Internal, INFINITE );
}

void ASyncEvent::SingalWIN32() {
    SetEvent( Internal );
}
#endif

void ASyncEvent::WaitTimeout( int _Milliseconds, bool & _TimedOut ) {
    _TimedOut = false;

#ifdef AN_OS_WIN32
    if ( WaitForSingleObject( Internal, _Milliseconds ) == WAIT_TIMEOUT ) {
        _TimedOut = true;
    }
#else
    int64_t timeStamp = StdChrono::duration_cast< StdChrono::milliseconds >( StdChrono::high_resolution_clock::now().time_since_epoch() ).count()
            + _Milliseconds;

    int64_t seconds = timeStamp / 1000;
    int64_t nanoseconds = ( timeStamp - seconds * 1000 ) * 1000000;

    struct timespec ts = {
        seconds,
        nanoseconds
    };

    AMutexGurad syncGuard( Sync );
    while ( !bSignaled ) {
        if ( pthread_cond_timedwait( &Internal, &Sync.Internal, &ts ) == ETIMEDOUT ) {
            _TimedOut = true;
            return;
        }
    }
    bSignaled = false;
#endif
}
