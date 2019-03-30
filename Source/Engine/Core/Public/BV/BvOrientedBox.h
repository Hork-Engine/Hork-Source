#pragma once

#include "BvAxisAlignedBox.h"

class BvOrientedBox {
public:
    Float3      Center;
    Float3      HalfSize;
    Float3x3    Orient;

    BvOrientedBox() = default;
    BvOrientedBox( const Float3 & _Center, const Float3 & _HalfSize ) : Center(_Center), HalfSize(_HalfSize) {}

    // TODO: Check correcteness

    void FromAxisAlignedBox( const BvAxisAlignedBox & _AxisAlignedBox, const Float3 & _Origin, const Float3x3 & _Orient ) {
        HalfSize = _AxisAlignedBox.HalfSize();
        Orient = _Orient;
        Center = _Origin + _Orient * _AxisAlignedBox.Center();
    }

    void FromAxisAlignedBoxWithPadding( const BvAxisAlignedBox & _AxisAlignedBox, const Float3 & _Origin, const Float3x3 & _Orient, const Float & _Padding ) {
        HalfSize = _AxisAlignedBox.HalfSize() + _Padding;
        Orient = _Orient;
        Center = _Origin + _Orient * _AxisAlignedBox.Center();
    }

    void FromAxisAlignedBox( const BvAxisAlignedBox & _AxisAlignedBox, const Float3x4 & _TransformMatrix ) {
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

    void FromAxisAlignedBoxWithPadding( const BvAxisAlignedBox & _AxisAlignedBox, const Float3x4 & _TransformMatrix, const Float & _Padding ) {
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
