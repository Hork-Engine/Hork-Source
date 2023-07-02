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
