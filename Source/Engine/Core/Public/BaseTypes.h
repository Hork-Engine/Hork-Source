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

#if defined( DEBUG ) || defined( _DEBUG )
#   define _CRTDBG_MAP_ALLOC
#endif

#include <cmath>
#include <cstdlib>
#include <cassert>
#include <ctype.h>
#include <memory>
#include <string>
#include <string.h>
#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <functional>
#include <type_traits>

/*

Predefines:
AN_DEBUG           - debug compilation
AN_RELEASE         - release compilation
AN_ENDIAN_STRING   - Big/Little endian (string)
AN_BIG_ENDIAN      - Big endian platform
AN_LITTLE_ENDIAN   - Little endian platform
AN_COMPILER_STRING - Compilator name (string)
AN_COMPILER_MSVC   - Microsoft Visual Studio compiler
AN_COMPILER_GCC    - Gnu GCC compiler
AN_OS_WIN32        - Windows platform (Win32 or Win64)
AN_OS_WIN64        - 64-bit Windows platform
AN_OS_XBOX         - XBOX platform
AN_OS_LINUX        - Linux platform
AN_OS_UNKNOWN      - Unknown platform
AN_OS_STRING       - Operating system name (string)

*/

#if defined( AN_BIG_ENDIAN )
#   define AN_ENDIAN_STRING "Big"
#endif

#if defined( AN_LITTLE_ENDIAN )
#   define AN_ENDIAN_STRING "Little"
#endif

#if !defined( AN_ENDIAN_STRING )
#   error "Unknown endianness"
#endif

#if defined _MSC_VER
#   define AN_COMPILER_STRING "Microsoft Visual C++"
#   define AN_COMPILER_MSVC
#endif

#if defined __GNUC__ && !defined __clang__
#   define AN_COMPILER_STRING "Gnu GCC"
#   define AN_COMPILER_GCC 1
#endif

#if !defined AN_COMPILER_STRING
#   define AN_COMPILER_STRING "Unknown compiler"
#endif

#if defined _XBOX || defined _XBOX_VER
#   define AN_OS_XBOX
#   define AN_OS_STRING "XBOX"
#endif

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__
#   define AN_OS_WIN32
#   if !defined AN_OS_XBOX
#       if defined _WIN64
#           define AN_OS_WIN64
#           define AN_OS_STRING "Win64"
#       else
#           if !defined AN_OS_STRING
#               define AN_OS_STRING "Win32"
#           endif
#       endif
#   endif
#endif

#if defined linux || defined __linux__
#   define AN_OS_LINUX
#   define AN_OS_STRING "Linux"
#endif

#if !defined AN_OS_STRING
#   define AN_OS_UNKNOWN
#   define AN_OS_STRING "Unknown"
#endif

#ifdef AN_COMPILER_MSVC
#   if defined( DEBUG ) || defined( _DEBUG )
#       define AN_DEBUG
#   endif
#endif

#ifdef AN_COMPILER_GCC
#   if !defined( NDEBUG )
#       define AN_DEBUG
#   endif
#endif

#if !defined( AN_DEBUG )
#   define AN_RELEASE
#endif

/*

Function call

*/
#define AN_STDCALL __stdcall

/*

Library import/export switch

ANGIE_STATIC_LIBRARY - Is static library
ANGIE_ENGINE_EXPORTS - Export/Import switch
ANGIE_API            - Export or Import class/function
ANGIE_TEMPLATE       - Export or Import class template

*/
#ifdef ANGIE_STATIC_LIBRARY
#   define ANGIE_API
#   define ANGIE_TEMPLATE
#else
#   ifdef AN_COMPILER_MSVC
#       ifdef ANGIE_ENGINE_EXPORTS
#           define ANGIE_API __declspec(dllexport)
#           define ANGIE_TEMPLATE
#       else
#           define ANGIE_API __declspec(dllimport)
#           define ANGIE_TEMPLATE extern
#       endif
#   else
#       define ANGIE_API __attribute__ ((visibility("default")))
#       define ANGIE_TEMPLATE
#   endif
#endif

/*

Microsoft Visual Studio versions

*/
#define AN_MSVC_2003   1310    // Visual Studio 2003
#define AN_MSVC_2005   1400    // Visual Studio 2005
#define AN_MSVC_2008   1500    // Visual Studio 2008
#define AN_MSVC_2010   1600    // Visual Studio 2010
#define AN_MSVC_2012   1700    // Visual Studio 2012
#define AN_MSVC_2013   1800    // Visual Studio 2013

/*

Inline defines

*/
#ifdef AN_COMPILER_MSVC
#   define AN_FORCEINLINE  __forceinline
#else
#   define AN_FORCEINLINE  inline __attribute__((always_inline))
#endif

#define AN_INLINE       inline

/*

Deprecated code marker

*/
#ifdef AN_COMPILER_MSVC
#   define AN_DEPRECATED __declspec(deprecated)
#else
#   define AN_DEPRECATED __attribute__((__deprecated__))
#endif

/*

Function signature

*/
#if defined( AN_COMPILER_MSVC )
#   define AN_FUNCSIG __FUNCSIG__
#elif defined( __GNUC__ )
#   define AN_FUNCSIG __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#   define AN_FUNCSIG __func__
#else
#   define AN_FUNCSIG "[Function name unavailable]"
#endif

/*

Suppress "Unused variable" warning

*/
#define AN_UNUSED(x)    ((void)x)


