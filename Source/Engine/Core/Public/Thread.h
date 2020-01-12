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

#pragma once

#include "BaseTypes.h"

class AThread final {
    AN_FORBID_COPY( AThread )

public:
    static const int NumHardwareThreads;

    AThread() {

    }

    ~AThread() {
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

AThreadSync

Thread mutex.

*/
class AThreadSync final {
    AN_FORBID_COPY( AThreadSync )

    friend class ASyncEvent;
public:

    AThreadSync();
    ~AThreadSync();

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

ASyncGuard

A ASyncGuard controls mutex ownership within a scope, releasing
ownership in the destructor.

*/
class ASyncGuard final {
    AN_FORBID_COPY( ASyncGuard )
public:
    explicit ASyncGuard( AThreadSync & _Sync );
    ~ASyncGuard();
private:
    AThreadSync & Sync;
};

AN_FORCEINLINE ASyncGuard::ASyncGuard( AThreadSync & _Sync )
  : Sync( _Sync )
{
    Sync.BeginScope();
}

AN_FORCEINLINE ASyncGuard::~ASyncGuard() {
    Sync.EndScope();
}

/*

ASyncGuardCond

A ASyncGuard controls mutex ownership within a scope, releasing
ownership in the destructor. Checks condition.

*/
class ASyncGuardCond final {
    AN_FORBID_COPY( ASyncGuardCond )
public:
    explicit ASyncGuardCond( AThreadSync & _Sync, const bool _Cond = true );
    ~ASyncGuardCond();
private:
    AThreadSync & Sync;
    const bool Cond;
};

AN_FORCEINLINE ASyncGuardCond::ASyncGuardCond( AThreadSync & _Sync, const bool _Cond )
  : Sync( _Sync )
  , Cond( _Cond )
{
    if ( Cond ) {
        Sync.BeginScope();
    }
}

AN_FORCEINLINE ASyncGuardCond::~ASyncGuardCond() {
    if ( Cond ) {
        Sync.EndScope();
    }
}

/*

ASyncEvent

Thread event.

*/
class ASyncEvent final {
    AN_FORBID_COPY( ASyncEvent )
public:

    ASyncEvent();
    ~ASyncEvent();

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
    AThreadSync Sync;
    pthread_cond_t Internal = PTHREAD_COND_INITIALIZER;
    bool bSignaled;
#endif
};

AN_FORCEINLINE ASyncEvent::ASyncEvent() {
#ifdef AN_OS_WIN32
    CreateEventWIN32();
#else
    bSignaled = false;
#endif
}

AN_FORCEINLINE ASyncEvent::~ASyncEvent() {
#ifdef AN_OS_WIN32
    DestroyEventWIN32();
#endif
}

AN_FORCEINLINE void ASyncEvent::Wait() {
#ifdef AN_OS_WIN32
    WaitWIN32();
#else
    ASyncGuard syncGuard( Sync );
    while ( !bSignaled ) {
        pthread_cond_wait( &Internal, &Sync.Internal );
    }
    bSignaled = false;
#endif
}

AN_FORCEINLINE void ASyncEvent::Signal() {
#ifdef AN_OS_WIN32
    SingalWIN32();
#else
    {
        ASyncGuard syncGuard( Sync );
        bSignaled = true;
    }
    pthread_cond_signal( &Internal );
#endif
}
