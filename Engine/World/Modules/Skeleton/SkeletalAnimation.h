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

#include <Engine/World/Resources/Resource_Skeleton.h>

HK_NAMESPACE_BEGIN

struct SkeletalAnimationTrack : RefCounted
{
    enum PLAYBACK_MODE
    {
        PLAYBACK_WRAP,
        PLAYBACK_MIRROR,
        PLAYBACK_CLAMP
    };

    SkeletalAnimationTrack(StringView animation);

    String const& GetAnimation() const;

    void SetPlaybackMode(PLAYBACK_MODE mode);
    PLAYBACK_MODE GetPlaybackMode() const;

    void SetQuantizer(float quantizer);
    float GetQuantizer() const;

private:
    String m_Animation;
    float m_Quantizer{};
    PLAYBACK_MODE m_PlaybackMode{PLAYBACK_CLAMP};
};

class AnimationInstance;

//enum ANIMATION_STATE_FLAGS
//{
//    ANIMATION_STATE_DEFAULT = 0,
//    ANIMATION_STATE_PLAY_ONCE = 1
//};
//
//HK_FLAG_ENUM_OPERATORS(ANIMATION_STATE_FLAGS)

class AnimationBlendMachine : public RefCounted
{
public:
    template <typename T>
    struct Handle
    {
        Handle() = default;
        explicit Handle(uint32_t id) :
            Id(id)
        {}

        bool IsValid() const
        {
            return Id != 0;
        }

        bool operator==(Handle<T> const& rhs) const
        {
            return Id == rhs.Id;
        }

        bool operator!=(Handle<T> const& rhs) const
        {
            return Id != rhs.Id;
        }

        uint32_t GetId() const
        {
            return Id;
        }

    private:
        uint32_t Id{};
    };

    enum NODE_TYPE
    {
        NODE_ANIM,
        NODE_BLEND
    };

    struct Node
    {
        NODE_TYPE Type;
        Ref<SkeletalAnimationTrack> Track;
    };

    struct BlendPose
    {
        Handle<Node> AnimNode;
        float Weight;

        BlendPose() = default;

        BlendPose(Handle<Node> anim, float weight) :
            AnimNode(anim),
            Weight(weight)
        {}
    };

    struct BlendNode : Node
    {
        Vector<BlendPose> BlendPoses;
    };

    struct State
    {
        String Name;
        Handle<AnimationBlendMachine::Node> Node;
        //ANIMATION_STATE_FLAGS Flags;
    };

    struct Transition
    {
        String Name;
        uint64_t Key;
        float Time;
        float SyncFactor;

        static uint64_t MakeKey(Handle<State> state1, Handle<State> state2)
        {
            return (uint64_t(state1.GetId()) << 32) | state2.GetId();
        }
    };

    using NodeHandle = Handle<Node>;
    using StateHandle = Handle<State>;

    struct LayerData;
    struct Layer
    {
        friend struct LayerData;

        Layer(StringView name) :
            m_Name(name)
        {}

        ~Layer()
        {
            for (auto* node : m_Nodes)
                delete node;
            for (auto* state : m_States)
                delete state;
            for (auto* tr : m_Transitions)
                delete tr;
        }

        StringView GetName() const
        {
            return m_Name;
        }

        NodeHandle AddNode(SkeletalAnimationTrack* track)
        {
            Node* node = new Node;
            node->Type = NODE_ANIM;
            node->Track = track;
            m_Nodes.Add(node);
            return NodeHandle(m_Nodes.Size());
        }

        NodeHandle AddBlendNode(std::initializer_list<BlendPose> blendPoses)
        {
            BlendNode* node = new BlendNode;
            node->Type = NODE_BLEND;
            node->BlendPoses = blendPoses;
            m_Nodes.Add(node);
            return NodeHandle(m_Nodes.Size());
        }

        StateHandle AddState(StringView name, NodeHandle node/*, ANIMATION_STATE_FLAGS flags = ANIMATION_STATE_DEFAULT*/)
        {
            State* state = new State;
            state->Name = name;
            state->Node = node;
            //state->Flags = flags;
            m_States.Add(state);
            return StateHandle(m_States.Size());
        }

        // Sync factor = state2.duration / state1.duration
        void AddTransition(StringView name, StateHandle state1, StateHandle state2, float time, float syncFactor = 0)
        {
            Transition* tr = new Transition;
            tr->Name = name;
            tr->Key = Transition::MakeKey(state1, state2);
            tr->Time = time;
            tr->SyncFactor = syncFactor;
            m_Transitions.Add(tr);
        }

