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

#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/String.h>

#if defined AN_COMPILER_MSVC && defined AN_DEBUG
#include <Engine/Core/Public/WindowsDefs.h>
#endif

#ifdef AN_OS_ANDROID
#include <android/log.h>
#endif

FLogger GLogger;

thread_local static char LogBuffer[16384];

void FLogger::Critical( const char * _Format, ... ) {
    va_list VaList;
    va_start( VaList, _Format );
    FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    MessageCallback( LevCritical, LogBuffer );
}

void FLogger::Error( const char * _Format, ... ) {
    va_list VaList;
    va_start( VaList, _Format );
    FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    MessageCallback( LevError, LogBuffer );
}

void FLogger::Warning( const char * _Format, ... ) {
    va_list VaList;
    va_start( VaList, _Format );
    FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    MessageCallback( LevWarning, LogBuffer );
}

void FLogger::DebugMessage( const char * _Format, ... ) {
#ifdef AN_DEBUG
    va_list VaList;
    va_start( VaList, _Format );
    FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    MessageCallback( LevMessage, LogBuffer );
#endif
}

void FLogger::Printf( const char * _Format, ... ) {
    va_list VaList;
    va_start( VaList, _Format );
    FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    MessageCallback( LevMessage, LogBuffer );
}

void FLogger::Print( const char * _Message ) {
    MessageCallback( LevMessage, _Message );
}

void FLogger::_Printf( int _Level, const char * _Format, ... ) {
    va_list VaList;
    va_start( VaList, _Format );
    FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
    va_end( VaList );
    MessageCallback( _Level, LogBuffer );
}

void FLogger::SetMessageCallback( void (*_MessageCallback)( int, const char * ) ) {
    MessageCallback = _MessageCallback;
}

void FLogger::DefaultMessageCallback( int, const char * _Message ) {
#if defined AN_COMPILER_MSVC && defined AN_DEBUG
    OutputDebugStringA( _Message );
#else
#ifdef AN_OS_ANDROID
    __android_log_print( ANDROID_LOG_INFO, "Angie Engine", _Message );
#else
    fprintf( stdout, "%s", _Message );
    fflush( stdout );
#endif
#endif
}