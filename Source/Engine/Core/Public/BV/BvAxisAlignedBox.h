#pragma once

#include <Engine/Core/Public/Float.h>

class AN_ALIGN_SSE BvAxisAlignedBoxSSE {
public:
    Float4 Mins;
    Float4 Maxs;
};

class BvAxisAlignedBox {
public:
    Float3 Mins;
    Float3 Maxs;

    BvAxisAlignedBox() = default;
    BvAxisAlignedBox( const Float3 & _Mins, const Float3 & _Maxs );

    Float * ToPtr();
    const Float * ToPtr() const;

    void operator/=( const Float & _Scale );
    void operator*=( const Float & _Scale );

    void operator+=( const Float3 & _Vec );
    void operator-=( const Float3 & _Vec );

    BvAxisAlignedBox operator+( const Float3 & _Vec ) const;
    BvAxisAlignedBox operator-( const Float3 & _Vec ) const;

    Bool Compare( const BvAxisAlignedBox & _Other ) const;
    Bool CompareEps( const BvAxisAlignedBox & _Other, const Float & _Epsilon ) const;
    Bool operator==( const BvAxisAlignedBox & _Other ) const;
    Bool operator!=( const BvAxisAlignedBox & _Other ) const;

    void Clear();
    void AddPoint( const Float3 & _Point );
    void AddPoint( const Float & _X, const Float _Y, const Float & _Z );
    void AddAABB( const BvAxisAlignedBox & _Other );
    Float3 Center() const;
    float Radius() const;
    Float3 Size() const;
    Float3 HalfSize() const;

    BvAxisAlignedBox Transform( const Float3 & _Origin, const Float3x3 & _Orient ) const {
        const Float3 InCenter = Center();
        const Float3 InEdge = HalfSize();
        const Float3 OutCenter( _Orient[0][0] * InCenter[0] + _Orient[1][0] * InCenter[1] + _Orient[2][0] * InCenter[2] + _Origin.X,
                                _Orient[0][1] * InCenter[0] + _Orient[1][1] * InCenter[1] + _Orient[2][1] * InCenter[2] + _Origin.Y,
                                _Orient[0][2] * InCenter[0] + _Orient[1][2] * InCenter[1] + _Orient[2][2] * InCenter[2] + _Origin.Z );
        const Float3 OutEdge( _Orient[0][0].Abs() * InEdge.X + _Orient[1][0].Abs() * InEdge.Y + _Orient[2][0].Abs() * InEdge.Z,
                              _Orient[0][1].Abs() * InEdge.X + _Orient[1][1].Abs() * InEdge.Y + _Orient[2][1].Abs() * InEdge.Z,
                              _Orient[0][2].Abs() * InEdge.X + _Orient[1][2].Abs() * InEdge.Y + _Orient[2][2].Abs() * InEdge.Z );
        return BvAxisAlignedBox( OutCenter - OutEdge, OutCenter + OutEdge );
    }

    BvAxisAlignedBox Transform( const Float3x4 & _TransformMatrix ) const {
        const Float3 InCenter = Center();
        const Float3 InEdge = HalfSize();
        const Float3 OutCenter( _TransformMatrix[0][0] * InCenter[0] + _TransformMatrix[0][1] * InCenter[1] + _TransformMatrix[0][2] * InCenter[2] + _TransformMatrix[0][3],
                                _TransformMatrix[1][0] * InCenter[0] + _TransformMatrix[1][1] * InCenter[1] + _TransformMatrix[1][2] * InCenter[2] + _TransformMatrix[1][3],
                                _TransformMatrix[2][0] * InCenter[0] + _TransformMatrix[2][1] * InCenter[1] + _TransformMatrix[2][2] * InCenter[2] + _TransformMatrix[2][3] );
        const Float3 OutEdge( _TransformMatrix[0][0].Abs() * InEdge.X + _TransformMatrix[0][1].Abs() * InEdge.Y + _TransformMatrix[0][2].Abs() * InEdge.Z,
                              _TransformMatrix[1][0].Abs() * InEdge.X + _TransformMatrix[1][1].Abs() * InEdge.Y + _TransformMatrix[1][2].Abs() * InEdge.Z,
                              _TransformMatrix[2][0].Abs() * InEdge.X + _TransformMatrix[2][1].Abs() * InEdge.Y + _TransformMatrix[2][2].Abs() * InEdge.Z );
        return BvAxisAlignedBox( OutCenter - OutEdge, OutCenter + OutEdge );
    }
};

