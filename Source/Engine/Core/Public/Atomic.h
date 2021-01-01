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

#include "BaseTypes.h"

// Define AN_STD_ATOMIC to force using std::atomic
#define AN_STD_ATOMIC

#if !defined AN_OS_WIN32 || defined AN_STD_ATOMIC
#include <atomic>
#endif

class AAtomicBool {
    AN_FORBID_COPY( AAtomicBool )

public:
    typedef bool AtomicType;

    AAtomicBool() {}
    explicit AAtomicBool( const AtomicType & _i ) : i( _i ) {}

    // Load relaxed
    AtomicType LoadRelaxed() const;

    // Store relaxed
    void StoreRelaxed( AtomicType _i );

    // Atomic load
    AtomicType Load() const;

    // Atomic store
    void Store( AtomicType _i );

    AtomicType Exchange( AtomicType _Exchange );

    //bool CompareExchange( AtomicType _Exchange, AtomicType _Comparand );

private:
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicType i;
#else
    std::atomic_bool i;
#endif
};

class AAtomicShort {
    AN_FORBID_COPY( AAtomicShort )

public:
    typedef int16_t AtomicType;

    AAtomicShort() {}
    explicit AAtomicShort( const AtomicType & _i ) : i(_i) {}

    // Load relaxed
    AtomicType LoadRelaxed() const;

    // Store relaxed
    void StoreRelaxed( AtomicType _i );

    // Atomic load
    AtomicType Load() const;

    // Atomic store
    void Store( AtomicType _i );

    // Increment, return incremented value
    AtomicType Increment();

    // Increment, return initial value
    AtomicType FetchIncrement();

    // Decrement, return decremented value
    AtomicType Decrement();

    // Decrement, return initial value
    AtomicType FetchDecrement();

    // And, return resulting value
    AtomicType And( AtomicType _i );

    // And, return initial value
    AtomicType FetchAnd( AtomicType _i );

    // Or, return resulting value
    AtomicType Or( AtomicType _i );

    // Or, return initial value
    AtomicType FetchOr( AtomicType _i );

    // Xor, return resulting value
    AtomicType Xor( AtomicType _i );

    // Xor, return initial value
    AtomicType FetchXor( AtomicType _i );

    AtomicType Exchange( AtomicType _Exchange );

    bool CompareExchange( AtomicType _Exchange, AtomicType _Comparand );

private:
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicType i;
#else
    //std::atomic_uint16_t i;
    std::atomic_short i;
#endif
};

class AAtomicInt {
    AN_FORBID_COPY( AAtomicInt )

public:
    typedef int32_t AtomicType;

    AAtomicInt() {}
    explicit AAtomicInt( const AtomicType & _i ) : i(_i) {}

    // Load relaxed
    AtomicType LoadRelaxed() const;

    // Store relaxed
    void StoreRelaxed( AtomicType _i );

    // Atomic load
    AtomicType Load() const;

    // Atomic store
    void Store( AtomicType _i );

    // Increment, return incremented value
    AtomicType Increment();

    // Increment, return initial value
    AtomicType FetchIncrement();

    // Decrement, return decremented value
    AtomicType Decrement();

    // Decrement, return initial value
    AtomicType FetchDecrement();

    // Add, return resulting value
    AtomicType Add( AtomicType _i );

    // Add, return initial value
    AtomicType FetchAdd( AtomicType _i );

    // Subtract, return resulting value
    AtomicType Sub( AtomicType _i );

    // Subtract, return initial value
    AtomicType FetchSub( AtomicType _i );

    // And, return resulting value
    AtomicType And( AtomicType _i );

    // And, return initial value
    AtomicType FetchAnd( AtomicType _i );

    // Or, return resulting value
    AtomicType Or( AtomicType _i );

    // Or, return initial value
    AtomicType FetchOr( AtomicType _i );

    // Xor, return resulting value
    AtomicType Xor( AtomicType _i );

    // Xor, return initial value
    AtomicType FetchXor( AtomicType _i );

    AtomicType Exchange( AtomicType _Exchange );

    bool CompareExchange( AtomicType _Exchange, AtomicType _Comparand );

private:
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicType i;
#else
    std::atomic< int32_t > i;
#endif
};

class AAtomicLong {
    AN_FORBID_COPY( AAtomicLong )

public:
    typedef int64_t AtomicType;

    AAtomicLong() {}
    explicit AAtomicLong( const AtomicType & _i ) : i(_i) {}

    // Load relaxed
    AtomicType LoadRelaxed() const;

    // Store relaxed
    void StoreRelaxed( AtomicType _i );

