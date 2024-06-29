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

#include <Engine/World/Modules/NavMesh/NavMeshInterface.h>
#include <Engine/World/Component.h>
#include <Engine/World/GameObject.h>

HK_NAMESPACE_BEGIN

class OffMeshLinkComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    //
    // Properties
    //

    void                    SetDestination(GameObjectHandle destination);
    GameObjectHandle        GetDestination() const;

    /// Radius
    void                    SetRadius(float radius);
    float                   GetRadius() const;

    /// A flag that indicates that an off-mesh link can be traversed in both directions
    void                    SetBidirectional(bool bidirectional);
    bool                    IsBidirectional() const;

    /// Area id assigned to the link
    void                    SetAreaType(NAV_MESH_AREA area);
    NAV_MESH_AREA           GetAreaType() const;

private:
    GameObjectHandle        m_Destination;
    float                   m_Radius = 0.5f;
    NAV_MESH_AREA           m_AreaType = NAV_MESH_AREA_GROUND;
    bool                    m_Bidirectional = false;    
};

HK_FORCEINLINE void OffMeshLinkComponent::SetDestination(GameObjectHandle destination)
{
    m_Destination = destination;
}

HK_FORCEINLINE GameObjectHandle OffMeshLinkComponent::GetDestination() const
{
    return m_Destination;
}

HK_FORCEINLINE void OffMeshLinkComponent::SetRadius(float radius)
{
    m_Radius = radius;
}

HK_FORCEINLINE float OffMeshLinkComponent::GetRadius() const
{
    return m_Radius;
}

HK_FORCEINLINE void OffMeshLinkComponent::SetBidirectional(bool bidirectional)
{
    m_Bidirectional = bidirectional;
}

HK_FORCEINLINE bool OffMeshLinkComponent::IsBidirectional() const
{
    return m_Bidirectional;
}

HK_FORCEINLINE void OffMeshLinkComponent::SetAreaType(NAV_MESH_AREA area)
{
    m_AreaType = area;
}

HK_FORCEINLINE NAV_MESH_AREA OffMeshLinkComponent::GetAreaType() const
{
    return m_AreaType;
}

HK_NAMESPACE_END
