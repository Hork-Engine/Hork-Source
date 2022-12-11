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

#include "PointLightComponent.h"
#include "MeshComponent.h"
#include "World.h"
#include "DebugRenderer.h"
#include "ResourceManager.h"

#include <Core/ConsoleVar.h>

static const float DEFAULT_RADIUS = 15; //1.0f;
static const float MIN_RADIUS     = 0.01f;

ConsoleVar com_DrawPointLights("com_DrawPointLights"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(PointLightComponent)

PointLightComponent::PointLightComponent()
{
    m_Radius = DEFAULT_RADIUS;
    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);

    UpdateWorldBounds();
}

void PointLightComponent::OnCreateAvatar()
{
    Super::OnCreateAvatar();

    // TODO: Create mesh or sprite for point light avatar
    static TStaticResourceFinder<IndexedMesh> Mesh("/Default/Meshes/Sphere"s);
    static TStaticResourceFinder<MaterialInstance> MaterialInstance("AvatarMaterialInstance"s);

    MeshRenderView* meshRender = NewObj<MeshRenderView>();
    meshRender->SetMaterial(MaterialInstance);

    MeshComponent* meshComponent = GetOwnerActor()->CreateComponent<MeshComponent>("PointLightAvatar");
    meshComponent->SetMotionBehavior(MB_KINEMATIC);
    meshComponent->SetCollisionGroup(CM_NOCOLLISION);
    meshComponent->SetMesh(Mesh.GetObject());
    meshComponent->SetRenderView(meshRender);
    meshComponent->SetCastShadow(false);
    meshComponent->SetAbsoluteScale(true);
    meshComponent->SetAbsoluteRotation(true);
    meshComponent->SetScale(0.1f);
    meshComponent->AttachTo(this);
    meshComponent->SetHideInEditor(true);
}

void PointLightComponent::SetRadius(float _Radius)
{
    m_Radius = Math::Max(MIN_RADIUS, _Radius);
    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);

    UpdateWorldBounds();
}

void PointLightComponent::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void PointLightComponent::UpdateWorldBounds()
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

void PointLightComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawPointLights)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            Float3 pos = GetWorldPosition();

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 1, 1, 1));
            InRenderer->DrawSphere(pos, m_Radius);
        }
    }
}

void PointLightComponent::PackLight(Float4x4 const& InViewMatrix, LightParameters& Light)
{
    Light.Position              = Float3(InViewMatrix * GetWorldPosition());
    Light.Radius                = GetRadius();
    Light.CosHalfOuterConeAngle = 0;
    Light.CosHalfInnerConeAngle = 0;
    Light.InverseSquareRadius   = m_InverseSquareRadius;
    Light.Direction             = InViewMatrix.TransformAsFloat3x3(-GetWorldDirection()); // Only for photometric light
    Light.SpotExponent          = 0;
    Light.Color                 = GetEffectiveColor(-1.0f);
    Light.LightType             = CLUSTER_LIGHT_POINT;
    Light.RenderMask            = ~0u; //RenderMask; // TODO
    PhotometricProfile* profile = GetPhotometricProfile();
    Light.PhotometricProfile    = profile ? profile->GetPhotometricProfileIndex() : 0xffffffff;
}
