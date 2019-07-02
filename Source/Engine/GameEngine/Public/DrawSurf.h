/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

#include "PhysicalBody.h"



/*

FDrawSurf

Base class for drawing surfaces

*/
class ANGIE_API FDrawSurf : public FPhysicalBody {
    AN_COMPONENT( FDrawSurf, FPhysicalBody )

//    friend class FLevel;
//    friend class FRenderFrontend;

public:
    enum { RENDERING_LAYERS_DEFAULT = 1 };

    // Set rendering layers to filter mesh during rendering
    void SetRenderingLayers( uint32_t _RenderingLayers ) { RenderingLayers = _RenderingLayers; }

    // Get rendering layers
    uint32_t GetRenderingLayers() const { return RenderingLayers; }

    // Helper. Return true if surface is skinned mesh
    bool IsSkinnedMesh() const { return bSkinnedMesh; }

protected:
    FDrawSurf();

    uint32_t                RenderingLayers; // Bit mask
    bool                    bSkinnedMesh;
};

/*
 Алгоритм:
 При обновлении world-space bounding box поверхность добавляется в список на обновления принадлежности к Area
 При удалении поверхности не забыть удалить ее из вышеуказанного списка.
 Перед VSD_PASS_PORTALS:
 for ( FDrawSurf * surf = World->MarkedForAreaUpdate ; surf = surf->NextMarkedForAreaUpdate ) {
    RemoveSurfacesFromAllAreas( surf );
    AddSurfaceToAreas( surf );
 }
 Очистить список (World->MarkedForAreaUpdate = World->MarkedForAreaUpdateTail = nullptr)
*/

//// Areas where located surface
//class FPortalAreaSurf {
//public:
//    FDrawSurf * Surf;
//    FPortalArea * Area;
//    FPortalArea * Next;
//    FPortalArea * Prev;
//};
//
//class FPortalArea {
//public:
//    // Surfaces inside area
//    TPodArray< FPortalAreaSurf > Surfs;
//};
