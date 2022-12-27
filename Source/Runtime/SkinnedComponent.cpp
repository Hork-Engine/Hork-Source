/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

/*
 TODO: Future optimizations: parallel, SSE
 */

#include "SkinnedComponent.h"
#include "AnimationController.h"
#include "Actor.h"
#include "World.h"
#include "DebugRenderer.h"
#include "ResourceManager.h"
#include "Animation.h"
#include "BulletCompatibility.h"

#include <Assets/Asset.h>
#include <Platform/Logger.h>
#include <Core/ConsoleVar.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <RenderCore/VertexMemoryGPU.h>

ConsoleVar com_DrawSkeleton("com_DrawSkeleton"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(SkinnedComponent)

SkinnedComponent::SkinnedComponent()
{
    m_DrawableType = DRAWABLE_SKINNED_MESH;

    m_bUpdateBounds = false;
    m_bUpdateControllers = true;
    m_bUpdateRelativeTransforms = false;
    m_bUpdateAbsoluteTransforms = false;
    m_bJointsSimulatedByPhysics = false;
    m_bSkinnedMesh = true;

    // Raycasting of skinned meshes is not supported yet
    m_Primitive->RaycastCallback = nullptr;
    m_Primitive->RaycastClosestCallback = nullptr;

    static TStaticResourceFinder<Skeleton> SkeletonResource("/Default/Skeleton/Default"s);
    m_Skeleton = SkeletonResource.GetObject();
}

SkinnedComponent::~SkinnedComponent()
{
    RemoveAnimationControllers();
}

void SkinnedComponent::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->SkinningSystem.SkinnedMeshes.Add(this);
}

void SkinnedComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->SkinningSystem.SkinnedMeshes.Remove(this);
}

void SkinnedComponent::UpdateMesh()
{
    Super::UpdateMesh();

    Skeleton* newSkeleton = GetMesh()->GetSkeleton();

    if (m_Skeleton == newSkeleton)
    {
        return;
    }

    m_Skeleton = newSkeleton;

    TPodVector<SkeletonJoint> const& joints = m_Skeleton->GetJoints();

    int numJoints = joints.Size();

    m_AbsoluteTransforms.ResizeInvalidate(numJoints + 1); // + 1 for root's parent
    m_AbsoluteTransforms[0].SetIdentity();

    m_RelativeTransforms.ResizeInvalidate(numJoints);
    for (int i = 0; i < numJoints; i++)
    {
        m_RelativeTransforms[i] = joints[i].LocalTransform;
    }

    m_bUpdateControllers = true;
}

void SkinnedComponent::AddAnimationController(AnimationController* _Controller)
{
    if (!_Controller)
    {
        return;
    }
    if (_Controller->Owner)
    {
        if (_Controller->Owner != this)
        {
            LOG("SkinnedComponent::AddAnimationController: animation controller already added to other component\n");
        }
        return;
    }
    _Controller->Owner = this;
    _Controller->AddRef();
    m_AnimControllers.Add(_Controller);
    m_bUpdateControllers = true;
}

void SkinnedComponent::RemoveAnimationController(AnimationController* _Controller)
{
    if (!_Controller)
    {
        return;
    }
    if (_Controller->Owner != this)
    {
        return;
    }
    for (int i = 0, count = m_AnimControllers.Size(); i < count; i++)
    {
        if (m_AnimControllers[i]->Id == _Controller->Id)
        {
            _Controller->Owner = nullptr;
            _Controller->RemoveRef();
            m_AnimControllers.Remove(i);
            m_bUpdateControllers = true;
            return;
        }
    }
}

void SkinnedComponent::RemoveAnimationControllers()
{
    for (AnimationController* controller : m_AnimControllers)
    {
        controller->Owner = nullptr;
        controller->RemoveRef();
    }
    m_AnimControllers.Clear();
    m_bUpdateControllers = true;
}

void SkinnedComponent::SetTimeBroadcast(float _Time)
{
    for (int i = 0, count = m_AnimControllers.Size(); i < count; i++)
    {
        AnimationController* controller = m_AnimControllers[i];
        controller->SetTime(_Time);
    }
}

void SkinnedComponent::AddTimeDeltaBroadcast(float _TimeDelta)
{
    for (int i = 0, count = m_AnimControllers.Size(); i < count; i++)
    {
        AnimationController* controller = m_AnimControllers[i];
        controller->AddTimeDelta(_TimeDelta);
    }
}

