/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include <Hork/Core/StringID.h>
#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/Ref.h>

#include "Value.h"

HK_NAMESPACE_BEGIN

enum class AnimGraph_NodeType : uint8_t
{
    // Pose nodes
    Clip,
    Blend,
    Sum,
    Playback,
    Random,
    State,
    StateMachine,
    StateTransition,

    // Value nodes
    And,
    Param,
    ParamComparison,
    StateCondition
};

class AnimGraph_Node : public Noncopyable
{
    friend class            AnimationGraph;

public:
    virtual                 ~AnimGraph_Node() {}

    AnimGraph_NodeType      GetType() const { return m_Type; }

    bool                    IsValue() const { return m_Type > AnimGraph_NodeType::StateTransition; }

    bool                    IsPose() const { return !IsValue(); }

    uint32_t                GetID() const { return m_ID; }

protected:
    explicit                AnimGraph_Node(AnimGraph_NodeType type, uint32_t id) : m_Type(type), m_ID(id) {}

private:
    AnimGraph_NodeType      m_Type;
    uint32_t                m_ID;
};

class AnimGraph_Clip final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Clip;

    void                    SetClipID(StringView id) { m_ClipID = id; }

    String                  GetClipID() const { return m_ClipID; }

private:
    explicit                AnimGraph_Clip(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    String                  m_ClipID;
};

class AnimGraph_Blend final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Blend;

    struct PoseNode
    {
        uint32_t ID;
        float Factor;

        PoseNode(uint32_t id, float factor) : ID(id), Factor(factor)
        {}
    };

    void                    AddPoseNode(uint32_t nodeID, float factor) { m_PoseNodes.EmplaceBack(nodeID, factor); }

    void                    SetFactorNodeID(uint32_t nodeID) { m_FactorNodeID = nodeID; }

    Vector<PoseNode> const& GetPoseNodes() const { return m_PoseNodes; }

    uint32_t                GetFactorNodeID() const { return m_FactorNodeID; }

private:
    explicit                AnimGraph_Blend(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    Vector<PoseNode>        m_PoseNodes;
    uint32_t                m_FactorNodeID = ~0u;
};

class AnimGraph_Playback final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Playback;

    void                    SetSpeedProviderNode(uint32_t nodeID) { m_SpeedProviderNodeID = nodeID; }

    void                    SetChildNode(uint32_t nodeID) { m_ChildNodeID = nodeID; }

    uint32_t                GetSpeedProviderNode() const { return m_SpeedProviderNodeID; }

    uint32_t                GetChildNode() const { return m_ChildNodeID; }

private:
    explicit                AnimGraph_Playback(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    uint32_t                m_SpeedProviderNodeID = ~0u;
    uint32_t                m_ChildNodeID = ~0u;
};

class AnimGraph_Param final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Param;

    void                    SetParamID(StringID id) { m_ParamID = id; }

    StringID                GetParamID() const { return m_ParamID; }

private:
    explicit                AnimGraph_Param(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    StringID                m_ParamID;
};

class AnimGraph_ParamComparison final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::ParamComparison;

    enum class Op
    {
        Equal,
        NotEqual
    };

    void                    SetParamID(StringID id) { m_ParamID = id; }

    StringID                GetParamID() const { return m_ParamID; }

    template <typename T>
    void                    SetValue(T value) { m_Value.Value = value; }

    AnimGraph_Value const&  GetValue() const { return m_Value; }

    void                    SetOp(Op op) { m_Op = op; }

    Op                      GetOp() const { return m_Op; }

private:
    explicit                AnimGraph_ParamComparison(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    StringID                m_ParamID;
    Op                      m_Op = Op::Equal;
    AnimGraph_Value         m_Value;
};

class AnimGraph_And final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::And;

    void                    SetChildrenNodes(std::initializer_list<uint32_t> list) { m_Children = list; }

    Vector<uint32_t> const& GetChildrenNodes() const { return m_Children; }

private:
    explicit                AnimGraph_And(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    Vector<uint32_t>        m_Children;
};

class AnimGraph_Random final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Random;

    void                    SetChildrenNodes(std::initializer_list<uint32_t> list) { m_Children = list; }

    Vector<uint32_t> const& GetChildrenNodes() const { return m_Children; }

private:
    explicit                AnimGraph_Random(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    Vector<uint32_t>        m_Children;
};

