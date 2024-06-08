#include "TerrainComponent.h"

#include <Engine/Core/ConsoleVar.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/World/DebugRenderer.h>

#include <Engine/World/Modules/Render/RenderInterfaceImpl.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawTerrainMesh("com_DrawTerrainMesh"s, "0"s);

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