HK_FORCEINLINE float Quantize(float _Lerp, float _Quantizer)
{
    return _Quantizer > 0.0f ? Math::Floor(_Lerp * _Quantizer) / _Quantizer : _Lerp;
}

void SkinnedComponent::MergeJointAnimations()
{
    if (m_bJointsSimulatedByPhysics)
    {
        // TODO:
        if (SoftBody && m_bUpdateAbsoluteTransforms)
        {

            //LOG("Update abs matrices\n");
            TPodVector<SkeletonJoint> const& joints = m_Skeleton->GetJoints();
            for (int j = 0; j < joints.Size(); j++)
            {
                // TODO: joint rotation from normal?
                m_AbsoluteTransforms[j + 1].Compose(btVectorToFloat3(SoftBody->m_nodes[j].m_x), Float3x3::Identity());
            }
            m_bUpdateAbsoluteTransforms = false;
            //bWriteTransforms = true;
        }
    }
    else
    {
        UpdateControllersIfDirty();
        UpdateTransformsIfDirty();
        UpdateAbsoluteTransformsIfDirty();
    }
}

void SkinnedComponent::UpdateTransformsIfDirty()
{
    if (!m_bUpdateRelativeTransforms)
    {
        return;
    }

    UpdateTransforms();
}

void SkinnedComponent::UpdateTransforms()
{
    SkeletonJoint const* joints = m_Skeleton->GetJoints().ToPtr();
    int jointsCount = m_Skeleton->GetJoints().Size();

    TPodVector<Transform> tempTransforms;
    TPodVector<float>      weights;

    tempTransforms.Resize(m_AnimControllers.Size());
    weights.Resize(m_AnimControllers.Size());

    Platform::ZeroMem(m_RelativeTransforms.ToPtr(), sizeof(m_RelativeTransforms[0]) * jointsCount);

    for (int jointIndex = 0; jointIndex < jointsCount; jointIndex++)
    {
        float sumWeight = 0;

        int n = 0;

        for (int controllerId = 0, count = m_AnimControllers.Size(); controllerId < count; controllerId++)
        {
            AnimationController* controller = m_AnimControllers[controllerId];
            SkeletalAnimation*   animation  = controller->Animation;

            if (!controller->bEnabled || !animation || !animation->IsValid())
            {
                continue;
            }

            // TODO: Enable/Disable joint animation?

            unsigned short channelIndex = animation->GetChannelIndex(jointIndex);

            if (channelIndex == (unsigned short)-1)
            {
                continue;
            }

            TPodVector<AnimationChannel> const& animJoints = animation->GetChannels();

            AnimationChannel const& jointAnim = animJoints[channelIndex];

            TPodVector<Transform> const& transforms = animation->GetTransforms();

            Transform& transform = tempTransforms[n];
            weights[n]            = controller->Weight;
            n++;

            if (controller->Frame == controller->NextFrame || controller->Blend < 0.0001f)
            {
                transform = transforms[jointAnim.TransformOffset + controller->Frame];
            }
            else
            {
                Transform const& frame1 = transforms[jointAnim.TransformOffset + controller->Frame];
                Transform const& frame2 = transforms[jointAnim.TransformOffset + controller->NextFrame];

                transform.Position = Math::Lerp(frame1.Position, frame2.Position, controller->Blend);
                transform.Rotation = Math::Slerp(frame1.Rotation, frame2.Rotation, controller->Blend);
                transform.Scale    = Math::Lerp(frame1.Scale, frame2.Scale, controller->Blend);
            }

            sumWeight += controller->Weight;
        }

        Float3x4& resultTransform = m_RelativeTransforms[jointIndex];

        if (n > 0)
        {
            Float3x4 m;

            const float sumWeightReciprocal = (sumWeight == 0.0f) ? 0.0f : 1.0f / sumWeight;

            for (int i = 0; i < n; i++)
            {
                const float weight = weights[i] * sumWeightReciprocal;

                tempTransforms[i].ComputeTransformMatrix(m);

                resultTransform[0] += m[0] * weight;
                resultTransform[1] += m[1] * weight;
                resultTransform[2] += m[2] * weight;
            }
        }
        else
        {
            resultTransform = joints[jointIndex].LocalTransform;
        }
    }

    m_bUpdateRelativeTransforms = false;
    m_bUpdateAbsoluteTransforms = true;
}

