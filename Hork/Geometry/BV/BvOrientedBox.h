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

#include "BvAxisAlignedBox.h"

HK_NAMESPACE_BEGIN

struct BvOrientedBox
{
    Float3   Center;
    Float3   HalfSize;
    Float3x3 Orient;

    BvOrientedBox() = default;
    BvOrientedBox(Float3 const& center, Float3 const& halfSize) :
        Center(center), HalfSize(halfSize), Orient(1) {}

    bool operator==(BvOrientedBox const& rhs) const
    {
        return Center == rhs.Center && HalfSize == rhs.HalfSize && Orient == rhs.Orient;
    }

    bool operator!=(BvOrientedBox const& rhs) const
    {
        return !(operator==(rhs));
    }

    void GetVertices(Float3 vertices[8]) const;

    void FromAxisAlignedBox(BvAxisAlignedBox const& axisAlignedBox, Float3 const& origin, Float3x3 const& orient)
    {
        HalfSize = axisAlignedBox.HalfSize();
        Orient   = orient;
        Center   = origin + orient * axisAlignedBox.Center();
    }

    void FromAxisAlignedBoxWithPadding(BvAxisAlignedBox const& axisAlignedBox, Float3 const& origin, Float3x3 const& orient, float padding)
    {
        HalfSize = axisAlignedBox.HalfSize() + padding;
        Orient   = orient;
        Center   = origin + orient * axisAlignedBox.Center();
    }

    void FromAxisAlignedBox(BvAxisAlignedBox const& axisAlignedBox, Float3x4 const& transformMatrix)
    {
        const Float3 AabbCenter = axisAlignedBox.Center();
        HalfSize                = axisAlignedBox.HalfSize();
        Orient[0][0]            = transformMatrix[0][0];
        Orient[0][1]            = transformMatrix[1][0];
        Orient[0][2]            = transformMatrix[2][0];
        Orient[1][0]            = transformMatrix[0][1];
        Orient[1][1]            = transformMatrix[1][1];
        Orient[1][2]            = transformMatrix[2][1];
        Orient[2][0]            = transformMatrix[0][2];
        Orient[2][1]            = transformMatrix[1][2];
        Orient[2][2]            = transformMatrix[2][2];
        Center.X                = transformMatrix[0][0] * AabbCenter[0] + transformMatrix[0][1] * AabbCenter[1] + transformMatrix[0][2] * AabbCenter[2] + transformMatrix[0][3];
        Center.Y                = transformMatrix[1][0] * AabbCenter[0] + transformMatrix[1][1] * AabbCenter[1] + transformMatrix[1][2] * AabbCenter[2] + transformMatrix[1][3];
        Center.Z                = transformMatrix[2][0] * AabbCenter[0] + transformMatrix[2][1] * AabbCenter[1] + transformMatrix[2][2] * AabbCenter[2] + transformMatrix[2][3];
    }

    void FromAxisAlignedBoxWithPadding(BvAxisAlignedBox const& axisAlignedBox, Float3x4 const& transformMatrix, float padding)
    {
        Float3 AabbCenter = axisAlignedBox.Center();
        HalfSize          = axisAlignedBox.HalfSize() + padding;
        Orient[0][0]      = transformMatrix[0][0];
        Orient[0][1]      = transformMatrix[1][0];
        Orient[0][2]      = transformMatrix[2][0];
        Orient[1][0]      = transformMatrix[0][1];
        Orient[1][1]      = transformMatrix[1][1];
        Orient[1][2]      = transformMatrix[2][1];
        Orient[2][0]      = transformMatrix[0][2];
        Orient[2][1]      = transformMatrix[1][2];
        Orient[2][2]      = transformMatrix[2][2];
        Center.X          = transformMatrix[0][0] * AabbCenter[0] + transformMatrix[0][1] * AabbCenter[1] + transformMatrix[0][2] * AabbCenter[2] + transformMatrix[0][3];
        Center.Y          = transformMatrix[1][0] * AabbCenter[0] + transformMatrix[1][1] * AabbCenter[1] + transformMatrix[1][2] * AabbCenter[2] + transformMatrix[1][3];
        Center.Z          = transformMatrix[2][0] * AabbCenter[0] + transformMatrix[2][1] * AabbCenter[1] + transformMatrix[2][2] * AabbCenter[2] + transformMatrix[2][3];
    }
};

HK_NAMESPACE_END
