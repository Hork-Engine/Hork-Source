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

#include "TerrainComponent.h"

#include <Engine/Core/ConsoleVar.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/World/DebugRenderer.h>

#include <Engine/World/Modules/Render/RenderInterfaceImpl.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawTerrainMesh("com_DrawTerrainMesh"_s, "0"_s);

void TerrainComponent::SetResource(TerrainHandle resource)
{
    m_Resource = resource;
}

void TerrainComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawTerrainMesh)
    {
        if (TerrainResource* resource = GameApplication::GetResourceManager().TryGet(m_Resource))
        {
            auto* renderInterfaceImpl = GetWorld()->GetInterface<RenderInterface>().GetImpl();

            renderer.SetDepthTest(false);
            renderer.SetColor(Color4(0, 0, 1, 0.5f));

            Float3x4 transform_matrix;
            transform_matrix.Compose(GetOwner()->GetWorldPosition(), GetOwner()->GetWorldRotation().ToMatrix3x3());

            Float3x4 transform_matrix_inv = transform_matrix.Inversed();
            Float3 local_view_position = transform_matrix_inv * renderer.GetRenderView()->ViewPosition;

            BvAxisAlignedBox local_bounds(local_view_position - 4, local_view_position + 4);

            local_bounds.Mins.Y = -FLT_MAX;
            local_bounds.Maxs.Y = FLT_MAX;

            auto& vertices = renderInterfaceImpl->m_DebugDrawVertices;
            auto& indices = renderInterfaceImpl->m_DebugDrawIndices;

            vertices.Clear();
            indices.Clear();
            resource->GatherGeometry(local_bounds, vertices, indices);

            renderer.PushTransform(transform_matrix);
            renderer.DrawTriangleSoupWireframe(vertices, indices);
            renderer.PopTransform();
        }
    }
}

HK_NAMESPACE_END
