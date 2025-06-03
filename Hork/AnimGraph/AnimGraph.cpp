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

#include "AnimGraph.h"
#include "AnimGraph_Cooked.h"

#include <Hork/Core/Logger.h>

HK_NAMESPACE_BEGIN

AnimGraph_Node* AnimationGraph::FindNode(uint32_t id)
{
    return const_cast<AnimGraph_Node*>(const_cast<AnimationGraph const*>(this)->FindNode(id));
}

AnimGraph_Node const* AnimationGraph::FindNode(uint32_t id) const
{
    for (auto const& node : m_Nodes)
    {
        if (node->GetID() == id)
            return node.RawPtr();
    }
    return nullptr;
}

struct IdPatcher
{
    HashMap<uint32_t, uint32_t> Remap;

    void PatchID(uint32_t& id) const
    {
        auto it = Remap.find(id);
        if (it == Remap.end())
            id = ~0u;
        else
            id = it->second;
    }
};

void AnimationGraph::Optimize()
{
    if (m_Nodes.IsEmpty())
        return;

    IdPatcher patcher;

    uint32_t id = 0;
    bool isOptimal = true;
    for (auto& node : m_Nodes)
    {
        if (node->GetID() != id)
            isOptimal = false;
        patcher.Remap[node->GetID()] = id++;
    }

    if (isOptimal)
        return;

    for (auto& node : m_Nodes)
    {
        PatchDescendantIDs(patcher, node.RawPtr());
    }
}

void AnimationGraph::PatchDescendantIDs(IdPatcher const& patcher, AnimGraph_Node* node)
{
    patcher.PatchID(node->m_ID);

    switch (node->GetType())
    {
        case AnimGraph_NodeType::Clip:
            break;
        case AnimGraph_NodeType::Blend:
        {
            auto blend = static_cast<AnimGraph_Blend*>(node);
            for (auto& poseNode : blend->m_PoseNodes)
                patcher.PatchID(poseNode.ID);
            patcher.PatchID(blend->m_FactorNodeID);
            break;
        }
        case AnimGraph_NodeType::Sum:
        {
            auto sum = static_cast<AnimGraph_Sum*>(node);
            patcher.PatchID(sum->m_FirstNodeID);
            patcher.PatchID(sum->m_SecondNodeID);
            break;
        }
        case AnimGraph_NodeType::And:
        {
            auto logicAnd = static_cast<AnimGraph_And*>(node);
            for (auto& childID : logicAnd->m_Children)
                patcher.PatchID(childID);
            break;
        }
        case AnimGraph_NodeType::Param:
            break;
        case AnimGraph_NodeType::ParamComparison:
            break;
        case AnimGraph_NodeType::Playback:
        {
            auto playback = static_cast<AnimGraph_Playback*>(node);
            patcher.PatchID(playback->m_SpeedProviderNodeID);
            patcher.PatchID(playback->m_ChildNodeID);
            break;
        }
        case AnimGraph_NodeType::Random:
        {
            auto random = static_cast<AnimGraph_Random*>(node);
            for (auto& childID : random->m_Children)
                patcher.PatchID(childID);
            break;
        }
        case AnimGraph_NodeType::State:
        {
            auto state = static_cast<AnimGraph_State*>(node);
            patcher.PatchID(state->m_PoseNodeID);
            for (auto& transitionNodeID : state->m_OutputTransitionNodes)
                patcher.PatchID(transitionNodeID);
            break;
        }
        case AnimGraph_NodeType::StateMachine:
        {
            auto stateMachine = static_cast<AnimGraph_StateMachine*>(node);
            for (auto stateNodeID : stateMachine->m_StateNodes)
                patcher.PatchID(stateNodeID);
            break;
        }
        case AnimGraph_NodeType::StateCondition:
            break;
        case AnimGraph_NodeType::StateTransition:
        {
            auto stateTransition = static_cast<AnimGraph_StateTransition*>(node);
            patcher.PatchID(stateTransition->m_ConditionNodeID);
            patcher.PatchID(stateTransition->m_DestinationStateNodeID);
            break;
        }
        default:
            HK_ASSERT(0);
            LOG("Unknown node type {}\n", ToUnderlying(node->GetType()));
            break;
    }
}

bool AnimationGraph::Validate() const
{
    if (m_Nodes.IsEmpty())
        return false;

    AnimGraph_Node* root = m_Nodes[m_RootNode < m_Nodes.Size() ? m_RootNode : 0].RawPtr();
    if (!root->IsPose())
    {
        LOG("AnimationGraph::Validate: the root node must be a pose node\n");
        return false;
    }

    for (auto& node : m_Nodes)
    {
        if (!ValidateNode(node.RawPtr()))
            return false;
    }

    return true;
}

