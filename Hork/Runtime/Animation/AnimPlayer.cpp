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

#include "AnimPlayer.h"
#include "SkeletonPose.h"

#include <Hork/Core/Logger.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/skeleton.h>

HK_NAMESPACE_BEGIN

struct AnimationSampleContext : ozz::animation::SamplingJob::Context {};

static Vector<AnimPlayer_StateMachine*> s_ActiveStateMachineStack;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimPlayer_Pose::UpdateDuration(AnimPlayerContext& context)
{
    m_LastDurationUpdateTick = context.GetTickIndex();
}

uint32_t AnimPlayer_Pose::Tick(AnimPlayerContext& context)
{
    UpdateTickIndex(context);

    if (m_LastDurationUpdateTick != context.GetTickIndex())
    {
        UpdateDuration(context);
        m_LastDurationUpdateTick = context.GetTickIndex();
    }

    return ~0u;
}

float AnimPlayer_Pose::GetNextPhaseUnwrapped(AnimPlayerContext& context)
{
    if (!!(m_PhaseFlags & UpdatePhaseFlags::Copy))
    {
        HK_ASSERT(m_PhaseCopySource != nullptr);
        return m_PhaseCopySource->GetNextPhaseUnwrapped(context);
    }

    if (!!(m_PhaseFlags & UpdatePhaseFlags::Sync) && context.IsSyncEnabled())
    {
        return context.GetSyncPhase();
    }

    if (m_Duration == 0.0f)
        return 1.0f;

    if (IsFirstPlay(context))
        return 0.0f;

    float direction = !!(m_PhaseFlags & UpdatePhaseFlags::Reversed) ? -1.0f : 1.0f;
    return m_Phase + direction * context.GetSpeed() / m_Duration;
}

void AnimPlayer_Pose::ApplyNextPhase(AnimPlayerContext& context)
{
    if (!!(m_PhaseFlags & UpdatePhaseFlags::Copy))
    {
        HK_ASSERT(m_PhaseCopySource != nullptr);
        m_Phase = m_PhaseCopySource->m_Phase;
        return;
    }

    float nextPhaseUnwrapped = GetNextPhaseUnwrapped(context);

    if (!!(m_PhaseFlags & UpdatePhaseFlags::Wrap))
        m_Phase = Math::FMod(nextPhaseUnwrapped, 1.0f);
    else
        m_Phase = Math::Saturate(nextPhaseUnwrapped);
}

bool AnimPlayer_Pose::IsFirstPlay(AnimPlayerContext& context) const
{
    UpdateTickIndex(context);
    return m_IsFirstPlay;
}

