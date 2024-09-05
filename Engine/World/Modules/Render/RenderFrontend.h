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

#pragma once

#include <Engine/Renderer/RenderDefs.h>
#include <Engine/World/DebugRenderer.h>
#include <Engine/Canvas/Canvas.h>
#include <Engine/World/Modules/Render/TerrainMesh.h>
#include "LightVoxelizer.h"
#include "WorldRenderView.h"

HK_NAMESPACE_BEGIN

class DirectionalLightComponent;

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

class RenderFrontend final : public Noncopyable
{
public:
    RenderFrontend();

    void Render(class FrameLoop* FrameLoop, Canvas* InCanvas);

    /// Get render frame data
    RenderFrameData* GetFrameData() { return &m_FrameData; }

    RenderFrontendStat const& GetStat() const { return m_Stat; }

private:
    void ClearRenderView(RenderViewData* view);
    void RenderView(WorldRenderView* worldRenderView, RenderViewData* view);
    void SortRenderInstances();
    void SortShadowInstances(LightShadowmap const* shadowMap);

    void QueryVisiblePrimitives(World* world);
    void QueryShadowCasters(World* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, Vector<PrimitiveDef*>& Primitives);

    void AddShadowmapCascades(DirectionalLightComponent const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* StreamedMemory, RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades);
    void AddDirectionalLightShadows(LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef);

    template <typename MeshComponentType>
    void AddMeshes();

    template <typename MeshComponentType, typename LightComponentType>
    void AddMeshesShadow(LightShadowmap* shadowMap, BvAxisAlignedBox const& lightBounds={});

    bool AddLightShadowmap(class PunctualLightComponent* Light, float Radius);

    RenderFrameData m_FrameData;
    DebugRenderer m_DebugDraw;
    int m_FrameNumber = 0;
    World* m_World;
    RenderViewData* m_View;

    RenderFrontendStat m_Stat;

    Vector<PrimitiveDef*> m_VisPrimitives;
    //Vector<PunctualLightComponent*> m_VisLights;
    //Vector<EnvironmentProbe*> m_VisEnvProbes;

    int m_VisPass = 0;

    // TODO: We can keep ready shadowCasters[] and boxes[]
    //Vector<Drawable*> m_ShadowCasters;
    Vector<BvAxisAlignedBoxSSE> m_ShadowBoxes;

    struct alignas(16) CullResult
    {
        int32_t Result[4];
    };
    Vector<CullResult> m_ShadowCasterCullResult;

    RenderFrontendDef m_RenderDef;

    //Ref<EnvironmentMap> m_DummyEnvironmentMap;

    LightVoxelizer m_LightVoxelizer;

    FrameLoop* m_FrameLoop;

    ResourceManager* m_ResourceManager{};
};

HK_NAMESPACE_END
