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

#include "PodArray.h"

/*

TPodStack

Stack for POD types

*/
template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
class TPodStack final {
public:
    enum { TYPE_SIZEOF = sizeof( T ) };

    TPodStack();
    TPodStack( TPodStack const & _Stack );
    ~TPodStack();

    void            Clear();
    void            Free();
    void            ShrinkToFit();
    void            Reserve( int _NewCapacity );
    void            ReserveInvalidate( int _NewCapacity );
    void            Memset( const int _Value );
    void            ZeroMem();
    void            Reverse();
    bool            IsEmpty() const;
    T *             Push();
    T *             Pop();
    T *             Top() const;
    T *             Bottom() const;
    T *             ToPtr();
    const T *       ToPtr() const;
    int             Size() const;
    int             Reserved() const;
    int             StackPoint() const;
    TPodStack &     operator=( TPodStack const & _Stack );
    void            Set( T const * _Elements, int _NumElements );

private:
    TPodArray< T, BASE_CAPACITY, GRANULARITY, Allocator > Array;
};

template< typename T >
using TPodStackLite = TPodStack< T, 1 >;

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodStack< T, BASE_CAPACITY, GRANULARITY >::TPodStack()
{
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodStack< T, BASE_CAPACITY, GRANULARITY >::TPodStack( TPodStack< T, BASE_CAPACITY, GRANULARITY > const & _Stack )
    : Array( _Stack.ToPtr(), _Stack.Size() )
{
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodStack< T, BASE_CAPACITY, GRANULARITY >::~TPodStack() {
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::Clear() {
    Array.Clear();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::Free() {
    Array.Free();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::ShrinkToFit() {
    Array.ShrinkToFit();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::Reserve( int _NewCapacity ) {
    Array.Reserve( _NewCapacity );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::ReserveInvalidate( int _NewCapacity ) {
    Array.ReserveInvalidate( _NewCapacity );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::Memset( const int _Value ) {
    Array.Memset( _Value );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::ZeroMem() {
    Array.ZeroMem();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::Reverse() {
    Array.Reverse();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T * TPodStack< T, BASE_CAPACITY, GRANULARITY >::Push() {
    Array.Resize( Array.Size() + 1 );
    return Array.ToPtr()[ Array.Size() - 1 ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T * TPodStack< T, BASE_CAPACITY, GRANULARITY >::Pop() {
    if ( IsEmpty() ) {
        return nullptr;
    }
    T * element = &Array.Last();
    Array.RemoveLast();
    return element;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE bool TPodStack< T, BASE_CAPACITY, GRANULARITY >::IsEmpty() const {
    return Array.IsEmpty();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T * TPodStack< T, BASE_CAPACITY, GRANULARITY >::Top() const {
    return IsEmpty() ? nullptr : &Array.Last();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T * TPodStack< T, BASE_CAPACITY, GRANULARITY >::Bottom() const {
    return IsEmpty() ? nullptr : Array.ToPtr();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T * TPodStack< T, BASE_CAPACITY, GRANULARITY >::ToPtr() {
    return Array.ToPtr();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE const T * TPodStack< T, BASE_CAPACITY, GRANULARITY >::ToPtr() const {
    return Array.ToPtr();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TPodStack< T, BASE_CAPACITY, GRANULARITY >::Size() const {
    return Array.Size();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TPodStack< T, BASE_CAPACITY, GRANULARITY >::Reserved() const {
    return Array.Reserved();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TPodStack< T, BASE_CAPACITY, GRANULARITY >::StackPoint() const {
    return Size() - 1;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodStack< T, BASE_CAPACITY, GRANULARITY > & TPodStack< T, BASE_CAPACITY, GRANULARITY >::operator=( TPodStack< T, BASE_CAPACITY, GRANULARITY > const & _Stack ) {
    Set( _Stack.ToPtr(), _Stack.Size() );
    return *this;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodStack< T, BASE_CAPACITY, GRANULARITY >::Set( T const * _Elements, int _NumElements ) {
    Array.Set( _Elements, _NumElements );
}
