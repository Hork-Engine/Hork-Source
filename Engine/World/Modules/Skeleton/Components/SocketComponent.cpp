#include "SocketComponent.h"

#include <Engine/World/GameObject.h>
#include <Engine/World/DebugRenderer.h>
#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSockets("com_DrawSockets"s, "0"s, CVAR_CHEAT);

void SocketComponent::FixedUpdate()
{
    if (Pose)
    {
        // TODO: Сейчас мы считаем, что SocketIndex == JointIndex. Скорее всего, правильно было бы сокеты хранить
        // отдельно от костей.
        auto& socketTransform = Pose->GetJointTransform(SocketIndex);

        Float3 position, scale;
        Float3x3 rotationMat;
        Quat rotation;

        // TODO: Убрать декомпозицию, хранить в позе не матрицы, а по отдельности: позицию, ориентацию, масштаб.
        socketTransform.DecomposeAll(position, rotationMat, scale);
        rotation.FromMatrix(rotationMat);

        GetOwner()->SetTransform(position, rotation, scale);
    }
}

void SocketComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawSockets)
    {
        renderer.SetDepthTest(false);
        renderer.DrawAxis(GetOwner()->GetWorldTransformMatrix(), true);
    }
}

HK_NAMESPACE_END
