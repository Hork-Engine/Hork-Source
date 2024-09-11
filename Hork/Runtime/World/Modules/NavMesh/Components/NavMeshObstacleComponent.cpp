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

#include "NavMeshObstacleComponent.h"
#include <Hork/Runtime/World/Modules/NavMesh/NavMeshInterface.h>
#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawNavMeshObstacles("com_DrawNavMeshObstacles"_s, "0"_s, CVAR_CHEAT);

void NavMeshObstacleComponent::BeginPlay()
{
    auto& navmesh = GetWorld()->GetInterface<NavMeshInterface>();

    m_Position = GetOwner()->GetWorldPosition();
    navmesh.AddObstacle(this);

    m_Update = false;
}

void NavMeshObstacleComponent::EndPlay()
{
    auto& navmesh = GetWorld()->GetInterface<NavMeshInterface>();

    navmesh.RemoveObstacle(this);
}

void NavMeshObstacleComponent::FixedUpdate()
{
    auto& navmesh = GetWorld()->GetInterface<NavMeshInterface>();

    if (m_Update || m_Position != GetOwner()->GetWorldPosition())
    {
        m_Position = GetOwner()->GetWorldPosition();
        m_Update = false;

        navmesh.UpdateObstacle(this);
    }
}

void NavMeshObstacleComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawNavMeshObstacles)
    {
        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1,1,0,1));
        if (m_Shape == NavMeshObstacleShape::Box)
        {
            if (m_Angle == 0.0f)
                renderer.DrawBox(m_Position, m_HalfExtents);
            else
                renderer.DrawOrientedBox(m_Position, Float3x3::sRotationY(Math::Radians(m_Angle)), m_HalfExtents);
        }
        else
            renderer.DrawCylinder(m_Position, Float3x3::sIdentity(), GetRadius(), GetHeight());
    }
}

HK_NAMESPACE_END
