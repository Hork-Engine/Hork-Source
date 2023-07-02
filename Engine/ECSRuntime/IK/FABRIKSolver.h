#pragma once

#include <Engine/Core/String.h>

#include <vector>
#include "Constraints.h"

HK_NAMESPACE_BEGIN

// Forward and Backward Reaching Inverse Kinematics
class FABRIKSolver
{
public:
    FABRIKSolver(int chainSize,
                 IkTransform* IKChain,
                 IkTransform* worldChainTransform,
                 Float3* worldChain,
                 float* lengths,
                 IkConstraint* constraints);

    virtual ~FABRIKSolver() = default;

    int GetChainSize() const { return m_ChainSize; };

    void SetMaxIterations(int maxIterations)
    {
        m_MaxIterations = maxIterations;
    };

    int GetMaxIterations() const { return m_MaxIterations; };

    void SetThreshold(float threshold)
    {
        m_Threshold = threshold;
    }

    float GetThreshold() const { return m_Threshold; };

    void SetLocalTransform(int index, IkTransform const& transform)
    {
        HK_ASSERT(index >= 0 && index < m_ChainSize);
        m_IKChain[index] = transform;
    }

    IkTransform const& GetLocalTransform(int index) const
    {
        HK_ASSERT(index >= 0 && index < m_ChainSize);
        return m_IKChain[index];
    }

    IkConstraint& GetConstraint(int index)
    {
        HK_ASSERT(index >= 0 && index <= m_ChainSize);
        return m_Constraints[index];
    }

    bool Solve(IkTransform const& target);

protected:
    void Reset(int chainSize,
               IkTransform* IKChain,
               IkTransform* worldChainTransform,
               Float3* worldChain,
               float* lengths,
               IkConstraint* constraints);

private:
    void IKChainToWorld();
    void WorldToIKChain();
    void IterateBackward(Float3 const& goal);
    void IterateForward(Float3 const& base);
    void CalcWorldTransform();

    int m_ChainSize;
    IkTransform* m_IKChain;
    IkTransform* m_WorldChainTransform;
    Float3* m_WorldChain;
    float* m_Lengths;
    IkConstraint* m_Constraints;
    int m_MaxIterations{4};
    float m_Threshold{1e-6f};
};

template <size_t N>
class FABRIKSolverN : public FABRIKSolver
{
public:
    FABRIKSolverN() : FABRIKSolver(N, m_IKChain, m_WorldChainTransform, m_WorldChain, m_Lengths, m_Constraints)
    {}

private:
    IkTransform m_IKChain[N];
    IkTransform m_WorldChainTransform[N];
    Float3 m_WorldChain[N];
    float m_Lengths[N];
    IkConstraint m_Constraints[N];
};

class FABRIKSolverDynamic : public FABRIKSolver
{
public:
    FABRIKSolverDynamic() : FABRIKSolver(0, nullptr, nullptr, nullptr, nullptr, nullptr)
    {}

    void SetChainSize(int size);

private:
    std::vector<IkTransform> m_IKChain;
    std::vector<IkTransform> m_WorldChainTransform;
    std::vector<Float3> m_WorldChain;
    std::vector<float> m_Lengths;
    std::vector<IkConstraint> m_Constraints;
};

using FABRIKSolver4 = FABRIKSolverN<4>;




//struct Character
//{
//    uint32_t id;

//    EntityHandle m_Camera;
//    EntityHandle m_SpringArm;

//    void Destroy(ECS::CommandBuffer* cmdBuffer)
//    {
//        cmdBuffer->DestroyEntity(m_Camera);
//        cmdBuffer->DestroyEntity(m_SpringArm);
//    }
//};

//std::unordered_map<uint32_t, Character*> Characters;
#if 0
struct SkeletonNode
{
    IkTransform Local;
    IkTransform Global;
};

class SkeletonPose
{
public:
    std::vector<SkeletonNode> m_Joints;

    void SetLocalRotation(int nodeIndex, Quat const& rotation)
    {
        m_Joints[nodeIndex].Local.rotation = rotation;
    }

    void SetLocalTransform(int nodeIndex, IkTransform const& transform)
    {
        m_Joints[nodeIndex].Local = transform;
    }

    IkTransform const& GetLocalTransform(int nodeIndex)
    {
        return m_Joints[nodeIndex].Local;
    }

