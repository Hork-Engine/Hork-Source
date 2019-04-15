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

#define _CRTDBG_MAP_ALLOC

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

#if defined( DEBUG ) || defined( _DEBUG )
#ifndef AN_DEBUG
#define AN_DEBUG
#endif
#endif

#if !defined( AN_DEBUG )
#define AN_RELEASE
#endif

#if defined( AN_BIG_ENDIAN )
#define AN_ENDIAN_STRING "Big"
#endif

#if defined( AN_LITTLE_ENDIAN )
#define AN_ENDIAN_STRING "Little"
#endif

#if !defined( AN_ENDIAN_STRING )
#error "Unknown endianness"
#endif

#if defined _MSC_VER
#  define AN_COMPILER_STRING "Microsoft Visual C++"
#  define AN_COMPILER_MSVC
#endif

#if defined __GNUC__ && !defined __clang__
#  define AN_COMPILER_STRING "Gnu GCC"
#  define AN_COMPILER_GCC 1
#endif

#if !defined AN_COMPILER_STRING
#  define AN_COMPILER_STRING "Unknown compiler"
#endif

#if defined _XBOX || defined _XBOX_VER
#  define AN_OS_XBOX
#  define AN_OS_STRING "XBOX"
#endif

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__
#  define AN_OS_WIN32
#  if !defined AN_OS_XBOX
#     if defined _WIN64
#        define AN_OS_WIN64
#        define AN_OS_STRING "Win64"
#     else
#        if !defined AN_OS_STRING
#           define AN_OS_STRING "Win32"
#        endif
#     endif
#  endif
#endif

#if defined linux || defined __linux__
#  define AN_OS_LINUX
#  define AN_OS_STRING "Linux"
#endif

#if !defined AN_OS_STRING
#  define AN_OS_UNKNOWN
#  define AN_OS_STRING "Unknown"
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
#define ANGIE_API
#define ANGIE_TEMPLATE
#else
#ifdef AN_COMPILER_MSVC
#ifdef ANGIE_ENGINE_EXPORTS
#define ANGIE_API __declspec(dllexport)
#define ANGIE_TEMPLATE
#else
#define ANGIE_API __declspec(dllimport)
#define ANGIE_TEMPLATE extern
#endif
#else
#define ANGIE_API __attribute__ ((visibility("default")))
#define ANGIE_TEMPLATE
#endif
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

Disable some warnings
Move this to CMakeLists.txt?

*/
#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4996 )   // warning: deprecated functions
#pragma warning( disable : 4018 )   // warning: signed/unsigned mismatch
#pragma warning( disable : 4244 )   // warning: type conversion, possible loss of data
#pragma warning( disable : 4267 )   // warning: type conversion, possible loss of data
#pragma warning( disable : 4251 )   // warning: ...needs to have dll-interface to be used by clients of class...
#pragma warning( disable : 4505 )   // warning: unused local function
#pragma warning( disable : 4800 )
#endif

#ifdef AN_COMPILER_GCC
#pragma GCC diagnostic ignored "-Wsign-compare"    // warning: comparison between signed and unsigned integer
#pragma GCC diagnostic ignored "-Wunused-function" // warning: unused local function
#pragma GCC diagnostic ignored "-Wstrict-aliasing" // warning: strict-aliasing rules
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

/*

Inline defines

*/
#ifdef AN_COMPILER_MSVC
#define AN_FORCEINLINE  __forceinline
#else
#define AN_FORCEINLINE  inline __attribute__((always_inline))
#endif
#define AN_INLINE       inline

/*

Deprecated code marker

*/
#ifdef AN_COMPILER_MSVC
#define AN_DEPRECATED __declspec(deprecated)
#else
#define AN_DEPRECATED __attribute__((__deprecated__))
#endif

/*

Function signature

*/
#if defined( AN_COMPILER_MSVC )
#define AN_FUNCSIG __FUNCSIG__
#elif defined( __GNUC__ )
#define AN_FUNCSIG __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#define AN_FUNCSIG __func__
#else
#define AN_FUNCSIG "[Function name unavailable]"
#endif

/*

"Unused variable" warning suppress

*/
#define AN_UNUSED(x)    ((void)x)

