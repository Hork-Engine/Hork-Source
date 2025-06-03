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

#include "AnimJob.h"

#include <Hork/AnimGraph/AnimGraph.h>

#include <Hork/AnimGraph/AnimGraph_Cooked.h>

#include <Hork/Core/Containers/Array.h>
#include <Hork/Core/Containers/Hash.h>

#include <ozz/animation/runtime/skeleton.h> // TODO: move to cpp

HK_NAMESPACE_BEGIN

using OzzSkeleton = ozz::animation::Skeleton;

class AnimationParameterSet
{
public:
    HashMap<StringID, AnimGraph_Value> m_Params;

    //template <typename T>
    //void                    SetParam(StringID paramID, T value) { m_Params[paramID] = AnimGraph_Value{static_cast<float>(value)}; }

    //AnimGraph_Value         GetParam(StringID paramID) { return m_Params[paramID]; }
};

struct AnimPlayerStack
{
    float                   m_Speed;
    bool                    m_SyncEnabled = false;
    float                   m_SyncPhase = 0;
};

struct AnimPlayerContext
{
    friend class            AnimationPlayer;

    void                    SetStackPointer(AnimPlayerStack* stack) { m_pStack = stack; }

    AnimPlayerStack*        GetStackPointer() { return m_pStack; }

    float                   GetSpeed() { return m_pStack->m_Speed; }

    AnimGraph_Value         GetParam(StringID paramID) { return m_ParameterSet->m_Params[paramID]; }

    bool                    IsSyncEnabled() const { return m_pStack->m_SyncEnabled; }

    float                   GetSyncPhase() const { return m_pStack->m_SyncPhase; }

    template <typename T>
    T&                      AddJob() { T* job = new T; m_JobQueue.EmplaceBack(job); return *job; }

    uint32_t                GetCurrentJobID() const { return m_JobQueue.Size() - 1; }

    uint32_t                AcquireSavedPoseSlot() { return m_SavedPoseSlot++; /* TODO */ }

    uint32_t                GetTickIndex() const { return m_TickIndex; }

    uint32_t                GetSavedPoseSlotCount() const { return m_SavedPoseSlot; }

private:
    AnimationParameterSet*  m_ParameterSet = nullptr;
    AnimPlayerStack*        m_pStack = nullptr;
    Vector<UniqueRef<AnimJob>>  m_JobQueue;
    uint32_t                m_TickIndex{};
    uint32_t                m_SavedPoseSlot{};
};

class AnimPlayer_Node : public Noncopyable
{
public:
    virtual                 ~AnimPlayer_Node() = default;

    AnimGraph_NodeType      GetType() const { return m_Type; }

protected:
                            AnimPlayer_Node(AnimGraph_NodeType type) : m_Type(type) {}

private:
    AnimGraph_NodeType      m_Type;
};

class AnimPlayer_Value : public AnimPlayer_Node
{
public:
    virtual AnimGraph_Value Compute(AnimPlayerContext& context) = 0;

    virtual void            CollectDescendants(Vector<AnimPlayer_Value const*>& descendants) const = 0;

protected:
                            AnimPlayer_Value(AnimGraph_NodeType type) : AnimPlayer_Node(type) {}
};

class AnimPlayer_And final : public AnimPlayer_Value
{
    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::And;

    virtual AnimGraph_Value Compute(AnimPlayerContext& context) override;

    virtual void            CollectDescendants(Vector<AnimPlayer_Value const*>& descendants) const override;

private:
                            AnimPlayer_And() : AnimPlayer_Value(AnimGraph_NodeType::And) {}

    Vector<AnimPlayer_Value*> m_Children;
};

class AnimPlayer_Param final : public AnimPlayer_Value
{
    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Param;

    virtual AnimGraph_Value Compute(AnimPlayerContext& context) override;

    virtual void            CollectDescendants(Vector<AnimPlayer_Value const*>& descendants) const override {}

private:
                            AnimPlayer_Param() : AnimPlayer_Value(AnimGraph_NodeType::Param) {}

    StringID                m_ParamID;
};

class AnimPlayer_ParamComparison final : public AnimPlayer_Value
{
    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::ParamComparison;

