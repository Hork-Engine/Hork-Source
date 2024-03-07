#include "SkinningSystem.h"
#include "../GameFrame.h"
#include "../Components/RenderTransformComponent.h"
#include "../Components/SkeletonControllerComponent.h"
#include "../Components/SkeletonPoseComponent.h"
#include "../Components/SocketComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/WorldTransformComponent.h"
#include "../Resources/ResourceManager.h"
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSkeleton("com_DrawSkeleton"s, "0"s);

SkinningSystem_ECS::SkinningSystem_ECS(ECS::World* world) :
    m_World(world)
{}

void SkinningSystem_ECS::UpdatePoses(GameFrame const& frame)
{
    m_FrameIndex = frame.StateIndex;

    using Query = ECS::Query<>
        ::Required<SkeletonPoseComponent>
        ::Required<SkeletonControllerComponent>
        ;

    for (Query::Iterator it(*m_World); it; it++)
    {
        SkeletonPoseComponent* poseComponent = it.Get<SkeletonPoseComponent>();
        SkeletonControllerComponent* controllerComponent = it.Get<SkeletonControllerComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            SkeletonPose* pose = poseComponent[i].Pose;
            SkeletonControllerComponent& controller = controllerComponent[i];

            controller.AnimInstance->Update(frame.FixedTimeStep, pose);
            #if 0
            SkeletonResource* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
            if (skeleton)
            {
                {
                    SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
                    uint32_t jointsCount = skeleton->GetJointsCount();

                    for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
                    {
                        pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
                    }
                }

                IkTransform target;
                
                int joint = skeleton->FindJoint("Bone.011");
                target.Rotation.FromMatrix(pose->m_AbsoluteTransforms[joint+1].DecomposeRotation());
                target.Position = pose->m_AbsoluteTransforms[joint+1].DecomposeTranslation();

                target.Position.Y += (sin(frame.VariableTime*2) * 0.5f + 0.5f) * 0.6f;
                //if (target.Position.Y < 0.6f)
                //    target.Position.Y = 0.6f;

                skeleton->Solve(pose, 1, target);

                SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
                uint32_t jointsCount = skeleton->GetJointsCount();

                for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
                {
                    pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
                }
            }
            #else

            SkeletonResource* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
            if (skeleton)
            {
                SkeletonJoint const* joints = skeleton->GetJoints().ToPtr();
                uint32_t jointsCount = skeleton->GetJointsCount();
                for (uint32_t jointIndex = 0; jointIndex < jointsCount; jointIndex++)
                {
                    pose->m_AbsoluteTransforms[jointIndex + 1] = pose->m_AbsoluteTransforms[joints[jointIndex].Parent + 1] * pose->m_RelativeTransforms[jointIndex];
                }
            }

            #endif
        }
    }
}

void SkinningSystem_ECS::UpdateSockets()
{
    using Query = ECS::Query<>
        ::ReadOnly<SocketComponent>
        ::Required<TransformComponent>
        ;

    Float3x3 rotation;

    for (Query::Iterator it(*m_World); it; it++)
    {
        SocketComponent const* sockets = it.Get<SocketComponent>();
        TransformComponent* transforms = it.Get<TransformComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            auto& socket = sockets[i];
            auto& transform = transforms[i];

            if (socket.Pose)
            {
                // TODO: Сейчас мы считаем, что SocketIndex == JointIndex. Скорее всего, правильно было бы сокеты хранить
                // отдельно от костей.
                auto& socket_transform = socket.Pose->GetJointTransform(socket.SocketIndex);

                // TODO: Убрать декомпозицию, хранить в позе не матрицы, а по отдельности: позицию, ориентацию, масштаб.
                socket_transform.DecomposeAll(transform.Position, rotation, transform.Scale);
                transform.Rotation.FromMatrix(rotation);
            }
        }
    }
}

void SkinningSystem_ECS::UpdateSkins()
{
    // TODO: Update skins only if object visible? Update only bounding box?

    using Query = ECS::Query<>
        ::ReadOnly<SkeletonPoseComponent>
        ;

    StreamedMemoryGPU* streamedMemory = GameApplication::GetFrameLoop().GetStreamedMemoryGPU();

    for (Query::Iterator it(*m_World); it; it++)
    {
        SkeletonPoseComponent const* poseComponent = it.Get<SkeletonPoseComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            SkeletonPose* pose = poseComponent[i].Pose;

            SkeletonResource const* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
            if (!skeleton)
                continue;

            MeshResource const* meshResource = GameApplication::GetResourceManager().TryGet(poseComponent[i].Mesh);
            if (!meshResource)
                continue;

            MeshSkin const& skin = meshResource->GetSkin();

            pose->m_SkeletonSize = skin.JointIndices.Size() * sizeof(Float3x4);
            if (pose->m_SkeletonSize > 0)
            {
                // Write joints from previous frame
                pose->m_SkeletonOffsetMB = streamedMemory->AllocateJoint(pose->m_SkeletonSize, pose->m_SkinningTransforms);

                // Write joints from current frame
                pose->m_SkeletonOffset = streamedMemory->AllocateJoint(pose->m_SkeletonSize, nullptr);
                Float3x4* data = (Float3x4*)streamedMemory->Map(pose->m_SkeletonOffset);
                for (int j = 0; j < skin.JointIndices.Size(); j++)
                {
                    int jointIndex = skin.JointIndices[j];
                    data[j] = pose->m_SkinningTransforms[j] = pose->m_AbsoluteTransforms[jointIndex + 1] * skin.OffsetMatrices[j];
                }
            }
            else
            {
                pose->m_SkeletonOffset = pose->m_SkeletonOffsetMB = 0;
            }
        }
    }
}

void SkinningSystem_ECS::DrawDebug(DebugRenderer& renderer)
{
    // Draw skeleton
    if (com_DrawSkeleton)
    {
        renderer.SetColor(Color4(1, 0, 0, 1));
        renderer.SetDepthTest(false);

        using Query = ECS::Query<>
            ::ReadOnly<SkeletonPoseComponent>
            ::ReadOnly<WorldTransformComponent>;

        Float3x4 tranformMat;

        for (Query::Iterator it(*m_World); it; it++)
        {
            SkeletonPoseComponent const* poseComponent = it.Get<SkeletonPoseComponent>();
            WorldTransformComponent const* transform = it.Get<WorldTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                SkeletonPose* pose = poseComponent[i].Pose;

                SkeletonResource* skeleton = GameApplication::GetResourceManager().TryGet(pose->Skeleton);
                if (skeleton)
                {
                    TVector<SkeletonJoint> const& joints = skeleton->GetJoints();

                    tranformMat.Compose(transform[i].Position[m_FrameIndex], transform[i].Rotation[m_FrameIndex].ToMatrix3x3(), transform[i].Scale[m_FrameIndex]);

                    for (int jointIndex = 0, count = joints.Size(); jointIndex < count; ++jointIndex)
                    {
                        SkeletonJoint const& joint = joints[jointIndex];

                        Float3x4 t = tranformMat * pose->GetJointTransform(jointIndex);
                        Float3 v1 = t.DecomposeTranslation();

                        renderer.DrawOrientedBox(v1, t.DecomposeRotation(), Float3(0.01f));

                        if (joint.Parent >= 0)
                        {
                            Float3 v0 = (tranformMat * pose->GetJointTransform(joint.Parent)).DecomposeTranslation();
                            renderer.DrawLine(v0, v1);
                        }
                    }
                }
            }
        }        
    }

    // TODO: Draw bounding box
}

HK_NAMESPACE_END
