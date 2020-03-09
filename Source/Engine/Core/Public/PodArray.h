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

#include "Alloc.h"

#define TPodArrayTemplateDecorate \
    template< typename T, int BASE_CAPACITY, int GRANULARITY, typename Allocator > AN_FORCEINLINE

#define TPodArrayTemplate \
    TPodArray< T, BASE_CAPACITY, GRANULARITY, Allocator >

/*

TPodArray

Array for POD types

*/
template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
class TPodArray final {
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
    void            Append( TPodArrayTemplate const & _Array );
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

    T &             operator[]( const int _Index );
    T const &       operator[]( const int _Index ) const;

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

//    // Byte serialization
//    void Write( IStreamBase & _Stream ) const {
//        _Stream.WriteUInt32( ArraySize );
//        for ( int i = 0 ; i < ArraySize ; i++ ) {
//            ArrayData[i].Write( _Stream );
//        }
//    }

//    void Read( IStreamBase & _Stream ) {
//        int newLength = _Stream.ReadUInt32();
//        ResizeInvalidate( newLength );
//        for ( int i = 0 ; i < ArraySize ; i++ ) {
//            ArrayData[i].Read( _Stream );
//        }
//    }

private:
    alignas( 16 ) T StaticData[BASE_CAPACITY];
    T *             ArrayData;
    int             ArraySize;
    int             ArrayCapacity;

    static_assert( std::is_trivial< T >::value, "Expected POD type" );
    //static_assert( IsAligned( sizeof( T ), Alignment ), "Alignment check" );
};

template< typename T >
using TPodArrayLite = TPodArray< T, 1 >;

template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32 >
using TPodArrayHeap = TPodArray< T, BASE_CAPACITY, GRANULARITY, AHeapAllocator >;

TPodArrayTemplateDecorate
TPodArrayTemplate::TPodArray()
    : ArrayData( StaticData ), ArraySize( 0 ), ArrayCapacity( BASE_CAPACITY )
{
    static_assert( BASE_CAPACITY > 0, "TPodArray: Invalid BASE_CAPACITY" );
    AN_ASSERT( IsAlignedPtr( StaticData, 16 ) );
}

TPodArrayTemplateDecorate
TPodArrayTemplate::TPodArray( TPodArrayTemplate const & _Array ) {
    ArraySize = _Array.ArraySize;
    if ( _Array.ArrayCapacity > BASE_CAPACITY ) {
        ArrayCapacity = _Array.ArrayCapacity;
        ArrayData = ( T * )Allocator::Inst().ClearedAlloc( TYPE_SIZEOF * ArrayCapacity );
    } else {
        ArrayCapacity = BASE_CAPACITY;
        ArrayData = StaticData;
    }
    memcpy( ArrayData, _Array.ArrayData, TYPE_SIZEOF * ArraySize );
}

TPodArrayTemplateDecorate
TPodArrayTemplate::TPodArray( T const * _Elements, int _NumElements ) {
    ArraySize = _NumElements;
    if ( _NumElements > BASE_CAPACITY ) {
        const int mod = _NumElements % GRANULARITY;
        ArrayCapacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        ArrayData = ( T * )Allocator::Inst().Alloc( TYPE_SIZEOF * ArrayCapacity );
    } else {
        ArrayCapacity = BASE_CAPACITY;
        ArrayData = StaticData;
    }
    memcpy( ArrayData, _Elements, TYPE_SIZEOF * ArraySize );
}

