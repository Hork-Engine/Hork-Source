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

#include "Atomic.h"

/**

AThread

Thread.

*/
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


/**

AMutex

Thread mutex.

*/
class AMutex final {
    AN_FORBID_COPY( AMutex )

    friend class ASyncEvent;

public:
    AMutex();
    ~AMutex();

    void Lock();

    bool TryLock();

    void Unlock();

private:
#ifdef AN_OS_WIN32
    byte Internal[ 40 ];
#else
    pthread_mutex_t Internal = PTHREAD_MUTEX_INITIALIZER;
#endif
};


AN_FORCEINLINE void YieldCPU() // Unfortunately, the name Yield is already used by the macro defined in WinBase.h.
{
#if defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
    /* x86/x64 */
    #if (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__DMC__)) && !defined(__clang__)
        #if _MSC_VER >= 1400
            _mm_pause();
        #else
            #if defined(__DMC__)
                /* Digital Mars does not recognize the PAUSE opcode. Fall back to NOP. */
                __asm nop;
            #else
                __asm pause;
            #endif
        #endif
    #else
        __asm__ __volatile__ ("pause");
    #endif
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 7) || (defined(_M_ARM) && _M_ARM >= 7) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6T2__)
    /* ARM */
    #if defined(_MSC_VER)
        /* Apparently there is a __yield() intrinsic that's compatible with ARM, but I cannot find documentation for it nor can I find where it's declared. */
        __yield();
    #else
        __asm__ __volatile__ ("yield"); /* ARMv6K/ARMv6T2 and above. */
    #endif
#else
    /* Unknown or unsupported architecture. No-op. */
#endif
}


/**

ASpinLock

*/
class ASpinLock final {
    AN_FORBID_COPY( ASpinLock )

public:
    ASpinLock() : LockVar( false ) {}

    AN_FORCEINLINE void Lock() noexcept
    {
        // https://rigtorp.se/spinlock/

        for ( ;; ) {
            // Optimistically assume the lock is free on the first try
            if ( !LockVar.Exchange( true ) ) {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while ( LockVar.LoadRelaxed() ) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between hyper-threads
                YieldCPU();
            }
        }
    }

    AN_FORCEINLINE bool TryLock() noexcept
    {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!TryLock())
        return !LockVar.LoadRelaxed() && !LockVar.Exchange( true );
    }

    AN_FORCEINLINE void Unlock() noexcept
    {
        LockVar.Store( false );
    }

private:
    AAtomicBool LockVar;
};


/**

TLockGuard

Controls a synchronization primitive ownership within a scope, releasing
ownership in the destructor.

*/
template< typename T >
class TLockGuard
{
    AN_FORBID_COPY( TLockGuard )
public:
    AN_FORCEINLINE explicit TLockGuard( T & _Primitive )
      : Primitive( _Primitive )
    {
        Primitive.Lock();
    }

    AN_FORCEINLINE ~TLockGuard() {
        Primitive.Unlock();
    }
private:
    T & Primitive;
};


/**

TLockGuardCond

Controls a synchronization primitive ownership within a scope, releasing
ownership in the destructor. Checks condition.

*/
template< typename T >
class TLockGuardCond
{
    AN_FORBID_COPY( TLockGuardCond )
public:
    AN_FORCEINLINE explicit TLockGuardCond( T & _Primitive, const bool _Cond = true )
      : Primitive( _Primitive )
      , Cond( _Cond )
    {
        if ( Cond ) {
            Primitive.Lock();
        }
    }

    AN_FORCEINLINE ~TLockGuardCond() {
        if ( Cond ) {
            Primitive.Unlock();
        }
    }
private:
    T & Primitive;
    const bool Cond;
};


using AMutexGurad = TLockGuard< AMutex >;
using ASpinLockGuard = TLockGuard< ASpinLock >;


/**

ASyncEvent

Thread event.

*/
class ASyncEvent final {
    AN_FORBID_COPY( ASyncEvent )

public:
    ASyncEvent();
    ~ASyncEvent();

    /** Waits until the event is in the signaled state. */
    void Wait();

    /** Waits until the event is in the signaled state or the timeout interval elapses. */
    void WaitTimeout( int _Milliseconds, bool & _TimedOut );

    /** Set event to the signaled state. */
    void Signal();

private:
#ifdef AN_OS_WIN32
    void CreateEventWIN32();
    void DestroyEventWIN32();
    void WaitWIN32();
    void SingalWIN32();
    void * Internal;
#else
    AMutex Sync;
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
    AMutexGurad syncGuard( Sync );
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
        AMutexGurad syncGuard( Sync );
        bSignaled = true;
    }
    pthread_cond_signal( &Internal );
#endif
}
