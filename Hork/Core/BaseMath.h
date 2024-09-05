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

#include <Hork/Core/BaseTypes.h>

#include <emmintrin.h>

HK_NAMESPACE_BEGIN

namespace Math
{

template <typename T>
constexpr int BitsCount()
{
    return sizeof(T) * 8;
}

template <typename T>
HK_FORCEINLINE T Abs(T value)
{
    return std::abs(value);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE constexpr T Dist(T a, T b)
{
    // NOTE: We don't use Abs() to control value overflow
    return (b > a) ? (b - a) : (a - b);
}

HK_FORCEINLINE float Dist(float a, float b)
{
    return Abs(a - b);
}

HK_FORCEINLINE double Dist(double a, double b)
{
    return Abs(a - b);
}

template <typename T>
constexpr T MinValue() { return std::numeric_limits<T>::min(); }

template <typename T>
constexpr T MaxValue() { return std::numeric_limits<T>::max(); }

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr int SignBits(T value)
{
    return std::is_signed<T>::value ? static_cast<uint64_t>(value) >> (BitsCount<T>() - 1) : 0;
}

HK_FORCEINLINE int SignBits(float value)
{
    return *reinterpret_cast<const uint32_t*>(&value) >> 31;
}

HK_FORCEINLINE int SignBits(double value)
{
    return *reinterpret_cast<const uint64_t*>(&value) >> 63;
}

/// Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
template <typename T>
HK_FORCEINLINE T Sign(T value)
{
    return value > 0 ? 1 : -SignBits(value);
}

template <typename T>
constexpr T MaxPowerOfTwo()
{
    return T(1) << (BitsCount<T>() - 2);
}

template <>
constexpr float MaxPowerOfTwo()
{
    return float(1u << 31);
}

template <typename T>
constexpr T MinPowerOfTwo()
{
    return T(1);
}

HK_FORCEINLINE constexpr int32_t ToIntFast(float value)
{
    return static_cast<int32_t>(value);
}

HK_FORCEINLINE constexpr int64_t ToLongFast(float value)
{
    return static_cast<int64_t>(value);
}

template <typename T>
T ToGreaterPowerOfTwo(T value);

template <typename T>
T ToLessPowerOfTwo(T value);

template <>
HK_FORCEINLINE int8_t ToGreaterPowerOfTwo<int8_t>(int8_t value)
{
    using T = int8_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    return val + 1;
}

template <>
HK_FORCEINLINE uint8_t ToGreaterPowerOfTwo<uint8_t>(uint8_t value)
{
    using T = uint8_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    return val + 1;
}

template <>
HK_FORCEINLINE int16_t ToGreaterPowerOfTwo<int16_t>(int16_t value)
{
    using T = int16_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    return val + 1;
}

template <>
HK_FORCEINLINE uint16_t ToGreaterPowerOfTwo<uint16_t>(uint16_t value)
{
    using T = uint16_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    return val + 1;
}

template <>
HK_FORCEINLINE int32_t ToGreaterPowerOfTwo<int32_t>(int32_t value)
{
    using T = int32_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return val + 1;
}

template <>
HK_FORCEINLINE uint32_t ToGreaterPowerOfTwo<uint32_t>(uint32_t value)
{
    using T = uint32_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return val + 1;
}

template <>
HK_FORCEINLINE int64_t ToGreaterPowerOfTwo<int64_t>(int64_t value)
{
    using T = int64_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val |= val >> 32;
    return val + 1;
}

template <>
HK_FORCEINLINE uint64_t ToGreaterPowerOfTwo<uint64_t>(uint64_t value)
{
    using T = uint64_t;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    T val = value - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val |= val >> 32;
    return val + 1;
}

template <>
HK_FORCEINLINE float ToGreaterPowerOfTwo(float value)
{
    using T = float;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    uint32_t val = ToIntFast(value) - 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return static_cast<float>(val + 1);
}

template <>
HK_FORCEINLINE int8_t ToLessPowerOfTwo<int8_t>(int8_t value)
{
    using T = int8_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE uint8_t ToLessPowerOfTwo<uint8_t>(uint8_t value)
{
    using T = uint8_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE int16_t ToLessPowerOfTwo<int16_t>(int16_t value)
{
    using T = int16_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE uint16_t ToLessPowerOfTwo<uint16_t>(uint16_t value)
{
    using T = uint16_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE int32_t ToLessPowerOfTwo<int32_t>(int32_t value)
{
    using T = int32_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE uint32_t ToLessPowerOfTwo<uint32_t>(uint32_t value)
{
    using T = uint32_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE int64_t ToLessPowerOfTwo<int64_t>(int64_t value)
{
    using T = int64_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val |= val >> 32;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE uint64_t ToLessPowerOfTwo<uint64_t>(uint64_t value)
{
    using T = uint64_t;
    T val   = value;
    if (val < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val |= val >> 32;
    return val - (val >> 1);
}

template <>
HK_FORCEINLINE float ToLessPowerOfTwo(float value)
{
    using T = float;
    if (value >= MaxPowerOfTwo<T>())
        return MaxPowerOfTwo<T>();
    if (value < MinPowerOfTwo<T>())
        return MinPowerOfTwo<T>();
    uint32_t val = ToIntFast(value);
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return static_cast<float>(val - (val >> 1));
}

template <typename T>
HK_FORCEINLINE T ToClosestPowerOfTwo(T value)
{
    T GreaterPow = ToGreaterPowerOfTwo(value);
    T LessPow    = ToLessPowerOfTwo(value);
    return Dist(GreaterPow, value) < Dist(LessPow, value) ? GreaterPow : LessPow;
}

HK_FORCEINLINE int Log2(uint32_t value)
{
    uint32_t r;
    uint32_t shift;
    uint32_t v = value;
    r = (v > 0xffff) << 4;
    v >>= r;
    shift = (v > 0xff) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xf) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3) << 1;
    v >>= shift;
    r |= shift;
    r |= (v >> 1);
    return r;
}

HK_FORCEINLINE int Log2(uint8_t value)
{
    int log2 = 0;
    while (value >>= 1)
        log2++;
    return log2;
}

HK_FORCEINLINE int Log2(uint16_t value)
{
    int log2 = 0;
    while (value >>= 1)
        log2++;
    return log2;
}

HK_FORCEINLINE int Log2(uint64_t value)
{
    int log2 = 0;
    while (value >>= 1)
        log2++;
    return log2;
}

}

extern "C" uint16_t half_from_float(uint32_t f);
extern "C" uint32_t half_to_float(uint16_t h);
extern "C" uint16_t half_add(uint16_t x, uint16_t y);
extern "C" uint16_t half_mul(uint16_t x, uint16_t y);
extern "C" uint32_t fast_half_to_float(uint16_t h);

HK_FORCEINLINE uint16_t f32tof16(float f)
{
    return half_from_float(*reinterpret_cast<uint32_t*>(&f));
}

HK_FORCEINLINE float f16tof32(uint16_t f)
{
    #if 0
    uint32_t r = half_to_float(f);
    #else
    uint32_t r = fast_half_to_float(f);
    #endif
    return *reinterpret_cast<float*>(&r);
}

struct Half
{
    uint16_t v;

    Half() = default;

    Half(Half const&) = default;

    Half(float f) :
        v(f32tof16(f))
    {}

    static Half sMakeHalf(uint16_t val)
    {
        Half f;
        f.v = val;
        return f;
    }

    Half& operator=(Half const&) = default;

    Half& operator=(float f)
    {
        v = f32tof16(f);
        return *this;
    }

    operator float() const
    {
        return f16tof32(v);
    }

    Half& operator*=(Half const& rhs)
    {
        v = half_mul(v, rhs.v);
        return *this;
    }

    Half& operator+=(Half const& rhs)
    {
        v = half_add(v, rhs.v);
        return *this;
    }

    Half operator*(Half const& rhs)
    {
        return sMakeHalf(half_mul(v, rhs.v));
    }

    Half operator+(Half const& rhs)
    {
        return sMakeHalf(half_add(v, rhs.v));
    }

    /// Return half float sign bit
    int SignBits() const
    {
        return v >> 15;
    }

    /// Return half float exponent
    int Exponent() const
    {
        return (v >> 10) & 0x1f;
    }

    /// Return half float mantissa
    int Mantissa() const
    {
        return v & 0x3ff;
    }
};

namespace Math
{

/// Return floating point exponent
HK_FORCEINLINE int Exponent(float value)
{
    return (*reinterpret_cast<const uint32_t*>(&value) >> 23) & 0xff;
}

/// Return floating point mantissa
HK_FORCEINLINE int Mantissa(float value)
{
    return *reinterpret_cast<const uint32_t*>(&value) & 0x7fffff;
}

/// Return floating point exponent
HK_FORCEINLINE int Exponent(double value)
{
    return (*reinterpret_cast<const uint64_t*>(&value) >> 52) & 0x7ff;
}

/// Return floating point mantissa
HK_FORCEINLINE int64_t Mantissa(double value)
{
    return *reinterpret_cast<const uint64_t*>(&value) & 0xfffffffffffff;
}

HK_FORCEINLINE bool IsInfinite(float value)
{
    //return std::isinf( Self );
    return (*reinterpret_cast<const uint32_t*>(&value) & 0x7fffffff) == 0x7f800000;
}

HK_FORCEINLINE bool IsNan(float value)
{
    //return std::isnan( Self );
    return (*reinterpret_cast<const uint32_t*>(&value) & 0x7f800000) == 0x7f800000;
}

HK_FORCEINLINE bool IsNormal(float value)
{
    return std::isnormal(value);
}

HK_FORCEINLINE bool IsDenormal(float value)
{
    return (*reinterpret_cast<const uint32_t*>(&value) & 0x7f800000) == 0x00000000 && (*reinterpret_cast<const uint32_t*>(&value) & 0x007fffff) != 0x00000000;
}

HK_FORCEINLINE bool IsInfinite(double value)
{
    //return std::isinf( value );
    return (*reinterpret_cast<const uint64_t*>(&value) & uint64_t(0x7fffffffffffffffULL)) == uint64_t(0x7f80000000000000ULL);
}

HK_FORCEINLINE bool IsNan(double value)
{
    //return std::isnan( value );
    return (*reinterpret_cast<const uint64_t*>(&value) & uint64_t(0x7f80000000000000ULL)) == uint64_t(0x7f80000000000000ULL);
}

HK_FORCEINLINE bool IsNormal(double value)
{
    return std::isnormal(value);
}

HK_FORCEINLINE bool IsDenormal(double value)
{
    return (*reinterpret_cast<const uint64_t*>(&value) & uint64_t(0x7f80000000000000ULL)) == uint64_t(0x0000000000000000ULL) && (*reinterpret_cast<const uint64_t*>(&value) & uint64_t(0x007fffffffffffffULL)) != uint64_t(0x0000000000000000ULL);
}

template <typename T>
constexpr int MaxExponent();

template <>
constexpr int MaxExponent<float>() { return 127; }

template <>
constexpr int MaxExponent<double>() { return 1023; }

template <typename T>
HK_FORCEINLINE T Floor(T value)
{
    return std::floor(value);
}

template <typename T>
HK_FORCEINLINE T Ceil(T value)
{
    return std::ceil(value);
}

template <typename T>
HK_FORCEINLINE T Fract(T value)
{
    return value - std::floor(value);
}

template <typename T>
HK_FORCEINLINE T Step(T const& value, T const& edge)
{
    return value < edge ? T(0) : T(1);
}

template <typename T>
HK_FORCEINLINE T SmoothStep(T const& value, T const& edge0, T const& edge1)
{
    const T t = Saturate((value - edge0) / (edge1 - edge0));
    return t * t * (T(3) - T(2) * t);
}

template <typename T>
constexpr T Lerp(T const& a, T const& b, T const& frac)
{
    return a + frac * (b - a);
}

template <typename T>
HK_FORCEINLINE T Round(T const& value)
{
    return std::round(value);
}

template <typename T>
HK_FORCEINLINE T RoundN(T const& value, T const& n)
{
    return std::round(value * n) / n;
}

template <typename T>
HK_FORCEINLINE T Round1(T const& value) { return RoundN(value, T(10)); }

template <typename T>
HK_FORCEINLINE T Round2(T const& value) { return RoundN(value, T(100)); }

template <typename T>
HK_FORCEINLINE T Round3(T const& value) { return RoundN(value, T(1000)); }

template <typename T>
HK_FORCEINLINE T Round4(T const& value) { return RoundN(value, T(10000)); }

template <typename T>
HK_FORCEINLINE T Snap(T const& value, T const& snapValue)
{
    HK_ASSERT(snapValue > T(0));
    return Round(value / snapValue) * snapValue;
}

constexpr double _PI_DBL      = 3.1415926535897932384626433832795;
constexpr double _2PI_DBL     = 2.0 * _PI_DBL;
constexpr double _HALF_PI_DBL = 0.5 * _PI_DBL;
constexpr double _EXP_DBL     = 2.71828182845904523536;
constexpr double _DEG2RAD_DBL = _PI_DBL / 180.0;
constexpr double _RAD2DEG_DBL = 180.0 / _PI_DBL;

constexpr float _PI             = static_cast<float>(_PI_DBL);
constexpr float _2PI            = static_cast<float>(_2PI_DBL);
constexpr float _HALF_PI        = static_cast<float>(_HALF_PI_DBL);
constexpr float _EXP            = static_cast<float>(_EXP_DBL);
constexpr float _DEG2RAD        = static_cast<float>(_DEG2RAD_DBL);
constexpr float _RAD2DEG        = static_cast<float>(_RAD2DEG_DBL);
constexpr float _INFINITY       = 1e30f;
constexpr float _ZERO_TOLERANCE = 1.1754944e-038f;

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE T Min(T a, T b)
{
    return std::min(a, b);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE T Min3(T a, T b, T c)
{
    return Min(Min(a, b), c);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE T Max(T a, T b)
{
    return std::max(a, b);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE T Max3(T a, T b, T c)
{
    return Max(Max(a, b), c);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE T Clamp(T val, T a, T b)
{
    return std::min(std::max(val, a), b);
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
HK_FORCEINLINE T Saturate(T val)
{
    return Clamp(val, T(0), T(1));
}

HK_FORCEINLINE float Min(float a, float b)
{
    _mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
    return a;
}

HK_FORCEINLINE float Min3(float a, float b, float c)
{
    _mm_store_ss(&a, _mm_min_ss(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)), _mm_set_ss(c)));
    return a;
}

HK_FORCEINLINE float Max(float a, float b)
{
    _mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
    return a;
}

HK_FORCEINLINE float Max3(float a, float b, float c)
{
    _mm_store_ss(&a, _mm_max_ss(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)), _mm_set_ss(c)));
    return a;
}

HK_FORCEINLINE float Clamp(float val, float minval, float maxval)
{
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(minval)), _mm_set_ss(maxval)));
    return val;
}

HK_FORCEINLINE float Saturate(float val)
{
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_setzero_ps()), _mm_set_ss(1.0f)));
    return val;
}

HK_FORCEINLINE double Min(double a, double b)
{
    _mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
    return a;
}

HK_FORCEINLINE double Min3(double a, double b, double c)
{
    _mm_store_sd(&a, _mm_min_sd(_mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)), _mm_set_sd(c)));
    return a;
}

HK_FORCEINLINE double Max(double a, double b)
{
    _mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
    return a;
}

HK_FORCEINLINE double Max3(double a, double b, double c)
{
    _mm_store_sd(&a, _mm_max_sd(_mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)), _mm_set_sd(c)));
    return a;
}

