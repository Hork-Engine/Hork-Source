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

#include "IBLComponent.h"
#include "World.h"
#include "DebugRenderer.h"

#include <Core/ConsoleVar.h>

constexpr float DEFAULT_RADIUS = 1.0f;

AConsoleVar com_DrawIBL(_CTS("com_DrawIBL"), _CTS("0"), CVAR_CHEAT);

AN_CLASS_META(AIBLComponent)

AIBLComponent::AIBLComponent()
{
    Radius = DEFAULT_RADIUS;

    UpdateWorldBounds();
}

void AIBLComponent::SetRadius(float _Radius)
{
    Radius = Math::Max(0.001f, _Radius);

    UpdateWorldBounds();
}

void AIBLComponent::SetIrradianceMap(int _Index)
{
    IrradianceMap = _Index; // TODO: Clamp to possible range
}

void AIBLComponent::SetReflectionMap(int _Index)
{
    ReflectionMap = _Index; // TODO: Clamp to possible range
}

void AIBLComponent::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void AIBLComponent::UpdateWorldBounds()
{
    SphereWorldBounds.Radius = Radius;
    SphereWorldBounds.Center = GetWorldPosition();
    AABBWorldBounds.Mins     = SphereWorldBounds.Center - Radius;
    AABBWorldBounds.Maxs     = SphereWorldBounds.Center + Radius;
    OBBWorldBounds.Center    = SphereWorldBounds.Center;
    OBBWorldBounds.HalfSize  = Float3(SphereWorldBounds.Radius);
    OBBWorldBounds.Orient.SetIdentity();

    // TODO: Optimize?
    Float4x4 OBBTransform = Float4x4::Translation(OBBWorldBounds.Center) * Float4x4::Scale(OBBWorldBounds.HalfSize);
    OBBTransformInverse   = OBBTransform.Inversed();

    Primitive.Sphere = SphereWorldBounds;

    if (IsInitialized())
    {
        GetLevel()->MarkPrimitive(&Primitive);
    }
}

void AIBLComponent::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawIBL)
    {
        if (Primitive.VisPass == InRenderer->GetVisPass())
        {
            Float3 pos = GetWorldPosition();

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 0, 1, 1));
            InRenderer->DrawSphere(pos, Radius);
        }
    }
}

void AIBLComponent::PackProbe(Float4x4 const& InViewMatrix, SProbeParameters& Probe)
{
    Probe.Position      = Float3(InViewMatrix * GetWorldPosition());
    Probe.Radius        = Radius;
    Probe.IrradianceMap = IrradianceMap;
    Probe.ReflectionMap = ReflectionMap;
}
