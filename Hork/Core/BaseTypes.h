/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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
//#    define _CRTDBG_MAP_ALLOC
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

#ifdef __GNUC__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include <EASTL/internal/config.h>
#include <EASTL/internal/hashtable.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_function.h>

#ifdef __GNUC__
#    pragma GCC diagnostic pop
#endif

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


#define HK_NAMESPACE_BEGIN \
    namespace Hk           \
    {

#define HK_NAMESPACE_END }


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

#ifndef HK_LIKELY
#    if defined(__GNUC__) && (__GNUC__ >= 3)
#        define HK_LIKELY(x)      __builtin_expect(!!(x), true)
#        define HK_UNLIKELY(x) __builtin_expect(!!(x), false)
#    else
#        define HK_LIKELY(x)   (x)
#        define HK_UNLIKELY(x) (x)
#    endif
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
#define HK_MAYBE_UNUSED(x) ((void)x)

/*

Misc

*/

#ifdef HK_DEBUG
#    define HK_ALLOW_ASSERTS
#endif

#ifdef HK_ALLOW_ASSERTS
#    define HK_ASSERT_(assertion, comment) ((assertion) ? static_cast<void>(0) : ::Hk::AssertFunction(__FILE__, __LINE__, HK_FUNCSIG, HK_STRINGIFY(assertion), comment))

HK_NAMESPACE_BEGIN
extern void AssertFunction(const char* file, int line, const char* function, const char* assertion, const char* comment);
extern void TerminateWithError(const char* message);
HK_NAMESPACE_END

#else
#    define HK_ASSERT_(assertion, comment)
#endif
#define HK_ASSERT(assertion)   HK_ASSERT_(assertion, nullptr)

#define HK_IF_ASSERT(x) \
    HK_ASSERT(x);       \
    if (x)

#define HK_IF_NOT_ASSERT(x) \
    HK_ASSERT(x);       \
    if (!(x))

#define HK_VERIFY(Expression, Message)                               \
    do                                                               \
    {                                                                \
        if (HK_UNLIKELY(!(Expression)))                              \
            Hk::TerminateWithError(HK_FORMAT("{} Expected {}\n", Message, #Expression).ToPtr()); \
    } while (false)

#define HK_VERIFY_R(Expression, Message)                   \
    do                                                     \
    {                                                      \
        if (HK_UNLIKELY(!(Expression)))                    \
        {                                                  \
            Hk::LOG("{} Expected {}\n", Message, #Expression); \
            return {};                                     \
        }                                                  \
    } while (false)

#define HK_STRINGIFY(text)     #text
#define HK_BIT(sh)             (1u << (sh))
#define HK_BIT64(sh)           (uint64_t(1) << (sh))
#define HK_HASBITi(v, bit_i)   ((v) & (1u << (bit_i)))
#define HK_HASBIT64i(v, bit_i) ((v) & (uint64_t(1) << (bit_i)))
#define HK_HASFLAG(v, flag)    (((v) & (flag)) == (flag))
#define HK_OFS(type, name)     offsetof(type, name) //(&(( type * )0)->name)
#define HK_ARRAY_SIZE(Array)   (sizeof(Array) / sizeof(Array[0]))

#define HK_NARG(...)  HK_NARG_(__VA_ARGS__, HK_RSEQ_N())
#define HK_NARG_(...) HK_ARG_N(__VA_ARGS__)

#define HK_ARG_N(                                     \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,          \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, N, ...) N

#define HK_RSEQ_N()                             \
    63, 62, 61, 60,                             \
        59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0


#define _HK_CONCAT(a, b) a##b
#define HK_CONCAT(a, b)  _HK_CONCAT(a, b)

#define HK_NAME_GEN(prefix) HK_CONCAT(prefix, __COUNTER__)

#define HK_FIND_METHOD(Method) \
    template <typename T, typename = int> \
    struct __Has##Method : std::false_type { }; \
    template <typename T> \
    struct __Has##Method<T, decltype(&T::Method, 0)> : std::true_type { };

#define HK_HAS_METHOD(Class, Method) \
    __Has##Method<Class>::value

HK_NAMESPACE_BEGIN
class Noncopyable
{
public:
                    Noncopyable() = default;
                    Noncopyable(Noncopyable const&) = delete;

    Noncopyable&    operator=(Noncopyable const&) = delete;
};
HK_NAMESPACE_END

/*

Some headers may predefine min/max macroses. So undefine it.

*/
#undef min
#undef max

/*

Base types

*/

HK_NAMESPACE_BEGIN
using byte   = uint8_t;
HK_NAMESPACE_END

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
HK_VALIDATE_TYPE_SIZE(Hk::byte, 1)
HK_VALIDATE_TYPE_SIZE(eastl_size_t, sizeof(size_t))

// characters must be signed
#if defined(_CHAR_UNSIGNED)
#    error "signed char mismatch"
#endif

static_assert((char)-1 < 0, "signed char mismatch");


/*

Flag enum operators

*/
#define HK_FLAG_ENUM_OPERATORS(Enum)                                                                                                                                                                      \
    inline Enum&          operator|=(Enum& a, Enum b) { return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(a) |= static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum&          operator&=(Enum& a, Enum b) { return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(a) &= static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline Enum&          operator^=(Enum& a, Enum b) { return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(a) ^= static_cast<std::underlying_type_t<Enum>>(b)); } \
    inline constexpr Enum operator|(Enum a, Enum b) { return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(a) | static_cast<std::underlying_type_t<Enum>>(b)); }                \
    inline constexpr Enum operator&(Enum a, Enum b) { return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(a) & static_cast<std::underlying_type_t<Enum>>(b)); }                \
    inline constexpr Enum operator^(Enum a, Enum b) { return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(a) ^ static_cast<std::underlying_type_t<Enum>>(b)); }                \
    inline constexpr Enum operator~(Enum a) { return static_cast<Enum>(~static_cast<std::underlying_type_t<Enum>>(a)); } \
    inline constexpr bool operator!(Enum a) { return a == static_cast<Enum>(0); }

HK_NAMESPACE_BEGIN

template <typename Enum>
constexpr std::underlying_type_t<Enum> ToUnderlying(Enum in) { return static_cast<std::underlying_type_t<Enum>>(in); }

/// Power of two compile-time check
template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr bool IsPowerOfTwo(const T value)
{
    return (value & (value - 1)) == 0 && value > 0;
}

template <size_t Alignment>
constexpr bool IsAligned(size_t n)
{
    static_assert(IsPowerOfTwo(Alignment), "Alignment must be power of two");
    return (n & (Alignment - 1)) == 0;
}

constexpr bool IsAligned(size_t n, size_t alignment)
{
    return (n & (alignment - 1)) == 0;
}

HK_FORCEINLINE bool IsAlignedPtr(const void* ptr, size_t alignment)
{
    return (reinterpret_cast<size_t>(ptr) & (alignment - 1)) == 0;
}

constexpr size_t IsSSEAligned(size_t n)
{
    return IsAligned<16>(n);
}

constexpr size_t Align(size_t n, size_t alignment)
{
    return (n + (alignment - 1)) & ~(alignment - 1);
}

HK_FORCEINLINE void* AlignPtr(void* ptr, size_t alignment)
{
    return reinterpret_cast<void*>(Align(reinterpret_cast<size_t>(ptr), alignment));
}

namespace Core
{
template <typename T>
HK_FORCEINLINE void Swap(T& a, T& b)
{
    eastl::swap(a, b);
}

} // namespace Core

HK_NAMESPACE_END