        StateHandle FindState(StringView name) const
        {
            uint32_t id{};
            for (auto* state : m_States)
            {
                id++;
                if (state->Name == name)
                    return StateHandle(id);
            }
            return {};
        }

    private:
        Node const* GetNode(NodeHandle handle) const
        {
            return m_Nodes[handle.GetId() - 1];
        }

        State const* GetState(StateHandle handle) const
        {
            return m_States[handle.GetId() - 1];
        }

        Transition* FindTransition(StateHandle state1, StateHandle state2) const
        {
            uint64_t key = Transition::MakeKey(state1, state2);
            for (auto& tr : m_Transitions)
            {
                if (tr->Key == key)
                    return tr;
            }
            return nullptr;
        }

        void ProcessNode(Node const* node, float weight, float position, SkeletonResource const* skeleton, SkeletonPose* pose) const
        {
            if (weight < std::numeric_limits<float>::epsilon())
                return;

            if (node->Type == NODE_BLEND)
            {
                auto blendNode = static_cast<BlendNode const*>(node);
                for (auto const& blendPose : blendNode->BlendPoses)
                {
                    ProcessNode(GetNode(blendPose.AnimNode), weight * blendPose.Weight, position, skeleton, pose);
                }
            }
            else
            {
                SampleAnimationTrack(skeleton, node->Track, weight, position, pose);
            }
        }

        void SampleAnimationTrack(SkeletonResource const* skeleton, SkeletalAnimationTrack const* track, float weight, float position, SkeletonPose* pose) const;

        String m_Name;

        Vector<Node*> m_Nodes;
        Vector<State*> m_States;
        Vector<Transition*> m_Transitions;
    };

    struct LayerData
    {
        void SetDefaultTransitionTime(float transitionTime)
        {
            m_DefaultTransitionTime = transitionTime;
        }

        void SetState(StringView name)
        {
            StateHandle state = m_Layer->FindState(name);
            if (state.IsValid())
                SetState(state);
        }

        bool ChangeState(StringView name)
        {
            StateHandle state = m_Layer->FindState(name);
            if (state.IsValid())
                return ChangeState(state);
            return false;
        }

        void SetState(StateHandle state)
        {
            m_CurrentState = state;
            m_TransitionState = state;
            m_TransitionTime = 0;
            m_CurTransitionTime = 0;
            m_PlaybackPosition[0] = 0;
            m_PlaybackPosition[1] = 0;
        }

        bool ChangeState(StateHandle newState)
        {
            if (!m_CurrentState.IsValid())
            {
                SetState(newState);
                return true;
            }

            if (newState == m_TransitionState)
                return true;

            auto* tr = m_Layer->FindTransition(m_CurrentState, newState);
            if (tr)
            {
                m_TransitionState = newState;
                m_TransitionTime = tr->Time;
                m_SyncFactor = tr->SyncFactor;
                if (m_SyncFactor)
                {
                    // Synchronize playback position
                    m_PlaybackPosition[1] = m_PlaybackPosition[0] * m_SyncFactor;
                }
                else
                    m_PlaybackPosition[1] = 0;
                m_CurTransitionTime = 0;
            }
            else
            {
                m_TransitionState = newState;
                m_TransitionTime = m_DefaultTransitionTime; // default transition time
                m_SyncFactor = 0;
                m_PlaybackPosition[1] = 0;
                m_CurTransitionTime = 0;
            }
            return true;
        }

        StateHandle GetState() const
        {
            return m_TransitionState;
        }