AN_FORCEINLINE BvAxisAlignedBox::BvAxisAlignedBox( const Float3 & _Mins, const Float3 & _Maxs )
: Mins(_Mins)
, Maxs(_Maxs)
{}

AN_FORCEINLINE void BvAxisAlignedBox::operator/=( const Float & _Scale ) {
    const float InvScale = 1.0f / _Scale;
    Mins *= InvScale;
    Maxs *= InvScale;
}

AN_FORCEINLINE void BvAxisAlignedBox::operator*=( const Float & _Scale ) {
    Mins *= _Scale;
    Maxs *= _Scale;
}

AN_FORCEINLINE void BvAxisAlignedBox::operator+=( const Float3 & _Vec ) {
    Mins += _Vec;
    Maxs += _Vec;
}

AN_FORCEINLINE void BvAxisAlignedBox::operator-=( const Float3 & _Vec ) {
    Mins -= _Vec;
    Maxs -= _Vec;
}

AN_FORCEINLINE BvAxisAlignedBox BvAxisAlignedBox::operator+( const Float3 & _Vec ) const {
    return BvAxisAlignedBox( Mins + _Vec, Maxs + _Vec );
}

AN_FORCEINLINE BvAxisAlignedBox BvAxisAlignedBox::operator-( const Float3& _Vec ) const {
    return BvAxisAlignedBox( Mins - _Vec, Maxs - _Vec );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::Compare( const BvAxisAlignedBox & _Other ) const {
    return Mins.Compare( _Other.Mins ) && Maxs.Compare( _Other.Maxs );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::CompareEps( const BvAxisAlignedBox & _Other, const Float & _Epsilon ) const {
    return Mins.CompareEps( _Other.Mins, _Epsilon ) && Maxs.CompareEps( _Other.Maxs, _Epsilon );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::operator==( const BvAxisAlignedBox & _Other ) const {
    return Compare( _Other );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::operator!=( const BvAxisAlignedBox & _Other ) const {
    return !Compare( _Other );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddPoint( const Float3 & _Point ) {
    Mins.X = FMath::Min( _Point.X, Mins.X );
    Maxs.X = FMath::Max( _Point.X, Maxs.X );
    Mins.Y = FMath::Min( _Point.Y, Mins.Y );
    Maxs.Y = FMath::Max( _Point.Y, Maxs.Y );
    Mins.Z = FMath::Min( _Point.Z, Mins.Z );
    Maxs.Z = FMath::Max( _Point.Z, Maxs.Z );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddPoint( const Float & _X, const Float _Y, const Float & _Z ) {
    Mins.X = FMath::Min( _X, Mins.X );
    Maxs.X = FMath::Max( _X, Maxs.X );
    Mins.Y = FMath::Min( _Y, Mins.Y );
    Maxs.Y = FMath::Max( _Y, Maxs.Y );
    Mins.Z = FMath::Min( _Z, Mins.Z );
    Maxs.Z = FMath::Max( _Z, Maxs.Z );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddAABB( const BvAxisAlignedBox & _Other ) {
    Mins.X = FMath::Min( _Other.Mins.X, Mins.X );
    Maxs.X = FMath::Max( _Other.Maxs.X, Maxs.X );
    Mins.Y = FMath::Min( _Other.Mins.Y, Mins.Y );
    Maxs.Y = FMath::Max( _Other.Maxs.Y, Maxs.Y );
    Mins.Z = FMath::Min( _Other.Mins.Z, Mins.Z );
    Maxs.Z = FMath::Max( _Other.Maxs.Z, Maxs.Z );
}

AN_FORCEINLINE void BvAxisAlignedBox::Clear() {
    Mins = Float3( 9999999999.0f );
    Maxs = Float3( -9999999999.0f );
}

AN_FORCEINLINE Float3 BvAxisAlignedBox::Center() const {
    return (Maxs + Mins) * 0.5f;
}

AN_FORCEINLINE float BvAxisAlignedBox::Radius() const {
    return HalfSize().Length();
}

AN_FORCEINLINE Float3 BvAxisAlignedBox::Size() const {
    return Maxs - Mins;
}

AN_FORCEINLINE Float3 BvAxisAlignedBox::HalfSize() const {
    return ( Maxs - Mins ) * 0.5f;
}

AN_FORCEINLINE Float * BvAxisAlignedBox::ToPtr() {
    return &Mins.X;
}

AN_FORCEINLINE const Float * BvAxisAlignedBox::ToPtr() const {
    return &Mins.X;
}
