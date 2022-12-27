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

#include "Drawable.h"
#include "World.h"
#include "RenderFrontend.h"
#include <Core/IntrusiveLinkedListMacro.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(Drawable)

static void EvaluateRaycastResult(PrimitiveDef*       Self,
                                  Level const*        LightingLevel,
                                  MeshVertex const*   pVertices,
                                  MeshVertexUV const* pLightmapVerts,
                                  int                 LightmapBlock,
                                  unsigned int const* pIndices,
                                  Float3 const&       HitLocation,
                                  Float2 const&       HitUV,
                                  Float3*             Vertices,
                                  Float2&             TexCoord,
                                  Float3&             LightmapSample)
{
    SceneComponent* primitiveOwner = Self->Owner;
    Float3x4 const&  transform      = primitiveOwner->GetWorldTransformMatrix();

    Float3 const& v0 = pVertices[pIndices[0]].Position;
    Float3 const& v1 = pVertices[pIndices[1]].Position;
    Float3 const& v2 = pVertices[pIndices[2]].Position;

    // transform triangle vertices to worldspace
    Vertices[0] = transform * v0;
    Vertices[1] = transform * v1;
    Vertices[2] = transform * v2;

    const float hitW = 1.0f - HitUV[0] - HitUV[1];

    Float2 uv0 = pVertices[pIndices[0]].GetTexCoord();
    Float2 uv1 = pVertices[pIndices[1]].GetTexCoord();
    Float2 uv2 = pVertices[pIndices[2]].GetTexCoord();
    TexCoord   = uv0 * hitW + uv1 * HitUV[0] + uv2 * HitUV[1];

    if (pLightmapVerts && LightingLevel && LightmapBlock >= 0)
    {
        Float2 const& lm0             = pLightmapVerts[pIndices[0]].TexCoord;
        Float2 const& lm1             = pLightmapVerts[pIndices[1]].TexCoord;
        Float2 const& lm2             = pLightmapVerts[pIndices[2]].TexCoord;
        Float2        lighmapTexcoord = lm0 * hitW + lm1 * HitUV[0] + lm2 * HitUV[1];

        LightmapSample = LightingLevel->SampleLight(LightmapBlock, lighmapTexcoord);
    }
    else
    {
        LightmapSample = Float3(0.0f);
    }
}

Drawable::Drawable()
{
    m_Bounds.Clear();
    m_WorldBounds.Clear();
    m_OverrideBoundingBox.Clear();

    m_Primitive = VisibilitySystem::AllocatePrimitive();
    m_Primitive->Owner = this;
    m_Primitive->Type = VSD_PRIMITIVE_BOX;
    m_Primitive->VisGroup = VISIBILITY_GROUP_DEFAULT;
    m_Primitive->QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS | VSD_QUERY_MASK_SHADOW_CAST;
    m_Primitive->EvaluateRaycastResult = EvaluateRaycastResult;

    m_bOverrideBounds = false;
    m_bSkinnedMesh = false;
    m_bCastShadow = true;
    m_bAllowRaycast = false;
}

Drawable::~Drawable()
{
    VisibilitySystem::DeallocatePrimitive(m_Primitive);
}

