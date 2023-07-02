#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Geometry/Transform.h>

#include "Systems/PhysicsSystem.h"
#include "SceneGraph.h"

#include "Resources/ResourceManager.h"
#include "Resources/MaterialManager.h"

HK_NAMESPACE_BEGIN

struct SceneNodeDesc
{
    ECS::EntityHandle Parent;
    Float3            Position;
    Quat              Rotation;
    Float3            Scale = Float3(1);
    SCENE_NODE_FLAGS  NodeFlags = SCENE_NODE_FLAGS_DEFAULT;
    bool              bMovable = false;
    bool              bTransformInterpolation = true;
};

enum MOTION_BEHAVIOR
{
    /** Static non-movable object */
    MB_STATIC,

    /** Object motion is simulated by physics engine */
    MB_SIMULATED,

    /** Movable object */
    MB_KINEMATIC
};
struct RigidBodyDesc
{
    ECS::EntityHandle   Parent;
    TRef<CollisionModel> Model;
    Float3              Position;
    Quat                Rotation;
    Float3              Scale = Float3(1);
    SCENE_NODE_FLAGS    NodeFlags = SCENE_NODE_FLAGS_DEFAULT;
    MOTION_BEHAVIOR     MotionBehavior = MB_STATIC;
    uint8_t             CollisionGroup = CollisionGroup::DEFAULT; 
    bool                bTransformInterpolation = true;
    bool                bIsTrigger = false;
    ECS::ComponentTypeId TriggerClass = ECS::ComponentTypeId(-1);
};

struct CharacterControllerDesc
{
    Hk::Float3 Position;
    Hk::Quat   Rotation;
    bool       bTransformInterpolation = true;
};


ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc);
ECS::EntityHandle CreateRigidBody(ECS::CommandBuffer& commandBuffer, RigidBodyDesc const& desc);
ECS::EntityHandle CreateCharacterController(ECS::CommandBuffer& commandBuffer, CharacterControllerDesc const& desc);
ECS::EntityHandle CreateSkybox(ECS::CommandBuffer& commandBuffer, ECS::EntityHandle parent);

Transform CalculateWorldTransform(ECS::World* world, ECS::EntityHandle entity);

void DestroyEntityWithChildren(ECS::World* world, ECS::CommandBuffer& commandBuffer, ECS::EntityHandle handle);

HK_NAMESPACE_END
