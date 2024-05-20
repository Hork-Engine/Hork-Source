#include "EntityGraph.h"
#include <Engine/World/Resources/Resource_Skeleton.h>

HK_NAMESPACE_BEGIN

EntityGraph::EntityGraph(TRef<SceneGraph> inGraph) :
    m_Graph(std::move(inGraph))
{
    m_Root = m_Graph->m_NodeAllocator.EntityAlloc();

    auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);
    root_ref.Owner = this;

    m_Graph->m_Entities.Add(this);
}

EntityGraph::~EntityGraph()
{
    auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);

    // If root is attached to another entity
    if (root_ref.Parent)
    {
        auto& parent = m_Graph->m_NodeAllocator.GetEntityRef(root_ref.Parent);
        parent.Children.Remove(parent.Children.IndexOf(m_Root));
    }
    else
        m_Graph->m_Entities.Remove(m_Graph->m_Entities.IndexOf(this));

    FreeNodesRecursive(m_Root);
}

void EntityGraph::Attach(Handle<Node> inParent)
{
    auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);

    if (root_ref.Parent == inParent)
        return;

    if (!inParent)
    {
        Detach();
        return;
    }

    auto& new_parent = m_Graph->m_NodeAllocator.GetEntityRef(inParent);
    if (new_parent.Version != inParent.GetVersion())
        return;

    if (root_ref.Parent)
    {
        auto& parent = m_Graph->m_NodeAllocator.GetEntityRef(root_ref.Parent);
        parent.Children.Remove(parent.Children.IndexOf(m_Root));
    }
    else
        m_Graph->m_Entities.Remove(m_Graph->m_Entities.IndexOf(this));

    root_ref.Parent = inParent;
    new_parent.Children.Add(m_Root);
}

void EntityGraph::Detach()
{
    auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);

    if (root_ref.Parent)
    {
        auto& parent = m_Graph->m_NodeAllocator.GetEntityRef(root_ref.Parent);

        parent.Children.Remove(parent.Children.IndexOf(m_Root));
        root_ref.Parent = {};

        m_Graph->m_Entities.Add(this);
    }
}

Handle<EntityGraph::Node> EntityGraph::CreateNode(Handle<Node> inParent)
{
    auto handle = m_Graph->m_NodeAllocator.EntityAlloc();
    auto& node = m_Graph->m_NodeAllocator.GetEntityRef(handle);

    if (!inParent || m_Graph->m_NodeAllocator.GetEntityRef(inParent).Version != inParent.GetVersion())
        node.Parent = m_Root;
    else
        node.Parent = inParent;
    
    node.Owner = this;

    auto& parent = m_Graph->m_NodeAllocator.GetEntityRef(node.Parent);
    parent.Children.Add(handle);
    return handle;
}

Handle<EntityGraph::Node> EntityGraph::CreateSocket(TRef<SkeletonPose> inPose, uint32_t inJoint)
{
    auto handle = CreateNode();

    auto& node = m_Graph->m_NodeAllocator.GetEntityRef(handle);
    node.Pose = inPose;
    node.Joint = inJoint;

    //m_Sockets.Add({handle, inPose, inJoint});
    return handle;
}

void EntityGraph::FreeNodesRecursive(Handle<Node> inHandle)
{
    auto& node_ref = m_Graph->m_NodeAllocator.GetEntityRef(inHandle);

    // Check... is we still in this EntityGraph
    if (node_ref.Owner != this)
    {
        node_ref.Parent = {};
        m_Graph->m_Entities.Add(node_ref.Owner);
        return;
    }

    for (int i = 0; i < node_ref.Children.Size(); i++)
        FreeNodesRecursive(node_ref.Children[i]);

    m_Graph->m_NodeAllocator.EntityFreeUnlocked(inHandle);
}

void EntityGraph::DestroyNode(Handle<Node> inHandle, bool recursive)
{
    if (inHandle == m_Root)
        return;

    auto& node_ref = m_Graph->m_NodeAllocator.GetEntityRef(inHandle);
    if (node_ref.Version != inHandle.GetVersion())
        return;

    HK_IF_NOT_ASSERT(node_ref.Owner == this)
    {
        return;
    }

    if (node_ref.Parent)
    {
        auto& parent_ref = m_Graph->m_NodeAllocator.GetEntityRef(node_ref.Parent);
        parent_ref.Children.Remove(parent_ref.Children.IndexOf(inHandle));
    }

    if (recursive)
    {
        FreeNodesRecursive(inHandle);
    }
    else
    {
        auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);
        for (auto& child : node_ref.Children)
        {
            auto& child_ref = m_Graph->m_NodeAllocator.GetEntityRef(child);
            child_ref.Parent = m_Root;
            root_ref.Children.Add(child);
        }
        m_Graph->m_NodeAllocator.EntityFreeUnlocked(inHandle);
    }
}

