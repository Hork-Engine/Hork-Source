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

#include "BaseMath.h"
#include "Format.h"

extern float stbir__srgb_uchar_to_linear_float[256];

HK_NAMESPACE_BEGIN

HK_INLINE void EncodeRGBE(uint8_t RGBE[4], const float* LinearRGB)
{
    float maxcomp = Math::Max3(LinearRGB[0], LinearRGB[1], LinearRGB[2]);

    if (maxcomp < 1e-32f)
    {
        RGBE[0] = RGBE[1] = RGBE[2] = RGBE[3] = 0;
    }
    else
    {
        int   exponent;
        float normalize = frexp(maxcomp, &exponent) * 256.0f / maxcomp;

        RGBE[0] = (unsigned char)(LinearRGB[0] * normalize);
        RGBE[1] = (unsigned char)(LinearRGB[1] * normalize);
        RGBE[2] = (unsigned char)(LinearRGB[2] * normalize);
        RGBE[3] = (unsigned char)(exponent + 128);
    }
}

HK_INLINE void DecodeRGBE(float* LinearRGB, const uint8_t RGBE[4])
{
    if (RGBE[3] != 0)
    {
        float scale  = ldexp(1.0f, RGBE[3] - (int)(128 + 8));
        LinearRGB[0] = RGBE[0] * scale;
        LinearRGB[1] = RGBE[1] * scale;
        LinearRGB[2] = RGBE[2] * scale;
    }
    else
    {
        LinearRGB[0] = LinearRGB[1] = LinearRGB[2] = 0;
    }
}


struct Color4
{
    float R{1};
    float G{1};
    float B{1};
    float A{1};

    Color4() = default;
    constexpr explicit Color4(float _Value) :
        R(_Value), G(_Value), B(_Value), A(_Value) {}
    constexpr Color4(float InR, float InG, float InB) :
        R(InR), G(InG), B(InB), A(1.0f) {}
    constexpr Color4(float InR, float InG, float InB, float InA) :
        R(InR), G(InG), B(InB), A(InA) {}

    float* ToPtr()
    {
        return &R;
    }

    float const* ToPtr() const
    {
        return &R;
    }

    float& operator[](int _Index)
    {
        HK_ASSERT_(_Index >= 0 && _Index < NumComponents(), "Index out of range");
        return (&R)[_Index];
    }

    float const& operator[](int _Index) const
    {
        HK_ASSERT_(_Index >= 0 && _Index < NumComponents(), "Index out of range");
        return (&R)[_Index];
    }

    constexpr Color4 operator*(Color4 const& Rhs) const
    {
        return Color4(R * Rhs.R, G * Rhs.G, B * Rhs.B, A * Rhs.A);
    }
    constexpr Color4 operator/(Color4 const& Rhs) const
    {
        return Color4(R / Rhs.R, G / Rhs.G, B / Rhs.B, A / Rhs.A);
    }
    constexpr Color4 operator*(float Rhs) const
    {
        return Color4(R * Rhs, G * Rhs, B * Rhs, A * Rhs);
    }
    constexpr Color4 operator/(float Rhs) const
    {
        return (*this) * (1.0f / Rhs);
    }

    Color4& operator*=(Color4 const& Rhs)
    {
        R *= Rhs.R;
        G *= Rhs.G;
        B *= Rhs.B;
        A *= Rhs.A;
        return *this;
    }
    Color4& operator/=(Color4 const& Rhs)
    {
        R /= Rhs.R;
        G /= Rhs.G;
        B /= Rhs.B;
        A /= Rhs.A;
        return *this;
    }
    Color4& operator*=(float Rhs)
    {
        R *= Rhs;
        G *= Rhs;
        B *= Rhs;
        A *= Rhs;
        return *this;
    }
    Color4& operator/=(float Rhs)
    {
        float invRhs = 1.0f / Rhs;
        R *= invRhs;
        G *= invRhs;
        B *= invRhs;
        A *= invRhs;
        return *this;
    }

    void SwapRGB();

    void  SetAlpha(float _Alpha);
    float GetAlpha() const { return A; }

