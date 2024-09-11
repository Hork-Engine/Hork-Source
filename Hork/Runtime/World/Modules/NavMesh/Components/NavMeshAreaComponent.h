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

#include <Hork/Core/Containers/ArrayView.h>
#include <Hork/Runtime/World/Modules/NavMesh/NavMeshInterface.h>
#include <Hork/Runtime/World/Component.h>
#include <Hork/Runtime/World/GameObject.h>

HK_NAMESPACE_BEGIN

enum class NavMeshAreaShape : uint8_t
{
    Box,
    Cylinder,
    ConvexVolume
};

class NavMeshAreaComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    //
    // Constants
    //

    static constexpr int    MaxVolumeVerts = 32;

    //
    // Properties
    //

    void                    SetShape(NavMeshAreaShape shape);
    NavMeshAreaShape        GetShape() const;

    /// Area id assigned to the link (see NAV_MESH_AREA)
    void                    SetAreaType(NAV_MESH_AREA area);
    NAV_MESH_AREA           GetAreaType() const;    

    BvAxisAlignedBox        CalcBoundingBox() const;

    void                    SetHalfExtents(Float3 const& halfExtents);
    Float3 const&           GetHalfExtents() const;

    void                    SetCylinderRadius(float radius);
    float                   GetCylinderRadius() const;

    void                    SetHeight(float height);
    float                   GetHeight() const;

    void                    SetVolumeContour(ArrayView<Float2> vertices);
    StaticVector<Float2, MaxVolumeVerts> const& GetVolumeContour() const;

    // Internal
    void                    DrawDebug(DebugRenderer& renderer);

private:
    NavMeshAreaShape        m_Shape = NavMeshAreaShape::Box;
    NAV_MESH_AREA           m_AreaType = NAV_MESH_AREA_GROUND;
    Float3                  m_HalfExtents;
    StaticVector<Float2, MaxVolumeVerts> m_VolumeContour;
};

HK_FORCEINLINE void NavMeshAreaComponent::SetShape(NavMeshAreaShape shape)
{
    m_Shape = shape;
}

HK_FORCEINLINE NavMeshAreaShape NavMeshAreaComponent::GetShape() const
{
    return m_Shape;
}

HK_FORCEINLINE void NavMeshAreaComponent::SetAreaType(NAV_MESH_AREA area)
{
    m_AreaType = area;
}

HK_FORCEINLINE NAV_MESH_AREA NavMeshAreaComponent::GetAreaType() const
{
    return m_AreaType;
}

HK_FORCEINLINE void NavMeshAreaComponent::SetHalfExtents(Float3 const& halfExtents)
{
    m_HalfExtents = halfExtents;
}

HK_FORCEINLINE Float3 const& NavMeshAreaComponent::GetHalfExtents() const
{
    return m_HalfExtents;
}

HK_FORCEINLINE void NavMeshAreaComponent::SetCylinderRadius(float radius)
{
    m_HalfExtents.X = m_HalfExtents.Z = radius;
}

HK_FORCEINLINE float NavMeshAreaComponent::GetCylinderRadius() const
{
    return Math::Max(m_HalfExtents.X, m_HalfExtents.Z);
}

HK_FORCEINLINE void NavMeshAreaComponent::SetHeight(float height)
{
    m_HalfExtents.Y = height * 0.5f;
}

HK_FORCEINLINE float NavMeshAreaComponent::GetHeight() const
{
    return m_HalfExtents.Y * 2.0f;
}

HK_FORCEINLINE void NavMeshAreaComponent::SetVolumeContour(ArrayView<Float2> vertices)
{
    if (vertices.Size() > MaxVolumeVerts)
        return;
    m_VolumeContour.Resize(vertices.Size());
    Core::Memcpy(m_VolumeContour.ToPtr(), vertices.ToPtr(), vertices.Size() * sizeof(vertices[0]));
}

HK_FORCEINLINE StaticVector<Float2, NavMeshAreaComponent::MaxVolumeVerts> const& NavMeshAreaComponent::GetVolumeContour() const { return m_VolumeContour; }

HK_NAMESPACE_END
