#pragma once

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Quat.h>

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

//inline Quat FromTo(Float3 const& from, Float3 const& to)
//{
//    Float3 f = from.Normalized();
//    Float3 t = to.Normalized();

//    if (f == t)
//        return Quat::Identity();

//    if (f == t * -1.0f)
//    {
//        Float3 ortho = Float3(1, 0, 0);
//        if (fabsf(f.y) < fabsf(f.x))
//            ortho = Float3(0, 1, 0);
//        if (fabsf(f.z) < fabs(f.y) && fabs(f.z) < fabsf(f.x))
//            ortho = Float3(0, 0, 1);

//        Float3 axis = cross(f, ortho).Normalized();
//        return Quat(0, axis.x, axis.y, axis.z);
//    }

//    Float3 half = (f + t).Normalized();
//    Float3 axis = cross(f, half);

//    return Quat(dot(f, half), axis.x, axis.y, axis.z);
//}

//inline Quat FromTo(Float3 const& from, Float3 const& to)
//{
//    // It is important that the inputs are of equal length when
//    // calculating the half-way vector.
//    Float3 u = from.Normalized();
//    Float3 v = to.Normalized();

//    if (u == v)
//        return Quat::Identity();

//    // Unfortunately, we have to check for when u == -v, as u + v
//    // in this case will be (0, 0, 0), which cannot be normalized.
//    if (u == -v)
//    {
//        // 180 degree rotation around any orthogonal vector
//        Float3 axis = u.perpendicular();
//        return Quat(0, axis.x, axis.y, axis.z);
//    }

//    Float3 half = (u + v).Normalized();
//    Float3 axis = cross(u, half);
//    return Quat(dot(u, half), axis.x, axis.y, axis.z);
//}

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
