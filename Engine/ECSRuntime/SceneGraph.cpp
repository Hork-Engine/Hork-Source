#include "SceneGraph.h"
#include "Components/NodeComponent.h"

HK_NAMESPACE_BEGIN

SceneGraph::SceneGraph(ECS::World* world) :
    m_World(world)
{}

SceneGraph::~SceneGraph()
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(LocalTransform);
    Core::GetHeapAllocator<HEAP_MISC>().Free(WorldTransform);
    Core::GetHeapAllocator<HEAP_MISC>().Free(WorldTransformMatrix);
}

size_t SceneGraph::GetHierarchySize() const
{
    return m_Hierarchy.Size();
}

void SceneGraph::UpdateHierarchy()
{
    if (!m_HierarchyDirty)
        return;

    for (SceneNode* node : m_UnlinkedNodes)
    {
        ECS::EntityView parent = m_World->GetEntityView(node->ParentEntity);

        NodeComponent* h = parent.GetComponent<NodeComponent>();
        if (h)
        {
            h->m_Node->Children.Add(node);
            node->Parent = h->m_Node;
        }
        else
        {
            m_Root.Children.Add(node);
            node->Parent = &m_Root;
        }
    }
    m_UnlinkedNodes.Clear();

    m_Hierarchy.Clear();

    // Root stub
    m_Hierarchy.Add(0);

    UpdateHierarchy_r(&m_Root, 0);

    m_NumRootNodes = m_Root.Children.Size() + 1;

    if (m_NumTransforms < m_Hierarchy.Size())
    {
        m_NumTransforms = m_Hierarchy.Size();

        Core::GetHeapAllocator<HEAP_MISC>().Free(LocalTransform);
        Core::GetHeapAllocator<HEAP_MISC>().Free(WorldTransform);
        Core::GetHeapAllocator<HEAP_MISC>().Free(WorldTransformMatrix);

        LocalTransform = (NodeTransform*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(m_NumTransforms * sizeof(NodeTransform));
        WorldTransform = (NodeTransform*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(m_NumTransforms * sizeof(NodeTransform));
        WorldTransform[0].Position = Float3(0);
        WorldTransform[0].Rotation = Quat::Identity();
        WorldTransform[0].Scale = Float3(1);

        WorldTransformMatrix = (Float3x4*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(m_NumTransforms * sizeof(*WorldTransformMatrix));
        WorldTransformMatrix[0] = Float3x4::Identity();

        Flags.Resize(m_NumTransforms);
    }

    m_HierarchyDirty = false;
}

void SceneGraph::UpdateWorldTransforms()
{
    Core::Memcpy(WorldTransform + 1, LocalTransform + 1, sizeof(WorldTransform[0]) * (m_NumRootNodes - 1));

    for (size_t i = 1; i < m_NumRootNodes; ++i)
    {
        WorldTransformMatrix[i].Compose(WorldTransform[i].Position, WorldTransform[i].Rotation.ToMatrix3x3(), WorldTransform[i].Scale);
    }

    for (size_t i = m_NumRootNodes; i < m_Hierarchy.Size(); ++i)
    {
        auto parent = m_Hierarchy[i];

        WorldTransform[i].Position = Flags[i] & SCENE_NODE_ABSOLUTE_POSITION ? LocalTransform[i].Position : WorldTransformMatrix[parent]    * LocalTransform[i].Position;
        WorldTransform[i].Rotation = Flags[i] & SCENE_NODE_ABSOLUTE_ROTATION ? LocalTransform[i].Rotation : WorldTransform[parent].Rotation * LocalTransform[i].Rotation;
        WorldTransform[i].Scale    = Flags[i] & SCENE_NODE_ABSOLUTE_SCALE    ? LocalTransform[i].Scale    : WorldTransform[parent].Scale    * LocalTransform[i].Scale;

        WorldTransformMatrix[i].Compose(WorldTransform[i].Position, WorldTransform[i].Rotation.ToMatrix3x3(), WorldTransform[i].Scale);
    }
}

SceneNode* SceneGraph::CreateNode(ECS::EntityHandle entity, ECS::EntityHandle parent)
{
    SceneNode* node = new (m_Allocator.Allocate()) SceneNode;
    node->Entity = entity;
    node->ParentEntity = parent;
    node->Graph = this;

    m_UnlinkedNodes.Add(node);
    m_HierarchyDirty = true;

    // FIXME: Update world transform here?

    return node;
}

void SceneGraph::DetachNode(SceneNode* node)
{
    HK_ASSERT(node);

    auto index = node->Parent->Children.IndexOf(node);
    if (index != Core::NPOS)
        node->Parent->Children.Remove(index);

    node->ParentEntity = 0;
    node->Parent = &m_Root;

    m_HierarchyDirty = true;

    // FIXME: Update world transforms here?
}

void SceneGraph::DestroyNode(SceneNode* node)
{
    HK_ASSERT(node);

    auto it = m_UnlinkedNodes.Find(node);
    if (it != m_UnlinkedNodes.End())
    {
        m_UnlinkedNodes.Erase(it);
    }

    for (auto* child : node->Children)
    {
        m_Root.Children.Add(child);
        child->ParentEntity = 0;
        child->Parent = &m_Root;
    }

    auto index = node->Parent->Children.IndexOf(node);
    if (index != Core::NPOS)
        node->Parent->Children.Remove(index);

    node->~SceneNode();
    m_Allocator.Deallocate(node);

    m_HierarchyDirty = true;
}

void SceneGraph::UpdateHierarchy_r(SceneNode* node, int parent)
{
    for (auto* child : node->Children)
    {
        m_Hierarchy.Add(parent);
        child->Index = m_Hierarchy.Size() - 1;
    }
    for (auto* child : node->Children)
        UpdateHierarchy_r(child, child->Index);
}

void SceneNode::SetTransform(Float3 const& position, Quat const& rotation, Float3 const& scale, SCENE_NODE_FLAGS flags)
{
    HK_ASSERT(Index && Index < Graph->GetHierarchySize());
    Graph->LocalTransform[Index].Position = position;
    Graph->LocalTransform[Index].Rotation = rotation;
    Graph->LocalTransform[Index].Scale    = scale;
    Graph->Flags[Index] = flags;
}

HK_NAMESPACE_END