/*

Misc

*/

#ifdef AN_DEBUG
#   define AN_ALLOW_ASSERTS
#endif

#ifdef AN_ALLOW_ASSERTS
#   define AN_ASSERT_(assertion,comment)  ((assertion) ? static_cast<void>(0) : AssertFunction(__FILE__,__LINE__,AN_FUNCSIG,AN_STRINGIFY(assertion),comment))
    extern void AssertFunction( const char * _File, int _Line, const char * _Function, const char * _Assertion, const char * _Comment );
#else
#   define AN_ASSERT_(assertion,comment)   ((void)(assertion))
#endif
#define AN_ASSERT(assertion)            AN_ASSERT_(assertion,nullptr)
#define AN_STRINGIFY(text)              #text
#define AN_BIT(sh)                      (1<<(sh))
#define AN_BIT64(sh)                    (uint64_t(1)<<(sh))
#define AN_HASBITi(v,bit_i)             ((v) & (1<<(bit_i)))
#define AN_HASBIT64i(v,bit_i)           ((v) & (uint64_t(1)<<(bit_i)))
#define AN_HASFLAG(v,flag)              (((v) & (flag)) == (flag))
#define AN_OFS(type, name)              offsetof( type, name )//(&(( type * )0)->name)
#define AN_ARRAY_SIZE( Array )          ( sizeof( Array ) / sizeof( Array[0] ) )

/*

Forbid to copy object

*/
#define AN_FORBID_COPY( _Class ) \
    _Class( const _Class & ) = delete; \
    _Class & operator=( const _Class & ) = delete;


/*

Singleton

*/
#define AN_SINGLETON( _Class ) \
    AN_FORBID_COPY( _Class ) \
    public:\
    static _Class & Inst() { \
        static _Class ThisClassInstance; \
        return ThisClassInstance; \
    } \
    protected: _Class();\
    private:


/*

NULL declaration

*/
#ifndef NULL
#   define NULL 0
#endif

/*

Some headers may predefine min/max macroses. So undefine it.

*/
#undef min
#undef max

/*

Base types

*/

typedef unsigned char byte;
typedef int16_t  word;
typedef int32_t  dword;
typedef int64_t  ddword;

#define AN_SIZEOF_STATIC_CHECK( a, n ) static_assert( sizeof( a ) == (n), "AN_SIZEOF_STATIC_CHECK" );

// Check is the data types has expected size
AN_SIZEOF_STATIC_CHECK( bool, 1 )
AN_SIZEOF_STATIC_CHECK( char, 1 )
AN_SIZEOF_STATIC_CHECK( short, 2 )
AN_SIZEOF_STATIC_CHECK( float, 4 )
AN_SIZEOF_STATIC_CHECK( double, 8 )
AN_SIZEOF_STATIC_CHECK( int8_t, 1 )
AN_SIZEOF_STATIC_CHECK( int16_t, 2 )
AN_SIZEOF_STATIC_CHECK( int32_t, 4 )
AN_SIZEOF_STATIC_CHECK( int64_t, 8 )
AN_SIZEOF_STATIC_CHECK( uint8_t, 1 )
AN_SIZEOF_STATIC_CHECK( uint16_t, 2 )
AN_SIZEOF_STATIC_CHECK( uint32_t, 4 )
AN_SIZEOF_STATIC_CHECK( uint64_t, 8 )
AN_SIZEOF_STATIC_CHECK( byte, 1 )
AN_SIZEOF_STATIC_CHECK( word, 2 )
AN_SIZEOF_STATIC_CHECK( dword, 4 )
AN_SIZEOF_STATIC_CHECK( ddword, 8 )

// characters must be signed
#if defined( _CHAR_UNSIGNED )
#error "signed char mismatch"
#endif

static_assert( (char)-1 < 0, "signed char mismatch" );

/*

enable_if_t for C++11 workaround

*/
template< bool C, class T = void >
using TStdEnableIf = typename std::enable_if< C, T >::type;

/*

Power of two compile-time check

*/
template< typename T, typename = TStdEnableIf< std::is_integral<T>::value > >
constexpr bool IsPowerOfTwo( const T _Value ) {
    return (_Value & (_Value - 1)) == 0 && _Value > 0;
}

/*

Alignment stuff

*/

template< size_t Alignment >
constexpr bool IsAligned( size_t N ) {
    static_assert( IsPowerOfTwo( Alignment ), "Alignment must be power of two" );
    return (N & (Alignment - 1)) == 0;
}

constexpr bool IsAligned( size_t N, size_t Alignment ) {
    return (N & (Alignment - 1)) == 0;
}

constexpr bool IsAlignedPtr( void * Ptr, size_t Alignment ) {
    return ((size_t)Ptr & (Alignment - 1)) == 0;
}

constexpr size_t IsSSEAligned( size_t N ) {
    return IsAligned< 16 >( N );
}

constexpr size_t Align( size_t N, size_t Alignment ) {
    return ( N + ( Alignment - 1 ) ) & ~( Alignment - 1 );
}

constexpr void * AlignPtr( void * Ptr, size_t Alignment ) {
#if 0
    struct SAligner {
        union {
            void * p;
            size_t i;
        };
    };
    SAligner aligner;
    aligner.p = _UnalignedPtr;
    aligner.i = Align( aligner.i, _Alignment );
    return aligner.p;
#else
    return ( void * )( Align( (size_t)Ptr, Alignment ) );
#endif
}
