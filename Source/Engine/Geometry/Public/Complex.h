/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

struct SComplex
{
    float R;
    float I;

    SComplex() = default;

    constexpr SComplex(float r, float i) :
        R(r), I(i)
    {
    }

    constexpr SComplex operator+(SComplex const& rhs) const
    {
        return SComplex(R + rhs.R, I + rhs.I);
    }

    constexpr SComplex operator-(SComplex const& rhs) const
    {
        return SComplex(R - rhs.R, I - rhs.I);
    }

    constexpr SComplex operator*(SComplex const& rhs) const
    {
        return SComplex(R * rhs.R - I * rhs.I, R * rhs.I + I * rhs.R);
    }

    SComplex operator/(SComplex const& rhs) const
    {
        float d = 1.0f / (rhs.R * rhs.R + rhs.I * rhs.I);
        return SComplex((R * rhs.R + I * rhs.I) * d,
                        (rhs.R * I - R * rhs.I) * d);
    }

    constexpr SComplex operator*(float rhs) const
    {
        return SComplex(R * rhs, I * rhs);
    }

    constexpr SComplex operator/(float rhs) const
    {
        return (*this) * (1.0f / rhs);
    }

    void operator+=(SComplex const& rhs)
    {
        R += rhs.R;
        I += rhs.I;
    }    

    void operator-=(SComplex const& rhs)
    {
        R -= rhs.R;
        I -= rhs.I;
    }

    void operator*=(SComplex const& rhs)
    {
        float tempR = R;
        R           = tempR * rhs.R - I * rhs.I;
        I           = tempR * rhs.I + I * rhs.R;
    }

    void operator/=(SComplex const& rhs)
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
