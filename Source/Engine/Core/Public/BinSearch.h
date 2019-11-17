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

#include "BaseTypes.h"

// Массив должен быть отсортирован по возрастанию

namespace Core {

// Ищет последний элемент массива, который меньше заданного значения
template< typename type >
AN_FORCEINLINE int BinSearchLess( const type * _Array, const int _ArrayLength, const type & _Value ) {
    int Len = _ArrayLength;
    int Mid = Len;
    int Offset = 0;
    while ( Mid > 0 ) {
        Mid = Len >> 1;
        if ( _Array[Offset+Mid] < _Value ) {
            Offset += Mid;
        }
        Len -= Mid;
    }
    return Offset;
}

// Ищет последний элемент массива, который меньше или равен заданному значению
template< typename type >
AN_FORCEINLINE int BinSearchLequal( const type * _Array, const int _ArrayLength, const type & _Value ) {
    int Len = _ArrayLength;
    int Mid = Len;
    int Offset = 0;
    while ( Mid > 0 ) {
        Mid = Len >> 1;
        if ( _Array[Offset+Mid] <= _Value ) {
            Offset += Mid;
        }
        Len -= Mid;
    }
    return Offset;
}

// Ищет первый элемент массива, который больше заданного значения
template< typename type >
AN_FORCEINLINE int BinSearchGreater( const type * _Array, const int _ArrayLength, const type & _Value ) {
    int Len = _ArrayLength;
    int Mid = Len;
    int Offset = 0;
    int Res = 0;
    while ( Mid > 0 ) {
        Mid = Len >> 1;
        if ( _Array[Offset+Mid] > _Value ) {
            Res = 0;
        } else {
            Offset += Mid;
            Res = 1;
        }
        Len -= Mid;
    }
    return Offset+Res;
}

// Ищет первый элемент массива, который больше или равен заданному значению
template< typename type >
AN_FORCEINLINE int BinSearchGequal( const type * _Array, const int _ArrayLength, const type & _Value ) {
    int Len = _ArrayLength;
    int Mid = Len;
    int Offset = 0;
    int Res = 0;
    while ( Mid > 0 ) {
        Mid = Len >> 1;
        if ( _Array[Offset+Mid] >= _Value ) {
            Res = 0;
        } else {
            Offset += Mid;
            Res = 1;
        }
        Len -= Mid;
    }
    return Offset+Res;
}

}

