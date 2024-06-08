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

#include "Resource_Skeleton.h"
#include "ResourceManager.h"

HK_NAMESPACE_BEGIN

SkeletalAnimation::SkeletalAnimation(IBinaryStreamReadInterface& stream)
{
    Read(stream);
}

SkeletalAnimation::SkeletalAnimation(int frameCount, float frameDelta, Transform const* transforms, int transformsCount, AnimationChannel const* animatedJoints, int numAnimatedJoints, BvAxisAlignedBox const* bounds)
{
    HK_ASSERT(transformsCount == frameCount * numAnimatedJoints);

    m_Channels.ResizeInvalidate(numAnimatedJoints);
    Core::Memcpy(m_Channels.ToPtr(), animatedJoints, sizeof(m_Channels[0]) * numAnimatedJoints);

    m_Transforms.ResizeInvalidate(transformsCount);
    Core::Memcpy(m_Transforms.ToPtr(), transforms, sizeof(m_Transforms[0]) * transformsCount);

    m_Bounds.ResizeInvalidate(frameCount);
    Core::Memcpy(m_Bounds.ToPtr(), bounds, sizeof(m_Bounds[0]) * frameCount);

    m_FrameCount = frameCount;
    m_FrameDelta = frameDelta;

    Initialize();
}

void SkeletalAnimation::Initialize()
{
    m_FrameRate          = 1.0f / m_FrameDelta;
    m_DurationInSeconds  = (m_FrameCount - 1) * m_FrameDelta;
    m_DurationNormalizer = 1.0f / m_DurationInSeconds;
    m_bIsAnimationValid  = m_FrameCount > 0 && !m_Channels.IsEmpty();
}

void SkeletalAnimation::Read(IBinaryStreamReadInterface& stream)
{
    m_Name = stream.ReadString();
    m_FrameDelta = stream.ReadFloat();
    m_FrameCount = stream.ReadUInt32();
    stream.ReadArray(m_Channels);
    stream.ReadArray(m_Transforms);
    stream.ReadArray(m_Bounds);

    Initialize();
}

void SkeletalAnimation::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteString(m_Name);
    stream.WriteFloat(m_FrameDelta);
    stream.WriteUInt32(m_FrameCount);
    stream.WriteArray(m_Channels);
    stream.WriteArray(m_Transforms);
    stream.WriteArray(m_Bounds);
}

SkeletonResource::SkeletonResource(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    Read(stream, resManager);
}

SkeletonResource::SkeletonResource(SkeletonJoint* joints, int jointsCount, BvAxisAlignedBox const& bindposeBounds)
{
    if (jointsCount < 0)
    {
        LOG("Skeleton::ctor: joints count < 0\n");
        jointsCount = 0;
    }

    // Copy joints
    m_Joints.ResizeInvalidate(jointsCount);
    if (jointsCount > 0)
    {
        Core::Memcpy(m_Joints.ToPtr(), joints, sizeof(*joints) * jointsCount);
    }

    m_BindposeBounds = bindposeBounds;
}

SkeletonResource::~SkeletonResource()
{
    for (auto& sinfo : m_Solvers)
        delete sinfo.Solver;
}

int SkeletonResource::FindJoint(StringView name) const
{
    for (int j = 0; j < m_Joints.Size(); j++)
    {
        if (!name.Icmp(m_Joints[j].Name))
        {
            return j;
        }
    }
    return -1;
}

