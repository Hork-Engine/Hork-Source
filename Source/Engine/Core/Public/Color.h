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

#include "Float.h"

struct AColor4 : Float4 {
    AColor4() = default;
    explicit constexpr AColor4( const float & _Value ) : Float4( _Value ) {}
    constexpr AColor4( const float & _X, const float & _Y, const float & _Z ) : Float4( _X, _Y, _Z, 1.0f ) {}
    constexpr AColor4( const float & _X, const float & _Y, const float & _Z, const float & _W ) : Float4( _X, _Y, _Z, _W ) {}
    AColor4( Float4 const & _Value ) : Float4( _Value ) {}
    AColor4( Float3 const & _Value ) : Float4( _Value.X, _Value.Y, _Value.Z, 1.0f ) {}

    void SwapRGB();

    void SetAlpha( float _Alpha );
    float GetAlpha() const { return W; }

    bool IsTransparent() const { return W < 0.0001f; }

    /** Assume temperature is range between 1000 and 40000. */
    void SetTemperature( float _Temperature );

    void SetByte( byte _Red, byte _Green, byte _Blue );
    void SetByte( byte _Red, byte _Green, byte _Blue, byte _Alpha );

    void GetByte( byte & _Red, byte & _Green, byte & _Blue ) const;
    void GetByte( byte & _Red, byte & _Green, byte & _Blue, byte & _Alpha ) const;

    void SetDWord( uint32_t _Color );
    uint32_t GetDWord() const;

    void SetUShort565( unsigned short _565 );
    unsigned short GetUShort565() const;

    void SetYCoCgAlpha( const byte _YCoCgAlpha[4] );
    void GetYCoCgAlpha( byte _YCoCgAlpha[4] ) const;

    void SetYCoCg( const byte _YCoCg[3] );
    void GetYCoCg( byte _YCoCg[3] ) const;

    void SetCoCg_Y( const byte _CoCg_Y[4] );
    void GetCoCg_Y( byte _CoCg_Y[4] ) const;

    void SetHSL( float _Hue, float _Saturation, float _Lightness );
    void GetHSL( float & _Hue, float & _Saturation, float & _Lightness ) const;

    void SetCMYK( float _Cyan, float _Magenta, float _Yellow, float _Key );
    void GetCMYK( float & _Cyan, float & _Magenta, float & _Yellow, float & _Key ) const;

    /** Assume color is in linear space */
    float GetLuminance() const;

    AColor4 ToLinear() const;
    AColor4 ToSRGB() const;

    Float3 & GetRGB() { return *(Float3 *)(this); }
    Float3 const & GetRGB() const { return *(Float3 *)(this); }

    static AColor4 const & White() {
        static AColor4 color(1.0f);
        return color;
    }

    static AColor4 const & Black() {
        static AColor4 color(0.0f,0.0f,0.0f,1.0f);
        return color;
    }

    static AColor4 const & Red() {
        static AColor4 color(1.0f,0.0f,0.0f,1.0f);
        return color;
    }

    static AColor4 const & Green() {
        static AColor4 color(0.0f,1.0f,0.0f,1.0f);
        return color;
    }

    static AColor4 const & Blue() {
        static AColor4 color(0.0f,0.0f,1.0f,1.0f);
        return color;
    }
};

//#define SRGB_GAMMA_APPROX

AN_FORCEINLINE float LinearFromSRGB( const float & _sRGB ) {
#ifdef SRGB_GAMMA_APPROX
    return StdPow( _sRGB, 2.2f );
#else
    if ( _sRGB < 0.0f ) return 0.0f;
    if ( _sRGB > 1.0f ) return 1.0f;
    if ( _sRGB <= 0.04045 ) return _sRGB / 12.92f;
    return StdPow( ( _sRGB + 0.055f ) / 1.055f, 2.4f );
#endif
}

AN_FORCEINLINE float LinearToSRGB( const float & _lRGB ) {
#ifdef SRGB_GAMMA_APPROX
    return StdPow( _lRGB, 1.0f / 2.2f );
#else
    if ( _lRGB < 0.0f ) return 0.0f;
    if ( _lRGB > 1.0f ) return 1.0f;
    if ( _lRGB <= 0.0031308 ) return _lRGB * 12.92f;
    return 1.055f * StdPow( _lRGB, 1.0f / 2.4f ) - 0.055f;
#endif
}

