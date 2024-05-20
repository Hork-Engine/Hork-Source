/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#pragma once

#include <Engine/Renderer/RenderDefs.h>
#include <Engine/World/Common/DebugRenderer.h>
#include <Engine/Canvas/Canvas.h>
#include <Engine/World/Modules/Terrain/TerrainMesh.h>
#include "LightVoxelizer.h"
#include "WorldRenderView.h"

HK_NAMESPACE_BEGIN

class World;

struct RenderFrontendStat
{
    int PolyCount;
    int ShadowMapPolyCount;
    int FrontendTime;
};

struct RenderFrontendDef
{
    WorldRenderView* WorldRV;
    RenderViewData* View;
    BvFrustum const* Frustum;
    VISIBILITY_GROUP VisibilityMask;
    int FrameNumber;
    int PolyCount;
    int ShadowMapPolyCount;
    //int LightPortalPolyCount;
    //int TerrainPolyCount;
    class StreamedMemoryGPU* StreamedMemory;
};

class RenderFrontend : public RefCounted
{
public:
    RenderFrontend();

    void Render(class FrameLoop* FrameLoop, Canvas* InCanvas);

    /** Get render frame data */
    RenderFrameData* GetFrameData() { return &m_FrameData; }

    RenderFrontendStat const& GetStat() const { return m_Stat; }

private:
    void ClearRenderView(RenderViewData* view);
    void RenderView(WorldRenderView* worldRenderView, RenderViewData* view);
    void SortRenderInstances();
    void SortShadowInstances(LightShadowmap const* shadowMap);

    void QueryVisiblePrimitives(World* world);
    void QueryShadowCasters(World* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, TVector<PrimitiveDef*>& Primitives);
    void AddRenderInstances(World* world);

    //bool AddLightShadowmap(PunctualLightComponent* Light, float Radius);

    RenderFrameData m_FrameData;
    DebugRenderer m_DebugDraw;
    int m_FrameNumber = 0;

    RenderFrontendStat m_Stat;

    TVector<PrimitiveDef*> m_VisPrimitives;
    //TVector<PunctualLightComponent*> m_VisLights;
    //TVector<EnvironmentProbe*> m_VisEnvProbes;

    int m_VisPass = 0;

    // TODO: We can keep ready shadowCasters[] and boxes[]
    //TVector<Drawable*> m_ShadowCasters;
    TVector<BvAxisAlignedBoxSSE> m_ShadowBoxes;

    struct alignas(16) CullResult
    {
        int32_t Result[4];
    };
    TVector<CullResult> m_ShadowCasterCullResult;

    RenderFrontendDef m_RenderDef;

    TRef<RenderCore::ITexture> m_PhotometricProfiles;
    //TRef<EnvironmentMap> m_DummyEnvironmentMap;

    LightVoxelizer m_LightVoxelizer;

    FrameLoop* m_FrameLoop;

    ResourceManager* m_ResourceManager{};
};

HK_NAMESPACE_END
