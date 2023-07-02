#pragma once

#include <Engine/ECS/ECS.h>

#include <Engine/Core/Allocators/PoolAllocator.h>

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Quat.h>

#include "Systems/PhysicsSystem.h"

HK_NAMESPACE_BEGIN

enum SCENE_NODE_FLAGS : uint8_t
{
    SCENE_NODE_FLAGS_DEFAULT = 0,
    SCENE_NODE_ABSOLUTE_POSITION = 1,
    SCENE_NODE_ABSOLUTE_ROTATION = 2,
    SCENE_NODE_ABSOLUTE_SCALE = 4
};
HK_FLAG_ENUM_OPERATORS(SCENE_NODE_FLAGS);

class SceneNode
{
public:
    void SetTransform(Float3 const& position, Quat const& rotation, Float3 const& scale, SCENE_NODE_FLAGS flags);

    int Index{};
    ECS::EntityHandle Entity;
    ECS::EntityHandle ParentEntity;
    SceneNode* Parent{};
    TVector<SceneNode*> Children;
    class SceneGraph* Graph;
};

class SceneGraph
{
public:
    struct NodeTransform
    {
        Float3 Position;
        Quat Rotation;
        Float3 Scale;
    };

    NodeTransform* LocalTransform{};
    NodeTransform* WorldTransform{};

    Float3x4* WorldTransformMatrix{};
    TVector<SCENE_NODE_FLAGS> Flags;

    size_t GetHierarchySize() const;

    SceneGraph(ECS::World* world);

    ~SceneGraph();

    void UpdateHierarchy();

    void UpdateWorldTransforms();

    SceneNode* CreateNode(ECS::EntityHandle entity, ECS::EntityHandle parent);
    void DestroyNode(SceneNode* node);

    void DetachNode(SceneNode* node);

private:
    void UpdateHierarchy_r(SceneNode* node, int parent);

    TPoolAllocator<SceneNode, 1024> m_Allocator;
    ECS::World* m_World;
    SceneNode m_Root;
    TVector<uint16_t> m_Hierarchy;
    size_t m_NumTransforms{};
    TVector<SceneNode*> m_UnlinkedNodes;
    size_t m_NumRootNodes{};
    bool m_HierarchyDirty{true};
};

HK_NAMESPACE_END
