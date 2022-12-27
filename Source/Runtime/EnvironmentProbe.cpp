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

HK_NAMESPACE_BEGIN

constexpr float DEFAULT_RADIUS = 1.0f;

ConsoleVar com_DrawEnvironmentProbes("com_DrawEnvironmentProbes"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(EnvironmentProbe)

EnvironmentProbe::EnvironmentProbe()
{
    m_AABBWorldBounds.Clear();
    m_OBBTransformInverse.Clear();

    m_Primitive = VisibilitySystem::AllocatePrimitive();
    m_Primitive->Owner = this;
    m_Primitive->Type = VSD_PRIMITIVE_SPHERE;
    m_Primitive->VisGroup = VISIBILITY_GROUP_DEFAULT;
    m_Primitive->QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;

    m_Radius = DEFAULT_RADIUS;

    UpdateWorldBounds();
}

EnvironmentProbe::~EnvironmentProbe()
{
    VisibilitySystem::DeallocatePrimitive(m_Primitive);
}

void EnvironmentProbe::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->VisibilitySystem.AddPrimitive(m_Primitive);
}

void EnvironmentProbe::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->VisibilitySystem.RemovePrimitive(m_Primitive);
}

void EnvironmentProbe::SetEnabled(bool _Enabled)
{
    m_bEnabled = _Enabled;

    if (_Enabled)
    {
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    }
    else
    {
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

void EnvironmentProbe::SetRadius(float _Radius)
{
    m_Radius = Math::Max(0.001f, _Radius);

    UpdateWorldBounds();
}

void EnvironmentProbe::SetEnvironmentMap(EnvironmentMap* _EnvironmentMap)
{
    m_EnvironmentMap = _EnvironmentMap;

    if (m_EnvironmentMap)
    {
        m_IrradianceMapHandle = m_EnvironmentMap->GetIrradianceHandle();
        m_ReflectionMapHandle = m_EnvironmentMap->GetReflectionHandle();
    }
    else
    {
        m_IrradianceMapHandle = 0;
        m_ReflectionMapHandle = 0;
    }
}

void EnvironmentProbe::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void EnvironmentProbe::UpdateWorldBounds()
{
    m_SphereWorldBounds.Radius = m_Radius;
    m_SphereWorldBounds.Center = GetWorldPosition();
    m_AABBWorldBounds.Mins = m_SphereWorldBounds.Center - m_Radius;
    m_AABBWorldBounds.Maxs = m_SphereWorldBounds.Center + m_Radius;
    m_OBBWorldBounds.Center = m_SphereWorldBounds.Center;
    m_OBBWorldBounds.HalfSize = Float3(m_SphereWorldBounds.Radius);
    m_OBBWorldBounds.Orient.SetIdentity();

    // TODO: Optimize?
    Float4x4 OBBTransform = Float4x4::Translation(m_OBBWorldBounds.Center) * Float4x4::Scale(m_OBBWorldBounds.HalfSize);
    m_OBBTransformInverse = OBBTransform.Inversed();

    m_Primitive->Sphere = m_SphereWorldBounds;

    if (IsInitialized())
    {
        GetWorld()->VisibilitySystem.MarkPrimitive(m_Primitive);
    }
}

void EnvironmentProbe::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawEnvironmentProbes)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            Float3 pos = GetWorldPosition();

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 0, 1, 1));
            InRenderer->DrawSphere(pos, m_Radius);
        }
    }
}

void EnvironmentProbe::PackProbe(Float4x4 const& InViewMatrix, ProbeParameters& Probe)
{
    Probe.Position = Float3(InViewMatrix * GetWorldPosition());
    Probe.Radius = m_Radius;
    Probe.IrradianceMap = m_IrradianceMapHandle;
    Probe.ReflectionMap = m_ReflectionMapHandle;
}

HK_NAMESPACE_END