HK_FORCEINLINE double Clamp(double val, double minval, double maxval)
{
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(minval)), _mm_set_sd(maxval)));
    return val;
}

HK_FORCEINLINE double Saturate(double val)
{
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_setzero_pd()), _mm_set_sd(1.0)));
    return val;
}

HK_FORCEINLINE uint8_t Saturate8(int x)
{
    if ((unsigned int)x <= 255)
        return x;

    if (x < 0)
        return 0;

    return 255;
}

HK_FORCEINLINE uint16_t Saturate16(int x)
{
    if ((unsigned int)x <= 65535)
        return x;

    if (x < 0)
        return 0;

    return 65535;
}

HK_FORCEINLINE void MinMax(float a, float b, float& mn, float& mx)
{
    __m128 v1 = _mm_set_ss(a);
    __m128 v2 = _mm_set_ss(b);

    _mm_store_ss(&mn, _mm_min_ss(v1, v2));
    _mm_store_ss(&mx, _mm_max_ss(v1, v2));
#if 0
    if (a > b)
    {
        mn = b;
        mx = a;
    }
    else
    {
        mn = a;
        mx = b;
    }
#endif
}

HK_FORCEINLINE void MinMax(double a, double b, double& mn, double& mx)
{
    __m128d v1 = _mm_set_sd(a);
    __m128d v2 = _mm_set_sd(b);

    _mm_store_sd(&mn, _mm_min_sd(v1, v2));
    _mm_store_sd(&mx, _mm_max_sd(v1, v2));
}