/*

Memory alignment

*/
#ifdef AN_COMPILER_MSVC
#define AN_ALIGN16( _StructName, _StructBlock ) __declspec(align(16)) struct _StructName _StructBlock;
#define AN_ALIGN16_ADJ( _MaxSize, _StructName, _StructBlock ) __declspec(align(16)) struct _StructName _StructBlock; static_assert( sizeof( _StructName ) <= _MaxSize, "Max size overflow" );
#define AN_ALIGN16_TYPEDEF( _Type, _Name ) typedef __declspec(align(16)) _Type _Name
#define AN_ALIGN( x ) __declspec(align(x))
#else
#define AN_ALIGN16( _StructName, _StructBlock ) struct _StructName _StructBlock __attribute__ ((aligned (16)));
#define AN_ALIGN16_ADJ( _MaxSize, _StructName, _StructBlock ) struct _StructName _StructBlock __attribute__ ((aligned (16))); static_assert( sizeof( _StructName ) <= _MaxSize, "Max size overflow" );
#define AN_ALIGN16_TYPEDEF( _Type, _Name) typedef _Type _Name __attribute__((aligned(16)))
#define AN_ALIGN( x ) __attribute__((aligned(x)))
#endif
#define AN_ALIGN_SSE AN_ALIGN( 16 )

#include "CriticalError.h"

/*

Misc

*/

#if 1
#define AN_Assert(expression) \
    assert(expression)
#else
    if ( !(expression) ) { \
        CriticalError( "Assertion Failed on line %d, file %s.\nExpression %s.\n", __LINE__, __FILE__, AN_STRINGIFY(expression) ); \
    }
#endif
//#define AN_ASSERT_(expression)          assert(expression)
#define AN_ASSERT(expression,text)      AN_Assert((expression) && (text))
#define AN_STRINGIFY(text)              #text
#define AN_BIT(sh)                      (1<<(sh))
#define AN_BIT64(sh)                    (uint64_t(1)<<(sh))
#define AN_HASBITi(v,bit_i)             ((v) & (1<<(bit_i)))
#define AN_HASBIT64i(v,bit_i)           ((v) & (uint64_t(1)<<(bit_i)))
#define AN_HASFLAG(v,flag)              (((v) & (flag)) == (flag))
#define AN_OFS(type, name)              (&(( type * )0)->name)
#define AN_ARRAY_LENGTH( Array )        ( sizeof( Array ) / sizeof( Array[0] ) )

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
#define NULL 0
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

typedef unsigned short FWideChar;

namespace FCore {

    // Std swap function wrapper
    template< typename type >
    AN_FORCEINLINE void SwapArgs( type & _Arg1, type & _Arg2 ) {
        std::swap( _Arg1, _Arg2 );
    }

}

/*

Power of two compile-time check

*/
template< typename integral_type >
constexpr bool IsPowerOfTwoConstexpr( const integral_type & _value ) { return (_value & (_value - 1)) == 0 && _value > 0; }

/*

enable_if_t for C++11 workaround

*/
template< bool C, class T = void >
using TEnableIf = typename std::enable_if< C, T >::type;

/*

STD wrappers

*/
#define FMath_Max   std::max
#define FMath_Min   std::min
#define TVector     std::vector
#define TFunction   std::function
#define StdSort     std::sort
#define StdFind     std::find
#define StdChrono   std::chrono
#define StdSqrt     std::sqrt
#define StdForward  std::forward

/*

TCallback

Template callback class

*/
class FDummy;

template< typename T >
struct TCallback;

template< typename TReturn, typename... TArgs >
struct TCallback < TReturn( TArgs... ) > {
    TCallback()
        : Object( nullptr )
    {
    }

    template< typename T >
    TCallback( T * _Object, TReturn ( T::*_Method )(TArgs...) )
        : Object( _Object )
        , Method( (void (FDummy::*)(TArgs...))_Method )
    {
    }

    template< typename T >
    void Initialize( T * _Object, TReturn ( T::*_Method )(TArgs...) ) {
        Object = _Object;
        Method = (void (FDummy::*)(TArgs...))_Method;
    }

    bool IsInitialized() const {
        return Object != nullptr;
    }

    void Clear() {
        Object = nullptr;
    }

    TReturn operator()( TArgs... _Args ) const {
        AN_Assert( IsInitialized() );
        return (Object->*Method)(StdForward< TArgs >( _Args )...);
    }

private:
    FDummy * Object;
    TReturn ( FDummy::*Method )(TArgs...);
};
