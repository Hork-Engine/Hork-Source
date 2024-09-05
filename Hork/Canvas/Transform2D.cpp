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

#include "Transform2D.h"

HK_NAMESPACE_BEGIN

void Transform2D::SetIdentity()
{
    Col0[0] = 1.0f;
    Col0[1] = 0.0f;
    Col1[0] = 0.0f;
    Col1[1] = 1.0f;
    Col2[0] = 0.0f;
    Col2[1] = 0.0f;
}

Transform2D Transform2D::sTranslation(Float2 const& vec)
{
    return Transform2D(1, 0,
                       0, 1,
                       vec.X, vec.Y);
}

Transform2D Transform2D::sScaling(Float2 const& scale)
{
    return Transform2D(scale.X, 0,
                       0, scale.Y,
                       0, 0);
}

Transform2D Transform2D::sRotation(float angleInRadians)
{
    float cs = std::cos(angleInRadians), sn = std::sin(angleInRadians);

    return Transform2D(cs, sn,
                       -sn, cs,
                       0, 0);
}

Transform2D Transform2D::sSkewX(float angleInRadians)
{
    return Transform2D(1, 0,
                       std::tan(angleInRadians), 1,
                       0, 0);
}

Transform2D Transform2D::sSkewY(float angleInRadians)
{
    return Transform2D(1.0f, std::tan(angleInRadians),
                       0, 1,
                       0, 0);
}

Transform2D Transform2D::Inversed() const
{
    Transform2D const& m = *this;

    const float Determinant = m[0][0] * m[1][1] - m[1][0] * m[0][1];
    const float OneOverDeterminant = 1.0f / Determinant;

    Transform2D Result;

    Result[0][0] = m[1][1] * OneOverDeterminant;
    Result[0][1] = -m[0][1] * OneOverDeterminant;

    Result[1][0] = -m[1][0] * OneOverDeterminant;
    Result[1][1] = m[0][0] * OneOverDeterminant;

    Result[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
    Result[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;

    return Result;

    #if 0
    double det = (double)Col0[0] * Col1[1] - (double)Col1[0] * Col0[1];
    if (det > -1e-6 && det < 1e-6)
    {
        return {};
    }
    double invdet = 1.0 / det;

    Transform2D inv;
    inv[0][0] = (float)(Col1[1] * invdet);
    inv[0][1] = (float)(-Col0[1] * invdet);
    inv[1][0] = (float)(-Col1[0] * invdet);
    inv[1][1] = (float)(Col0[0] * invdet);
    inv[2][0] = (float)(((double)Col1[0] * Col2[1] - (double)Col1[1] * Col2[0]) * invdet);
    inv[2][1] = (float)(((double)Col0[1] * Col2[0] - (double)Col0[0] * Col2[1]) * invdet);
    return inv;
    #endif
}

HK_NAMESPACE_END
