#include "Utils.h"

#include "SceneGraph.h"

#include "Components/MeshComponent.h"
#include "Components/RenderTransformComponent.h"

#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

ECS::EntityHandle CreateSkybox(ECS::CommandBuffer& commandBuffer, ECS::EntityHandle parent)
{
    SceneNodeDesc nodeDesc;

    nodeDesc.Parent = parent;
    nodeDesc.NodeFlags = SCENE_NODE_ABSOLUTE_ROTATION;
    nodeDesc.bMovable = true;
    nodeDesc.bTransformInterpolation = true;

    ECS::EntityHandle handle = CreateSceneNode(commandBuffer, nodeDesc);

    commandBuffer.AddComponent<RenderTransformComponent>(handle);

    MeshComponent_ECS& mesh = commandBuffer.AddComponent<MeshComponent_ECS>(handle);
    mesh.Mesh = GameApplication::GetResourceManager().GetResource<MeshResource>("/Root/default/skybox.mesh");
    mesh.SubmeshIndex = 0;
    mesh.BoundingBox = BvAxisAlignedBox(Float3(-0.5f), Float3(0.5f));
    mesh.Materials[0] = GameApplication::GetMaterialManager().Get("skybox");

    return handle;
}
#if 0
Transform CalculateWorldTransform(ECS::World* world, ECS::EntityHandle entity)
{
    ECS::EntityView entityView = world->GetEntityView(entity);

    Transform selfTransform;

    bool isDynamic = entityView.HasComponent<DynamicBodyComponent>();

    TransformComponent* transform = entityView.GetComponent<TransformComponent>();
    if (transform)
    {
        selfTransform.Position = transform->Position;
        selfTransform.Rotation = transform->Rotation;
        selfTransform.Scale    = transform->Scale;
    }

    if (!isDynamic)
    {
        NodeComponent* hierarchy = entityView.GetComponent<NodeComponent>();
        if (hierarchy)
        {
            Transform parentTransform;

            ECS::EntityHandle parent = hierarchy->GetParent();
            if (parent)
            {
                parentTransform = CalculateWorldTransform(world, parent);

                Float3x4 parentTransformMatrix;
                parentTransform.ComputeTransformMatrix(parentTransformMatrix);

                selfTransform.Position = hierarchy->Flags & SCENE_NODE_ABSOLUTE_POSITION ? selfTransform.Position : parentTransformMatrix * selfTransform.Position;
                selfTransform.Rotation = hierarchy->Flags & SCENE_NODE_ABSOLUTE_ROTATION ? selfTransform.Rotation : parentTransform.Rotation * selfTransform.Rotation;
                selfTransform.Scale    = hierarchy->Flags & SCENE_NODE_ABSOLUTE_SCALE ? selfTransform.Scale : parentTransform.Scale * selfTransform.Scale;
            }
        }
    }

    return selfTransform;
}

namespace
{

void DestroyEntityWithChildren_r(ECS::CommandBuffer& commandBuffer, SceneNode* node)
{
    for (auto& child : node->Children)
    {
        DestroyEntityWithChildren_r(commandBuffer, child);
    }

    commandBuffer.DestroyEntity(node->Entity);
}

} // namespace

void DestroyEntityWithChildren(ECS::World* world, ECS::CommandBuffer& commandBuffer, ECS::EntityHandle handle)
{
    ECS::EntityView entity = world->GetEntityView(handle);

    if (!entity.IsValid())
        return;

    NodeComponent* nodeComponent = entity.GetComponent<NodeComponent>();
    if (nodeComponent)
    {
        auto* node = nodeComponent->GetNode();
        if (node)
        {
            DestroyEntityWithChildren_r(commandBuffer, node);
            return;
        }
    }

    commandBuffer.DestroyEntity(handle);
}

//void DestroyActor(ECS::CommandBuffer& commandBuffer, GameActor& actor)
//{
//    for (auto entity : actor.Entities)
//        commandBuffer.DestroyEntity(entity);
//    actor.Entities.Clear();
//}

#endif

HK_NAMESPACE_END
