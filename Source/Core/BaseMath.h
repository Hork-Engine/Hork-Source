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

#include <Platform/BaseTypes.h>

#include <emmintrin.h>

#ifndef FLT_DIG
#    define FLT_DIG 6
#endif
#ifndef DBL_DIG
#    define DBL_DIG 10
#endif

namespace Math
{

template <typename T>
constexpr bool IsSigned()
{
    return std::is_signed<T>::value;
}

template <typename T>
constexpr bool IsUnsigned()
{
    return std::is_unsigned<T>::value;
}

template <typename T>
constexpr bool IsIntegral()
{
    return std::is_integral<T>::value;
}

template <typename T>
constexpr bool IsReal()
{
    return std::is_floating_point<T>::value;
}

template <typename T>
constexpr int BitsCount()
{
    return sizeof(T) * 8;
}

template <typename T>
constexpr int FloatingPointPrecision();

template <>
constexpr int FloatingPointPrecision<float>()
{
    return FLT_DIG;
}

template <>
constexpr int FloatingPointPrecision<double>()
{
    return DBL_DIG;
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
T Abs(T Value)
{
    const T Mask = Value >> (BitsCount<T>() - 1);
    return ((Value ^ Mask) - Mask);
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
constexpr T Dist(T A, T B)
{
    // NOTE: We don't use Abs() to control value overflow
    return (B > A) ? (B - A) : (A - B);
}

HK_FORCEINLINE float Abs(float Value)
{
    int32_t i = *reinterpret_cast<const int32_t*>(&Value) & 0x7FFFFFFF;
    return *reinterpret_cast<const float*>(&i);
}

HK_FORCEINLINE double Abs(double Value)
{
    int64_t i = *reinterpret_cast<const int64_t*>(&Value) & 0x7FFFFFFFFFFFFFFFLL;
    return *reinterpret_cast<const double*>(&i);
}

HK_FORCEINLINE float Dist(float A, float B)
{
    return Abs(A - B);
}

HK_FORCEINLINE double Dist(double A, double B)
{
    return Abs(A - B);
}

template <typename T>
constexpr T MinValue() { return std::numeric_limits<T>::min(); }

template <typename T>
constexpr T MaxValue() { return std::numeric_limits<T>::max(); }

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
constexpr int SignBits(T Value)
{
    return IsSigned<T>() ? static_cast<uint64_t>(Value) >> (BitsCount<T>() - 1) : 0;
}

HK_FORCEINLINE int SignBits(float Value)
{
    return *reinterpret_cast<const uint32_t*>(&Value) >> 31;
}

HK_FORCEINLINE int SignBits(double Value)
{
    return *reinterpret_cast<const uint64_t*>(&Value) >> 63;
}

/** Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0 */
template <typename T>
HK_FORCEINLINE T Sign(T Value)
{
    return Value > 0 ? 1 : -SignBits(Value);
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

constexpr int32_t ToIntFast(float Value)
{
    return static_cast<int32_t>(Value);
}

constexpr int64_t ToLongFast(float Value)
{
    return static_cast<int64_t>(Value);
}

template <typename T>
T ToGreaterPowerOfTwo(T Value);

template <typename T>
T ToLessPowerOfTwo(T Value);

template <>
HK_FORCEINLINE int8_t ToGreaterPowerOfTwo<int8_t>(int8_t Value)
{
    using T = int8_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val + 1;
}

template <>
HK_FORCEINLINE uint8_t ToGreaterPowerOfTwo<uint8_t>(uint8_t Value)
{
    using T = uint8_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val + 1;
}

template <>
HK_FORCEINLINE int16_t ToGreaterPowerOfTwo<int16_t>(int16_t Value)
{
    using T = int16_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val + 1;
}

template <>
HK_FORCEINLINE uint16_t ToGreaterPowerOfTwo<uint16_t>(uint16_t Value)
{
    using T = uint16_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val + 1;
}

template <>
HK_FORCEINLINE int32_t ToGreaterPowerOfTwo<int32_t>(int32_t Value)
{
    using T = int32_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val + 1;
}

template <>
HK_FORCEINLINE uint32_t ToGreaterPowerOfTwo<uint32_t>(uint32_t Value)
{
    using T = uint32_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val + 1;
}

template <>
HK_FORCEINLINE int64_t ToGreaterPowerOfTwo<int64_t>(int64_t Value)
{
    using T = int64_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val + 1;
}

template <>
HK_FORCEINLINE uint64_t ToGreaterPowerOfTwo<uint64_t>(uint64_t Value)
{
    using T = uint64_t;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    T Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val + 1;
}

template <>
HK_FORCEINLINE float ToGreaterPowerOfTwo(float Value)
{
    using T = float;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    uint32_t Val = ToIntFast(Value) - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast<float>(Val + 1);
}

template <>
HK_FORCEINLINE int8_t ToLessPowerOfTwo<int8_t>(int8_t Value)
{
    using T = int8_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE uint8_t ToLessPowerOfTwo<uint8_t>(uint8_t Value)
{
    using T = uint8_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE int16_t ToLessPowerOfTwo<int16_t>(int16_t Value)
{
    using T = int16_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE uint16_t ToLessPowerOfTwo<uint16_t>(uint16_t Value)
{
    using T = uint16_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE int32_t ToLessPowerOfTwo<int32_t>(int32_t Value)
{
    using T = int32_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE uint32_t ToLessPowerOfTwo<uint32_t>(uint32_t Value)
{
    using T = uint32_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE int64_t ToLessPowerOfTwo<int64_t>(int64_t Value)
{
    using T = int64_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE uint64_t ToLessPowerOfTwo<uint64_t>(uint64_t Value)
{
    using T = uint64_t;
    T Val   = Value;
    if (Val < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val - (Val >> 1);
}

template <>
HK_FORCEINLINE float ToLessPowerOfTwo(float Value)
{
    using T = float;
    if (Value >= MaxPowerOfTwo<T>())
    {
        return MaxPowerOfTwo<T>();
    }
    if (Value < MinPowerOfTwo<T>())
    {
        return MinPowerOfTwo<T>();
    }
    uint32_t Val = ToIntFast(Value);
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return static_cast<float>(Val - (Val >> 1));
}

template <typename T>
HK_FORCEINLINE T ToClosestPowerOfTwo(T Value)
{
    T GreaterPow = ToGreaterPowerOfTwo(Value);
    T LessPow    = ToLessPowerOfTwo(Value);
    return Dist(GreaterPow, Value) < Dist(LessPow, Value) ? GreaterPow : LessPow;
}

HK_FORCEINLINE int Log2(uint32_t Value)
{
    uint32_t r;
    uint32_t shift;
    uint32_t v = Value;
    r          = (v > 0xffff) << 4;
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

    Half(uint16_t v) = delete;

    static Half MakeHalf(uint16_t val)
    {
        Half f;
        f.v = val;
        return f;
    }

    Half(float f) :
        v(f32tof16(f))
    {}

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
        return MakeHalf(half_mul(v, rhs.v));
    }

    Half operator+(Half const& rhs)
    {
        return MakeHalf(half_add(v, rhs.v));
    }

    /** Return half float sign bit */
    int SignBits() const
    {
        return v >> 15;
    }

    /** Return half float exponent */
    int Exponent() const
    {
        return (v >> 10) & 0x1f;
    }

    /** Return half float mantissa */
    int Mantissa() const
    {
        return v & 0x3ff;
    }
};

namespace Math
{

/** Return floating point exponent */
HK_FORCEINLINE int Exponent(float Value)
{
    return (*reinterpret_cast<const uint32_t*>(&Value) >> 23) & 0xff;
}

/** Return floating point mantissa */
HK_FORCEINLINE int Mantissa(float Value)
{
    return *reinterpret_cast<const uint32_t*>(&Value) & 0x7fffff;
}

/** Return floating point exponent */
HK_FORCEINLINE int Exponent(double Value)
{
    return (*reinterpret_cast<const uint64_t*>(&Value) >> 52) & 0x7ff;
}

/** Return floating point mantissa */
HK_FORCEINLINE int64_t Mantissa(double Value)
{
    return *reinterpret_cast<const uint64_t*>(&Value) & 0xfffffffffffff;
}

HK_FORCEINLINE bool IsInfinite(float Value)
{
    //return std::isinf( Self );
    return (*reinterpret_cast<const uint32_t*>(&Value) & 0x7fffffff) == 0x7f800000;
}

HK_FORCEINLINE bool IsNan(float Value)
{
    //return std::isnan( Self );
    return (*reinterpret_cast<const uint32_t*>(&Value) & 0x7f800000) == 0x7f800000;
}

HK_FORCEINLINE bool IsNormal(float Value)
{
    return std::isnormal(Value);
}

HK_FORCEINLINE bool IsDenormal(float Value)
{
    return (*reinterpret_cast<const uint32_t*>(&Value) & 0x7f800000) == 0x00000000 && (*reinterpret_cast<const uint32_t*>(&Value) & 0x007fffff) != 0x00000000;
}

// Floating point specific
HK_FORCEINLINE bool IsInfinite(double Value)
{
    //return std::isinf( Value );
    return (*reinterpret_cast<const uint64_t*>(&Value) & uint64_t(0x7fffffffffffffffULL)) == uint64_t(0x7f80000000000000ULL);
}

HK_FORCEINLINE bool IsNan(double Value)
{
    //return std::isnan( Value );
    return (*reinterpret_cast<const uint64_t*>(&Value) & uint64_t(0x7f80000000000000ULL)) == uint64_t(0x7f80000000000000ULL);
}

HK_FORCEINLINE bool IsNormal(double Value)
{
    return std::isnormal(Value);
}

HK_FORCEINLINE bool IsDenormal(double Value)
{
    return (*reinterpret_cast<const uint64_t*>(&Value) & uint64_t(0x7f80000000000000ULL)) == uint64_t(0x0000000000000000ULL) && (*reinterpret_cast<const uint64_t*>(&Value) & uint64_t(0x007fffffffffffffULL)) != uint64_t(0x0000000000000000ULL);
}

template <typename T>
constexpr int MaxExponent();

template <>
constexpr int MaxExponent<float>() { return 127; }

template <>
constexpr int MaxExponent<double>() { return 1023; }

template <typename T>
HK_FORCEINLINE T Floor(T Value)
{
    return std::floor(Value);
}

template <typename T>
HK_FORCEINLINE T Ceil(T Value)
{
    return std::ceil(Value);
}

template <typename T>
HK_FORCEINLINE T Fract(T Value)
{
    return Value - std::floor(Value);
}

template <typename T>
HK_FORCEINLINE T Step(T const& Value, T const& Edge)
{
    return Value < Edge ? T(0) : T(1);
}

template <typename T>
HK_FORCEINLINE T SmoothStep(T const& Value, T const& Edge0, T const& Edge1)
{
    const T t = Saturate((Value - Edge0) / (Edge1 - Edge0));
    return t * t * (T(3) - T(2) * t);
}

template <typename T>
constexpr T Lerp(T const& _From, T const& _To, T const& _Mix)
{
    return _From + _Mix * (_To - _From);
}

template <typename T>
HK_FORCEINLINE T Round(T const& Value)
{
    return std::round(Value);
}

template <typename T>
HK_FORCEINLINE T RoundN(T const& Value, T const& _N)
{
    return std::round(Value * _N) / _N;
}

template <typename T>
HK_FORCEINLINE T Round1(T const& Value) { return RoundN(Value, T(10)); }

template <typename T>
HK_FORCEINLINE T Round2(T const& Value) { return RoundN(Value, T(100)); }

template <typename T>
HK_FORCEINLINE T Round3(T const& Value) { return RoundN(Value, T(1000)); }

template <typename T>
HK_FORCEINLINE T Round4(T const& Value) { return RoundN(Value, T(10000)); }

template <typename T>
HK_FORCEINLINE T Snap(T const& Value, T const& SnapValue)
{
    HK_ASSERT(SnapValue > T(0));
    return Round(Value / SnapValue) * SnapValue;
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

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
HK_FORCEINLINE T Min(T a, T b)
{
    return std::min(a, b);
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
HK_FORCEINLINE T Min3(T a, T b, T c)
{
    return Min(Min(a, b), c);
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
HK_FORCEINLINE T Max(T a, T b)
{
    return std::max(a, b);
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
HK_FORCEINLINE T Max3(T a, T b, T c)
{
    return Max(Max(a, b), c);
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
HK_FORCEINLINE T Clamp(T val, T a, T b)
{
    return std::min(std::max(val, a), b);
}

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
HK_FORCEINLINE T Saturate(T val)
{
    return Clamp(val, T(0), T(1));
}

/** Branchless min */
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

/** Branchless max */
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

/** Branchless clamp */
HK_FORCEINLINE float Clamp(float val, float minval, float maxval)
{
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(minval)), _mm_set_ss(maxval)));
    return val;
}

/** Branchless saturate */
HK_FORCEINLINE float Saturate(float val)
{
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_setzero_ps()), _mm_set_ss(1.0f)));
    return val;
}

/** Branchless min */
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

/** Branchless max */
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

/** Branchless clamp */
HK_FORCEINLINE double Clamp(double val, double minval, double maxval)
{
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(minval)), _mm_set_sd(maxval)));
    return val;
}

/** Branchless saturate */
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

HK_FORCEINLINE void MinMax(float _A, float _B, float& _Min, float& _Max)
{
    __m128 v1 = _mm_set_ss(_A);
    __m128 v2 = _mm_set_ss(_B);

    _mm_store_ss(&_Min, _mm_min_ss(v1, v2));
    _mm_store_ss(&_Max, _mm_max_ss(v1, v2));
#if 0
    if ( _A > _B ) {
        _Min = _B;
        _Max = _A;
    } else {
        _Min = _A;
        _Max = _B;
    }
#endif
}

HK_FORCEINLINE void MinMax(double _A, double _B, double& _Min, double& _Max)
{
    __m128d v1 = _mm_set_sd(_A);
    __m128d v2 = _mm_set_sd(_B);

    _mm_store_sd(&_Min, _mm_min_sd(v1, v2));
    _mm_store_sd(&_Max, _mm_max_sd(v1, v2));
}

HK_FORCEINLINE void MinMax(float _A, float _B, float _C, float& _Min, float& _Max)
{
    __m128 v1 = _mm_set_ss(_A);
    __m128 v2 = _mm_set_ss(_B);
    __m128 v3 = _mm_set_ss(_C);

    _mm_store_ss(&_Min, _mm_min_ss(_mm_min_ss(v1, v2), v3));
    _mm_store_ss(&_Max, _mm_max_ss(_mm_max_ss(v1, v2), v3));

#if 0
    if ( _A > _B ) {
        _Min = _B;
        _Max = _A;
    } else {
        _Min = _A;
        _Max = _B;
    }

    if ( _C > _Max ) {
        _Max = _C;
    } else if ( _C < _Min ) {
        _Min = _C;
    }
#endif
}

HK_FORCEINLINE void MinMax(double _A, double _B, double _C, double& _Min, double& _Max)
{
    __m128d v1 = _mm_set_sd(_A);
    __m128d v2 = _mm_set_sd(_B);
    __m128d v3 = _mm_set_sd(_C);

    _mm_store_sd(&_Min, _mm_min_sd(_mm_min_sd(v1, v2), v3));
    _mm_store_sd(&_Max, _mm_max_sd(_mm_max_sd(v1, v2), v3));
}

template <typename T>
HK_FORCEINLINE T Square(T const& a)
{
    return a * a;
}

template <typename T>
HK_FORCEINLINE T Sqrt(T const& _Value)
{
    return _Value > T(0) ? T(std::sqrt(_Value)) : T(0);
}

template <typename T>
HK_FORCEINLINE T InvSqrt(T const& _Value)
{
    return _Value > _ZERO_TOLERANCE ? std::sqrt(T(1) / _Value) : _INFINITY;
}

// Approximately equivalent to 1/sqrt(x). Returns a large value when value == 0.
HK_FORCEINLINE float RSqrt(const float& _Value)
{
    float   Half   = _Value * 0.5f;
    int32_t Temp   = 0x5f3759df - (*reinterpret_cast<const int32_t*>(&_Value) >> 1);
    float   Result = *reinterpret_cast<const float*>(&Temp);
    Result         = Result * (1.5f - Result * Result * Half);
    return Result;
}

template <typename T>
HK_FORCEINLINE T Pow(T const Value, T const Power)
{
    return std::pow(Value, Power);
}

HK_FORCEINLINE float FMod(float _X, float _Y)
{
    return std::fmod(_X, _Y);
}

HK_FORCEINLINE double FMod(double _X, double _Y)
{
    return std::fmod(_X, _Y);
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
    return s0 * p0 + s1 * m0 * t + s2 * p1 + s3 * m1 * t;
}

// Comparision
template <typename T>
constexpr bool CompareEps(T const& A, T const& B, T const& Epsilon)
{
    return Dist(A, B) < Epsilon;
}

// Trigonometric

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr T Degrees(T _Rad) { return _Rad * T(_RAD2DEG_DBL); }

template <typename T, typename = TStdEnableIf<IsReal<T>()>>
constexpr T Radians(T _Deg) { return _Deg * T(_DEG2RAD_DBL); }

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
constexpr float Degrees(T _Rad) { return static_cast<float>(_Rad) * _RAD2DEG; }

template <typename T, typename = TStdEnableIf<IsIntegral<T>()>>
constexpr float Radians(T _Deg) { return static_cast<float>(_Deg) * _DEG2RAD; }

template <typename T>
HK_FORCEINLINE T Sin(T _Rad) { return std::sin(_Rad); }

template <typename T>
HK_FORCEINLINE T Cos(T _Rad) { return std::cos(_Rad); }

template <typename T>
HK_FORCEINLINE T DegSin(T _Deg) { return std::sin(Radians(_Deg)); }

template <typename T>
HK_FORCEINLINE T DegCos(T _Deg) { return std::cos(Radians(_Deg)); }

template <typename T>
HK_FORCEINLINE void SinCos(T _Rad, T& _Sin, T& _Cos)
{
    _Sin = std::sin(_Rad);
    _Cos = std::cos(_Rad);
}

template <typename T>
HK_FORCEINLINE void DegSinCos(T _Deg, T& _Sin, T& _Cos)
{
    SinCos(Radians(_Deg), _Sin, _Cos);
}


HK_FORCEINLINE float Atan2(float _Y, float _X)
{
    return std::atan2(_Y, _X);
}

HK_FORCEINLINE float Atan2Fast(float _Y, float _X)
{
    const float k1    = _PI / 4.0f;
    const float k2    = 3.0f * k1;
    const float absY  = Abs(_Y);
    const float angle = _X >= 0.0f ? (k1 - k1 * ((_X - absY) / (_X + absY))) : (k2 - k1 * ((_X + absY) / (absY - _X)));
    return _Y < 0.0f ? -angle : angle;
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
    constexpr Int2(const int32_t& _X, const int32_t& _Y) :
        X(_X), Y(_Y) {}

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
