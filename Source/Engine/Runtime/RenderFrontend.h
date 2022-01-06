/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
#include "Canvas.h"
#include "IndexedMesh.h"
#include "World.h"
#include "Terrain.h"
#include "LightVoxelizer.h"

class AAnalyticLightComponent;
class AIBLComponent;
class AMeshComponent;
class ASkinnedComponent;
class AProceduralMeshComponent;

struct SRenderFrontendStat {
    int PolyCount;
    int ShadowMapPolyCount;
    int FrontendTime;
};

class ARenderFrontend : public ABaseObject
{
    AN_CLASS( ARenderFrontend, ABaseObject )

public:
    ARenderFrontend();
    ~ARenderFrontend();

    void Render(class AFrameLoop* FrameLoop, ACanvas* InCanvas);

    /** Get render frame data */
    SRenderFrame * GetFrameData() { return &FrameData; }

    SRenderFrontendStat const & GetStat() const { return Stat; }

private:
    void RenderCanvas( ACanvas * InCanvas );
    void RenderView( int _Index );

    void QueryVisiblePrimitives( AWorld * InWorld );
    void QueryShadowCasters( AWorld * InWorld, Float4x4 const & LightViewProjection, Float3 const & LightPosition, Float3x3 const & LightBasis,
                             TPodVector< SPrimitiveDef * > & Primitives, TPodVector< SSurfaceDef * > & Surfaces );
    void AddRenderInstances( AWorld * InWorld );
    void AddDrawable( ADrawable * InComponent );
    void AddTerrain( ATerrainComponent * InComponent );
    void AddStaticMesh( AMeshComponent * InComponent );
    void AddSkinnedMesh( ASkinnedComponent * InComponent );
    void AddProceduralMesh( AProceduralMeshComponent * InComponent );
    void AddDirectionalShadowmapInstances( AWorld * InWorld );
    void AddShadowmap_StaticMesh( SLightShadowmap * ShadowMap, AMeshComponent * InComponent );
    void AddShadowmap_SkinnedMesh( SLightShadowmap * ShadowMap, ASkinnedComponent * InComponent );
    void AddShadowmap_ProceduralMesh( SLightShadowmap * ShadowMap, AProceduralMeshComponent * InComponent );

    void AddSurfaces( SSurfaceDef * const * Surfaces, int SurfaceCount );
    void AddSurface( ALevel * Level, AMaterialInstance * MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex/*, int _RenderingOrder*/ );

    void AddShadowmapSurfaces( SLightShadowmap * ShadowMap, SSurfaceDef * const * Surfaces, int SurfaceCount );
    void AddShadowmapSurface( SLightShadowmap * ShadowMap, AMaterialInstance * MaterialInstance, int _NumIndices, int _FirstIndex/*, int _RenderingOrder*/ );

    void AddLightShadowmap( AAnalyticLightComponent * Light, float Radius, int * pShadowmapIndex );

    SRenderFrame   FrameData;
    ADebugRenderer DebugDraw;
    int FrameNumber = 0;

    SRenderFrontendStat Stat;

    TPodVector< SViewport const * > Viewports;
    int MaxViewportWidth = 0;
    int MaxViewportHeight = 0;

    TPodVector< SPrimitiveDef * > VisPrimitives;
    TPodVector< SSurfaceDef * > VisSurfaces;
    TPodVector< AAnalyticLightComponent * > VisLights;
    TPodVector< AIBLComponent * > VisIBLs;

    int VisPass = 0;

    // TODO: We can keep ready shadowCasters[] and boxes[]
    TPodVector< ADrawable * > ShadowCasters;
    TPodVector< BvAxisAlignedBoxSSE > ShadowBoxes;
    TPodVector< int > ShadowCasterCullResult;

    struct SSurfaceStream {
        size_t VertexAddr;
        size_t VertexLightAddr;
        size_t VertexUVAddr;
        size_t IndexAddr;
    };

    SSurfaceStream SurfaceStream;

    SRenderFrontendDef RenderDef;
    ARenderingParameters * ViewRP;

    TRef< ATexture > PhotometricProfiles;

    TRef< ATerrainMesh > TerrainMesh;

    ALightVoxelizer LightVoxelizer;

    AFrameLoop* FrameLoop;
};