    bool IsTransparent() const { return A < 0.0001f; }

    /** Convert temperature in Kelvins to RGB color.
    Assume temperature is range between 1000 and 15000. */
    void SetTemperature(float _Temperature);

    void SetByte(uint8_t _Red, uint8_t _Green, uint8_t _Blue);
    void SetByte(uint8_t _Red, uint8_t _Green, uint8_t _Blue, uint8_t _Alpha);

    void GetByte(uint8_t& _Red, uint8_t& _Green, uint8_t& _Blue) const;
    void GetByte(uint8_t& _Red, uint8_t& _Green, uint8_t& _Blue, uint8_t& _Alpha) const;

    void     SetDWord(uint32_t _Color);
    uint32_t GetDWord() const;

    void           SetUShort565(unsigned short _565);
    unsigned short GetUShort565() const;

    void SetYCoCgAlpha(const uint8_t _YCoCgAlpha[4]);
    void GetYCoCgAlpha(uint8_t _YCoCgAlpha[4]) const;

    void SetYCoCg(const uint8_t _YCoCg[3]);
    void GetYCoCg(uint8_t _YCoCg[3]) const;

    void SetCoCg_Y(const uint8_t _CoCg_Y[4]);
    void GetCoCg_Y(uint8_t _CoCg_Y[4]) const;

    void SetHSL(float _Hue, float _Saturation, float _Lightness);
    void GetHSL(float& _Hue, float& _Saturation, float& _Lightness) const;

    void SetCMYK(float _Cyan, float _Magenta, float _Yellow, float _Key);
    void GetCMYK(float& _Cyan, float& _Magenta, float& _Yellow, float& _Key) const;

    /** Assume color is in linear space */
    float GetLuminance() const;

    Color4 ToLinear() const;
    Color4 ToSRGB() const;

    /** Assume color is in linear space */
    void SetRGBE(uint32_t RGBE)
    {
        DecodeRGBE(ToPtr(), (uint8_t const*)&RGBE);
    }

    /** Assume color is in linear space */
    uint32_t GetRGBE() const
    {
        uint32_t rgbe;
        EncodeRGBE((uint8_t*)&rgbe, ToPtr());
        return rgbe;
    }

    static constexpr Color4 White()
    {
        return Color4(1.0f);
    }

    static constexpr Color4 Black()
    {
        return Color4(0.0f, 0.0f, 0.0f);
    }

    static constexpr Color4 Red()
    {
        return Color4(1.0f, 0.0f, 0.0f);
    }

    static constexpr Color4 Green()
    {
        return Color4(0.0f, 1.0f, 0.0f);
    }

    static constexpr Color4 Blue()
    {
        return Color4(0.0f, 0.0f, 1.0f);
    }

    static constexpr Color4 Orange()
    {
        return Color4(1.0f, 0.456f, 0.1f);
    }


    static constexpr int NumComponents() { return 4; }
};

HK_FORCEINLINE float LinearFromSRGB(float Color)
{
    if (Color < 0.0f) return 0.0f;
    if (Color > 1.0f) return 1.0f;
    return (Color <= 0.04045f) ? Color / 12.92f : Math::Pow((Color + 0.055f) / 1.055f, 2.4f);
}

HK_FORCEINLINE float LinearToSRGB(float LinearColor)
{
    if (LinearColor < 0.0f) return 0.0f;
    if (LinearColor > 1.0f) return 1.0f;
    return (LinearColor <= 0.0031308f) ? LinearColor * 12.92f : Math::Pow(LinearColor, 1.0f / 2.4f) * 1.055f - 0.055f;
}

HK_FORCEINLINE float LinearFromSRGB_UChar(uint8_t in)
{
    // Uses true SRGB conversion
    return stbir__srgb_uchar_to_linear_float[in];
}

extern const uint32_t FP32ToSRGB8[104];