#if 0
void EntityGraph::UpdateSockets()
{
    Float3x3 rotation;

    for (auto& socket : m_Sockets)
    {
        if (socket.Pose)
        {
            auto& socket_transform = socket.Pose->GetJointTransform(socket.Joint);

            // TODO:  Убрать декомпозицию, хранить в позе не матрицы, а по отдельности: позицию, ориентацию, масштаб.
            socket_transform.DecomposeAll(socket.Node->Position, rotation, socket.Node->Scale);
            socket.Node->Rotation.FromMatrix(rotation);
        }
    }
}
#endif

void EntityGraph::CalcWorldTransform(int inStateIndex)
{
    auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);

    root_ref.WorldPosition[inStateIndex] = root_ref.Position;
    root_ref.WorldRotation[inStateIndex] = root_ref.Rotation;
    root_ref.WorldScale[inStateIndex]    = root_ref.Scale;

    CalcWorldTransform(inStateIndex, root_ref);
}

void EntityGraph::CalcWorldTransform(int inStateIndex, Node& inNode)
{
    if (inNode.Children.IsEmpty())
        return;

    Float3x4 node_world_transform;
    node_world_transform.Compose(inNode.WorldPosition[inStateIndex], inNode.WorldRotation[inStateIndex].ToMatrix3x3(), inNode.WorldScale[inStateIndex]);

    for (auto& children_handle : inNode.Children)
    {
        auto& children = m_Graph->m_NodeAllocator.GetEntityRef(children_handle);
        if (children.Pose)
        {
            auto& socket_transform = children.Pose->GetJointTransform(children.Joint);

            Float3 socket_position;
            Float3x3 socket_rotation_mat;
            Float3 socket_scale;

            socket_transform.DecomposeAll(socket_position, socket_rotation_mat, socket_scale);

            if (children.bAbsolutePosition)
            {
                children.WorldPosition[inStateIndex] = children.Position;
            }
            else
            {
                Float3 relative_position = socket_transform * children.Position;
                children.WorldPosition[inStateIndex] = node_world_transform * relative_position;
            }

            if (children.bAbsoluteRotation)
            {
                children.WorldRotation[inStateIndex] = children.Rotation;
            }
            else
            {
                Quat socket_rotation;
                socket_rotation.FromMatrix(socket_rotation_mat);
                Quat relative_rotation = socket_rotation * children.Rotation;
                children.WorldRotation[inStateIndex] = inNode.WorldRotation[inStateIndex] * relative_rotation;
            }

            if (children.bAbsoluteScale)
            {
                children.WorldScale[inStateIndex] = children.Scale;
            }
            else
            {
                Float3 relative_scale = socket_scale * children.Scale;
                children.WorldScale[inStateIndex] = inNode.WorldScale[inStateIndex] * relative_scale;
            }
        }
        else
        {
            children.WorldPosition[inStateIndex] = children.bAbsolutePosition ? children.Position : node_world_transform * children.Position;
            children.WorldRotation[inStateIndex] = children.bAbsoluteRotation ? children.Rotation : inNode.WorldRotation[inStateIndex] * children.Rotation;
            children.WorldScale[inStateIndex]    = children.bAbsoluteScale    ? children.Scale    : inNode.WorldScale[inStateIndex] * children.Scale;
        }

        CalcWorldTransform(inStateIndex, children);
    }
}

void EntityGraph::InterpolateTransformState(int inPrevState, int inCurState, float inInterpolate)
{
    InterpolateTransformState(m_Root, inPrevState, inCurState, inInterpolate);
}

void EntityGraph::InterpolateTransformState(Handle<Node> inHandle, int inPrevState, int inCurState, float inInterpolate)
{
    auto& node = m_Graph->m_NodeAllocator.GetEntityRef(inHandle);
    if (node.bInterpolate)
    {
        node.LerpPosition = Math::Lerp(node.WorldPosition[inPrevState], node.WorldPosition[inCurState], inInterpolate);
        node.LerpRotation = Math::Slerp(node.WorldRotation[inPrevState], node.WorldRotation[inCurState], inInterpolate);
        node.LerpScale    = Math::Lerp(node.WorldScale[inPrevState], node.WorldScale[inCurState], inInterpolate);
    }
    else
    {
        node.LerpPosition = node.WorldPosition[inCurState];
        node.LerpRotation = node.WorldRotation[inCurState];
        node.LerpScale    = node.WorldScale[inCurState];
    }

    for (auto& children : node.Children)
    {
        InterpolateTransformState(children, inPrevState, inCurState, inInterpolate);
    }
}

