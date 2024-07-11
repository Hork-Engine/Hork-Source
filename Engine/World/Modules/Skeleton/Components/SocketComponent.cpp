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

void SocketComponent::LateUpdate()
{
    if (Pose && JointIndex < Pose->m_ModelMatrices.Size())
    {
        SimdFloat4x4 transform = Pose->m_ModelMatrices[JointIndex] * SimdFloat4x4::Translation(Simd::LoadFloat4(Offset.X, Offset.Y, Offset.Z, 0.0f));

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