HK_FORCEINLINE uint8_t LinearToSRGB_UChar(float in)
{
    // From https://gist.github.com/rygorous/2203834
    // Assume float is in IEEE format

    typedef union
    {
        uint32_t u;
        float    f;
    } FP32;

    static const FP32 almostone = {0x3f7fffff}; // 1-eps
    static const FP32 minval    = {(127 - 13) << 23};
    uint32_t          tab, bias, scale, t;
    FP32              f;

    // Clamp to [2^(-13), 1-eps]; these two values map to 0 and 1, respectively.
    // The tests are carefully written so that NaNs map to 0, same as in the reference
    // implementation.
    if (!(in > minval.f)) // written this way to catch NaNs
        in = minval.f;
    if (in > almostone.f)
        in = almostone.f;

    // Do the table lookup and unpack bias, scale
    f.f   = in;
    tab   = FP32ToSRGB8[(f.u - minval.u) >> 20];
    bias  = (tab >> 16) << 9;
    scale = tab & 0xffff;

    // Grab next-highest mantissa bits and perform linear interpolation
    t = (f.u >> 12) & 0xff;
    return (unsigned char)((bias + scale * t) >> 16);
}

HK_FORCEINLINE Color4 Color4::ToLinear() const
{
    return Color4(LinearFromSRGB(R), LinearFromSRGB(G), LinearFromSRGB(B), A);
}

HK_FORCEINLINE Color4 Color4::ToSRGB() const
{
    return Color4(LinearToSRGB(R), LinearToSRGB(G), LinearToSRGB(B), A);
}

HK_FORCEINLINE void Color4::SwapRGB()
{
    std::swap(R, B);
}

HK_FORCEINLINE void Color4::SetAlpha(float _Alpha)
{
    A = Math::Saturate(_Alpha);
}

HK_FORCEINLINE void Color4::SetTemperature(float _Temperature)
{
#if 1
    // Approximate Planckian locus in CIE 1960 UCS
    // Urho3D and UE4 uses same formulas

    const float temperature = Math::Clamp(_Temperature, 1000.0f, 15000.0f);

    const float u = (0.860117757f + 1.54118254e-4f * temperature + 1.28641212e-7f * temperature * temperature) /
        (1.0f + 8.42420235e-4f * temperature + 7.08145163e-7f * temperature * temperature);
    const float v = (0.317398726f + 4.22806245e-5f * temperature + 4.20481691e-8f * temperature * temperature) /
        (1.0f - 2.89741816e-5f * temperature + 1.61456053e-7f * temperature * temperature);

    const float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
    const float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
    const float z = 1.0f - x - y;

    const float x_ = 1.0f / y * x;
    const float z_ = 1.0f / y * z;

    R = Math::Saturate(3.2404542f * x_ + -1.5371385f + -0.4985314f * z_);
    G = Math::Saturate(-0.9692660f * x_ + 1.8760108f + 0.0415560f * z_);
    B = Math::Saturate(0.0556434f * x_ + -0.2040259f + 1.0572252f * z_);
#endif

#if 0
    // Based on code by Benjamin 'BeRo' Rosseaux
    _Temperature = Math::Clamp( _Temperature, 1000.0f, 15000.0f );

    if ( _Temperature <= 6500.0f ) {
        R = 1.0f;
        G = -2902.1955373783176f / (1669.5803561666639f + _Temperature) + 1.3302673723350029f;
        B = -8257.7997278925690f / (2575.2827530017594f + _Temperature) + 1.8993753891711275f;
        B = Math::Max( 0.0f, B );
    } else {
        R = 1745.0425298314172f / (-2666.3474220535695f + _Temperature) + 0.55995389139931482f;
        G = 1216.6168361476490f / (-2173.1012343082230f + _Temperature) + 0.70381203140554553f;
        B = -8257.7997278925690f / (2575.2827530017594f + _Temperature) + 1.8993753891711275f;
        R = Math::Min( 1.0f, R );
        B = Math::Min( 1.0f, B );
    }
#endif

#if 0
    // Based on code from http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
    _Temperature = Math::Clamp( _Temperature, 1000.0f, 15000.0f ) * 0.01f;

    float Value;

    // Calculate each color in turn

    if ( _Temperature <= 66 ) {
        R = 1.0f;

        // Note: the R-squared value for this approximation is .996
        Value = (99.4708025861/255.0) * log( _Temperature ) - (161.1195681661/255.0);
        G = Math::Min( 1.0f, Value );//Math::Clamp( Value, 0.0f, 1.0f );

    } else {
        // Note: the R-squared value for this approximation is .988
        Value = (329.698727446 / 255.0) * Math::Pow( _Temperature - 60, -0.1332047592 );
        R = Math::Min( 1.0f, Value );//Math::Clamp( Value, 0.0f, 1.0f );

                                            // Note: the R-squared value for this approximation is .987
        Value = (288.1221695283/255.0) * Math::Pow( _Temperature - 60, -0.0755148492 );
        G = Value;//Math::Clamp( Value, 0.0f, 1.0f );
    }

    if ( _Temperature >= 66 ) {
        B = 1.0f;
    } else if ( _Temperature <= 19 ) {
        B = 0.0f;
    } else {
        // Note: the R-squared value for this approximation is .998
        Value = (138.5177312231/255.0) * log( _Temperature - 10 ) - (305.0447927307/255.0);
        B = Math::Max( 0.0f, Value );//Math::Clamp( Value, 0.0f, 1.0f );
    }
#endif
}

