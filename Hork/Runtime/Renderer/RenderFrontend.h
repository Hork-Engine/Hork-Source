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

#include <Hork/RenderDefs/RenderDefs.h>
#include <Hork/Runtime/Canvas/Canvas.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Runtime/World/Modules/Render/TerrainMesh.h>
#include <Hork/Runtime/World/Modules/Render/RenderContext.h>
#include "LightVoxelizer.h"

HK_NAMESPACE_BEGIN

struct RenderFrontendStat
{
    int PolyCount;
    int ShadowMapPolyCount;
    int FrontendTime;
};

class RenderFrontend final : public Noncopyable
{
public:
    /// Add render view to render
    void                        AddRenderView(WorldRenderView* renderView);

    /// Build frame data
    void                        Render(class FrameLoop* frameLoop);

    /// Get render frame data
    RenderFrameData*            GetFrameData() { return &m_FrameData; }

    /// Get frame statistic
    RenderFrontendStat const&   GetStat() const { return m_Stat; }

private:
    void                        ClearRenderView(RenderViewData* view);
    void                        RenderView(WorldRenderView* worldRenderView, RenderViewData* view);
    void                        SortRenderInstances();
    void                        SortShadowInstances(LightShadowmap const* shadowMap);
    void                        QueryVisiblePrimitives(World* world);
    void                        QueryShadowCasters(World* world, Float4x4 const& lightViewProjection, Float3 const& lightPosition, Float3x3 const& lightBasis, Vector<PrimitiveDef*>& primitives);
    void                        AddShadowmapCascades(class DirectionalLightComponent const& light, Float3x3 const& rotationMat, StreamedMemoryGPU* streamedMemory, RenderViewData* view, size_t* viewProjStreamHandle, int* pFirstCascade, int* pNumCascades);
    void                        AddDirectionalLightShadows(LightShadowmap* shadowmap, DirectionalLightInstance const* lightDef);

    template <typename MeshComponentType>
    void                        AddMeshes();
    template <typename MeshComponentType, typename LightComponentType>
    void                        AddMeshesShadow(LightShadowmap* shadowMap, BvAxisAlignedBox const& lightBounds={});
    bool                        AddLightShadowmap(class PunctualLightComponent* light, float radius);

    Vector<Ref<WorldRenderView>> m_RenderViews;
    FrameLoop*                  m_FrameLoop;
    RenderFrameData             m_FrameData;
    RenderContext               m_Context;
    RenderFrontendStat          m_Stat;
    DebugRenderer               m_DebugDraw;
    int                         m_FrameNumber = 0;
    // World for current render view
    World*                      m_World;
    // Current render view data
    RenderViewData*             m_View;
    //Vector<PrimitiveDef*>       m_VisPrimitives;
    int                         m_VisPass = 0;
    //Vector<BvAxisAlignedBoxSSE> m_ShadowBoxes;
    //struct alignas(16) CullResult { int32_t Result[4]; };
    //Vector<CullResult>          m_ShadowCasterCullResult;
    LightVoxelizer              m_LightVoxelizer;
};

HK_NAMESPACE_END
