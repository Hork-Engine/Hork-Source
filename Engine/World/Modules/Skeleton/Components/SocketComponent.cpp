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
