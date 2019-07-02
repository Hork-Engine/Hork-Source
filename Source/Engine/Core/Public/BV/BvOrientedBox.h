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

    void FromAxisAlignedBoxWithPadding( BvAxisAlignedBox const & _AxisAlignedBox, Float3 const & _Origin, Float3x3 const & _Orient, Float const & _Padding ) {
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

    void FromAxisAlignedBoxWithPadding( BvAxisAlignedBox const & _AxisAlignedBox, Float3x4 const & _TransformMatrix, Float const & _Padding ) {
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