void AnimPlayer_Pose::UpdateTickIndex(AnimPlayerContext& context) const
{
    if (m_LastTick != context.GetTickIndex())
    {
        uint32_t nextTick = m_LastTick + 1;
        if (nextTick == ~0u)
            nextTick = 0;

        m_IsFirstPlay = (m_LastTick == ~0u) || (nextTick != context.GetTickIndex());
        m_LastTick = context.GetTickIndex();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t AnimPlayer_Clip::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (AnimationResource* animation = resourceMngr.TryGet(m_AnimClip))
    {
        m_Duration = animation->GetDuration();
    }
    else
    {
        LOG("AnimPlayer_Clip::Tick: Animation clip is not loaded\n");
        m_Duration = 1;
    }

    ApplyNextPhase(context);

    if (!m_SamplingContext)
        m_SamplingContext = std::make_shared<AnimationSampleContext>();

    auto& job = context.AddJob<AnimJob_Sample>();
    job.m_Clip = m_AnimClip;
    job.m_Phase = GetPhase();
    job.m_SamplingContext = m_SamplingContext;
    return context.GetCurrentJobID();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimPlayer_Blend::SelectPoses(AnimPlayerContext& context)
{
    auto factor = m_FactorNode->Compute(context).GetFloat();

    if (m_PrevFactor == factor)
        return;

    m_PrevFactor = factor;

    uint32_t numPoses = m_PoseNodes.Size();
    HK_ASSERT(numPoses != 0);  // Should never happen! All validation should be on graph preprocessing step.

    uint32_t nextPoseIndex;
    for (nextPoseIndex = 0; nextPoseIndex < numPoses; ++nextPoseIndex)
    {
        if (m_PoseNodes[nextPoseIndex].Factor >= factor)
            break;
    }

    if (nextPoseIndex == numPoses)
    {
        uint32_t curPoseIndex = numPoses - 1;
        m_CurPose = nullptr;
        m_NextPose = m_PoseNodes[curPoseIndex].Pose;
        m_Weight = 1.0f;
        return;
    }

    if (nextPoseIndex == 0)
    {
        uint32_t curPoseIndex = 0;
        m_CurPose = nullptr;
        m_NextPose = m_PoseNodes[curPoseIndex].Pose;
        m_Weight = 1.0f;
        return;
    }

    if (fabs(factor - m_PoseNodes[nextPoseIndex].Factor) < std::numeric_limits<float>::epsilon())
    {
        m_CurPose = nullptr;
        m_NextPose = m_PoseNodes[nextPoseIndex].Pose;
        m_Weight = 1.0f;
        return;
    }

    uint32_t curPoseIndex = nextPoseIndex - 1;
    float range = m_PoseNodes[nextPoseIndex].Factor - m_PoseNodes[curPoseIndex].Factor;
    if (range < std::numeric_limits<float>::epsilon())
    {
        m_CurPose = nullptr;
        m_NextPose = m_PoseNodes[nextPoseIndex].Pose;
        m_Weight = 1.0f;
        return;
    }

    m_CurPose = m_PoseNodes[curPoseIndex].Pose;
    m_NextPose = m_PoseNodes[nextPoseIndex].Pose;
    m_Weight = (factor - m_PoseNodes[curPoseIndex].Factor) / range;
}

void AnimPlayer_Blend::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    SelectPoses(context);

    if (!m_CurPose)
    {
        m_NextPose->UpdateDuration(context);
        m_Duration = m_NextPose->GetDuration();
    }
    else
    {
        m_CurPose->UpdateDuration(context);
        m_NextPose->UpdateDuration(context);
        m_Duration = Math::Lerp(m_CurPose->GetDuration(), m_NextPose->GetDuration(), m_Weight);
    }
}

uint32_t AnimPlayer_Blend::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    float nextPhaseUnwrapped = GetNextPhaseUnwrapped(context);
    ApplyNextPhase(context);

    auto* prevStack = context.GetStackPointer();
    AnimPlayerStack stack = *prevStack;
    stack.m_SyncEnabled = true;
    stack.m_SyncPhase = nextPhaseUnwrapped;

    context.SetStackPointer(&stack);

    if (!m_CurPose)
    {
        auto result = m_NextPose->Tick(context);

        context.SetStackPointer(prevStack);
        return result;
    }

    auto pose1 = m_CurPose->Tick(context);
    auto pose2 = m_NextPose->Tick(context);

    auto& job = context.AddJob<AnimJob_Blend>();
    job.m_ChildJobIDs[0] = pose1;
    job.m_ChildJobIDs[1] = pose2;
    job.m_Weight = m_Weight;
    auto jobBlendID = context.GetCurrentJobID();

    context.SetStackPointer(prevStack);

    return jobBlendID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimPlayer_Sum::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    m_FirstNode->UpdateDuration(context);
    m_SecondNode->UpdateDuration(context);
    m_Duration = std::max(m_FirstNode->GetDuration(), m_SecondNode->GetDuration());
}

uint32_t AnimPlayer_Sum::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    ApplyNextPhase(context);

    uint32_t pose1 = m_FirstNode->Tick(context);
    uint32_t pose2 = m_SecondNode->Tick(context);

    auto& job = context.AddJob<AnimJob_Sum>();
    job.m_ChildJobIDs[0] = pose1;
    job.m_ChildJobIDs[1] = pose2;

    return context.GetCurrentJobID();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimPlayer_Playback::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    m_Duration = m_ChildNode->GetDuration();
}

uint32_t AnimPlayer_Playback::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    AnimGraph_Value speed = m_SpeedProviderNode->Compute(context);

    AnimPlayerStack* prevStack = context.GetStackPointer();
    AnimPlayerStack stack = *prevStack;

    stack.m_Speed *= speed.GetFloat();

    context.SetStackPointer(&stack);

    auto result = m_ChildNode->Tick(context);

    context.SetStackPointer(prevStack);

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimPlayer_Random::AnimPlayer_Random() :
    AnimPlayer_Pose(AnimGraph_NodeType::Random)
{
    m_PhaseFlags = UpdatePhaseFlags::Copy;
}

void AnimPlayer_Random::SelectPose()
{
    uint32_t index = rand() % m_Children.Size();
    m_SelectedPose = m_Children[index];
    m_PhaseCopySource = m_SelectedPose;
}

void AnimPlayer_Random::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    if (!m_SelectedPose)
        SelectPose();

    m_SelectedPose->UpdateDuration(context);
    m_Duration = m_SelectedPose->GetDuration();
}