void SkinnedComponent::UpdateAbsoluteTransformsIfDirty()
{
    if (!m_bUpdateAbsoluteTransforms)
    {
        return;
    }

    TPodVector<SkeletonJoint> const& joints = m_Skeleton->GetJoints();

    for (int j = 0; j < joints.Size(); j++)
    {
        SkeletonJoint const& joint = joints[j];

        // ... Update relative joints physics here ...

        m_AbsoluteTransforms[j + 1] = m_AbsoluteTransforms[joint.Parent + 1] * m_RelativeTransforms[j];

        // ... Update absolute joints physics here ...
    }

    m_bUpdateAbsoluteTransforms = false;
    //m_bWriteTransforms = true;
}

void SkinnedComponent::UpdateControllersIfDirty()
{
    if (!m_bUpdateControllers)
    {
        return;
    }

    UpdateControllers();
}

void SkinnedComponent::UpdateControllers()
{
    float controllerTimeLine;
    int   keyFrame;
    float lerp;
    int   take;

    for (int controllerId = 0, count = m_AnimControllers.Size(); controllerId < count; controllerId++)
    {
        AnimationController* controller = m_AnimControllers[controllerId];
        SkeletalAnimation*   anim       = controller->Animation;

        if (!anim)
        {
            continue;
        }

        if (anim->GetFrameCount() > 1)
        {
            switch (controller->PlayMode)
            {
                case ANIMATION_PLAY_CLAMP:

                    // clamp and normalize 0..1
                    if (controller->TimeLine <= 0.0f)
                    {
                        controller->Blend     = 0;
                        controller->Frame     = 0;
                        controller->NextFrame = 0;
                    }
                    else if (controller->TimeLine >= anim->GetDurationInSeconds())
                    {
                        controller->Blend = 0;
                        controller->Frame = controller->NextFrame = anim->GetFrameCount() - 1;
                    }
                    else
                    {
                        // normalize 0..1
                        controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();

                        // adjust 0...framecount-1
                        controllerTimeLine = controllerTimeLine * (float)(anim->GetFrameCount() - 1);

                        keyFrame = Math::Floor(controllerTimeLine);
                        lerp     = Math::Fract(controllerTimeLine);

                        controller->Frame     = keyFrame;
                        controller->NextFrame = keyFrame + 1;
                        controller->Blend     = Quantize(lerp, controller->Quantizer);
                    }
                    break;

                case ANIMATION_PLAY_WRAP:

                    // normalize 0..1
#if 1
                    controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();
                    controllerTimeLine = Math::Fract(controllerTimeLine);
#else
                    controllerTimeLine = fmod(controller->TimeLine, anim->GetDurationInSeconds()) * anim->GetDurationNormalize();
                    if (controllerTimeLine < 0.0f)
                    {
                        controllerTimeLine += 1.0f;
                    }
#endif

                    // adjust 0...framecount-1
                    controllerTimeLine = controllerTimeLine * (float)(anim->GetFrameCount() - 1);

                    keyFrame = Math::Floor(controllerTimeLine);
                    lerp     = Math::Fract(controllerTimeLine);

                    if (controller->TimeLine < 0.0f)
                    {
                        controller->Frame     = keyFrame + 1;
                        controller->NextFrame = keyFrame;
                        controller->Blend     = Quantize(1.0f - lerp, controller->Quantizer);
                    }
                    else
                    {
                        controller->Frame     = keyFrame;
                        controller->NextFrame = controller->Frame + 1;
                        controller->Blend     = Quantize(lerp, controller->Quantizer);
                    }
                    break;

                case ANIMATION_PLAY_MIRROR:

                    // normalize 0..1
                    controllerTimeLine = controller->TimeLine * anim->GetDurationNormalizer();
                    take               = Math::Floor(Math::Abs(controllerTimeLine));
                    controllerTimeLine = Math::Fract(controllerTimeLine);

                    // adjust 0...framecount-1
                    controllerTimeLine = controllerTimeLine * (float)(anim->GetFrameCount() - 1);

                    keyFrame = Math::Floor(controllerTimeLine);
                    lerp     = Math::Fract(controllerTimeLine);

                    if (controller->TimeLine < 0.0f)
                    {
                        controller->Frame     = keyFrame + 1;
                        controller->NextFrame = keyFrame;
                        controller->Blend     = Quantize(1.0f - lerp, controller->Quantizer);
                    }
                    else
                    {
                        controller->Frame     = keyFrame;
                        controller->NextFrame = controller->Frame + 1;
                        controller->Blend     = Quantize(lerp, controller->Quantizer);
                    }

                    if (take & 1)
                    {
                        controller->Frame     = anim->GetFrameCount() - controller->Frame - 1;
                        controller->NextFrame = anim->GetFrameCount() - controller->NextFrame - 1;
                    }
                    break;
            }
        }
        else if (anim->GetFrameCount() == 1)
        {
            controller->Blend     = 0;
            controller->Frame     = 0;
            controller->NextFrame = 0;
        }
    }

    m_bUpdateControllers = false;
    m_bUpdateBounds = true;
    m_bUpdateRelativeTransforms = true;
}

