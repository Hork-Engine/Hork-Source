#pragma once

#include "../SceneGraph.h"

HK_NAMESPACE_BEGIN

struct NodeComponent
{
    friend class TransformSystem;
    friend class SceneGraph;

    NodeComponent(ECS::EntityHandle parent, SCENE_NODE_FLAGS flags = SCENE_NODE_FLAGS_DEFAULT) :
        m_Parent(parent),
        Flags(flags)
    {}

    ECS::EntityHandle GetParent() const
    {
        return m_Parent;
    }

    //ECS::EntityHandle GetFirstChildren() const
    //{
    //    return m_FirstChildren;
    //}

    //ECS::EntityHandle GetNextSibling() const
    //{
    //    return m_NextSibling;
    //}

    SCENE_NODE_FLAGS Flags{SCENE_NODE_FLAGS_DEFAULT};

    SceneNode* GetNode()
    {
        return m_Node;
    }

private:
    ECS::EntityHandle m_Parent;
    //ECS::EntityHandle m_FirstChildren;
    //ECS::EntityHandle m_NextSibling;

    SceneNode* m_Node{};
};




#if 0
struct NodeComponent
{
    friend class TransformSystem;
    friend class SceneGraph;

    Transform LocalTransform;
    Transform WorldTransform;

    NodeComponent(ECS::EntityHandle parent, SCENE_NODE_FLAGS flags = SCENE_NODE_FLAGS_DEFAULT) :
        m_Parent(parent),
        Flags(flags)
    {}

    ECS::EntityHandle GetParent() const
    {
        return m_Parent;
    }

    SCENE_NODE_FLAGS Flags{SCENE_NODE_FLAGS_DEFAULT};

private:
    ECS::EntityHandle m_Parent;
    SceneNode* m_Node{};
};
#endif

HK_NAMESPACE_END
