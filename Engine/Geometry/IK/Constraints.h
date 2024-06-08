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

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

struct IkTransform
{
    Float3 Position;
    Quat Rotation;
};

inline IkTransform operator*(const IkTransform& lhs, const IkTransform& rhs)
{
    IkTransform out;

    //out.scale = lhs.scale * rhs.scale;
    out.Rotation = lhs.Rotation * rhs.Rotation;
    out.Position = lhs.Position + lhs.Rotation * (/*lhs.scale * */ rhs.Position);

    return out;
}

enum IK_CONSTRAINT_TYPE
{
    IK_CONSTRAINT_UNDEFINED,
    IK_CONSTRAINT_ANGLE,
    IK_CONSTRAINT_HINGE
};

struct IkConstraint
{
    IK_CONSTRAINT_TYPE Type{IK_CONSTRAINT_UNDEFINED};

    Quat DefaultLocalRotation;
    Float3 Axis{0.0f, 0.0f, 1.0f};

    struct LimitAngle
    {
        float SwingLimit;
    };

    struct LimitHinge
    {
        float MinAngle;
        float MaxAngle;
        float LastAngle;
    };

    void Clear()
    {
        Type = IK_CONSTRAINT_UNDEFINED;
    }

    void InitAngleConstraint(float swingLimit = 45.0f)
    {
        Type = IK_CONSTRAINT_ANGLE;
        m_Data.Angle.SwingLimit = swingLimit;
    }

    void InitHingeConstraint(float minAngle = -45, float maxAngle = 90)
    {
        Type = IK_CONSTRAINT_ANGLE;
        m_Data.Hinge.MinAngle = minAngle;
        m_Data.Hinge.MaxAngle = maxAngle;
        m_Data.Hinge.LastAngle = 0;
    }

    Quat Apply(Quat const& rotation);

private:
    union
    {
        LimitAngle Angle;
        LimitHinge Hinge;
    } m_Data;

    Quat PreRotation(Quat const& rotation) const;

    Quat PostRotation(Quat const& rotation) const;

    Float3 SecondaryAxis() const { return Float3(Axis.Y, -Axis.Z, Axis.X); }

    Quat LimitSwing(Quat const& rotation);

    Quat LimitAngle(Quat const& rotation);

    Quat LimitHinge(Quat const& rotation);
};

HK_NAMESPACE_END