        void Update(float timeStep, float weight, SkeletonResource const* skeleton, SkeletonPose* pose)
        {
            if (!m_CurrentState.IsValid() || !m_TransitionState.IsValid())
                return;

            //if (m_Layer->GetState(m_CurrentState)->Flags & ANIMATION_STATE_PLAY_ONCE)
            //{

            //}

            if (m_CurTransitionTime != m_TransitionTime)
            {
                m_CurTransitionTime += timeStep;
                if (m_CurTransitionTime > m_TransitionTime)
                    m_CurTransitionTime = m_TransitionTime;

                float transitionBlend = m_CurTransitionTime / m_TransitionTime;

                State const* current = m_Layer->GetState(m_CurrentState);
                State const* transition = m_Layer->GetState(m_TransitionState);

                Node const* currentNode = m_Layer->GetNode(current->Node);
                Node const* transitionNode = m_Layer->GetNode(transition->Node);

                float curTimeStep = timeStep;
                float targetTimeStep = timeStep;

                if (m_SyncFactor > 0)
                {
                    curTimeStep *= Math::Lerp(1.0f, 1.0f / m_SyncFactor, transitionBlend);
                    targetTimeStep *= Math::Lerp(m_SyncFactor, 1.0f, transitionBlend);
                }

                m_PlaybackPosition[0] += curTimeStep;
                m_PlaybackPosition[1] += targetTimeStep;

                m_Layer->ProcessNode(currentNode, weight * (1.0f - transitionBlend), m_PlaybackPosition[0], skeleton, pose);
                m_Layer->ProcessNode(transitionNode, weight * transitionBlend, m_PlaybackPosition[1], skeleton, pose);

                if (m_CurTransitionTime == m_TransitionTime)
                {
                    m_CurrentState = m_TransitionState;
                    m_PlaybackPosition[0] = m_PlaybackPosition[1];
                }
            }
            else
            {
                m_PlaybackPosition[0] += timeStep;

                State const* current = m_Layer->GetState(m_CurrentState);
                Node const* currentNode = m_Layer->GetNode(current->Node);
                m_Layer->ProcessNode(currentNode, weight, m_PlaybackPosition[0], skeleton, pose);
            }
        }

        AnimationBlendMachine::Layer const* m_Layer;
        AnimationBlendMachine::StateHandle m_CurrentState;
        AnimationBlendMachine::StateHandle m_TransitionState;
        float m_TransitionTime{};
        float m_CurTransitionTime{};
        float m_PlaybackPosition[2]{0, 0};
        float m_SyncFactor{};
        float m_DefaultTransitionTime{0.1f};
    };

    AnimationBlendMachine(SkeletonHandle skeleton) :
        m_Skeleton(skeleton)
    {}

    ~AnimationBlendMachine()
    {
        for (Layer* layer : m_Layers)
            delete layer;
    }

    SkeletonHandle GetSkeleton() const
    {
        return m_Skeleton;
    }

    Layer* CreateLayer(StringView name)
    {
        m_Layers.Add(new Layer(name));
        return m_Layers.Last();
    }

    Layer* GetLayer(StringView name)
    {
        for (auto* layer : m_Layers)
        {
            if (layer->GetName() == name)
                return layer;
        }
        return nullptr;
    }

    int GetLayerIndex(StringView name) const
    {
        int index{};
        for (auto* layer : m_Layers)
        {
            if (layer->GetName() == name)
                return index;
            index++;
        }
        return -1;
    }

    float GetAnimationDuration(StringView animation);

    Ref<AnimationInstance> Instantiate()
    {
        return MakeRef<AnimationInstance>(this);
    }

private:
    Vector<Layer*> m_Layers;
    SkeletonHandle m_Skeleton;

    friend class AnimationInstance;
};

class AnimationInstance : public RefCounted
{
public:
    using StateHandle = AnimationBlendMachine::StateHandle;

    AnimationInstance(AnimationBlendMachine* blendMachine) :
        m_BlendMachine(blendMachine)
    {
        m_Layers.Resize(m_BlendMachine->m_Layers.Size());
        for (int i = 0, count = m_Layers.Size(); i < count; ++i)
        {
            m_Layers[i].m_Layer = m_BlendMachine->m_Layers[i];
        }
    }

    AnimationBlendMachine* GetBlendMachine()
    {
        return m_BlendMachine;
    }

    void SetLayerState(int layerIndex, StringView name)
    {
        if (layerIndex < 0 || layerIndex >= m_Layers.Size())
            return;

        m_Layers[layerIndex].SetState(name);
    }

    bool ChangeLayerState(int layerIndex, StringView name)
    {
        if (layerIndex < 0 || layerIndex >= m_Layers.Size())
            return false;

        return m_Layers[layerIndex].ChangeState(name);
    }

    void SetLayerState(int layerIndex, StateHandle state)
    {
        if (layerIndex < 0 || layerIndex >= m_Layers.Size())
            return;

        m_Layers[layerIndex].SetState(state);
    }

    bool ChangeLayerState(int layerIndex, StateHandle newState)
    {
        if (layerIndex < 0 || layerIndex >= m_Layers.Size())
            return false;

        return m_Layers[layerIndex].ChangeState(newState);
    }

    StateHandle GetLayerState(int layerIndex) const
    {
        if (layerIndex < 0 || layerIndex >= m_Layers.Size())
            return {};

        return m_Layers[layerIndex].GetState();
    }

    void Update(float timeStep, SkeletonPose* pose);

private:
    Ref<AnimationBlendMachine> m_BlendMachine;
    Vector<AnimationBlendMachine::LayerData> m_Layers;
};

HK_NAMESPACE_END