class AnimGraph_Sum final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Sum;

    void                    SetFirstNode(uint32_t id) { m_FirstNodeID = id; }

    void                    SetSecondNode(uint32_t id) { m_SecondNodeID = id; }

    uint32_t                GetFirstNode() const { return m_FirstNodeID; }

    uint32_t                GetSecondNode() const { return m_SecondNodeID; }

private:
    explicit                AnimGraph_Sum(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    uint32_t                m_FirstNodeID = ~0u;
    uint32_t                m_SecondNodeID = ~0u;
};

class AnimGraph_State final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::State;

    void                    SetPoseNode(uint32_t id) { m_PoseNodeID = id; }

    uint32_t                GetPoseNode() const { return m_PoseNodeID; }

    void                    SetName(StringView name) { m_Name = name; }

    String const&           GetName() const { return m_Name; }

    void                    AddOutputTransitionNode(uint32_t id) { m_OutputTransitionNodes.Add(id); }

    Vector<uint32_t> const& GetOutputTransitionNodes() const { return m_OutputTransitionNodes; }

private:
    explicit                AnimGraph_State(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    uint32_t                m_PoseNodeID = ~0u;
    String                  m_Name;
    Vector<uint32_t>        m_OutputTransitionNodes;
};

class AnimGraph_StateCondition final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::StateCondition;

    void                    SetPhase(float phase) { m_Phase = phase; }

    float                   GetPhase() const { return m_Phase; }

private:
    explicit                AnimGraph_StateCondition(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    float                   m_Phase = 0.0f;
};

class AnimGraph_StateTransition final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::StateTransition;

    enum class TransitionType : uint8_t
    {
        FrozenFade
    };

    void                    SetConditionNode(uint32_t id) { m_ConditionNodeID = id; }

    uint32_t                GetConditionNode() const { return m_ConditionNodeID; }

    void                    SetDestinationStateNode(uint32_t id) { m_DestinationStateNodeID = id; }

    uint32_t                GetDestinationStateNode() const { return m_DestinationStateNodeID; }

    void                    SetDuration(float duration) { m_Duration = duration; }

    float                   GetDuration() const { return m_Duration; }

    void                    SetTransitionType(TransitionType type) { m_TransitionType = type; }

    TransitionType          GetTransitionType() const { return m_TransitionType; }

    void                    SetReversible(bool isReversible) { m_IsReversible = isReversible; }

    bool                    IsReversible() const { return m_IsReversible; }

private:
    explicit AnimGraph_StateTransition(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    uint32_t                m_ConditionNodeID = ~0u;
    uint32_t                m_DestinationStateNodeID = ~0u;
    float                   m_Duration = 0;
    TransitionType          m_TransitionType = TransitionType::FrozenFade;
    bool                    m_IsReversible = false;
};

class AnimGraph_StateMachine final : public AnimGraph_Node
{
    friend class            AnimationGraph;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::StateMachine;

    void                    SetStateNodes(std::initializer_list<uint32_t> nodes) { m_StateNodes = nodes; }

    Vector<uint32_t> const& GetStateNodes() const { return m_StateNodes; }

private:
    explicit                AnimGraph_StateMachine(uint32_t id) : AnimGraph_Node(TYPE, id) {}

    Vector<uint32_t>        m_StateNodes;
};

class AnimationGraph_Cooked;

class AnimationGraph : public RefCounted
{
public:
    template <typename T>
    T&                      AddNode() { m_Nodes.EmplaceBack(new T(GenerateNodeID())); return *static_cast<T*>(m_Nodes.Last().RawPtr()); }

    void                    SetRootNode(uint32_t id) { m_RootNode = id; }

    uint32_t                GetRootNode() const { return m_RootNode; }

    AnimGraph_Node*         FindNode(uint32_t id);

    AnimGraph_Node const*   FindNode(uint32_t id) const;

    void                    Optimize();

    bool                    Validate() const;

    Ref<AnimationGraph_Cooked> Cook();

private:
    uint32_t                GenerateNodeID() { return m_NodeIDGen++; }

    bool                    ValidateNode(AnimGraph_Node const* node) const;
    void                    PatchDescendantIDs(struct IdPatcher const& patcher, AnimGraph_Node* node);

    Vector<UniqueRef<AnimGraph_Node>> m_Nodes;
    uint32_t                m_NodeIDGen = 0;
    uint32_t                m_RootNode = ~0u;
};

HK_NAMESPACE_END
