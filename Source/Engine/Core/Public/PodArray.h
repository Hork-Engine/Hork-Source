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

#include "Alloc.h"
#include "Integer.h"

/*

TPodArray

Array for POD types

*/
template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32 >
class TPodArray final {

    using Allocator = FAllocator;
public:
    typedef T * Iterator;
    typedef const T * ConstIterator;

    enum { TYPE_SIZEOF = sizeof( T ) };

    TPodArray();
    TPodArray( TPodArray const & _Array );
    TPodArray( T const * _Elements, int _NumElements );
    ~TPodArray();

    void            Clear();
    void            Free();
    void            ShrinkToFit();
    void            Reserve( int _NewCapacity );
    void            ReserveInvalidate( int _NewCapacity );
    void            Resize( int _NumElements );
    void            ResizeInvalidate( int _NumElements );
    void            Memset( const int _Value );
    void            ZeroMem();

    // Переставляет местами элементы
    void            Swap( int _Index1, int _Index2 );

    void            Reverse();

    // Отражает элементы в диапазоне [ _FirstIndex ; _LastIndex )
    void            Reverse( int _FirstIndex, int _LastIndex );

    void            Insert( int _Index, const T & _Element );

    void            Append( T const & _Element );
    void            Append( TPodArray< T, BASE_CAPACITY, GRANULARITY > const & _Array );
    void            Append( T const * _Elements, int _NumElements );
    T &             Append();

    void            Remove( int _Index );
    void            RemoveLast();
    void            RemoveDuplicates();

    // Оптимизированное удаление элемента массива без сдвигов путем перемещения
    // последнего элемента массива на место удаленного
    void            RemoveSwap( int _Index );

    // Удаляет элементы в диапазоне [ _FirstIndex ; _LastIndex )
    void            Remove( int _FirstIndex, int _LastIndex );

    bool            IsEmpty() const;

    T &             operator[]( int _Index );
    T const &       operator[]( int _Index ) const;

    T &             Last();
    T const &       Last() const;

    T &             First();
    T const &       First() const;

    Iterator        Begin();
    ConstIterator   Begin() const;
    Iterator        End();
    ConstIterator   End() const;

    // foreach compatibility
    Iterator        begin() { return Begin(); }
    ConstIterator   begin() const { return Begin(); }
    Iterator        end() { return End(); }
    ConstIterator   end() const { return End(); }

    Iterator        Erase( ConstIterator _Iterator );
    Iterator        EraseSwap( ConstIterator _Iterator );
    Iterator        Insert( ConstIterator _Iterator, const T & _Element );

    ConstIterator   Find( T const & _Element ) const;
    ConstIterator   Find( ConstIterator _Begin, ConstIterator _End, const T & _Element ) const;

    bool            IsExist( T const & _Element ) const;

    int             IndexOf( T const & _Element ) const;

    T *             ToPtr();
    T const *       ToPtr() const;

    // Возвращает количество элементов в массиве
    int             Size() const;

    // Возвращает количество элементов, под которые выделена память
    int             Reserved() const;

    TPodArray &     operator=( TPodArray const & _Array );

    void            Set( T const * _Elements, int _NumElements );

    // Byte serialization
    template< typename Stream >
    void Write( FStreamBase< Stream > & _Stream ) const {
        _Stream << Int(ArrayLength);
        for ( int i = 0 ; i < ArrayLength ; i++ ) {
            _Stream << ArrayData[i];
        }
    }

    template< typename Stream >
    void Read( FStreamBase< Stream > & _Stream ) {
        Int newLength;
        _Stream >> newLength;
        ResizeInvalidate( newLength );
        for ( int i = 0 ; i < ArrayLength ; i++ ) {
            _Stream >> ArrayData[i];
        }
    }

private:
    T *             ArrayData;
    T               StaticData[BASE_CAPACITY];
    int             ArrayLength;
    int             ArrayCapacity;
};

