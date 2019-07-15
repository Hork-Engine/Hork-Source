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

#include <Engine/Core/Public/Plane.h>
#include "BvAxisAlignedBox.h"
#include "BvOrientedBox.h"
#include "BvSphere.h"

#include <xmmintrin.h>
#include <emmintrin.h>

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4701)
#endif

#ifdef AN_COMPILER_MSVC
#define AN_FRUSTUM_USE_SSE
#endif

enum EFrustumPlane {
    FPL_RIGHT,
    FPL_LEFT,
    FPL_TOP,
    FPL_BOTTOM,
    FPL_FAR,
    FPL_NEAR
};

struct FFrustumPlane : public PlaneF {
    int CachedSignBits;
};

class ANGIE_API FFrustum {
public:
    FFrustum();

    void			FromMatrix( const Float4x4 & _Matrix );
    void            UpdateSignBits();

    const FFrustumPlane & operator[]( int _Index ) const;

    bool			CheckPoint( const Float3 & _Point ) const;
    bool			CheckPoint2( const Float3 & _Point ) const;
    float           CheckSphere( const BvSphere & _Sphere ) const;
    float			CheckSphere( const Float3 & _Point, float _Radius ) const;
    float           CheckSphere2( const BvSphere & _Sphere ) const;
    float           CheckSphere2( const Float3 & _Point, float _Radius ) const;
    bool			CheckAABB( const Float3 & _Mins, const Float3 & _Maxs ) const;
    bool			CheckAABB( const Float4 & _Mins, const Float4 & _Maxs ) const;
    bool			CheckAABB( const BvAxisAlignedBox & b ) const;
    bool			CheckAABB2( const Float3 & _Mins, const Float3 & _Maxs ) const;
    bool			CheckAABB2( const Float4 & _Mins, const Float4 & _Maxs ) const;
    bool			CheckAABB2( const BvAxisAlignedBox & b ) const;
    bool			CheckOBB( const BvOrientedBox & b ) const;

    byte            CheckAABBPosX( const BvAxisAlignedBox & b ) const;
    byte            CheckAABBNegX( const BvAxisAlignedBox & b ) const;
    byte            CheckAABBPosY( const BvAxisAlignedBox & b ) const;
    byte            CheckAABBNegY( const BvAxisAlignedBox & b ) const;
    byte            CheckAABBPosZ( const BvAxisAlignedBox & b ) const;
    byte            CheckAABBNegZ( const BvAxisAlignedBox & b ) const;
    byte            CheckAABBSides( const BvAxisAlignedBox & b ) const;

