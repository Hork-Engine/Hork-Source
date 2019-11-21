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

#include <Runtime/Public/RenderCore.h>
#include <Core/Public/BV/BvFrustum.h>

class ABinarySpacePlane : public PlaneF {
public:
    byte Type; // axial type
};

struct SNodeBase {
    int VisFrame;
    BvAxisAlignedBox Bounds;
    struct SBinarySpaceNode * Parent;
};

struct SBinarySpaceNode : SNodeBase {
    ABinarySpacePlane * Plane;
    int ChildrenIdx[2];
};

enum EBinarySpaceLeafContents {
    BSP_CONTENTS_NORMAL,
    BSP_CONTENTS_INVISIBLE
};

struct SBinarySpaceLeaf : SNodeBase {
    int Cluster;
    byte const * Visdata;
    int FirstSurface;
    int NumSurfaces;
    int Contents;       // EBinarySpaceLeafContents
    byte AmbientType[4];
    byte AmbientVolume[4];
};

#define MAX_SURFACE_LIGHTMAPS 4

enum ESurfaceType {
    SURF_UNKNOWN,
    SURF_PLANAR,
    SURF_TRISOUP,
    //SURF_BEZIER_PATCH
};

struct SSurfaceDef {
    BvAxisAlignedBox Bounds; // currently unused
    int FirstVertex;
    int NumVertices;
    int FirstIndex;
    int NumIndices;
    //int PatchWidth;
    //int PatchHeight;
    ESurfaceType Type;
    PlaneF Plane;
    int LightmapGroup; // union of texture and lightmap block
    int LightmapWidth;
    int LightmapHeight;
    int LightmapOffsetX;
    int LightmapOffsetY;
    byte LightStyles[MAX_SURFACE_LIGHTMAPS];
    int LightDataOffset;

    int Marker;
};

struct SBinarySpaceData {
    TPodArray< ABinarySpacePlane > Planes;
    TPodArray< SBinarySpaceNode >  Nodes;
    TPodArray< SBinarySpaceLeaf >  Leafs;
    byte *                         Visdata;
    bool                           bCompressedVisData;
    int                            NumVisClusters;
    TPodArray< SSurfaceDef >       Surfaces;
    TPodArray< int >               Marksurfaces;
    TPodArray< SMeshVertex >       Vertices;
    TPodArray< SMeshLightmapUV >   LightmapVerts;
    TPodArray< SMeshVertexLight >  VertexLight;
    TPodArray< unsigned int >      Indices;

    TPodArray< SSurfaceDef * >     VisSurfs;
    int                            NumVisSurfs;

    SBinarySpaceData();
    ~SBinarySpaceData();

    int FindLeaf( const Float3 & _Position );
    byte const * LeafPVS( SBinarySpaceLeaf const * _Leaf );

    int MarkLeafs( int _ViewLeaf );

    byte const * DecompressVisdata( byte const * _Data );

    void PerformVSD( Float3 const & _ViewOrigin, BvFrustum const & _Frustum, bool _SortLightmapGroup );

private:
    // For node marking
    int VisFrameCount;
    int ViewLeafCluster;

    void Traverse_r( int _NodeIndex, int _CullBits );

    // for traversing
    int VisFrame;
    Float3 ViewOrigin;
    BvFrustum const * Frustum;
};
