/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include <Hork/Core/BaseMath.h>
#include <Hork/Core/IO.h>

HK_NAMESPACE_BEGIN

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
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }
    bool const& operator[](int index) const
    {
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }

    constexpr bool operator==(Bool2 const& rhs) const { return X == rhs.X && Y == rhs.Y; }
    constexpr bool operator!=(Bool2 const& rhs) const { return X != rhs.X || Y != rhs.Y; }

    //constexpr bool Any() const { return ( X || Y ); }
    //constexpr bool All() const { return ( X && Y ); }

    constexpr bool Any() const { return (X | Y); }
    constexpr bool All() const { return (X & Y); }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteBool(X);
        stream.WriteBool(Y);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        X = stream.ReadBool();
        Y = stream.ReadBool();
    }

    // Static methods
    static constexpr int   sNumComponents() { return 2; }
    static constexpr Bool2 sMinValue() { return Bool2(std::numeric_limits<bool>::min()); }
    static constexpr Bool2 sMaxValue() { return Bool2(std::numeric_limits<bool>::max()); }
    static Bool2 const&    sZero()
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
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }
    bool const& operator[](int index) const
    {
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }

    constexpr bool operator==(Bool3 const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z; }
    constexpr bool operator!=(Bool3 const& rhs) const { return X != rhs.X || Y != rhs.Y || Z != rhs.Z; }

    //constexpr bool Any() const { return ( X || Y || Z ); }
    //constexpr bool All() const { return ( X && Y && Z ); }

    constexpr bool Any() const { return (X | Y | Z); }
    constexpr bool All() const { return (X & Y & Z); }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteBool(X);
        stream.WriteBool(Y);
        stream.WriteBool(Z);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        X = stream.ReadBool();
        Y = stream.ReadBool();
        Z = stream.ReadBool();
    }

    // Static methods
    static constexpr int sNumComponents() { return 3; }
    static Bool3 const&  sZero()
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
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }
    bool const& operator[](int index) const
    {
        HK_ASSERT_(index >= 0 && index < sNumComponents(), "Index out of range");
        return (&X)[index];
    }

    constexpr bool operator==(Bool4 const& rhs) const { return X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W; }
    constexpr bool operator!=(Bool4 const& rhs) const { return X != rhs.X || Y != rhs.Y || Z != rhs.Z || W != rhs.W; }

    //constexpr bool Any() const { return ( X || Y || Z || W ); }
    //constexpr bool All() const { return ( X && Y && Z && W ); }
    constexpr bool Any() const { return (X | Y | Z | W); }
    constexpr bool All() const { return (X & Y & Z & W); }

    // Byte serialization
    void Write(IBinaryStreamWriteInterface& stream) const
    {
        stream.WriteBool(X);
        stream.WriteBool(Y);
        stream.WriteBool(Z);
        stream.WriteBool(W);
    }

    void Read(IBinaryStreamReadInterface& stream)
    {
        X = stream.ReadBool();
        Y = stream.ReadBool();
        Z = stream.ReadBool();
        W = stream.ReadBool();
    }

    // Static methods
    static constexpr int sNumComponents() { return 4; }
    static Bool4 const&  sZero()
    {
        static constexpr Bool4 ZeroVec(false);
        return ZeroVec;
    }
};

HK_NAMESPACE_END

HK_FORMAT_DEF_(Hk::Bool2, "( {} {} )", v.X, v.Y);
HK_FORMAT_DEF_(Hk::Bool3, "( {} {} {} )", v.X, v.Y, v.Z);
HK_FORMAT_DEF_(Hk::Bool4, "( {} {} {} {} )", v.X, v.Y, v.Z, v.W);