bool SkeletonResource::Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    stream.ReadArray(m_Joints);
    stream.ReadObject(m_BindposeBounds);

    uint32_t numAnimations = stream.ReadUInt32();
    m_Animations.Clear();
    m_Animations.Reserve(numAnimations);
    for (uint32_t i = 0 ; i < numAnimations ; ++i)
    {
        m_Animations.Add(MakeRef<SkeletalAnimation>(stream));
    }

    // -------------- TEST -------------------

    for (auto& joint : m_Joints)
        LOG("Joint: '{}'\n", joint.Name);

    int jointIndex = FindJoint("Bone.011");
    auto lfoot_solver =
        AddSolver("LFoot", jointIndex, 3);

    auto* solver = GetSolver(lfoot_solver);
    //solver->GetConstraint(0).InitAngleConstraint(45);
    //solver->GetConstraint(0).DefaultLocalRotation.FromAngles(0, Math::Radians(15), 0);
    //solver->GetConstraint(1).InitAngleConstraint(45);
    //solver->GetConstraint(1).DefaultLocalRotation.FromAngles(0, Math::Radians(-15), 0);
    //solver->GetConstraint(2).InitAngleConstraint(45);
    //solver->GetConstraint(2).DefaultLocalRotation.FromAngles(0, Math::Radians(15), 0);
    //solver->GetConstraint(1).InitHingeConstraint(-120, 5);
    //solver->GetConstraint(2).InitHingeConstraint(-45, 5);
    HK_UNUSED(solver);

    // ---------------------------------------

    return true;
}

void SkeletonResource::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteUInt32(MakeResourceMagic(Type, Version));
    stream.WriteArray(m_Joints);
    stream.WriteObject(m_BindposeBounds);

    uint32_t numAnimations = m_Animations.Size();
    stream.WriteUInt32(numAnimations);
    for (uint32_t i = 0 ; i < numAnimations ; ++i)
    {
        m_Animations[i]->Write(stream);
    }
}

void SkeletonResource::SetAnimations(TArrayView<TRef<SkeletalAnimation>> const& animations)
{
    m_Animations.Clear();
    m_Animations.Reserve(animations.Size());
    for (SkeletalAnimation* animation : animations)
        m_Animations.Add() = animation;
}

uint32_t SkeletonResource::FindAnimation(StringView name) const
{
    uint32_t id{};
    for (SkeletalAnimation* animation : m_Animations)
    {
        if (animation->GetName().Icompare(name))
        {
            return id;
        }
        id++;
    }
    return (uint32_t)-1;
}

int SkeletonResource::AddSolver(StringView name, int jointIndex, int chainSize)
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
    sinfo.JointIndex = jointIndex;
    m_Solvers.Add(sinfo);
    return m_Solvers.Size();
}

void SkeletonResource::Solve(SkeletonPose* pose, int solverHandle, IkTransform const& target)
{
    SolverInfo& sinfo = m_Solvers[solverHandle - 1];
    auto* solver = sinfo.Solver;

    int jointIndex = sinfo.JointIndex;
    //int chainRootJoint = jointIndex;
    for (int i = solver->GetChainSize() - 1; i >= 0; --i)
    {
        IkTransform localTransform;
        localTransform.Position = pose->m_RelativeTransforms[jointIndex].DecomposeTranslation();
        localTransform.Rotation.FromMatrix(pose->m_RelativeTransforms[jointIndex].DecomposeRotation());

        solver->SetLocalTransform(i, localTransform);

        //chainRootJoint = jointIndex;
        jointIndex = m_Joints[jointIndex].Parent;
    }

    Float3x4 m = pose->m_AbsoluteTransforms[jointIndex + 1];
    Quat baseRotation;
    baseRotation.FromMatrix(m.DecomposeRotation());
    IkTransform baseTransform;
    baseTransform.Position = m.Inversed().DecomposeTranslation();
    baseTransform.Rotation = baseRotation.Inversed();

    solver->Solve(baseTransform * target);

    jointIndex = sinfo.JointIndex;
    for (int i = solver->GetChainSize() - 1; i >= 0; --i)
    {
        IkTransform const& t = solver->GetLocalTransform(i);

        pose->m_RelativeTransforms[jointIndex].SetRotationAndResetScale(t.Rotation.ToMatrix3x3());

        //pose->SetLocalRotation(jointIndex, t.Rotation);
        jointIndex = m_Joints[jointIndex].Parent;
    }
}

HK_NAMESPACE_END
