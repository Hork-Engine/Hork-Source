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

#include "MeshComponent.h"
#include "StaticMesh.h"

enum EVSDPass {
    VSD_PASS_IGNORE     = 0,
    VSD_PASS_ALL        = ~0,
    VSD_PASS_PORTALS    = 1,
    VSD_PASS_BOUNDS     = 2,
    VSD_PASS_CUSTOM_VISIBLE_STEP = 4,
    VSD_PASS_VIS_MARKER = 8,
    VSD_PASS_DEFAULT    = VSD_PASS_PORTALS | VSD_PASS_BOUNDS,
};

class FCameraComponent;

/*

FStaticMeshComponent

*/
class ANGIE_API FStaticMeshComponent : public FMeshComponent {
    AN_COMPONENT( FStaticMeshComponent, FMeshComponent )

    friend class FWorld;
public:
    int             VisMarker;
    int             VSDPasses = VSD_PASS_DEFAULT;
    int             LightmapBlock;
    Float4          LightmapOffset;
    TRefHolder< FLightmapUV >   LightmapUVChannel;
    TRefHolder< FVertexLight >  VertexLightChannel;
    bool            bUseDynamicRange;
    unsigned int    DynamicRangeIndexCount;
    unsigned int    DynamicRangeStartIndexLocation;
    int             DynamicRangeBaseVertexLocation;

    void SetMesh( FIndexedMesh * _Mesh );
    void SetMeshSubpart( FIndexedMeshSubpart * _Subpart );

    FIndexedMesh * GetMesh() const { return Mesh; }
    FIndexedMeshSubpart * GetMeshSubpart() const { return Subpart; }

    FStaticMeshComponent * NextWorldMesh() { return Next; }
    FStaticMeshComponent * PrevWorldMesh() { return Prev; }

    virtual void OnCustomVisibleStep( FCameraComponent * _Camera, bool & _OutVisibleFlag ) {}

protected:
    FStaticMeshComponent();

    // [called from Actor's InitializeComponents(), overridable]
    void InitializeComponent() override;

    // [Actor friend, overridable]
    void BeginPlay() override;

    // [Actor friend, overridable]
    void EndPlay() override;

    void OnUpdateWorldBounds() override;

private:
    FStaticMeshComponent * Next;
    FStaticMeshComponent * Prev;

    TRefHolder< FIndexedMesh > Mesh;
    TRefHolder< FIndexedMeshSubpart > Subpart;   
};