AN_FORCEINLINE AColor4 AColor4::ToLinear() const {
    return AColor4( LinearFromSRGB( X ), LinearFromSRGB( Y ), LinearFromSRGB( Z ), W );
}

AN_FORCEINLINE AColor4 AColor4::ToSRGB() const {
    return AColor4( LinearToSRGB( X ), LinearToSRGB( Y ), LinearToSRGB( Z ), W );
}

AN_FORCEINLINE void AColor4::SwapRGB() {
    StdSwap( X, Z );
}

AN_FORCEINLINE void AColor4::SetAlpha( float _Alpha ) {
    W = Math::Saturate( _Alpha );
}

AN_FORCEINLINE void AColor4::SetTemperature( float _Temperature ) {
     if ( _Temperature <= 6500.0f ) {
         X = 1.0f;
         Y = -2902.1955373783176f / ( 1669.5803561666639f + _Temperature ) + 1.3302673723350029f;
         Z = -8257.7997278925690f / ( 2575.2827530017594f + _Temperature ) + 1.8993753891711275f;
         Z = Math::Max( 0.0f, Z );
     } else {
         X = 1745.0425298314172f / ( -2666.3474220535695f + _Temperature ) + 0.55995389139931482f;
         Y = 1216.6168361476490f / ( -2173.1012343082230f + _Temperature ) + 0.70381203140554553f;
         Z = -8257.7997278925690f / ( 2575.2827530017594f + _Temperature ) + 1.8993753891711275f;
         X = Math::Min( 1.0f, X );
         Z = Math::Min( 1.0f, Z );
     }
 }

AN_FORCEINLINE void AColor4::SetByte( byte _Red, byte _Green, byte _Blue ) {
    constexpr float scale = 1.0f / 255.0f;
    X = _Red * scale;
    Y = _Green * scale;
    Z = _Blue * scale;
}

AN_FORCEINLINE void AColor4::SetByte( byte _Red, byte _Green, byte _Blue, byte _Alpha ) {
    constexpr float scale = 1.0f / 255.0f;
    X = _Red * scale;
    Y = _Green * scale;
    Z = _Blue * scale;
    W = _Alpha * scale;
}

AN_FORCEINLINE void AColor4::GetByte( byte & _Red, byte & _Green, byte & _Blue ) const {
    _Red   = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    _Green = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    _Blue  = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );
}

AN_FORCEINLINE void AColor4::GetByte( byte & _Red, byte & _Green, byte & _Blue, byte & _Alpha ) const {
    _Red   = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    _Green = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    _Blue  = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );
    _Alpha = Math::Clamp( Math::ToIntFast( W * 255 ), 0, 255 );
}

AN_FORCEINLINE void AColor4::SetDWord( uint32_t _Color ) {
    const int r = _Color & 0xff;
    const int g = ( _Color >> 8 ) & 0xff;
    const int b = ( _Color >> 16 ) & 0xff;
    const int a = ( _Color >> 24 );

    constexpr float scale = 1.0f / 255.0f;
    X = r * scale;
    Y = g * scale;
    Z = b * scale;
    W = a * scale;
}

AN_FORCEINLINE uint32_t AColor4::GetDWord() const {
    const int r = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    const int g = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    const int b = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );
    const int a = Math::Clamp( Math::ToIntFast( W * 255 ), 0, 255 );

    return r | ( g << 8 ) | ( b << 16 ) | ( a << 24 );
}

AN_FORCEINLINE void AColor4::SetUShort565( unsigned short _565 ) {
    constexpr float scale = 1.0f / 255.0f;
    const int r = ( ( _565 >> 8 ) & ( ( ( 1 << ( 8 - 3 ) ) - 1 ) << 3 ) ) | ( ( _565 >> 13 ) & ((1<<3)-1) );
    const int g = ( ( _565 >> 3 ) & ( ( ( 1 << ( 8 - 2 ) ) - 1 ) << 2 ) ) | ( ( _565 >>  9 ) & ((1<<2)-1) );
    const int b = ( ( _565 << 3 ) & ( ( ( 1 << ( 8 - 3 ) ) - 1 ) << 3 ) ) | ( ( _565 >>  2 ) & ((1<<3)-1) );
    X = r * scale;
    Y = g * scale;
    Z = b * scale;
}