void EntityGraph::CopyTransformState(int inPrevState, int inCurState)
{
    CopyTransformState(m_Root, inPrevState, inCurState);
}

void EntityGraph::CopyTransformState(Handle<Node> inHandle, int inPrevState, int inCurState)
{
    auto& node = m_Graph->m_NodeAllocator.GetEntityRef(inHandle);
    node.LerpPosition = node.WorldPosition[inCurState];
    node.LerpRotation = node.WorldRotation[inCurState];
    node.LerpScale    = node.WorldScale[inCurState];

    for (auto& children : node.Children)
    {
        CopyTransformState(children, inPrevState, inCurState);
    }
}

void EntityGraph::SetTransform(Float3 const& inPosition, Quat const& inRotation, Float3 const& inScale)
{
    auto& root_ref = m_Graph->m_NodeAllocator.GetEntityRef(m_Root);

    root_ref.Position = inPosition;
    root_ref.Rotation = inRotation;
    root_ref.Scale = inScale;
}

TUniqueRef<EntityGraph> SceneGraph::CreateEntityGraph(EntityNodeID inParent)
{
    TUniqueRef<EntityGraph> entity_graph = MakeUnique<EntityGraph>(TRef<SceneGraph>(this));
    entity_graph->Attach(inParent);
    return entity_graph;
}

void SceneGraph::SetTransform(EntityNodeID inHandle, Float3 const& inPosition, Quat const& inRotation, Float3 const& inScale)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
        {
            node_ref.Position = inPosition;
            node_ref.Rotation = inRotation;
            node_ref.Scale    = inScale;
            return;
        }
    }
}

void SceneGraph::GetTransform(EntityNodeID inHandle, Float3& outPosition, Quat& outRotation, Float3& outScale)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
        {
            outPosition = node_ref.Position;
            outRotation = node_ref.Rotation;
            outScale    = node_ref.Scale;
            return;
        }
    }

    outPosition.Clear();
    outRotation.SetIdentity();
    outScale = Float3(1);
}

void SceneGraph::SetPositionAndRotation(EntityNodeID inHandle, Float3 const& inPosition, Quat const& inRotation)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
        {
            node_ref.Position = inPosition;
            node_ref.Rotation = inRotation;
        }
    }
}


void SceneGraph::GetLerpTransform(EntityNodeID inHandle, Float3& outPosition, Quat& outRotation, Float3& outScale)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
        {
            outPosition = node_ref.LerpPosition;
            outRotation = node_ref.LerpRotation;
            outScale    = node_ref.LerpScale;
            return;
        }
    }

    outPosition.Clear();
    outRotation.SetIdentity();
    outScale = Float3(1);
}

void SceneGraph::GetLerpPositionAndRotation(EntityNodeID inHandle, Float3& outPosition, Quat& outRotation)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
        {
            outPosition = node_ref.LerpPosition;
            outRotation = node_ref.LerpRotation;
            return;
        }
    }

    outPosition.Clear();
    outRotation.SetIdentity();
}
void SceneGraph::SetAbsolutePosition(EntityNodeID inHandle, bool inAbsolute)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
            node_ref.bAbsolutePosition = inAbsolute;
    }
}
void SceneGraph::SetAbsoluteRotation(EntityNodeID inHandle, bool inAbsolute)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
            node_ref.bAbsoluteRotation = inAbsolute;
    }
}
void SceneGraph::SetAbsoluteScale(EntityNodeID inHandle, bool inAbsolute)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
            node_ref.bAbsoluteScale = inAbsolute;
    }
}
void SceneGraph::SetInterpolate(EntityNodeID inHandle, bool inInterpolate)
{
    if (inHandle)
    {
        auto& node_ref = m_NodeAllocator.GetEntityRef(inHandle);
        if (node_ref.Version == inHandle.GetVersion())
            node_ref.bInterpolate = inInterpolate;
    }
}
void SceneGraph::CalcWorldTransform(int inStateIndex)
{
   for (auto* entity_ref : m_Entities)
    {
        //entity_ref->UpdateSockets();
        entity_ref->CalcWorldTransform(inStateIndex);
    }
}

void SceneGraph::InterpolateTransformState(int inPrevState, int inCurState, float inInterpolate)
{
    for (auto* entity_ref : m_Entities)
    {
        entity_ref->InterpolateTransformState(inPrevState, inCurState, inInterpolate);
    }
}

void SceneGraph::CopyTransformState(int inPrevState, int inCurState)
{
    for (auto* entity_ref : m_Entities)
    {
        entity_ref->CopyTransformState(inPrevState, inCurState);
    }
}

HK_NAMESPACE_END
