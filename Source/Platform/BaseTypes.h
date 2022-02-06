/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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
HK_DEBUG           - debug compilation
HK_RELEASE         - release compilation
HK_ENDIAN_STRING   - Big/Little endian (string)
HK_BIG_ENDIAN      - Big endian platform
HK_LITTLE_ENDIAN   - Little endian platform
HK_COMPILER_STRING - Compilator name (string)
HK_COMPILER_MSVC   - Microsoft Visual Studio compiler
HK_COMPILER_GCC    - Gnu GCC compiler
HK_OS_WIN32        - Windows platform (Win32 or Win64)
HK_OS_WIN64        - 64-bit Windows platform
HK_OS_XBOX         - XBOX platform
HK_OS_LINUX        - Linux platform
HK_OS_UNKNOWN      - Unknown platform
HK_OS_STRING       - Operating system name (string)

*/

#if defined(HK_BIG_ENDIAN)
#    define HK_ENDIAN_STRING "Big"
#endif

#if defined(HK_LITTLE_ENDIAN)
#    define HK_ENDIAN_STRING "Little"
#endif

#if !defined(HK_ENDIAN_STRING)
#    error "Unknown endianness"
#endif

#if defined _MSC_VER
#    define HK_COMPILER_STRING "Microsoft Visual C++"
#    define HK_COMPILER_MSVC
#endif

#if defined __GNUC__ && !defined __clang__
#    define HK_COMPILER_STRING "Gnu GCC"
#    define HK_COMPILER_GCC    1
#endif

#if !defined HK_COMPILER_STRING
#    define HK_COMPILER_STRING "Unknown compiler"
#endif

#if defined _XBOX || defined _XBOX_VER
#    define HK_OS_XBOX
#    define HK_OS_STRING "XBOX"
#endif

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__
#    define HK_OS_WIN32
#    if !defined HK_OS_XBOX
#        if defined _WIN64
#            define HK_OS_WIN64
#            define HK_OS_STRING "Win64"
#        else
#            if !defined HK_OS_STRING
#                define HK_OS_STRING "Win32"
#            endif
#        endif
#    endif
#endif

#if defined linux || defined __linux__
#    define HK_OS_LINUX
#    define HK_OS_STRING "Linux"
#endif

#if !defined HK_OS_STRING
#    define HK_OS_UNKNOWN
#    define HK_OS_STRING "Unknown"
#endif

#ifdef HK_COMPILER_MSVC
#    if defined(DEBUG) || defined(_DEBUG)
#        define HK_DEBUG
#    endif
#endif

#ifdef HK_COMPILER_GCC
#    if !defined(NDEBUG)
#        define HK_DEBUG
#    endif
#endif

#if !defined(HK_DEBUG)
#    define HK_RELEASE
#endif

/*

Function call

*/
#define HK_STDCALL __stdcall

/*

Inline defines

*/
#ifdef HK_COMPILER_MSVC
#    define HK_FORCEINLINE __forceinline
#else
#    define HK_FORCEINLINE inline __attribute__((always_inline))
#endif

#define HK_INLINE inline

/*

Deprecated code marker

*/
#ifdef HK_COMPILER_MSVC
#    define HK_DEPRECATED __declspec(deprecated)
#else
#    define HK_DEPRECATED __attribute__((__deprecated__))
#endif

#define HK_NODISCARD [[nodiscard]]

/*

Function signature

*/
#if defined(HK_COMPILER_MSVC)
#    define HK_FUNCSIG __FUNCSIG__
#elif defined(__GNUC__)
#    define HK_FUNCSIG __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#    define HK_FUNCSIG __func__
#else
#    define HK_FUNCSIG "[Function name unavailable]"
#endif

/*

Suppress "Unused variable" warning

*/
#define HK_UNUSED(x) ((void)x)


/*

Misc

*/

#ifdef HK_DEBUG
#    define HK_ALLOW_ASSERTS
#endif

#ifdef HK_ALLOW_ASSERTS
#    define HK_ASSERT_(assertion, comment) ((assertion) ? static_cast<void>(0) : AssertFunction(__FILE__, __LINE__, HK_FUNCSIG, HK_STRINGIFY(assertion), comment))
extern void AssertFunction(const char* _File, int _Line, const char* _Function, const char* _Assertion, const char* _Comment);
#else
#    define HK_ASSERT_(assertion, comment)
#endif
#define HK_ASSERT(assertion)   HK_ASSERT_(assertion, nullptr)
#define HK_STRINGIFY(text)     #text
#define HK_BIT(sh)             (1 << (sh))
#define HK_BIT64(sh)           (uint64_t(1) << (sh))
#define HK_HASBITi(v, bit_i)   ((v) & (1 << (bit_i)))
#define HK_HASBIT64i(v, bit_i) ((v) & (uint64_t(1) << (bit_i)))
#define HK_HASFLAG(v, flag)    (((v) & (flag)) == (flag))
#define HK_OFS(type, name)     offsetof(type, name) //(&(( type * )0)->name)
#define HK_ARRAY_SIZE(Array)   (sizeof(Array) / sizeof(Array[0]))

/*

Forbid to copy object

*/
#define HK_FORBID_COPY(_Class)      \
    _Class(_Class const&) = delete; \
    _Class& operator=(_Class const&) = delete;

class ANoncopyable
{
    HK_FORBID_COPY(ANoncopyable)

public:
    ANoncopyable() {}
};


/*

Some headers may predefine min/max macroses. So undefine it.

*/
#undef min
#undef max

/*

Base types

*/

using byte   = uint8_t;

#define HK_VALIDATE_TYPE_SIZE(a, n) static_assert(sizeof(a) == (n), "HK_VALIDATE_TYPE_SIZE");

// Check is the data types has expected size
HK_VALIDATE_TYPE_SIZE(bool, 1)
HK_VALIDATE_TYPE_SIZE(char, 1)
HK_VALIDATE_TYPE_SIZE(short, 2)
HK_VALIDATE_TYPE_SIZE(float, 4)
HK_VALIDATE_TYPE_SIZE(double, 8)
HK_VALIDATE_TYPE_SIZE(int8_t, 1)
HK_VALIDATE_TYPE_SIZE(int16_t, 2)
HK_VALIDATE_TYPE_SIZE(int32_t, 4)
HK_VALIDATE_TYPE_SIZE(int64_t, 8)
HK_VALIDATE_TYPE_SIZE(uint8_t, 1)
HK_VALIDATE_TYPE_SIZE(uint16_t, 2)
HK_VALIDATE_TYPE_SIZE(uint32_t, 4)
HK_VALIDATE_TYPE_SIZE(uint64_t, 8)
HK_VALIDATE_TYPE_SIZE(byte, 1)

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

#define HK_FLAG_ENUM_OPERATORS(ENUMTYPE)                                                                                                                                                                      \
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
