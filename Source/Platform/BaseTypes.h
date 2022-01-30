/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#if defined(DEBUG) || defined(_DEBUG)
#    define _CRTDBG_MAP_ALLOC
#endif

#include <cmath>
#include <cstdlib>
#include <cstddef>
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

#if defined(AN_BIG_ENDIAN)
#    define AN_ENDIAN_STRING "Big"
#endif

#if defined(AN_LITTLE_ENDIAN)
#    define AN_ENDIAN_STRING "Little"
#endif

#if !defined(AN_ENDIAN_STRING)
#    error "Unknown endianness"
#endif

#if defined _MSC_VER
#    define AN_COMPILER_STRING "Microsoft Visual C++"
#    define AN_COMPILER_MSVC
#endif

#if defined __GNUC__ && !defined __clang__
#    define AN_COMPILER_STRING "Gnu GCC"
#    define AN_COMPILER_GCC    1
#endif

#if !defined AN_COMPILER_STRING
#    define AN_COMPILER_STRING "Unknown compiler"
#endif

#if defined _XBOX || defined _XBOX_VER
#    define AN_OS_XBOX
#    define AN_OS_STRING "XBOX"
#endif

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__
#    define AN_OS_WIN32
#    if !defined AN_OS_XBOX
#        if defined _WIN64
#            define AN_OS_WIN64
#            define AN_OS_STRING "Win64"
#        else
#            if !defined AN_OS_STRING
#                define AN_OS_STRING "Win32"
#            endif
#        endif
#    endif
#endif

#if defined linux || defined __linux__
#    define AN_OS_LINUX
#    define AN_OS_STRING "Linux"
#endif

#if !defined AN_OS_STRING
#    define AN_OS_UNKNOWN
#    define AN_OS_STRING "Unknown"
#endif

#ifdef AN_COMPILER_MSVC
#    if defined(DEBUG) || defined(_DEBUG)
#        define AN_DEBUG
#    endif
#endif

#ifdef AN_COMPILER_GCC
#    if !defined(NDEBUG)
#        define AN_DEBUG
#    endif
#endif

#if !defined(AN_DEBUG)
#    define AN_RELEASE
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
#    define ANGIE_API
#    define ANGIE_TEMPLATE
#else
#    ifdef AN_COMPILER_MSVC
#        ifdef ANGIE_ENGINE_EXPORTS
#            define ANGIE_API __declspec(dllexport)
#            define ANGIE_TEMPLATE
#        else
#            define ANGIE_API      __declspec(dllimport)
#            define ANGIE_TEMPLATE extern
#        endif
#    else
#        define ANGIE_API __attribute__((visibility("default")))
#        define ANGIE_TEMPLATE
#    endif
#endif

/*

Inline defines

*/
#ifdef AN_COMPILER_MSVC
#    define AN_FORCEINLINE __forceinline
#else
#    define AN_FORCEINLINE inline __attribute__((always_inline))
#endif

#define AN_INLINE inline

/*

Deprecated code marker

*/
#ifdef AN_COMPILER_MSVC
#    define AN_DEPRECATED __declspec(deprecated)
#else
#    define AN_DEPRECATED __attribute__((__deprecated__))
#endif

/*

Function signature

*/
#if defined(AN_COMPILER_MSVC)
#    define AN_FUNCSIG __FUNCSIG__
#elif defined(__GNUC__)
#    define AN_FUNCSIG __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#    define AN_FUNCSIG __func__
#else
#    define AN_FUNCSIG "[Function name unavailable]"
#endif

/*

Suppress "Unused variable" warning

*/
#define AN_UNUSED(x) ((void)x)


/*

Misc

*/

#ifdef AN_DEBUG
#    define AN_ALLOW_ASSERTS
#endif

#ifdef AN_ALLOW_ASSERTS
#    define AN_ASSERT_(assertion, comment) ((assertion) ? static_cast<void>(0) : AssertFunction(__FILE__, __LINE__, AN_FUNCSIG, AN_STRINGIFY(assertion), comment))
extern void AssertFunction(const char* _File, int _Line, const char* _Function, const char* _Assertion, const char* _Comment);
#else
#    define AN_ASSERT_(assertion, comment) ((void)(assertion))
#endif
#define AN_ASSERT(assertion)   AN_ASSERT_(assertion, nullptr)
#define AN_STRINGIFY(text)     #text
#define AN_BIT(sh)             (1 << (sh))
#define AN_BIT64(sh)           (uint64_t(1) << (sh))
#define AN_HASBITi(v, bit_i)   ((v) & (1 << (bit_i)))
#define AN_HASBIT64i(v, bit_i) ((v) & (uint64_t(1) << (bit_i)))
#define AN_HASFLAG(v, flag)    (((v) & (flag)) == (flag))
#define AN_OFS(type, name)     offsetof(type, name) //(&(( type * )0)->name)
#define AN_ARRAY_SIZE(Array)   (sizeof(Array) / sizeof(Array[0]))

/*

Forbid to copy object

*/
#define AN_FORBID_COPY(_Class)      \
    _Class(_Class const&) = delete; \
    _Class& operator=(_Class const&) = delete;

class ANoncopyable
{
    AN_FORBID_COPY(ANoncopyable)

public:
    ANoncopyable() {}
};


