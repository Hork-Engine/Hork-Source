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

#include "MeshComponent.h"

#include <Engine/Core/ConsoleVar.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/World/WorldInterface.h>
#include <Engine/World/World.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawMeshDebug("com_DrawMeshDebug"s, "0"s);
ConsoleVar com_DrawMeshBounds("com_DrawMeshBounds"s, "0"s);

void MeshComponent::UpdateBoundingBox()
{
    // TODO
    //if (m_Pose)
    //    m_WorldBoundingBox = m_Pose->m_Bounds.Transform(GetOwner()->GetWorldTransformMatrix());
    //else
    //    m_WorldBoundingBox = BoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());
}

void MeshComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawMeshDebug)
    {
        if (MeshResource* resource = GameApplication::GetResourceManager().TryGet(m_Resource))
        {
            renderer.PushTransform(GetOwner()->GetWorldTransformMatrix());
            resource->DrawDebug(renderer);
            for (int surfaceIndex = 0; surfaceIndex < m_Surfaces.Size(); ++surfaceIndex)
                resource->DrawDebugSubpart(renderer, surfaceIndex);
            renderer.PopTransform();
        }
    }

    if (com_DrawMeshBounds)
    {
        // TODO
        //renderer.SetDepthTest(false);
        //renderer.SetColor(m_Pose ? Color4(0.5f, 0.5f, 1, 1) : Color4(1, 1, 1, 1));
        //renderer.DrawAABB(m_WorldBoundingBox);
    }
}

void ProceduralMeshComponent::UpdateBoundingBox()
{
    // TODO
    //m_WorldBoundingBox = BoundingBox.Transform(GetOwner()->GetWorldTransformMatrix());
}

void ProceduralMeshComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawMeshBounds)
    {
        // TODO
        //renderer.SetDepthTest(false);
        //renderer.SetColor(Color4(0.5f, 1, 0.5f, 1));
        //renderer.DrawAABB(m_WorldBoundingBox);
    }
}

HK_NAMESPACE_END
