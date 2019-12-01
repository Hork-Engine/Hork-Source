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
#include "Components/Drawable.h"

#include <Core/Public/ConvexHull.h>
#include <Core/Public/BitMask.h>
#include <Runtime/Public/RenderCore.h>

class AActor;
class ALevel;
struct SAreaPortal;
class ATexture;
class AMeshComponent;

class ALevelArea : public ABaseObject {
    AN_CLASS( ALevelArea, ABaseObject )

    friend class ALevel;

public:

    SAreaPortal const * GetPortals() const { return PortalList; }

    TPodArray< ADrawable * > const & GetDrawables() const { return Drawables; }

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
    TPodArray< ADrawable * > Drawables;
    //TPodArray< FLightComponent * > Lights;
    //TPodArray< FEnvCaptureComponent * > EnvCaptures;

    // Linked portals
    SAreaPortal * PortalList;

    // Area bounding box
    BvAxisAlignedBox Bounds;

    // TODO: AAudioClip * Ambient[4];
    //       float AmbientVolume[4];
    //       int NumAmbients; // 0..4

    //TRef< ASpatialTree > Tree; TODO: octree?
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

/**

ALevel

Subpart of a world

*/
class ANGIE_API ALevel : public ABaseObject {
    AN_CLASS( ALevel, ABaseObject )

    friend class AWorld;

public:
    /** Level is persistent if created by world */
    bool IsPersistentLevel() const { return bIsPersistent; }

    /** Get level world */
    AWorld * GetOwnerWorld() const { return OwnerWorld; }

    /** Get actors in level */
    TPodArray< AActor * > const & GetActors() const { return Actors; }

    /** Get level areas */
    TPodArray< ALevelArea * > const & GetAreas() const { return Areas; }

    /** Get level outdoor area */
    ALevelArea * GetOutdoorArea() const { return OutdoorArea; }

    /** Get level indoor bounding box */
    BvAxisAlignedBox const & GetIndoorBounds() const { return IndoorBounds; }

    /** Find visibility area */
    int FindArea( Float3 const & _Position );

    /** Destroy all actors in level */
    void DestroyActors();

    /** Create vis area */
    ALevelArea * AddArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint );

    /** Create vis portal */
    ALevelPortal * AddPortal( Float3 const * _HullPoints, int _NumHullPoints, ALevelArea * _Area1, ALevelArea * _Area2 );

    /** Destroy all vis areas and portals */
    void DestroyPortalTree();

    /** Build level visibility */
    void BuildPortals();

    // TODO: future:
    //void BuildLight();
    //void BuildStaticBatching();

    // Static lightmaps (experemental)
    TPodArray< ATexture * > Lightmaps;    
    void ClearLightmaps();
    void SetLightData( const byte * _Data, int _Size );
    /*const */byte * GetLightData() /*const */{ return LightData; }

protected:

    ALevel();
    ~ALevel();

private:

    /** Level ticking. Called by owner world. */
    void Tick( float _TimeStep );

    /** Draw debug. Called by owner world. */
    void DrawDebug( ADebugRenderer * InRenderer );

    /** Callback on add level to world. Called by owner world. */
    void OnAddLevelToWorld();

    /** Callback on remove level to world. Called by owner world. */
    void OnRemoveLevelFromWorld();

private:
    // Allow mesh component to register self in level
    friend class AMeshComponent;
    void AddStaticMesh( AMeshComponent * InStaticMesh );
    void RemoveStaticMesh( AMeshComponent * InStaticMesh );

private:
    void AddDrawable( ADrawable * _Drawable );
    void RemoveDrawable( ADrawable * _Drawable );

    void PurgePortals();

    void AddDrawables();
    void RemoveDrawables();

    void AddDrawableToArea( int _AreaNum, ADrawable * _Drawable );

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
    AMeshComponent * StaticMeshList;
    AMeshComponent * StaticMeshListTail;
};
