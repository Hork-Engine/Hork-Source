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

#include "BaseMath.h"
#include "Bool.h"
#include "Char.h"
#include "IO.h"

class SignedByte;
class Byte;
class Short;
class UShort;
class Int;
class UInt;
class Long;
class ULong;
class Float;
class Double;

// 8 bits integer

class SignedByte final {
public:
    int8_t Value;

    SignedByte() = default;
    constexpr SignedByte( const int8_t & _Value ) : Value( _Value ) {}

    constexpr SignedByte( const SignedByte & _Value );
    constexpr SignedByte( const Byte & _Value );
    constexpr SignedByte( const Short & _Value );
    constexpr SignedByte( const UShort & _Value );
    constexpr SignedByte( const Int & _Value );
    constexpr SignedByte( const UInt & _Value );
    constexpr SignedByte( const Long & _Value );
    constexpr SignedByte( const ULong & _Value );
    constexpr SignedByte( const Float & _Value );
    constexpr SignedByte( const Double & _Value );

    SignedByte * ToPtr() {
        return this;
    }

    const SignedByte * ToPtr() const {
        return this;
    }

    Char * ToCharPtr() {
        return reinterpret_cast< Char * >( this );
    }

    const Char * ToCharPtr() const {
        return reinterpret_cast< const Char * >( this );
    }

    Char & ToChar() {
        return *reinterpret_cast< Char * >( this );
    }

    const Char & ToChar() const {
        return *reinterpret_cast< const Char * >( this );
    }

    operator int8_t() const {
        return Value;
    }

    SignedByte & operator=( const int8_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const int8_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int8_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int8_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int8_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int8_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int8_t & _Other ) const {
        return Value >= _Other;
    }
    bool operator<( const int & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = int8_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }

#if 0
    // SignedByte::operator==
    friend bool operator==( const SignedByte & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const SignedByte & _Right );
    friend bool operator==( const SignedByte & _Left, const SignedByte & _Right );

    // SignedByte::operator!=
    friend bool operator!=( const SignedByte & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const SignedByte & _Right );
    friend bool operator!=( const SignedByte & _Left, const SignedByte & _Right );

    // SignedByte::operator<
    friend bool operator<( const SignedByte & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const SignedByte & _Right );
    friend bool operator<( const SignedByte & _Left, const SignedByte & _Right );

    // SignedByte::operator>
    friend bool operator>( const SignedByte & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const SignedByte & _Right );
    friend bool operator>( const SignedByte & _Left, const SignedByte & _Right );

    // SignedByte::operator<=
    friend bool operator<=( const SignedByte & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const SignedByte & _Right );
    friend bool operator<=( const SignedByte & _Left, const SignedByte & _Right );

    // SignedByte::operator>=
    friend bool operator>=( const SignedByte & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const SignedByte & _Right );
    friend bool operator>=( const SignedByte & _Left, const SignedByte & _Right );
#endif

