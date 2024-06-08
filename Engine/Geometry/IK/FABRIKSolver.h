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
