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
    BvAxisAlignedBox( Float3 const & _Mins, Float3 const & _Maxs );

    float * ToPtr();
    const float * ToPtr() const;

    void operator/=( float const & _Scale );
    void operator*=( float const & _Scale );

    BvAxisAlignedBox operator/( float const & _Scale ) const {
        float invScale = 1.0f / _Scale;
        return BvAxisAlignedBox( Mins * invScale, Maxs * invScale );
    }

    BvAxisAlignedBox operator*( float const & _Scale ) const {
        return BvAxisAlignedBox( Mins * _Scale, Maxs * _Scale );
    }

    void operator+=( Float3 const & _Vec );
    void operator-=( Float3 const & _Vec );

    BvAxisAlignedBox operator+( Float3 const & _Vec ) const;
    BvAxisAlignedBox operator-( Float3 const & _Vec ) const;

    Bool Compare( BvAxisAlignedBox const & _Other ) const;
    Bool CompareEps( BvAxisAlignedBox const & _Other, float const & _Epsilon ) const;
    Bool operator==( BvAxisAlignedBox const & _Other ) const;
    Bool operator!=( BvAxisAlignedBox const & _Other ) const;

    void Clear();
    void AddPoint( Float3 const & _Point );
    void AddPoint( float const & _X, float const & _Y, float const & _Z );
    void AddAABB( BvAxisAlignedBox const & _Other );
    void AddAABB( Float3 const & _Mins, Float3 const & _Maxs );
    Float3 Center() const;
    float Radius() const;
    Float3 Size() const;
    Float3 HalfSize() const;

    bool IsEmpty() const {
        return Mins.X >= Maxs.X || Mins.Y >= Maxs.Y || Mins.Z >= Maxs.Z;
    }

    BvAxisAlignedBox Transform( Float3 const & _Origin, Float3x3 const & _Orient ) const {
        const Float3 InCenter = Center();
        const Float3 InEdge = HalfSize();
        const Float3 OutCenter( _Orient[0][0] * InCenter[0] + _Orient[1][0] * InCenter[1] + _Orient[2][0] * InCenter[2] + _Origin.X,
                                _Orient[0][1] * InCenter[0] + _Orient[1][1] * InCenter[1] + _Orient[2][1] * InCenter[2] + _Origin.Y,
                                _Orient[0][2] * InCenter[0] + _Orient[1][2] * InCenter[1] + _Orient[2][2] * InCenter[2] + _Origin.Z );
        const Float3 OutEdge( Math::Abs( _Orient[0][0] ) * InEdge.X + Math::Abs( _Orient[1][0] ) * InEdge.Y + Math::Abs( _Orient[2][0] ) * InEdge.Z,
                              Math::Abs( _Orient[0][1] ) * InEdge.X + Math::Abs( _Orient[1][1] ) * InEdge.Y + Math::Abs( _Orient[2][1] ) * InEdge.Z,
                              Math::Abs( _Orient[0][2] ) * InEdge.X + Math::Abs( _Orient[1][2] ) * InEdge.Y + Math::Abs( _Orient[2][2] ) * InEdge.Z );
        return BvAxisAlignedBox( OutCenter - OutEdge, OutCenter + OutEdge );
    }

    BvAxisAlignedBox Transform( Float3x4 const & _TransformMatrix ) const {
        const Float3 InCenter = Center();
        const Float3 InEdge = HalfSize();
        const Float3 OutCenter( _TransformMatrix[0][0] * InCenter[0] + _TransformMatrix[0][1] * InCenter[1] + _TransformMatrix[0][2] * InCenter[2] + _TransformMatrix[0][3],
                                _TransformMatrix[1][0] * InCenter[0] + _TransformMatrix[1][1] * InCenter[1] + _TransformMatrix[1][2] * InCenter[2] + _TransformMatrix[1][3],
                                _TransformMatrix[2][0] * InCenter[0] + _TransformMatrix[2][1] * InCenter[1] + _TransformMatrix[2][2] * InCenter[2] + _TransformMatrix[2][3] );
        const Float3 OutEdge( Math::Abs( _TransformMatrix[0][0] ) * InEdge.X + Math::Abs( _TransformMatrix[0][1] ) * InEdge.Y + Math::Abs( _TransformMatrix[0][2] ) * InEdge.Z,
                              Math::Abs( _TransformMatrix[1][0] ) * InEdge.X + Math::Abs( _TransformMatrix[1][1] ) * InEdge.Y + Math::Abs( _TransformMatrix[1][2] ) * InEdge.Z,
                              Math::Abs( _TransformMatrix[2][0] ) * InEdge.X + Math::Abs( _TransformMatrix[2][1] ) * InEdge.Y + Math::Abs( _TransformMatrix[2][2] ) * InEdge.Z );
        return BvAxisAlignedBox( OutCenter - OutEdge, OutCenter + OutEdge );
    }

    static BvAxisAlignedBox const & Empty() {
        static BvAxisAlignedBox EmptyBox( Float3(9999999999.0f), Float3(-9999999999.0f) );
        return EmptyBox;
    }

    // Byte serialization
    void Write( IStreamBase & _Stream ) const {
        Mins.Write( _Stream );
        Maxs.Write( _Stream );
    }

    void Read( IStreamBase & _Stream ) {
        Mins.Read( _Stream );
        Maxs.Read( _Stream );
    }
};

