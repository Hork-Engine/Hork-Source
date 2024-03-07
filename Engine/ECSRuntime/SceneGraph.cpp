#include "SceneGraph.h"
#include "Components/NodeComponent.h"
#include "Components/TransformComponent.h"
#include "Components/WorldTransformComponent.h"
#include "Components/FinalTransformComponent.h"
#include "Components/MovableTag.h"
#include "Components/TransformInterpolationTag.h"

HK_NAMESPACE_BEGIN

void SceneGraphInterface::Clear()
{
    m_LocalTransforms.Clear();
    m_WorldTransforms.Clear();
    m_WorldTransformMatrix.Clear();
    m_Flags.Clear();
    m_Roots = nullptr;
    m_NumRootNodes = 0;
    m_Hierarchy.Clear();
    m_NodeHash.Clear();
}

SceneNodeID SceneGraphInterface::Attach(ECS::EntityHandle entity, ECS::EntityHandle parent)
{
    assert(entity != parent);

    Node* entity_node = m_NodeHash.Insert(entity);

    if (parent)
    {
        Node* parent_node = m_NodeHash.Insert(parent);

        entity_node->NextSibling = parent_node->Children;
        parent_node->Children = entity_node;
    }
    else
    {
        entity_node->NextSibling = m_Roots;
        m_Roots = entity_node;
    }

    return SceneNodeID(entity_node);
}

void SceneGraphInterface::FinalizeGraph()
{
    UpdateIndex();

    // TODO: Вектор вызывает ненужные конструкторы/деструкторы.

    m_LocalTransforms.Resize(m_Hierarchy.Size());
    m_WorldTransforms.Resize(m_Hierarchy.Size());

    m_WorldTransforms[0].Position = Float3(0);
    m_WorldTransforms[0].Rotation = Quat::Identity();
    m_WorldTransforms[0].Scale = Float3(1);

    m_WorldTransformMatrix.Resize(m_Hierarchy.Size());
    m_WorldTransformMatrix[0] = Float3x4::Identity();

    m_Flags.Resize(m_Hierarchy.Size());
}

void SceneGraphInterface::UpdateIndex()
{
    m_Hierarchy.Clear();
    m_Hierarchy.Add(0);

    for (Node* node = m_Roots; node; node = node->NextSibling)
    {
        m_Hierarchy.Add(0);
        node->Index = m_Hierarchy.Size() - 1;
    }

    m_NumRootNodes = m_Hierarchy.Size();

    for (Node* node = m_Roots; node; node = node->NextSibling)
        UpdateIndex_r(node, node->Index);
}

void SceneGraphInterface::UpdateIndex_r(Node* node, int index)
{
    for (Node* child = node->Children; child; child = child->NextSibling)
    {
        assert(child->Index == 0);
        if (child->Index)
            return; // cyclic dependency

        m_Hierarchy.Add(index);
        child->Index = m_Hierarchy.Size() - 1;
    }
    for (Node* child = node->Children; child; child = child->NextSibling)
        UpdateIndex_r(child, child->Index);
}

void SceneGraphInterface::CalcWorldTransform()
{
    // Copy local transform to world transform for all root nodes
    Core::Memcpy(m_WorldTransforms.ToPtr() + 1, m_LocalTransforms.ToPtr() + 1, sizeof(NodeTransform) * (m_NumRootNodes - 1));

    // Compose world transform matrix for all root nodes
    for (size_t i = 1; i < m_NumRootNodes; ++i)
    {
        m_WorldTransformMatrix[i].Compose(m_WorldTransforms[i].Position, m_WorldTransforms[i].Rotation.ToMatrix3x3(), m_WorldTransforms[i].Scale);
    }

    for (size_t i = m_NumRootNodes; i < m_Hierarchy.Size(); ++i)
    {
        auto parent = m_Hierarchy[i];

        m_WorldTransforms[i].Position = m_Flags[i] & SCENE_NODE_ABSOLUTE_POSITION ? m_LocalTransforms[i].Position : m_WorldTransformMatrix[parent]     * m_LocalTransforms[i].Position;
        m_WorldTransforms[i].Rotation = m_Flags[i] & SCENE_NODE_ABSOLUTE_ROTATION ? m_LocalTransforms[i].Rotation : m_WorldTransforms[parent].Rotation * m_LocalTransforms[i].Rotation;
        m_WorldTransforms[i].Scale    = m_Flags[i] & SCENE_NODE_ABSOLUTE_SCALE    ? m_LocalTransforms[i].Scale    : m_WorldTransforms[parent].Scale    * m_LocalTransforms[i].Scale;

        m_WorldTransformMatrix[i].Compose(m_WorldTransforms[i].Position, m_WorldTransforms[i].Rotation.ToMatrix3x3(), m_WorldTransforms[i].Scale);
    }
}

void SceneGraphInterface::NodePool::Clear()
{
    m_Address = 0;
}

SceneGraphInterface::Node* SceneGraphInterface::NodePool::Allocate()
{
    int page_num = m_Address >> 10;
    int page_offset = m_Address & 1023;

    m_Address++;

    if (page_num == m_Pages.Size())
        m_Pages.Add(new Page);

    return &m_Pages[page_num]->Nodes[page_offset];
}

SceneGraphInterface::NodePool::~NodePool()
{
    for (Page* page : m_Pages)
        delete page;
}

void SceneGraphInterface::NodeHash::Clear()
{
    m_NodeAllocator.Clear();
    memset(m_HashTable, 0, sizeof(m_HashTable));
}

SceneGraphInterface::Node* SceneGraphInterface::NodeHash::Insert(ECS::EntityHandle key)
{
    uint32_t hash = key.Hash() & 1023;

    for (Node* node = m_HashTable[hash]; node; node = node->next)
    {
        if (node->key == key)
            return node;
    }

    Node* node = m_NodeAllocator.Allocate();
    node->key = key;
    node->next = m_HashTable[hash];
    m_HashTable[hash] = node;

    node->Children = nullptr;
    node->NextSibling = nullptr;
    node->Index = 0;

    return node;
}

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

HK_NAMESPACE_END
