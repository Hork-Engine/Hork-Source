/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "Quat.h"

struct Angl
{
    using ElementType = float;

    float Pitch{0};
    float Yaw{0};
    float Roll{0};

    Angl() = default;
    explicit constexpr Angl(Float3 const& v) :
        Pitch(v.X), Yaw(v.Y), Roll(v.Z) {}
    constexpr Angl(float pitch, float yaw, float roll) :
        Pitch(pitch), Yaw(yaw), Roll(roll) {}

    float* ToPtr()
    {
        return &Pitch;
    }

    const float* ToPtr() const
    {
        return &Pitch;
    }

    Float3& ToFloat3()
    {
        return *reinterpret_cast<Float3*>(&Pitch);
    }

    Float3 const& ToFloat3() const
    {
        return *reinterpret_cast<const Float3*>(&Pitch);
    }

    float& operator[](int index)
    {
        HK_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Pitch)[index];
    }

    float const& operator[](int index) const
    {
        HK_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&Pitch)[index];
    }

    bool operator==(Angl const& rhs) const { return Pitch == rhs.Pitch && Yaw == rhs.Yaw && Roll == rhs.Roll; }
    bool operator!=(Angl const& rhs) const { return !(operator==(rhs)); }

    Angl operator+() const
    {
        return *this;
    }
    Angl operator-() const
    {
        return Angl(-Pitch, -Yaw, -Roll);
    }
    Angl operator+(Angl const& rhs) const
    {
        return Angl(Pitch + rhs.Pitch, Yaw + rhs.Yaw, Roll + rhs.Roll);
    }
    Angl operator-(Angl const& rhs) const
    {
        return Angl(Pitch - rhs.Pitch, Yaw - rhs.Yaw, Roll - rhs.Roll);
    }
    Angl& operator+=(Angl const& rhs)
    {
        Pitch += rhs.Pitch;
        Yaw += rhs.Yaw;
        Roll += rhs.Roll;
        return *this;
    }
    Angl& operator-=(Angl const& rhs)
    {
        Pitch -= rhs.Pitch;
        Yaw -= rhs.Yaw;
        Roll -= rhs.Roll;
        return *this;
    }

    Bool3 IsInfinite() const
    {
        return Bool3(Math::IsInfinite(Pitch), Math::IsInfinite(Yaw), Math::IsInfinite(Roll));
    }

    Bool3 IsNan() const
    {
        return Bool3(Math::IsNan(Pitch), Math::IsNan(Yaw), Math::IsNan(Roll));
    }

    Bool3 IsNormal() const
    {
        return Bool3(Math::IsNormal(Pitch), Math::IsNormal(Yaw), Math::IsNormal(Roll));
    }

    bool CompareEps(Angl const& rhs, float epsilon) const
    {
        return Bool3(Math::CompareEps(Pitch, rhs.Pitch, epsilon),
                     Math::CompareEps(Yaw, rhs.Yaw, epsilon),
                     Math::CompareEps(Roll, rhs.Roll, epsilon))
            .All();
    }

    void Clear()
    {
        Pitch = Yaw = Roll = 0;
    }

    Quat ToQuat() const
    {
        float sx, sy, sz, cx, cy, cz;

        Math::DegSinCos(Pitch * 0.5f, sx, cx);
        Math::DegSinCos(Yaw * 0.5f, sy, cy);
        Math::DegSinCos(Roll * 0.5f, sz, cz);

        const float w = cy * cx;
        const float x = cy * sx;
        const float y = sy * cx;
        const float z = sy * sx;

        return Quat(w * cz + z * sz, x * cz + y * sz, -x * sz + y * cz, w * sz - z * cz);
    }

    Float3x3 ToMatrix3x3() const
    {
        float sx, sy, sz, cx, cy, cz;

        Math::DegSinCos(Pitch, sx, cx);
        Math::DegSinCos(Yaw, sy, cy);
        Math::DegSinCos(Roll, sz, cz);

        return Float3x3(cy * cz + sy * sx * sz, sz * cx, -sy * cz + cy * sx * sz,
                        -cy * sz + sy * sx * cz, cz * cx, sz * sy + cy * sx * cz,
                        sy * cx, -sx, cy * cx);
    }

    Float4x4 ToMatrix4x4() const
    {
        float sx, sy, sz, cx, cy, cz;

        Math::DegSinCos(Pitch, sx, cx);
        Math::DegSinCos(Yaw, sy, cy);
        Math::DegSinCos(Roll, sz, cz);

        return Float4x4(cy * cz + sy * sx * sz, sz * cx, -sy * cz + cy * sx * sz, 0.0f,
                        -cy * sz + sy * sx * cz, cz * cx, sz * sy + cy * sx * cz, 0.0f,
                        sy * cx, -sx, cy * cx, 0,
                        0.0f, 0.0f, 0.0f, 1.0f);
    }

    static float Normalize360(float angle)
    {
        //return (360.0f/65536) * (static_cast< int >(angle*(65536/360.0f)) & 65535);
        float norm = Math::FMod(angle, 360.0f);
        if (norm < 0.0f)
        {
            norm += 360.0f;
        }
        return norm;
    }

    static float Normalize180(float angle)
    {
        float norm = Normalize360(angle);
        if (norm > 180.0f)
        {
            norm -= 360.0f;
        }
        return norm;
    }

    void Normalize360Self()
    {
        Pitch = Normalize360(Pitch);
        Yaw   = Normalize360(Yaw);
        Roll  = Normalize360(Roll);
    }

    Angl Normalized360() const
    {
        return Angl(Normalize360(Pitch),
                    Normalize360(Yaw),
                    Normalize360(Roll));
    }

    void Normalize180Self()
    {
        Pitch = Normalize180(Pitch);
        Yaw   = Normalize180(Yaw);
        Roll  = Normalize180(Roll);
    }

    Angl Normalized180() const
    {
        return Angl(Normalize180(Pitch),
                    Normalize180(Yaw),
                    Normalize180(Roll));
    }

    Angl Delta(Angl const& rhs) const
    {
        return (*this - rhs).Normalized180();
    }

    static byte PackByte(float angle)
    {
        return Math::ToIntFast(angle * (256.0f / 360.0f)) & 255;
    }

    static uint16_t PackShort(float angle)
    {
        return Math::ToIntFast(angle * (65536.0f / 360.0f)) & 65535;
    }

    static float UnpackByte(byte angle)
    {
        return angle * (360.0f / 256.0f);
    }

    static float UnpackShort(uint16_t angle)
    {
        return angle * (360.0f / 65536.0f);
    }

    // String conversions
    AString ToString(int precision = -1) const
    {
        return AString("( ") + Math::ToString(Pitch, precision) + " " + Math::ToString(Yaw, precision) + " " + Math::ToString(Roll, precision) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(Pitch, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Yaw, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Roll, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteFloat(Pitch);
        stream.WriteFloat(Yaw);
        stream.WriteFloat(Roll);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        Pitch = stream.ReadFloat();
        Yaw   = stream.ReadFloat();
        Roll  = stream.ReadFloat();
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }

    static Angl const& Zero()
    {
        static constexpr Angl ZeroAngle(0.0f, 0.0f, 0.0f);
        return ZeroAngle;
    }
};

HK_FORCEINLINE Angl operator*(float lhs, Angl const& rhs)
{
    return Angl(lhs * rhs.Pitch, lhs * rhs.Yaw, lhs * rhs.Roll);
}
