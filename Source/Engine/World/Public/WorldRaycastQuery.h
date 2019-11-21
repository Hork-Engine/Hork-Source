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

#include <World/Public/Resource/IndexedMesh.h>

class ARenderWorld;
class ADrawable;

/** Box hit result */
struct SBoxHitResult
{
    ADrawable * Object;
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

/** Raycast drawable */
struct SWorldRaycastDrawable
{
    ADrawable * Object;
    int FirstHit;
    int NumHits;
    int ClosestHit;
};

/** Raycast result */
struct SWorldRaycastResult
{
    TPodArray< STriangleHitResult > Hits;
    TPodArray< SWorldRaycastDrawable > Drawables;

    void Sort() {

        struct ASortDrawables {

            TPodArray< STriangleHitResult > const & Hits;

            ASortDrawables( TPodArray< STriangleHitResult > const & _Hits ) : Hits(_Hits) {}

            bool operator() ( SWorldRaycastDrawable const & _A, SWorldRaycastDrawable const & _B ) {
                const float hitDistanceA = Hits[_A.ClosestHit].Distance;
                const float hitDistanceB = Hits[_B.ClosestHit].Distance;

                return ( hitDistanceA < hitDistanceB );
            }
        } SortDrawables( Hits );

        // Sort by drawables distance
        StdSort( Drawables.ToPtr(), Drawables.ToPtr() + Drawables.Size(), SortDrawables );

        struct ASortHit {
            bool operator() ( STriangleHitResult const & _A, STriangleHitResult const & _B ) {
                return ( _A.Distance < _B.Distance );
            }
        } SortHit;

        // Sort by hit distance
        for ( SWorldRaycastDrawable & drawable : Drawables ) {
            StdSort( Hits.ToPtr() + drawable.FirstHit, Hits.ToPtr() + (drawable.FirstHit + drawable.NumHits), SortHit );
            drawable.ClosestHit = drawable.FirstHit;
        }
    }

    void Clear() {
        Hits.Clear();
        Drawables.Clear();
    }
};

/** Closest hit result */
struct SWorldRaycastClosestResult
{
    ADrawable * Object;
    STriangleHitResult TriangleHit;

    float Fraction;
    Float3 Vertices[3];
    Float2 Texcoord;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

/** World raycast filter */
struct SWorldRaycastFilter
{
    /** Filter objects by mask */
    int RenderingMask;
    /** Sort result by the distance */
    bool bSortByDistance;

    SWorldRaycastFilter() {
        RenderingMask = ~0;
        bSortByDistance = true;
    }
};

/** World raycasting */
struct AWorldRaycastQuery
{
    /** Per-triangle raycast */
    static bool Raycast( ARenderWorld const * _World, SWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr );

    /** Per-AABB raycast */
    static bool RaycastAABB( ARenderWorld const * _World, TPodArray< SBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr );

    /** Per-triangle raycast */
    static bool RaycastClosest( ARenderWorld const * _World, SWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr );

    /** Per-AABB raycast */
    static bool RaycastClosestAABB( ARenderWorld const * _World, SBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter = nullptr );

private:
    static SWorldRaycastFilter DefaultRaycastFilter;
};