    // Math operators
    template< typename RightType >
    SignedByte operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    SignedByte operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    SignedByte operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    SignedByte operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    SignedByte operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    SignedByte operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    SignedByte operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    SignedByte operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    SignedByte operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    SignedByte operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    SignedByte operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }

    SignedByte & operator++() {
        ++Value;
        return *this;
    }
    SignedByte operator++(int) {
        return Value++;
    }
    SignedByte & operator--() {
        --Value;
        return *this;
    }
    SignedByte operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const int8_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const int8_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const int8_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const int8_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const int8_t & _Other ) const { return Value != _Other; }
    Bool Compare( const int8_t & _Other ) const { return Value == _Other; }

    SignedByte Abs() const {
        const int8_t Mask = Value >> 7;
        return ( ( Value ^ Mask ) - Mask );
    }

    SignedByte Dist( const int8_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    SignedByte ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    SignedByte ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    SignedByte ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    SignedByte Sign() const {
        return Value > 0 ? 1 : -SignBits();
    }

    int SignBits() const {
        return uint8_t( Value ) >> 7;
    }

    SignedByte Clamp( const int8_t & _Min, const int8_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    SignedByte SwapBytes() const {
        return Value;
    }

    SignedByte ToBigEndian() const {
        return Value;
    }

    SignedByte ToLittleEndian() const {
        return Value;
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 5 > buffer;
        return buffer.Sprintf( "%d", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%d", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    SignedByte & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    SignedByte & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( &Value, 1 );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( &Value, 1 );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return true; }
    static constexpr int BitsCount() { return sizeof( SignedByte ) * 8; }
    static constexpr SignedByte MinPowerOfTwo() { return 1; }
    static constexpr SignedByte MaxPowerOfTwo() { return 1 << ( BitsCount() - 2 ); }
    static constexpr SignedByte MinValue() { return std::numeric_limits< int8_t >::min(); }
    static constexpr SignedByte MaxValue() { return std::numeric_limits< int8_t >::max(); }
};

class Byte final {
public:
    uint8_t Value;

    Byte() = default;
    constexpr Byte( const uint8_t & _Value ) : Value( _Value ) {}

    constexpr Byte( const SignedByte & _Value );
    constexpr Byte( const Byte & _Value );
    constexpr Byte( const Short & _Value );
    constexpr Byte( const UShort & _Value );
    constexpr Byte( const Int & _Value );
    constexpr Byte( const UInt & _Value );
    constexpr Byte( const Long & _Value );
    constexpr Byte( const ULong & _Value );
    constexpr Byte( const Float & _Value );
    constexpr Byte( const Double & _Value );

    Byte * ToPtr() {
        return this;
    }

    const Byte * ToPtr() const {
        return this;
    }

    operator uint8_t() const {
        return Value;
    }

    Byte & operator=( const uint8_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const uint8_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const uint8_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const uint8_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const uint8_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const uint8_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const uint8_t & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = uint8_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }

#if 0
    // Byte::operator==
    friend bool operator==( const Byte & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const Byte & _Right );
    friend bool operator==( const Byte & _Left, const Byte & _Right );

    // Byte::operator!=
    friend bool operator!=( const Byte & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const Byte & _Right );
    friend bool operator!=( const Byte & _Left, const Byte & _Right );

    // Byte::operator<
    friend bool operator<( const Byte & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const Byte & _Right );
    friend bool operator<( const Byte & _Left, const Byte & _Right );

    // Byte::operator>
    friend bool operator>( const Byte & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const Byte & _Right );
    friend bool operator>( const Byte & _Left, const Byte & _Right );

    // Byte::operator<=
    friend bool operator<=( const Byte & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const Byte & _Right );
    friend bool operator<=( const Byte & _Left, const Byte & _Right );

    // Byte::operator>=
    friend bool operator>=( const Byte & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const Byte & _Right );
    friend bool operator>=( const Byte & _Left, const Byte & _Right );
#endif
    // Math operators
    template< typename RightType >
    Byte operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    Byte operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    Byte operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    Byte operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Byte operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Byte operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Byte operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Byte operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Byte operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Byte operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    Byte operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }

    Byte & operator++() {
        ++Value;
        return *this;
    }
    Byte operator++(int) {
        return Value++;
    }
    Byte & operator--() {
        --Value;
        return *this;
    }
    Byte operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const uint8_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const uint8_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const uint8_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const uint8_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const uint8_t & _Other ) const { return Value != _Other; }
    Bool Compare( const uint8_t & _Other ) const { return Value == _Other; }

    Byte Dist( const uint8_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Byte ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Byte ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Byte ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Byte Sign() const {
        return Value > 0;
    }

    constexpr int SignBits() const {
        return 0;
    }

    Byte Clamp( const uint8_t & _Min, const uint8_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    Byte SwapBytes() const {
        return Value;
    }

    Byte ToBigEndian() const {
        return Value;
    }

    Byte ToLittleEndian() const {
        return Value;
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 4 > buffer;
        return buffer.Sprintf( "%u", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%u", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Byte & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    Byte & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( &Value, 1 );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( &Value, 1 );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return false; }
    static constexpr int BitsCount() { return sizeof( Byte ) * 8; }
    static constexpr Byte MinPowerOfTwo() { return 1; }
    static constexpr Byte MaxPowerOfTwo() { return 1 << ( BitsCount() - 1 ); }
    static constexpr Byte MinValue() { return std::numeric_limits< uint8_t >::min(); }
    static constexpr Byte MaxValue() { return std::numeric_limits< uint8_t >::max(); }
};

// 16 bits integer

class Short final {
public:
    int16_t Value;

    Short() = default;
    constexpr Short( const int16_t & _Value ) : Value( _Value ) {}

    constexpr Short( const SignedByte & _Value );
    constexpr Short( const Byte & _Value );
    constexpr Short( const Short & _Value );
    constexpr Short( const UShort & _Value );
    constexpr Short( const Int & _Value );
    constexpr Short( const UInt & _Value );
    constexpr Short( const Long & _Value );
    constexpr Short( const ULong & _Value );
    constexpr Short( const Float & _Value );
    constexpr Short( const Double & _Value );

    Short * ToPtr() {
        return this;
    }

    const Short * ToPtr() const {
        return this;
    }

    operator int16_t() const {
        return Value;
    }

    Short & operator=( const int16_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const int16_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int16_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int16_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int16_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int16_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int16_t & _Other ) const {
        return Value >= _Other;
    }
    bool operator<( const int & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = int16_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }

    //// Short::operator==
    //friend bool operator==( const Short & _Left, const Type & _Right );
    //friend bool operator==( const Type & _Left, const Short & _Right );
    //friend bool operator==( const Short & _Left, const Short & _Right );

    //// Short::operator!=
    //friend bool operator!=( const Short & _Left, const Type & _Right );
    //friend bool operator!=( const Type & _Left, const Short & _Right );
    //friend bool operator!=( const Short & _Left, const Short & _Right );

    //// Short::operator<
    //friend bool operator<( const Short & _Left, const Type & _Right );
    //friend bool operator<( const Type & _Left, const Short & _Right );
    //friend bool operator<( const Short & _Left, const Short & _Right );

    //// Short::operator>
    //friend bool operator>( const Short & _Left, const Type & _Right );
    //friend bool operator>( const Type & _Left, const Short & _Right );
    //friend bool operator>( const Short & _Left, const Short & _Right );

    //// Short::operator<=
    //friend bool operator<=( const Short & _Left, const Type & _Right );
    //friend bool operator<=( const Type & _Left, const Short & _Right );
    //friend bool operator<=( const Short & _Left, const Short & _Right );

    //// Short::operator>=
    //friend bool operator>=( const Short & _Left, const Type & _Right );
    //friend bool operator>=( const Type & _Left, const Short & _Right );
    //friend bool operator>=( const Short & _Left, const Short & _Right );

    // Math operators
    template< typename RightType >
    Short operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    Short operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    Short operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    Short operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Short operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Short operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Short operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Short operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Short operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Short operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    Short operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }

    Short & operator++() {
        ++Value;
        return *this;
    }
    Short operator++(int) {
        return Value++;
    }
    Short & operator--() {
        --Value;
        return *this;
    }
    Short operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const int16_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const int16_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const int16_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const int16_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const int16_t & _Other ) const { return Value != _Other; }
    Bool Compare( const int16_t & _Other ) const { return Value == _Other; }

    Short Abs() const {
        const int16_t Mask = Value >> 15;
        return ( ( Value ^ Mask ) - Mask );
    }

    Short Dist( const int16_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Short ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Short ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Short ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Short Sign() const {
        return Value > 0 ? 1 : -SignBits();
    }

    int SignBits() const {
        return uint16_t( Value ) >> 15;
    }

    Short Clamp( const int16_t & _Min, const int16_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    Short SwapBytes() const {
        return ( ( Value & 0xff ) << 8 ) | ( ( Value >> 8 ) & 0xff);
    }

    Short ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    Short ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 7 > buffer;
        return buffer.Sprintf( "%d", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%d", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Short & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    Short & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( word * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( word * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return true; }
    static constexpr int BitsCount() { return sizeof( Short ) * 8; }
    static constexpr Short MinPowerOfTwo() { return 1; }
    static constexpr Short MaxPowerOfTwo() { return 1 << ( BitsCount() - 2 ); }
    static constexpr Short MinValue() { return std::numeric_limits< int16_t >::min(); }
    static constexpr Short MaxValue() { return std::numeric_limits< int16_t >::max(); }
};

class UShort final {
public:
    uint16_t Value;

    UShort() = default;
    constexpr UShort( const uint16_t & _Value ) : Value( _Value ) {}

    constexpr UShort( const SignedByte & _Value );
    constexpr UShort( const Byte & _Value );
    constexpr UShort( const Short & _Value );
    constexpr UShort( const UShort & _Value );
    constexpr UShort( const Int & _Value );
    constexpr UShort( const UInt & _Value );
    constexpr UShort( const Long & _Value );
    constexpr UShort( const ULong & _Value );
    constexpr UShort( const Float & _Value );
    constexpr UShort( const Double & _Value );

    UShort * ToPtr() {
        return this;
    }

    const UShort * ToPtr() const {
        return this;
    }

    operator uint16_t() const {
        return Value;
    }

    UShort & operator=( const uint16_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const uint16_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const uint16_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const uint16_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const uint16_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const uint16_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const uint16_t & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = uint16_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }
#if 0
    // UShort::operator==
    friend bool operator==( const UShort & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const UShort & _Right );
    friend bool operator==( const UShort & _Left, const UShort & _Right );

    // UShort::operator!=
    friend bool operator!=( const UShort & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const UShort & _Right );
    friend bool operator!=( const UShort & _Left, const UShort & _Right );

    // UShort::operator<
    friend bool operator<( const UShort & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const UShort & _Right );
    friend bool operator<( const UShort & _Left, const UShort & _Right );

    // UShort::operator>
    friend bool operator>( const UShort & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const UShort & _Right );
    friend bool operator>( const UShort & _Left, const UShort & _Right );

    // UShort::operator<=
    friend bool operator<=( const UShort & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const UShort & _Right );
    friend bool operator<=( const UShort & _Left, const UShort & _Right );

    // UShort::operator>=
    friend bool operator>=( const UShort & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const UShort & _Right );
    friend bool operator>=( const UShort & _Left, const UShort & _Right );
#endif

    // Math operators
    template< typename RightType >
    UShort operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    UShort operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    UShort operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    UShort operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UShort operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UShort operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UShort operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UShort operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UShort operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UShort operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    UShort operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }

    UShort & operator++() {
        ++Value;
        return *this;
    }
    UShort operator++(int) {
        return Value++;
    }
    UShort & operator--() {
        --Value;
        return *this;
    }
    UShort operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const uint16_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const uint16_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const uint16_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const uint16_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const uint16_t & _Other ) const { return Value != _Other; }
    Bool Compare( const uint16_t & _Other ) const { return Value == _Other; }

    UShort Dist( const uint16_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    UShort ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    UShort ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    UShort ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    UShort Sign() const {
        return Value > 0;
    }

    constexpr int SignBits() const {
        return 0;
    }

    // Return floating sign bit
    int HalfFloatSignBits() const {
        return Value >> 15;
    }

    // Return floating point exponent
    int HalfFloatExponent() const {
        return ( Value >> 10 ) & 0x1f;
    }

    // Return floating point mantissa
    int HalfFloatMantissa() const {
        return Value & 0x3ff;
    }

    UShort Clamp( const uint16_t & _Min, const uint16_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    UShort SwapBytes() const {
        return ( ( Value & 0xff ) << 8 ) | ( ( Value >> 8 ) & 0xff);
    }

    UShort ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    UShort ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 6 > buffer;
        return buffer.Sprintf( "%u", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%u", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    UShort & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    UShort & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( word * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( word * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return false; }
    static constexpr int BitsCount() { return sizeof( UShort ) * 8; }
    static constexpr UShort MinPowerOfTwo() { return 1; }
    static constexpr UShort MaxPowerOfTwo() { return 1 << ( BitsCount() - 1 ); }
    static constexpr UShort MinValue() { return std::numeric_limits< uint16_t >::min(); }
    static constexpr UShort MaxValue() { return std::numeric_limits< uint16_t >::max(); }
};

// 32 bits integer

class Int final {
public:
    int32_t Value;

    Int() = default;
    constexpr Int( const int32_t & _Value ) : Value( _Value ) {}

    constexpr Int( const SignedByte & _Value );
    constexpr Int( const Byte & _Value );
    constexpr Int( const Short & _Value );
    constexpr Int( const UShort & _Value );
    constexpr Int( const Int & _Value );
    constexpr Int( const UInt & _Value );
    constexpr Int( const Long & _Value );
    constexpr Int( const ULong & _Value );
    constexpr Int( const Float & _Value );
    constexpr Int( const Double & _Value );

    Int * ToPtr() {
        return this;
    }

    const Int * ToPtr() const {
        return this;
    }

    operator int32_t() const {
        return Value;
    }

//    Int & operator=( const int16_t & _Other ) {
//        Value = _Other;
//        return *this;
//    }

    Int & operator=( const int32_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const int32_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int32_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int32_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int32_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int32_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int32_t & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = int32_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }
#if 0
    // Int::operator==
    friend bool operator==( const Int & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const Int & _Right );
    friend bool operator==( const Int & _Left, const Int & _Right );

    // Int::operator!=
    friend bool operator!=( const Int & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const Int & _Right );
    friend bool operator!=( const Int & _Left, const Int & _Right );

    // Int::operator<
    friend bool operator<( const Int & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const Int & _Right );
    friend bool operator<( const Int & _Left, const Int & _Right );

    // Int::operator>
    friend bool operator>( const Int & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const Int & _Right );
    friend bool operator>( const Int & _Left, const Int & _Right );

    // Int::operator<=
    friend bool operator<=( const Int & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const Int & _Right );
    friend bool operator<=( const Int & _Left, const Int & _Right );

    // Int::operator>=
    friend bool operator>=( const Int & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const Int & _Right );
    friend bool operator>=( const Int & _Left, const Int & _Right );
#endif
    // Math operators
    template< typename RightType >
    Int operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    Int operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    Int operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    Int operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Int operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Int operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Int operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Int operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Int operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Int operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    Int operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }
    
    Int & operator++() {
        ++Value;
        return *this;
    }
    Int operator++(int) {
        return Value++;
    }
    Int & operator--() {
        --Value;
        return *this;
    }
    Int operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const int32_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const int32_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const int32_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const int32_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const int32_t & _Other ) const { return Value != _Other; }
    Bool Compare( const int32_t & _Other ) const { return Value == _Other; }

    Int Abs() const {
        const int32_t Mask = Value >> 31;
        return ( ( Value ^ Mask ) - Mask );
    }

    Int Dist( const int32_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Int ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Int ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Int ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Int Sign() const {
        return Value > 0 ? 1 : -SignBits();
    }

    int SignBits() const {
        return uint32_t( Value ) >> 31;
    }

    Int Clamp( const int32_t & _Min, const int32_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    Int SwapBytes() const {
        return ( ( Value & 0xff ) << 24 ) | ( ( ( Value >> 8 ) & 0xff ) << 16 ) | ( ( ( Value >> 16 ) & 0xff ) << 8 ) | ( ( Value >> 24 ) & 0xff );
    }

    Int ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    Int ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 12 > buffer;
        return buffer.Sprintf( "%d", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%d", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Int & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    Int & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( dword * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( dword * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return true; }
    static constexpr int BitsCount() { return sizeof( Int ) * 8; }
    static constexpr Int MinPowerOfTwo() { return 1; }
    static constexpr Int MaxPowerOfTwo() { return 1 << ( BitsCount() - 2 ); }
    static constexpr Int MinValue() { return std::numeric_limits< int32_t >::min(); }
    static constexpr Int MaxValue() { return std::numeric_limits< int32_t >::max(); }
};

class UInt final {
public:
    uint32_t Value;

    UInt() = default;
    constexpr UInt( const uint32_t & _Value ) : Value( _Value ) {}

    constexpr UInt( const SignedByte & _Value );
    constexpr UInt( const Byte & _Value );
    constexpr UInt( const Short & _Value );
    constexpr UInt( const UShort & _Value );
    constexpr UInt( const Int & _Value );
    constexpr UInt( const UInt & _Value );
    constexpr UInt( const Long & _Value );
    constexpr UInt( const ULong & _Value );
    constexpr UInt( const Float & _Value );
    constexpr UInt( const Double & _Value );

    UInt * ToPtr() {
        return this;
    }

    const UInt * ToPtr() const {
        return this;
    }

    operator uint32_t() const {
        return Value;
    }

    UInt & operator=( const uint32_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const uint32_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const uint32_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const uint32_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const uint32_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const uint32_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const uint32_t & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = uint32_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }
#if 0
    // UInt::operator==
    friend bool operator==( const UInt & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const UInt & _Right );
    friend bool operator==( const UInt & _Left, const UInt & _Right );

    // UInt::operator!=
    friend bool operator!=( const UInt & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const UInt & _Right );
    friend bool operator!=( const UInt & _Left, const UInt & _Right );

    // UInt::operator<
    friend bool operator<( const UInt & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const UInt & _Right );
    friend bool operator<( const UInt & _Left, const UInt & _Right );

    // UInt::operator>
    friend bool operator>( const UInt & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const UInt & _Right );
    friend bool operator>( const UInt & _Left, const UInt & _Right );

    // UInt::operator<=
    friend bool operator<=( const UInt & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const UInt & _Right );
    friend bool operator<=( const UInt & _Left, const UInt & _Right );

    // UInt::operator>=
    friend bool operator>=( const UInt & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const UInt & _Right );
    friend bool operator>=( const UInt & _Left, const UInt & _Right );
#endif
    // Math operators
    template< typename RightType >
    UInt operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    UInt operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    UInt operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    UInt operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UInt operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UInt operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UInt operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UInt operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UInt operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    UInt operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    UInt operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }
    
    UInt & operator++() {
        ++Value;
        return *this;
    }
    UInt operator++(int) {
        return Value++;
    }
    UInt & operator--() {
        --Value;
        return *this;
    }
    UInt operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const uint32_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const uint32_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const uint32_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const uint32_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const uint32_t & _Other ) const { return Value != _Other; }
    Bool Compare( const uint32_t & _Other ) const { return Value == _Other; }

    UInt Dist( const uint32_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    UInt ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    UInt ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    UInt ToClosestPowerOfTwo() const;

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    UInt Sign() const {
        return Value > 0;
    }

    constexpr int SignBits() const {
        return 0;
    }

    UInt Clamp( const uint32_t & _Min, const uint32_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    UInt SwapBytes() const {
        return ( ( Value & 0xff ) << 24 ) | ( ( ( Value >> 8 ) & 0xff ) << 16 ) | ( ( ( Value >> 16 ) & 0xff ) << 8 ) | ( ( Value >> 24 ) & 0xff );
    }

    UInt ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    UInt ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 11 > buffer;
        return buffer.Sprintf( "%u", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%u", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    UInt & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    UInt & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( dword * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( dword * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return false; }
    static constexpr int BitsCount() { return sizeof( UInt ) * 8; }
    static constexpr UInt MinPowerOfTwo() { return 1; }
    static constexpr UInt MaxPowerOfTwo() { return 1u << ( BitsCount() - 1 ); }
    static constexpr UInt MinValue() { return std::numeric_limits< uint32_t >::min(); }
    static constexpr UInt MaxValue() { return std::numeric_limits< uint32_t >::max(); }
};

// 64 bits integer

class Long final {
public:
    int64_t Value;

    Long() = default;
    constexpr Long( const int64_t & _Value ) : Value( _Value ) {}

    constexpr Long( const SignedByte & _Value );
    constexpr Long( const Byte & _Value );
    constexpr Long( const Short & _Value );
    constexpr Long( const UShort & _Value );
    constexpr Long( const Int & _Value );
    constexpr Long( const UInt & _Value );
    constexpr Long( const Long & _Value );
    constexpr Long( const ULong & _Value );
    constexpr Long( const Float & _Value );
    constexpr Long( const Double & _Value );

    Long * ToPtr() {
        return this;
    }

    const Long * ToPtr() const {
        return this;
    }

    operator int64_t() const {
        return Value;
    }

    Long & operator=( const int64_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const int64_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int64_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int64_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int64_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int64_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int64_t & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = int64_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }

#if 0
    // Long::operator==
    friend bool operator==( const Long & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const Long & _Right );
    friend bool operator==( const Long & _Left, const Long & _Right );

    // Long::operator!=
    friend bool operator!=( const Long & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const Long & _Right );
    friend bool operator!=( const Long & _Left, const Long & _Right );

    // Long::operator<
    friend bool operator<( const Long & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const Long & _Right );
    friend bool operator<( const Long & _Left, const Long & _Right );

    // Long::operator>
    friend bool operator>( const Long & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const Long & _Right );
    friend bool operator>( const Long & _Left, const Long & _Right );

    // Long::operator<=
    friend bool operator<=( const Long & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const Long & _Right );
    friend bool operator<=( const Long & _Left, const Long & _Right );

    // Long::operator>=
    friend bool operator>=( const Long & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const Long & _Right );
    friend bool operator>=( const Long & _Left, const Long & _Right );
#endif
    // Math operators
    template< typename RightType >
    Long operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    Long operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    Long operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    Long operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Long operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Long operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Long operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Long operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Long operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    Long operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    Long operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }
    
    Long & operator++() {
        ++Value;
        return *this;
    }
    Long operator++(int) {
        return Value++;
    }
    Long & operator--() {
        --Value;
        return *this;
    }
    Long operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const int64_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const int64_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const int64_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const int64_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const int64_t & _Other ) const { return Value != _Other; }
    Bool Compare( const int64_t & _Other ) const { return Value == _Other; }

    Long Abs() const {
        const int64_t Mask = Value >> 63;
        return ( ( Value ^ Mask ) - Mask );
    }

    Long Dist( const int64_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Long ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Long ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    Long ToClosestPowerOfTwo() const;

    UInt HighPart() const {
        return Value >> 32;
    }

    UInt LowPart() const {
        return Value & 0xFFFFFFFF;
    }

    // Return 1 if value is greater than 0, -1 if value is less than 0, 0 if value equal to 0
    Long Sign() const {
        return Value > 0 ? 1 : -SignBits();
    }

    int SignBits() const {
        return uint64_t( Value ) >> 63;
    }

    Long Clamp( const int64_t & _Min, const int64_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    Long SwapBytes() const {
        return ( ( ( Value       ) & 0xff ) << 56 )
             | ( ( ( Value >> 8  ) & 0xff ) << 48 )
             | ( ( ( Value >> 16 ) & 0xff ) << 40 )
             | ( ( ( Value >> 24 ) & 0xff ) << 32 )
             | ( ( ( Value >> 32 ) & 0xff ) << 24 )
             | ( ( ( Value >> 40 ) & 0xff ) << 16 )
             | ( ( ( Value >> 48 ) & 0xff ) << 8  )
             | ( ( ( Value >> 56 ) & 0xff )       );
    }

    Long ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    Long ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 21 > buffer;
        return buffer.Sprintf( "%d", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%d", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    Long & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    Long & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( ddword * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( ddword * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return true; }
    static constexpr int BitsCount() { return sizeof( Long ) * 8; }
    static constexpr Long MinPowerOfTwo() { return 1; }
    static constexpr Long MaxPowerOfTwo() { return int64_t(1) << ( BitsCount() - 2 ); }
    static constexpr Long MinValue() { return std::numeric_limits< int64_t >::min(); }
    static constexpr Long MaxValue() { return std::numeric_limits< int64_t >::max(); }
};

class ULong final {
public:
    uint64_t Value;

    ULong() = default;
    constexpr ULong( const uint64_t & _Value ) : Value( _Value ) {}

    constexpr ULong( const SignedByte & _Value );
    constexpr ULong( const Byte & _Value );
    constexpr ULong( const Short & _Value );
    constexpr ULong( const UShort & _Value );
    constexpr ULong( const Int & _Value );
    constexpr ULong( const UInt & _Value );
    constexpr ULong( const Long & _Value );
    constexpr ULong( const ULong & _Value );
    constexpr ULong( const Float & _Value );
    constexpr ULong( const Double & _Value );

    ULong * ToPtr() {
        return this;
    }

    const ULong * ToPtr() const {
        return this;
    }

    operator uint64_t() const {
        return Value;
    }

    ULong & operator=( const uint64_t & _Other ) {
        Value = _Other;
        return *this;
    }

    // Comparision operators
#if 0
    bool operator<( const uint64_t & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const uint64_t & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const uint64_t & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const uint64_t & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const uint64_t & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const uint64_t & _Other ) const {
        return Value >= _Other;
    }

    bool operator<( const int & _Other ) const {
        return Value < _Other;
    }
    bool operator>( const int & _Other ) const {
        return Value < _Other;
    }
    bool operator==( const int & _Other ) const {
        return Value == _Other;
    }
    bool operator!=( const int & _Other ) const {
        return Value != _Other;
    }
    bool operator<=( const int & _Other ) const {
        return Value <= _Other;
    }
    bool operator>=( const int & _Other ) const {
        return Value >= _Other;
    }
#endif
    using Type = uint64_t;

    template< typename RightType > bool operator==( const RightType & _Right ) const { return Value == _Right; }
    template< typename RightType > bool operator!=( const RightType & _Right ) const { return Value != _Right; }
    template< typename RightType > bool operator<( const RightType & _Right ) const { return Value < _Right; }
    template< typename RightType > bool operator>( const RightType & _Right ) const { return Value > _Right; }
    template< typename RightType > bool operator<=( const RightType & _Right ) const { return Value <= _Right; }
    template< typename RightType > bool operator>=( const RightType & _Right ) const { return Value >= _Right; }
#if 0
    // ULong::operator==
    friend bool operator==( const ULong & _Left, const Type & _Right );
    friend bool operator==( const Type & _Left, const ULong & _Right );
    friend bool operator==( const ULong & _Left, const ULong & _Right );

    // ULong::operator!=
    friend bool operator!=( const ULong & _Left, const Type & _Right );
    friend bool operator!=( const Type & _Left, const ULong & _Right );
    friend bool operator!=( const ULong & _Left, const ULong & _Right );

    // ULong::operator<
    friend bool operator<( const ULong & _Left, const Type & _Right );
    friend bool operator<( const Type & _Left, const ULong & _Right );
    friend bool operator<( const ULong & _Left, const ULong & _Right );

    // ULong::operator>
    friend bool operator>( const ULong & _Left, const Type & _Right );
    friend bool operator>( const Type & _Left, const ULong & _Right );
    friend bool operator>( const ULong & _Left, const ULong & _Right );

    // ULong::operator<=
    friend bool operator<=( const ULong & _Left, const Type & _Right );
    friend bool operator<=( const Type & _Left, const ULong & _Right );
    friend bool operator<=( const ULong & _Left, const ULong & _Right );

    // ULong::operator>=
    friend bool operator>=( const ULong & _Left, const Type & _Right );
    friend bool operator>=( const Type & _Left, const ULong & _Right );
    friend bool operator>=( const ULong & _Left, const ULong & _Right );
#endif
    // Math operators
    template< typename RightType >
    ULong operator+( const RightType & _Other ) const {
        return Value + _Other;
    }
    template< typename RightType >
    ULong operator-( const RightType & _Other ) const {
        return Value - _Other;
    }
    template< typename RightType >
    ULong operator/( const RightType & _Other ) const {
        return Value / _Other;
    }
    template< typename RightType >
    ULong operator*( const RightType & _Other ) const {
        return Value * _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    ULong operator%( const RightType & _Other ) const {
        return Value % _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    ULong operator<<( const RightType & _Other ) const {
        return Value << _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    ULong operator>>( const RightType & _Other ) const {
        return Value >> _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    ULong operator|( const RightType & _Other ) const {
        return Value | _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    ULong operator&( const RightType & _Other ) const {
        return Value & _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    ULong operator^( const RightType & _Other ) const {
        return Value ^ _Other;
    }
    ULong operator~() const {
        return ~Value;
    }
    template< typename RightType >
    void operator+=( const RightType & _Other ) {
        Value += _Other;
    }
    template< typename RightType >
    void operator-=( const RightType & _Other ) {
        Value -= _Other;
    }
    template< typename RightType >
    void operator/=( const RightType & _Other ) {
        Value /= _Other;
    }
    template< typename RightType >
    void operator*=( const RightType & _Other ) {
        Value *= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator%=( const RightType & _Other ) {
        Value %= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator<<=( const RightType & _Other ) {
        Value <<= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator>>=( const RightType & _Other ) {
        Value >>= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator|=( const RightType & _Other ) {
        Value |= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator&=( const RightType & _Other ) {
        Value &= _Other;
    }
    template< typename RightType, typename = TEnableIf< std::is_integral<RightType>::value > >
    void operator^=( const RightType & _Other ) {
        Value ^= _Other;
    }
    
    ULong & operator++() {
        ++Value;
        return *this;
    }
    ULong operator++(int) {
        return Value++;
    }
    ULong & operator--() {
        --Value;
        return *this;
    }
    ULong operator--(int) {
        return Value--;
    }

    // Comparision
    Bool LessThan( const uint64_t & _Other ) const { return Value < _Other; }
    Bool LequalThan( const uint64_t & _Other ) const { return Value <= _Other; }
    Bool GreaterThan( const uint64_t & _Other ) const { return Value > _Other; }
    Bool GequalThan( const uint64_t & _Other ) const { return Value >= _Other; }
    Bool NotEqual( const uint64_t & _Other ) const { return Value != _Other; }
    Bool Compare( const uint64_t & _Other ) const { return Value == _Other; }

    ULong Dist( const uint64_t & _Other ) const {
        // NOTE: We don't use Abs() to control value overflow
        return ( _Other > Value ) ? ( _Other - Value ) : ( Value - _Other );
    }

    Bool IsPowerOfTwo() const {
        return ( Value & ( Value - 1 ) ) == 0 && Value > 0;
    }

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    ULong ToGreaterPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    ULong ToLessPowerOfTwo() const;

    // Return value is between MinPowerOfTwo and MaxPowerOfTwo
    ULong ToClosestPowerOfTwo() const;

    UInt HighPart() const {
        return Value >> 32;
    }

    UInt LowPart() const {
        return Value & 0xFFFFFFFF;
    }

    ULong Sign() const {
        return Value > 0;
    }

    constexpr int SignBits() const {
        return 0;
    }

    ULong Clamp( const uint64_t & _Min, const uint64_t & _Max ) const {
        return FMath::Min( FMath::Max( Value, _Min ), _Max );
    }

    ULong SwapBytes() const {
        return ( ( ( Value       ) & 0xff ) << 56 )
             | ( ( ( Value >> 8  ) & 0xff ) << 48 )
             | ( ( ( Value >> 16 ) & 0xff ) << 40 )
             | ( ( ( Value >> 24 ) & 0xff ) << 32 )
             | ( ( ( Value >> 32 ) & 0xff ) << 24 )
             | ( ( ( Value >> 40 ) & 0xff ) << 16 )
             | ( ( ( Value >> 48 ) & 0xff ) << 8  )
             | ( ( ( Value >> 56 ) & 0xff )       );
    }

    ULong ToBigEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return SwapBytes();
    #else
        return Value;
    #endif
    }

    ULong ToLittleEndian() const {
    #ifdef AN_LITTLE_ENDIAN
        return Value;
    #else
        return SwapBytes();
    #endif
    }

    // String conversions
    FString ToString() const {
        TSprintfBuffer< 21 > buffer;
        return buffer.Sprintf( "%u", Value );
    }

    const char * ToConstChar() const {
        return FString::Fmt( "%u", Value );
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString::ToHexString( Value, _LeadingZeros, _Prefix );
    }

    ULong & FromString( const FString & _String ) {
        return FromString( _String.ToConstChar() );
    }

    ULong & FromString( const char * _String );

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream.Write( *( ddword * )&Value );
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream.Read( *( ddword * )&Value );
    }

    // Static methods
    static constexpr int NumComponents() { return 1; }
    static constexpr bool IsSigned() { return false; }
    static constexpr int BitsCount() { return sizeof( ULong ) * 8; }
    static constexpr ULong MinPowerOfTwo() { return 1; }
    static constexpr ULong MaxPowerOfTwo() { return uint64_t(1) << ( BitsCount() - 1 ); }
    static constexpr ULong MinValue() { return std::numeric_limits< uint64_t >::min(); }
    static constexpr ULong MaxValue() { return std::numeric_limits< uint64_t >::max(); }
};

AN_FORCEINLINE constexpr SignedByte::SignedByte( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr SignedByte::SignedByte( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr Byte::Byte( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Byte::Byte( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr Short::Short( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Short::Short( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr UShort::UShort( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UShort::UShort( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr Int::Int( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Int::Int( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr UInt::UInt( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr UInt::UInt( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr Long::Long( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr Long::Long( const ULong & _Value ) : Value( _Value.Value ) {}

AN_FORCEINLINE constexpr ULong::ULong( const SignedByte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const Byte & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const Short & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const UShort & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const Int & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const UInt & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const Long & _Value ) : Value( _Value.Value ) {}
AN_FORCEINLINE constexpr ULong::ULong( const ULong & _Value ) : Value( _Value.Value ) {}

#if 0
// SignedByte::operator==
AN_FORCEINLINE bool operator==( const SignedByte & _Left, const int8_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const int8_t & _Left, const SignedByte & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const SignedByte & _Left, const SignedByte & _Right ) { return _Left.Value == _Right.Value; }

// SignedByte::operator!=
AN_FORCEINLINE bool operator!=( const SignedByte & _Left, const int8_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const int8_t & _Left, const SignedByte & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const SignedByte & _Left, const SignedByte & _Right ) { return _Left.Value != _Right.Value; }

// SignedByte::operator<
AN_FORCEINLINE bool operator<( const SignedByte & _Left, const int8_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const int8_t & _Left, const SignedByte & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const SignedByte & _Left, const SignedByte & _Right ) { return _Left.Value < _Right.Value; }

// SignedByte::operator>
AN_FORCEINLINE bool operator>( const SignedByte & _Left, const int8_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const int8_t & _Left, const SignedByte & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const SignedByte & _Left, const SignedByte & _Right ) { return _Left.Value > _Right.Value; }

// SignedByte::operator<=
AN_FORCEINLINE bool operator<=( const SignedByte & _Left, const int8_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const int8_t & _Left, const SignedByte & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const SignedByte & _Left, const SignedByte & _Right ) { return _Left.Value <= _Right.Value; }

// SignedByte::operator>=
AN_FORCEINLINE bool operator>=( const SignedByte & _Left, const int8_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const int8_t & _Left, const SignedByte & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const SignedByte & _Left, const SignedByte & _Right ) { return _Left.Value >= _Right.Value; }

// Byte::operator==
AN_FORCEINLINE bool operator==( const Byte & _Left, const uint8_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const uint8_t & _Left, const Byte & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const Byte & _Left, const Byte & _Right ) { return _Left.Value == _Right.Value; }

// Byte::operator!=
AN_FORCEINLINE bool operator!=( const Byte & _Left, const uint8_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const uint8_t & _Left, const Byte & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const Byte & _Left, const Byte & _Right ) { return _Left.Value != _Right.Value; }

// Byte::operator<
AN_FORCEINLINE bool operator<( const Byte & _Left, const uint8_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const uint8_t & _Left, const Byte & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const Byte & _Left, const Byte & _Right ) { return _Left.Value < _Right.Value; }

// Byte::operator>
AN_FORCEINLINE bool operator>( const Byte & _Left, const uint8_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const uint8_t & _Left, const Byte & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const Byte & _Left, const Byte & _Right ) { return _Left.Value > _Right.Value; }

// Byte::operator<=
AN_FORCEINLINE bool operator<=( const Byte & _Left, const uint8_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const uint8_t & _Left, const Byte & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const Byte & _Left, const Byte & _Right ) { return _Left.Value <= _Right.Value; }

// Byte::operator>=
AN_FORCEINLINE bool operator>=( const Byte & _Left, const uint8_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const uint8_t & _Left, const Byte & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const Byte & _Left, const Byte & _Right ) { return _Left.Value >= _Right.Value; }

// Short::operator==
AN_FORCEINLINE bool operator==( const Short & _Left, const int16_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const int16_t & _Left, const Short & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const Short & _Left, const Short & _Right ) { return _Left.Value == _Right.Value; }

// Short::operator!=
AN_FORCEINLINE bool operator!=( const Short & _Left, const int16_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const int16_t & _Left, const Short & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const Short & _Left, const Short & _Right ) { return _Left.Value != _Right.Value; }

// Short::operator<
AN_FORCEINLINE bool operator<( const Short & _Left, const int16_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const int16_t & _Left, const Short & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const Short & _Left, const Short & _Right ) { return _Left.Value < _Right.Value; }

// Short::operator>
AN_FORCEINLINE bool operator>( const Short & _Left, const int16_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const int16_t & _Left, const Short & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const Short & _Left, const Short & _Right ) { return _Left.Value > _Right.Value; }

// Short::operator<=
AN_FORCEINLINE bool operator<=( const Short & _Left, const int16_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const int16_t & _Left, const Short & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const Short & _Left, const Short & _Right ) { return _Left.Value <= _Right.Value; }

// Short::operator>=
AN_FORCEINLINE bool operator>=( const Short & _Left, const int16_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const int16_t & _Left, const Short & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const Short & _Left, const Short & _Right ) { return _Left.Value >= _Right.Value; }

// UShort::operator==
AN_FORCEINLINE bool operator==( const UShort & _Left, const uint16_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const uint16_t & _Left, const UShort & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const UShort & _Left, const UShort & _Right ) { return _Left.Value == _Right.Value; }

// UShort::operator!=
AN_FORCEINLINE bool operator!=( const UShort & _Left, const uint16_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const uint16_t & _Left, const UShort & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const UShort & _Left, const UShort & _Right ) { return _Left.Value != _Right.Value; }

// UShort::operator<
AN_FORCEINLINE bool operator<( const UShort & _Left, const uint16_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const uint16_t & _Left, const UShort & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const UShort & _Left, const UShort & _Right ) { return _Left.Value < _Right.Value; }

// UShort::operator>
AN_FORCEINLINE bool operator>( const UShort & _Left, const uint16_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const uint16_t & _Left, const UShort & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const UShort & _Left, const UShort & _Right ) { return _Left.Value > _Right.Value; }

// UShort::operator<=
AN_FORCEINLINE bool operator<=( const UShort & _Left, const uint16_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const uint16_t & _Left, const UShort & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const UShort & _Left, const UShort & _Right ) { return _Left.Value <= _Right.Value; }

// UShort::operator>=
AN_FORCEINLINE bool operator>=( const UShort & _Left, const uint16_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const uint16_t & _Left, const UShort & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const UShort & _Left, const UShort & _Right ) { return _Left.Value >= _Right.Value; }

// Int::operator==
AN_FORCEINLINE bool operator==( const Int & _Left, const int32_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const int32_t & _Left, const Int & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const Int & _Left, const Int & _Right ) { return _Left.Value == _Right.Value; }

// Int::operator!=
AN_FORCEINLINE bool operator!=( const Int & _Left, const int32_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const int32_t & _Left, const Int & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const Int & _Left, const Int & _Right ) { return _Left.Value != _Right.Value; }

// Int::operator<
AN_FORCEINLINE bool operator<( const Int & _Left, const int32_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const int32_t & _Left, const Int & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const Int & _Left, const Int & _Right ) { return _Left.Value < _Right.Value; }

// Int::operator>
AN_FORCEINLINE bool operator>( const Int & _Left, const int32_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const int32_t & _Left, const Int & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const Int & _Left, const Int & _Right ) { return _Left.Value > _Right.Value; }

// Int::operator<=
AN_FORCEINLINE bool operator<=( const Int & _Left, const int32_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const int32_t & _Left, const Int & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const Int & _Left, const Int & _Right ) { return _Left.Value <= _Right.Value; }

// Int::operator>=
AN_FORCEINLINE bool operator>=( const Int & _Left, const int32_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const int32_t & _Left, const Int & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const Int & _Left, const Int & _Right ) { return _Left.Value >= _Right.Value; }

// UInt::operator==
AN_FORCEINLINE bool operator==( const UInt & _Left, const uint32_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const uint32_t & _Left, const UInt & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const UInt & _Left, const UInt & _Right ) { return _Left.Value == _Right.Value; }

// UInt::operator!=
AN_FORCEINLINE bool operator!=( const UInt & _Left, const uint32_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const uint32_t & _Left, const UInt & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const UInt & _Left, const UInt & _Right ) { return _Left.Value != _Right.Value; }

// UInt::operator<
AN_FORCEINLINE bool operator<( const UInt & _Left, const uint32_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const uint32_t & _Left, const UInt & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const UInt & _Left, const UInt & _Right ) { return _Left.Value < _Right.Value; }

// UInt::operator>
AN_FORCEINLINE bool operator>( const UInt & _Left, const uint32_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const uint32_t & _Left, const UInt & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const UInt & _Left, const UInt & _Right ) { return _Left.Value > _Right.Value; }

// UInt::operator<=
AN_FORCEINLINE bool operator<=( const UInt & _Left, const uint32_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const uint32_t & _Left, const UInt & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const UInt & _Left, const UInt & _Right ) { return _Left.Value <= _Right.Value; }

// UInt::operator>=
AN_FORCEINLINE bool operator>=( const UInt & _Left, const uint32_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const uint32_t & _Left, const UInt & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const UInt & _Left, const UInt & _Right ) { return _Left.Value >= _Right.Value; }

// Long::operator==
AN_FORCEINLINE bool operator==( const Long & _Left, const int64_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const int64_t & _Left, const Long & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const Long & _Left, const Long & _Right ) { return _Left.Value == _Right.Value; }

// Long::operator!=
AN_FORCEINLINE bool operator!=( const Long & _Left, const int64_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const int64_t & _Left, const Long & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const Long & _Left, const Long & _Right ) { return _Left.Value != _Right.Value; }

// Long::operator<
AN_FORCEINLINE bool operator<( const Long & _Left, const int64_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const int64_t & _Left, const Long & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const Long & _Left, const Long & _Right ) { return _Left.Value < _Right.Value; }

// Long::operator>
AN_FORCEINLINE bool operator>( const Long & _Left, const int64_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const int64_t & _Left, const Long & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const Long & _Left, const Long & _Right ) { return _Left.Value > _Right.Value; }

// Long::operator<=
AN_FORCEINLINE bool operator<=( const Long & _Left, const int64_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const int64_t & _Left, const Long & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const Long & _Left, const Long & _Right ) { return _Left.Value <= _Right.Value; }

// Long::operator>=
AN_FORCEINLINE bool operator>=( const Long & _Left, const int64_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const int64_t & _Left, const Long & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const Long & _Left, const Long & _Right ) { return _Left.Value >= _Right.Value; }

// ULong::operator==
AN_FORCEINLINE bool operator==( const ULong & _Left, const uint64_t & _Right ) { return _Left.Value == _Right; }
AN_FORCEINLINE bool operator==( const uint64_t & _Left, const ULong & _Right ) { return _Left == _Right.Value; }
AN_FORCEINLINE bool operator==( const ULong & _Left, const ULong & _Right ) { return _Left.Value == _Right.Value; }

// ULong::operator!=
AN_FORCEINLINE bool operator!=( const ULong & _Left, const uint64_t & _Right ) { return _Left.Value != _Right; }
AN_FORCEINLINE bool operator!=( const uint64_t & _Left, const ULong & _Right ) { return _Left != _Right.Value; }
AN_FORCEINLINE bool operator!=( const ULong & _Left, const ULong & _Right ) { return _Left.Value != _Right.Value; }

// ULong::operator<
AN_FORCEINLINE bool operator<( const ULong & _Left, const uint64_t & _Right ) { return _Left.Value < _Right; }
AN_FORCEINLINE bool operator<( const uint64_t & _Left, const ULong & _Right ) { return _Left < _Right.Value; }
AN_FORCEINLINE bool operator<( const ULong & _Left, const ULong & _Right ) { return _Left.Value < _Right.Value; }

// ULong::operator>
AN_FORCEINLINE bool operator>( const ULong & _Left, const uint64_t & _Right ) { return _Left.Value > _Right; }
AN_FORCEINLINE bool operator>( const uint64_t & _Left, const ULong & _Right ) { return _Left > _Right.Value; }
AN_FORCEINLINE bool operator>( const ULong & _Left, const ULong & _Right ) { return _Left.Value > _Right.Value; }

// ULong::operator<=
AN_FORCEINLINE bool operator<=( const ULong & _Left, const uint64_t & _Right ) { return _Left.Value <= _Right; }
AN_FORCEINLINE bool operator<=( const uint64_t & _Left, const ULong & _Right ) { return _Left <= _Right.Value; }
AN_FORCEINLINE bool operator<=( const ULong & _Left, const ULong & _Right ) { return _Left.Value <= _Right.Value; }

// ULong::operator>=
AN_FORCEINLINE bool operator>=( const ULong & _Left, const uint64_t & _Right ) { return _Left.Value >= _Right; }
AN_FORCEINLINE bool operator>=( const uint64_t & _Left, const ULong & _Right ) { return _Left >= _Right.Value; }
AN_FORCEINLINE bool operator>=( const ULong & _Left, const ULong & _Right ) { return _Left.Value >= _Right.Value; }

#endif

AN_FORCEINLINE SignedByte SignedByte::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    int8_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val + 1;
}

AN_FORCEINLINE SignedByte SignedByte::ToLessPowerOfTwo() const {
    int8_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE SignedByte SignedByte::ToClosestPowerOfTwo() const {
    SignedByte GreaterPow = ToGreaterPowerOfTwo();
    SignedByte LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE Byte Byte::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    uint8_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val + 1;
}

AN_FORCEINLINE Byte Byte::ToLessPowerOfTwo() const {
    uint8_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE Byte Byte::ToClosestPowerOfTwo() const {
    Byte GreaterPow = ToGreaterPowerOfTwo();
    Byte LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE Short Short::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    int16_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val + 1;
}

AN_FORCEINLINE Short Short::ToLessPowerOfTwo() const {
    int16_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE Short Short::ToClosestPowerOfTwo() const {
    Short GreaterPow = ToGreaterPowerOfTwo();
    Short LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE UShort UShort::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    uint16_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val + 1;
}

AN_FORCEINLINE UShort UShort::ToLessPowerOfTwo() const {
    uint16_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE UShort UShort::ToClosestPowerOfTwo() const {
    UShort GreaterPow = ToGreaterPowerOfTwo();
    UShort LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE Int Int::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    int32_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val + 1;
}

AN_FORCEINLINE Int Int::ToLessPowerOfTwo() const {
    int32_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE Int Int::ToClosestPowerOfTwo() const {
    Int GreaterPow = ToGreaterPowerOfTwo();
    Int LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE UInt UInt::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    uint32_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val + 1;
}

AN_FORCEINLINE UInt UInt::ToLessPowerOfTwo() const {
    uint32_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE UInt UInt::ToClosestPowerOfTwo() const {
    UInt GreaterPow = ToGreaterPowerOfTwo();
    UInt LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE Long Long::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    int64_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val + 1;
}

AN_FORCEINLINE Long Long::ToLessPowerOfTwo() const {
    int64_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE Long Long::ToClosestPowerOfTwo() const {
    Long GreaterPow = ToGreaterPowerOfTwo();
    Long LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

AN_FORCEINLINE ULong ULong::ToGreaterPowerOfTwo() const {
    if ( Value >= MaxPowerOfTwo() ) {
        return MaxPowerOfTwo();
    }
    if ( Value < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    uint64_t Val = Value - 1;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val + 1;
}

AN_FORCEINLINE ULong ULong::ToLessPowerOfTwo() const {
    uint64_t Val = Value;
    if ( Val < MinPowerOfTwo() ) {
        return MinPowerOfTwo();
    }
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    return Val - ( Val >> 1 );
}

AN_FORCEINLINE ULong ULong::ToClosestPowerOfTwo() const {
    ULong GreaterPow = ToGreaterPowerOfTwo();
    ULong LessPow = ToLessPowerOfTwo();
    return GreaterPow.Dist( Value ) < LessPow.Dist( Value ) ? GreaterPow : LessPow;
}

class SignedByte2 final {
public:
    SignedByte X;
    SignedByte Y;

    SignedByte2() = default;
    explicit constexpr SignedByte2( const int8_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr SignedByte2( const int8_t & _X, const int8_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class Byte2 final {
public:
    Byte X;
    Byte Y;

    Byte2() = default;
    explicit constexpr Byte2( const uint8_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Byte2( const uint8_t & _X, const uint8_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class Short2 final {
public:
    Short X;
    Short Y;

    Short2() = default;
    explicit constexpr Short2( const int16_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Short2( const int16_t & _X, const int16_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class UShort2 final {
public:
    UShort X;
    UShort Y;

    UShort2() = default;
    explicit constexpr UShort2( const uint16_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr UShort2( const uint16_t & _X, const uint16_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class Int2 final {
public:
    Int X;
    Int Y;

    Int2() = default;
    explicit constexpr Int2( const int32_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Int2( const int32_t & _X, const int32_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class UInt2 final {
public:
    UInt X;
    UInt Y;

    UInt2() = default;
    explicit constexpr UInt2( const uint32_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr UInt2( const uint32_t & _X, const uint32_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class Long2 final {
public:
    Long X;
    Long Y;

    Long2() = default;
    explicit constexpr Long2( const int64_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr Long2( const int64_t & _X, const int64_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class ULong2 final {
public:
    ULong X;
    ULong Y;

    ULong2() = default;
    explicit constexpr ULong2( const uint64_t & _Value ) : X( _Value ), Y( _Value ) {}
    constexpr ULong2( const uint64_t & _X, const uint64_t & _Y ) : X( _X ), Y( _Y ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
    }
};
class SignedByte3 final {
public:
    SignedByte X;
    SignedByte Y;
    SignedByte Z;

    SignedByte3() = default;
    explicit constexpr SignedByte3( const int8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr SignedByte3( const int8_t & _X, const int8_t & _Y, const int8_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class Byte3 final {
public:
    Byte X;
    Byte Y;
    Byte Z;

    Byte3() = default;
    explicit constexpr Byte3( const uint8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Byte3( const uint8_t & _X, const uint8_t & _Y, const uint8_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class Short3 final {
public:
    Short X;
    Short Y;
    Short Z;

    Short3() = default;
    explicit constexpr Short3( const int16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Short3( const int16_t & _X, const int16_t & _Y, const int16_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class UShort3 final {
public:
    UShort X;
    UShort Y;
    UShort Z;

    UShort3() = default;
    explicit constexpr UShort3( const uint16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr UShort3( const uint16_t & _X, const uint16_t & _Y, const uint16_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class Int3 final {
public:
    Int X;
    Int Y;
    Int Z;

    Int3() = default;
    explicit constexpr Int3( const int32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Int3( const int32_t & _X, const int32_t & _Y, const int32_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class UInt3 final {
public:
    UInt X;
    UInt Y;
    UInt Z;

    UInt3() = default;
    explicit constexpr UInt3( const uint32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr UInt3( const uint32_t & _X, const uint32_t & _Y, const uint32_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class Long3 final {
public:
    Long X;
    Long Y;
    Long Z;

    Long3() = default;
    explicit constexpr Long3( const int64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr Long3( const int64_t & _X, const int64_t & _Y, const int64_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class ULong3 final {
public:
    ULong X;
    ULong Y;
    ULong Z;

    ULong3() = default;
    explicit constexpr ULong3( const uint64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ) {}
    constexpr ULong3( const uint64_t & _X, const uint64_t & _Y, const uint64_t & _Z ) : X( _X ), Y( _Y ), Z( _Z ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
    }
};
class SignedByte4 final {
public:
    SignedByte X;
    SignedByte Y;
    SignedByte Z;
    SignedByte W;

    SignedByte4() = default;
    explicit constexpr SignedByte4( const int8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr SignedByte4( const int8_t & _X, const int8_t & _Y, const int8_t & _Z, const int8_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class Byte4 final {
public:
    Byte X;
    Byte Y;
    Byte Z;
    Byte W;

    Byte4() = default;
    explicit constexpr Byte4( const uint8_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Byte4( const uint8_t & _X, const uint8_t & _Y, const uint8_t & _Z, const uint8_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class Short4 final {
public:
    Short X;
    Short Y;
    Short Z;
    Short W;

    Short4() = default;
    explicit constexpr Short4( const int16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Short4( const int16_t & _X, const int16_t & _Y, const int16_t & _Z, const int16_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class UShort4 final {
public:
    UShort X;
    UShort Y;
    UShort Z;
    UShort W;

    UShort4() = default;
    explicit constexpr UShort4( const uint16_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr UShort4( const uint16_t & _X, const uint16_t & _Y, const uint16_t & _Z, const uint16_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class Int4 final {
public:
    Int X;
    Int Y;
    Int Z;
    Int W;

    Int4() = default;
    explicit constexpr Int4( const int32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Int4( const int32_t & _X, const int32_t & _Y, const int32_t & _Z, const int32_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class UInt4 final {
public:
    UInt X;
    UInt Y;
    UInt Z;
    UInt W;

    UInt4() = default;
    explicit constexpr UInt4( const uint32_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr UInt4( const uint32_t & _X, const uint32_t & _Y, const uint32_t & _Z, const uint32_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class Long4 final {
public:
    Long X;
    Long Y;
    Long Z;
    Long W;

    Long4() = default;
    explicit constexpr Long4( const int64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr Long4( const int64_t & _X, const int64_t & _Y, const int64_t & _Z, const int64_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};
class ULong4 final {
public:
    ULong X;
    ULong Y;
    ULong Z;
    ULong W;

    ULong4() = default;
    explicit constexpr ULong4( const uint64_t & _Value ) : X( _Value ), Y( _Value ), Z( _Value ), W( _Value ) {}
    constexpr ULong4( const uint64_t & _X, const uint64_t & _Y, const uint64_t & _Z, const uint64_t & _W ) : X( _X ), Y( _Y ), Z( _Z ), W( _W ) {}

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << X << Y << Z << W;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> X;
        _Stream >> Y;
        _Stream >> Z;
        _Stream >> W;
    }
};

#if SIZEOF_SIZE_T == 4
using FSize = UInt;
#else
using FSize = ULong;
#endif