void SkinnedComponent::UpdateBounds()
{
    UpdateControllersIfDirty();

    if (!m_bUpdateBounds)
    {
        return;
    }

    m_bUpdateBounds = false;

    if (m_AnimControllers.IsEmpty())
    {
        m_Bounds = m_Skeleton->GetBindposeBounds();
    }
    else
    {
        m_Bounds.Clear();
        for (int controllerId = 0, count = m_AnimControllers.Size(); controllerId < count; controllerId++)
        {
            AnimationController const* controller = m_AnimControllers[controllerId];
            SkeletalAnimation*         animation  = controller->Animation;

            if (!controller->bEnabled || !animation || animation->GetFrameCount() == 0)
            {
                continue;
            }

            m_Bounds.AddAABB(animation->GetBoundingBoxes()[controller->Frame]);
        }
    }

    // Mark to update world bounds
    UpdateWorldBounds();
}

void SkinnedComponent::GetSkeletonHandle(size_t& skeletonOffset, size_t& skeletonOffsetMB, size_t& skeletonSize)
{
    skeletonOffset = m_SkeletonOffset;
    skeletonOffsetMB = m_SkeletonOffsetMB;
    skeletonSize = m_SkeletonSize;
}

void SkinnedComponent::OnPreRenderUpdate(RenderFrontendDef const* _Def)
{
    Super::OnPreRenderUpdate(_Def);

    MergeJointAnimations();

    MeshSkin const& skin = GetMesh()->GetSkin();
    TPodVector<SkeletonJoint> const& joints = m_Skeleton->GetJoints();

    m_SkeletonSize = joints.Size() * sizeof(Float3x4);
    if (m_SkeletonSize > 0)
    {
        StreamedMemoryGPU* streamedMemory = _Def->StreamedMemory;

        // Write joints from previous frame
        m_SkeletonOffsetMB = streamedMemory->AllocateJoint(m_SkeletonSize, m_JointsBufferData);

        // Write joints from current frame
        m_SkeletonOffset = streamedMemory->AllocateJoint(m_SkeletonSize, nullptr);
        Float3x4* data = (Float3x4*)streamedMemory->Map(m_SkeletonOffset);
        for (int j = 0; j < skin.JointIndices.Size(); j++)
        {
            int jointIndex = skin.JointIndices[j];
            data[j] = m_JointsBufferData[j] = m_AbsoluteTransforms[jointIndex + 1] * skin.OffsetMatrices[j];
        }
    }
    else
    {
        m_SkeletonOffset = m_SkeletonOffsetMB = 0;
    }
}

Float3x4 const& SkinnedComponent::GetJointTransform(int _JointIndex)
{
    if (_JointIndex < 0 || _JointIndex >= m_Skeleton->GetJoints().Size())
    {
        return Float3x4::Identity();
    }

    MergeJointAnimations();

    return m_AbsoluteTransforms[_JointIndex + 1];
}

void SkinnedComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    // Draw skeleton
    if (com_DrawSkeleton)
    {
        InRenderer->SetColor(Color4(1, 0, 0, 1));
        InRenderer->SetDepthTest(false);
        TPodVector<SkeletonJoint> const& joints = m_Skeleton->GetJoints();

        for (int i = 0; i < joints.Size(); i++)
        {
            SkeletonJoint const& joint = joints[i];

            Float3x4 t  = GetWorldTransformMatrix() * GetJointTransform(i);
            Float3   v1 = t.DecomposeTranslation();

            InRenderer->DrawOrientedBox(v1, t.DecomposeRotation(), Float3(0.01f));

            if (joint.Parent >= 0)
            {
                Float3 v0 = (GetWorldTransformMatrix() * GetJointTransform(joint.Parent)).DecomposeTranslation();
                InRenderer->DrawLine(v0, v1);
            }
        }
    }
}

/*
void ToMatrix( Float3x4 & _JointTransform, Float3 const & Position, Quat const & Rotation ) const {
    float * mat = Math::ToPtr( _JointTransform );

    float x2 = Rotation.x + Rotation.x;
    float y2 = Rotation.y + Rotation.y;
    float z2 = Rotation.z + Rotation.z;

    float xx = Rotation.x * x2;
    float xy = Rotation.x * y2;
    float xz = Rotation.x * z2;

    float yy = Rotation.y * y2;
    float yz = Rotation.y * z2;
    float zz = Rotation.z * z2;

    float wx = Rotation.w * x2;
    float wy = Rotation.w * y2;
    float wz = Rotation.w * z2;

    mat[ 0 * 4 + 0 ] = 1.0f - ( yy + zz );
    mat[ 0 * 4 + 1 ] = xy + wz;
    mat[ 0 * 4 + 2 ] = xz - wy;

    mat[ 1 * 4 + 0 ] = xy - wz;
    mat[ 1 * 4 + 1 ] = 1.0f - ( xx + zz );
    mat[ 1 * 4 + 2 ] = yz + wx;

    mat[ 2 * 4 + 0 ] = xz + wy;
    mat[ 2 * 4 + 1 ] = yz - wx;
    mat[ 2 * 4 + 2 ] = 1.0f - ( xx + yy );

    mat[ 0 * 4 + 3 ] = Position[0];
    mat[ 1 * 4 + 3 ] = Position[1];
    mat[ 2 * 4 + 3 ] = Position[2];
}

void FromMatrix( Float3x4 const & _JointTransform, Float3 & Position, Quat & Rotation  ) {
    float trace;
    float s;
    float t;
    int i;
    int j;
    int k;

    static const int next[3] = { 1, 2, 0 };

    const float * mat = Math::ToPtr( _JointTransform );

    trace = mat[ 0 * 4 + 0 ] + mat[ 1 * 4 + 1 ] + mat[ 2 * 4 + 2 ];

    if ( trace > 0.0f ) {

        t = trace + 1.0f;
        s = Math::InvSqrt( t ) * 0.5f;

        Rotation[3] = s * t;
        Rotation[0] = ( mat[ 1 * 4 + 2 ] - mat[ 2 * 4 + 1 ] ) * s;
        Rotation[1] = ( mat[ 2 * 4 + 0 ] - mat[ 0 * 4 + 2 ] ) * s;
        Rotation[2] = ( mat[ 0 * 4 + 1 ] - mat[ 1 * 4 + 0 ] ) * s;

    } else {

        i = 0;
        if ( mat[ 1 * 4 + 1 ] > mat[ 0 * 4 + 0 ] ) {
            i = 1;
        }
        if ( mat[ 2 * 4 + 2 ] > mat[ i * 4 + i ] ) {
            i = 2;
        }
        j = next[i];
        k = next[j];

        t = ( mat[ i * 4 + i ] - ( mat[ j * 4 + j ] + mat[ k * 4 + k ] ) ) + 1.0f;
        s = Math::InvSqrt( t ) * 0.5f;

        Rotation[i] = s * t;
        Rotation[3] = ( mat[ j * 4 + k ] - mat[ k * 4 + j ] ) * s;
        Rotation[j] = ( mat[ i * 4 + j ] + mat[ j * 4 + i ] ) * s;
        Rotation[k] = ( mat[ i * 4 + k ] + mat[ k * 4 + i ] ) * s;
    }

    Position.x = mat[ 0 * 4 + 3 ];
    Position.y = mat[ 1 * 4 + 3 ];
    Position.z = mat[ 2 * 4 + 3 ];
}
*/
