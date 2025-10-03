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

HK_NAMESPACE_BEGIN

struct Complex
{
    float R{0};
    float I{0};

    Complex() = default;

    constexpr Complex(float r, float i) :
        R(r), I(i)
    {
    }

    constexpr Complex operator+(Complex const& rhs) const
    {
        return Complex(R + rhs.R, I + rhs.I);
    }

    constexpr Complex operator-(Complex const& rhs) const
    {
        return Complex(R - rhs.R, I - rhs.I);
    }

    constexpr Complex operator*(Complex const& rhs) const
    {
        return Complex(R * rhs.R - I * rhs.I, R * rhs.I + I * rhs.R);
    }

    Complex operator/(Complex const& rhs) const
    {
        float d = 1.0f / (rhs.R * rhs.R + rhs.I * rhs.I);
        return Complex((R * rhs.R + I * rhs.I) * d,
                        (rhs.R * I - R * rhs.I) * d);
    }

    constexpr Complex operator*(float rhs) const
    {
        return Complex(R * rhs, I * rhs);
    }

    constexpr Complex operator/(float rhs) const
    {
        return (*this) * (1.0f / rhs);
    }

    void operator+=(Complex const& rhs)
    {
        R += rhs.R;
        I += rhs.I;
    }    

    void operator-=(Complex const& rhs)
    {
        R -= rhs.R;
        I -= rhs.I;
    }

    void operator*=(Complex const& rhs)
    {
        float tempR = R;
        R           = tempR * rhs.R - I * rhs.I;
        I           = tempR * rhs.I + I * rhs.R;
    }

    void operator/=(Complex const& rhs)
    {
        float d     = 1.0f / (rhs.R * rhs.R + rhs.I * rhs.I);
        float tempR = R;
        R           = (tempR * rhs.R + I * rhs.I) * d;
        I           = (rhs.R * I - tempR * rhs.I) * d;
    }

    void operator*=(float rhs)
    {
        R *= rhs;
        I *= rhs;
    }

    void operator/=(float rhs)
    {
        float invRhs = 1.0f / rhs;
        R *= invRhs;
        I *= invRhs;
    }
};

HK_NAMESPACE_END