HK_FORCEINLINE void Color4::SetByte(uint8_t _Red, uint8_t _Green, uint8_t _Blue)
{
    const float scale = 1.0f / 255.0f;
    R                 = _Red * scale;
    G                 = _Green * scale;
    B                 = _Blue * scale;
}

HK_FORCEINLINE void Color4::SetByte(uint8_t _Red, uint8_t _Green, uint8_t _Blue, uint8_t _Alpha)
{
    const float scale = 1.0f / 255.0f;
    R                 = _Red * scale;
    G                 = _Green * scale;
    B                 = _Blue * scale;
    A                 = _Alpha * scale;
}

HK_FORCEINLINE void Color4::GetByte(uint8_t& _Red, uint8_t& _Green, uint8_t& _Blue) const
{
    _Red   = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    _Green = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    _Blue  = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);
}

HK_FORCEINLINE void Color4::GetByte(uint8_t& _Red, uint8_t& _Green, uint8_t& _Blue, uint8_t& _Alpha) const
{
    _Red   = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    _Green = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    _Blue  = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);
    _Alpha = Math::Clamp(Math::ToIntFast(A * 255), 0, 255);
}

HK_FORCEINLINE void Color4::SetDWord(uint32_t _Color)
{
    const int r = _Color & 0xff;
    const int g = (_Color >> 8) & 0xff;
    const int b = (_Color >> 16) & 0xff;
    const int a = (_Color >> 24);

    const float scale = 1.0f / 255.0f;
    R                 = r * scale;
    G                 = g * scale;
    B                 = b * scale;
    A                 = a * scale;
}

HK_FORCEINLINE uint32_t Color4::GetDWord() const
{
    const int r = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    const int g = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    const int b = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);
    const int a = Math::Clamp(Math::ToIntFast(A * 255), 0, 255);

    return r | (g << 8) | (b << 16) | (a << 24);
}

HK_FORCEINLINE void Color4::SetUShort565(unsigned short _565)
{
    const float scale = 1.0f / 255.0f;
    const int   r     = ((_565 >> 8) & (((1 << (8 - 3)) - 1) << 3)) | ((_565 >> 13) & ((1 << 3) - 1));
    const int   g     = ((_565 >> 3) & (((1 << (8 - 2)) - 1) << 2)) | ((_565 >> 9) & ((1 << 2) - 1));
    const int   b     = ((_565 << 3) & (((1 << (8 - 3)) - 1) << 3)) | ((_565 >> 2) & ((1 << 3) - 1));
    R                 = r * scale;
    G                 = g * scale;
    B                 = b * scale;
}

HK_FORCEINLINE unsigned short Color4::GetUShort565() const
{
    const int r = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    const int g = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    const int b = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);

    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