void Drawable::SetVisible(bool _Visible)
{
    if (_Visible)
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

bool Drawable::IsVisible() const
{
    return !!(m_Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE);
}

void Drawable::SetHiddenInLightPass(bool _HiddenInLightPass)
{
    if (_HiddenInLightPass)
    {
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
    else
    {
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
}

bool Drawable::IsHiddenInLightPass() const
{
    return !(m_Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS);
}

void Drawable::SetQueryGroup(VSD_QUERY_MASK _UserQueryGroup)
{
    m_Primitive->QueryGroup |= VSD_QUERY_MASK(_UserQueryGroup & 0xffff0000);
}

void Drawable::SetSurfaceFlags(SURFACE_FLAGS _Flags)
{
    m_Primitive->Flags = _Flags;
}

SURFACE_FLAGS Drawable::GetSurfaceFlags() const
{
    return m_Primitive->Flags;
}

void Drawable::SetFacePlane(PlaneF const& _Plane)
{
    m_Primitive->Face = _Plane;
}

PlaneF const& Drawable::GetFacePlane() const
{
    return m_Primitive->Face;
}

void Drawable::ForceOverrideBounds(bool _OverrideBounds)
{
    if (m_bOverrideBounds == _OverrideBounds)
    {
        return;
    }

    m_bOverrideBounds = _OverrideBounds;

    UpdateWorldBounds();
}

void Drawable::SetBoundsOverride(BvAxisAlignedBox const& _Bounds)
{
    m_OverrideBoundingBox = _Bounds;

    if (m_bOverrideBounds)
    {
        UpdateWorldBounds();
    }
}

BvAxisAlignedBox const& Drawable::GetBounds() const
{
    return m_bOverrideBounds ? m_OverrideBoundingBox : m_Bounds;
}

BvAxisAlignedBox const& Drawable::GetWorldBounds() const
{
    return m_WorldBounds;
}

void Drawable::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void Drawable::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->VisibilitySystem.AddPrimitive(m_Primitive);

    UpdateWorldBounds();

    if (m_bCastShadow)
    {
        GetWorld()->LightingSystem.ShadowCasters.Add(this);
    }
}

void Drawable::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->VisibilitySystem.RemovePrimitive(m_Primitive);

    if (m_bCastShadow)
    {
        GetWorld()->LightingSystem.ShadowCasters.Remove(this);
    }
}

void Drawable::SetCastShadow(bool _CastShadow)
{
    if (m_bCastShadow == _CastShadow)
    {
        return;
    }

    m_bCastShadow = _CastShadow;

    if (m_bCastShadow)
    {
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_SHADOW_CAST;
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_NO_SHADOW_CAST;
    }
    else
    {
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_SHADOW_CAST;
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_NO_SHADOW_CAST;
    }

    if (IsInitialized())
    {
        LightingSystem& LightingSystem = GetWorld()->LightingSystem;

        if (m_bCastShadow)
        {
            LightingSystem.ShadowCasters.Add(this);
        }
        else
        {
            LightingSystem.ShadowCasters.Remove(this);
        }
    }
}

void Drawable::UpdateWorldBounds()
{
    BvAxisAlignedBox const& boundingBox = GetBounds();

    m_WorldBounds = boundingBox.Transform(GetWorldTransformMatrix());

    m_Primitive->Box = m_WorldBounds;

    if (IsInitialized())
    {
        GetWorld()->VisibilitySystem.MarkPrimitive(m_Primitive);
    }
}

void Drawable::ForceOutdoor(bool _OutdoorSurface)
{
    if (m_Primitive->bIsOutdoor == _OutdoorSurface)
    {
        return;
    }

    m_Primitive->bIsOutdoor = _OutdoorSurface;

    if (IsInitialized())
    {
        GetWorld()->VisibilitySystem.MarkPrimitive(m_Primitive);
    }
}

bool Drawable::IsOutdoor() const
{
    return m_Primitive->bIsOutdoor;
}

void Drawable::PreRenderUpdate(RenderFrontendDef const* _Def)
{
    if (m_VisFrame != _Def->FrameNumber)
    {
        m_VisFrame = _Def->FrameNumber;

        OnPreRenderUpdate(_Def);
    }
}

bool Drawable::Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<TriangleHitResult>& Hits) const
{
    if (!m_Primitive->RaycastCallback)
    {
        return false;
    }

    Hits.Clear();

    return m_Primitive->RaycastCallback(m_Primitive, InRayStart, InRayEnd, Hits);
}

bool Drawable::RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit) const
{
    if (!m_Primitive->RaycastClosestCallback)
    {
        return false;
    }

    MeshVertex const* pVertices;

    return m_Primitive->RaycastClosestCallback(m_Primitive, InRayStart, InRayEnd, Hit, &pVertices);
}

HK_NAMESPACE_END
