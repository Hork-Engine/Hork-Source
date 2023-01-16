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

#include <Renderer/RenderDefs.h>
#include "DebugRenderer.h"
#include "Canvas/Canvas.h"
#include "IndexedMesh.h"
#include "Terrain.h"
#include "TerrainMesh.h"
#include "LightVoxelizer.h"
#include "EnvironmentMap.h"
#include "WorldRenderView.h"

HK_NAMESPACE_BEGIN

class World;
class PunctualLightComponent;
class EnvironmentProbe;
class Drawable;
class MeshComponent;
class SkinnedComponent;
class ProceduralMeshComponent;
class TerrainComponent;

struct RenderFrontendStat
{
    int PolyCount;
    int ShadowMapPolyCount;
    int FrontendTime;
};

struct RenderFrontendDef
{
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
    ~RenderFrontend();

    void Render(class FrameLoop* FrameLoop, Canvas* InCanvas);

    /** Get render frame data */
    RenderFrameData* GetFrameData() { return &m_FrameData; }

    RenderFrontendStat const& GetStat() const { return m_Stat; }

private:
    void RenderView(int _Index);

    void QueryVisiblePrimitives(World* InWorld);
    void QueryShadowCasters(World* InWorld, Float4x4 const& LightViewProjection, Float3 const& LightPosition, Float3x3 const& LightBasis, TPodVector<PrimitiveDef*>& Primitives, TPodVector<SurfaceDef*>& Surfaces);
    void AddRenderInstances(World* InWorld);
    void AddDrawable(Drawable* InComponent);
    void AddTerrain(TerrainComponent* InComponent);
    void AddStaticMesh(MeshComponent* InComponent);
    void AddSkinnedMesh(SkinnedComponent* InComponent);
    void AddProceduralMesh(ProceduralMeshComponent* InComponent);
    void AddDirectionalShadowmapInstances(World* InWorld);
    void AddShadowmap_StaticMesh(LightShadowmap* ShadowMap, MeshComponent* InComponent);
    void AddShadowmap_SkinnedMesh(LightShadowmap* ShadowMap, SkinnedComponent* InComponent);
    void AddShadowmap_ProceduralMesh(LightShadowmap* ShadowMap, ProceduralMeshComponent* InComponent);

    void AddSurfaces(SurfaceDef* const* Surfaces, int SurfaceCount);
    void AddSurface(Level* Level, MaterialInstance* MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex /*, int _RenderingOrder*/);

    void AddShadowmapSurfaces(LightShadowmap* ShadowMap, SurfaceDef* const* Surfaces, int SurfaceCount);
    void AddShadowmapSurface(LightShadowmap* ShadowMap, MaterialInstance* MaterialInstance, int _NumIndices, int _FirstIndex /*, int _RenderingOrder*/);

    bool AddLightShadowmap(PunctualLightComponent* Light, float Radius);

    RenderFrameData m_FrameData;
    DebugRenderer m_DebugDraw;
    int m_FrameNumber = 0;

    RenderFrontendStat m_Stat;

    TPodVector<PrimitiveDef*> m_VisPrimitives;
    TPodVector<SurfaceDef*> m_VisSurfaces;
    TPodVector<PunctualLightComponent*> m_VisLights;
    TPodVector<EnvironmentProbe*> m_VisEnvProbes;

    int m_VisPass = 0;

    // TODO: We can keep ready shadowCasters[] and boxes[]
    TPodVector<Drawable*> m_ShadowCasters;
    TPodVector<BvAxisAlignedBoxSSE> m_ShadowBoxes;

    struct alignas(16) CullResult
    {
        int32_t Result[4];
    };
    TPodVector<CullResult> m_ShadowCasterCullResult;

    struct SurfaceStream
    {
        size_t VertexAddr;
        size_t VertexLightAddr;
        size_t VertexUVAddr;
        size_t IndexAddr;
    };

    SurfaceStream SurfaceStream;

    RenderFrontendDef m_RenderDef;
    WorldRenderView* m_WorldRenderView;

    TRef<RenderCore::ITexture> m_PhotometricProfiles;
    TRef<EnvironmentMap> m_DummyEnvironmentMap;

    TRef<TerrainMesh> m_TerrainMesh;

    LightVoxelizer m_LightVoxelizer;

    FrameLoop* m_FrameLoop;
};

HK_NAMESPACE_END
