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

#include <Core/Public/Atomic.h>

// Compiler barriers.
// See: http://en.wikipedia.org/wiki/Memory_ordering
#ifdef AN_COMPILER_MSVC

#include <intrin.h>

#pragma intrinsic(_WriteBarrier)
#define Sys_WriteBarrier            _WriteBarrier

#pragma intrinsic(_ReadWriteBarrier)
#define Sys_ReadWriteBarrier        _ReadWriteBarrier

#if _MSC_VER >= 1400            // ReadBarrier is VC2005
#pragma intrinsic(_ReadBarrier)
#define Sys_ReadBarrier             _ReadBarrier
#else
#define Sys_ReadBarrier             _ReadWriteBarrier
#endif

#elif AN_COMPILER_GCC

#define Sys_ReadWriteBarrier()      asm volatile("" ::: "memory");
#define Sys_WriteBarrier            Sys_ReadWriteBarrier
#define Sys_ReadBarrier             Sys_ReadWriteBarrier

#else//#elif NV_CC_CLANG && NV_CPU_ARM

#error "Unknown compiler"

#if 0
/* this is not yet supported by LLVM 2.1 but it is planned
#define Sys_ReadWriteBarrier()      MemoryFence()
 */

// JBeilin: from what i read this should do the trick for ARM
// however this might also be wrong and dumb.
//#define Sys_ReadWriteBarrier()    asm volatile( "dmb;");
#define Sys_ReadWriteBarrier()      Sys_ReadWriteBarrier()
#define Sys_WriteBarrier            Sys_ReadWriteBarrier
#define Sys_ReadBarrier             Sys_ReadWriteBarrier
#endif

#endif

#define InterlockedIncrementAcquire16_ANGIE( a ) InterlockedIncrementAcquire16( ( SHORT * )a )
#define InterlockedDecrementRelease16_ANGIE( a ) InterlockedDecrementRelease16( ( SHORT * )a )
//#define InterlockedExchangeAdd16_ANGIE( a, b ) InterlockedExchangeAdd16( ( SHORT * )a, b )
//#define InterlockedExchangeSubtract16_ANGIE( a, b ) InterlockedExchangeSubtract( ( SHORT * )a, b )
#define InterlockedAnd16_ANGIE( a, b ) InterlockedAnd16( ( SHORT * )a, b )
#define InterlockedOr16_ANGIE( a, b ) InterlockedOr16( ( SHORT * )a, b )
#define InterlockedXor16_ANGIE( a, b ) InterlockedXor16( ( SHORT * )a, b )
#define InterlockedExchange16_ANGIE( a, b ) InterlockedExchange16( ( SHORT * )a, b )
#define InterlockedCompareExchange16_ANGIE( a, b, c ) (USHORT)InterlockedCompareExchange16( ( SHORT * )a, b, c )

#define InterlockedIncrementAcquire32_ANGIE( a ) InterlockedIncrementAcquire( ( unsigned * )a )
#define InterlockedDecrementRelease32_ANGIE( a ) InterlockedDecrementRelease( ( unsigned * )a )
#define InterlockedAdd32_ANGIE( a, b ) InterlockedAdd( ( LONG * )a, b )
#define InterlockedExchangeAdd32_ANGIE( a, b ) InterlockedExchangeAdd( ( unsigned * )a, b )
#define InterlockedExchangeSubtract32_ANGIE( a, b ) InterlockedExchangeSubtract( ( unsigned * )a, b )
#define InterlockedAnd32_ANGIE( a, b ) InterlockedAnd( ( LONG * )a, b )
#define InterlockedOr32_ANGIE( a, b ) InterlockedOr( ( LONG * )a, b )
#define InterlockedXor32_ANGIE( a, b ) InterlockedXor( ( LONG * )a, b )
#define InterlockedExchange32_ANGIE( a, b ) InterlockedExchange( ( unsigned * )a, b )
#define InterlockedCompareExchange32_ANGIE( a, b, c ) ( int )InterlockedCompareExchange( ( unsigned * )a, b, c )

