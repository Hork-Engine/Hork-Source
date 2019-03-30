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

#include "BaseTypes.h"

class FThread final {
    AN_FORBID_COPY( FThread )

public:
    static const int NumHardwareThreads;

    FThread() {

    }

    ~FThread() {
        Join();
    }

    void ( *Routine )( void * _Data ) = EmptyRoutine;
    void * Data;

    void Start();

    void Join();

    static size_t ThisThreadId();

private:
#ifdef AN_OS_WIN32
    static unsigned __stdcall StartRoutine( void * _Thread );
    void * Internal = nullptr;
#else
    static void *StartRoutine( void * _Thread );
    pthread_t Internal = 0;
#endif
    static void EmptyRoutine( void * ) {}
};

/*

FThreadSync

Thread mutex.

*/
class FThreadSync final {
    AN_FORBID_COPY( FThreadSync )

    friend class FSyncEvent;
public:

    FThreadSync();
    ~FThreadSync();

    void BeginScope();

    bool TryBeginScope();

    void EndScope();

private:
#ifdef AN_OS_WIN32
    byte Internal[ 40 ];
#else
    pthread_mutex_t Internal = PTHREAD_MUTEX_INITIALIZER;
#endif
};

/*

FSyncGuard

A FSyncGuard controls mutex ownership within a scope, releasing
ownership in the destructor.

*/
class FSyncGuard final {
    AN_FORBID_COPY( FSyncGuard )
public:
    explicit FSyncGuard( FThreadSync & _Sync );
    ~FSyncGuard();
private:
    FThreadSync & Sync;
};

AN_FORCEINLINE FSyncGuard::FSyncGuard( FThreadSync & _Sync )
  : Sync( _Sync )
{
    Sync.BeginScope();
}

AN_FORCEINLINE FSyncGuard::~FSyncGuard() {
    Sync.EndScope();
}

/*

FSyncGuardCond

A FSyncGuard controls mutex ownership within a scope, releasing
ownership in the destructor. Checks condition.

*/
class FSyncGuardCond final {
    AN_FORBID_COPY( FSyncGuardCond )
public:
    explicit FSyncGuardCond( FThreadSync & _Sync, const bool _Cond = true );
    ~FSyncGuardCond();
private:
    FThreadSync & Sync;
    const bool Cond;
};

AN_FORCEINLINE FSyncGuardCond::FSyncGuardCond( FThreadSync & _Sync, const bool _Cond )
  : Sync( _Sync )
  , Cond( _Cond )
{
    if ( Cond ) {
        Sync.BeginScope();
    }
}

AN_FORCEINLINE FSyncGuardCond::~FSyncGuardCond() {
    if ( Cond ) {
        Sync.EndScope();
    }
}

/*

FSyncEvent

Thread event.

*/
class FSyncEvent final {
    AN_FORBID_COPY( FSyncEvent )
public:

    FSyncEvent();
    ~FSyncEvent();

    // Waits until the event is in the signaled state.
    void Wait();

    // Waits until the event is in the signaled state or the timeout interval elapses.
    void WaitTimeout( int _Milliseconds, bool & _TimedOut );

    // Set event to the signaled state.
    void Signal();

private:
#ifdef AN_OS_WIN32
    void CreateEventWIN32();
    void DestroyEventWIN32();
    void WaitWIN32();
    void SingalWIN32();
    void * Internal;
#else
    FThreadSync Sync;
    pthread_cond_t Internal = PTHREAD_COND_INITIALIZER;
    bool bSignaled;
#endif
};

AN_FORCEINLINE FSyncEvent::FSyncEvent() {
#ifdef AN_OS_WIN32
    CreateEventWIN32();
#else
    bSignaled = false;
#endif
}

AN_FORCEINLINE FSyncEvent::~FSyncEvent() {
#ifdef AN_OS_WIN32
    DestroyEventWIN32();
#endif
}

AN_FORCEINLINE void FSyncEvent::Wait() {
#ifdef AN_OS_WIN32
    WaitWIN32();
#else
    FSyncGuard syncGuard( Sync );
    while ( !bSignaled ) {
        pthread_cond_wait( &Internal, &Sync.Internal );
    }
    bSignaled = false;
#endif
}

AN_FORCEINLINE void FSyncEvent::Signal() {
#ifdef AN_OS_WIN32
    SingalWIN32();
#else
    {
        FSyncGuard syncGuard( Sync );
        bSignaled = true;
    }
    pthread_cond_signal( &Internal );
#endif
}