HK_FORCEINLINE void Color4::SetYCoCgAlpha(const uint8_t _YCoCgAlpha[4])
{
    const float scale = 1.0f / 255.0f;
    const int   y     = _YCoCgAlpha[0];
    const int   co    = _YCoCgAlpha[1] - 128;
    const int   cg    = _YCoCgAlpha[2] - 128;
    R                 = Math::Clamp(y + (co - cg), 0, 255) * scale;
    G                 = Math::Clamp(y + (cg), 0, 255) * scale;
    B                 = Math::Clamp(y + (-co - cg), 0, 255) * scale;
    A                 = _YCoCgAlpha[3] * scale;
}

HK_FORCEINLINE void Color4::GetYCoCgAlpha(uint8_t _YCoCgAlpha[4]) const
{
    const int r = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    const int g = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    const int b = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);
    const int a = Math::Clamp(Math::ToIntFast(A * 255), 0, 255);

    _YCoCgAlpha[0] = Math::Clamp(((r + (g << 1) + b) + 2) >> 2, 0, 255);
    _YCoCgAlpha[1] = Math::Clamp(((((r << 1) - (b << 1)) + 2) >> 2) + 128, 0, 255);
    _YCoCgAlpha[2] = Math::Clamp((((-r + (g << 1) - b) + 2) >> 2) + 128, 0, 255);
    _YCoCgAlpha[3] = a;
}

HK_FORCEINLINE void Color4::SetYCoCg(const uint8_t _YCoCg[3])
{
    const float scale = 1.0f / 255.0f;
    const int   y     = _YCoCg[0];
    const int   co    = _YCoCg[1] - 128;
    const int   cg    = _YCoCg[2] - 128;
    R                 = Math::Clamp(y + (co - cg), 0, 255) * scale;
    G                 = Math::Clamp(y + (cg), 0, 255) * scale;
    B                 = Math::Clamp(y + (-co - cg), 0, 255) * scale;
}

HK_FORCEINLINE void Color4::GetYCoCg(uint8_t _YCoCg[3]) const
{
    const int r = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    const int g = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    const int b = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);
    _YCoCg[0]   = Math::Clamp(((r + (g << 1) + b) + 2) >> 2, 0, 255);
    _YCoCg[1]   = Math::Clamp(((((r << 1) - (b << 1)) + 2) >> 2) + 128, 0, 255);
    _YCoCg[2]   = Math::Clamp((((-r + (g << 1) - b) + 2) >> 2) + 128, 0, 255);
}

HK_FORCEINLINE void Color4::SetCoCg_Y(const uint8_t _CoCg_Y[4])
{
    const float scale = 1.0f / 255.0f;
    const int   y     = _CoCg_Y[3];
    const int   co    = _CoCg_Y[0] - 128;
    const int   cg    = _CoCg_Y[1] - 128;
    R                 = Math::Clamp(y + (co - cg), 0, 255) * scale;
    G                 = Math::Clamp(y + (cg), 0, 255) * scale;
    B                 = Math::Clamp(y + (-co - cg), 0, 255) * scale;
}

HK_FORCEINLINE void Color4::GetCoCg_Y(uint8_t _CoCg_Y[4]) const
{
    const int r = Math::Clamp(Math::ToIntFast(R * 255), 0, 255);
    const int g = Math::Clamp(Math::ToIntFast(G * 255), 0, 255);
    const int b = Math::Clamp(Math::ToIntFast(B * 255), 0, 255);

    _CoCg_Y[0] = Math::Clamp(((((r << 1) - (b << 1)) + 2) >> 2) + 128, 0, 255);
    _CoCg_Y[1] = Math::Clamp((((-r + (g << 1) - b) + 2) >> 2) + 128, 0, 255);
    _CoCg_Y[2] = 0;
    _CoCg_Y[3] = Math::Clamp(((r + (g << 1) + b) + 2) >> 2, 0, 255);
}

