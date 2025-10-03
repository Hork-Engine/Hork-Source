/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

struct Color3
{
    float               R{1};
    float               G{1};
    float               B{1};

                        Color3() = default;
    constexpr explicit  Color3(float value) : R(value), G(value), B(value) {}
    constexpr           Color3(float r, float g, float b) : R(r), G(g), B(b) {}

    float*              ToPtr() { return &R; }
    float const*        ToPtr() const { return &R; }

    float&              operator[](int index) { HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range"); return (&R)[index]; }
    float const&        operator[](int index) const { HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range"); return (&R)[index]; }

    constexpr Color3    operator*(Color3 const& rhs) const { return Color3(R * rhs.R, G * rhs.G, B * rhs.B); }
    constexpr Color3    operator/(Color3 const& rhs) const { return Color3(R / rhs.R, G / rhs.G, B / rhs.B); }
    constexpr Color3    operator*(float rhs) const { return Color3(R * rhs, G * rhs, B * rhs); }
    constexpr Color3    operator/(float rhs) const { return (*this) * (1.0f / rhs); }

    Color3&             operator*=(Color3 const& rhs);
    Color3&             operator/=(Color3 const& rhs);
    Color3&             operator*=(float rhs);
    Color3&             operator/=(float rhs);

    void                SwapRB();

    /// Convert temperature in Kelvins to RGB color.
    /// Assume temperature is range between 1000 and 15000.
    void                SetTemperature(float temperature);

    void                SetByte(uint8_t r, uint8_t g, uint8_t b);
    void                GetByte(uint8_t& r, uint8_t& g, uint8_t& b) const;

    void                SetDWord(uint32_t color);
    uint32_t            GetDWord() const;

    void                SetUShort565(uint16_t _565);
    uint16_t            GetUShort565() const;

    void                SetYCoCg(const uint8_t YCoCg[3]);
    void                GetYCoCg(uint8_t YCoCg[3]) const;

    void                SetHSL(float hue, float saturation, float lightness);
    void                GetHSL(float& hue, float& saturation, float& lightness) const;

    void                SetCMYK(float cyan, float magenta, float yellow, float key);
    void                GetCMYK(float& cyan, float& magenta, float& yellow, float& key) const;

    /// Assume color is in linear space
    float               GetLuminance() const { return R * 0.2126f + G * 0.7152f + B * 0.0722f; }

    Color3              ToLinear() const;
    Color3              ToLinearFast() const;
    Color3              ToSRGB() const;

    /// Assume color is in linear space
    void                SetRGBE(uint32_t rgbe);

    /// Assume color is in linear space
    uint32_t            GetRGBE() const;

    static constexpr int sNumComponents() { return 3; }

    static constexpr Color3 sWhite()    { return Color3(1.0f); }

    static constexpr Color3 sBlack()    { return Color3(0.0f, 0.0f, 0.0f); }

    static constexpr Color3 sRed()      { return Color3(1.0f, 0.0f, 0.0f); }

    static constexpr Color3 sGreen()    { return Color3(0.0f, 1.0f, 0.0f); }

    static constexpr Color3 sBlue()     { return Color3(0.0f, 0.0f, 1.0f); }

    static constexpr Color3 sOrange()   { return Color3(1.0f, 0.456f, 0.1f); }
};

struct Color4 : Color3
{
    float               A{1};

    Color4() = default;
    constexpr explicit  Color4(float value) : Color3(value), A(value) {}
    constexpr           Color4(float r, float g, float b) : Color3(r, g, b), A(1.0f) {}
    constexpr           Color4(float r, float g, float b, float a) : Color3(r, g, b), A(a) {}
    constexpr           Color4(Color3 const& color) : Color3(color), A(1.0f) {}

    float&              operator[](int index) { HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range"); return (&R)[index]; }
    float const&        operator[](int index) const { HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range"); return (&R)[index]; }

    constexpr Color4    operator*(Color4 const& rhs) const { return Color4(R * rhs.R, G * rhs.G, B * rhs.B, A * rhs.A); }
    constexpr Color4    operator/(Color4 const& rhs) const { return Color4(R / rhs.R, G / rhs.G, B / rhs.B, A / rhs.A); }
    constexpr Color4    operator*(float rhs) const { return Color4(R * rhs, G * rhs, B * rhs, A * rhs); }
    constexpr Color4    operator/(float rhs) const { return (*this) * (1.0f / rhs); }

    Color4&             operator*=(Color4 const& rhs);
    Color4&             operator/=(Color4 const& rhs);
    Color4&             operator*=(float rhs);
    Color4&             operator/=(float rhs);

    void                SetAlpha(float alpha);
    float               GetAlpha() const { return A; }

    bool                IsTranslucent() const { return A < 1.0f; }
    bool                IsTransparent() const { return A < 0.0001f; }

    void                SetByte(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void                GetByte(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;

    void                SetDWord(uint32_t color);
    uint32_t            GetDWord() const;

    void                SetYCoCgAlpha(const uint8_t YCoCgAlpha[4]);
    void                GetYCoCgAlpha(uint8_t YCoCgAlpha[4]) const;

    Color4              ToLinear() const;
    Color4              ToLinearFast() const;
    Color4              ToSRGB() const;

    static constexpr int sNumComponents() { return 4; }
};

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::Color3, "( {} {} {} )", v.R, v.G, v.B);
HK_FORMAT_DEF_(Hk::Color4, "( {} {} {} {} )", v.R, v.G, v.B, v.A);

#include "Color.inl"
