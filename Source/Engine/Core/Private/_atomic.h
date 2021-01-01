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

#if !defined( AAtomicClass ) || !defined( AAtomicClass_N )
#error "Do not include this file directly."
#endif

AAtomicClass::AtomicType AAtomicClass::LoadRelaxed() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return i;
#else
    return i.load( std::memory_order_relaxed );
#endif
}

void AAtomicClass::StoreRelaxed( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    i = _i;
#else
    i.store( _i, std::memory_order_relaxed );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Load() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AtomicLoadAcquire( &i );
#else
    return i.load( std::memory_order_acquire );
#endif
}

void AAtomicClass::Store( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicStoreRelease( &i, _i );
#else
    i.store( _i, std::memory_order_release );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Increment() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedIncrementAcquire )( &i );
#else
    return i.fetch_add( 1, std::memory_order_acquire ) + 1;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchIncrement() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedIncrementAcquire )( &i ) - 1;
#else
    return i.fetch_add( 1, std::memory_order_acquire );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Decrement() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedDecrementRelease )( &i );
#else
    return i.fetch_sub( 1, std::memory_order_release ) - 1;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchDecrement() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedDecrementRelease )( &i ) + 1;
#else
    return i.fetch_sub( 1, std::memory_order_release );
#endif
}

#ifdef AAtomicClass_SupportAdd
AAtomicClass::AtomicType AAtomicClass::Add( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedAdd )( &i, _i );
#else
    return i.fetch_add( _i ) + _i;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchAdd( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedExchangeAdd )( &i, _i );
#else
    return i.fetch_add( _i );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Sub( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedExchangeSubtract )( &i, _i ) - _i;
#else
    return i.fetch_sub( _i ) - _i;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchSub( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedExchangeSubtract )( &i, _i );
#else
    return i.fetch_sub( _i );
#endif
}
#endif // AAtomicClass_SupportAdd

AAtomicClass::AtomicType AAtomicClass::And( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedAnd )( &i, _i ) & _i;
#else
    return i.fetch_and( _i ) & _i;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchAnd( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedAnd )( &i, _i );
#else
    return i.fetch_and( _i );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Or( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedOr )( &i, _i ) | _i;
#else
    return i.fetch_or( _i ) | _i;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchOr( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedOr )( &i, _i );
#else
    return i.fetch_or( _i );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Xor( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedXor )( &i, _i ) ^ _i;
#else
    return i.fetch_xor( _i ) ^ _i;
#endif
}

AAtomicClass::AtomicType AAtomicClass::FetchXor( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedXor )( &i, _i );
#else
    return i.fetch_xor( _i );
#endif
}

AAtomicClass::AtomicType AAtomicClass::Exchange( AtomicType _Exchange ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedExchange )( &i, _Exchange );
#else
    return i.exchange( _Exchange );
#endif
}

bool AAtomicClass::CompareExchange( AtomicType _Expected, AtomicType _Desired ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AAtomicClass_N( InterlockedCompareExchange )( &i, _Desired, _Expected ) == _Expected;
#else
    // FIXME: strong or weak?
    return i.compare_exchange_strong( _Expected, _Desired );
#endif
}