HK_FORCEINLINE void Color4::SetHSL(float _Hue, float _Saturation, float _Lightness)
{
    _Hue        = Math::Saturate(_Hue);
    _Saturation = Math::Saturate(_Saturation);
    _Lightness  = Math::Saturate(_Lightness);

    const float max = _Lightness;
    const float min = (1.0f - _Saturation) * _Lightness;

    const float f = max - min;

    if (_Hue >= 0.0f && _Hue <= 1.0f / 6.0f)
    {
        R = max;
        G = Math::Saturate(min + _Hue * f * 6.0f);
        B = min;
        return;
    }

    if (_Hue <= 1.0f / 3.0f)
    {
        R = Math::Saturate(max - (_Hue - 1.0f / 6.0f) * f * 6.0f);
        G = max;
        B = min;
        return;
    }

    if (_Hue <= 0.5f)
    {
        R = min;
        G = max;
        B = Math::Saturate(min + (_Hue - 1.0f / 3.0f) * f * 6.0f);
        return;
    }

    if (_Hue <= 2.0f / 3.0f)
    {
        R = min;
        G = Math::Saturate(max - (_Hue - 0.5f) * f * 6.0f);
        B = max;
        return;
    }

    if (_Hue <= 5.0f / 6.0f)
    {
        R = Math::Saturate(min + (_Hue - 2.0f / 3.0f) * f * 6.0f);
        G = min;
        B = max;
        return;
    }

    if (_Hue <= 1.0f)
    {
        R = max;
        G = min;
        B = Math::Saturate(max - (_Hue - 5.0f / 6.0f) * f * 6.0f);
        return;
    }

    R = G = B = 0.0f;
}

HK_FORCEINLINE void Color4::GetHSL(float& _Hue, float& _Saturation, float& _Lightness) const
{
    float maxComponent, minComponent;

    const float r = Math::Saturate(R) * 255.0f;
    const float g = Math::Saturate(G) * 255.0f;
    const float b = Math::Saturate(B) * 255.0f;

    Math::MinMax(r, g, b, minComponent, maxComponent);

    const float dist = maxComponent - minComponent;

    const float f = (dist == 0.0f) ? 0.0f : 60.0f / dist;

    if (maxComponent == r)
    {
        // R 360
        if (g < b)
        {
            _Hue = (360.0f + f * (g - b)) / 360.0f;
        }
        else
        {
            _Hue = (f * (g - b)) / 360.0f;
        }
    }
    else if (maxComponent == g)
    {
        // G 120 degrees
        _Hue = (120.0f + f * (b - r)) / 360.0f;
    }
    else if (maxComponent == b)
    {
        // B 240 degrees
        _Hue = (240.0f + f * (r - g)) / 360.0f;
    }
    else
    {
        _Hue = 0.0f;
    }

    _Hue = Math::Saturate(_Hue);

    _Saturation = maxComponent == 0.0f ? 0.0f : dist / maxComponent;
    _Lightness  = maxComponent / 255.0f;
}

HK_FORCEINLINE void Color4::SetCMYK(float _Cyan, float _Magenta, float _Yellow, float _Key)
{
    const float scale = 1.0f - Math::Saturate(_Key);
    R                 = (1.0f - Math::Saturate(_Cyan)) * scale;
    G                 = (1.0f - Math::Saturate(_Magenta)) * scale;
    B                 = (1.0f - Math::Saturate(_Yellow)) * scale;
}

HK_FORCEINLINE void Color4::GetCMYK(float& _Cyan, float& _Magenta, float& _Yellow, float& _Key) const
{
    const float r            = Math::Saturate(R);
    const float g            = Math::Saturate(G);
    const float b            = Math::Saturate(B);
    const float maxComponent = Math::Max3(r, g, b);
    const float scale        = maxComponent > 0.0f ? 1.0f / maxComponent : 0.0f;

    _Cyan    = (maxComponent - r) * scale;
    _Magenta = (maxComponent - g) * scale;
    _Yellow  = (maxComponent - b) * scale;
    _Key     = 1.0f - maxComponent;
}

HK_FORCEINLINE float Color4::GetLuminance() const
{
    return R * 0.2126f + G * 0.7152f + B * 0.0722f;
}

HK_INLINE Color4 MakeColorU8(uint8_t _Red, uint8_t _Green, uint8_t _Blue, uint8_t _Alpha)
{
    const float scale = 1.0f / 255.0f;
    return Color4(_Red * scale, _Green * scale, _Blue * scale, _Alpha * scale);
}

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::Color4, "( {} {} {} {} )", v.R, v.G, v.B, v.A);
