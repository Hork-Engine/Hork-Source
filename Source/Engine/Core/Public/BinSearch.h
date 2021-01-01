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

namespace Core {

/** Find last element less than _Value in sorted array */
template< typename T >
AN_FORCEINLINE int BinSearchLess( T const * _Array, const int _ArrayLength, T const & _Value ) {
    int len = _ArrayLength;
    int mid = len;
    int offset = 0;
    while ( mid > 0 ) {
        mid = len >> 1;
        if ( _Array[offset+mid] < _Value ) {
            offset += mid;
        }
        len -= mid;
    }
    return offset;
}

/** Find last element less or equal than _Value in sorted array */
template< typename T >
AN_FORCEINLINE int BinSearchLequal( T const * _Array, const int _ArrayLength, T const & _Value ) {
    int len = _ArrayLength;
    int mid = len;
    int offset = 0;
    while ( mid > 0 ) {
        mid = len >> 1;
        if ( _Array[offset+mid] <= _Value ) {
            offset += mid;
        }
        len -= mid;
    }
    return offset;
}

/** Find first element greater than _Value in sorted array */
template< typename T >
AN_FORCEINLINE int BinSearchGreater( T const * _Array, const int _ArrayLength, T const & _Value ) {
    int len = _ArrayLength;
    int mid = len;
    int offset = 0;
    int res = 0;
    while ( mid > 0 ) {
        mid = len >> 1;
        if ( _Array[offset+mid] > _Value ) {
            res = 0;
        } else {
            offset += mid;
            res = 1;
        }
        len -= mid;
    }
    return offset+res;
}

/** Find first element greater or equal than _Value in sorted array */
template< typename T >
AN_FORCEINLINE int BinSearchGequal( T const * _Array, const int _ArrayLength, T const & _Value ) {
    int len = _ArrayLength;
    int mid = len;
    int offset = 0;
    int res = 0;
    while ( mid > 0 ) {
        mid = len >> 1;
        if ( _Array[offset+mid] >= _Value ) {
            res = 0;
        } else {
            offset += mid;
            res = 1;
        }
        len -= mid;
    }
    return offset+res;
}

}

