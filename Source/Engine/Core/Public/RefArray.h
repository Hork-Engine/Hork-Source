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

TRefArray

Array for ref counted types

*/
template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32 >
class TRefArray final {

    using Allocator = FAllocator;
public:
    typedef T * Ref;
    typedef Ref * Iterator;
    typedef const Ref * ConstIterator;

    enum { TYPE_SIZEOF = sizeof( Ref ) };

    TRefArray();
    TRefArray( const TRefArray & _Array );
    TRefArray( const Ref * _Elements, int _NumElements );
    ~TRefArray();

    void            Clear();
    void            Free();
    void            ShrinkToFit();
    void            Reserve( int _NewCapacity );
    void            Resize( int _NumElements );

    // Переставляет местами элементы
    void            Swap( int _Index1, int _Index2 );

    void            Reverse();

    // Отражает элементы в диапазоне [ _FirstIndex ; _LastIndex )
    void            Reverse( int _FirstIndex, int _LastIndex );

    void            InsertBefore( int _Index, const Ref & _Element );
    void            InsertAfter( int _Index, const Ref & _Element );

    void            Append( const Ref & _Element );
    void            Append( const TRefArray< T, BASE_CAPACITY, GRANULARITY > & _Array );
    void            Append( const Ref * _Elements, int _NumElements );

    void            Remove( int _Index );
    void            RemoveLast();

    // Оптимизированное удаление элемента массива без сдвигов путем перемещения
    // последнего элемента массива на место удаленного
    void            RemoveSwap( int _Index );

    // Удаляет элементы в диапазоне [ _FirstIndex ; _LastIndex )
    void            Remove( int _FirstIndex, int _LastIndex );

    bool            IsEmpty() const;

    void            Set( int _Index, const Ref & _Value );
    //Ref &             operator[]( int _Index );
    const Ref &       operator[]( int _Index ) const;

    //Ref &             Last();
    const Ref &       Last() const;

    //Ref &             First();
    const Ref &       First() const;

//    Iterator        Begin();
    ConstIterator   Begin() const;
//    Iterator        End();
    ConstIterator   End() const;

    // foreach compatibility
//    Iterator        begin() { return Begin(); }
    ConstIterator   begin() const { return Begin(); }
//    Iterator        end() { return End(); }
    ConstIterator   end() const { return End(); }

    ConstIterator   Erase( ConstIterator _Iterator );
    ConstIterator   EraseSwap( ConstIterator _Iterator );
    ConstIterator   Insert( ConstIterator _Iterator, const Ref & _Element );

    ConstIterator   Find( const Ref & _Element ) const;
    ConstIterator   Find( ConstIterator _Begin, ConstIterator _End, const Ref & _Element ) const;

    template< typename Cmp >
    void            Sort( Cmp & _Cmp ) { StdSort( ArrayData, ArrayData + ArrayLength, _Cmp ); }

    //Ref *             ToPtr();
    const Ref *       ToPtr() const;

    // Возвращает количество элементов в массиве
    int             Length() const;

    // Возвращает количество элементов, под которые выделена память
    int             Reserved() const;

    TRefArray &     operator=( const TRefArray & _Array );

    void            Set( const Ref * _Elements, int _NumElements );

//    // Byte serialization
//    template< typename Stream >
//    void Write( FStreamBase< Stream > & _Stream ) const {
//        _Stream << Int(ArrayLength);
//        for ( int i = 0 ; i < ArrayLength ; i++ ) {
//            _Stream << ArrayData[i];
//        }
//    }

//    template< typename Stream >
//    void Read( FStreamBase< Stream > & _Stream ) {
//        Int newLength;
//        _Stream >> newLength;
//        Clear();
//        Resize( newLength );
//        for ( int i = 0 ; i < ArrayLength ; i++ ) {
//            _Stream >> ArrayData[i];
//            ArrayData[i]->AddRef();
//        }
//    }

private:
    void AddRef( Ref * _Begin, Ref * _End );
    void RemoveRef( Ref * _Begin, Ref * _End );

    Ref *           ArrayData;
    Ref             StaticData[BASE_CAPACITY];
    int             ArrayLength;
    int             ArrayCapacity;
};

