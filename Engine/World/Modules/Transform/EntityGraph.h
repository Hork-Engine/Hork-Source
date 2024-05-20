#pragma once

#include <Engine/Core/Allocators/HandleAllocator.h>
#include <Engine/World/Resources/Resource_Skeleton.h>

HK_NAMESPACE_BEGIN

class SceneGraph;

class EntityGraph
{
public:
    struct Node
    {
        Handle<Node>            Parent;
        TVector<Handle<Node>>   Children;
        TRef<SkeletonPose>      Pose;
        EntityGraph*            Owner;

        Float3          Position;
        Quat            Rotation;
        Float3          Scale{1, 1, 1};

        Float3          WorldPosition[2];
        Quat            WorldRotation[2];
        Float3          WorldScale[2]{{1, 1, 1}, {1, 1, 1}};

        Float3          LerpPosition;
        Quat            LerpRotation;
        Float3          LerpScale;

        bool            bAbsolutePosition{};
        bool            bAbsoluteRotation{};
        bool            bAbsoluteScale{};

        bool            bInterpolate{};

        uint32_t        Joint{};

        uint32_t        Version;
    };

    explicit        EntityGraph(TRef<SceneGraph> inGraph);
                    ~EntityGraph();

    void            Attach(Handle<Node> inParent);
    void            Detach();

    Handle<Node>    GetRoot() const { return m_Root; }

    Handle<Node>    CreateNode(Handle<Node> inParent = {});

    Handle<Node>    CreateSocket(TRef<SkeletonPose> inPose, uint32_t inJoint);

    void            DestroyNode(Handle<Node> inHandle, bool inRecursive);

    void            SetTransform(Float3 const& inPosition, Quat const& inRotation, Float3 const& inScale);

    // TODO?
    //void          SetParent(EntityNodeID inHandle, EntityNodeID inParent);

private:
    void            CalcWorldTransform(int inStateIndex);
    void            CalcWorldTransform(int inStateIndex, Node& inNode);

    void            InterpolateTransformState(int inPrevState, int inCurState, float inInterpolate);
    void            InterpolateTransformState(Handle<Node> inHandle, int inPrevState, int inCurState, float inInterpolate);

    void            CopyTransformState(int inPrevState, int inCurState);
    void            CopyTransformState(Handle<Node> inHandle, int inPrevState, int inCurState);

    void            FreeNodesRecursive(Handle<Node> node);

    TRef<SceneGraph>    m_Graph;
    Handle<Node>        m_Root;

    //struct SocketData
    //{
    //    Handle<Node>                Node;
    //    TRef<SkeletonPose>   Pose;
    //    int                         Joint;
    //};
    //TVector<SocketData>     m_Sockets;

    friend class SceneGraph;
};

using EntityNodeID = Handle<EntityGraph::Node>;

#if 0
class EntityNodeView
{
public:
    EntityNodeView(EntityNodeID handle, HandleAllocator<EntityGraph::Node>& allocator);

    EntityNodeID GetHandle() const
    {
        return m_Handle;
    }

    bool IsValid() const
    {
        return m_EntityRef.Version == m_Handle.GetVersion();
    }

    void SetPosition(Float3 const& inPosition);
    void SetPositionAndRotaion(Float3 const& inPosition, Quat const& inRotation);
    void SetTransform(Float3 const& inPosition, Quat const& inRotation, Float3 const& inScale);

    void GetPosition(Float3& outPosition);
    void GetPositionAndRotaion(Float3& outPosition, Quat& outRotation);
    void GetTransform(Float3& outPosition, Quat& outRotation, Float3& outScale);

    void GetWorldPosition(Float3& outPosition);
    void GetWorldPositionAndRotaion(Float3& outPosition, Quat& outRotation);
    void GetWorldTransform(Float3& outPosition, Quat& outRotation, Float3& outScale);

    void GetLerpPosition(Float3& outPosition);
    void GetLerpPositionAndRotaion(Float3& outPosition, Quat& outRotation);
    void GetLerpTransform(Float3& outPosition, Quat& outRotation, Float3& outScale);

private:
    EntityNodeID m_Handle;
    EntityGraph::Node& m_EntityRef;
};
#endif

class SceneGraph : public RefCounted
{
public:
    TUniqueRef<EntityGraph> CreateEntityGraph(EntityNodeID inParent = {});

    void SetTransform(EntityNodeID inHandle, Float3 const& inPosition, Quat const& inRotation, Float3 const& inScale);
    void GetTransform(EntityNodeID inHandle, Float3& outPosition, Quat& outRotation, Float3& outScale);

    void SetPositionAndRotation(EntityNodeID inHandle, Float3 const& inPosition, Quat const& inRotation);

    void GetLerpTransform(EntityNodeID inHandle, Float3& outPosition, Quat& outRotation, Float3& outScale);
    void GetLerpPositionAndRotation(EntityNodeID inHandle, Float3& outPosition, Quat& outRotation);

    void SetAbsolutePosition(EntityNodeID inHandle, bool inAbsolute);
    void SetAbsoluteRotation(EntityNodeID inHandle, bool inAbsolute);
    void SetAbsoluteScale(EntityNodeID inHandle, bool inAbsolute);

    void SetInterpolate(EntityNodeID inHandle, bool inInterpolate);

    void CalcWorldTransform(int inStateIndex);
    void InterpolateTransformState(int inPrevState, int inCurState, float inInterpolate);
    void CopyTransformState(int inPrevState, int inCurState);

private:
    HandleAllocator<EntityGraph::Node> m_NodeAllocator;
    TVector<EntityGraph*> m_Entities;

    friend class EntityGraph;
};

// TODO: move to HierarchyComponent.h
struct HierarchyComponent
{
    TUniqueRef<EntityGraph> Graph;
};
// TODO: move to EntityAttachComponent.h
struct EntityAttachComponent
{
    EntityNodeID Node;
};

HK_NAMESPACE_END
