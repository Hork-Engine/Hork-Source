#pragma once

#include <Engine/ECS/ECS.h>

#include <Engine/Math/VectorMath.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

using SceneNodeID = size_t;

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

class SceneGraphInterface
{
    struct Node
    {
        ECS::EntityHandle key;
        Node* next;

        Node* Children;
        Node* NextSibling;
        int Index;
    };

    struct NodeTransform
    {
        Float3 Position;
        Quat Rotation;
        Float3 Scale;
    };

public:
    void Clear();

    SceneNodeID Attach(ECS::EntityHandle entity, ECS::EntityHandle parent);

    void FinalizeGraph();

    void CalcWorldTransform();

    void SetLocalTransform(SceneNodeID nodeId, Float3 const& position, Quat const& rotation, Float3 const& scale, SCENE_NODE_FLAGS flags);
    void GetWorldTransform(SceneNodeID nodeId, Float3& position, Quat& rotation, Float3& scale) const;

private:
    class NodePool
    {
        struct Page
        {
            Node Nodes[1024];
        };

        TVector<Page*> m_Pages;

        int m_Address{};

    public:
        void Clear();
        Node* Allocate();
        ~NodePool();
    };

    class NodeHash
    {
        NodePool m_NodeAllocator;
        Node* m_HashTable[1024];

    public:
        void Clear();
        Node* Insert(ECS::EntityHandle key);
    };

    void UpdateIndex();
    void UpdateIndex_r(Node* node, int index);

    TVector<NodeTransform> m_LocalTransforms;
    TVector<NodeTransform> m_WorldTransforms;
    TVector<Float3x4> m_WorldTransformMatrix;
    TVector<SCENE_NODE_FLAGS> m_Flags;
    Node* m_Roots{};
    int m_NumRootNodes{};
    TVector<int> m_Hierarchy;
    NodeHash m_NodeHash;
};

HK_FORCEINLINE void SceneGraphInterface::SetLocalTransform(SceneNodeID nodeId, Float3 const& position, Quat const& rotation, Float3 const& scale, SCENE_NODE_FLAGS flags)
{
    auto index = ((Node*)nodeId)->Index;

    m_LocalTransforms[index].Position = position;
    m_LocalTransforms[index].Rotation = rotation;
    m_LocalTransforms[index].Scale = scale;

    m_Flags[index] = flags;
}

HK_FORCEINLINE void SceneGraphInterface::GetWorldTransform(SceneNodeID nodeId, Float3& position, Quat& rotation, Float3& scale) const
{
    auto index = ((Node*)nodeId)->Index;

    position = m_WorldTransforms[index].Position;
    rotation = m_WorldTransforms[index].Rotation;
    scale    = m_WorldTransforms[index].Scale;
}

HK_NAMESPACE_END
