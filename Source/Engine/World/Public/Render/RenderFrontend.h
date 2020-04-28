/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <Runtime/Public/RenderCore.h>
#include <World/Public/Base/DebugRenderer.h>
#include <World/Public/Canvas.h>
#include <World/Public/Resource/IndexedMesh.h>
#include <World/Public/World.h>

class ABaseLightComponent;
class AMeshComponent;
class ASkinnedComponent;
class AProceduralMeshComponent;

struct SRenderFrontendStat {
    int PolyCount;
    int ShadowMapPolyCount;
    int FrontendTime;
};

class ARenderFrontend
{
    AN_SINGLETON( ARenderFrontend )

public:
    void Initialize();
    void Deinitialize();

    void Render( ACanvas * InCanvas );

    /** Get render frame data */
    SRenderFrame * GetFrameData() { return &FrameData; }

    SRenderFrontendStat const & GetStat() const { return Stat; }

private:
    void RenderCanvas( ACanvas * InCanvas );
    void RenderView( int _Index );

    void QueryVisiblePrimitives( ARenderWorld * InWorld );
    void AddRenderInstances( ARenderWorld * InWorld );
    void AddDrawable( ADrawable * InComponent );
    void AddStaticMesh( AMeshComponent * InComponent );
    void AddSkinnedMesh( ASkinnedComponent * InComponent );
    void AddProceduralMesh( AProceduralMeshComponent * InComponent );
    void AddDirectionalShadowmapInstances( ARenderWorld * InWorld );
    void AddDirectionalShadowmap_StaticMesh( AMeshComponent * InComponent );
    void AddDirectionalShadowmap_SkinnedMesh( ASkinnedComponent * InComponent );
    void AddDirectionalShadowmap_ProceduralMesh( AProceduralMeshComponent * InComponent );

    void AddSurfaces( SSurfaceDef * const * Surfaces, int SurfaceCount );
    void AddSurface( ALevel * Level, AMaterialInstance * MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex, int _RenderingOrder );

    void CreateDirectionalLightCascades( SRenderView * View );

    SRenderFrame   FrameData;
    ADebugRenderer DebugDraw;
    int FrameNumber = 0;

    SRenderFrontendStat Stat;

    SViewport const * Viewports[MAX_RENDER_VIEWS];
    int NumViewports = 0;
    int MaxViewportWidth = 0;
    int MaxViewportHeight = 0;

    TPodArray< SPrimitiveDef * > VisPrimitives;
    TPodArray< SSurfaceDef * > VisSurfaces;
    TPodArray< ABaseLightComponent * > Lights;
    int VisPass = 0;

    struct SSurfaceStream {
        size_t VertexAddr;
        size_t VertexLightAddr;
        size_t VertexUVAddr;
        size_t IndexAddr;
    };

    SSurfaceStream SurfaceStream;

    SRenderFrontendDef RenderDef;
};

extern ARenderFrontend & GRenderFrontend;