    using Op =              AnimGraph_ParamComparison::Op;

    virtual AnimGraph_Value Compute(AnimPlayerContext& context) override;

    virtual void            CollectDescendants(Vector<AnimPlayer_Value const*>& descendants) const override {}

private:
                            AnimPlayer_ParamComparison() : AnimPlayer_Value(AnimGraph_NodeType::ParamComparison) {}

    StringID                m_ParamID;
    Op                      m_Op = Op::Equal;
    AnimGraph_Value         m_Value;
};

class AnimPlayer_StateCondition final : public AnimPlayer_Value
{
    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::StateCondition;

    float                   GetPhase() const { return m_Phase; }

    virtual AnimGraph_Value Compute(AnimPlayerContext& context) override;

    virtual void            CollectDescendants(Vector<AnimPlayer_Value const*>& descendants) const override {}

private:
                            AnimPlayer_StateCondition() : AnimPlayer_Value(AnimGraph_NodeType::StateCondition) {}

    float                   m_Phase;
};

enum class UpdatePhaseFlags : int
{
    Copy = 1,
    Wrap = 2,
    Reversed = 4,
    Sync = 8
};

HK_FLAG_ENUM_OPERATORS(UpdatePhaseFlags);

class AnimPlayer_Pose : public AnimPlayer_Node
{
public:
    float                   GetPhase() const { return m_Phase; }
    float                   GetDuration() const { return m_Duration; }

    virtual void            UpdateDuration(AnimPlayerContext& context);
    virtual uint32_t        Tick(AnimPlayerContext& context);

    float                   GetNextPhaseUnwrapped(AnimPlayerContext& context);

protected:
                            AnimPlayer_Pose(AnimGraph_NodeType type) : AnimPlayer_Node(type) {}

private:
    float                   m_Phase = std::numeric_limits<float>::quiet_NaN();

protected:
    void                    ApplyNextPhase(AnimPlayerContext& context);
    bool                    IsFirstPlay(AnimPlayerContext& context) const;

    float                   m_Duration;
    UpdatePhaseFlags        m_PhaseFlags = UpdatePhaseFlags::Wrap | UpdatePhaseFlags::Sync;
    AnimPlayer_Pose*        m_PhaseCopySource = nullptr;

private:
    void                    UpdateTickIndex(AnimPlayerContext& context) const;

    mutable uint32_t        m_LastTick = ~0u;
    mutable bool            m_IsFirstPlay = false;

    mutable uint32_t        m_LastDurationUpdateTick = ~0u;
};


class AnimPlayer_Clip final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Clip;

    virtual uint32_t        Tick(AnimPlayerContext& context) override;

private:
                            AnimPlayer_Clip() : AnimPlayer_Pose(AnimGraph_NodeType::Clip) {}

    AnimationHandle         m_AnimClip;

    std::shared_ptr<AnimationSampleContext> m_SamplingContext;
};

class AnimPlayer_Blend final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Blend;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

private:
                            AnimPlayer_Blend() : AnimPlayer_Pose(AnimGraph_NodeType::Blend) {}

    void                    SelectPoses(AnimPlayerContext& context);

    struct PoseNode
    {
        AnimPlayer_Pose*    Pose;
        float               Factor;

        PoseNode(AnimPlayer_Pose* pose, float factor) : Pose(pose), Factor(factor)
        {}
    };

    Vector<PoseNode>        m_PoseNodes;
    AnimPlayer_Value*       m_FactorNode{};

    AnimPlayer_Pose*        m_CurPose{};
    AnimPlayer_Pose*        m_NextPose{};
    float                   m_Weight{};
    float                   m_PrevFactor = std::numeric_limits<float>::quiet_NaN();
};

class AnimPlayer_Sum final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Sum;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

private:
                            AnimPlayer_Sum() : AnimPlayer_Pose(AnimGraph_NodeType::Sum) {}

    AnimPlayer_Pose*        m_FirstNode;
    AnimPlayer_Pose*        m_SecondNode;
};

class AnimPlayer_Playback final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Playback;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

private:
                            AnimPlayer_Playback() : AnimPlayer_Pose(AnimGraph_NodeType::Playback) {}

    AnimPlayer_Value*       m_SpeedProviderNode;
    AnimPlayer_Pose*        m_ChildNode;
};

