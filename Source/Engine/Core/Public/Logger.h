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
#include "Std.h"

enum ELoggerLevel {
    LevCritical,
    LevError,
    LevWarning,
    LevMessage
};

class ALogger {
public:
    void Critical( const char * _Format, ... );
    void Error( const char * _Format, ... );
    void Warning( const char * _Format, ... );
    void DebugMessage( const char * _Format, ... );
    void Printf( const char * _Format, ... );
    void Print( const char * _Message );
    void _Printf( int _Level, const char * _Format, ... );

    template< typename... TArgs >
    void operator()( const char * _Format, TArgs... _Args ) {
        Printf( _Format, StdForward< TArgs >( _Args )... );
    }

    void SetMessageCallback( void (*_MessageCallback)( int, const char * ) );

private:
    void (*MessageCallback)( int, const char * ) = DefaultMessageCallback;

    static void DefaultMessageCallback( int, const char * );
};

extern ALogger GLogger;
