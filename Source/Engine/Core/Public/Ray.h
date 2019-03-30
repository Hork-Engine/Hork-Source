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

#include "Float.h"

class SegmentF final {
public:
    Float3 Start;
    Float3 End;

    SegmentF() = default;
    SegmentF( const Float3 & _Start, const Float3 & _End ) : Start(_Start), End(_End) {}

    SegmentF & operator=( const SegmentF & _Other ) {
        Start = _Other.Start;
        End = _Other.End;
        return *this;
    }

    Bool operator==( const SegmentF & _Other ) const { return Start ==  _Other.Start && End == _Other.End; }
    Bool operator!=( const SegmentF & _Other ) const { return Start !=  _Other.Start || End != _Other.End; }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Start << End;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Start;
        _Stream >> End;
    }
};

class RayF final {
public:
    Float3 Start;
    Float3 Dir;

    RayF() = default;
    RayF( const Float3 & _Start, const Float3 & _Dir ) : Start(_Start), Dir(_Dir) {}

    RayF & operator=( const RayF & _Other ) {
        Start = _Other.Start;
        Dir = _Other.Dir;
        return *this;
    }

    Bool operator==( const RayF & _Other ) const { return Start ==  _Other.Start && Dir == _Other.Dir; }
    Bool operator!=( const RayF & _Other ) const { return Start !=  _Other.Start || Dir != _Other.Dir; }

    void FromSegment( const SegmentF & _Segment ) {
        Start = _Segment.Start;
        Dir = ( _Segment.End - _Segment.Start ).Normalized();
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Start << Dir;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Start;
        _Stream >> Dir;
    }
};
