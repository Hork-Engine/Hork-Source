#pragma once

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

#include <Jolt/Jolt.h>

HK_NAMESPACE_BEGIN

HK_INLINE JPH::Vec3 ConvertVector(Float3 const& v)
{
    return JPH::Vec3(v.X, v.Y, v.Z);
}

HK_INLINE JPH::Vec4 ConvertVector(Float4 const& v)
{
    return JPH::Vec4(v.X, v.Y, v.Z, v.W);
}

HK_INLINE JPH::Quat ConvertQuaternion(Quat const& q)
{
    return JPH::Quat(q.X, q.Y, q.Z, q.W);
}

HK_INLINE Float3 ConvertVector(JPH::Vec3 const& v)
{
    return Float3(v.GetX(), v.GetY(), v.GetZ());
}

HK_INLINE Float4 ConvertVector(JPH::Vec4 const& v)
{
    return Float4(v.GetX(), v.GetY(), v.GetZ(), v.GetW());
}

HK_INLINE Quat ConvertQuaternion(JPH::Quat const& q)
{
    return Quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

HK_INLINE Float4x4 ConvertMatrix(JPH::Mat44 const& m)
{
    return Float4x4(ConvertVector(m.GetColumn4(0)),
                    ConvertVector(m.GetColumn4(1)),
                    ConvertVector(m.GetColumn4(2)),
                    ConvertVector(m.GetColumn4(3)));
}

HK_INLINE JPH::Mat44 ConvertMatrix(Float4x4 const& m)
{
    return JPH::Mat44(ConvertVector(m.Col0),
                      ConvertVector(m.Col1),
                      ConvertVector(m.Col2),
                      ConvertVector(m.Col3));
}

HK_NAMESPACE_END
