#include "PlayerControlSystem.h"
#include "../Components/TransformComponent.h"
#include "../Components/CharacterControllerComponent.h"
#include "../Components/FinalTransformComponent.h"
#include "../Components/ExperimentalComponents.h"

#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

PlayerControlSystem::PlayerControlSystem(ECS::World* world) :
    m_World(world)
{}

void PlayerControlSystem::Update(float timeStep)
{
    auto& inputState = GameApplication::GetInputSystem().GetInputState();

    using Query_CharacterUpdate = ECS::Query<>
        ::Required<PlayerControlComponent>
        ::Required<CharacterControllerComponent>;

    const float TurnSpeed = 10; //20;

    for (Query_CharacterUpdate::Iterator it(*m_World); it; it++)
    {
        PlayerControlComponent* playerControl = it.Get<PlayerControlComponent>();
        CharacterControllerComponent* character = it.Get<CharacterControllerComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            auto playerNum = playerControl[i].PlayerNum;

            Float3 dir;

            if (character[i].AttackTime > 0)
            {
                character[i].AttackTime -= timeStep;
                if (character[i].AttackTime < 0)
                    character[i].AttackTime = 0;
            }

            ECS::EntityView skelEntityView = m_World->GetEntityView(playerControl[i].SkeletonControl);
            ECS::EntityView pivotEntityView = m_World->GetEntityView(playerControl[i].Pivot);

            TransformComponent* pivotTransform = pivotEntityView.GetComponent<TransformComponent>();
            TransformComponent* skelTransform = skelEntityView.GetComponent<TransformComponent>();

            if (playerControl[i].TargetYaw != 1024)
            {
                Quat current = Quat::RotationAroundNormal(playerControl[i].CameraYaw, Float3(0, 1, 0));
                Quat target = Quat::RotationAroundNormal(playerControl[i].TargetYaw, Float3(0, 1, 0));

                Float3 forward = Math::Slerp(current, target, timeStep * 10).ZAxis();
                float angle = std::atan2(forward.X, forward.Z);

                playerControl[i].CameraYaw = angle;

                if (current == target)
                {
                    playerControl[i].CameraYaw = playerControl[i].TargetYaw;
                    playerControl[i].TargetYaw = 1024;
                }
                pivotTransform->Rotation.FromAngles(playerControl[i].CameraPitch, playerControl[i].CameraYaw, 0);
            }

            if (character[i].AttackTime > 0)
            {
                if (skelTransform)
                {
                    skelTransform->Rotation = Math::Slerp(skelTransform->Rotation, playerControl[i].MeshRotation, timeStep * TurnSpeed);
                }
            }

            if (character[i].AttackTime != 0)
                continue;            

            SkeletonControllerComponent* skeletonController = skelEntityView.GetComponent<SkeletonControllerComponent>();
            auto layer = skeletonController->AnimInstance->GetBlendMachine()->GetLayerIndex("Main");            

            if (inputState.GetAxisScale("Attack", playerNum))
            {
                if (inputState.GetAxisScale("MoveForward", playerNum))
                    skeletonController->AnimInstance->ChangeLayerState(layer, "Attack1");
                else
                    skeletonController->AnimInstance->ChangeLayerState(layer, "Attack2");

                character[i].AttackTime = 30.0f / 24.0f;

                Float3 forward = -playerControl[i].MeshRotation.ZAxis();
                float angle = std::atan2(forward.X, forward.Z);

                playerControl[i].TargetYaw = angle;
            }
            else
            {
                playerControl[i].TargetYaw = 1024;
            }

            character[i].Jump = false;
            character[i].MovementDirection.Clear();

            if (character[i].AttackTime != 0)
                continue;

            float moveForward = inputState.GetAxisScale("MoveForward", playerNum);
            if (moveForward)
            {
                dir.Z = Math::Sign(-moveForward);
            }

            float moveRight = inputState.GetAxisScale("MoveRight", playerNum);
            if (moveRight)
            {
                dir.X = Math::Sign(moveRight);
            }

            if (inputState.GetAxisScale("MoveUp", playerNum) > 0.0f)
            {
                character[i].Jump = true;
            }
            if (pivotTransform)
            {
                const float RotationSpeed = 0.01f;

                float turnRight = inputState.GetAxisScale("TurnRight", playerNum);
                float turnUp = inputState.GetAxisScale("TurnUp", playerNum);

                if (turnRight || turnUp)
                {
                    playerControl[i].CameraYaw -= turnRight * RotationSpeed;
                    while (playerControl[i].CameraYaw < -Math::_PI)
                        playerControl[i].CameraYaw += Math::_2PI;
                    while (playerControl[i].CameraYaw > Math::_PI)
                        playerControl[i].CameraYaw -= Math::_2PI;

                    constexpr float maxPitch = Math::Radians(35);
                    constexpr float minPitch = Math::Radians(-75);

                    playerControl[i].CameraPitch += turnUp * RotationSpeed;
                    if (playerControl[i].CameraPitch > maxPitch)
                        playerControl[i].CameraPitch = maxPitch;
                    else if (playerControl[i].CameraPitch < minPitch)
                        playerControl[i].CameraPitch = minPitch;

                    pivotTransform->Rotation.FromAngles(playerControl[i].CameraPitch, playerControl[i].CameraYaw, 0);
                }
            }

            if (moveForward || moveRight)
            {
                float angle = std::atan2(dir.X, dir.Z);
                playerControl[i].MeshRotation.FromAngles(0, playerControl[i].CameraYaw + angle, 0);

                character[i].MovementDirection = playerControl[i].MeshRotation.ZAxis();
            }

            if (skelTransform)
            {
                skelTransform->Rotation = Math::Slerp(skelTransform->Rotation, playerControl[i].MeshRotation, timeStep * TurnSpeed);
            }

            bool hasMove = character[i].MovementDirection.LengthSqr() > 0;

            bool run = inputState.GetAxisScale("Run", playerNum) > 0;

            if (run)
                character[i].cCharacterSpeed = 4; // run
            else
                character[i].cCharacterSpeed = 1.5f; // walk

            if (skeletonController)
            {
                if (character[i].m_pCharacter->GetGroundState() == JPH::CharacterVirtual::EGroundState::InAir)
                {
                    skeletonController->AnimInstance->ChangeLayerState(layer, "Jump");
                }
                else
                {
                    if (hasMove)
                    {
                        if (run)
                            skeletonController->AnimInstance->ChangeLayerState(layer, "Run");
                        else
                            skeletonController->AnimInstance->ChangeLayerState(layer, "Walk");
                    }
                    else
                    {
                        skeletonController->AnimInstance->ChangeLayerState(layer, "Idle");
                    }
                }
            }

            ECS::EntityView sprintArmView = m_World->GetEntityView(playerControl[i].SpringArm);
            SpringArmComponent* springArm = sprintArmView.GetComponent<SpringArmComponent>();
            if (springArm)
            {
                if (!hasMove)
                    springArm->DesiredDistance = 2.0f;
                else if (run)
                    springArm->DesiredDistance = 3;
                else
                    springArm->DesiredDistance = 2.5f;
            }            

            //for (InputState::Action const& action : inputState.m_ActionPool[playerNum])
            //{
            //    if (action.bPressed)
            //    {
            //        // Do something on press
            //        LOG("PRESS {}\n", action.Name);
            //    }
            //    else
            //    {
            //        // Do something on release
            //        LOG("RELEASE {}\n", action.Name);
            //    }
            //}
        }
    }
}

HK_NAMESPACE_END
