#include "Utils.h"

#include "Components/NodeComponent.h"
#include "Components/TransformComponent.h"
#include "Components/WorldTransformComponent.h"
#include "Components/MovableTag.h"
#include "Components/TransformInterpolationTag.h"
#include "Components/MeshComponent.h"
#include "Components/RenderTransformComponent.h"

#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc)
{
    ECS::EntityHandle handle = commandBuffer.SpawnEntity();

    commandBuffer.AddComponent<NodeComponent>(handle, desc.Parent, desc.NodeFlags);
    commandBuffer.AddComponent<TransformComponent>(handle, desc.Position, desc.Rotation, desc.Scale);
    commandBuffer.AddComponent<WorldTransformComponent>(handle, desc.Position, desc.Rotation, desc.Scale);

    if (desc.bMovable)
    {
        commandBuffer.AddComponent<MovableTag>(handle);

        if (desc.bTransformInterpolation)
            commandBuffer.AddComponent<TransformInterpolationTag>(handle);
    }

    return handle;
}

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

HK_NAMESPACE_END
