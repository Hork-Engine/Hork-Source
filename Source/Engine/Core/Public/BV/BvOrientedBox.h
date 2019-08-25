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

#include "BvAxisAlignedBox.h"

class BvOrientedBox {
public:
    Float3      Center;
    Float3      HalfSize;
    Float3x3    Orient;

    BvOrientedBox() = default;
    BvOrientedBox( Float3 const & _Center, Float3 const & _HalfSize ) : Center(_Center), HalfSize(_HalfSize) {}

    // TODO: Check correcteness

    void FromAxisAlignedBox( BvAxisAlignedBox const & _AxisAlignedBox, Float3 const & _Origin, Float3x3 const & _Orient ) {
        HalfSize = _AxisAlignedBox.HalfSize();
        Orient = _Orient;
        Center = _Origin + _Orient * _AxisAlignedBox.Center();
    }

    void FromAxisAlignedBoxWithPadding( BvAxisAlignedBox const & _AxisAlignedBox, Float3 const & _Origin, Float3x3 const & _Orient, float const & _Padding ) {
        HalfSize = _AxisAlignedBox.HalfSize() + _Padding;
        Orient = _Orient;
        Center = _Origin + _Orient * _AxisAlignedBox.Center();
    }

    void FromAxisAlignedBox( BvAxisAlignedBox const & _AxisAlignedBox, Float3x4 const & _TransformMatrix ) {
        const Float3 AabbCenter = _AxisAlignedBox.Center();
        HalfSize = _AxisAlignedBox.HalfSize();
        Orient[0][0] = _TransformMatrix[0][0];
        Orient[0][1] = _TransformMatrix[1][0];
        Orient[0][2] = _TransformMatrix[2][0];
        Orient[1][0] = _TransformMatrix[0][1];
        Orient[1][1] = _TransformMatrix[1][1];
        Orient[1][2] = _TransformMatrix[2][1];
        Orient[2][0] = _TransformMatrix[0][2];
        Orient[2][1] = _TransformMatrix[1][2];
        Orient[2][2] = _TransformMatrix[2][2];
        Center.X = _TransformMatrix[0][0] * AabbCenter[0] + _TransformMatrix[0][1] * AabbCenter[1] + _TransformMatrix[0][2] * AabbCenter[2] + _TransformMatrix[0][3];
        Center.Y = _TransformMatrix[1][0] * AabbCenter[0] + _TransformMatrix[1][1] * AabbCenter[1] + _TransformMatrix[1][2] * AabbCenter[2] + _TransformMatrix[1][3];
        Center.Z = _TransformMatrix[2][0] * AabbCenter[0] + _TransformMatrix[2][1] * AabbCenter[1] + _TransformMatrix[2][2] * AabbCenter[2] + _TransformMatrix[2][3];
    }

    void FromAxisAlignedBoxWithPadding( BvAxisAlignedBox const & _AxisAlignedBox, Float3x4 const & _TransformMatrix, float const & _Padding ) {
        Float3 AabbCenter = _AxisAlignedBox.Center();
        HalfSize = _AxisAlignedBox.HalfSize() + _Padding;
        Orient[0][0] = _TransformMatrix[0][0];
        Orient[0][1] = _TransformMatrix[1][0];
        Orient[0][2] = _TransformMatrix[2][0];
        Orient[1][0] = _TransformMatrix[0][1];
        Orient[1][1] = _TransformMatrix[1][1];
        Orient[1][2] = _TransformMatrix[2][1];
        Orient[2][0] = _TransformMatrix[0][2];
        Orient[2][1] = _TransformMatrix[1][2];
        Orient[2][2] = _TransformMatrix[2][2];
        Center.X = _TransformMatrix[0][0] * AabbCenter[0] + _TransformMatrix[0][1] * AabbCenter[1] + _TransformMatrix[0][2] * AabbCenter[2] + _TransformMatrix[0][3];
        Center.Y = _TransformMatrix[1][0] * AabbCenter[0] + _TransformMatrix[1][1] * AabbCenter[1] + _TransformMatrix[1][2] * AabbCenter[2] + _TransformMatrix[1][3];
        Center.Z = _TransformMatrix[2][0] * AabbCenter[0] + _TransformMatrix[2][1] * AabbCenter[1] + _TransformMatrix[2][2] * AabbCenter[2] + _TransformMatrix[2][3];
    }
};
