#pragma once

#include "Node.h"

HK_NAMESPACE_BEGIN

using SceneNodeID = size_t;

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
