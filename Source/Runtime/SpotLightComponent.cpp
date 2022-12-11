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

#include "SpotLightComponent.h"
#include "MeshComponent.h"
#include "World.h"
#include "DebugRenderer.h"
#include "ResourceManager.h"

#include <Core/ConsoleVar.h>

static const float DEFAULT_RADIUS           = 15; //1.0f;
static const float DEFAULT_INNER_CONE_ANGLE = 100.0f;
static const float DEFAULT_OUTER_CONE_ANGLE = 120.0f;
static const float DEFAULT_SPOT_EXPONENT    = 1.0f;
static const float MIN_CONE_ANGLE           = 1.0f;
static const float MIN_RADIUS               = 0.01f;

ConsoleVar com_DrawSpotLights("com_DrawSpotLights"s, "0"s, CVAR_CHEAT);

HK_BEGIN_CLASS_META(SpotLightComponent)
HK_PROPERTY(Radius, SetRadius, GetRadius, HK_PROPERTY_DEFAULT)
HK_PROPERTY(InnerConeAngle, SetInnerConeAngle, GetInnerConeAngle, HK_PROPERTY_DEFAULT)
HK_PROPERTY(OuterConeAngle, SetOuterConeAngle, GetOuterConeAngle, HK_PROPERTY_DEFAULT)
HK_PROPERTY(SpotExponent, SetSpotExponent, GetSpotExponent, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

SpotLightComponent::SpotLightComponent()
{
    m_Radius = DEFAULT_RADIUS;
    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    m_InnerConeAngle = DEFAULT_INNER_CONE_ANGLE;
    m_OuterConeAngle = DEFAULT_OUTER_CONE_ANGLE;
    m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
    m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));
    m_SpotExponent = DEFAULT_SPOT_EXPONENT;

    UpdateWorldBounds();
}

void SpotLightComponent::OnCreateAvatar()
{
    Super::OnCreateAvatar();

    // TODO: Create mesh or sprite for avatar
    static TStaticResourceFinder<IndexedMesh> Mesh("/Default/Meshes/Cone"s);
    static TStaticResourceFinder<MaterialInstance> MaterialInstance("AvatarMaterialInstance"s);

    MeshRenderView* meshRender = NewObj<MeshRenderView>();
    meshRender->SetMaterial(MaterialInstance);

    MeshComponent* meshComponent = GetOwnerActor()->CreateComponent<MeshComponent>("SpotLightAvatar");
    meshComponent->SetMotionBehavior(MB_KINEMATIC);
    meshComponent->SetCollisionGroup(CM_NOCOLLISION);
    meshComponent->SetMesh(Mesh.GetObject());
    meshComponent->SetRenderView(meshRender);
    meshComponent->SetCastShadow(false);
    meshComponent->SetAbsoluteScale(true);
    meshComponent->SetAngles(90, 0, 0);
    meshComponent->SetScale(0.1f);
    meshComponent->AttachTo(this);
    meshComponent->SetHideInEditor(true);
}

void SpotLightComponent::SetRadius(float radius)
{
    m_Radius = Math::Max(MIN_RADIUS, radius);
    m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);

    UpdateWorldBounds();
}

float SpotLightComponent::GetRadius() const
{
    return m_Radius;
}

void SpotLightComponent::SetInnerConeAngle(float angle)
{
    m_InnerConeAngle = Math::Clamp(angle, MIN_CONE_ANGLE, 180.0f);
    m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
}

float SpotLightComponent::GetInnerConeAngle() const
{
    return m_InnerConeAngle;
}

void SpotLightComponent::SetOuterConeAngle(float angle)
{
    m_OuterConeAngle = Math::Clamp(angle, MIN_CONE_ANGLE, 180.0f);
    m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));

    UpdateWorldBounds();
}

float SpotLightComponent::GetOuterConeAngle() const
{
    return m_OuterConeAngle;
}

void SpotLightComponent::SetSpotExponent(float exponent)
{
    m_SpotExponent = exponent;
}

float SpotLightComponent::GetSpotExponent() const
{
    return m_SpotExponent;
}