class AnimPlayer_Random final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::Random;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

private:
                            AnimPlayer_Random();

    void                    SelectPose();

    Vector<AnimPlayer_Pose*> m_Children;
    AnimPlayer_Pose*        m_SelectedPose = nullptr;
};

class AnimPlayer_StateTransition;

class AnimPlayer_State final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::State;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

    void                    UpdateBreakpoints();
    Vector<float> const&    GetBreakpoints() const { return m_Breakpoints; }

    Vector<AnimPlayer_StateTransition*> const& GetOutputTransitionNodes() const { return m_OutputTransitionNodes; }

private:
                            AnimPlayer_State() : AnimPlayer_Pose(AnimGraph_NodeType::State) {}

    AnimPlayer_Pose*        m_PoseNode;
    const char*             m_Name;
    Vector<AnimPlayer_StateTransition*> m_OutputTransitionNodes;
    Vector<float>           m_Breakpoints;
};

class AnimPlayer_StateMachine final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::StateMachine;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

    AnimPlayer_State*       GetTransitionSource() { return m_TransitionSource; }
    float                   GetTransitionSourceCandidatePhase() const { return m_TransitionSourceCandidatePhase; }

private:
                            AnimPlayer_StateMachine();

    bool                    UpdateState(AnimPlayerContext& context, float pendingPhase);
    void                    UpdatePhaseCopySource();

    Vector<AnimPlayer_State*> m_StateNodes;
    AnimPlayer_Pose*        m_CurrentNode = nullptr;
    AnimPlayer_State*       m_TransitionSource = nullptr;
    float                   m_TransitionSourceCandidatePhase = 0;
};

class AnimPlayer_StateTransition final : public AnimPlayer_Pose
{
    using Super =           AnimPlayer_Pose;

    friend class            AnimationPlayer;

public:
    static constexpr AnimGraph_NodeType TYPE = AnimGraph_NodeType::StateTransition;

    bool                    IsConditionMet(AnimPlayerContext& context) const;
    bool                    IsFinished(float phase) const;

    virtual void            UpdateDuration(AnimPlayerContext& context) override;
    virtual uint32_t        Tick(AnimPlayerContext& context) override;

    AnimPlayer_Value const* GetConditionNode() const { return m_ConditionNode; }
    AnimPlayer_State*       GetCurrentDestinationState() { return m_CurrentDestination; }

private:
                            AnimPlayer_StateTransition();

    AnimPlayer_Value*       m_ConditionNode;
    AnimPlayer_State*       m_DestinationStateNode;
    AnimGraph_StateTransition::TransitionType m_TransitionType;
    bool                    m_IsReversible;
    bool                    m_IsReversed = false;
    AnimPlayer_State*       m_CurrentSource = nullptr;
    AnimPlayer_State*       m_CurrentDestination = nullptr;
    bool                    m_IsSavedPoseSlotsAcquired = false;
    Array<uint32_t, 2>      m_SavedPoseSlots;
    uint32_t                m_SavedPoseSourceSlotIndex{0};
    float                   m_SavedPoseSourcePhase{0.0f};
};

class AnimationPlayer final : public Noncopyable
{
public:
    explicit                AnimationPlayer(AnimationGraph_Cooked* animGraph, OzzSkeleton const* skeleton);

    void                    Tick(float timeStep, AnimationParameterSet* parameterSet, class SkeletonPose* resultPose);

    AnimationGraph_Cooked*  GetGraph() const { return m_AnimGraph; }

private:
    struct BuildContext;
    void                    CreatePlayerNode(BuildContext& context, uint16_t id) const;

    AnimPlayer_Pose*        m_Root;
    Vector<UniqueRef<AnimPlayer_Node>> m_Nodes;
    AnimPlayerContext       m_Context;

    struct SavedPose
    {
        std::unique_ptr<SoaTransform[]> Pose;
    };

    Vector<SavedPose>       m_SavedPoseSlots;
    Ref<AnimationGraph_Cooked> m_AnimGraph;
    OzzSkeleton const*      m_Skeleton; // TODO: Ref count
};

HK_NAMESPACE_END
