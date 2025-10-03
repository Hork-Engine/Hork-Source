/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Runtime/World/GameObject.h>
#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSockets("com_DrawSockets"_s, "0"_s, CVAR_CHEAT);

void SocketComponent::BeginPlay()
{
    m_PoseComponent = GetOwner()->GetComponentHandle<SkeletonPoseComponent>();
}

void SocketComponent::LateUpdate()
{
    SkeletonPoseComponent* poseComponent = GetWorld()->GetComponent(m_PoseComponent);
    if (!poseComponent)
        return;

    SkeletonPose* pose = poseComponent->GetPose();
    if (!pose)
        return;

    if (JointIndex >= pose->m_ModelMatrices.Size())
        return;

    SimdFloat4x4 transform = pose->m_ModelMatrices[JointIndex] * SimdFloat4x4::Translation(Simd::LoadFloat4(Offset.X, Offset.Y, Offset.Z, 0.0f));

    SimdFloat4 p, r, s;
    if (Simd::Decompose(transform, &p, &r, &s))
    {
        alignas(16) Float4 position;
        alignas(16) Quat rotation;

        Simd::StorePtr(p, &position.X);
        Simd::StorePtr(r, &rotation.X);

        if (bApplyJointScale)
        {
            alignas(16) Float4 scale;
            Simd::StorePtr(s, &scale.X);
            GetOwner()->SetTransform(Float3(position), rotation, Float3(scale));
        }
        else
        {
            GetOwner()->SetPositionAndRotation(Float3(position), rotation);
        }
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
