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

#include <Engine/Core/Public/Integer.h>

#include "_math.h"

SignedByte & SignedByte::FromString( const char * _String ) {
    Value = StringToInt< int8_t >( _String );
    return *this;
}

Byte & Byte::FromString( const char * _String ) {
    Value = StringToInt< uint8_t >( _String );
    return *this;
}

Short & Short::FromString( const char * _String ) {
    Value = StringToInt< int16_t >( _String );
    return *this;
}

UShort & UShort::FromString( const char * _String ) {
    Value = StringToInt< uint16_t >( _String );
    return *this;
}

Int & Int::FromString( const char * _String ) {
    Value = StringToInt< int32_t >( _String );
    return *this;
}

UInt & UInt::FromString( const char * _String ) {
    Value = StringToInt< uint32_t >( _String );
    return *this;
}

Long & Long::FromString( const char * _String ) {
    Value = StringToInt< int64_t >( _String );
    return *this;
}

ULong & ULong::FromString( const char * _String ) {
    Value = StringToInt< uint64_t >( _String );
    return *this;
}
