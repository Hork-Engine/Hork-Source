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
#include <Core/IntrusiveLinkedListMacro.h>

HK_CLASS_META(ADrawable)

static void EvaluateRaycastResult(SPrimitiveDef*       Self,
                                  ALevel const*        LightingLevel,
                                  SMeshVertex const*   pVertices,
                                  SMeshVertexUV const* pLightmapVerts,
                                  int                  LightmapBlock,
                                  unsigned int const*  pIndices,
                                  Float3 const&        HitLocation,
                                  Float2 const&        HitUV,
                                  Float3*              Vertices,
                                  Float2&              TexCoord,
                                  Float3&              LightmapSample)
{
    ASceneComponent* primitiveOwner = Self->Owner;
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

ADrawable::ADrawable()
{
    Bounds.Clear();
    WorldBounds.Clear();
    OverrideBoundingBox.Clear();

    Primitive                        = ALevel::AllocatePrimitive();
    Primitive->Owner                 = this;
    Primitive->Type                  = VSD_PRIMITIVE_BOX;
    Primitive->VisGroup              = VISIBILITY_GROUP_DEFAULT;
    Primitive->QueryGroup            = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS | VSD_QUERY_MASK_SHADOW_CAST;
    Primitive->EvaluateRaycastResult = EvaluateRaycastResult;

    bOverrideBounds = false;
    bSkinnedMesh    = false;
    bCastShadow     = true;
    bAllowRaycast   = false;
}

ADrawable::~ADrawable()
{
    ALevel::DeallocatePrimitive(Primitive);
}

void ADrawable::SetVisible(bool _Visible)
{
    if (_Visible)
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

bool ADrawable::IsVisible() const
{
    return !!(Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE);
}

void ADrawable::SetHiddenInLightPass(bool _HiddenInLightPass)
{
    if (_HiddenInLightPass)
    {
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
    else
    {
        Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
}

bool ADrawable::IsHiddenInLightPass() const
{
    return !(Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS);
}

void ADrawable::SetQueryGroup(VSD_QUERY_MASK _UserQueryGroup)
{
    Primitive->QueryGroup |= VSD_QUERY_MASK(_UserQueryGroup & 0xffff0000);
}

void ADrawable::SetSurfaceFlags(uint8_t _Flags)
{
    Primitive->Flags = _Flags;
}

uint8_t ADrawable::GetSurfaceFlags() const
{
    return Primitive->Flags;
}

void ADrawable::SetFacePlane(PlaneF const& _Plane)
{
    Primitive->Face = _Plane;
}

PlaneF const& ADrawable::GetFacePlane() const
{
    return Primitive->Face;
}

void ADrawable::ForceOverrideBounds(bool _OverrideBounds)
{
    if (bOverrideBounds == _OverrideBounds)
    {
        return;
    }

    bOverrideBounds = _OverrideBounds;

    UpdateWorldBounds();
}

void ADrawable::SetBoundsOverride(BvAxisAlignedBox const& _Bounds)
{
    OverrideBoundingBox = _Bounds;

    if (bOverrideBounds)
    {
        UpdateWorldBounds();
    }
}

BvAxisAlignedBox const& ADrawable::GetBounds() const
{
    return bOverrideBounds ? OverrideBoundingBox : Bounds;
}

BvAxisAlignedBox const& ADrawable::GetWorldBounds() const
{
    return WorldBounds;
}

void ADrawable::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateWorldBounds();
}

void ADrawable::InitializeComponent()
{
    Super::InitializeComponent();

    GetLevel()->AddPrimitive(Primitive);

    UpdateWorldBounds();

    if (bCastShadow)
    {
        GetWorld()->GetRender().ShadowCasters.Add(this);
    }
}

void ADrawable::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetLevel()->RemovePrimitive(Primitive);

    if (bCastShadow)
    {
        GetWorld()->GetRender().ShadowCasters.Remove(this);
    }
}

void ADrawable::SetCastShadow(bool _CastShadow)
{
    if (bCastShadow == _CastShadow)
    {
        return;
    }

    bCastShadow = _CastShadow;

    if (bCastShadow)
    {
        Primitive->QueryGroup |= VSD_QUERY_MASK_SHADOW_CAST;
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_NO_SHADOW_CAST;
    }
    else
    {
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_SHADOW_CAST;
        Primitive->QueryGroup |= VSD_QUERY_MASK_NO_SHADOW_CAST;
    }

    if (IsInitialized())
    {
        ARenderWorld& RenderWorld = GetWorld()->GetRender();

        if (bCastShadow)
        {
            RenderWorld.ShadowCasters.Add(this);
        }
        else
        {
            RenderWorld.ShadowCasters.Remove(this);
        }
    }
}

void ADrawable::UpdateWorldBounds()
{
    BvAxisAlignedBox const& boundingBox = GetBounds();

    WorldBounds = boundingBox.Transform(GetWorldTransformMatrix());

    Primitive->Box = WorldBounds;

    if (IsInitialized())
    {
        GetLevel()->MarkPrimitive(Primitive);
    }
}

void ADrawable::ForceOutdoor(bool _OutdoorSurface)
{
    if (Primitive->bIsOutdoor == _OutdoorSurface)
    {
        return;
    }

    Primitive->bIsOutdoor = _OutdoorSurface;

    if (IsInitialized())
    {
        GetLevel()->MarkPrimitive(Primitive);
    }
}

bool ADrawable::IsOutdoor() const
{
    return Primitive->bIsOutdoor;
}

void ADrawable::PreRenderUpdate(SRenderFrontendDef const* _Def)
{
    if (VisFrame != _Def->FrameNumber)
    {
        VisFrame = _Def->FrameNumber;

        OnPreRenderUpdate(_Def);
    }
}

bool ADrawable::Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<STriangleHitResult>& Hits) const
{
    if (!Primitive->RaycastCallback)
    {
        return false;
    }

    Hits.Clear();

    return Primitive->RaycastCallback(Primitive, InRayStart, InRayEnd, Hits);
}

bool ADrawable::RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, STriangleHitResult& Hit) const
{
    if (!Primitive->RaycastClosestCallback)
    {
        return false;
    }

    SMeshVertex const* pVertices;

    return Primitive->RaycastClosestCallback(Primitive, InRayStart, InRayEnd, Hit, &pVertices);
}