bool AnimationGraph::ValidateNode(AnimGraph_Node const* node) const
{
    switch (node->GetType())
    {
        case AnimGraph_NodeType::Clip:
        {
            auto clipID = static_cast<AnimGraph_Clip const*>(node)->GetClipID();
            if (clipID.IsEmpty())
            {
                LOG("AnimationGraph::ValidateNode: [Clip] clip source is not specified\n");
                return false;
            }
            break;
        }
        case AnimGraph_NodeType::Blend:
        {
            auto blend = static_cast<AnimGraph_Blend const*>(node);

            if (blend->GetPoseNodes().IsEmpty())
            {
                LOG("AnimationGraph::ValidateNode: [Blend] pose nodes not specified\n");
                return false;
            }

            for (auto& poseNode : blend->GetPoseNodes())
            {
                AnimGraph_Node const* child = FindNode(poseNode.ID);
                if (!child || !child->IsPose())
                {
                    LOG("AnimationGraph::ValidateNode: [Blend] invalid pose node\n");
                    return false;
                }
            }

            auto factorNode = FindNode(blend->GetFactorNodeID());
            if (!factorNode || !factorNode->IsValue())
            {
                LOG("AnimationGraph::ValidateNode: [Blend] invalid factor node\n");
                return false;
            }

            break;
        }
        case AnimGraph_NodeType::Sum:
        {
            auto sum = static_cast<AnimGraph_Sum const*>(node);

            uint32_t childrenNodes[2] = {sum->GetFirstNode(), sum->GetSecondNode()};

            for (auto childID : childrenNodes)
            {
                auto child = FindNode(childID);
                if (!child || !child->IsPose())
                {
                    LOG("AnimationGraph::ValidateNode: [Sum] invalid child node\n");
                    return false;
                }
            }
            break;
        }
        case AnimGraph_NodeType::And:
        {
            auto logicAnd = static_cast<AnimGraph_And const*>(node);

            if (logicAnd->GetChildrenNodes().IsEmpty())
            {
                LOG("AnimationGraph::ValidateNode: [And] children nodes not specified\n");
                return false;
            }

            for (auto childID : logicAnd->GetChildrenNodes())
            {
                auto child = FindNode(childID);
                if (!child || !child->IsValue())
                {
                    LOG("AnimationGraph::ValidateNode: [And] invalid child node\n");
                    return false;
                }
            }
            break;
        }
        case AnimGraph_NodeType::Param:
        {
            break;
        }
        case AnimGraph_NodeType::ParamComparison:
        {
            break;
        }
        case AnimGraph_NodeType::Playback:
        {
            auto playback = static_cast<AnimGraph_Playback const*>(node);

            auto speedProviderNode = FindNode(playback->GetSpeedProviderNode());
            auto childNode = FindNode(playback->GetChildNode());

            if (!speedProviderNode || !speedProviderNode->IsValue())
            {
                LOG("AnimationGraph::ValidateNode: [Playback] invalid speed provider node\n");
                return false;
            }

            if (!childNode || !childNode->IsPose())
            {
                LOG("AnimationGraph::ValidateNode: [Playback] invalid child node\n");
                return false;
            }
            break;
        }
        case AnimGraph_NodeType::Random:
        {
            auto random = static_cast<AnimGraph_Random const*>(node);

            if (random->GetChildrenNodes().IsEmpty())
            {
                LOG("AnimationGraph::ValidateNode: [Random] children nodes not specified\n");
                return false;
            }

            for (auto childID : random->GetChildrenNodes())
            {
                auto child = FindNode(childID);
                if (!child || !child->IsPose())
                {
                    LOG("AnimationGraph::ValidateNode: [Random] invalid child node\n");
                    return false;
                }
            }
            break;
        }
        case AnimGraph_NodeType::State:
        {
            auto state = static_cast<AnimGraph_State const*>(node);

            auto poseNode = FindNode(state->GetPoseNode());
            if (!poseNode || !poseNode->IsPose())
            {
                LOG("AnimationGraph::ValidateNode: [State] invalid pose node\n");
                return false;
            }

            for (auto transitionNodeID : state->GetOutputTransitionNodes())
            {
                auto transitionNode = FindNode(transitionNodeID);
                if (!transitionNode || transitionNode->GetType() != AnimGraph_NodeType::StateTransition)
                {
                    LOG("AnimationGraph::ValidateNode: [State] invalid output transition node\n");
                    return false;
                }
            }
            break;
        }
        case AnimGraph_NodeType::StateMachine:
        {
            auto stateMachine = static_cast<AnimGraph_StateMachine const*>(node);

            if (stateMachine->GetStateNodes().IsEmpty())
            {
                LOG("AnimationGraph::ValidateNode: [StateMachine] state nodes not specified\n");
                return false;
            }

            for (auto stateNodeID : stateMachine->GetStateNodes())
            {
                auto stateNode = FindNode(stateNodeID);
                if (!stateNode || stateNode->GetType() != AnimGraph_NodeType::State)
                {
                    LOG("AnimationGraph::ValidateNode: [StateMachine] invalid state node\n");
                    return false;
                }
            }
            break;
        }
        case AnimGraph_NodeType::StateCondition:
        {
            break;
        }
        case AnimGraph_NodeType::StateTransition:
        {
            auto stateTransition = static_cast<AnimGraph_StateTransition const*>(node);

            auto conditionNode = FindNode(stateTransition->GetConditionNode());
            if (!conditionNode || !conditionNode->IsValue())
            {
                LOG("AnimationGraph::ValidateNode: [StateTransition] invalid condition node\n");
                return false;
            }

            auto destinationStateNode = FindNode(stateTransition->GetDestinationStateNode());
            if (!destinationStateNode || destinationStateNode->GetType() != AnimGraph_NodeType::State)
            {
                LOG("AnimationGraph::ValidateNode: [StateTransition] invalid destination state node\n");
                return false;
            }

            // TODO: validate duration?

            break;
        }
        default:
            HK_ASSERT(0);
            LOG("Unknown node type {}\n", ToUnderlying(node->GetType()));
            return false;
    }

    return true;
}