    void            CullSphere_Generic( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullSphere2_Generic( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullAABB_Generic( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullAABB2_Generic( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullSphere_SSE( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullSphere2_SSE( const BvSphereSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullAABB_SSE( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const;
    void            CullAABB2_SSE( const BvAxisAlignedBoxSSE * _Bounds, int _NumObjects, int * _Result ) const;

    // Top-right
    void            CornerVector_TR( Float3 & _Vector ) const;

    // Top-left
    void            CornerVector_TL( Float3 & _Vector ) const;

    // Bottom-right
    void            CornerVector_BR( Float3 & _Vector ) const;

    // Bottom-left
    void            CornerVector_BL( Float3 & _Vector ) const;

private:
#ifdef AN_FRUSTUM_USE_SSE
    mutable __m128 frustum_planes_x[6];
    mutable __m128 frustum_planes_y[6];
    mutable __m128 frustum_planes_z[6];
    mutable __m128 frustum_planes_d[6];
#endif

    FFrustumPlane m_Planes[6];
};

AN_FORCEINLINE FFrustum::FFrustum() {
    for ( int i = 0 ; i < 6 ; i++ ) {
        m_Planes[i].CachedSignBits = 0;
    }
}

AN_FORCEINLINE const FFrustumPlane & FFrustum::operator[]( int _Index ) const {
    AN_ASSERT( (unsigned)_Index < 6, "FFrustum[]" );
    return m_Planes[_Index];
}

AN_FORCEINLINE void FFrustum::UpdateSignBits() {
    for ( FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        p->CachedSignBits = p->SignBits();
    }
}

AN_FORCEINLINE bool FFrustum::CheckPoint( const Float3 & _Point ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ )
        if ( FMath::Dot( p->Normal, _Point ) + p->D <= 0.0f )
            return false;
    return true;
}

// far/near ignore
AN_FORCEINLINE bool FFrustum::CheckPoint2( const Float3 & _Point ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 4 ; p++ )
        if ( FMath::Dot( p->Normal, _Point ) + p->D <= 0.0f )
            return false;
    return true;
}

AN_FORCEINLINE float FFrustum::CheckSphere( const BvSphere & _Sphere ) const {
    return CheckSphere( _Sphere.Center, _Sphere.Radius );
}

AN_FORCEINLINE float FFrustum::CheckSphere( const Float3 & _Point, float _Radius ) const {
    float dist;
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        dist = FMath::Dot( p->Normal, _Point ) + p->D;
        if ( dist <= -_Radius )
            return 0;
    }
    return dist + _Radius;
}

// far/near ignore
AN_FORCEINLINE float FFrustum::CheckSphere2( const BvSphere & _Sphere ) const {
    return CheckSphere2( _Sphere.Center, _Sphere.Radius );
}

AN_FORCEINLINE float FFrustum::CheckSphere2( const Float3 & _Point, float _Radius ) const {
    float dist;
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 4 ; p++ ) {
        dist = FMath::Dot( p->Normal, _Point ) + p->D;
        if ( dist <= -_Radius )
            return 0;
    }
    return dist + _Radius;
}

AN_FORCEINLINE bool FFrustum::CheckAABB( const Float3 & _Mins, const Float3 & _Maxs ) const {
#if 0
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        return false;
    }
    return true;
#else
    bool inside = true;

    const FFrustumPlane * p = m_Planes;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
#endif
}

AN_FORCEINLINE bool FFrustum::CheckAABB( const Float4 & _Mins, const Float4 & _Maxs ) const {
#if 0
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        return false;
    }
    return true;
#else
    bool inside = true;

    const FFrustumPlane * p = m_Planes;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
#endif
}

AN_FORCEINLINE bool FFrustum::CheckAABB( const BvAxisAlignedBox & b ) const {
    return CheckAABB( b.Mins, b.Maxs );
}

AN_FORCEINLINE byte FFrustum::CheckAABBPosX( const BvAxisAlignedBox & b ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 2
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 4
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 6
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 8
        return 0;
    }
    return 1<<0;
}

AN_FORCEINLINE byte FFrustum::CheckAABBNegX( const BvAxisAlignedBox & b ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 1
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 3
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 5
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 7
        return 0;
    }
    return 1<<1;
}

AN_FORCEINLINE byte FFrustum::CheckAABBPosY( const BvAxisAlignedBox & b ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 3
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 4
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 7
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 8
        return 0;
    }
    return 1<<2;
}

AN_FORCEINLINE byte FFrustum::CheckAABBNegY( const BvAxisAlignedBox & b ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 1
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 2
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 5
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 6
        return 0;
    }
    return 1<<3;
}

AN_FORCEINLINE byte FFrustum::CheckAABBPosZ( const BvAxisAlignedBox & b ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 5
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 6
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 7
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Maxs[2] + p->D > 0.0f ) continue; // 8
        return 0;
    }
    return 1<<4;
}

