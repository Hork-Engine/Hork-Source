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

#include "SceneComponent.h"
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

enum ESurfaceType {
    SURF_UNKNOWN,
    SURF_PLANAR,
    SURF_TRISOUP,
    //SURF_BEZIER_PATCH,
    SURF_WEAPON    // for tricks with depth buffer
};

struct FAreaLink {
    int AreaNum;
    int Index;
};

#define DEFAULT_RENDERING_LAYERS 1//(~uint32_t(0))

//class ANGIE_API FRenderableComponent : public FSceneComponent {
//public:

//protected:
//    // [called from Actor's InitializeComponents(), overridable]
//    void InitializeComponent() override;
//};

class ANGIE_API FDrawSurf : public FSceneComponent {
    AN_COMPONENT( FDrawSurf, FSceneComponent )

    friend class FSpatialTreeComponent;

public:
    void SetBounds( BvAxisAlignedBox const & _Bounds );
    BvAxisAlignedBox const & GetBounds() const;

    BvAxisAlignedBox const & GetWorldBounds() const;

    void SetRenderingLayers( uint32_t _RenderingLayers ) { RenderingLayers = _RenderingLayers; }
    uint32_t GetRenderingLayers() const { return RenderingLayers; }

    void SetSurfaceType( ESurfaceType _Type );
    ESurfaceType GetSurfaceType() const;

    // Only for planar surfaces
    void SetSurfacePlane( const PlaneF & _Plane );
    const PlaneF & GetSurfacePlane() const;

    int VisFrame[ 1/*AN_MAX_RENDERTARGET_ENCLOSURE*/ ]; // Помечается на этапе обнаружения видимости по дереву

protected:
    FDrawSurf();

    virtual void OnUpdateWorldBounds();

    void InitializeComponent() override;
    void OnTransformDirty() override;

    void MarkBoundsDirty();
    
    BvAxisAlignedBox        Bounds;
    mutable BvAxisAlignedBox WorldBounds;
    mutable bool            WorldBoundsDirty;
    uint32_t                RenderingLayers; // Bit mask
    ESurfaceType            SurfaceType;
    PlaneF                  SurfacePlane;
    TPodArray< FAreaLink >  InArea; // list of intersected areas
};