Ref<AnimationGraph_Cooked> AnimationGraph::Cook()
{
    Optimize();

    if (!Validate())
        return {};

    Ref<AnimationGraph_Cooked> cooked;
    cooked.Attach(new AnimationGraph_Cooked);

    cooked->m_Nodes.Resize(m_Nodes.Size());
    cooked->m_RootNodeID = m_RootNode < m_Nodes.Size() ? m_RootNode : 0;

    uint16_t clipIDOffset{};
    uint16_t nameOffset{};
    uint16_t paramIDOffset{};

    HashMap<StringID, uint16_t> paramIDToOffset;

    uint16_t id = 0;
    for (auto const& node : m_Nodes)
    {
        HK_ASSERT(node->GetID() == id);

        auto& cookedNode = cooked->m_Nodes[id];
        cookedNode.NodeHeader.Type = static_cast<uint16_t>(node->GetType());

        switch (node->GetType())
        {
            case AnimGraph_NodeType::Clip:
            {
                auto clip = static_cast<AnimGraph_Clip const*>(node.RawPtr());

                cookedNode.NodeClip.ClipIDOffset = clipIDOffset;
                clipIDOffset += clip->m_ClipID.Length() + 1;

                cooked->m_Clips = (char*)Core::GetHeapAllocator<HEAP_STRING>().Realloc(cooked->m_Clips, clipIDOffset, 0);
                Core::Memcpy(&cooked->m_Clips[cookedNode.NodeClip.ClipIDOffset], clip->m_ClipID.ToPtr(), clip->m_ClipID.Length() + 1);

                break;
            }
            case AnimGraph_NodeType::Blend:
            {
                auto blend = static_cast<AnimGraph_Blend const*>(node.RawPtr());

                cookedNode.NodeBlend.FirstBlendPose = cooked->m_BlendPoses.Size();
                cookedNode.NodeBlend.NumBlendPoses = blend->m_PoseNodes.Size();
                for (auto& poseNode : blend->m_PoseNodes)
                {
                    cooked->m_BlendPoses.EmplaceBack(poseNode.ID, poseNode.Factor);
                }
                cookedNode.NodeBlend.FactorNodeID = blend->m_FactorNodeID;
                break;
            }
            case AnimGraph_NodeType::Sum:
            {
                auto sum = static_cast<AnimGraph_Sum const*>(node.RawPtr());

                cookedNode.NodeSum.FirstNodeID = sum->m_FirstNodeID;
                cookedNode.NodeSum.SecondNodeID = sum->m_SecondNodeID;
                break;
            }
            case AnimGraph_NodeType::Playback:
            {
                auto playback = static_cast<AnimGraph_Playback const*>(node.RawPtr());
                cookedNode.NodePlayback.SpeedProviderNodeID = playback->m_SpeedProviderNodeID;
                cookedNode.NodePlayback.ChildNodeID = playback->m_ChildNodeID;
                break;
            }
            case AnimGraph_NodeType::Random:
            {
                auto random = static_cast<AnimGraph_Random const*>(node.RawPtr());
                cookedNode.NodeRandom.FirstNode = cooked->m_NodeIDs.Size();
                cookedNode.NodeRandom.NumNodes = random->m_Children.Size();
                for (auto child : random->m_Children)
                    cooked->m_NodeIDs.Add(child);
                break;
            }
            case AnimGraph_NodeType::State:
            {
                auto state = static_cast<AnimGraph_State const*>(node.RawPtr());

                cookedNode.NodeState.NameOffset = nameOffset;
                nameOffset += state->m_Name.Length() + 1;

                cooked->m_Names = (char*)Core::GetHeapAllocator<HEAP_STRING>().Realloc(cooked->m_Names, nameOffset, 0);
                memcpy(&cooked->m_Names[cookedNode.NodeState.NameOffset], state->m_Name.CStr(), state->m_Name.Length() + 1);

                cookedNode.NodeState.PoseNodeID = state->m_PoseNodeID;
                cookedNode.NodeState.FirstOutputTransitionNode = cooked->m_NodeIDs.Size();
                cookedNode.NodeState.NumOutputTransitionNodes = state->m_OutputTransitionNodes.Size();
                for (auto child : state->m_OutputTransitionNodes)
                    cooked->m_NodeIDs.Add(child);

                break;
            }
            case AnimGraph_NodeType::StateMachine:
            {
                auto stateMachine = static_cast<AnimGraph_StateMachine const*>(node.RawPtr());

                cookedNode.NodeStateMachine.FirstStateNode = cooked->m_NodeIDs.Size();
                cookedNode.NodeStateMachine.NumStateNodes = stateMachine->m_StateNodes.Size();
                for (auto child : stateMachine->m_StateNodes)
                    cooked->m_NodeIDs.Add(child);
                break;
            }
            case AnimGraph_NodeType::StateTransition:
            {
                auto stateTransition = static_cast<AnimGraph_StateTransition const*>(node.RawPtr());

                cookedNode.NodeStateTransition.ConditionNodeID = stateTransition->m_ConditionNodeID;
                cookedNode.NodeStateTransition.DestinationNodeID = stateTransition->m_DestinationStateNodeID;
                cookedNode.NodeStateTransition.Duration = stateTransition->m_Duration;
                cookedNode.NodeStateTransition.IsReversible = stateTransition->m_IsReversible;
                cookedNode.NodeStateTransition.TransType = static_cast<uint8_t>(stateTransition->m_TransitionType);
                break;
            }
            case AnimGraph_NodeType::And:
            {
                auto logicalAnd = static_cast<AnimGraph_And const*>(node.RawPtr());

                cookedNode.NodeAnd.FirstNode = cooked->m_NodeIDs.Size();
                cookedNode.NodeAnd.NumNodes = logicalAnd->m_Children.Size();
                for (auto child : logicalAnd->m_Children)
                    cooked->m_NodeIDs.Add(child);
                break;
            }
            case AnimGraph_NodeType::Param:
            {
                auto param = static_cast<AnimGraph_Param const*>(node.RawPtr());

                auto it = paramIDToOffset.find(param->m_ParamID);
                if (it == paramIDToOffset.end())
                {
                    StringView paramID = param->m_ParamID.GetStringView();
                    cookedNode.NodeParam.ParamIDOffset = paramIDOffset;
                    paramIDOffset += paramID.Length() + 1;

                    cooked->m_ParamIDs = (char*)Core::GetHeapAllocator<HEAP_STRING>().Realloc(cooked->m_ParamIDs, paramIDOffset, 0);
                    memcpy(&cooked->m_ParamIDs[cookedNode.NodeParam.ParamIDOffset], paramID.ToPtr(), paramID.Length() + 1);
                }
                else
                {
                    cookedNode.NodeParam.ParamIDOffset = it->second;
                }

                break;
            }
            case AnimGraph_NodeType::ParamComparison:
            {
                auto paramComparision = static_cast<AnimGraph_ParamComparison const*>(node.RawPtr());

                auto it = paramIDToOffset.find(paramComparision->m_ParamID);
                if (it == paramIDToOffset.end())
                {
                    StringView paramID = paramComparision->m_ParamID.GetStringView();
                    cookedNode.NodeParamComparison.ParamIDOffset = paramIDOffset;
                    paramIDOffset += paramID.Length() + 1;

                    cooked->m_ParamIDs = (char*)Core::GetHeapAllocator<HEAP_STRING>().Realloc(cooked->m_ParamIDs, paramIDOffset, 0);
                    memcpy(&cooked->m_ParamIDs[cookedNode.NodeParamComparison.ParamIDOffset], paramID.ToPtr(), paramID.Length() + 1);
                }
                else
                {
                    cookedNode.NodeParamComparison.ParamIDOffset = it->second;
                }

                cookedNode.NodeParamComparison.Value = paramComparision->m_Value.GetFloat();
                cookedNode.NodeParamComparison.Op = static_cast<uint8_t>(paramComparision->m_Op);
                break;
            }
            case AnimGraph_NodeType::StateCondition:
            {
                cookedNode.NodeStateCondition.Phase = static_cast<AnimGraph_StateCondition const*>(node.RawPtr())->m_Phase;
                break;
            }
        }
        id++;
    }

    return cooked;
}

HK_NAMESPACE_END
