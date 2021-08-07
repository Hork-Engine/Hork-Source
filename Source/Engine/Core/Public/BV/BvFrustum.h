/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Core/Public/Plane.h>
#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

#include <xmmintrin.h>
#include <emmintrin.h>

#define AN_FRUSTUM_USE_SSE

enum EFrustumPlane
{
    FPL_RIGHT,
    FPL_LEFT,
    FPL_TOP,
    FPL_BOTTOM,
    FPL_FAR,
    FPL_NEAR
};

class BvFrustum
{
    AN_FORBID_COPY( BvFrustum )

public:
    BvFrustum();
    ~BvFrustum();

    void FromMatrix( Float4x4 const & InMatrix, bool bReversedDepth = false );

    PlaneF const & operator[]( const int _Index ) const;

    bool IsPointVisible( Float3 const & _Point ) const;
    bool IsPointVisible_IgnoreZ( Float3 const & _Point ) const;

    bool IsSphereVisible( BvSphere const & _Sphere ) const;
    bool IsSphereVisible( Float3 const & _Point, const float _Radius ) const;
    bool IsSphereVisible_IgnoreZ( BvSphere const & _Sphere ) const;
    bool IsSphereVisible_IgnoreZ( Float3 const & _Point, const float _Radius ) const;

    bool IsBoxVisible( Float3 const & _Mins, Float3 const & _Maxs ) const;
    bool IsBoxVisible( Float4 const & _Mins, Float4 const & _Maxs ) const;
    bool IsBoxVisible( BvAxisAlignedBox const & b ) const;
    bool IsBoxVisible_IgnoreZ( Float3 const & _Mins, Float3 const & _Maxs ) const;
    bool IsBoxVisible_IgnoreZ( Float4 const & _Mins, Float4 const & _Maxs ) const;
    bool IsBoxVisible_IgnoreZ( BvAxisAlignedBox const & b ) const;

    bool IsOrientedBoxVisible( BvOrientedBox const & b ) const;
    bool IsOrientedBoxVisible_IgnoreZ( BvOrientedBox const & b ) const;

    void CullSphere_Generic( BvSphereSSE const * _Bounds, const int _NumObjects, int * _Result ) const;
    void CullSphere_IgnoreZ_Generic( BvSphereSSE const * _Bounds, const int _NumObjects, int * _Result ) const;
    void CullBox_Generic( BvAxisAlignedBoxSSE const * _Bounds, const int _NumObjects, int * _Result ) const;
    void CullBox_IgnoreZ_Generic( BvAxisAlignedBoxSSE const * _Bounds, const int _NumObjects, int * _Result ) const;
    void CullSphere_SSE( BvSphereSSE const * _Bounds, int _NumObjects, int * _Result ) const;
    void CullSphere_IgnoreZ_SSE( BvSphereSSE const * _Bounds, const int _NumObjects, int * _Result ) const;
    void CullBox_SSE( BvAxisAlignedBoxSSE const * _Bounds, const int _NumObjects, int * _Result ) const;
    void CullBox_IgnoreZ_SSE( BvAxisAlignedBoxSSE const * _Bounds, const int _NumObjects, int * _Result ) const;

    // Top-right
    void CornerVector_TR( Float3 & _Vector ) const;

    // Top-left
    void CornerVector_TL( Float3 & _Vector ) const;

    // Bottom-right
    void CornerVector_BR( Float3 & _Vector ) const;

    // Bottom-left
    void CornerVector_BL( Float3 & _Vector ) const;

private:
#ifdef AN_FRUSTUM_USE_SSE
    struct sse_t
    {
        __m128 x[6];
        __m128 y[6];
        __m128 z[6];
        __m128 d[6];
    };
    mutable sse_t * PlanesSSE;
#endif

    PlaneF Planes[6];
};

AN_FORCEINLINE PlaneF const & BvFrustum::operator[]( const int _Index ) const
{
    AN_ASSERT_( (unsigned)_Index < 6, "BvFrustum[]" );
    return Planes[_Index];
}

