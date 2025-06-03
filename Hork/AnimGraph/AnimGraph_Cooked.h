/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Core/Ref.h>

HK_NAMESPACE_BEGIN

class AnimationGraph_Cooked final : public RefCounted
{
    friend class            AnimationGraph;

public:
                            ~AnimationGraph_Cooked();

    // TODO:
    //static Ref<AnimationGraph_Cooked> sLoad(BinaryStreamReadInterface& stream);
    //void                    Write(BinaryStreamWriteInterface& stream);

    struct BlendPose
    {
        uint16_t            ID;
        float               Factor;

                            BlendPose(uint16_t id, float factor) : ID(id), Factor(factor) {}
    };
    union Node
    {
        struct Header
        {
            uint16_t        Type;
        };

        struct Clip
        {
            uint16_t        Type;
            uint16_t        ClipIDOffset;
        };
        struct Blend
        {
            uint16_t        Type;
            uint16_t        FirstBlendPose;
            uint16_t        NumBlendPoses;
            uint16_t        FactorNodeID;
        };
        struct Playback
        {
            uint16_t        Type;
            uint16_t        SpeedProviderNodeID;
            uint16_t        ChildNodeID;
        };
        struct Param
        {
            uint16_t        Type;
            uint16_t        ParamIDOffset;
        };
        struct ParamComparison
        {
            uint16_t        Type;
            uint16_t        ParamIDOffset;
            float           Value;
            uint8_t         Op;
        };
        struct And
        {
            uint16_t        Type;
            uint16_t        FirstNode;
            uint16_t        NumNodes;
        };
        struct Random
        {
            uint16_t        Type;
            uint16_t        FirstNode;
            uint16_t        NumNodes;
        };
        struct Sum
        {
            uint16_t        Type;
            uint16_t        FirstNodeID;
            uint16_t        SecondNodeID;
        };
        struct State
        {
            uint16_t        Type;
            uint16_t        PoseNodeID;
            uint16_t        NameOffset;
            uint16_t        FirstOutputTransitionNode;
            uint16_t        NumOutputTransitionNodes;
        };
        struct StateCondition
        {
            uint16_t        Type;
            float           Phase;
        };
        struct StateTransition
        {
            uint16_t        Type;
            uint16_t        ConditionNodeID;
            uint16_t        DestinationNodeID;
            uint8_t         TransType;
            bool            IsReversible;
            float           Duration;
        };
        struct StateMachine
        {
            uint16_t        Type;
            uint16_t        FirstStateNode;
            uint16_t        NumStateNodes;
        };
        Header              NodeHeader;
        Clip                NodeClip;
        Blend               NodeBlend;
        Playback            NodePlayback;
        Param               NodeParam;
        ParamComparison     NodeParamComparison;
        And                 NodeAnd;
        Random              NodeRandom;
        Sum                 NodeSum;
        State               NodeState;
        StateCondition      NodeStateCondition;
        StateTransition     NodeStateTransition;
        StateMachine        NodeStateMachine;
    };

    Vector<Node> const&     GetNodes() const { return m_Nodes; }
    Vector<BlendPose> const&GetBlendPoses() const { return m_BlendPoses; }
    Vector<uint16_t> const& GetNodeIDs() const { return m_NodeIDs; }
    uint16_t                GetRootNodeID() const { return m_RootNodeID; }
    const char*             GetParamIDs() const { return m_ParamIDs; }
    const char*             GetNames() const { return m_Names; }
    const char*             GetClips() const { return m_Clips; }

private:
    Vector<Node>            m_Nodes;
    Vector<BlendPose>       m_BlendPoses;
    Vector<uint16_t>        m_NodeIDs; // Logical And, Random, Output transition nodes, State machine state nodes
    uint16_t                m_RootNodeID{};
    char*                   m_ParamIDs{};
    char*                   m_Names{};
    char*                   m_Clips{};

                            AnimationGraph_Cooked() = default;
};

HK_NAMESPACE_END
