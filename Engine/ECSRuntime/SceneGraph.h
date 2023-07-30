#pragma once

#include <Engine/ECS/ECS.h>

#include <Engine/Core/Allocators/PoolAllocator.h>

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

enum SCENE_NODE_FLAGS : uint8_t
{
    SCENE_NODE_FLAGS_DEFAULT = 0,
    SCENE_NODE_ABSOLUTE_POSITION = 1,
    SCENE_NODE_ABSOLUTE_ROTATION = 2,
    SCENE_NODE_ABSOLUTE_SCALE = 4
};
HK_FLAG_ENUM_OPERATORS(SCENE_NODE_FLAGS);

/// Regular scene node
struct SceneNodeDesc
{
    /// Scene node parent
    ECS::EntityHandle Parent;

    /// Position of the node
    Float3 Position;

    /// Rotation of the node
    Quat Rotation;

    /// Scale of the node
    Float3 Scale = Float3(1);

    SCENE_NODE_FLAGS NodeFlags = SCENE_NODE_FLAGS_DEFAULT;

    bool bMovable = false;

    /// Perform node transform interpolation between fixed time steps
    bool bTransformInterpolation = true;
};

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

ECS::EntityHandle CreateSceneNode(ECS::CommandBuffer& commandBuffer, SceneNodeDesc const& desc);

HK_NAMESPACE_END
