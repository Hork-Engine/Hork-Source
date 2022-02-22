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

#include "EnvironmentProbe.h"
#include "EnvironmentMap.h"
#include "World.h"
#include "DebugRenderer.h"

#include <Core/ConsoleVar.h>

constexpr float DEFAULT_RADIUS = 1.0f;

AConsoleVar com_DrawEnvironmentProbes(_CTS("com_DrawEnvironmentProbes"), _CTS("0"), CVAR_CHEAT);

HK_CLASS_META(AEnvironmentProbe)

AEnvironmentProbe::AEnvironmentProbe()
{
    AABBWorldBounds.Clear();
    OBBTransformInverse.Clear();

    Primitive             = AVisibilitySystem::AllocatePrimitive();
    Primitive->Owner      = this;
    Primitive->Type       = VSD_PRIMITIVE_SPHERE;
    Primitive->VisGroup   = VISIBILITY_GROUP_DEFAULT;
    Primitive->QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;

    Radius = DEFAULT_RADIUS;

    UpdateWorldBounds();
}

AEnvironmentProbe::~AEnvironmentProbe()
{
    AVisibilitySystem::DeallocatePrimitive(Primitive);
}

void AEnvironmentProbe::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->VisibilitySystem.AddPrimitive(Primitive);
}

void AEnvironmentProbe::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->VisibilitySystem.RemovePrimitive(Primitive);
}

void AEnvironmentProbe::SetEnabled(bool _Enabled)
{
    bEnabled = _Enabled;

    if (_Enabled)
    {
        Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    }
    else
    {
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

void AEnvironmentProbe::SetRadius(float _Radius)
{
    Radius = Math::Max(0.001f, _Radius);

    UpdateWorldBounds();
}

void AEnvironmentProbe::SetEnvironmentMap(AEnvironmentMap* _EnvironmentMap)
{
    EnvironmentMap = _EnvironmentMap;

    if (EnvironmentMap)
    {
        IrradianceMapHandle = EnvironmentMap->GetIrradianceHandle();
        ReflectionMapHandle = EnvironmentMap->GetReflectionHandle();
    }
    else
    {
        IrradianceMapHandle = 0;
        ReflectionMapHandle = 0;
    }
}

void AEnvironmentProbe::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void AEnvironmentProbe::UpdateWorldBounds()
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

    Primitive->Sphere = SphereWorldBounds;

    if (IsInitialized())
    {
        GetWorld()->VisibilitySystem.MarkPrimitive(Primitive);
    }
}

void AEnvironmentProbe::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawEnvironmentProbes)
    {
        if (Primitive->VisPass == InRenderer->GetVisPass())
        {
            Float3 pos = GetWorldPosition();

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 0, 1, 1));
            InRenderer->DrawSphere(pos, Radius);
        }
    }
}

void AEnvironmentProbe::PackProbe(Float4x4 const& InViewMatrix, SProbeParameters& Probe)
{
    Probe.Position      = Float3(InViewMatrix * GetWorldPosition());
    Probe.Radius        = Radius;
    Probe.IrradianceMap = IrradianceMapHandle;
    Probe.ReflectionMap = ReflectionMapHandle;
}