/*

Singleton

*/
#define AN_SINGLETON(_Class)             \
    AN_FORBID_COPY(_Class)               \
public:                                  \
    static _Class& Inst()                \
    {                                    \
        static _Class ThisClassInstance; \
        return ThisClassInstance;        \
    }                                    \
                                         \
protected:                               \
    _Class();                            \
                                         \
private:


/*

NULL declaration

*/
#ifndef NULL
#    define NULL 0
#endif

/*

Some headers may predefine min/max macroses. So undefine it.

*/
#undef min
#undef max

/*

Base types

*/

using byte   = uint8_t;

#define AN_VALIDATE_TYPE_SIZE(a, n) static_assert(sizeof(a) == (n), "AN_VALIDATE_TYPE_SIZE");

// Check is the data types has expected size
AN_VALIDATE_TYPE_SIZE(bool, 1)
AN_VALIDATE_TYPE_SIZE(char, 1)
AN_VALIDATE_TYPE_SIZE(short, 2)
AN_VALIDATE_TYPE_SIZE(float, 4)
AN_VALIDATE_TYPE_SIZE(double, 8)
AN_VALIDATE_TYPE_SIZE(int8_t, 1)
AN_VALIDATE_TYPE_SIZE(int16_t, 2)
AN_VALIDATE_TYPE_SIZE(int32_t, 4)
AN_VALIDATE_TYPE_SIZE(int64_t, 8)
AN_VALIDATE_TYPE_SIZE(uint8_t, 1)
AN_VALIDATE_TYPE_SIZE(uint16_t, 2)
AN_VALIDATE_TYPE_SIZE(uint32_t, 4)
AN_VALIDATE_TYPE_SIZE(uint64_t, 8)
AN_VALIDATE_TYPE_SIZE(byte, 1)

// characters must be signed
#if defined(_CHAR_UNSIGNED)
#    error "signed char mismatch"
#endif

static_assert((char)-1 < 0, "signed char mismatch");


/*

Flag enum operators

*/
template <typename EnumType>
using _UNDERLYING_ENUM_T = typename std::underlying_type<EnumType>::type;

#define AN_FLAG_ENUM_OPERATORS(ENUMTYPE)                                                                                                                                                                      \
    inline ENUMTYPE&          operator|=(ENUMTYPE& a, ENUMTYPE b) { return reinterpret_cast<ENUMTYPE&>(reinterpret_cast<_UNDERLYING_ENUM_T<ENUMTYPE>&>(a) |= static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(b)); } \
    inline ENUMTYPE&          operator&=(ENUMTYPE& a, ENUMTYPE b) { return reinterpret_cast<ENUMTYPE&>(reinterpret_cast<_UNDERLYING_ENUM_T<ENUMTYPE>&>(a) &= static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(b)); } \
    inline ENUMTYPE&          operator^=(ENUMTYPE& a, ENUMTYPE b) { return reinterpret_cast<ENUMTYPE&>(reinterpret_cast<_UNDERLYING_ENUM_T<ENUMTYPE>&>(a) ^= static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(b)); } \
    inline constexpr ENUMTYPE operator|(ENUMTYPE a, ENUMTYPE b) { return static_cast<ENUMTYPE>(static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(a) | static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(b)); }                \
    inline constexpr ENUMTYPE operator&(ENUMTYPE a, ENUMTYPE b) { return static_cast<ENUMTYPE>(static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(a) & static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(b)); }                \
    inline constexpr ENUMTYPE operator^(ENUMTYPE a, ENUMTYPE b) { return static_cast<ENUMTYPE>(static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(a) ^ static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(b)); }                \
    inline constexpr ENUMTYPE operator~(ENUMTYPE a) { return static_cast<ENUMTYPE>(~static_cast<_UNDERLYING_ENUM_T<ENUMTYPE>>(a)); }

/*

enable_if_t for C++11 workaround

*/
template <bool C, class T = void>
using TStdEnableIf = typename std::enable_if<C, T>::type;

/*

Power of two compile-time check

*/
template <typename T, typename = TStdEnableIf<std::is_integral<T>::value>>
constexpr bool IsPowerOfTwo(const T _Value)
{
    return (_Value & (_Value - 1)) == 0 && _Value > 0;
}

/*

Alignment stuff

*/

template <size_t Alignment>
constexpr bool IsAligned(size_t N)
{
    static_assert(IsPowerOfTwo(Alignment), "Alignment must be power of two");
    return (N & (Alignment - 1)) == 0;
}

constexpr bool IsAligned(size_t N, size_t Alignment)
{
    return (N & (Alignment - 1)) == 0;
}

constexpr bool IsAlignedPtr(void* Ptr, size_t Alignment)
{
    return ((size_t)Ptr & (Alignment - 1)) == 0;
}

constexpr size_t IsSSEAligned(size_t N)
{
    return IsAligned<16>(N);
}

constexpr size_t Align(size_t N, size_t Alignment)
{
    return (N + (Alignment - 1)) & ~(Alignment - 1);
}

constexpr void* AlignPtr(void* Ptr, size_t Alignment)
{
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
    return (void*)(Align((size_t)Ptr, Alignment));
#endif
}
