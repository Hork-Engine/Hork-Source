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

#include <Geometry/VectorMath.h>

struct Transform2D
{
    Float2 Col0{1,0};
    Float2 Col1{0,1};
    Float2 Col2{0,0};

    Transform2D() = default;

    constexpr Transform2D(float _00, float _01, float _10, float _11, float _20, float _21) :
        Col0(_00, _01),
        Col1(_10, _11),
        Col2(_20, _21)
    {}

    constexpr Transform2D(Float2 const& col0, Float2 const& col1, Float2 const& col2) :
        Col0(col0),
        Col1(col1),
        Col2(col2)
    {}

    Float2& operator[](int index)
    {
        HK_ASSERT(index >= 0 && index < 3);
        return (&Col0)[index];
    }

    Float2 const& operator[](int index) const
    {
        HK_ASSERT(index >= 0 && index < 3);
        return (&Col0)[index];
    }

    Float3x4 ToMatrix3x4() const
    {
        return Float3x4(Float4(Col0[0], Col0[1], 0.0f, 0.0f),
                        Float4(Col1[0], Col1[1], 0.0f, 0.0f),
                        Float4(Col2[0], Col2[1], 1.0f, 0.0f));
    }

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }

    void SetIdentity();

    static Transform2D Translation(Float2 const& vec);

    static Transform2D Scaling(Float2 const& scale);

    static Transform2D Rotation(float angleInRadians);

    static Transform2D SkewX(float angleInRadians);

    static Transform2D SkewY(float angleInRadians);

    Transform2D operator*(Transform2D const& rhs) const
    {
        return Transform2D(Col0[0] * rhs[0][0] + Col0[1] * rhs[1][0],
                           Col0[0] * rhs[0][1] + Col0[1] * rhs[1][1],

                           Col1[0] * rhs[0][0] + Col1[1] * rhs[1][0],
                           Col1[0] * rhs[0][1] + Col1[1] * rhs[1][1],

                           Col2[0] * rhs[0][0] + Col2[1] * rhs[1][0] + rhs[2][0],
                           Col2[0] * rhs[0][1] + Col2[1] * rhs[1][1] + rhs[2][1]);
    }

    Transform2D& operator*=(Transform2D const& rhs)
    {
        float m00 = Col0[0] * rhs.Col0[0] + Col0[1] * rhs.Col1[0];
        float m10 = Col1[0] * rhs.Col0[0] + Col1[1] * rhs.Col1[0];
        float m20 = Col2[0] * rhs.Col0[0] + Col2[1] * rhs.Col1[0] + rhs.Col2[0];

        Col0[1] = Col0[0] * rhs.Col0[1] + Col0[1] * rhs.Col1[1];
        Col1[1] = Col1[0] * rhs.Col0[1] + Col1[1] * rhs.Col1[1];
        Col2[1] = Col2[0] * rhs.Col0[1] + Col2[1] * rhs.Col1[1] + rhs.Col2[1];
        Col0[0] = m00;
        Col1[0] = m10;
        Col2[0] = m20;

        return *this;
    }

    Float2 operator*(Float2 const& p) const
    {
        return Float2(p.X * Col0[0] + p.Y * Col1[0] + Col2[0],
                      p.X * Col0[1] + p.Y * Col1[1] + Col2[1]);
    }

    Transform2D Inversed() const;
};