AN_FORCEINLINE unsigned short AColor4::GetUShort565() const {
    const int r = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    const int g = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    const int b = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );

    return ( ( r >> 3 ) << 11 ) | ( ( g >> 2 ) << 5 ) | ( b >> 3 );
}

AN_FORCEINLINE void AColor4::SetYCoCgAlpha( const byte _YCoCgAlpha[4] ) {
    constexpr float scale = 1.0f / 255.0f;
    const int y  = _YCoCgAlpha[0];
    const int co = _YCoCgAlpha[1] - 128;
    const int cg = _YCoCgAlpha[2] - 128;
    X = Math::Clamp( y + ( co - cg ), 0, 255 ) * scale;
    Y = Math::Clamp( y + ( cg ), 0, 255 ) * scale;
    Z = Math::Clamp( y + ( - co - cg ), 0, 255 ) * scale;
    W = _YCoCgAlpha[3] * scale;
}

AN_FORCEINLINE void AColor4::GetYCoCgAlpha( byte _YCoCgAlpha[4] ) const {
    const int r = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    const int g = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    const int b = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );
    const int a = Math::Clamp( Math::ToIntFast( W * 255 ), 0, 255 );

    _YCoCgAlpha[0] = Math::Clamp( ( ( r + ( g << 1 ) +  b ) + 2 ) >> 2, 0, 255 );
    _YCoCgAlpha[1] = Math::Clamp( ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 ) + 128, 0, 255 );
    _YCoCgAlpha[2] = Math::Clamp( ( ( ( -r + ( g << 1 ) - b ) + 2 ) >> 2 ) + 128, 0, 255 );
    _YCoCgAlpha[3] = a;
}

AN_FORCEINLINE void AColor4::SetYCoCg( const byte _YCoCg[3] ) {
    constexpr float scale = 1.0f / 255.0f;
    const int y  = _YCoCg[0];
    const int co = _YCoCg[1] - 128;
    const int cg = _YCoCg[2] - 128;
    X = Math::Clamp( y + ( co - cg ), 0, 255 ) * scale;
    Y = Math::Clamp( y + ( cg ), 0, 255 ) * scale;
    Z = Math::Clamp( y + ( - co - cg ), 0, 255 ) * scale;
}

AN_FORCEINLINE void AColor4::GetYCoCg( byte _YCoCg[3] ) const {
    const int r = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    const int g = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    const int b = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );
    _YCoCg[0] = Math::Clamp( ( ( r + ( g << 1 ) +  b ) + 2 ) >> 2, 0, 255 );
    _YCoCg[1] = Math::Clamp( ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 ) + 128, 0, 255 );
    _YCoCg[2] = Math::Clamp( ( ( ( -r + ( g << 1 ) - b ) + 2 ) >> 2 ) + 128, 0, 255 );
}

AN_FORCEINLINE void AColor4::SetCoCg_Y( const byte _CoCg_Y[4] ) {
    constexpr float scale = 1.0f / 255.0f;
    const int y  = _CoCg_Y[3];
    const int co = _CoCg_Y[0] - 128;
    const int cg = _CoCg_Y[1] - 128;
    X = Math::Clamp( y + ( co - cg ), 0, 255 ) * scale;
    Y = Math::Clamp( y + ( cg ), 0, 255 ) * scale;
    Z = Math::Clamp( y + ( - co - cg ), 0, 255 ) * scale;
}

AN_FORCEINLINE void AColor4::GetCoCg_Y( byte _CoCg_Y[4] ) const {
    const int r = Math::Clamp( Math::ToIntFast( X * 255 ), 0, 255 );
    const int g = Math::Clamp( Math::ToIntFast( Y * 255 ), 0, 255 );
    const int b = Math::Clamp( Math::ToIntFast( Z * 255 ), 0, 255 );

    _CoCg_Y[0] = Math::Clamp( ( ( ( ( r << 1 ) - ( b << 1 ) ) + 2 ) >> 2 ) + 128, 0, 255 );
    _CoCg_Y[1] = Math::Clamp( ( ( ( -r + ( g << 1) - b ) + 2 ) >> 2 ) + 128, 0, 255 );
    _CoCg_Y[2] = 0;
    _CoCg_Y[3] = Math::Clamp( ( ( r + ( g << 1 ) +  b ) + 2 ) >> 2, 0, 255 );
}

