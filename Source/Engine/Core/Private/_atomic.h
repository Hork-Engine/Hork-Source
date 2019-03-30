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

#if !defined( FAtomicClass ) || !defined( FAtomicClass_N )
#error "Do not include this file directly."
#endif

FAtomicClass::AtomicType FAtomicClass::LoadRelaxed() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return i;
#else
    return i.load( std::memory_order_relaxed );
#endif
}

void FAtomicClass::StoreRelaxed( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    i = _i;
#else
    i.store( _i, std::memory_order_relaxed );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Load() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AtomicLoadAcquire( &i );
#else
    return i.load( std::memory_order_acquire );
#endif
}

void FAtomicClass::Store( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicStoreRelease( &i, _i );
#else
    i.store( _i, std::memory_order_release );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Increment() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedIncrementAcquire )( &i );
#else
    return i.fetch_add( 1, std::memory_order_acquire ) + 1;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchIncrement() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedIncrementAcquire )( &i ) - 1;
#else
    return i.fetch_add( 1, std::memory_order_acquire );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Decrement() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedDecrementRelease )( &i );
#else
    return i.fetch_sub( 1, std::memory_order_release ) - 1;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchDecrement() {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedDecrementRelease )( &i ) + 1;
#else
    return i.fetch_sub( 1, std::memory_order_release );
#endif
}

#ifdef FAtomicClass_SupportAdd
FAtomicClass::AtomicType FAtomicClass::Add( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedAdd )( &i, _i );
#else
    return i.fetch_add( _i ) + _i;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchAdd( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedExchangeAdd )( &i, _i );
#else
    return i.fetch_add( _i );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Sub( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedExchangeSubtract )( &i, _i ) - _i;
#else
    return i.fetch_sub( _i ) - _i;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchSub( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedExchangeSubtract )( &i, _i );
#else
    return i.fetch_sub( _i );
#endif
}
#endif // FAtomicClass_SupportAdd

FAtomicClass::AtomicType FAtomicClass::And( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedAnd )( &i, _i ) & _i;
#else
    return i.fetch_and( _i ) & _i;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchAnd( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedAnd )( &i, _i );
#else
    return i.fetch_and( _i );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Or( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedOr )( &i, _i ) | _i;
#else
    return i.fetch_or( _i ) | _i;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchOr( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedOr )( &i, _i );
#else
    return i.fetch_or( _i );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Xor( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedXor )( &i, _i ) ^ _i;
#else
    return i.fetch_xor( _i ) ^ _i;
#endif
}

FAtomicClass::AtomicType FAtomicClass::FetchXor( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedXor )( &i, _i );
#else
    return i.fetch_xor( _i );
#endif
}

FAtomicClass::AtomicType FAtomicClass::Exchange( AtomicType _Exchange ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedExchange )( &i, _Exchange );
#else
    return i.exchange( _Exchange );
#endif
}

bool FAtomicClass::CompareExchange( AtomicType _Expected, AtomicType _Desired ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return FAtomicClass_N( InterlockedCompareExchange )( &i, _Desired, _Expected ) == _Expected;
#else
    // FIXME: strong or weak?
    return i.compare_exchange_strong( _Expected, _Desired );
#endif
}
