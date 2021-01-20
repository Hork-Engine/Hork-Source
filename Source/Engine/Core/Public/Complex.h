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

    SComplex( float InR, float InI )
        : R(InR)
        , I(InI)
    {
    }

    SComplex operator+( SComplex const & Rhs ) const {
        return SComplex( R+Rhs.R, I+Rhs.I );
    }

    void operator+=( SComplex const & Rhs ) {
        R += Rhs.R;
        I += Rhs.I;
    }

    SComplex operator-( SComplex const & Rhs ) const {
        return SComplex( R-Rhs.R, I-Rhs.I );
    }

    void operator-=( SComplex const & Rhs ) {
        R -= Rhs.R;
        I -= Rhs.I;
    }

    SComplex operator*( SComplex const & Rhs ) const {
        return SComplex( R*Rhs.R - I*Rhs.I, R*Rhs.I + I*Rhs.R );
    }

    void operator*=( SComplex const & Rhs ) {
        float tempR = R;
        R = tempR*Rhs.R - I*Rhs.I;
        I = tempR*Rhs.I + I*Rhs.R;
    }

    SComplex operator/( SComplex const & Rhs ) const {
        float d = 1.0f / (Rhs.R*Rhs.R + Rhs.I*Rhs.I);
        return SComplex( ( R*Rhs.R + I*Rhs.I ) * d,
                         ( Rhs.R * I - R * Rhs.I ) * d );
    }

    void operator/=( SComplex const & Rhs ) {
        float d = 1.0f / (Rhs.R*Rhs.R + Rhs.I*Rhs.I);
        float tempR = R;
        R = ( tempR * Rhs.R + I * Rhs.I ) * d;
        I = ( Rhs.R * I - tempR * Rhs.I ) * d;
    }

    SComplex operator*( float const & Rhs ) const {
        return SComplex( R * Rhs, I * Rhs );
    }

    void operator*=( float const & Rhs ) {
        R *= Rhs;
        I *= Rhs;
    }

    SComplex operator/( float const & Rhs ) const {
        float invRhs = 1.0f / Rhs;
        return SComplex( R * invRhs, I * invRhs );
    }

    void operator/=( float const & Rhs ) {
        float invRhs = 1.0f / Rhs;
        R *= invRhs;
        I *= invRhs;
    }
};
