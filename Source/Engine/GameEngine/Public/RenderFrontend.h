/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Core/Public/BaseTypes.h>
#include <Engine/Runtime/Public/RenderBackend.h>
#include <Engine/GameEngine/Public/DebugDraw.h>

class FRenderingSurface;
class FRenderingParameters;
class FMaterialInstance;
class FCameraComponent;
class FMeshComponent;
class FPlayerController;
class FHUD;
class FWorld;
class FLevel;
class FLevelArea;
class FFrustum;
class FCanvas;
class FConvexHull;

class FRenderFrontend {
    AN_SINGLETON( FRenderFrontend )

    friend class FRenderingSurface;
public:

    void Initialize();
    void Deinitialize();

    int GetVisMarker() const { return VisMarker; }

    void BuildFrameData();

    //int64_t RenderTimeDelta() const { return GRuntime.GetFrameData()->RenderTimeDelta; }

    int GetPolyCount() const { return PolyCount; }
    int GetFrontendTime() const { return FrontendTime; }

    void WriteDrawList( struct ImDrawList const * _DrawList );

private:
    void WriteDrawList( FCanvas * _Canvas );
    void RenderView( int _Index );
    void AddInstances();
    void CullLevelInstances( FLevel * _Level );
    void FlowThroughPortals_r( FLevelArea * _Area );
    void AddSurface( FMeshComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount );
    void UpdateMaterialInstanceFrameData( FMaterialInstance * _Instance );
    void UpdateSurfaceAreas();
    

    FRenderFrame * CurFrameData;
    FRenderView * RV;
    FRenderingParameters * RP;
    FCameraComponent * Camera;
    FFrustum const * Frustum;
    FWorld * World;
    FDebugDraw DebugDraw;
    int VisMarker = 0;
    int PolyCount = 0;
    int FrontendTime = 0;
    Float3 ViewOrigin;
    int ViewArea;
    FConvexHull * Polygon[ 2 ];
};

extern FRenderFrontend & GRenderFrontend;