template< typename T >
using TPodArrayLite = TPodArray< T, 1 >;

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodArray< T, BASE_CAPACITY, GRANULARITY >::TPodArray()
    : ArrayData( StaticData ), ArrayLength( 0 ), ArrayCapacity( BASE_CAPACITY )
{
    static_assert( BASE_CAPACITY > 0, "TPodArray: Invalid BASE_CAPACITY" );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodArray< T, BASE_CAPACITY, GRANULARITY >::TPodArray( TPodArray< T, BASE_CAPACITY, GRANULARITY > const & _Array ) {
    ArrayLength = _Array.ArrayLength;
    if ( _Array.ArrayCapacity > BASE_CAPACITY ) {
        ArrayCapacity = _Array.ArrayCapacity;
        ArrayData = ( T * )Allocator::Alloc< 1 >( TYPE_SIZEOF * ArrayCapacity );
    } else {
        ArrayCapacity = BASE_CAPACITY;
        ArrayData = StaticData;
    }
    memcpy( ArrayData, _Array.ArrayData, TYPE_SIZEOF * ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodArray< T, BASE_CAPACITY, GRANULARITY >::TPodArray( T const * _Elements, int _NumElements ) {
    ArrayLength = _NumElements;
    if ( _NumElements > BASE_CAPACITY ) {
        const int mod = _NumElements % GRANULARITY;
        ArrayCapacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        ArrayData = ( T * )Allocator::Alloc< 1 >( TYPE_SIZEOF * ArrayCapacity );
    } else {
        ArrayCapacity = BASE_CAPACITY;
        ArrayData = StaticData;
    }
    memcpy( ArrayData, _Elements, TYPE_SIZEOF * ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodArray< T, BASE_CAPACITY, GRANULARITY >::~TPodArray() {
    if ( ArrayData != StaticData ) {
        Allocator::Dealloc( ArrayData );
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Clear() {
    ArrayLength = 0;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Free() {
    if ( ArrayData != StaticData ) {
        Allocator::Dealloc( ArrayData );
        ArrayData = StaticData;
    }
    ArrayLength = 0;
    ArrayCapacity = BASE_CAPACITY;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::ShrinkToFit() {
    if ( ArrayData == StaticData || ArrayCapacity == ArrayLength ) {
        return;
    }

    if ( ArrayLength <= BASE_CAPACITY ) {
        if ( ArrayLength > 0 ) {
            memcpy( StaticData, ArrayData, TYPE_SIZEOF * ArrayLength );
        }
        Allocator::Dealloc( ArrayData );
        ArrayData = StaticData;
        ArrayCapacity = BASE_CAPACITY;
        return;
    }

    T * data = ( T * )Allocator::Alloc< 1 >( TYPE_SIZEOF * ArrayLength );
    memcpy( data, ArrayData, TYPE_SIZEOF * ArrayLength );
    Allocator::Dealloc( ArrayData );
    ArrayData = data;
    ArrayCapacity = ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Reserve( int _NewCapacity ) {
    if ( _NewCapacity <= ArrayCapacity ) {
        return;
    }
    if ( ArrayData == StaticData ) {
        ArrayData = ( T * )Allocator::Alloc< 1 >( TYPE_SIZEOF * _NewCapacity );
        memcpy( ArrayData, StaticData, TYPE_SIZEOF * ArrayLength );
    } else {
        ArrayData = ( T * )Allocator::Extend< 1 >( ArrayData, TYPE_SIZEOF * ArrayCapacity, TYPE_SIZEOF * _NewCapacity, true );
    }
    ArrayCapacity = _NewCapacity;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::ReserveInvalidate( int _NewCapacity ) {
    if ( _NewCapacity <= ArrayCapacity ) {
        return;
    }
    if ( ArrayData == StaticData ) {
        ArrayData = ( T * )Allocator::Alloc< 1 >( TYPE_SIZEOF * _NewCapacity );
    } else {
        ArrayData = ( T * )Allocator::Extend< 1 >( ArrayData, TYPE_SIZEOF * ArrayCapacity, TYPE_SIZEOF * _NewCapacity, false );
    }
    ArrayCapacity = _NewCapacity;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Resize( int _NumElements ) {
    if ( _NumElements > ArrayCapacity ) {
        const int mod = _NumElements % GRANULARITY;
        const int capacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        Reserve( capacity );
    }
    ArrayLength = _NumElements;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::ResizeInvalidate( int _NumElements ) {
    if ( _NumElements > ArrayCapacity ) {
        const int mod = _NumElements % GRANULARITY;
        const int capacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        ReserveInvalidate( capacity );
    }
    ArrayLength = _NumElements;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Memset( const int _Value ) {
    memset( ArrayData, _Value, TYPE_SIZEOF * ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::ZeroMem() {
    Memset( 0 );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Swap( int _Index1, int _Index2 ) {
    const T tmp = (*this)[ _Index1 ];
    (*this)[ _Index1 ] = (*this)[ _Index2 ];
    (*this)[ _Index2 ] = tmp;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Reverse() {
    const int HalfArrayLength = ArrayLength >> 1;
    const int ArrayLengthMinusOne = ArrayLength - 1;
    T tmp;
    for ( int i = 0 ; i < HalfArrayLength ; i++ ) {
        tmp = ArrayData[ i ];
        ArrayData[ i ] = ArrayData[ ArrayLengthMinusOne - i ];
        ArrayData[ ArrayLengthMinusOne - i ] = tmp;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Reverse( int _FirstIndex, int _LastIndex ) {
    AN_ASSERT( _FirstIndex >= 0 && _FirstIndex < ArrayLength, "TPodArray::Reverse: array index is out of bounds" );
    AN_ASSERT( _LastIndex >= 0 && _LastIndex <= ArrayLength, "TPodArray::Reverse: array index is out of bounds" );
    AN_ASSERT( _FirstIndex < _LastIndex, "TPodArray::Reverse: invalid order" );

    const int HalfRangeLength = ( _LastIndex - _FirstIndex ) >> 1;
    const int LastIndexMinusOne = _LastIndex - 1;

    T tmp;
    for ( int i = 0 ; i < HalfRangeLength ; i++ ) {
        tmp = ArrayData[ _FirstIndex + i ];
        ArrayData[ _FirstIndex + i ] = ArrayData[ LastIndexMinusOne - i ];
        ArrayData[ LastIndexMinusOne - i ] = tmp;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Insert( int _Index, T const & _Element ) {
    if ( _Index == ArrayLength ) {
        Append( _Element );
        return;
    }

    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TPodArray::Insert: array index is out of bounds" );

    const int newLength = ArrayLength + 1;
    const int mod = newLength % GRANULARITY;
    const int capacity = mod ? newLength + GRANULARITY - mod : newLength;

    if ( capacity > ArrayCapacity ) {
        T * data = ( T * )Allocator::Alloc< 1 >( TYPE_SIZEOF * capacity );

        memcpy( data, ArrayData, TYPE_SIZEOF * _Index );
        data[ _Index ] = _Element;
        const int elementsToMove = ArrayLength - _Index;
        memcpy( &data[ _Index + 1 ], &ArrayData[ _Index ], TYPE_SIZEOF * elementsToMove );

        if ( ArrayData != StaticData ) {
            Allocator::Dealloc( ArrayData );
        }
        ArrayData = data;
        ArrayCapacity = capacity;
        ArrayLength = newLength;
        return;
    }

    memmove( ArrayData + _Index + 1, ArrayData + _Index, TYPE_SIZEOF * ( ArrayLength - _Index ) );
    ArrayData[ _Index ] = _Element;
    ArrayLength = newLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Append( T const & _Element ) {
    Resize( ArrayLength + 1 );
    ArrayData[ ArrayLength - 1 ] = _Element;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Append( TPodArray< T, BASE_CAPACITY, GRANULARITY > const & _Array ) {
    Append( _Array.ArrayData, _Array.ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Append( T const * _Elements, int _NumElements ) {
    int length = ArrayLength;

    Resize( ArrayLength + _NumElements );
    memcpy( &ArrayData[ length ], _Elements, TYPE_SIZEOF * _NumElements );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T & TPodArray< T, BASE_CAPACITY, GRANULARITY >::Append() {
    Resize( ArrayLength + 1 );
    return ArrayData[ ArrayLength - 1 ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Remove( int _Index ) {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TPodArray::Remove: array index is out of bounds" );

    memmove( ArrayData + _Index, ArrayData + _Index + 1, TYPE_SIZEOF * ( ArrayLength - _Index - 1 ) );

    ArrayLength--;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::RemoveLast() {
    if ( ArrayLength > 0 ) {
        ArrayLength--;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::RemoveDuplicates() {

    for ( int i = 0 ; i < ArrayLength ; i++ ) {
        for ( int j = ArrayLength-1 ; j > i ; j-- ) {

            if ( ArrayData[j] == ArrayData[i] ) {
                // duplicate found

                memmove( ArrayData + j, ArrayData + j + 1, TYPE_SIZEOF * ( ArrayLength - j - 1 ) );
                ArrayLength--;
            }
        }
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::RemoveSwap( int _Index ) {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TPodArray::RemoveSwap: array index is out of bounds" );

    if ( ArrayLength > 0 ) {
        ArrayData[ _Index ] = ArrayData[ ArrayLength - 1 ];
        ArrayLength--;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Remove( int _FirstIndex, int _LastIndex ) {
    AN_ASSERT( _FirstIndex >= 0 && _FirstIndex < ArrayLength, "TPodArray::Remove: array index is out of bounds" );
    AN_ASSERT( _LastIndex >= 0 && _LastIndex <= ArrayLength, "TPodArray::Remove: array index is out of bounds" );
    AN_ASSERT( _FirstIndex < _LastIndex, "TPodArray::Remove: invalid order" );

    memmove( ArrayData + _FirstIndex, ArrayData + _LastIndex, TYPE_SIZEOF * ( ArrayLength - _LastIndex ) );
    ArrayLength -= _LastIndex - _FirstIndex;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE bool TPodArray< T, BASE_CAPACITY, GRANULARITY >::IsEmpty() const {
    return ArrayLength == 0;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T & TPodArray< T, BASE_CAPACITY, GRANULARITY >::operator[]( int _Index ) {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TPodArray::operator[]" );
    return ArrayData[ _Index ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T const & TPodArray< T, BASE_CAPACITY, GRANULARITY >::operator[]( int _Index ) const {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TPodArray::operator[]" );
    return ArrayData[ _Index ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T & TPodArray< T, BASE_CAPACITY, GRANULARITY >::Last() {
    AN_ASSERT( ArrayLength > 0, "TPodArray::Last" );
    return ArrayData[ ArrayLength - 1 ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T const & TPodArray< T, BASE_CAPACITY, GRANULARITY >::Last() const {
    AN_ASSERT( ArrayLength > 0, "TPodArray::Last" );
    return ArrayData[ ArrayLength - 1 ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T & TPodArray< T, BASE_CAPACITY, GRANULARITY >::First() {
    AN_ASSERT( ArrayLength > 0, "TPodArray::First" );
    return ArrayData[ 0 ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T const & TPodArray< T, BASE_CAPACITY, GRANULARITY >::First() const {
    AN_ASSERT( ArrayLength > 0, "TPodArray::First" );
    return ArrayData[ 0 ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::Begin() {
    return ArrayData;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::Begin() const {
    return ArrayData;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::End() {
    return ArrayData + ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::End() const {
    return ArrayData + ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::Erase( ConstIterator _Iterator ) {
    AN_ASSERT( _Iterator >= ArrayData && _Iterator < ArrayData + ArrayLength, "TPodArray::Erase" );
    int Index = _Iterator - ArrayData;
    memmove( ArrayData + Index, ArrayData + Index + 1, TYPE_SIZEOF * ( ArrayLength - Index - 1 ) );
    ArrayLength--;
    return ArrayData + Index;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::EraseSwap( ConstIterator _Iterator ) {
    AN_ASSERT( _Iterator >= ArrayData && _Iterator < ArrayData + ArrayLength, "TPodArray::Erase" );
    int Index = _Iterator - ArrayData;
    ArrayData[ Index ] = ArrayData[ ArrayLength - 1 ];
    ArrayLength--;
    return ArrayData + Index;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::Insert( ConstIterator _Iterator, T const & _Element ) {
    AN_ASSERT( _Iterator >= ArrayData && _Iterator <= ArrayData + ArrayLength, "TPodArray::Insert" );
    int Index = _Iterator - ArrayData;
    Insert( Index, _Element );
    return ArrayData + Index;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::Find( T const & _Element ) const {
    return Find( Begin(), End(), _Element );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TPodArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TPodArray< T, BASE_CAPACITY, GRANULARITY >::Find( ConstIterator _Begin, ConstIterator _End, T const & _Element ) const {
    for ( auto It = _Begin ; It != _End ; It++ ) {
        if ( *It == _Element ) {
            return It;
        }
    }
    return End();
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE bool TPodArray< T, BASE_CAPACITY, GRANULARITY >::IsExist( T const & _Element ) const {
    for ( int i = 0 ; i < ArrayLength ; i++ ) {
        if ( ArrayData[i] == _Element ) {
            return true;
        }
    }
    return false;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TPodArray< T, BASE_CAPACITY, GRANULARITY >::IndexOf( T const & _Element ) const {
    for ( int i = 0 ; i < ArrayLength ; i++ ) {
        if ( ArrayData[i] == _Element ) {
            return i;
        }
    }
    return -1;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T * TPodArray< T, BASE_CAPACITY, GRANULARITY >::ToPtr() {
    return ArrayData;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE T const * TPodArray< T, BASE_CAPACITY, GRANULARITY >::ToPtr() const {
    return ArrayData;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TPodArray< T, BASE_CAPACITY, GRANULARITY >::Size() const {
    return ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TPodArray< T, BASE_CAPACITY, GRANULARITY >::Reserved() const {
    return ArrayCapacity;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TPodArray< T, BASE_CAPACITY, GRANULARITY > & TPodArray< T, BASE_CAPACITY, GRANULARITY >::operator=( TPodArray< T, BASE_CAPACITY, GRANULARITY > const & _Array ) {
    Set( _Array.ArrayData, _Array.ArrayLength );
    return *this;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TPodArray< T, BASE_CAPACITY, GRANULARITY >::Set( T const * _Elements, int _NumElements ) {
    ResizeInvalidate( _NumElements );
    memcpy( ArrayData, _Elements, TYPE_SIZEOF * _NumElements );
}
