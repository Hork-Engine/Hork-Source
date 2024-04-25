#include "Node.h"

#include <Engine/World/Modules/Transform/Components/NodeComponent.h>
#include <Engine/World/Modules/Transform/Components/TransformComponent.h>
#include <Engine/World/Modules/Transform/Components/WorldTransformComponent.h>
#include <Engine/World/Modules/Transform/Components/MovableTag.h>
#include <Engine/World/Modules/Transform/Components/TransformInterpolationTag.h>

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

HK_NAMESPACE_END