uint32_t AnimPlayer_Random::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    float nextPhaseUnwrapped = m_SelectedPose->GetNextPhaseUnwrapped(context);
    if (nextPhaseUnwrapped > 1.0f)
    {
        SelectPose();
        m_SelectedPose->UpdateDuration(context);
    }

    auto result = m_SelectedPose->Tick(context);

    ApplyNextPhase(context);

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimPlayer_State::UpdateBreakpoints()
{
    m_Breakpoints.Clear();

    Vector<AnimPlayer_Value const*> conditions;// TODO: Use SmallVector or vector with temp allocator

    for (auto transition : m_OutputTransitionNodes)
    {
        auto conditionNode = transition->GetConditionNode();

        conditions.Add(conditionNode);
        conditionNode->CollectDescendants(conditions);

        for (auto condition : conditions)
        {
            if (condition->GetType() != AnimGraph_NodeType::StateCondition)
                continue;

            auto stateCondition = static_cast<AnimPlayer_StateCondition const*>(condition);

            float breakpoint = stateCondition->GetPhase();

            auto it = std::find(m_Breakpoints.begin(), m_Breakpoints.end(), breakpoint);
            if (it == m_Breakpoints.end())
            {
                m_Breakpoints.Add(breakpoint);
            }
        }
        conditions.Clear();
    }
    std::sort(m_Breakpoints.begin(), m_Breakpoints.end());
}

void AnimPlayer_State::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    m_PoseNode->UpdateDuration(context);
    m_Duration = m_PoseNode->GetDuration();
}

uint32_t AnimPlayer_State::Tick(AnimPlayerContext& context)
{
    //LOG("State: {}\n", m_Name);

    Super::Tick(context);

    auto result = m_PoseNode->Tick(context);

    ApplyNextPhase(context);

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimPlayer_StateMachine::AnimPlayer_StateMachine() :
    AnimPlayer_Pose(AnimGraph_NodeType::StateMachine)
{
    m_PhaseFlags = UpdatePhaseFlags::Copy;
}

void AnimPlayer_StateMachine::UpdatePhaseCopySource()
{
    if (m_CurrentNode->GetType() == AnimGraph_NodeType::StateTransition)
    {
        m_PhaseCopySource = static_cast<AnimPlayer_StateTransition*>(m_CurrentNode)->GetCurrentDestinationState();
    }
    else
    {
        m_PhaseCopySource = m_CurrentNode;
    }
}

bool AnimPlayer_StateMachine::UpdateState(AnimPlayerContext& context, float pendingPhase)
{
    if (m_CurrentNode->GetType() == AnimGraph_NodeType::State)
    {
        auto state = static_cast<AnimPlayer_State*>(m_CurrentNode);
        auto const& transitions = state->GetOutputTransitionNodes();

        //m_TransitionSourceCandidate = state;

        for (float breakpoint : state->GetBreakpoints())
        {
            if (breakpoint >= pendingPhase)
                break;

            m_TransitionSourceCandidatePhase = breakpoint;

            for (auto transition : transitions)
            {
                if (transition->IsConditionMet(context))
                {
                    m_TransitionSource = state;
                    m_CurrentNode = transition;
                    return true;
                }
            }
        }

        m_TransitionSourceCandidatePhase = pendingPhase;

        for (auto transition : transitions)
        {
            if (transition->IsConditionMet(context))
            {
                m_TransitionSource = state;
                m_CurrentNode = transition;
                return true;
            }
        }
    }
    else
    {
        auto transition = static_cast<AnimPlayer_StateTransition*>(m_CurrentNode);
        if (transition->IsFinished(pendingPhase))
        {
            m_CurrentNode = transition->GetCurrentDestinationState();
            return true;
        }
    }

    return false;
}

void AnimPlayer_StateMachine::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    s_ActiveStateMachineStack.Add(this);

    if (IsFirstPlay(context))
    {
        m_CurrentNode = m_StateNodes[0];
    }

    if (m_CurrentNode->GetType() == AnimGraph_NodeType::StateTransition || !context.IsSyncEnabled())
    {
        m_CurrentNode->UpdateDuration(context);

        float nextPhaseUnwrapped = m_CurrentNode->GetNextPhaseUnwrapped(context);
        if (UpdateState(context, nextPhaseUnwrapped))
        {
            // Transition is finished, update the new state duration
            m_CurrentNode->UpdateDuration(context);
        }
    }
    else
    {
        UpdateState(context, GetPhase());
        m_CurrentNode->UpdateDuration(context);
    }

    if (m_CurrentNode->GetType() == AnimGraph_NodeType::StateTransition)
    {
        auto destination = static_cast<AnimPlayer_StateTransition*>(m_CurrentNode)->GetCurrentDestinationState();
        m_Duration = destination->GetDuration();
    }
    else
    {
        m_Duration = m_CurrentNode->GetDuration();
    }

    UpdatePhaseCopySource();

    s_ActiveStateMachineStack.RemoveLast();
}