AN_FORCEINLINE bool BvFrustum::IsPointVisible( Float3 const & _Point ) const
{
    bool inside = true;
    for ( PlaneF const * p = Planes; p < Planes + 6; p++ ) {
        inside &= ( Math::Dot( *p, _Point ) > 0.0f );
    }
    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsPointVisible_IgnoreZ( Float3 const & _Point ) const
{
    bool inside = true;
    for ( PlaneF const * p = Planes; p < Planes + 4; p++ ) {
        inside &= ( Math::Dot( *p, _Point ) > 0.0f );
    }
    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsSphereVisible( BvSphere const & _Sphere ) const
{
    return IsSphereVisible( _Sphere.Center, _Sphere.Radius );
}

AN_FORCEINLINE bool BvFrustum::IsSphereVisible( Float3 const & _Point, const float _Radius ) const
{
    bool inside = true;
    for ( PlaneF const * p = Planes; p < Planes + 6; p++ ) {
        inside &= ( Math::Dot( *p, _Point ) > -_Radius );
    }
    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsSphereVisible_IgnoreZ( BvSphere const & _Sphere ) const
{
    return IsSphereVisible_IgnoreZ( _Sphere.Center, _Sphere.Radius );
}

AN_FORCEINLINE bool BvFrustum::IsSphereVisible_IgnoreZ( Float3 const & _Point, const float _Radius ) const
{
    bool inside = true;
    for ( PlaneF const * p = Planes; p < Planes + 4; p++ ) {
        inside &= ( Math::Dot( *p, _Point ) > -_Radius );
    }
    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsBoxVisible( Float3 const & _Mins, Float3 const & _Maxs ) const
{
    bool inside = true;

    PlaneF const * p = Planes;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsBoxVisible( Float4 const & _Mins, Float4 const & _Maxs ) const
{
    bool inside = true;

    PlaneF const * p = Planes;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsBoxVisible( BvAxisAlignedBox const & b ) const
{
    return IsBoxVisible( b.Mins, b.Maxs );
}

AN_FORCEINLINE bool BvFrustum::IsBoxVisible_IgnoreZ( Float3 const & _Mins, Float3 const & _Maxs ) const
{
    bool inside = true;

    PlaneF const * p = Planes;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsBoxVisible_IgnoreZ( Float4 const & _Mins, Float4 const & _Maxs ) const
{
    bool inside = true;

    PlaneF const * p = Planes;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
}

AN_FORCEINLINE bool BvFrustum::IsBoxVisible_IgnoreZ( BvAxisAlignedBox const & b ) const
{
    return IsBoxVisible_IgnoreZ( b.Mins, b.Maxs );
}

AN_FORCEINLINE bool BvFrustum::IsOrientedBoxVisible( BvOrientedBox const & b ) const
{
    Float3 point;

    for ( PlaneF const * p = Planes; p < Planes + 6; p++ ) {
        const float x = Math::Dot( b.Orient[0], p->Normal ) >= 0.0f ? -b.HalfSize[0] : b.HalfSize[0];
        const float y = Math::Dot( b.Orient[1], p->Normal ) >= 0.0f ? -b.HalfSize[1] : b.HalfSize[1];
        const float z = Math::Dot( b.Orient[2], p->Normal ) >= 0.0f ? -b.HalfSize[2] : b.HalfSize[2];

        point = b.Center + ( x * b.Orient[0] + y * b.Orient[1] + z * b.Orient[2] );

        if ( Math::Dot( point, *p ) >= 0.0f ) {
            return false;
        }
    }

    return true;
}

AN_FORCEINLINE bool BvFrustum::IsOrientedBoxVisible_IgnoreZ( BvOrientedBox const & b ) const
{
    Float3 point;

    for ( PlaneF const * p = Planes; p < Planes + 4; p++ ) {
        const float x = Math::Dot( b.Orient[0], p->Normal ) >= 0.0f ? -b.HalfSize[0] : b.HalfSize[0];
        const float y = Math::Dot( b.Orient[1], p->Normal ) >= 0.0f ? -b.HalfSize[1] : b.HalfSize[1];
        const float z = Math::Dot( b.Orient[2], p->Normal ) >= 0.0f ? -b.HalfSize[2] : b.HalfSize[2];

        point = b.Center + ( x * b.Orient[0] + y * b.Orient[1] + z * b.Orient[2] );

        if ( Math::Dot( point, *p ) >= 0.0f ) {
            return false;
        }
    }

    return true;
}