HK_FORCEINLINE void MinMax(float a, float b, float c, float& mn, float& mx)
{
    __m128 v1 = _mm_set_ss(a);
    __m128 v2 = _mm_set_ss(b);
    __m128 v3 = _mm_set_ss(c);

    _mm_store_ss(&mn, _mm_min_ss(_mm_min_ss(v1, v2), v3));
    _mm_store_ss(&mx, _mm_max_ss(_mm_max_ss(v1, v2), v3));

#if 0
    if (a > b)
    {
        mn = b;
        mx = a;
    }
    else
    {
        mn = a;
        mx = b;
    }

    if (c > mx)
    {
        mx = c;
    }
    else if (c < mn)
    {
        mn = c;
    }
#endif
}

HK_FORCEINLINE void MinMax(double a, double b, double c, double& mn, double& mx)
{
    __m128d v1 = _mm_set_sd(a);
    __m128d v2 = _mm_set_sd(b);
    __m128d v3 = _mm_set_sd(c);

    _mm_store_sd(&mn, _mm_min_sd(_mm_min_sd(v1, v2), v3));
    _mm_store_sd(&mx, _mm_max_sd(_mm_max_sd(v1, v2), v3));
}

template <typename T>
HK_FORCEINLINE constexpr T Square(T const& a)
{
    return a * a;
}

