/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Core/BaseMath.h>

struct Bool2
{
    bool X{false};
    bool Y{false};

    Bool2() = default;
    explicit constexpr Bool2(bool v) :
        X(v), Y(v) {}
    constexpr Bool2(bool x, bool y) :
        X(x), Y(y) {}

    bool* ToPtr()
    {
        return &X;
    }

    const bool* ToPtr() const
    {
        return &X;
    }

    Bool2& operator=(Bool2 const& rhs)
    {
        X = rhs.X;
        Y = rhs.Y;
        return *this;
    }

    bool& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }
    bool const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    constexpr bool operator==(Bool2 const& rhs) const { return X == rhs.X && Y == rhs.Y; }
    constexpr bool operator!=(Bool2 const& rhs) const { return X != rhs.X || Y != rhs.Y; }

    //constexpr bool Any() const { return ( X || Y ); }
    //constexpr bool All() const { return ( X && Y ); }

    constexpr bool Any() const { return (X | Y); }
    constexpr bool All() const { return (X & Y); }

    // String conversions
    AString ToString() const
    {
        return AString("( ") + Math::ToString(X) + " " + Math::ToString(Y) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(X, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Y, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        stream.WriteBool(X);
        stream.WriteBool(Y);
    }

    void Read(IBinaryStream& stream)
    {
        X = stream.ReadBool();
        Y = stream.ReadBool();
    }

    // Static methods
    static constexpr int   NumComponents() { return 2; }
    static constexpr Bool2 MinValue() { return Bool2(std::numeric_limits<bool>::min()); }
    static constexpr Bool2 MaxValue() { return Bool2(std::numeric_limits<bool>::max()); }
    static Bool2 const&    Zero()
    {
        static constexpr Bool2 ZeroVec(false);
        return ZeroVec;
    }
};

struct Bool3
{
    bool X{false};
    bool Y{false};
    bool Z{false};

    Bool3() = default;
    explicit constexpr Bool3(bool v) :
        X(v), Y(v), Z(v) {}
    constexpr Bool3(bool x, bool y, bool z) :
        X(x), Y(y), Z(z) {}

    bool* ToPtr()
    {
        return &X;
    }

    const bool* ToPtr() const
    {
        return &X;
    }

    Bool3& operator=(Bool3 const& rhs)
    {
        X = rhs.X;
        Y = rhs.Y;
        Z = rhs.Z;
        return *this;
    }

    bool& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }
    bool const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    constexpr bool operator==(Bool3 const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z; }
    constexpr bool operator!=(Bool3 const& rhs) const { return X != rhs.X || Y != rhs.Y || Z != rhs.Z; }

    //constexpr bool Any() const { return ( X || Y || Z ); }
    //constexpr bool All() const { return ( X && Y && Z ); }

    constexpr bool Any() const { return (X | Y | Z); }
    constexpr bool All() const { return (X & Y & Z); }

    // String conversions
    AString ToString() const
    {
        return AString("( ") + Math::ToString(X) + " " + Math::ToString(Y) + " " + Math::ToString(Z) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(X, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Y, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Z, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        stream.WriteBool(X);
        stream.WriteBool(Y);
        stream.WriteBool(Z);
    }

    void Read(IBinaryStream& stream)
    {
        X = stream.ReadBool();
        Y = stream.ReadBool();
        Z = stream.ReadBool();
    }

    // Static methods
    static constexpr int NumComponents() { return 3; }
    static Bool3 const&  Zero()
    {
        static constexpr Bool3 ZeroVec(false);
        return ZeroVec;
    }
};

struct Bool4
{
    bool X{false};
    bool Y{false};
    bool Z{false};
    bool W{false};

    Bool4() = default;
    explicit constexpr Bool4(bool v) :
        X(v), Y(v), Z(v), W(v) {}
    constexpr Bool4(bool x, bool y, bool z, bool w) :
        X(x), Y(y), Z(z), W(w) {}

    bool* ToPtr()
    {
        return &X;
    }

    const bool* ToPtr() const
    {
        return &X;
    }

    Bool4& operator=(Bool4 const& rhs)
    {
        X = rhs.X;
        Y = rhs.Y;
        Z = rhs.Z;
        W = rhs.W;
        return *this;
    }

    bool& operator[](int index)
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }
    bool const& operator[](int index) const
    {
        AN_ASSERT_(index >= 0 && index < NumComponents(), "Index out of range");
        return (&X)[index];
    }

    constexpr bool operator==(Bool4 const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W; }
    constexpr bool operator!=(Bool4 const& rhs) const { return X != rhs.X || Y != rhs.Y || Z != rhs.Z || W != rhs.W; }

    //constexpr bool Any() const { return ( X || Y || Z || W ); }
    //constexpr bool All() const { return ( X && Y && Z && W ); }
    constexpr bool Any() const { return (X | Y | Z | W); }
    constexpr bool All() const { return (X & Y & Z & W); }

    // String conversions
    AString ToString() const
    {
        return AString("( ") + Math::ToString(X) + " " + Math::ToString(Y) + " " + Math::ToString(Z) + " " + Math::ToString(W) + " )";
    }

    AString ToHexString(bool bLeadingZeros = false, bool bPrefix = false) const
    {
        return AString("( ") + Math::ToHexString(X, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Y, bLeadingZeros, bPrefix) + " " + Math::ToHexString(Z, bLeadingZeros, bPrefix) + " " + Math::ToHexString(W, bLeadingZeros, bPrefix) + " )";
    }

    // Byte serialization
    void Write(IBinaryStream& stream) const
    {
        stream.WriteBool(X);
        stream.WriteBool(Y);
        stream.WriteBool(Z);
        stream.WriteBool(W);
    }

    void Read(IBinaryStream& stream)
    {
        X = stream.ReadBool();
        Y = stream.ReadBool();
        Z = stream.ReadBool();
        W = stream.ReadBool();
    }

    // Static methods
    static constexpr int NumComponents() { return 4; }
    static Bool4 const&  Zero()
    {
        static constexpr Bool4 ZeroVec(false);
        return ZeroVec;
    }
};
