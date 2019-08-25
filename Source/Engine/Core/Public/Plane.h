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

#include "Float.h"

enum class EPlaneSide {
    Back = -1,
    Front = 1,
    On = 0,
    Cross = 2
};

class PlaneF;
class PlaneD;

// Plane equalation: Normal.X * X + Normal.Y * Y + Normal.Z * Z + D = 0
class PlaneF /*final*/ {
public:
    Float3  Normal;
    float   D;

    PlaneF() = default;
    PlaneF( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 );
    PlaneF( float const & _A, float const & _B, float const & _C, const float & _D );
    PlaneF( Float3 const & _Normal, float const & _Dist );
    PlaneF( Float3 const & _Normal, Float3 const & _PointOnPlane );
    explicit PlaneF( PlaneD const & _Plane );

    float * ToPtr() {
        return &Normal.X;
    }

    float const * ToPtr() const {
        return &Normal.X;
    }

    PlaneF operator-() const;
    PlaneF & operator=( const PlaneF & p );

    Bool operator==( PlaneF const & _Other ) const { return Compare( _Other ); }
    Bool operator!=( PlaneF const & _Other ) const { return !Compare( _Other ); }

    Bool Compare( PlaneF const & _Other ) const {
        return ToVec4() == _Other.ToVec4();
    }

    Bool CompareEps( const PlaneF & _Other, float const & _NormalEpsilon, float const & _DistEpsilon ) const {
        return Bool4( FMath::Dist( Normal.X, _Other.Normal.X ) < _NormalEpsilon,
                      FMath::Dist( Normal.Y, _Other.Normal.Y ) < _NormalEpsilon,
                      FMath::Dist( Normal.Z, _Other.Normal.Z ) < _NormalEpsilon,
                      FMath::Dist( D, _Other.D ) < _DistEpsilon ).All();
    }

    void Clear();

    float Dist() const;

    void SetDist( const float & _Dist );

    int AxialType() const;

    int PositiveAxialType() const;

    int SignBits() const;

    void FromPoints( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 );
    void FromPoints( Float3 const _Points[3] );

    float Dist( Float3 const & _Point ) const;

    EPlaneSide SideOffset( Float3 const & _Point, float const & _Epsilon ) const;

    void NormalizeSelf() {
        const float NormalLength = Normal.Length();
        if ( NormalLength != 0.0f ) {
            const float OneOverLength = 1.0f / NormalLength;
            Normal *= OneOverLength;
            D *= OneOverLength;
        }
    }

    PlaneF Normalized() const {
        const float NormalLength = Normal.Length();
        if ( NormalLength != 0.0f ) {
            const float OneOverLength = 1.0f / NormalLength;
            return PlaneF( Normal * OneOverLength, D * OneOverLength );
        }
        return *this;
    }

    PlaneF Snap( const float & _NormalEpsilon, const float & _DistEpsilon ) const {
        PlaneF SnapPlane( Normal.SnapNormal( _NormalEpsilon ), D );
        const float SnapD = FMath::Round( D );
        if ( FMath::Abs( D - SnapD ) < _DistEpsilon ) {
            SnapPlane.D = SnapD;
        }
        return SnapPlane;
    }

    //Float3 ProjectPoint( const Float3 & _Point ) const;

    Float4 & ToVec4() { return *reinterpret_cast< Float4 * >( this ); }
    Float4 const & ToVec4() const { return *reinterpret_cast< const Float4 * >( this ); }