#define InterlockedIncrementAcquire64_ANGIE( a ) InterlockedIncrementAcquire64( ( LONGLONG * )a )
#define InterlockedDecrementRelease64_ANGIE( a ) InterlockedDecrementRelease64( ( LONGLONG * )a )
#define InterlockedAdd64_ANGIE( a, b ) InterlockedAdd64( ( LONGLONG * )a, b )
#define InterlockedExchangeAdd64_ANGIE( a, b ) InterlockedExchangeAdd64( ( LONGLONG * )a, b )
#define InterlockedExchangeSubtract64_ANGIE( a, b ) InterlockedExchangeSubtract( (uint64_t *)a, b )
#define InterlockedAnd64_ANGIE( a, b ) InterlockedAnd64( ( LONGLONG * )a, b )
#define InterlockedOr64_ANGIE( a, b ) InterlockedOr64( ( LONGLONG * )a, b )
#define InterlockedXor64_ANGIE( a, b ) InterlockedXor64( ( LONGLONG * )a, b )
#define InterlockedExchange64_ANGIE( a, b ) InterlockedExchange64( ( LONGLONG * )a, b )
#define InterlockedCompareExchange64_ANGIE( a, b, c ) InterlockedCompareExchange64( ( LONGLONG * )a, b, c )

#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC

#include <Core/Public/WindowsDefs.h>

// Macroses from posh.h
#if defined __X86__ || defined __i386__ || defined i386 || defined _M_IX86 || defined __386__ || defined __x86_64__ || defined _M_X64
#  define POSH_CPU_X86 1
#  if defined __x86_64__ || defined _M_X64
#     define POSH_CPU_X86_64 1
#  endif
#endif
#if defined ARM || defined __arm__ || defined _ARM
#  define POSH_CPU_STRONGARM 1
#endif
#if defined __aarch64__
#  define POSH_CPU_AARCH64 1
#endif
#if defined __PPC__ || defined __POWERPC__  || defined powerpc || defined _POWER || defined __ppc__ || defined __powerpc__ || defined _M_PPC
#  if !defined GEKKO && !defined mc68000 && !defined m68k && !defined __MC68K__ && !defined m68000
#    if defined __powerpc64__
#       define POSH_CPU_PPC64 1
#    endif
#  endif
#endif

template< typename T >
AN_INLINE T AtomicLoadAcquire( const volatile T * ptr ) {
    // This function based on Ignacio Castano code from nvidia texture tools library (public domain)
    //

    //assert((intptr_t(ptr) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
    T ret = *ptr;  // on x86, loads are Acquire
    Sys_ReadBarrier();
    return ret;
#elif POSH_CPU_STRONGARM || POSH_CPU_AARCH64
    // need more specific cpu type for armv7?
    // also utilizes a full barrier
    // currently treating load like x86 - this could be wrong

    // this is the easiest but slowest way to do this
    Sys_ReadWriteBarrier();
    T ret = *ptr; // replace with ldrex?
    Sys_ReadWriteBarrier();
    return ret;
#elif POSH_CPU_PPC64
    // need more specific cpu type for ppc64?
    // also utilizes a full barrier
    // currently treating load like x86 - this could be wrong

    // this is the easiest but slowest way to do this
    Sys_ReadWriteBarrier();
    T ret = *ptr; // replace with ldrex?
    Sys_ReadWriteBarrier();
    return ret;
#else
#error "Not implemented"
#endif
}

template< typename T >
AN_INLINE void AtomicStoreRelease( volatile T * ptr, T value ) {
    // This function based on Ignacio Castano code from nvidia texture tools library (public domain)
    //

    //assert((intptr_t(ptr) & 3) == 0);
    //assert((intptr_t(&value) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
    Sys_WriteBarrier();
    *ptr = value;   // on x86, stores are Release
                    //Sys_WriteBarrier(); // @@ IC: Where does this barrier go? In nvtt it was after, in Witness before. Not sure which one is right.
#elif POSH_CPU_STRONGARM || POSH_CPU_AARCH64
    // this is the easiest but slowest way to do this
    Sys_ReadWriteBarrier();
    *ptr = value; //strex?
    Sys_ReadWriteBarrier();
#elif POSH_CPU_PPC64
    // this is the easiest but slowest way to do this
    Sys_ReadWriteBarrier();
    *ptr = value; //strex?
    Sys_ReadWriteBarrier();
#else
#error "Atomics not implemented."
#endif
}