void SpotLightComponent::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void SpotLightComponent::UpdateWorldBounds()
{
    const float  ToHalfAngleRadians = 0.5f / 180.0f * Math::_PI;
    const float  HalfConeAngle      = m_OuterConeAngle * ToHalfAngleRadians;
    const Float3 WorldPos           = GetWorldPosition();
    const float  SinHalfConeAngle   = Math::Sin(HalfConeAngle);

    // Compute cone OBB for voxelization
    m_OBBWorldBounds.Orient = GetWorldRotation().ToMatrix3x3();

    const Float3 SpotDir = -m_OBBWorldBounds.Orient[2];

    //m_OBBWorldBounds.HalfSize.X = m_OBBWorldBounds.HalfSize.Y = tan( HalfConeAngle ) * m_Radius;
    m_OBBWorldBounds.HalfSize.X = m_OBBWorldBounds.HalfSize.Y = SinHalfConeAngle * m_Radius;
    m_OBBWorldBounds.HalfSize.Z = m_Radius * 0.5f;
    m_OBBWorldBounds.Center = WorldPos + SpotDir * (m_OBBWorldBounds.HalfSize.Z);

    // TODO: Optimize?
    Float4x4 OBBTransform = Float4x4::Translation(m_OBBWorldBounds.Center) * Float4x4(m_OBBWorldBounds.Orient) * Float4x4::Scale(m_OBBWorldBounds.HalfSize);
    m_OBBTransformInverse = OBBTransform.Inversed();

    // Compute cone AABB for culling
    m_AABBWorldBounds.Clear();
    m_AABBWorldBounds.AddPoint(WorldPos);
    Float3 v = WorldPos + SpotDir * m_Radius;
    Float3 vx = m_OBBWorldBounds.Orient[0] * m_OBBWorldBounds.HalfSize.X;
    Float3 vy = m_OBBWorldBounds.Orient[1] * m_OBBWorldBounds.HalfSize.X;
    m_AABBWorldBounds.AddPoint(v + vx);
    m_AABBWorldBounds.AddPoint(v - vx);
    m_AABBWorldBounds.AddPoint(v + vy);
    m_AABBWorldBounds.AddPoint(v - vy);

    // Посмотреть, как более эффективно распределяется площадь - у сферы или AABB
    // Compute cone Sphere bounds
    if (HalfConeAngle > Math::_PI / 4)
    {
        m_SphereWorldBounds.Radius = SinHalfConeAngle * m_Radius;
        m_SphereWorldBounds.Center = WorldPos + SpotDir * (m_CosHalfOuterConeAngle * m_Radius);
    }
    else
    {
        m_SphereWorldBounds.Radius = m_Radius / (2.0 * m_CosHalfOuterConeAngle);
        m_SphereWorldBounds.Center = WorldPos + SpotDir * m_SphereWorldBounds.Radius;
    }

    m_Primitive->Sphere = m_SphereWorldBounds;

    if (IsInitialized())
    {
        GetWorld()->VisibilitySystem.MarkPrimitive(m_Primitive);
    }
}

void SpotLightComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawSpotLights)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            Float3   pos    = GetWorldPosition();
            Float3x3 orient = GetWorldRotation().ToMatrix3x3();
            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
            InRenderer->DrawCone(pos, orient, m_Radius, Math::Radians(m_InnerConeAngle) * 0.5f);
            InRenderer->SetColor(Color4(1, 1, 1, 1));
            InRenderer->DrawCone(pos, orient, m_Radius, Math::Radians(m_OuterConeAngle) * 0.5f);
        }
    }
}

void SpotLightComponent::PackLight(Float4x4 const& InViewMatrix, LightParameters& Light)
{
    PhotometricProfile* profile = GetPhotometricProfile();

    Light.Position              = Float3(InViewMatrix * GetWorldPosition());
    Light.Radius                = GetRadius();
    Light.CosHalfOuterConeAngle = m_CosHalfOuterConeAngle;
    Light.CosHalfInnerConeAngle = m_CosHalfInnerConeAngle;
    Light.InverseSquareRadius   = m_InverseSquareRadius;
    Light.Direction             = InViewMatrix.TransformAsFloat3x3(-GetWorldDirection());
    Light.SpotExponent          = m_SpotExponent;
    Light.Color                 = GetEffectiveColor(Math::Min(m_CosHalfOuterConeAngle, 0.9999f));
    Light.LightType             = CLUSTER_LIGHT_SPOT;
    Light.RenderMask            = ~0u; //RenderMask; // TODO
    Light.PhotometricProfile    = profile ? profile->GetPhotometricProfileIndex() : 0xffffffff;
}