AN_FORCEINLINE void AColor4::SetHSL( float _Hue, float _Saturation, float _Lightness ) {
    _Hue = Math::Saturate( _Hue );
    _Saturation = Math::Saturate( _Saturation );
    _Lightness = Math::Saturate( _Lightness );

    const float max = _Lightness;
    const float min = ( 1.0f - _Saturation ) * _Lightness;

    const float f = max - min;

    if ( _Hue >= 0.0f && _Hue <= 1.0f / 6.0f ) {
        X = max;
        Y = Math::Saturate( min + _Hue * f * 6.0f );
        Z = min;
        return;
    }

    if ( _Hue <= 1.0f / 3.0f ) {
        X = Math::Saturate( max - ( _Hue - 1.0f / 6.0f ) * f * 6.0f );
        Y = max;
        Z = min;
        return;
    }

    if ( _Hue <= 0.5f ) {
        X = min;
        Y = max;
        Z = Math::Saturate( min + ( _Hue - 1.0f / 3.0f ) * f * 6.0f );
        return;
    }

    if ( _Hue <= 2.0f / 3.0f ) {
        X = min;
        Y = Math::Saturate( max - ( _Hue - 0.5f ) * f * 6.0f );
        Z = max;
        return;
    }

    if ( _Hue <= 5.0f / 6.0f ) {
        X = Math::Saturate( min + ( _Hue - 2.0f / 3.0f ) * f * 6.0f );
        Y = min;
        Z = max;
        return;
    }

    if ( _Hue <= 1.0f ) {
        X = max;
        Y = min;
        Z = Math::Saturate( max - ( _Hue - 5.0f / 6.0f ) * f * 6.0f );
        return;
    }

    X = Y = Z = 0.0f;
}

AN_FORCEINLINE void AColor4::GetHSL( float & _Hue, float & _Saturation, float & _Lightness ) const {
    float maxComponent, minComponent;

    const float r = Math::Saturate(X) * 255.0f;
    const float g = Math::Saturate(Y) * 255.0f;
    const float b = Math::Saturate(Z) * 255.0f;

    Math::MinMax( r, g, b, minComponent, maxComponent );

    const float dist = maxComponent - minComponent;

    const float f = ( dist == 0.0f ) ? 0.0f : 60.0f / dist;

    if ( maxComponent == r ) {
        // R 360
        if ( g < b ) {
            _Hue = ( 360.0f + f * ( g - b ) ) / 360.0f;
        } else {
            _Hue = (          f * ( g - b ) ) / 360.0f;
        }
    } else if ( maxComponent == g ) {
        // G 120 degrees
        _Hue = ( 120.0f + f * ( b - r ) ) / 360.0f;
    } else if ( maxComponent == b ) {
        // B 240 degrees
        _Hue = ( 240.0f + f * ( r - g ) ) / 360.0f;
    } else {
        _Hue = 0.0f;
    }

    _Hue = Math::Saturate( _Hue );

    _Saturation = maxComponent == 0.0f ? 0.0f : dist / maxComponent;
    _Lightness = maxComponent / 255.0f;
}

AN_FORCEINLINE void AColor4::SetCMYK( float _Cyan, float _Magenta, float _Yellow, float _Key ) {
    const float scale = 1.0f - Math::Saturate( _Key );
    X = ( 1.0f - Math::Saturate( _Cyan ) ) * scale;
    Y = ( 1.0f - Math::Saturate( _Magenta ) ) * scale;
    Z = ( 1.0f - Math::Saturate( _Yellow ) ) * scale;
}

AN_FORCEINLINE void AColor4::GetCMYK( float & _Cyan, float & _Magenta, float & _Yellow, float & _Key ) const {
    const float r = Math::Saturate( X );
    const float g = Math::Saturate( Y );
    const float b = Math::Saturate( Z );
    const float maxComponent = Math::Max( r, g, b );
    const float scale = maxComponent > 0.0f ? 1.0f / maxComponent : 0.0f;

    _Cyan    = ( maxComponent - r ) * scale;
    _Magenta = ( maxComponent - g ) * scale;
    _Yellow  = ( maxComponent - b ) * scale;
    _Key     = 1.0f - maxComponent;
}

AN_FORCEINLINE float AColor4::GetLuminance() const {
    return X * 0.2126f + Y * 0.7152f + Z * 0.0722f;
}
