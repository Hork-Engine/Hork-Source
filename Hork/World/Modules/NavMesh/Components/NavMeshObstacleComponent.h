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

#include <Hork/World/Modules/NavMesh/NavMeshInterface.h>
#include <Hork/World/Component.h>

HK_NAMESPACE_BEGIN

enum class NavMeshObstacleShape : uint8_t
{
    Box,
    Cylinder
};

class NavMeshObstacleComponent final : public Component
{
    friend class NavMeshInterface;

public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    //
    // Properties
    //

    /// The shape of the obstacle
    void                    SetShape(NavMeshObstacleShape shape);
    NavMeshObstacleShape    GetShape() const;

    /// The half size of the obstacle
    void                    SetHalfExtents(Float3 const& halfExtents);
    Float3 const&           GetHalfExtents() const;

    /// Height of the obstacle
    void                    SetHeight(float height);
    float                   GetHeight() const;

    /// Radius of the obstacle
    void                    SetRadius(float radius);
    float                   GetRadius() const;

    /// Rotation of the box around the Y axis, in degrees
    void                    SetAngle(float angle);
    float                   GetAngle() const;

    // Internal
    void                    BeginPlay();
    void                    EndPlay();
    void                    FixedUpdate();
    void                    DrawDebug(DebugRenderer& renderer);

private:
    NavMeshObstacleShape    m_Shape = NavMeshObstacleShape::Box;
    bool                    m_Update = false;
    unsigned int            m_ObstacleRef = 0;
    Float3                  m_Position;
    Float3                  m_HalfExtents = Float3(0.5f);
    float                   m_Angle = 0;
};

HK_FORCEINLINE void NavMeshObstacleComponent::SetShape(NavMeshObstacleShape shape)
{
    m_Shape = shape;
    m_Update = true;
}

HK_FORCEINLINE NavMeshObstacleShape NavMeshObstacleComponent::GetShape() const
{
    return m_Shape;
}

HK_FORCEINLINE void NavMeshObstacleComponent::SetHalfExtents(Float3 const& halfExtents)
{
    m_HalfExtents = halfExtents;
    m_Update = true;
}

HK_FORCEINLINE Float3 const& NavMeshObstacleComponent::GetHalfExtents() const
{
    return m_HalfExtents;
}

HK_FORCEINLINE void NavMeshObstacleComponent::SetHeight(float height)
{
    m_HalfExtents.Y = height * 0.5f;
    m_Update = true;
}

HK_FORCEINLINE float NavMeshObstacleComponent::GetHeight() const
{
    return m_HalfExtents.Y * 2;
}

HK_FORCEINLINE void NavMeshObstacleComponent::SetRadius(float radius)
{
    m_HalfExtents.X = m_HalfExtents.Z = radius;
    m_Update = true;
}

HK_FORCEINLINE float NavMeshObstacleComponent::GetRadius() const
{
    return Math::Max(m_HalfExtents.X, m_HalfExtents.Z);
}

HK_FORCEINLINE void NavMeshObstacleComponent::SetAngle(float angle)
{
    m_Angle = angle;
    m_Update = true;
}

HK_FORCEINLINE float NavMeshObstacleComponent::GetAngle() const
{
    return m_Angle;
}

HK_NAMESPACE_END
