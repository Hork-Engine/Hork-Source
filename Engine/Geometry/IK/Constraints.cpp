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

#include "Constraints.h"

HK_NAMESPACE_BEGIN

inline float dot(Quat const& a, Quat const& b)
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
}

float Angle(Quat const& a, Quat const& b)
{
    float num = std::min(std::abs(dot(a, b)), 1.0f);
    return (num > 0.999999f) ? 0.0f : (std::acos(num) * 2.0f * 57.29578f);
}

#define FLOAT_EPSILON 0.000001f
Quat RotateTowards(Quat const& from, Quat const& to, float maxDegreesDelta)
{
    float num = Angle(from, to);

    if (fabs(num) < FLOAT_EPSILON)
        return to;

    return Math::Slerp(from, to, std::min(1.0f, maxDegreesDelta / num));
}

void OrthoNormalize(Float3& normal, Float3& tangent)
{
    normal.NormalizeSelf();
    tangent = tangent - normal * Math::Dot(tangent, normal);
    tangent.NormalizeSelf();
}

// The direction and up vectors are assumed to be normalized.
Quat LookRotation(Float3 const& direcion, Float3 const& up)
{
    // Find orthonormal basis vectors
    Float3 f = direcion;
    Float3 u = up;
    Float3 r = Math::Cross(u, f);
    u = Math::Cross(f, r);

    // From world forward to object forward
    Quat f2d = Math::GetRotation(Float3(0, 0, 1), f);

    // what direction is the new object up?
    Float3 objectUp = f2d * Float3(0, 1, 0);
    // From object up to desired up
    Quat u2u = Math::GetRotation(objectUp, u);

    // Rotate to forward direction first, then twist to correct up
    Quat result = f2d * u2u; // TODO: Need to check out. Order changed.
    // Don't forget to normalize the result
    return result.Normalized();
}

static Quat LimitTwist(Quat const& rotation, Float3 const& axis, Float3 const& orthoAxis, float twistLimit)
{
    if (twistLimit >= 180)
        return rotation;

    twistLimit = std::max(twistLimit, 0.0f);

    Float3 normal = rotation * axis;
    Float3 orthoTangent = orthoAxis;
    OrthoNormalize(normal, orthoTangent);

    Float3 rotatedOrthoTangent = rotation * orthoAxis;
    OrthoNormalize(normal, rotatedOrthoTangent);

    Quat fixedRotation = Math::GetRotation(rotatedOrthoTangent, orthoTangent) * rotation;

    if (twistLimit <= 0)
        return fixedRotation;

    // Rotate from zero twist to free twist by the limited angle
    return RotateTowards(fixedRotation, rotation, twistLimit);
}


// Limits rotation to a single degree of freedom (along axis)
static Quat Limit1DOF(Quat const& rotation, Float3 const& axis)
{
    return Math::GetRotation(rotation * axis, axis) * rotation;
}

Quat IkConstraint::Apply(Quat const& rotation)
{
    switch (Type)
    {
    case IK_CONSTRAINT_UNDEFINED:
        return rotation;
    case IK_CONSTRAINT_ANGLE:
        return PostRotation(LimitAngle(PreRotation(rotation)));
    case IK_CONSTRAINT_HINGE:
        return PostRotation(LimitHinge(PreRotation(rotation)));
    }
    HK_ASSERT(0);
    return rotation;
}

Quat IkConstraint::PreRotation(Quat const& rotation) const
{
    return DefaultLocalRotation.Inversed() * rotation;
}

Quat IkConstraint::PostRotation(Quat const& rotation) const
{
    return DefaultLocalRotation * rotation;
}

Quat IkConstraint::LimitSwing(Quat const& rotation)
{
    if (Axis == Float3(0.0) || rotation == Quat::Identity() || m_Data.Angle.SwingLimit >= 180)
        return rotation;

    Float3 swingAxis = rotation * Axis;
    // Get the limited swing axis
    Quat swingRotation = Math::GetRotation(Axis, swingAxis);
    Quat limitedSwingRotation = RotateTowards(Quat::Identity(), swingRotation, m_Data.Angle.SwingLimit);

    // Rotation from current(illegal) swing rotation to the limited(legal) swing rotation
    Quat toLimits = Math::GetRotation(swingAxis, limitedSwingRotation * Axis);

    // Subtract the illegal rotation
    return toLimits * rotation;
}

Quat IkConstraint::LimitAngle(Quat const& rotation)
{
    const float TwistLimit = 180;  // Limit of twist rotation around the main axis.

    // Subtracting off-limits swing
    Quat swing = LimitSwing(rotation);

    // Apply twist limits
    return LimitTwist(swing, Axis, SecondaryAxis(), TwistLimit);
}

Quat IkConstraint::LimitHinge(Quat const& rotation)
{
    if (m_Data.Hinge.MinAngle == 0 && m_Data.Hinge.MaxAngle == 0)
        return Quat::Identity();

    // Get 1 degree of freedom rotation along axis
    Quat free1DOF = Limit1DOF(rotation, Axis);
    Quat workingSpace = (Quat::RotationAroundNormal(Math::Radians(m_Data.Hinge.LastAngle), Axis) * LookRotation(SecondaryAxis(), Axis)).Inversed();
    Float3 d = workingSpace * free1DOF * SecondaryAxis();
    float deltaAngle = Math::Degrees(std::atan2(d.X, d.Z));
    m_Data.Hinge.LastAngle = Math::Clamp(m_Data.Hinge.LastAngle + deltaAngle, m_Data.Hinge.MinAngle, m_Data.Hinge.MaxAngle);
    return Quat::RotationAroundNormal(Math::Radians(m_Data.Hinge.LastAngle), Axis);
}

HK_NAMESPACE_END
