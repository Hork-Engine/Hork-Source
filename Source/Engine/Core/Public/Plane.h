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

#include "VectorMath.h"

/*

Plane equalation: Normal.X * X + Normal.Y * Y + Normal.Z * Z + D = 0

*/

template< typename T >
struct TPlane
{
    TVector3< T > Normal;
    T             D;

    /** Construct uninitialized */
    TPlane() = default;

    /** Construct from plane equalation: A * X + B * Y + C * Z + D = 0 */
    constexpr explicit TPlane( T const & _A, T const & _B, T const & _C, T const & _D ) :
        Normal( _A, _B, _C ), D( _D ) {}

    /** Construct from normal and distance to plane */
    constexpr explicit TPlane( TVector3< T > const & _Normal, T const & _Dist ) :
        Normal( _Normal ), D( -_Dist ) {}

    /** Construct from normal and point on plane */
    constexpr explicit TPlane( TVector3< T > const & _Normal, TVector3< T > const & _PointOnPlane ) :
        Normal( _Normal ), D( -Math::Dot( _PointOnPlane, _Normal ) ) {}

    /** Construct from three points on plane */
    explicit TPlane( TVector3< T > const & _P0, TVector3< T > const & _P1, TVector3< T > const & _P2 )
    {
        FromPoints( _P0, _P1, _P2 );
    }

    /** Construct from another plane */
    template< typename T2 >
    constexpr explicit TPlane( TPlane< T2 > const & _Plane ) :
        Normal( _Plane.Normal ), D( _Plane.D ) {}

    T * ToPtr()
    {
        return &Normal.X;
    }

    T const * ToPtr() const
    {
        return &Normal.X;
    }

    constexpr TPlane operator-() const
    {
        // construct from normal and distance. D = -distance
        return TPlane( -Normal, D );
    }

    constexpr bool operator==( TPlane const & _Other ) const { return Compare( _Other ); }
    constexpr bool operator!=( TPlane const & _Other ) const { return !Compare( _Other ); }

    constexpr bool Compare( TPlane const & _Other ) const
    {
        return ToVec4() == _Other.ToVec4();
    }

    constexpr bool CompareEps( const TPlane & _Other, T const & _NormalEpsilon, T const & _DistEpsilon ) const
    {
        return Bool4( Math::Dist( Normal.X, _Other.Normal.X ) < _NormalEpsilon,
                      Math::Dist( Normal.Y, _Other.Normal.Y ) < _NormalEpsilon,
                      Math::Dist( Normal.Z, _Other.Normal.Z ) < _NormalEpsilon,
                      Math::Dist( D, _Other.D ) < _DistEpsilon )
            .All();
    }

    void Clear()
    {
        Normal.X = Normal.Y = Normal.Z = D = T( 0 );
    }

    constexpr T Dist() const
    {
        return -D;
    }

    void SetDist( T const & _Dist )
    {
        D = -_Dist;
    }

    int AxialType() const
    {
        return Normal.NormalAxialType();
    }

    int PositiveAxialType() const
    {
        return Normal.NormalPositiveAxialType();
    }

    constexpr int SignBits() const
    {
        return Normal.SignBits();
    }

    void FromPoints( TVector3< T > const & _P0, TVector3< T > const & _P1, TVector3< T > const & _P2 )
    {
        Normal = Math::Cross( _P0 - _P1, _P2 - _P1 ).Normalized();
        D      = -Math::Dot( Normal, _P1 );
    }

    void FromPoints( TVector3< T > const _Points[3] )
    {
        FromPoints( _Points[0], _Points[1], _Points[2] );
    }

    T Dist( TVector3< T > const & _Point ) const
    {
        return Math::Dot( _Point, Normal ) + D;
    }

    void NormalizeSelf()
    {
        const T NormalLength = Normal.Length();
        if ( NormalLength != T( 0 ) ) {
            const T OneOverLength = T( 1 ) / NormalLength;
            Normal *= OneOverLength;
            D *= OneOverLength;
        }
    }

    TPlane Normalized() const
    {
        const T NormalLength = Normal.Length();
        if ( NormalLength != T( 0 ) ) {
            const T OneOverLength = T( 1 ) / NormalLength;
            return TPlane( Normal * OneOverLength, D * OneOverLength );
        }
        return *this;
    }

    TPlane Snap( T const & _NormalEpsilon, T const & _DistEpsilon ) const
    {
        TPlane  SnapPlane( Normal.SnapNormal( _NormalEpsilon ), D );
        const T SnapD = Math::Round( D );
        if ( Math::Abs( D - SnapD ) < _DistEpsilon ) {
            SnapPlane.D = SnapD;
        }
        return SnapPlane;
    }

    TVector4< T > &                 ToVec4() { return *reinterpret_cast< TVector4< T > * >( this ); }
    constexpr TVector4< T > const & ToVec4() const { return *reinterpret_cast< const TVector4< T > * >( this ); }

    // String conversions
    AString ToString( int _Precision = -1 ) const
    {
        return AString( "( " ) + Math::ToString( Normal.X, _Precision ) + " " + Math::ToString( Normal.Y, _Precision ) + " " + Math::ToString( Normal.Z, _Precision ) + " " + Math::ToString( D, _Precision ) + " )";
    }

    AString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const
    {
        return AString( "( " ) + Math::ToHexString( Normal.X, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Normal.Y, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( Normal.Z, _LeadingZeros, _Prefix ) + " " + Math::ToHexString( D, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    void Write( IBinaryStream & _Stream ) const
    {
        struct Writer
        {
            Writer( IBinaryStream & _Stream, const float & _Value )
            {
                _Stream.WriteFloat( _Value );
            }

            Writer( IBinaryStream & _Stream, const double & _Value )
            {
                _Stream.WriteDouble( _Value );
            }
        };
        Normal.Write( _Stream );
        Writer( _Stream, D );
    }

    void Read( IBinaryStream & _Stream )
    {
        struct Reader
        {
            Reader( IBinaryStream & _Stream, float & _Value )
            {
                _Value = _Stream.ReadFloat();
            }

            Reader( IBinaryStream & _Stream, double & _Value )
            {
                _Value = _Stream.ReadDouble();
            }
        };
        Normal.Read( _Stream );
        Reader( _Stream, D );
    }
};

using PlaneF = TPlane< float >;
using PlaneD = TPlane< double >;
