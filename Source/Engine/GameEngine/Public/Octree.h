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
#include "IndexedMesh.h"

#include <Engine/Core/Public/BV/Frustum.h>

class FLevelArea;
class FSpatialObject;
class FMaterialInstance;

struct FBoxHitResult {
    FSpatialObject * Object;
    Float3 HitLocationMin;
    Float3 HitLocationMax;
    float HitDistanceMin;
    float HitDistanceMax;
    //float HitFractionMin;
    //float HitFractionMax;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

struct FWorldRaycastEntity {
    FSpatialObject * Object;

//    Float3 GetWorldHitLocation() {
//        Object->
//    }

    int FirstHit;
    int LastHit;
    int ClosestHit;
};

struct FWorldRaycastResult {
    TPodArray< FTriangleHitResult > Hits;
    TPodArray< FWorldRaycastEntity > Entities;

    void Sort() {

        struct FSortEntity {

            TPodArray< FTriangleHitResult > const & Hits;

            FSortEntity( TPodArray< FTriangleHitResult > const & _Hits ) : Hits(_Hits) {}

            bool operator() ( FWorldRaycastEntity const & _A, FWorldRaycastEntity const & _B ) {
                const float hitDistanceA = Hits[_A.ClosestHit].HitDistance;
                const float hitDistanceB = Hits[_B.ClosestHit].HitDistance;

                return ( hitDistanceA < hitDistanceB );
            }
        } SortEntity( Hits );

        // Sort by entity distance
        StdSort( &Entities[0], &Entities[Entities.Length()], SortEntity );

        struct FSortHit {
            bool operator() ( FTriangleHitResult const & _A, FTriangleHitResult const & _B ) {
                return ( _A.HitDistance < _B.HitDistance );
            }
        } SortHit;

        // Sort by hit distance
        for ( FWorldRaycastEntity & entity : Entities ) {
            StdSort( &Hits[entity.FirstHit], &Hits[entity.LastHit], SortHit );
            entity.ClosestHit = entity.FirstHit;
        }
    }

    void Clear() {
        Hits.Clear();
        Entities.Clear();
    }
};

struct FWorldRaycastClosestResult {
    FSpatialObject * Object;
    Float3 Position;
    Float3 Normal;
    float Distance;
    float Fraction;
    Float3 Vertices[3];
    Float2 UV;
    Float2 Texcoord;
    unsigned int TriangleIndices[3];
    FMaterialInstance * Material;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

struct FWorldRaycastFilter {
    FWorldRaycastFilter() {
        RenderingMask = ~0;
        bSortByDistance = true;
    }

    int RenderingMask;
    bool bSortByDistance;
};

class FSpatialTree : public FBaseObject {
    AN_CLASS( FSpatialTree, FBaseObject )

public:

    void AddObject( FSpatialObject * _Object );
    void RemoveObject( FSpatialObject * _Object );
    void UpdateObject( FSpatialObject * _Object );

    virtual void Build() {

    }

    virtual bool Trace( FWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd ) {
        return false;
    }

    virtual void Update();

    FLevelArea * Owner;

protected:
    FSpatialTree() {}
    ~FSpatialTree();

    int FindPendingObject( FSpatialObject * _Object );

    void ClearPendingList();

    struct FPendingObjectInfo {
        enum { PENDING_ADD, PENDING_REMOVE, PENDING_UPDATE };

        FSpatialObject * Object;
        int PendingOp;
    };

    TPodArray< FPendingObjectInfo > PendingObjects;
};

class FOctreeNode {
public:
    BvAxisAlignedBox BoundingBox;

    FOctreeNode * Parent;
    FOctreeNode * Childs[8];
};

class FOctree : public FSpatialTree {
    AN_CLASS( FOctree, FSpatialTree )

public:

    void Build() override;

    void Purge() {

    }

    bool Trace( FWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd ) override {
        return false;
    }

    void Update() override;

protected:

    FOctree() {}

    ~FOctree() {
        Purge();
    }

private:
    void TreeAddObject( FSpatialObject * _Object );
    void TreeRemoveObject( int _Index );
    void TreeUpdateObject( int _Index );

//    FOctreeNode * Root;
//    FOctreeNode * Nodes;
    int NumLevels;

    TPodArray< FSpatialObject * > ObjectsInTree;
};
