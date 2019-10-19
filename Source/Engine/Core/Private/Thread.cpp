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

#include <Engine/Core/Public/Thread.h>
#include <Engine/Core/Public/Std.h>

#ifdef AN_OS_WIN32
#include <Engine/Core/Public/WindowsDefs.h>
#include <process.h>
#endif

#include <thread>
#include <chrono>

const int FThread::NumHardwareThreads = FStdThread::hardware_concurrency();

void FThread::Start() {
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

void FThread::Join() {
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

size_t FThread::ThisThreadId() {
#ifdef AN_OS_WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

#ifdef AN_OS_WIN32
unsigned __stdcall FThread::StartRoutine( void * _Thread ) {
    static_cast< FThread * >( _Thread )->Routine( static_cast< FThread * >( _Thread )->Data );
    _endthreadex( 0 );
    return 0;
}
#else
void * FThread::StartRoutine( void * _Thread ) {
    static_cast< FThread * >( _Thread )->Routine( static_cast< FThread * >( _Thread )->Data );
    return 0;
}
#endif

#ifdef AN_OS_WIN32
constexpr int INTERNAL_SIZEOF = sizeof( CRITICAL_SECTION );
#endif

FThreadSync::FThreadSync() {
#ifdef AN_OS_WIN32
    static_assert( sizeof( Internal ) == INTERNAL_SIZEOF, "Critical section sizeof check" );
    InitializeCriticalSection( ( CRITICAL_SECTION * )&Internal[0] );
#endif
}

FThreadSync::~FThreadSync() {
#ifdef AN_OS_WIN32
    DeleteCriticalSection( (CRITICAL_SECTION *)&Internal[0] );
#endif
}

void FThreadSync::BeginScope() {
#ifdef AN_OS_WIN32
    EnterCriticalSection( (CRITICAL_SECTION *)&Internal[0] );
#else
    pthread_mutex_lock( &Internal );
#endif
}

bool FThreadSync::TryBeginScope() {
#ifdef AN_OS_WIN32
    return TryEnterCriticalSection( (CRITICAL_SECTION *)&Internal[0] ) != FALSE;
#else
    return !pthread_mutex_trylock( &Internal );
#endif
}

void FThreadSync::EndScope() {
#ifdef AN_OS_WIN32
    LeaveCriticalSection( (CRITICAL_SECTION *)&Internal[0] );
#else
    pthread_mutex_unlock( &Internal );
#endif
}

#ifdef AN_OS_WIN32
void FSyncEvent::CreateEventWIN32() {
    Internal = CreateEvent( NULL, FALSE, FALSE, NULL );
}

void FSyncEvent::DestroyEventWIN32() {
    CloseHandle( Internal );
}

void FSyncEvent::WaitWIN32() {
    WaitForSingleObject( Internal, INFINITE );
}

void FSyncEvent::SingalWIN32() {
    SetEvent( Internal );
}
#endif

void FSyncEvent::WaitTimeout( int _Milliseconds, bool & _TimedOut ) {
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

    FSyncGuard syncGuard( Sync );
    while ( !bSignaled ) {
        if ( pthread_cond_timedwait( &Internal, &Sync.Internal, &ts ) == ETIMEDOUT ) {
            _TimedOut = true;
            return;
        }
    }
    bSignaled = false;
#endif
}