template <typename T>
HK_FORCEINLINE T Sqrt(T const& value)
{
    return value > T(0) ? T(std::sqrt(value)) : T(0);
}

template <typename T>
HK_FORCEINLINE T InvSqrt(T const& value)
{
    return value > _ZERO_TOLERANCE ? std::sqrt(T(1) / value) : _INFINITY;
}

// Approximately equivalent to 1/sqrt(x). Returns a large value when value == 0.
HK_FORCEINLINE float RSqrt(float value)
{
    const float x2 = value * 0.5F;
    const float threehalfs = 1.5F;

    union
    {
        float f;
        uint32_t i;
    } conv = {value};
    conv.i = 0x5f3759df - ( conv.i >> 1 );
    conv.f *= threehalfs - x2 * conv.f * conv.f;
    return conv.f;
}

template <typename T>
HK_FORCEINLINE T Pow(T const value, T const power)
{
    return std::pow(value, power);
}

HK_FORCEINLINE float FMod(float x, float y)
{
    return std::fmod(x, y);
}

HK_FORCEINLINE double FMod(double x, double y)
{
    return std::fmod(x, y);
}

template <typename T>
HK_INLINE T GreaterCommonDivisor(T m, T n)
{
    return (m < T(0.0001)) ? n : GreaterCommonDivisor(FMod(n, m), m);
}

