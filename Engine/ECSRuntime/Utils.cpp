#include "Utils.h"

#include "Components/TransformComponent.h"
#include "Components/WorldTransformComponent.h"
#include "Components/FinalTransformComponent.h"
#include "Components/NodeComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/CameraComponent.h"
#include "Components/MovableTag.h"
#include "Components/TransformInterpolationTag.h"
#include "Components/ExperimentalComponents.h"

#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

//
//struct GameLevel
//{
//};
//
//struct GameLevelResource
//{
//    GameLevel* Instantiate()
//    {
//    }
//};



//ECS::EntityHandle MakePlayer(Float3 const& )
//{
//}




ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc)
{
    ECS::EntityHandle handle = commandBuffer.SpawnEntity();
    commandBuffer.AddComponent<NodeComponent>(handle, desc.Parent, desc.NodeFlags);
    commandBuffer.AddComponent<TransformComponent>(handle, desc.Position, desc.Rotation, desc.Scale);
    commandBuffer.AddComponent<WorldTransformComponent>(handle, desc.Position, desc.Rotation, desc.Scale);
    commandBuffer.AddComponent<FinalTransformComponent>(handle);

    if (desc.bMovable)
    {
        commandBuffer.AddComponent<MovableTag>(handle);

        if (desc.bTransformInterpolation)
            commandBuffer.AddComponent<TransformInterpolationTag>(handle);
    }

    return handle;
}

ECS::EntityHandle CreateRigidBody(ECS::CommandBuffer& commandBuffer, RigidBodyDesc const& desc)
{
    SceneNodeDesc nodeDesc;

    nodeDesc.Parent = desc.Parent;
    nodeDesc.Position = desc.Position;
    nodeDesc.Rotation = desc.Rotation;
    nodeDesc.Scale = desc.Scale;
    nodeDesc.NodeFlags = desc.MotionBehavior != MB_SIMULATED ? desc.NodeFlags : SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE;
    nodeDesc.bMovable = desc.MotionBehavior != MB_STATIC;
    nodeDesc.bTransformInterpolation = desc.bTransformInterpolation;

    ECS::EntityHandle handle = CreateSceneNode(commandBuffer, nodeDesc);

    switch (desc.MotionBehavior)
    {
        case MB_STATIC:
            commandBuffer.AddComponent<StaticBodyComponent>(handle, desc.Model, desc.CollisionGroup);
            break;
        case MB_SIMULATED:
            commandBuffer.AddComponent<DynamicBodyComponent>(handle, desc.Model, desc.CollisionGroup);
            break;
        case MB_KINEMATIC:
            commandBuffer.AddComponent<KinematicBodyComponent>(handle, desc.Model, desc.CollisionGroup);
            break;
    }

    if (desc.bIsTrigger)
    {
        TriggerComponent& trigger = commandBuffer.AddComponent<TriggerComponent>(handle);
        trigger.TriggerClass = desc.TriggerClass;
    }

    return handle;
}

ECS::EntityHandle CreateCharacterController(ECS::CommandBuffer& commandBuffer, CharacterControllerDesc const& desc)
{
    SceneNodeDesc nodeDesc;

    nodeDesc.Position = desc.Position;
    nodeDesc.Rotation = desc.Rotation;
    nodeDesc.NodeFlags = SCENE_NODE_ABSOLUTE_POSITION | SCENE_NODE_ABSOLUTE_ROTATION | SCENE_NODE_ABSOLUTE_SCALE;
    nodeDesc.bMovable = true;
    nodeDesc.bTransformInterpolation = desc.bTransformInterpolation;

    ECS::EntityHandle handle = CreateSceneNode(commandBuffer, nodeDesc);

    commandBuffer.AddComponent<CharacterControllerComponent>(handle);

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

    MeshComponent_ECS& mesh = commandBuffer.AddComponent<MeshComponent_ECS>(handle);
    mesh.Mesh = GameApplication::GetResourceManager().GetResource<MeshResource>("/Root/default/skybox.mesh");
    mesh.SubmeshIndex = 0;
    mesh.BoundingBox = BvAxisAlignedBox(Float3(-0.5f), Float3(0.5f));
    mesh.Materials[0] = GameApplication::GetMaterialManager().Get("skybox");

    return handle;
}

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

HK_NAMESPACE_END