TPodArrayTemplateDecorate
TPodArrayTemplate::~TPodArray() {
    if ( ArrayData != StaticData ) {
        Allocator::Inst().Dealloc( ArrayData );
    }
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Clear() {
    ArraySize = 0;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Free() {
    if ( ArrayData != StaticData ) {
        Allocator::Inst().Dealloc( ArrayData );
        ArrayData = StaticData;
    }
    ArraySize = 0;
    ArrayCapacity = BASE_CAPACITY;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::ShrinkToFit() {
    if ( ArrayData == StaticData || ArrayCapacity == ArraySize ) {
        return;
    }

    if ( ArraySize <= BASE_CAPACITY ) {
        if ( ArraySize > 0 ) {
            memcpy( StaticData, ArrayData, TYPE_SIZEOF * ArraySize );
        }
        Allocator::Inst().Dealloc( ArrayData );
        ArrayData = StaticData;
        ArrayCapacity = BASE_CAPACITY;
        return;
    }

    T * data = ( T * )Allocator::Inst().Alloc( TYPE_SIZEOF * ArraySize );
    MemcpySSE( data, ArrayData, TYPE_SIZEOF * ArraySize );
    Allocator::Inst().Dealloc( ArrayData );
    ArrayData = data;
    ArrayCapacity = ArraySize;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Reserve( int _NewCapacity ) {
    if ( _NewCapacity <= ArrayCapacity ) {
        return;
    }
    if ( ArrayData == StaticData ) {
        ArrayData = ( T * )Allocator::Inst().Alloc( TYPE_SIZEOF * _NewCapacity );
        memcpy( ArrayData, StaticData, TYPE_SIZEOF * ArraySize );
    } else {
        ArrayData = ( T * )Allocator::Inst().Realloc( ArrayData, TYPE_SIZEOF * _NewCapacity, true );
    }
    ArrayCapacity = _NewCapacity;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::ReserveInvalidate( int _NewCapacity ) {
    if ( _NewCapacity <= ArrayCapacity ) {
        return;
    }
    if ( ArrayData == StaticData ) {
        ArrayData = ( T * )Allocator::Inst().Alloc( TYPE_SIZEOF * _NewCapacity );
    } else {
        ArrayData = ( T * )Allocator::Inst().Realloc( ArrayData, TYPE_SIZEOF * _NewCapacity, false );
    }
    ArrayCapacity = _NewCapacity;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Resize( int _NumElements ) {
    if ( _NumElements > ArrayCapacity ) {
        const int mod = _NumElements % GRANULARITY;
        const int capacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        Reserve( capacity );
    }
    ArraySize = _NumElements;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::ResizeInvalidate( int _NumElements ) {
    if ( _NumElements > ArrayCapacity ) {
        const int mod = _NumElements % GRANULARITY;
        const int capacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        ReserveInvalidate( capacity );
    }
    ArraySize = _NumElements;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Memset( const int _Value ) {
    memset( ArrayData, _Value, TYPE_SIZEOF * ArraySize );
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::ZeroMem() {
    Memset( 0 );
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Swap( int _Index1, int _Index2 ) {
    const T tmp = (*this)[ _Index1 ];
    (*this)[ _Index1 ] = (*this)[ _Index2 ];
    (*this)[ _Index2 ] = tmp;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Reverse() {
    const int HalfArrayLength = ArraySize >> 1;
    const int ArrayLengthMinusOne = ArraySize - 1;
    T tmp;
    for ( int i = 0 ; i < HalfArrayLength ; i++ ) {
        tmp = ArrayData[ i ];
        ArrayData[ i ] = ArrayData[ ArrayLengthMinusOne - i ];
        ArrayData[ ArrayLengthMinusOne - i ] = tmp;
    }
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Reverse( int _FirstIndex, int _LastIndex ) {
    AN_ASSERT_( _FirstIndex >= 0 && _FirstIndex < ArraySize, "TPodArray::Reverse: array index is out of bounds" );
    AN_ASSERT_( _LastIndex >= 0 && _LastIndex <= ArraySize, "TPodArray::Reverse: array index is out of bounds" );
    AN_ASSERT_( _FirstIndex < _LastIndex, "TPodArray::Reverse: invalid order" );

    const int HalfRangeLength = ( _LastIndex - _FirstIndex ) >> 1;
    const int LastIndexMinusOne = _LastIndex - 1;

    T tmp;
    for ( int i = 0 ; i < HalfRangeLength ; i++ ) {
        tmp = ArrayData[ _FirstIndex + i ];
        ArrayData[ _FirstIndex + i ] = ArrayData[ LastIndexMinusOne - i ];
        ArrayData[ LastIndexMinusOne - i ] = tmp;
    }
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Insert( int _Index, T const & _Element ) {
    if ( _Index == ArraySize ) {
        Append( _Element );
        return;
    }

    AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TPodArray::Insert: array index is out of bounds" );

    const int newLength = ArraySize + 1;
    const int mod = newLength % GRANULARITY;
    const int capacity = mod ? newLength + GRANULARITY - mod : newLength;

    if ( capacity > ArrayCapacity ) {
        T * data = ( T * )Allocator::Inst().Alloc( TYPE_SIZEOF * capacity );

        memcpy( data, ArrayData, TYPE_SIZEOF * _Index );
        data[ _Index ] = _Element;
        const int elementsToMove = ArraySize - _Index;
        memcpy( &data[ _Index + 1 ], &ArrayData[ _Index ], TYPE_SIZEOF * elementsToMove );

        if ( ArrayData != StaticData ) {
            Allocator::Inst().Dealloc( ArrayData );
        }
        ArrayData = data;
        ArrayCapacity = capacity;
        ArraySize = newLength;
        return;
    }

    memmove( ArrayData + _Index + 1, ArrayData + _Index, TYPE_SIZEOF * ( ArraySize - _Index ) );
    ArrayData[ _Index ] = _Element;
    ArraySize = newLength;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Append( T const & _Element ) {
    Resize( ArraySize + 1 );
    ArrayData[ ArraySize - 1 ] = _Element;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Append( TPodArrayTemplate const & _Array ) {
    Append( _Array.ArrayData, _Array.ArraySize );
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Append( T const * _Elements, int _NumElements ) {
    int length = ArraySize;

    Resize( ArraySize + _NumElements );
    memcpy( &ArrayData[ length ], _Elements, TYPE_SIZEOF * _NumElements );
}

TPodArrayTemplateDecorate
T & TPodArrayTemplate::Append() {
    Resize( ArraySize + 1 );
    return ArrayData[ ArraySize - 1 ];
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Remove( int _Index ) {
    AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TPodArray::Remove: array index is out of bounds" );

    memmove( ArrayData + _Index, ArrayData + _Index + 1, TYPE_SIZEOF * ( ArraySize - _Index - 1 ) );

    ArraySize--;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::RemoveLast() {
    if ( ArraySize > 0 ) {
        ArraySize--;
    }
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::RemoveDuplicates() {

    for ( int i = 0 ; i < ArraySize ; i++ ) {
        for ( int j = ArraySize-1 ; j > i ; j-- ) {

            if ( ArrayData[j] == ArrayData[i] ) {
                // duplicate found

                memmove( ArrayData + j, ArrayData + j + 1, TYPE_SIZEOF * ( ArraySize - j - 1 ) );
                ArraySize--;
            }
        }
    }
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::RemoveSwap( int _Index ) {
    AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TPodArray::RemoveSwap: array index is out of bounds" );

    if ( ArraySize > 0 ) {
        ArrayData[ _Index ] = ArrayData[ ArraySize - 1 ];
        ArraySize--;
    }
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Remove( int _FirstIndex, int _LastIndex ) {
    AN_ASSERT_( _FirstIndex >= 0 && _FirstIndex < ArraySize, "TPodArray::Remove: array index is out of bounds" );
    AN_ASSERT_( _LastIndex >= 0 && _LastIndex <= ArraySize, "TPodArray::Remove: array index is out of bounds" );
    AN_ASSERT_( _FirstIndex < _LastIndex, "TPodArray::Remove: invalid order" );

    memmove( ArrayData + _FirstIndex, ArrayData + _LastIndex, TYPE_SIZEOF * ( ArraySize - _LastIndex ) );
    ArraySize -= _LastIndex - _FirstIndex;
}

TPodArrayTemplateDecorate
bool TPodArrayTemplate::IsEmpty() const {
    return ArraySize == 0;
}

TPodArrayTemplateDecorate
T & TPodArrayTemplate::operator[]( const int _Index ) {
    AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TPodArray::operator[]" );
    return ArrayData[ _Index ];
}

TPodArrayTemplateDecorate
T const & TPodArrayTemplate::operator[]( const int _Index ) const {
    AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TPodArray::operator[]" );
    return ArrayData[ _Index ];
}

TPodArrayTemplateDecorate
T & TPodArrayTemplate::Last() {
    AN_ASSERT_( ArraySize > 0, "TPodArray::Last" );
    return ArrayData[ ArraySize - 1 ];
}

TPodArrayTemplateDecorate
T const & TPodArrayTemplate::Last() const {
    AN_ASSERT_( ArraySize > 0, "TPodArray::Last" );
    return ArrayData[ ArraySize - 1 ];
}

TPodArrayTemplateDecorate
T & TPodArrayTemplate::First() {
    AN_ASSERT_( ArraySize > 0, "TPodArray::First" );
    return ArrayData[ 0 ];
}

TPodArrayTemplateDecorate
T const & TPodArrayTemplate::First() const {
    AN_ASSERT_( ArraySize > 0, "TPodArray::First" );
    return ArrayData[ 0 ];
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::Iterator TPodArrayTemplate::Begin() {
    return ArrayData;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::ConstIterator TPodArrayTemplate::Begin() const {
    return ArrayData;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::Iterator TPodArrayTemplate::End() {
    return ArrayData + ArraySize;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::ConstIterator TPodArrayTemplate::End() const {
    return ArrayData + ArraySize;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::Iterator TPodArrayTemplate::Erase( ConstIterator _Iterator ) {
    AN_ASSERT_( _Iterator >= ArrayData && _Iterator < ArrayData + ArraySize, "TPodArray::Erase" );
    int Index = _Iterator - ArrayData;
    memmove( ArrayData + Index, ArrayData + Index + 1, TYPE_SIZEOF * ( ArraySize - Index - 1 ) );
    ArraySize--;
    return ArrayData + Index;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::Iterator TPodArrayTemplate::EraseSwap( ConstIterator _Iterator ) {
    AN_ASSERT_( _Iterator >= ArrayData && _Iterator < ArrayData + ArraySize, "TPodArray::Erase" );
    int Index = _Iterator - ArrayData;
    ArrayData[ Index ] = ArrayData[ ArraySize - 1 ];
    ArraySize--;
    return ArrayData + Index;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::Iterator TPodArrayTemplate::Insert( ConstIterator _Iterator, T const & _Element ) {
    AN_ASSERT_( _Iterator >= ArrayData && _Iterator <= ArrayData + ArraySize, "TPodArray::Insert" );
    int Index = _Iterator - ArrayData;
    Insert( Index, _Element );
    return ArrayData + Index;
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::ConstIterator TPodArrayTemplate::Find( T const & _Element ) const {
    return Find( Begin(), End(), _Element );
}

TPodArrayTemplateDecorate
typename TPodArrayTemplate::ConstIterator TPodArrayTemplate::Find( ConstIterator _Begin, ConstIterator _End, T const & _Element ) const {
    for ( auto It = _Begin ; It != _End ; It++ ) {
        if ( *It == _Element ) {
            return It;
        }
    }
    return End();
}

TPodArrayTemplateDecorate
bool TPodArrayTemplate::IsExist( T const & _Element ) const {
    for ( int i = 0 ; i < ArraySize ; i++ ) {
        if ( ArrayData[i] == _Element ) {
            return true;
        }
    }
    return false;
}

TPodArrayTemplateDecorate
int TPodArrayTemplate::IndexOf( T const & _Element ) const {
    for ( int i = 0 ; i < ArraySize ; i++ ) {
        if ( ArrayData[i] == _Element ) {
            return i;
        }
    }
    return -1;
}

TPodArrayTemplateDecorate
T * TPodArrayTemplate::ToPtr() {
    return ArrayData;
}

TPodArrayTemplateDecorate
T const * TPodArrayTemplate::ToPtr() const {
    return ArrayData;
}

TPodArrayTemplateDecorate
int TPodArrayTemplate::Size() const {
    return ArraySize;
}

TPodArrayTemplateDecorate
int TPodArrayTemplate::Reserved() const {
    return ArrayCapacity;
}

TPodArrayTemplateDecorate
TPodArrayTemplate & TPodArrayTemplate::operator=( TPodArrayTemplate const & _Array ) {
    Set( _Array.ArrayData, _Array.ArraySize );
    return *this;
}

TPodArrayTemplateDecorate
void TPodArrayTemplate::Set( T const * _Elements, int _NumElements ) {
    ResizeInvalidate( _NumElements );
    memcpy( ArrayData, _Elements, TYPE_SIZEOF * _NumElements );
}