template <typename T>
HK_FORCEINLINE T HermiteCubicSpline(T const& p0, T const& m0, T const& p1, T const& m1, float t)
{
    const float tt  = t * t;
    const float ttt = tt * t;
    const float s2  = -2 * ttt + 3 * tt;
    const float s3  = ttt - tt;
    const float s0  = 1 - s2;
    const float s1  = s3 - tt + t;
    return p0 * s0 + m0 * (s1 * t) + p1 * s2 + m1 * (s3 * t);
}

// Comparision
template <typename T>
constexpr bool CompareEps(T const& a, T const& b, T const& epsilon)
{
    return Dist(a, b) < epsilon;
}

// Trigonometric

template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
constexpr T Degrees(T rad) { return rad * T(_RAD2DEG_DBL); }

template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
constexpr T Radians(T deg) { return deg * T(_DEG2RAD_DBL); }

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr float Degrees(T rad) { return static_cast<float>(rad) * _RAD2DEG; }

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr float Radians(T deg) { return static_cast<float>(deg) * _DEG2RAD; }

template <typename T>
HK_FORCEINLINE T Sin(T rad) { return std::sin(rad); }

template <typename T>
HK_FORCEINLINE T Cos(T rad) { return std::cos(rad); }

template <typename T>
HK_FORCEINLINE T DegSin(T deg) { return std::sin(Radians(deg)); }