AN_FORCEINLINE BvAxisAlignedBox::BvAxisAlignedBox( Float3 const & _Mins, Float3 const & _Maxs )
: Mins(_Mins)
, Maxs(_Maxs)
{}

AN_FORCEINLINE void BvAxisAlignedBox::operator/=( float const & _Scale ) {
    const float InvScale = 1.0f / _Scale;
    Mins *= InvScale;
    Maxs *= InvScale;
}

AN_FORCEINLINE void BvAxisAlignedBox::operator*=( float const & _Scale ) {
    Mins *= _Scale;
    Maxs *= _Scale;
}

AN_FORCEINLINE void BvAxisAlignedBox::operator+=( Float3 const & _Vec ) {
    Mins += _Vec;
    Maxs += _Vec;
}

AN_FORCEINLINE void BvAxisAlignedBox::operator-=( Float3 const & _Vec ) {
    Mins -= _Vec;
    Maxs -= _Vec;
}

AN_FORCEINLINE BvAxisAlignedBox BvAxisAlignedBox::operator+( Float3 const & _Vec ) const {
    return BvAxisAlignedBox( Mins + _Vec, Maxs + _Vec );
}

AN_FORCEINLINE BvAxisAlignedBox BvAxisAlignedBox::operator-( Float3 const & _Vec ) const {
    return BvAxisAlignedBox( Mins - _Vec, Maxs - _Vec );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::Compare( BvAxisAlignedBox const & _Other ) const {
    return Mins.Compare( _Other.Mins ) && Maxs.Compare( _Other.Maxs );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::CompareEps( BvAxisAlignedBox const & _Other, float const & _Epsilon ) const {
    return Mins.CompareEps( _Other.Mins, _Epsilon ) && Maxs.CompareEps( _Other.Maxs, _Epsilon );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::operator==( BvAxisAlignedBox const & _Other ) const {
    return Compare( _Other );
}

AN_FORCEINLINE Bool BvAxisAlignedBox::operator!=( BvAxisAlignedBox const & _Other ) const {
    return !Compare( _Other );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddPoint( Float3 const & _Point ) {
    Mins.X = Math::Min( _Point.X, Mins.X );
    Maxs.X = Math::Max( _Point.X, Maxs.X );
    Mins.Y = Math::Min( _Point.Y, Mins.Y );
    Maxs.Y = Math::Max( _Point.Y, Maxs.Y );
    Mins.Z = Math::Min( _Point.Z, Mins.Z );
    Maxs.Z = Math::Max( _Point.Z, Maxs.Z );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddPoint( float const & _X, float const & _Y, float const & _Z ) {
    Mins.X = Math::Min( _X, Mins.X );
    Maxs.X = Math::Max( _X, Maxs.X );
    Mins.Y = Math::Min( _Y, Mins.Y );
    Maxs.Y = Math::Max( _Y, Maxs.Y );
    Mins.Z = Math::Min( _Z, Mins.Z );
    Maxs.Z = Math::Max( _Z, Maxs.Z );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddAABB( BvAxisAlignedBox const & _Other ) {
    Mins.X = Math::Min( _Other.Mins.X, Mins.X );
    Maxs.X = Math::Max( _Other.Maxs.X, Maxs.X );
    Mins.Y = Math::Min( _Other.Mins.Y, Mins.Y );
    Maxs.Y = Math::Max( _Other.Maxs.Y, Maxs.Y );
    Mins.Z = Math::Min( _Other.Mins.Z, Mins.Z );
    Maxs.Z = Math::Max( _Other.Maxs.Z, Maxs.Z );
}

AN_FORCEINLINE void BvAxisAlignedBox::AddAABB( Float3 const & _Mins, Float3 const & _Maxs ) {
    Mins.X = Math::Min( _Mins.X, Mins.X );
    Maxs.X = Math::Max( _Maxs.X, Maxs.X );
    Mins.Y = Math::Min( _Mins.Y, Mins.Y );
    Maxs.Y = Math::Max( _Maxs.Y, Maxs.Y );
    Mins.Z = Math::Min( _Mins.Z, Mins.Z );
    Maxs.Z = Math::Max( _Maxs.Z, Maxs.Z );
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

AN_FORCEINLINE float * BvAxisAlignedBox::ToPtr() {
    return &Mins.X;
}

AN_FORCEINLINE const float * BvAxisAlignedBox::ToPtr() const {
    return &Mins.X;
}
