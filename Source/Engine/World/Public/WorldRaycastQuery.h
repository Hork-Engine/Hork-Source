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

#include <Engine/Resource/Public/IndexedMesh.h>

class FWorld;
class FSpatialObject;

/** Box hit result */
struct FBoxHitResult
{
    FSpatialObject * Object;
    Float3 LocationMin;
    Float3 LocationMax;
    float DistanceMin;
    float DistanceMax;
    //float FractionMin;
    //float FractionMax;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

/** Raycast entity */
struct FWorldRaycastEntity
{
    FSpatialObject * Object;
    int FirstHit;
    int NumHits;
    int ClosestHit;
};

/** Raycast result */
struct FWorldRaycastResult
{
    TPodArray< FTriangleHitResult > Hits;
    TPodArray< FWorldRaycastEntity > Entities;

    void Sort() {

        struct FSortEntity {

            TPodArray< FTriangleHitResult > const & Hits;

            FSortEntity( TPodArray< FTriangleHitResult > const & _Hits ) : Hits(_Hits) {}

            bool operator() ( FWorldRaycastEntity const & _A, FWorldRaycastEntity const & _B ) {
                const float hitDistanceA = Hits[_A.ClosestHit].Distance;
                const float hitDistanceB = Hits[_B.ClosestHit].Distance;

                return ( hitDistanceA < hitDistanceB );
            }
        } SortEntity( Hits );

        // Sort by entity distance
        StdSort( Entities.ToPtr(), Entities.ToPtr() + Entities.Size(), SortEntity );

        struct FSortHit {
            bool operator() ( FTriangleHitResult const & _A, FTriangleHitResult const & _B ) {
                return ( _A.Distance < _B.Distance );
            }
        } SortHit;

        // Sort by hit distance
        for ( FWorldRaycastEntity & entity : Entities ) {
            StdSort( Hits.ToPtr() + entity.FirstHit, Hits.ToPtr() + (entity.FirstHit + entity.NumHits), SortHit );
            entity.ClosestHit = entity.FirstHit;
        }
    }

    void Clear() {
        Hits.Clear();
        Entities.Clear();
    }
};

/** Closest hit result */
struct FWorldRaycastClosestResult
{
    FSpatialObject * Object;
    FTriangleHitResult TriangleHit;

    float Fraction;
    Float3 Vertices[3];
    Float2 Texcoord;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

/** World raycast filter */
struct FWorldRaycastFilter
{
    /** Filter objects by mask */
    int RenderingMask;
    /** Sort result by the distance */
    bool bSortByDistance;

    FWorldRaycastFilter() {
        RenderingMask = ~0;
        bSortByDistance = true;
    }
};

/** World raycasting */
struct FWorldRaycastQuery
{
    /** Per-triangle raycast */
    static bool Raycast( FWorld const * _World, FWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter const * _Filter = nullptr );

    /** Per-AABB raycast */
    static bool RaycastAABB( FWorld const * _World, TPodArray< FBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter const * _Filter = nullptr );

    /** Per-triangle raycast */
    static bool RaycastClosest( FWorld const * _World, FWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter const * _Filter = nullptr );

    /** Per-AABB raycast */
    static bool RaycastClosestAABB( FWorld const * _World, FBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter const * _Filter = nullptr );

private:
    static FWorldRaycastFilter DefaultRaycastFilter;
};