#endif

//#define AAtomicClass AAtomicBool
//#define AAtomicClass_N( _A ) _A##8_ANGIE
//#undef AAtomicClass_SupportAdd
//#include "_atomic.h"
//#undef AAtomicClass
//#undef AAtomicClass_N

AAtomicBool::AtomicType AAtomicBool::LoadRelaxed() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return i;
#else
    return i.load( std::memory_order_relaxed );
#endif
}

// Store relaxed
void AAtomicBool::StoreRelaxed( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    i = _i;
#else
    i.store( _i, std::memory_order_relaxed );
#endif
}

// Atomic load
AAtomicBool::AtomicType AAtomicBool::Load() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AtomicLoadAcquire( &i );
#else
    return i.load( std::memory_order_acquire );
#endif
}

// Atomic store
void AAtomicBool::Store( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicStoreRelease( &i, _i );
#else
    i.store( _i, std::memory_order_release );
#endif
}

AAtomicBool::AtomicType AAtomicBool::Exchange( AtomicType _Exchange ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return InterlockedExchange8( reinterpret_cast< char *>( &i ), _Exchange );
#else
    return i.exchange( _Exchange );
#endif
}

//bool AAtomicBool::CompareExchange( AtomicType _Exchange, AtomicType _Comparand ) {
//#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
//    return InterlockedCompareExchange8( &i, _Comparand, _Exchange ) == _Exchange; !!!
//#else
//    // FIXME: strong or weak?
//    return i.compare_exchange_strong( _Exchange, _Comparand );
//#endif
//}

#define AAtomicClass AAtomicShort
#define AAtomicClass_N( _A ) _A##16_ANGIE
#undef AAtomicClass_SupportAdd
#include "_atomic.h"
#undef AAtomicClass
#undef AAtomicClass_N

#define AAtomicClass AAtomicInt
#define AAtomicClass_N( _A ) _A##32_ANGIE
#define AAtomicClass_SupportAdd
#include "_atomic.h"
#undef AAtomicClass
#undef AAtomicClass_N

#define AAtomicClass AAtomicLong
#define AAtomicClass_N( _A ) _A##64_ANGIE
#include "_atomic.h"
#undef AAtomicClass
#undef AAtomicClass_N

#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC

volatile void * AtomicLoadAcquirePointer( volatile void *& _Ptr ) {
    //static_assert( sizeof( Type ) == sizeof( intptr_t ), "AtomicLoadAcquirePointer" );
    //assert((((intptr_t)ptr) % sizeof(intptr_t)) == 0);
    volatile void * Ret = _Ptr;   // on x86, loads are Acquire
    Sys_ReadBarrier();
    return Ret;
}

void AtomicStoreReleasePointer( volatile void *& _Dst, void * _Src ) {
    //static_assert( sizeof(Type) == sizeof(intptr_t), "AtomicStoreReleasePointer" );
    //assert((((intptr_t)pTo) % sizeof(intptr_t)) == 0);
    //assert((((intptr_t)&from) % sizeof(intptr_t)) == 0);
    Sys_WriteBarrier();
    _Dst = _Src;    // on x86, stores are Release
}

void * AtomicExchangePointer( void *& _Ptr, void * _Exchange ) {
    return InterlockedExchangePointer( &_Ptr, _Exchange );
}

void * AtomicCompareExchangePointer( void *& _Ptr, void * _Desired, void * _Expected ) {
    return InterlockedCompareExchangePointer( &_Ptr, _Desired, _Expected );
}

#endif