    // Atomic load
    AtomicType Load() const;

    // Atomic store
    void Store( AtomicType _i );

    // Increment, return incremented value
    AtomicType Increment();

    // Increment, return initial value
    AtomicType FetchIncrement();

    // Decrement, return decremented value
    AtomicType Decrement();

    // Decrement, return initial value
    AtomicType FetchDecrement();

    // Add, return resulting value
    AtomicType Add( AtomicType _i );

    // Add, return initial value
    AtomicType FetchAdd( AtomicType _i );

    // Subtract, return resulting value
    AtomicType Sub( AtomicType _i );

    // Subtract, return initial value
    AtomicType FetchSub( AtomicType _i );

    // And, return resulting value
    AtomicType And( AtomicType _i );

    // And, return initial value
    AtomicType FetchAnd( AtomicType _i );

    // Or, return resulting value
    AtomicType Or( AtomicType _i );

    // Or, return initial value
    AtomicType FetchOr( AtomicType _i );

    // Xor, return resulting value
    AtomicType Xor( AtomicType _i );

    // Xor, return initial value
    AtomicType FetchXor( AtomicType _i );

    AtomicType Exchange( AtomicType _Exchange );

    bool CompareExchange( AtomicType _Exchange, AtomicType _Comparand );

private:
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicType i;
#else
    std::atomic< int64_t > i;
#endif
};

template< typename Type >
class TAtomicPtr {
public:
    typedef Type * AtomicType;

    // Load relaxed
    AtomicType LoadRelaxed() const;

    // Store relaxed
    void StoreRelaxed( AtomicType _i );

    // Atomic load
    AtomicType Load() const;

    // Atomic store
    void Store( AtomicType _i );

//    // Increment, return incremented value
//    AtomicType Increment();

//    // Increment, return initial value
//    AtomicType FetchIncrement();

//    // Decrement, return decremented value
//    AtomicType Decrement();

//    // Decrement, return initial value
//    AtomicType FetchDecrement();

//    // Add, return resulting value
//    AtomicType Add( AtomicType _i );

//    // Add, return initial value
//    AtomicType FetchAdd( AtomicType _i );

//    // Subtract, return resulting value
//    AtomicType Sub( AtomicType _i );

//    // Subtract, return initial value
//    AtomicType FetchSub( AtomicType _i );

    AtomicType Exchange( AtomicType _Exchange );

    bool CompareExchange( AtomicType _Exchange, AtomicType _Comparand );

private:
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicType i;
#else
    std::atomic< AtomicType > i;
#endif
};

template< typename Type >
AN_FORCEINLINE typename TAtomicPtr< Type >::AtomicType TAtomicPtr< Type >::LoadRelaxed() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return i;
#else
    return i.load( std::memory_order_relaxed );
#endif
}

template< typename Type >
AN_FORCEINLINE void TAtomicPtr< Type >::StoreRelaxed( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    i = _i;
#else
    i.store( _i, std::memory_order_relaxed );
#endif
}

#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
volatile void * AtomicLoadAcquirePointer( volatile void *& _Ptr );
void AtomicStoreReleasePointer( volatile void *& _Dst, void * _Src );
void * AtomicExchangePointer( void *& _Ptr, void * _Exchange );
void * AtomicCompareExchangePointer( void *& _Ptr, void * _Desired, void * _Expected );
#endif

template< typename Type >
AN_FORCEINLINE typename TAtomicPtr< Type >::AtomicType TAtomicPtr< Type >::Load() const {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AtomicLoadAcquirePointer( i );
#else
    return i.load( std::memory_order_acquire );
#endif
}

template< typename Type >
AN_FORCEINLINE void TAtomicPtr< Type >::Store( AtomicType _i ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    AtomicStoreReleasePointer( i, _i );
#else
    i.store( _i, std::memory_order_release );
#endif
}

template< typename Type >
AN_FORCEINLINE typename TAtomicPtr< Type >::AtomicType TAtomicPtr< Type >::Exchange( AtomicType _Exchange ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AtomicExchangePointer( i, _Exchange );
#else
    return i.exchange( _Exchange );
#endif
}

template< typename Type >
AN_FORCEINLINE bool TAtomicPtr< Type >::CompareExchange( AtomicType _Expected, AtomicType _Desired ) {
#if defined AN_OS_WIN32 && !defined AN_STD_ATOMIC
    return AtomicCompareExchangePointer( i, _Desired, _Expected ) == _Expected; // FIXME
#else
    // FIXME: strong or weak?
    return i.compare_exchange_strong( _Expected, _Desired );
#endif
}
