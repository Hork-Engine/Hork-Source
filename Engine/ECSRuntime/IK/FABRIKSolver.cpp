#include "FABRIKSolver.h"

HK_NAMESPACE_BEGIN

FABRIKSolver::FABRIKSolver(int chainSize,
                           IkTransform* IKChain,
                           IkTransform* worldChainTransform,
                           Float3* worldChain,
                           float* lengths,
                           IkConstraint* constraints) :
    m_ChainSize(chainSize),
    m_IKChain(IKChain),
    m_WorldChainTransform(worldChainTransform),
    m_WorldChain(worldChain),
    m_Lengths(lengths),
    m_Constraints(constraints)
{
}

void FABRIKSolver::Reset(int chainSize,
                         IkTransform* IKChain,
                         IkTransform* worldChainTransform,
                         Float3* worldChain,
                         float* lengths,
                         IkConstraint* constraints)
{
    m_ChainSize = chainSize;
    m_IKChain = IKChain;
    m_WorldChainTransform = worldChainTransform;
    m_WorldChain = worldChain;
    m_Lengths = lengths;
    m_Constraints = constraints;
}

bool FABRIKSolver::Solve(IkTransform const& target)
{
    if (m_ChainSize == 0)
        return false;

    IKChainToWorld();

    Float3 goal = target.Position;
    Float3 base = m_WorldChain[0];

    int last = m_ChainSize - 1;
    for (int i = 0; i < m_MaxIterations; ++i)
    {
        Float3 effector = m_WorldChain[last];
        if ((goal - effector).LengthSqr() < m_Threshold)
        {
            WorldToIKChain();
            return true;
        }

        IterateBackward(goal);
        IterateForward(base);

        for (int j = 0; j < m_ChainSize; ++j)
        {
            if (m_Constraints[j].Type != IK_CONSTRAINT_UNDEFINED)
            {
                WorldToIKChain();
                m_IKChain[j].Rotation = m_Constraints[j].Apply(m_IKChain[j].Rotation);
                IKChainToWorld();
            }
        }
    }

    WorldToIKChain();

    Float3 effector = m_WorldChainTransform[last].Position;

    return (goal - effector).LengthSqr() < m_Threshold;
}

void FABRIKSolver::CalcWorldTransform()
{
    m_WorldChainTransform[0] = m_IKChain[0];
    for (int i = 1 ; i < m_ChainSize ; i++)
        m_WorldChainTransform[i] = m_WorldChainTransform[i - 1] * m_IKChain[i];
}

void FABRIKSolver::IKChainToWorld()
{
    CalcWorldTransform();

    m_WorldChain[0] = m_WorldChainTransform[0].Position;
    m_Lengths[0] = 0.0f;

    for (int i = 1; i < m_ChainSize; ++i)
    {
        IkTransform const& world = m_WorldChainTransform[i];

        m_WorldChain[i] = world.Position;
        m_Lengths[i] = (world.Position - m_WorldChain[i - 1]).Length();
    }
}

void FABRIKSolver::WorldToIKChain()
{
    CalcWorldTransform();

    for (int i = 0; i < m_ChainSize - 1; ++i)
    {
        IkTransform const& currWorld = m_WorldChainTransform[i];
        IkTransform const& childWorld = m_WorldChainTransform[i + 1];

        Quat rotationInv = currWorld.Rotation.Inversed();

        Float3 toChild = rotationInv * (childWorld.Position - currWorld.Position);
        Float3 toDesired = rotationInv * (m_WorldChain[i + 1] - currWorld.Position);
        Quat delta = Math::GetRotation(toChild, toDesired);
        m_IKChain[i].Rotation = m_IKChain[i].Rotation * delta;
    }
}

void FABRIKSolver::IterateBackward(Float3 const& goal)
{
    m_WorldChain[m_ChainSize - 1] = goal;
    for (int i = m_ChainSize - 2; i >= 0; --i)
    {
        Float3 dir = (m_WorldChain[i] - m_WorldChain[i + 1]).Normalized();
        m_WorldChain[i] = m_WorldChain[i + 1] + dir * m_Lengths[i + 1];
    }
}

void FABRIKSolver::IterateForward(Float3 const& base)
{
    m_WorldChain[0] = base;
    for (int i = 1; i < m_ChainSize; ++i)
    {
        Float3 dir = (m_WorldChain[i] - m_WorldChain[i - 1]).Normalized();
        m_WorldChain[i] = m_WorldChain[i - 1] + dir * m_Lengths[i];
    }
}

void FABRIKSolverDynamic::SetChainSize(int size)
{
    m_IKChain.resize(size);
    m_WorldChainTransform.resize(size);
    m_WorldChain.resize(size);
    m_Lengths.resize(size);
    m_Constraints.resize(size);

    Reset(size, m_IKChain.data(), m_WorldChainTransform.data(), m_WorldChain.data(), m_Lengths.data(), m_Constraints.data());
}

HK_NAMESPACE_END
