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

    //int GetFrameNumber() const { return FrameNumber; }

    SRenderFrontendStat const & GetStat() const { return Stat; }

private:
    void RenderCanvas( ACanvas * InCanvas );
    //void RenderImgui();
    //void RenderImgui( struct ImDrawList const * _DrawList );
    void RenderView( int _Index );

    void AddLevelInstances( ARenderWorld * InWorld, SRenderFrontendDef * _Def );
    void AddDirectionalShadowmapInstances( ARenderWorld * InWorld, SRenderFrontendDef * _Def );

    void AddSurfaces( SRenderFrontendDef * RenderDef, SSurfaceDef * const * Surfaces, int SurfaceCount );
    void AddSurface( SRenderFrontendDef * RenderDef, ALevel * Level, AMaterialInstance * MaterialInstance, int _LightmapBlock, int _NumIndices, int _FirstIndex, int _RenderingOrder );
    void AddMesh( SRenderFrontendDef * RenderDef, AMeshComponent * Component );

    void CreateDirectionalLightCascades( SRenderFrame * Frame, SRenderView * View );

    SRenderFrame * FrameData;
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

    int numVerts;
    int numIndices;

    // TODO: SurfaceMesh,SurfaceLightmapUV,SurfaceVertexLight must live per view

    /** Mesh for surface batching */
    TRef< AIndexedMesh > SurfaceMesh;

    /** Lightmap UV channel */
    TRef< ALightmapUV > SurfaceLightmapUV;

    /** Vertex light channel */
    TRef< AVertexLight > SurfaceVertexLight;

};

extern ARenderFrontend & GRenderFrontend;
