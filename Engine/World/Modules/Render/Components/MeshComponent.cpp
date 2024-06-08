#include "MeshComponent.h"

#include <Engine/Core/ConsoleVar.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/World/WorldInterface.h>
#include <Engine/World/World.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawMeshDebug("com_DrawMeshDebug"s, "0"s);
ConsoleVar com_DrawMeshBounds("com_DrawMeshBounds"s, "0"s);

void MeshComponent::BeginPlay()
{
}

void MeshComponent::EndPlay()
{
}

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
