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

#include "Octree.h"
#include "AINavigationMesh.h"

#include <Engine/Core/Public/ConvexHull.h>
#include <Engine/Core/Public/BitMask.h>
#include <Engine/GameEngine/Public/DrawSurf.h>
#include <Engine/Runtime/Public/RenderBackend.h>

//class FWorld;
class FActor;
class FTexture;
class FLevel;
class FAreaPortal;

class FLevelArea : public FBaseObject {
    AN_CLASS( FLevelArea, FBaseObject )

    friend class FLevel;

public:

    FAreaPortal const * GetPortals() const { return PortalList; }

    TPodArray< FSpatialObject * > const & GetSurfs() const { return Movables; }

    BvAxisAlignedBox const & GetBoundingBox() const { return Bounds; }

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

    // Objects in area
    TPodArray< FSpatialObject * > Movables;
    //TPodArray< FLightComponent * > Lights;
    //TPodArray< FEnvCaptureComponent * > EnvCaptures;

    // Linked portals
    FAreaPortal * PortalList;

    // Area bounding box
    BvAxisAlignedBox Bounds;

    // TODO: FAudioClip * Ambient[4];
    //       float AmbientVolume[4];
    //       int NumAmbients; // 0..4

    TRef< FSpatialTree > Tree;
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
    TRef< FLevelArea > Area1;
    TRef< FLevelArea > Area2;

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
    friend class FSpatialObject;

public:

    // Navigation bounding box is used to cutoff level geometry outside of it. Use with bOverrideNavigationBoundingBox = true.
    BvAxisAlignedBox NavigationBoundingBox;

    // Use navigation bounding box to cutoff level geometry outside of it.
    //bool bOverrideNavigationBoundingBox;

    // Navigation bounding box padding is required to extend current level geometry bounding box.
    //float NavigationBoundingBoxPadding = 1.0f;

    // Navigation mesh.
    FAINavigationMesh NavMesh;

    // Navigation mesh connections. You must rebuild navigation mesh if you change connections.
    TPodArray< FAINavMeshConnection > NavMeshConnections;

    // Navigation areas. You must rebuild navigation mesh if you change areas.
    TPodArray< FAINavigationArea > NavigationAreas;

    // Level is persistent if created by world
    bool IsPersistentLevel() const { return bIsPersistent; }

    // Get level world
    FWorld * GetOwnerWorld() const { return OwnerWorld; }

    // Get actors in level
    TPodArray< FActor * > const & GetActors() const { return Actors; }

    // Get level areas. Level is actually visibility areas.
    TPodArray< FLevelArea * > const & GetAreas() const { return Areas; }

    // Get level indoor bounding box
    BvAxisAlignedBox const & GetIndoorBounds() const { return IndoorBounds; }

    // Find visibility area
    int FindArea( Float3 const & _Position );

    // Destroy all actors in level
    void DestroyActors();

    // Create vis area
    FLevelArea * CreateArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint );

    // Create vis portal
    FLevelPortal * CreatePortal( Float3 const * _HullPoints, int _NumHullPoints, FLevelArea * _Area1, FLevelArea * _Area2 );

    // Destroy all vis areas and portals
    void DestroyPortalTree();

    // Build level visibility
    void BuildPortals();

    // Build level navigation
    void BuildNavMesh();

    // TODO: future:
    //void BuildLight();
    //void BuildStaticBatching();

    // Static lightmaps (experemental)
    TPodArray< FTexture * > Lightmaps;    
    void ClearLightmaps();
    void SetLightData( const byte * _Data, int _Size );
    /*const */byte * GetLightData() /*const */{ return LightData; }

    void GenerateSourceNavMesh( TPodArray< Float3 > & allVertices,
                                TPodArray< unsigned int > & allIndices,
                                TBitMask<> & _WalkableTriangles,
                                BvAxisAlignedBox & _ResultBoundingBox,
                                BvAxisAlignedBox const * _ClipBoundingBox );

protected:

    FLevel();
    ~FLevel();

private:

    // Level ticking. Called by owner world.
    void Tick( float _TimeStep );

    // Draw debug. Called by owner world.
    void DrawDebug( FDebugDraw * _DebugDraw );

    // Callback on add level to world. Called by owner world.
    void OnAddLevelToWorld();

    // Callback on remove level to world. Called by owner world.
    void OnRemoveLevelFromWorld();

private:
    void PurgePortals();

    void AddSurfaces();

    void RemoveSurfaces();

    void AddSurfaceAreas( FSpatialObject * _Surf );

    void AddSurfaceToArea( int _AreaNum, FSpatialObject * _Surf );

    void RemoveSurfaceAreas( FSpatialObject * _Surf );

    FWorld * OwnerWorld;
    int IndexInArrayOfLevels = -1;
    bool bIsPersistent;
    TPodArray< FActor * > Actors;
    TPodArray< FLevelArea * > Areas;
    TRef< FLevelArea > OutdoorArea;
    TPodArray< FLevelPortal * > Portals;
    TPodArray< FAreaPortal > AreaPortals;
    byte * LightData;
    BvAxisAlignedBox IndoorBounds;
};
