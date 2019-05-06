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
#include <Engine/Core/Public/ConvexHull.h>
#include <Engine/World/Public/DrawSurf.h>
#include <Engine/Runtime/Public/RenderBackend.h>

//class FWorld;
class FActor;
class FTexture;
class FLevel;
class FAreaPortal;

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

enum ESurfaceType {
    SURF_UNKNOWN,
    SURF_PLANAR,
    SURF_TRISOUP,
    //SURF_BEZIER_PATCH
};

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

class FLevelArea : public FBaseObject {
    AN_CLASS( FLevelArea, FBaseObject )

    friend class FLevel;

public:

    FAreaPortal const * GetPortals() const { return PortalList; }

    TPodArray< FDrawSurf * > const & GetSurfs() const { return Movables; }

protected:
    FLevelArea() {}
    ~FLevelArea() {}

private:
    // Area property
    Float3 Position;

    // Area property
    Float3 Extents;

    // Area property
    Float3 ReferencePoint;

    // Owner
    FLevel * ParentLevel;

    // Surfaces in area
    TPodArray< FDrawSurf * > Movables;
    //TPodArray< FLightComponent * > Lights;
    //TPodArray< FEnvCaptureComponent * > EnvCaptures;

    // Linked portals
    FAreaPortal * PortalList;

    // Area bounding box
    BvAxisAlignedBox Bounds;
};

class FLevelPortal : public FBaseObject {
    AN_CLASS( FLevelPortal, FBaseObject )

    friend class FLevel;

public:

    // Vis marker. Set by render frontend.
    mutable int VisMark;

protected:
    FLevelPortal() {}

    ~FLevelPortal() {
        FConvexHull::Destroy( Hull );
    }

private:
    // Owner
    FLevel * ParentLevel;

    // Linked areas
    TRefHolder< FLevelArea > Area1;
    TRefHolder< FLevelArea > Area2;

    // Portal to areas
    FAreaPortal * Portals[2];

    // Portal hull
    FConvexHull * Hull;

    // Portal plane
    PlaneF Plane;

    // Blocking bits for doors (TODO)
    //int BlockingBits;
};

class FAreaPortal {
public:
    FLevelArea * ToArea;
    FConvexHull * Hull;
    PlaneF Plane;
    FAreaPortal * Next; // Next portal inside area
    FLevelPortal * Owner;
};

/*

FLevel

Logical subpart of a world

*/
class ANGIE_API FLevel : public FBaseObject {
    AN_CLASS( FLevel, FBaseObject )

    friend class FWorld;
    friend class FRenderFrontend;
    friend class FDrawSurf;

public:
    bool IsPersistentLevel() const { return bIsPersistent; }

    FWorld * GetOwnerWorld() const { return OwnerWorld; }

    TPodArray< FActor * > const & GetActors() const { return Actors; }

    TPodArray< FLevelArea * > const & GetAreas() const { return Areas; }

    BvAxisAlignedBox const & GetLevelBounds() const { return LevelBounds; }

    int FindArea( Float3 const & _Position );

    // Destroy all actors in level
    void DestroyActors();

    void ClearLightmaps();

    void SetLightData( const byte * _Data, int _Size );
    /*const */byte * GetLightData() /*const */{ return LightData; }

    FLevelArea * CreateArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint );
    FLevelPortal * CreatePortal( Float3 const * _HullPoints, int _NumHullPoints, FLevelArea * _Area1, FLevelArea * _Area2 );
    void DestroyPortalTree();

    void BuildPortals();

    void DrawDebug( FDebugDraw * _DebugDraw );

    TPodArray< FTexture * > Lightmaps;

protected:
    FLevel();
    ~FLevel();

private:
    void OnAddLevelToWorld();
    void OnRemoveLevelFromWorld();

    void AddSurfaces();
    void RemoveSurfaces();

    void PurgePortals();

    void AddSurfaceAreas( FDrawSurf * _Surf );
    void AddSurfaceToArea( int _AreaNum, FDrawSurf * _Surf );

    void RemoveSurfaceAreas( FDrawSurf * _Surf );

    FWorld * OwnerWorld;
    int IndexInArrayOfLevels = -1;
    bool bIsPersistent;
    TPodArray< FActor * > Actors;
    TPodArray< FLevelArea * > Areas;
    TRefHolder< FLevelArea > OutdoorArea;
    TPodArray< FLevelPortal * > Portals;
    TPodArray< FAreaPortal > AreaPortals;
    byte * LightData;
    BvAxisAlignedBox LevelBounds;
};
