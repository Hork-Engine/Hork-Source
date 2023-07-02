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

HK_NAMESPACE_END