template< typename T >
using TRefArrayLite = TRefArray< T, 1 >;

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TRefArray< T, BASE_CAPACITY, GRANULARITY >::TRefArray()
    : ArrayData( StaticData ), ArrayLength( 0 ), ArrayCapacity( BASE_CAPACITY )
{
    static_assert( BASE_CAPACITY > 0, "TRefArray: Invalid BASE_CAPACITY" );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TRefArray< T, BASE_CAPACITY, GRANULARITY >::TRefArray( const TRefArray< T, BASE_CAPACITY, GRANULARITY > & _Array ) {
    ArrayLength = _Array.ArrayLength;
    if ( _Array.ArrayCapacity > BASE_CAPACITY ) {
        ArrayCapacity = _Array.ArrayCapacity;
        ArrayData = ( Ref * )Allocator::Alloc< 1 >( TYPE_SIZEOF * ArrayCapacity );
    } else {
        ArrayCapacity = BASE_CAPACITY;
        ArrayData = StaticData;
    }
    memcpy( ArrayData, _Array.ArrayData, TYPE_SIZEOF * ArrayLength );
    AddRef( ArrayData, ArrayData + ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TRefArray< T, BASE_CAPACITY, GRANULARITY >::TRefArray( const Ref * _Elements, int _NumElements ) {
    ArrayLength = _NumElements;
    if ( _NumElements > BASE_CAPACITY ) {
        const int mod = _NumElements % GRANULARITY;
        ArrayCapacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        ArrayData = ( Ref * )Allocator::Alloc< 1 >( TYPE_SIZEOF * ArrayCapacity );
    } else {
        ArrayCapacity = BASE_CAPACITY;
        ArrayData = StaticData;
    }
    memcpy( ArrayData, _Elements, TYPE_SIZEOF * ArrayLength );
    AddRef( ArrayData, ArrayData + ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TRefArray< T, BASE_CAPACITY, GRANULARITY >::~TRefArray() {
    RemoveRef( ArrayData, ArrayData + ArrayLength );
    if ( ArrayData != StaticData ) {
        Allocator::Dealloc( ArrayData );
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::AddRef( Ref * _Begin, Ref * _End ) {
    for ( Ref * p = _Begin ; p < _End ; p++ ) {
        if ( *p ) {
            (*p)->AddRef();
        }
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::RemoveRef( Ref * _Begin, Ref * _End ) {
    for ( Ref * p = _Begin ; p < _End ; p++ ) {
        if ( *p ) {
            (*p)->RemoveRef();
        }
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Clear() {
    RemoveRef( ArrayData, ArrayData + ArrayLength );
    ArrayLength = 0;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Free() {
    RemoveRef( ArrayData, ArrayData + ArrayLength );
    if ( ArrayData != StaticData ) {
        Allocator::Dealloc( ArrayData );
        ArrayData = StaticData;
    }
    ArrayLength = 0;
    ArrayCapacity = BASE_CAPACITY;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::ShrinkToFit() {
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

    Ref * data = ( Ref * )Allocator::Alloc< 1 >( TYPE_SIZEOF * ArrayLength );
    memcpy( data, ArrayData, TYPE_SIZEOF * ArrayLength );
    Allocator::Dealloc( ArrayData );
    ArrayData = data;
    ArrayCapacity = ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Reserve( int _NewCapacity ) {
    if ( _NewCapacity <= ArrayCapacity ) {
        return;
    }
    if ( ArrayData == StaticData ) {
        ArrayData = ( Ref * )Allocator::Alloc< 1 >( TYPE_SIZEOF * _NewCapacity );
        memcpy( ArrayData, StaticData, TYPE_SIZEOF * ArrayLength );
    } else {
        ArrayData = ( Ref * )Allocator::Extend< 1 >( ArrayData, TYPE_SIZEOF * ArrayLength, TYPE_SIZEOF * _NewCapacity, true );
    }
    ArrayCapacity = _NewCapacity;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Resize( int _NumElements ) {
    RemoveRef( ArrayData + _NumElements, ArrayData + ArrayLength );
    if ( _NumElements > ArrayCapacity ) {
        const int mod = _NumElements % GRANULARITY;
        const int capacity = mod ? _NumElements + GRANULARITY - mod : _NumElements;
        Reserve( capacity );
    }
    if ( _NumElements > ArrayLength ) {
        memset( ArrayData + ArrayLength, 0, TYPE_SIZEOF * ( _NumElements - ArrayLength ) );
    }
    ArrayLength = _NumElements;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Swap( int _Index1, int _Index2 ) {
    const Ref tmp = (*this)[ _Index1 ];
    (*this)[ _Index1 ] = (*this)[ _Index2 ];
    (*this)[ _Index2 ] = tmp;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Reverse() {
    const int HalfArrayLength = ArrayLength >> 1;
    const int ArrayLengthMinusOne = ArrayLength - 1;
    Ref tmp;
    for ( int i = 0 ; i < HalfArrayLength ; i++ ) {
        tmp = ArrayData[ i ];
        ArrayData[ i ] = ArrayData[ ArrayLengthMinusOne - i ];
        ArrayData[ ArrayLengthMinusOne - i ] = tmp;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Reverse( int _FirstIndex, int _LastIndex ) {
    AN_ASSERT( _FirstIndex >= 0 && _FirstIndex < ArrayLength, "TRefArray::Reverse: array index is out of bounds" );
    AN_ASSERT( _LastIndex >= 0 && _LastIndex <= ArrayLength, "TRefArray::Reverse: array index is out of bounds" );
    AN_ASSERT( _FirstIndex < _LastIndex, "TRefArray::Reverse: invalid order" );

    const int HalfRangeLength = ( _LastIndex - _FirstIndex ) >> 1;
    const int LastIndexMinusOne = _LastIndex - 1;

    Ref tmp;
    for ( int i = 0 ; i < HalfRangeLength ; i++ ) {
        tmp = ArrayData[ _FirstIndex + i ];
        ArrayData[ _FirstIndex + i ] = ArrayData[ LastIndexMinusOne - i ];
        ArrayData[ LastIndexMinusOne - i ] = tmp;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::InsertBefore( int _Index, const Ref & _Element ) {
    if ( _Index == 0 && ArrayLength == 0 ) {
        Append( _Element );
        return;
    }

    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TRefArray::InsertBefore: array index is out of bounds" );

    const int newLength = ArrayLength + 1;
    const int mod = newLength % GRANULARITY;
    const int capacity = mod ? newLength + GRANULARITY - mod : newLength;

    if ( capacity > ArrayCapacity ) {
        Ref * data = ( Ref * )Allocator::Alloc< 1 >( TYPE_SIZEOF * capacity );

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
    if ( _Element ) {
        _Element->AddRef();
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::InsertAfter( int _Index, const Ref & _Element ) {
    if ( _Index + 1 == ArrayLength ) {
        Append( _Element );
    } else {
        InsertBefore( _Index + 1, _Element );
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Append( const Ref & _Element ) {
    Resize( ArrayLength + 1 );
    ArrayData[ ArrayLength - 1 ] = _Element;
    if ( _Element ) {
        _Element->AddRef();
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Append( const TRefArray< T, BASE_CAPACITY, GRANULARITY > & _Array ) {
    Append( _Array.ArrayData, _Array.ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Append( const Ref * _Elements, int _NumElements ) {
    int length = ArrayLength;

    Resize( ArrayLength + _NumElements );
    memcpy( &ArrayData[ length ], _Elements, TYPE_SIZEOF * _NumElements );
    AddRef( ArrayData + length, ArrayData + ArrayLength );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Remove( int _Index ) {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TRefArray::Remove: array index is out of bounds" );

    if ( ArrayData[ _Index ] ) {
        ArrayData[ _Index ]->RemoveRef();
    }

    memmove( ArrayData + _Index, ArrayData + _Index + 1, TYPE_SIZEOF * ( ArrayLength - _Index - 1 ) );

    ArrayLength--;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::RemoveLast() {
    if ( ArrayLength > 0 ) {
        ArrayLength--;
        if ( ArrayData[ArrayLength] ) {
            ArrayData[ArrayLength]->RemoveRef();
        }
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::RemoveSwap( int _Index ) {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TRefArray::RemoveSwap: array index is out of bounds" );

    if ( ArrayLength > 0 ) {
        if ( ArrayData[ _Index ] ) {
            ArrayData[ _Index ]->RemoveRef();
        }
        ArrayData[ _Index ] = ArrayData[ ArrayLength - 1 ];
        ArrayLength--;
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Remove( int _FirstIndex, int _LastIndex ) {
    AN_ASSERT( _FirstIndex >= 0 && _FirstIndex < ArrayLength, "TRefArray::Remove: array index is out of bounds" );
    AN_ASSERT( _LastIndex >= 0 && _LastIndex <= ArrayLength, "TRefArray::Remove: array index is out of bounds" );
    AN_ASSERT( _FirstIndex < _LastIndex, "TRefArray::Remove: invalid order" );

    RemoveRef( ArrayData + _FirstIndex, ArrayData + _LastIndex );

    memmove( ArrayData + _FirstIndex, ArrayData + _LastIndex, TYPE_SIZEOF * ( ArrayLength - _LastIndex ) );
    ArrayLength -= _LastIndex - _FirstIndex;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE bool TRefArray< T, BASE_CAPACITY, GRANULARITY >::IsEmpty() const {
    return ArrayLength == 0;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Set( int _Index, const Ref & _Value ) {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TRefArray::Set" );
    if ( ArrayData[ _Index ] == _Value ) {
        return;
    }
    if ( ArrayData[ _Index ] ) {
        ArrayData[ _Index ]->RemoveRef();
    }
    ArrayData[ _Index ] = _Value;
    if ( _Value ) {
        _Value->AddRef();
    }
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE const typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::Ref & TRefArray< T, BASE_CAPACITY, GRANULARITY >::operator[]( int _Index ) const {
    AN_ASSERT( _Index >= 0 && _Index < ArrayLength, "TRefArray::operator[]" );
    return ArrayData[ _Index ];
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE const typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::Ref & TRefArray< T, BASE_CAPACITY, GRANULARITY >::Last() const {
    AN_ASSERT( ArrayLength > 0, "TRefArray::Last" );
    return ArrayData[ ArrayLength - 1 ];
}

//template< typename T, int BASE_CAPACITY, int GRANULARITY >
//AN_FORCEINLINE Ref & TRefArray< T, BASE_CAPACITY, GRANULARITY >::First() {
//    AN_ASSERT( ArrayLength > 0, "TRefArray::First" );
//    return ArrayData[ 0 ];
//}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE const typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::Ref & TRefArray< T, BASE_CAPACITY, GRANULARITY >::First() const {
    AN_ASSERT( ArrayLength > 0, "TRefArray::First" );
    return ArrayData[ 0 ];
}

//template< typename T, int BASE_CAPACITY, int GRANULARITY >
//AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::Begin() {
//    return ArrayData;
//}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::Begin() const {
    return ArrayData;
}

//template< typename T, int BASE_CAPACITY, int GRANULARITY >
//AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::Iterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::End() {
//    return ArrayData + ArrayLength;
//}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::End() const {
    return ArrayData + ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::Erase( ConstIterator _Iterator ) {
    AN_ASSERT( _Iterator >= ArrayData && _Iterator < ArrayData + ArrayLength, "TRefArray::Erase" );
    int Index = _Iterator - ArrayData;
    if ( ArrayData[Index] ) {
        ArrayData[Index]->RemoveRef();
    }
    memmove( ArrayData + Index, ArrayData + Index + 1, TYPE_SIZEOF * ( ArrayLength - Index - 1 ) );
    ArrayLength--;
    return ArrayData + Index;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::EraseSwap( ConstIterator _Iterator ) {
    AN_ASSERT( _Iterator >= ArrayData && _Iterator < ArrayData + ArrayLength, "TRefArray::Erase" );
    int Index = _Iterator - ArrayData;
    if ( ArrayData[ Index ] ) {
        ArrayData[ Index ]->RemoveRef();
    }
    ArrayData[ Index ] = ArrayData[ ArrayLength - 1 ];
    ArrayLength--;
    return ArrayData + Index;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::Insert( ConstIterator _Iterator, const Ref & _Element ) {
    AN_ASSERT( _Iterator >= ArrayData && _Iterator <= ArrayData + ArrayLength, "TRefArray::Insert" );
    int Index = _Iterator - ArrayData;
    InsertBefore( Index, _Element );
    return ArrayData + Index;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::Find( const Ref & _Element ) const {
    return Find( Begin(), End(), _Element );
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::ConstIterator TRefArray< T, BASE_CAPACITY, GRANULARITY >::Find( ConstIterator _Begin, ConstIterator _End, const Ref & _Element ) const {
    for ( auto It = _Begin ; It != _End ; It++ ) {
        if ( *It == _Element ) {
            return It;
        }
    }
    return End();
}

//template< typename T, int BASE_CAPACITY, int GRANULARITY >
//AN_FORCEINLINE Ref * TRefArray< T, BASE_CAPACITY, GRANULARITY >::ToPtr() {
//    return ArrayData;
//}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE const typename TRefArray< T, BASE_CAPACITY, GRANULARITY >::Ref * TRefArray< T, BASE_CAPACITY, GRANULARITY >::ToPtr() const {
    return ArrayData;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TRefArray< T, BASE_CAPACITY, GRANULARITY >::Length() const {
    return ArrayLength;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE int TRefArray< T, BASE_CAPACITY, GRANULARITY >::Reserved() const {
    return ArrayCapacity;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE TRefArray< T, BASE_CAPACITY, GRANULARITY > & TRefArray< T, BASE_CAPACITY, GRANULARITY >::operator=( const TRefArray< T, BASE_CAPACITY, GRANULARITY > & _Array ) {
    Set( _Array.ArrayData, _Array.ArrayLength );
    return *this;
}

template< typename T, int BASE_CAPACITY, int GRANULARITY >
AN_FORCEINLINE void TRefArray< T, BASE_CAPACITY, GRANULARITY >::Set( const Ref * _Elements, int _NumElements ) {
    Clear();
    Resize( _NumElements );
    memcpy( ArrayData, _Elements, TYPE_SIZEOF * _NumElements );
    AddRef( ArrayData, ArrayData + _NumElements );
}