AN_FORCEINLINE byte FFrustum::CheckAABBNegZ( const BvAxisAlignedBox & b ) const {
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 1
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Mins[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 2
        if ( p->Normal[0] * b.Mins[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 3
        if ( p->Normal[0] * b.Maxs[0] + p->Normal[1] * b.Maxs[1] + p->Normal[2] * b.Mins[2] + p->D > 0.0f ) continue; // 4
        return 0;
    }
    return 1<<5;
}

AN_FORCEINLINE byte FFrustum::CheckAABBSides( const BvAxisAlignedBox & b ) const {
    return CheckAABBPosX( b ) | CheckAABBNegX( b )
         | CheckAABBPosY( b ) | CheckAABBNegY( b )
         | CheckAABBPosZ( b ) | CheckAABBNegZ( b );
}

// far/near ignore
AN_FORCEINLINE bool FFrustum::CheckAABB2( const Float3 & _Mins, const Float3 & _Maxs ) const {
#if 0
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 4 ; p++ ) {
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        return false;
    }
    return true;
#else
    bool inside = true;

    const FFrustumPlane * p = m_Planes;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
#endif
}

// far/near ignore
AN_FORCEINLINE bool FFrustum::CheckAABB2( const Float4 & _Mins, const Float4 & _Maxs ) const {
#if 0
    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 4 ; p++ ) {
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Mins[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Mins[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Mins[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * _Maxs[0] + p->Normal[1] * _Maxs[1] + p->Normal[2] * _Maxs[2] + p->D > 0.0f ) continue;
        return false;
    }
    return true;
#else
    bool inside = true;

    const FFrustumPlane * p = m_Planes;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    ++p;
    inside &= ( FMath::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X ) + FMath::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y ) + FMath::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z ) + p->D ) > 0.0f;

    return inside;
#endif
}

// far/near ignore
AN_FORCEINLINE bool FFrustum::CheckAABB2( const BvAxisAlignedBox & b ) const {
    return CheckAABB2( b.Mins, b.Maxs );
}

AN_FORCEINLINE bool FFrustum::CheckOBB( const BvOrientedBox & b ) const {
    Float3 points[8];

    Float3  Mins = -b.HalfSize;
    Float3  Maxs = b.HalfSize;

    // TODO: optimize
    points[0] = b.Orient * Float3( Mins.X, Mins.Y, Maxs.Z ) + b.Center;	// 0
    points[1] = b.Orient * Float3( Maxs.X, Mins.Y, Maxs.Z ) + b.Center;	// 1
    points[2] = b.Orient * Float3( Maxs.X, Maxs.Y, Maxs.Z ) + b.Center;	// 2
    points[3] = b.Orient * Float3( Mins.X, Maxs.Y, Maxs.Z ) + b.Center;	// 3
    points[4] = b.Orient * Float3( Maxs.X, Mins.Y, Mins.Z ) + b.Center;	// 4
    points[5] = b.Orient * Float3( Mins.X, Mins.Y, Mins.Z ) + b.Center;	// 5
    points[6] = b.Orient * Float3( Mins.X, Maxs.Y, Mins.Z ) + b.Center;	// 6
    points[7] = b.Orient * Float3( Maxs.X, Maxs.Y, Mins.Z ) + b.Center;	// 7

    for ( const FFrustumPlane * p = m_Planes ; p < m_Planes + 6 ; p++ ) {
        if ( p->Normal[0] * points[0][0] + p->Normal[1] * points[0][1] + p->Normal[2] * points[0][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[1][0] + p->Normal[1] * points[1][1] + p->Normal[2] * points[1][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[2][0] + p->Normal[1] * points[2][1] + p->Normal[2] * points[2][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[3][0] + p->Normal[1] * points[3][1] + p->Normal[2] * points[3][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[4][0] + p->Normal[1] * points[4][1] + p->Normal[2] * points[4][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[5][0] + p->Normal[1] * points[5][1] + p->Normal[2] * points[5][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[6][0] + p->Normal[1] * points[6][1] + p->Normal[2] * points[6][2] + p->D > 0.0f ) continue;
        if ( p->Normal[0] * points[7][0] + p->Normal[1] * points[7][1] + p->Normal[2] * points[7][2] + p->D > 0.0f ) continue;
        return false;
    }
    return true;
}

#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif
