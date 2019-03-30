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

#include "BaseObject.h"

#include <Engine/Core/Public/BV/Frustum.h>
#include <Engine/World/Public/RenderableComponent.h>
#include <Engine/Runtime/Public/RenderBackend.h>

//class FWorld;
class FActor;
class FTexture;

class FBinarySpacePlane : public PlaneF {
public:
    byte Type; // axial type
};

class FNodeBase {
public:
    int VisFrame;
    BvAxisAlignedBox Bounds;
    class FBinarySpaceNode * Parent;
};

class FBinarySpaceNode : public FNodeBase {
public:
    FBinarySpacePlane * Plane;
    int ChildrenIdx[2];
};

class FBinarySpaceLeaf : public FNodeBase {
public:
    int Cluster;
    byte const * Visdata;
    int FirstSurface;
    int NumSurfaces;
};

#define MAX_SURFACE_LIGHTMAPS 4

class FSurfaceDef {
public:
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

struct FBinarySpaceData {
    TPodArray< FBinarySpacePlane > Planes;
    TPodArray< FBinarySpaceNode >  Nodes;
    TPodArray< FBinarySpaceLeaf >  Leafs;
    byte *                         Visdata;
    bool                           bCompressedVisData;
    int                            NumVisClusters;
    TPodArray< FSurfaceDef >       Surfaces;
    TPodArray< int >               Marksurfaces;
    TPodArray< FMeshVertex >       Vertices;
    TPodArray< FMeshLightmapUV >   LightmapVerts;
    TPodArray< FMeshVertexLight >  VertexLight;
    TPodArray< unsigned int >      Indices;

    TPodArray< FSurfaceDef * >     VisSurfs;
    int                            NumVisSurfs;

    FBinarySpaceData();
    ~FBinarySpaceData();

    int FindLeaf( const Float3 & _Position );
    byte const * LeafPVS( FBinarySpaceLeaf const * _Leaf );

    int MarkLeafs( int _ViewLeaf );

    byte const * DecompressVisdata( byte const * _Data );

    void PerformVSD( Float3 const & _ViewOrigin, FFrustum const & _Frustum, bool _SortLightmapGroup );

private:
    // For node marking
    int VisFrameCount;
    int ViewLeafCluster;

    void Traverse_r( int _NodeIndex, int _CullBits );

    // for traversing
    int VisFrame;
    Float3 ViewOrigin;
    FFrustum const * Frustum;
};

/*

FLevel

Logical subpart of a world

*/
class ANGIE_API FLevel : public FBaseObject {
    AN_CLASS( FLevel, FBaseObject )

    friend class FWorld;

public:

    TPodArray< FActor * > const & GetActors() const { return Actors; }

    // Destroy all actors in level
    void DestroyActors();

    void ClearLightmaps();

    void SetLightData( const byte * _Data, int _Size );
    /*const */byte * GetLightData() /*const */{ return LightData; }

    TPodArray< FTexture * > Lightmaps;

protected:
    FLevel() {}
    ~FLevel();

private:

    TPodArray< FActor * > Actors;
    byte *                LightData;
    //FWorld * World;
};