uint32_t AnimPlayer_StateMachine::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    s_ActiveStateMachineStack.Add(this);

    if (m_CurrentNode->GetType() == AnimGraph_NodeType::State && context.IsSyncEnabled())
    {
        auto& breakpoints = static_cast<AnimPlayer_State*>(m_CurrentNode)->GetBreakpoints();
        if (!breakpoints.IsEmpty())
        {
            UpdateState(context, GetPhase());
        }
    }

    uint32_t result = m_CurrentNode->Tick(context);

    UpdatePhaseCopySource();
    ApplyNextPhase(context);

    s_ActiveStateMachineStack.RemoveLast();

    return result;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimGraph_Value AnimPlayer_StateCondition::Compute(AnimPlayerContext& context)
{
    auto stateMachine = s_ActiveStateMachineStack.Last();
    HK_ASSERT(stateMachine);

    if (stateMachine->GetTransitionSourceCandidatePhase() >= GetPhase())
        return true;

    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimPlayer_StateTransition::AnimPlayer_StateTransition() :
    AnimPlayer_Pose(AnimGraph_NodeType::StateTransition)
{
    m_PhaseFlags = (UpdatePhaseFlags)0;
}

bool AnimPlayer_StateTransition::IsConditionMet(AnimPlayerContext& context) const
{
    return m_ConditionNode->Compute(context).GetBool();
}

bool AnimPlayer_StateTransition::IsFinished(float phase) const
{
    if (m_IsReversed)
        return phase <= 0.0f;
    return phase >= 1.0f;
}

void AnimPlayer_StateTransition::UpdateDuration(AnimPlayerContext& context)
{
    Super::UpdateDuration(context);

    if (IsFirstPlay(context))
    {
        HK_ASSERT(IsConditionMet(context));

        m_IsReversed = false;

        m_SavedPoseSourceSlotIndex = 0;
        m_SavedPoseSourcePhase = 0.0F;
    }

    bool switchReversedStatus{false};
    if (m_IsReversible)
    {
        const bool canContinue = IsConditionMet(context);
        switchReversedStatus = (m_IsReversed && canContinue) || (!m_IsReversed && !canContinue);
    }

    if (switchReversedStatus)
    {
        m_IsReversed = !m_IsReversed;

        m_SavedPoseSourceSlotIndex = (m_SavedPoseSourceSlotIndex + 1) & 1;
        m_SavedPoseSourcePhase = GetPhase();

        // TODO: when we reverse transition in any direction,
        // the state we're transitioning to will be reset to zero phase.
        // Is this a problem? Should we freeze its phase? Should we move its phase?
    }

    if (m_IsReversed)
        m_PhaseFlags |= UpdatePhaseFlags::Reversed;
    else
        m_PhaseFlags &= ~UpdatePhaseFlags::Reversed;

    // Update current source and destination states and update destination's duration

    auto stateMachine = s_ActiveStateMachineStack.Last();
    HK_ASSERT(stateMachine != nullptr);

    if (m_IsReversed)
    {
        m_CurrentSource = m_DestinationStateNode;
        m_CurrentDestination = stateMachine->GetTransitionSource();
    }
    else
    {
        m_CurrentSource = stateMachine->GetTransitionSource();
        m_CurrentDestination = m_DestinationStateNode;
    }

    m_CurrentDestination->UpdateDuration(context);
}


uint32_t AnimPlayer_StateTransition::Tick(AnimPlayerContext& context)
{
    Super::Tick(context);

    ApplyNextPhase(context);

    if (!m_IsSavedPoseSlotsAcquired)
    {
        m_IsSavedPoseSlotsAcquired = true;

        for (uint32_t i = 0; i < m_SavedPoseSlots.Size(); ++i)
            m_SavedPoseSlots[i] = context.AcquireSavedPoseSlot();
    }

    const uint32_t savedPoseSourceSlot = m_SavedPoseSlots[m_SavedPoseSourceSlotIndex];
    const uint32_t savedPoseTransitionSlot = m_SavedPoseSlots[(m_SavedPoseSourceSlotIndex + 1) & 1];

    const auto* const stateMachine = s_ActiveStateMachineStack.Last();
    HK_ASSERT(stateMachine != nullptr);

    if (IsFirstPlay(context))
    {
        // Play source state on phase we started the transition at
        // Remember result pose and use it during the transition for blending

        AnimPlayerStack* prevStack = context.GetStackPointer();
        AnimPlayerStack stack = *prevStack;

        stack.m_SyncEnabled = true;
        stack.m_SyncPhase = Math::Saturate(stateMachine->GetTransitionSourceCandidatePhase());

        context.SetStackPointer(&stack);

        uint32_t jobSourceID = m_CurrentSource->Tick(context);

        auto& jobBackup = context.AddJob<AnimJob_Backup>();

        jobBackup.m_SavedJobID = jobSourceID;
        jobBackup.m_SavedPoseIndex = savedPoseSourceSlot;

        context.SetStackPointer(prevStack);
    }

    // Restore saved pose (used as a blend source)

    auto& jobRestore = context.AddJob<AnimJob_Restore>();

    jobRestore.m_SavedPoseIndex = savedPoseSourceSlot;

    uint32_t jobRestoreID = context.GetCurrentJobID();

    // Compute destination pose (used as a blend destination)

    uint32_t jobDestinationPoseID = m_CurrentDestination->Tick(context);

    // Setup blending job

    const float blendPhaseCurrent = GetPhase();
    const float blendPhaseFrom = m_SavedPoseSourcePhase;
    const float blendPhaseDuration = m_IsReversed ? m_SavedPoseSourcePhase : 1.0f - m_SavedPoseSourcePhase;

    auto& jobBlend = context.AddJob<AnimJob_Blend>();
    jobBlend.m_ChildJobIDs[0] = jobRestoreID;
    jobBlend.m_ChildJobIDs[1] = jobDestinationPoseID;

    if (blendPhaseDuration == 0.0f)
    {
        jobBlend.m_Weight = 1.0f;
    }
    else
    {
        jobBlend.m_Weight = Math::Abs(blendPhaseCurrent - blendPhaseFrom) / blendPhaseDuration;
    }

    auto jobBlendID = context.GetCurrentJobID();

    // Remember transition pose resulted from this update
    auto& jobBackup = context.AddJob<AnimJob_Backup>();

    jobBackup.m_SavedJobID = jobBlendID;
    jobBackup.m_SavedPoseIndex = savedPoseTransitionSlot;

    return jobBlendID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimGraph_Value AnimPlayer_And::Compute(AnimPlayerContext& context)
{
    for (auto child : m_Children)
    {
        if (!child->Compute(context).GetBool())
            return false;
    }
    return true;
}

void AnimPlayer_And::CollectDescendants(Vector<AnimPlayer_Value const*>& descendants) const
{
    for (auto child : m_Children)
    {
        descendants.Add(child);
        child->CollectDescendants(descendants);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimGraph_Value AnimPlayer_Param::Compute(AnimPlayerContext& context)
{
    return context.GetParam(m_ParamID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AnimGraph_Value AnimPlayer_ParamComparison::Compute(AnimPlayerContext& context)
{
    bool result = false;
    switch (m_Op)
    {
    case AnimGraph_ParamComparison::Op::Equal:
        result = context.GetParam(m_ParamID) == m_Value;
        break;
    case AnimGraph_ParamComparison::Op::NotEqual:
        result = context.GetParam(m_ParamID) != m_Value;
        break;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct AnimationPlayer::BuildContext
{
    template <typename T>
    T* GetNode(uint32_t id)
    {
        auto node = m_Nodes[id].RawPtr();
        HK_ASSERT(node->GetType() == T::TYPE);
        return static_cast<T*>(node);
    }

    AnimPlayer_Pose* GetPoseNode(uint32_t id)
    {
        auto node = m_Nodes[id].RawPtr();
        HK_ASSERT(node && node->GetType() <= AnimGraph_NodeType::StateTransition);
        return static_cast<AnimPlayer_Pose*>(node);
    }

    AnimPlayer_Value* GetValueNode(uint32_t id)
    {
        auto node = m_Nodes[id].RawPtr();
        HK_ASSERT(node && node->GetType() > AnimGraph_NodeType::StateTransition);
        return static_cast<AnimPlayer_Value*>(node);
    }

    Vector<UniqueRef<AnimPlayer_Node>>& m_Nodes;
};

AnimationPlayer::AnimationPlayer(AnimationGraph_Cooked* animGraph, OzzSkeleton const* skeleton)
{
    m_AnimGraph = animGraph;
    // TODO: resourceMngr.AddRef(animGraph), in destructor call resourceMngr.RemoveRef to prevent resource unloading!!!
    m_Skeleton = skeleton;

    auto& graphNodes = animGraph->GetNodes();
    if (graphNodes.IsEmpty())
        return;

    BuildContext context{m_Nodes};

    m_Nodes.Reserve(graphNodes.Size());

    for (auto& node : graphNodes)
    {
        switch (AnimGraph_NodeType(node.NodeHeader.Type))
        {
        case AnimGraph_NodeType::Clip:
            m_Nodes.EmplaceBack(new AnimPlayer_Clip);
            break;
        case AnimGraph_NodeType::Blend:
            m_Nodes.EmplaceBack(new AnimPlayer_Blend);
            break;
        case AnimGraph_NodeType::Sum:
            m_Nodes.EmplaceBack(new AnimPlayer_Sum);
            break;
        case AnimGraph_NodeType::And:
            m_Nodes.EmplaceBack(new AnimPlayer_And);
            break;
        case AnimGraph_NodeType::Param:
            m_Nodes.EmplaceBack(new AnimPlayer_Param);
            break;
        case AnimGraph_NodeType::ParamComparison:
            m_Nodes.EmplaceBack(new AnimPlayer_ParamComparison);
            break;
        case AnimGraph_NodeType::Playback:
            m_Nodes.EmplaceBack(new AnimPlayer_Playback);
            break;
        case AnimGraph_NodeType::Random:
            m_Nodes.EmplaceBack(new AnimPlayer_Random);
            break;
        case AnimGraph_NodeType::State:
            m_Nodes.EmplaceBack(new AnimPlayer_State);
            break;
        case AnimGraph_NodeType::StateMachine:
            m_Nodes.EmplaceBack(new AnimPlayer_StateMachine);
            break;
        case AnimGraph_NodeType::StateCondition:
            m_Nodes.EmplaceBack(new AnimPlayer_StateCondition);
            break;
        case AnimGraph_NodeType::StateTransition:
            m_Nodes.EmplaceBack(new AnimPlayer_StateTransition);
            break;
        }
    }

    m_Root = context.GetPoseNode(animGraph->GetRootNodeID());

    for (uint16_t id = 0, count = graphNodes.Size(); id < count; ++id)
    {
        CreatePlayerNode(context, id);
    }

    for (auto& node : m_Nodes)
    {
        if (node->GetType() == AnimGraph_NodeType::State)
        {
            static_cast<AnimPlayer_State*>(node.RawPtr())->UpdateBreakpoints();
        }
    }
}

void AnimationPlayer::CreatePlayerNode(BuildContext& context, uint16_t id) const
{
    AnimationGraph_Cooked::Node const* node = &m_AnimGraph->GetNodes()[id];

    switch (AnimGraph_NodeType(node->NodeHeader.Type))
    {
    case AnimGraph_NodeType::Clip:
    {
        auto& resourceMngr = GameApplication::sGetResourceManager();
        auto player = context.GetNode<AnimPlayer_Clip>(id);
        player->m_AnimClip = resourceMngr.GetResource<AnimationResource>(&m_AnimGraph->GetClips()[node->NodeClip.ClipIDOffset]);
        break;
    }
    case AnimGraph_NodeType::Blend:
    {
        auto& blend = node->NodeBlend;
        auto player = context.GetNode<AnimPlayer_Blend>(id);

        player->m_PoseNodes.Reserve(blend.NumBlendPoses);
        for (uint16_t i = 0; i < blend.NumBlendPoses; ++i)
        {
            uint16_t index = blend.FirstBlendPose + i;
            player->m_PoseNodes.EmplaceBack(context.GetPoseNode(m_AnimGraph->GetBlendPoses()[index].ID), m_AnimGraph->GetBlendPoses()[index].Factor);
        }

        player->m_FactorNode = context.GetValueNode(blend.FactorNodeID);
        break;
    }
    case AnimGraph_NodeType::Sum:
    {
        auto& sum = node->NodeSum;
        auto player = context.GetNode<AnimPlayer_Sum>(id);

        player->m_FirstNode = context.GetPoseNode(sum.FirstNodeID);
        player->m_SecondNode = context.GetPoseNode(sum.SecondNodeID);
        break;
    }
    case AnimGraph_NodeType::And:
    {
        auto& logicAnd = node->NodeAnd;
        auto player = context.GetNode<AnimPlayer_And>(id);

        player->m_Children.Reserve(logicAnd.NumNodes);
        for (uint16_t i = 0; i < logicAnd.NumNodes; ++i)
        {
            uint16_t index = logicAnd.FirstNode + i;
            player->m_Children.Add(context.GetValueNode(m_AnimGraph->GetNodeIDs()[index]));
        }
        break;
    }
    case AnimGraph_NodeType::Param:
    {
        auto player = context.GetNode<AnimPlayer_Param>(id);
        player->m_ParamID.FromString(&m_AnimGraph->GetParamIDs()[node->NodeParam.ParamIDOffset]);
        break;
    }
    case AnimGraph_NodeType::ParamComparison:
    {
        auto& paramComparison = node->NodeParamComparison;
        auto player = context.GetNode<AnimPlayer_ParamComparison>(id);

        player->m_ParamID.FromString(&m_AnimGraph->GetParamIDs()[paramComparison.ParamIDOffset]);
        player->m_Op = AnimGraph_ParamComparison::Op(paramComparison.Op);
        player->m_Value = paramComparison.Value;
        break;
    }
    case AnimGraph_NodeType::Playback:
    {
        auto& playback = node->NodePlayback;
        auto player = context.GetNode<AnimPlayer_Playback>(id);

        player->m_SpeedProviderNode = context.GetValueNode(playback.SpeedProviderNodeID);
        player->m_ChildNode = context.GetPoseNode(playback.ChildNodeID);
        break;
    }
    case AnimGraph_NodeType::Random:
    {
        auto& random = node->NodeRandom;
        auto player = context.GetNode<AnimPlayer_Random>(id);

        player->m_Children.Reserve(random.NumNodes);
        for (uint16_t i = 0; i < random.NumNodes; ++i)
        {
            uint16_t index = random.FirstNode + i;
            player->m_Children.Add(context.GetPoseNode(m_AnimGraph->GetNodeIDs()[index]));
        }
        break;
    }
    case AnimGraph_NodeType::State:
    {
        auto& state = node->NodeState;
        auto player = context.GetNode<AnimPlayer_State>(id);

        player->m_PoseNode = context.GetPoseNode(state.PoseNodeID);
        player->m_Name = &m_AnimGraph->GetNames()[state.NameOffset];

        player->m_OutputTransitionNodes.Reserve(state.NumOutputTransitionNodes);
        for (uint16_t i = 0; i < state.NumOutputTransitionNodes; ++i)
        {
            uint16_t index = state.FirstOutputTransitionNode + i;
            player->m_OutputTransitionNodes.Add(context.GetNode<AnimPlayer_StateTransition>(m_AnimGraph->GetNodeIDs()[index]));
        }
        break;
    }
    case AnimGraph_NodeType::StateMachine:
    {
        auto& stateMachine = node->NodeStateMachine;
        auto player = context.GetNode<AnimPlayer_StateMachine>(id);

        player->m_StateNodes.Reserve(stateMachine.NumStateNodes);
        for (uint16_t i = 0; i < stateMachine.NumStateNodes; ++i)
        {
            uint16_t index = stateMachine.FirstStateNode + i;
            player->m_StateNodes.Add(context.GetNode<AnimPlayer_State>(m_AnimGraph->GetNodeIDs()[index]));
        }
        break;
    }
    case AnimGraph_NodeType::StateCondition:
    {
        auto player = context.GetNode<AnimPlayer_StateCondition>(id);

        player->m_Phase = node->NodeStateCondition.Phase;
        break;
    }
    case AnimGraph_NodeType::StateTransition:
    {
        auto& stateTransition = node->NodeStateTransition;
        auto player = context.GetNode<AnimPlayer_StateTransition>(id);

        player->m_ConditionNode = context.GetValueNode(stateTransition.ConditionNodeID);
        player->m_DestinationStateNode = context.GetNode<AnimPlayer_State>(stateTransition.DestinationNodeID);
        player->m_Duration = stateTransition.Duration;
        player->m_TransitionType = AnimGraph_StateTransition::TransitionType(stateTransition.TransType);
        player->m_IsReversible = stateTransition.IsReversible;
        break;
    }
    default:
        HK_ASSERT(0);
        break;
    }
}

Vector<SoaTransform*> PoseAllocator;

struct AnimationMixerContext
{
    ozz::animation::Skeleton const* Skeleton;
    uint32_t SoaJointCount;

    SoaTransform* AllocatePose()
    {
        // TODO: Use frame memory
        PoseAllocator.Add(new SoaTransform[SoaJointCount]);
        return PoseAllocator.Last();
    }
};

void Sample(AnimationMixerContext& context, AnimationSampleContext* samplingContext, SoaTransform* pose, AnimationHandle animClip, float phase)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (AnimationResource* animation = resourceMngr.TryGet(animClip))
    {
        if (static_cast<uint32_t>(samplingContext->max_soa_tracks()) != context.SoaJointCount)
            samplingContext->Resize(context.Skeleton->num_joints());

        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = animation->GetImpl();
        samplingJob.context = samplingContext;
        samplingJob.ratio = phase;
        samplingJob.output = ozz::span{pose, context.SoaJointCount};
        samplingJob.Run();
    }
    else
    {
        Core::Memcpy(pose, context.Skeleton->joint_rest_poses().data(), context.SoaJointCount * sizeof(pose[0]));
    }
}

void Blend(AnimationMixerContext& context, SoaTransform* pose, SoaTransform* source, SoaTransform* destination, float blendWeight)
{
    ozz::animation::BlendingJob blendingJob;
    ozz::animation::BlendingJob::Layer layers[2];

    layers[0].weight = 1.0f - blendWeight;
    layers[0].transform = ozz::span{source, context.SoaJointCount};

    layers[1].weight = blendWeight;
    layers[1].transform = ozz::span{destination, context.SoaJointCount};

    blendingJob.layers = layers;
    blendingJob.output = ozz::span{pose, context.SoaJointCount};
    blendingJob.rest_pose = context.Skeleton->joint_rest_poses();

    blendingJob.Run();
}

void Sum(AnimationMixerContext& context, SoaTransform* pose, SoaTransform* poseA, SoaTransform* poseB)
{
    ozz::animation::BlendingJob blendingJob;
    ozz::animation::BlendingJob::Layer layers[2];

    layers[0].weight = 1;
    layers[0].transform = ozz::span{poseA, context.SoaJointCount};

    layers[1].weight = 1;
    layers[1].transform = ozz::span{poseB, context.SoaJointCount};

    blendingJob.additive_layers = layers;
    blendingJob.output = ozz::span{pose, context.SoaJointCount};
    blendingJob.rest_pose = context.Skeleton->joint_rest_poses();

    blendingJob.Run();
}

void Copy(AnimationMixerContext& context, SoaTransform* pose, SoaTransform* source)
{
    Core::Memcpy(pose, source, context.SoaJointCount * sizeof(pose[0]));
}

void AnimationPlayer::Tick(float timeStep, AnimationParameterSet* parameterSet, SkeletonPose* resultPose)
{
    //LOG("---------------------------------------\n");

    HK_ASSERT(s_ActiveStateMachineStack.IsEmpty());

    AnimPlayerStack stack;
    stack.m_Speed = timeStep;

    m_Context.m_ParameterSet = parameterSet;
    m_Context.SetStackPointer(&stack);
    m_Context.m_JobQueue.Clear();

    uint32_t jobFinalID = m_Root->Tick(m_Context);

    m_Context.SetStackPointer(nullptr);

    m_Context.m_TickIndex++;
    if (m_Context.m_TickIndex == ~0u)
        m_Context.m_TickIndex = 0;

    uint32_t savedPoseSlotCount = m_Context.GetSavedPoseSlotCount();
    if (m_SavedPoseSlots.Size() < savedPoseSlotCount)
    {
        m_SavedPoseSlots.Reserve(savedPoseSlotCount);

        while (m_SavedPoseSlots.Size() < savedPoseSlotCount)
        {
            m_SavedPoseSlots.EmplaceBack();
            auto& slot = m_SavedPoseSlots.Last();
            slot.Pose.reset(new SoaTransform[m_Skeleton->num_soa_joints()]);
        }
    }

    AnimationMixerContext mixerContext;
    mixerContext.Skeleton = m_Skeleton;
    mixerContext.SoaJointCount = m_Skeleton->num_soa_joints();

    uint32_t jobID = 0;
    for (auto& job : m_Context.m_JobQueue)
    {
        switch (job->GetType())
        {
            case AnimJobType::Sample:
            {
                auto clip = static_cast<AnimJob_Sample*>(job.RawPtr());
                //LOG("{} Job [Sample]: {} phase {})\n", jobID, clip->m_Clip, clip->m_Phase);

                clip->Pose = mixerContext.AllocatePose();
                Sample(mixerContext, clip->m_SamplingContext.get(), clip->Pose, clip->m_Clip, clip->m_Phase);
                break;
            }
            case AnimJobType::Blend:
            {
                auto blend = static_cast<AnimJob_Blend*>(job.RawPtr());
                //LOG("{} Job [Blend]: weight {} poses: {} {}\n", jobID, blend->m_Weight, blend->m_ChildJobIDs[0], blend->m_ChildJobIDs[1]);

                blend->Pose = mixerContext.AllocatePose();
                Blend(mixerContext, blend->Pose, m_Context.m_JobQueue[blend->m_ChildJobIDs[0]]->Pose, m_Context.m_JobQueue[blend->m_ChildJobIDs[1]]->Pose, blend->m_Weight);
                break;
            }
            case AnimJobType::Sum:
            {
                auto sum = static_cast<AnimJob_Sum*>(job.RawPtr());
                //LOG("{} Job [Sum]: poses: {} {}\n", jobID, sum->m_ChildJobIDs[0], sum->m_ChildJobIDs[1]);

                sum->Pose = mixerContext.AllocatePose();
                Sum(mixerContext, sum->Pose, m_Context.m_JobQueue[sum->m_ChildJobIDs[0]]->Pose, m_Context.m_JobQueue[sum->m_ChildJobIDs[1]]->Pose);
                break;
            }
            case AnimJobType::Backup:
            {
                auto save = static_cast<AnimJob_Backup*>(job.RawPtr());
                //LOG("{} Job [Backup]: pose index {} saved job id {}\n", jobID, save->m_SavedPoseIndex, save->m_SavedJobID);

                save->Pose = m_SavedPoseSlots[save->m_SavedPoseIndex].Pose.get();
                Copy(mixerContext, save->Pose, m_Context.m_JobQueue[save->m_SavedJobID]->Pose);
                break;
            }
            case AnimJobType::Restore:
            {
                auto restore = static_cast<AnimJob_Restore*>(job.RawPtr());
                //LOG("{} Job [Restore]: pose index {}\n", jobID, restore->m_SavedPoseIndex);

                restore->Pose = mixerContext.AllocatePose();
                Copy(mixerContext, restore->Pose, m_SavedPoseSlots[restore->m_SavedPoseIndex].Pose.get());
                break;
            }
        }
        jobID++;
    }

    //LOG("Result job {}\n", jobFinalID);

    SoaTransform* finalPose = m_Context.m_JobQueue[jobFinalID]->Pose;

    if (resultPose->m_LocalMatrices.Size() != mixerContext.SoaJointCount)
        resultPose->m_LocalMatrices.Resize(mixerContext.SoaJointCount);

    Core::Memcpy(resultPose->m_LocalMatrices.ToPtr(), finalPose, mixerContext.SoaJointCount * sizeof(SoaTransform));

    for (auto pose : PoseAllocator)
        delete pose;

    PoseAllocator.Clear();
}

HK_NAMESPACE_END