    // String conversions
    FString ToString( int _Precision = - 1 ) const {
        return FString( "( " ) + FMath::ToString( Normal.X, _Precision ) + " " + FMath::ToString( Normal.Y, _Precision ) + " " + FMath::ToString( Normal.Z, _Precision ) + " " + FMath::ToString( D, _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + FMath::ToHexString( Normal.X, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Normal.Y, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( Normal.Z, _LeadingZeros, _Prefix ) + " " + FMath::ToHexString( D, _LeadingZeros, _Prefix ) + " )";
    }

    // Byte serialization
    template< typename T >
    void Write( FStreamBase< T > & _Stream ) const {
        _Stream << Normal << D;
    }

    template< typename T >
    void Read( FStreamBase< T > & _Stream ) {
        _Stream >> Normal;
        _Stream >> D;
    }
};

AN_FORCEINLINE PlaneF::PlaneF( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {
    FromPoints( _P0, _P1, _P2 );
}

AN_FORCEINLINE PlaneF::PlaneF( float const & _A, float const & _B, float const & _C, float const & _D )
    : Normal( _A, _B, _C ), D( _D )
{
}

AN_FORCEINLINE PlaneF::PlaneF( Float3 const & _Normal, float const & _Dist )
    : Normal( _Normal ), D( -_Dist )
{
}

AN_FORCEINLINE PlaneF::PlaneF( Float3 const & _Normal, Float3 const & _PointOnPlane )
    : Normal( _Normal ), D( -_PointOnPlane.Dot( _Normal ) )
{
}

AN_FORCEINLINE void PlaneF::Clear() {
    Normal.X = Normal.Y = Normal.Z = D = 0;
}

AN_FORCEINLINE float PlaneF::Dist() const {
    return -D;
}

AN_FORCEINLINE void PlaneF::SetDist( const float & _Dist ) {
    D = -_Dist;
}

AN_FORCEINLINE int PlaneF::AxialType() const {
    return Normal.NormalAxialType();
}

AN_FORCEINLINE int PlaneF::PositiveAxialType() const {
    return Normal.NormalPositiveAxialType();
}

AN_FORCEINLINE int PlaneF::SignBits() const {
    return Normal.SignBits();
}

AN_FORCEINLINE PlaneF & PlaneF::operator=( PlaneF const & _Other ) {
    Normal = _Other.Normal;
    D = _Other.D;
    return *this;
}

AN_FORCEINLINE PlaneF PlaneF::operator-() const {
    return PlaneF( -Normal, D );
}

AN_FORCEINLINE void PlaneF::FromPoints( Float3 const & _P0, Float3 const & _P1, Float3 const & _P2 ) {
    Normal = ( _P0 - _P1 ).Cross( _P2 - _P1 ).Normalized();
    D = -Normal.Dot( _P1 );
}

AN_FORCEINLINE void PlaneF::FromPoints( Float3 const _Points[3] ) {
    FromPoints( _Points[0], _Points[1], _Points[2] );
}

AN_FORCEINLINE float PlaneF::Dist( Float3 const & _Point ) const {
    return _Point.Dot( Normal ) + D;
}

AN_FORCEINLINE EPlaneSide PlaneF::SideOffset( Float3 const & _Point, float const & _Epsilon ) const {
    const float Distance = Dist( _Point );
    if ( Distance > _Epsilon ) {
        return EPlaneSide::Front;
    }
    if ( Distance < -_Epsilon ) {
        return EPlaneSide::Back;
    }
    return EPlaneSide::On;
}

//// Assumes Plane.D == 0
//AN_FORCEINLINE Float3 PlaneF::ProjectPoint( Float3 const & _Point ) const {
//    const float OneOverLengthSqr = 1.0f / Normal.LengthSqr();
//    const Float3 N = Normal * OneOverLengthSqr;
//    return _Point - ( Normal.Dot( _Point ) * OneOverLengthSqr ) * N;
//}

//namespace FMath {

//// Assumes Plane.D == 0
//AN_FORCEINLINE Float3 ProjectPointOnPlane( Float3 const & _Point, Float3 const & _Normal ) {
//    const float OneOverLengthSqr = 1.0f / _Normal.LengthSqr();
//    const Float3 N = _Normal * OneOverLengthSqr;
//    return _Point - ( _Normal.Dot( _Point ) * OneOverLengthSqr ) * N;
//}

//}


class PlaneD /*final*/ {
public:
    Double3 Normal;
    Double  D;

    PlaneD() = default;
    PlaneD( Double3 const & _P0, Double3 const & _P1, Double3 const & _P2 );
    PlaneD( Double const & _A, Double const & _B, Double const & _C, Double const & _D );
    PlaneD( Double3 const & _Normal, Double const  & _Dist );
    PlaneD( Double3 const & _Normal, Double3 const & _PointOnPlane );
    explicit PlaneD( PlaneF const & _Plane );

    Double * ToPtr() {
        return &Normal.X;
    }

    Double const * ToPtr() const {
        return &Normal.X;
    }

    PlaneD operator-() const;
    PlaneD & operator=( PlaneD const & p );

    Bool operator==( PlaneD const & _Other ) const { return Compare( _Other ); }
    Bool operator!=( PlaneD const & _Other ) const { return !Compare( _Other ); }

    Bool Compare( PlaneD const & _Other ) const {
        return ToVec4() == _Other.ToVec4();
    }

    Bool CompareEps( PlaneD const & _Other, Double const & _NormalEpsilon, Double const & _DistEpsilon ) const {
        return Bool4( Normal.X.Dist( _Other.Normal.X ) < _NormalEpsilon,
                      Normal.Y.Dist( _Other.Normal.Y ) < _NormalEpsilon,
                      Normal.Z.Dist( _Other.Normal.Z ) < _NormalEpsilon,
                      D.Dist( _Other.D ) < _DistEpsilon ).All();
    }

    void Clear();

    Double Dist() const;

    void SetDist( const double & _Dist );

    int AxialType() const;

    int PositiveAxialType() const;

    int SignBits() const;

    void FromPoints( Double3 const & _P1, Double3 const & _P2, Double3 const & _P3 );
    void FromPoints( Double3 const _Points[3] );

    Double Dist( Double3 const & _Point ) const;

    EPlaneSide SideOffset( Double3 const & _Point, Double const & _Epsilon ) const;

    void NormalizeSelf() {
        const double NormalLength = Normal.Length();
        if ( NormalLength != 0.0f ) {
            const double OneOverLength = 1.0f / NormalLength;
            Normal *= OneOverLength;
            D *= OneOverLength;
        }
    }

    PlaneD Normalized() const {
        const double NormalLength = Normal.Length();
        if ( NormalLength != 0.0f ) {
            const double OneOverLength = 1.0f / NormalLength;
            return PlaneD( Normal * OneOverLength, D * OneOverLength );
        }
        return *this;
    }

    PlaneD Snap( const double & _NormalEpsilon, const double & _DistEpsilon ) const {
        PlaneD SnapPlane( Normal.SnapNormal( _NormalEpsilon ), D );
        const double SnapD = D.Round();
        if ( ( D - SnapD ).Abs() < _DistEpsilon ) {
            SnapPlane.D = SnapD;
        }
        return SnapPlane;
    }

    //Double3 ProjectPoint( const Double3 & _Point ) const;

    Double4 & ToVec4() { return *reinterpret_cast< Double4 * >( this ); }
    Double4 const & ToVec4() const { return *reinterpret_cast< const Double4 * >( this ); }

    // String conversions
    FString ToString( int _Precision = - 1 ) const {
        return FString( "( " ) + Normal.X.ToString( _Precision ) + " " + Normal.Y.ToString( _Precision ) + " " + Normal.Z.ToString( _Precision ) + " " + D.ToString( _Precision ) + " )";
    }

    FString ToHexString( bool _LeadingZeros = false, bool _Prefix = false ) const {
        return FString( "( " ) + Normal.X.ToHexString( _LeadingZeros, _Prefix ) + " " + Normal.Y.ToHexString( _LeadingZeros, _Prefix ) + " " + Normal.Z.ToHexString( _LeadingZeros, _Prefix ) + " " + D.ToHexString( _LeadingZeros, _Prefix ) + " )";
    }
};

AN_FORCEINLINE PlaneD::PlaneD( Double3 const & _P0, Double3 const & _P1, Double3 const & _P2 ) {
    FromPoints( _P0, _P1, _P2 );
}

AN_FORCEINLINE PlaneD::PlaneD( Double const & _A, Double const & _B, Double const & _C, Double const & _D )
    : Normal( _A, _B, _C ), D( _D )
{
}

AN_FORCEINLINE PlaneD::PlaneD( Double3 const & _Normal, Double const & _Dist )
    : Normal( _Normal ), D( -_Dist )
{
}

AN_FORCEINLINE PlaneD::PlaneD( Double3 const & _Normal, Double3 const & _PointOnPlane )
    : Normal( _Normal ), D( -_PointOnPlane.Dot( _Normal ) )
{
}

AN_FORCEINLINE void PlaneD::Clear() {
    Normal.X = Normal.Y = Normal.Z = D = 0;
}

AN_FORCEINLINE Double PlaneD::Dist() const {
    return -D;
}

AN_FORCEINLINE void PlaneD::SetDist( const double & _Dist ) {
    D = -_Dist;
}

AN_FORCEINLINE int PlaneD::AxialType() const {
    return Normal.NormalAxialType();
}

AN_FORCEINLINE int PlaneD::PositiveAxialType() const {
    return Normal.NormalPositiveAxialType();
}

AN_FORCEINLINE int PlaneD::SignBits() const {
    return Normal.SignBits();
}

AN_FORCEINLINE PlaneD & PlaneD::operator=( PlaneD const & _Other ) {
    Normal = _Other.Normal;
    D = _Other.D;
    return *this;
}

AN_FORCEINLINE PlaneD PlaneD::operator-() const {
    return PlaneD( -Normal, D );
}

AN_FORCEINLINE void PlaneD::FromPoints( Double3 const & _P0, Double3 const & _P1, Double3 const & _P2 ) {
    Normal = ( _P0 - _P1 ).Cross( _P2 - _P1 ).Normalized();
    D = -Normal.Dot( _P1 );
}

AN_FORCEINLINE void PlaneD::FromPoints( Double3 const _Points[3] ) {
    FromPoints( _Points[0], _Points[1], _Points[2] );
}

AN_FORCEINLINE Double PlaneD::Dist( Double3 const & _Point ) const {
    return _Point.Dot( Normal ) + D;
}

AN_FORCEINLINE EPlaneSide PlaneD::SideOffset( Double3 const & _Point, Double const & _Epsilon ) const {
    const double Distance = Dist( _Point );
    if ( Distance > _Epsilon ) {
        return EPlaneSide::Front;
    }
    if ( Distance < -_Epsilon ) {
        return EPlaneSide::Back;
    }
    return EPlaneSide::On;
}

AN_FORCEINLINE PlaneF::PlaneF( PlaneD const & _Plane ) : Normal( _Plane.Normal ), D( _Plane.D ) {}
AN_FORCEINLINE PlaneD::PlaneD( PlaneF const & _Plane ) : Normal( _Plane.Normal ), D( _Plane.D ) {}
