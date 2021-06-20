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

#include <algorithm>

/**

TArray

C array replacement

*/
template< typename T, size_t ArraySize >
class TArray final {
public:
    typedef T * Iterator;
    typedef const T * ConstIterator;

    void Swap( int _Index1, int _Index2 )
    {
        std::swap( (*this)[_Index1], (*this)[_Index2] )
    }

    void Reverse()
    {
        const int HalfArrayLength = ArraySize >> 1;
        const int ArrayLengthMinusOne = ArraySize - 1;
        for ( int i = 0 ; i < HalfArrayLength ; i++ ) {
            std::swap( ArrayData[i], ArrayData[ArrayLengthMinusOne - i] );
        }
    }

    /** Reverse elements order in range [ _FirstIndex ; _LastIndex ) */
    void Reverse( int _FirstIndex, int _LastIndex )
    {
        AN_ASSERT_( _FirstIndex >= 0 && _FirstIndex < ArraySize, "TArray::Reverse: array index is out of bounds" );
        AN_ASSERT_( _LastIndex >= 0 && _LastIndex <= ArraySize, "TArray::Reverse: array index is out of bounds" );
        AN_ASSERT_( _FirstIndex < _LastIndex, "TArray::Reverse: invalid order" );

        const int HalfRangeLength = (_LastIndex - _FirstIndex) >> 1;
        const int LastIndexMinusOne = _LastIndex - 1;

        for ( int i = 0 ; i < HalfRangeLength ; i++ ) {
            std::swap( ArrayData[_FirstIndex + i], ArrayData[LastIndexMinusOne - i] );
        }
    }

    T & operator[]( const int _Index )
    {
        AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TArray::operator[]" );
        return ArrayData[_Index];
    }

    T const & operator[]( const int _Index ) const
    {
        AN_ASSERT_( _Index >= 0 && _Index < ArraySize, "TArray::operator[]" );
        return ArrayData[_Index];
    }

    T & Last()
    {
        AN_ASSERT_( ArraySize > 0, "TArray::Last" );
        return ArrayData[ArraySize - 1];
    }

    T const & Last() const
    {
        AN_ASSERT_( ArraySize > 0, "TArray::Last" );
        return ArrayData[ArraySize - 1];
    }

    T & First()
    {
        AN_ASSERT_( ArraySize > 0, "TArray::First" );
        return ArrayData[0];
    }

    T const & First() const
    {
        AN_ASSERT_( ArraySize > 0, "TArray::First" );
        return ArrayData[0];
    }

    Iterator Begin()
    {
        return ArrayData;
    }

    ConstIterator Begin() const
    {
        return ArrayData;
    }

    Iterator End()
    {
        return ArrayData + ArraySize;
    }

    ConstIterator End() const
    {
        return ArrayData + ArraySize;
    }

    /** foreach compatibility */
    Iterator begin() { return Begin(); }

    /** foreach compatibility */
    ConstIterator begin() const { return Begin(); }

    /** foreach compatibility */
    Iterator end() { return End(); }

    /** foreach compatibility */
    ConstIterator end() const { return End(); }

    ConstIterator Find( T const & _Element ) const
    {
        return Find( Begin(), End(), _Element );
    }

    ConstIterator Find( ConstIterator _Begin, ConstIterator _End, const T & _Element ) const
    {
        for ( auto It = _Begin ; It != _End ; It++ ) {
            if ( *It == _Element ) {
                return It;
            }
        }
        return End();
    }

    bool IsExist( T const & _Element ) const
    {
        for ( int i = 0 ; i < ArraySize ; i++ ) {
            if ( ArrayData[i] == _Element ) {
                return true;
            }
        }
        return false;
    }

    int IndexOf( T const & _Element ) const
    {
        for ( int i = 0 ; i < ArraySize ; i++ ) {
            if ( ArrayData[i] == _Element ) {
                return i;
            }
        }
        return -1;
    }

    T * ToPtr()
    {
        return ArrayData;
    }

    T const * ToPtr() const
    {
        return ArrayData;
    }

    int Size() const
    {
        return ArraySize;
    }

    T ArrayData[ArraySize];
};
