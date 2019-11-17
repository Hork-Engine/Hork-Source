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
#include <Engine/Runtime/Public/RenderCore.h>
#include "Components/DrawSurf.h"

class AActor;
class ALevel;
struct SAreaPortal;

class ALevelArea : public ABaseObject {
    AN_CLASS( ALevelArea, ABaseObject )

    friend class ALevel;

public:

    SAreaPortal const * GetPortals() const { return PortalList; }

    TPodArray< ASpatialObject * > const & GetSurfs() const { return Movables; }

    //TPodArray< FLightComponent * > const & GetLights() const { return Lights; }

    BvAxisAlignedBox const & GetBoundingBox() const { return Bounds; }

protected:
    ALevelArea() {}
    ~ALevelArea() {}

private:
    // Area property
    Float3 Position;

    // Area property
    Float3 Extents;

    // Area property
    Float3 ReferencePoint;

    // Owner
    ALevel * ParentLevel;

    // Objects in area
    TPodArray< ASpatialObject * > Movables;
    //TPodArray< FLightComponent * > Lights;
    //TPodArray< FEnvCaptureComponent * > EnvCaptures;

    // Linked portals
    SAreaPortal * PortalList;

    // Area bounding box
    BvAxisAlignedBox Bounds;

    // TODO: AAudioClip * Ambient[4];
    //       float AmbientVolume[4];
    //       int NumAmbients; // 0..4

    TRef< ASpatialTree > Tree;
};

class ALevelPortal : public ABaseObject {
    AN_CLASS( ALevelPortal, ABaseObject )

    friend class ALevel;

public:

    // Vis marker. Set by render frontend.
    mutable int VisMark;

protected:
    ALevelPortal() {}

    ~ALevelPortal() {
        AConvexHull::Destroy( Hull );
    }

private:
    // Owner
    ALevel * ParentLevel;

    // Linked areas
    TRef< ALevelArea > Area1;
    TRef< ALevelArea > Area2;

    // Portal to areas
    SAreaPortal * Portals[2];

    // Portal hull
    AConvexHull * Hull;

    // Portal plane
    PlaneF Plane;

    // Blocking bits for doors (TODO)
    //int BlockingBits;
};

struct SAreaPortal {
    ALevelArea * ToArea;
    AConvexHull * Hull;
    PlaneF Plane;
    SAreaPortal * Next; // Next portal inside area
    ALevelPortal * Owner;
};

/*

ALevel

Logical subpart of a world

*/
class ANGIE_API ALevel : public ABaseObject {
    AN_CLASS( ALevel, ABaseObject )

    friend class AWorld;
    friend class ASpatialObject;

public:

    // Navigation bounding box is used to cutoff level geometry outside of it. Use with bOverrideNavigationBoundingBox = true.
    BvAxisAlignedBox NavigationBoundingBox;

    // Use navigation bounding box to cutoff level geometry outside of it.
    //bool bOverrideNavigationBoundingBox;

    // Navigation bounding box padding is required to extend current level geometry bounding box.
    //float NavigationBoundingBoxPadding = 1.0f;

    // Navigation mesh.
    AAINavigationMesh NavMesh;

    // Navigation mesh connections. You must rebuild navigation mesh if you change connections.
    TPodArray< SAINavMeshConnection > NavMeshConnections;

    // Navigation areas. You must rebuild navigation mesh if you change areas.
    TPodArray< SAINavigationArea > NavigationAreas;

    // Level is persistent if created by world
    bool IsPersistentLevel() const { return bIsPersistent; }

    // Get level world
    AWorld * GetOwnerWorld() const { return OwnerWorld; }

    // Get actors in level
    TPodArray< AActor * > const & GetActors() const { return Actors; }

    // Get level areas
    TPodArray< ALevelArea * > const & GetAreas() const { return Areas; }

    // Get level outdoor area
    ALevelArea * GetOutdoorArea() const { return OutdoorArea; }

    // Get level indoor bounding box
    BvAxisAlignedBox const & GetIndoorBounds() const { return IndoorBounds; }

    // Find visibility area
    int FindArea( Float3 const & _Position );

    // Destroy all actors in level
    void DestroyActors();

    // Create vis area
    ALevelArea * AddArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint );

    // Create vis portal
    ALevelPortal * AddPortal( Float3 const * _HullPoints, int _NumHullPoints, ALevelArea * _Area1, ALevelArea * _Area2 );

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
    TPodArray< ATexture * > Lightmaps;    
    void ClearLightmaps();
    void SetLightData( const byte * _Data, int _Size );
    /*const */byte * GetLightData() /*const */{ return LightData; }

    void GenerateSourceNavMesh( TPodArray< Float3 > & allVertices,
                                TPodArray< unsigned int > & allIndices,
                                TBitMask<> & _WalkableTriangles,
                                BvAxisAlignedBox & _ResultBoundingBox,
                                BvAxisAlignedBox const * _ClipBoundingBox );

    void RenderFrontend_AddInstances( SRenderFrontendDef * _Def );

protected:

    ALevel();
    ~ALevel();

private:

    // Level ticking. Called by owner world.
    void Tick( float _TimeStep );

    // Draw debug. Called by owner world.
    void DrawDebug( ADebugDraw * _DebugDraw );

    // Callback on add level to world. Called by owner world.
    void OnAddLevelToWorld();

    // Callback on remove level to world. Called by owner world.
    void OnRemoveLevelFromWorld();

private:
    void PurgePortals();

    void AddSurfaces();

    void RemoveSurfaces();

    void AddSurfaceAreas( ASpatialObject * _Surf );

    void AddSurfaceToArea( int _AreaNum, ASpatialObject * _Surf );

    void RemoveSurfaceAreas( ASpatialObject * _Surf );

    void CullInstances( SRenderFrontendDef * _Def );
    void FlowThroughPortals_r( SRenderFrontendDef * _Def, ALevelArea * _Area );
    void AddRenderInstances( SRenderFrontendDef * _Def, class AMeshComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount );
    //void AddLightInstance( SRenderFrontendDef * _Def, FLightComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount );

    AWorld * OwnerWorld;
    int IndexInArrayOfLevels = -1;
    bool bIsPersistent;
    TPodArray< AActor * > Actors;
    TPodArray< ALevelArea * > Areas;
    TRef< ALevelArea > OutdoorArea;
    TPodArray< ALevelPortal * > Portals;
    TPodArray< SAreaPortal > AreaPortals;
    byte * LightData;
    BvAxisAlignedBox IndoorBounds;
    int LastVisitedArea;
};