    void RecalcGlobalTransform()
    {
        m_Joints[0].Global = m_Joints[0].Local;
        for (int i = 1 ; i < m_Joints.size() ; i++)
            m_Joints[i].Global = m_Joints[i-1].Global * m_Joints[i].Local;
    }
};

class Skeleton
{
public:
    struct SolverInfo
    {
        std::string Name;
        FABRIKSolver* Solver;
        int Node;
    };

    std::vector<SolverInfo> m_Solvers;

    int GetParent(int index)
    {
        return index - 1;
    }

    int AddSolver(StringView name, int node, int chainSize)
    {
        FABRIKSolver* solver;
        switch (chainSize)
        {
        case 1:
            solver = new FABRIKSolverN<1>();
            break;
        case 2:
            solver = new FABRIKSolverN<2>();
            break;
        case 3:
            solver = new FABRIKSolverN<3>();
            break;
        case 4:
            solver = new FABRIKSolverN<4>();
            break;
        default:
            return 0;
        }

        SolverInfo sinfo;
        sinfo.Name = name;
        sinfo.Solver = solver;
        sinfo.Node = node;
        m_Solvers.push_back(sinfo);
        return m_Solvers.size();
    }

    FABRIKSolver* GetSolver(int solverHandle)
    {
        return m_Solvers[solverHandle-1].Solver;
    }

    void Solve(SkeletonPose* pose, int solverHandle, IkTransform const& target)
    {
        SolverInfo& sinfo = m_Solvers[solverHandle-1];
        auto* solver = sinfo.Solver;

        int node = sinfo.Node;
        for (int i = solver->GetChainSize() - 1 ; i >= 0 ; --i)
        {
            solver->SetLocalTransform(i, pose->GetLocalTransform(node));
            node = GetParent(node);
        }

        solver->Solve(target);

        node = sinfo.Node;
        for (int i = solver->GetChainSize() - 1 ; i >= 0 ; --i)
        {
            IkTransform t = solver->GetLocalTransform(i);
            pose->SetLocalRotation(node, t.rotation);
            node = GetParent(node);
        }
    }
};



struct IkTest
{
    Skeleton skeleton;
    SkeletonPose pose;
    int lfoot_solver;

    IkTest()
    {
        lfoot_solver = skeleton.AddSolver("LFoot", 3, 4);

        auto* solver = skeleton.GetSolver(lfoot_solver);
        solver->GetConstraint(0).InitAngleConstraint(45);
        solver->GetConstraint(1).InitHingeConstraint(-120, 5);
        solver->GetConstraint(2).InitHingeConstraint(-45, 5);

        pose.m_Joints.resize(4);
        pose.m_Joints[0].Local.position = Float3{0, 0, 0};
        pose.m_Joints[0].Local.rotation = Quat::Identity();

        pose.m_Joints[1].Local.position = Float3{0, 2, 0};
        pose.m_Joints[1].Local.rotation = Quat::Identity();

        pose.m_Joints[2].Local.position = Float3{0, 1.5f, 0};
        pose.m_Joints[2].Local.rotation = Quat::Identity();

        pose.m_Joints[3].Local.position = Float3{0, 1, 0};
        pose.m_Joints[3].Local.rotation = Quat::Identity();
    }

//    void draw(DebugRenderer& debug_renderer)
//    {
//        static double time = 0;
//        time += 0.01;

//        IkTransform targetTransform;
//        targetTransform.position.x = std::sin(time)*2;
//        targetTransform.position.y = 1;
//        targetTransform.position.z = 0;
//        targetTransform.rotation = Quat::Identity();

//        skeleton.Solve(&pose, lfoot_solver, targetTransform);

//        pose.RecalcGlobalTransform();

//        for (int i = 0; i < pose.m_Joints.size(); ++i)
//        {
//            if (i > 0)
//                debug_renderer.add_line(scaled(pose.m_Joints[i].Global.position), scaled(pose.m_Joints[i-1].Global.position), color_u32({1,0,0,1}));

//            BoundingBoxd box;
//            box.mins = scaled(pose.m_Joints[i].Global.position - Float3(0.1f));
//            box.maxs = scaled(pose.m_Joints[i].Global.position + Float3(0.1f));

//            debug_renderer.add_bounding_box(box, color_u32({1,1,0,1}));
//        }
//    }
};
#endif

HK_NAMESPACE_END