template <typename T>
HK_FORCEINLINE T DegCos(T deg) { return std::cos(Radians(deg)); }

template <typename T>
HK_FORCEINLINE void SinCos(T rad, T& s, T& c)
{
    s = std::sin(rad);
    c = std::cos(rad);
}

template <typename T>
HK_FORCEINLINE void DegSinCos(T deg, T& s, T& c)
{
    SinCos(Radians(deg), s, c);
}

HK_FORCEINLINE float Atan2(float y, float x)
{
    return std::atan2(y, x);
}

HK_FORCEINLINE float Atan2Fast(float y, float x)
{
    const float k1    = _PI / 4.0f;
    const float k2    = 3.0f * k1;
    const float absY  = Abs(y);
    const float angle = x >= 0.0f ? (k1 - k1 * ((x - absY) / (x + absY))) : (k2 - k1 * ((x + absY) / (absY - x)));
    return y < 0.0f ? -angle : angle;
}

constexpr int32_t INT64_HIGH_INT(uint64_t i64)
{
    return i64 >> 32;
}

constexpr int32_t INT64_LOW_INT(uint64_t i64)
{
    return i64 & 0xFFFFFFFF;
}

} // namespace Math

struct Int2
{
    int32_t X, Y;

    Int2() = default;
    constexpr Int2(const int32_t& x, const int32_t& y) :
        X(x), Y(y) {}

    int32_t& operator[](const int& _Index)
    {
        HK_ASSERT_(_Index >= 0 && _Index < 2, "Index out of range");
        return (&X)[_Index];
    }

    int32_t const& operator[](const int& _Index) const
    {
        HK_ASSERT_(_Index >= 0 && _Index < 2, "Index out of range");
        return (&X)[_Index];
    }
};

HK_NAMESPACE_END
