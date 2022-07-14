/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "ScriptEngine.h"
#include "World.h"
#include "Actor.h"

#include <Platform/Logger.h>
#include <Geometry/BV/BvIntersect.h>

#include <angelscript.h>
#include <angelscript_addon/scriptstdstring/scriptstdstring.h>
#include <angelscript_addon/scriptbuilder/scriptbuilder.h>
#include <angelscript_addon/weakref/weakref.h>
#include <angelscript_addon/scripthandle/scripthandle.h>

void PrintMessage(std::string const& message)
{
    LOG(message.c_str());
}

struct SScopedContext
{
    HK_FORBID_COPY(SScopedContext)

    asIScriptContext* Self;
    AScriptEngine*    pEngine;

    SScopedContext(AScriptEngine* pEngine, asIScriptObject* pObject, asIScriptFunction* pFunction) :
        pEngine(pEngine)
    {
        Self = pEngine->GetContextPool().PrepareContext(pObject, pFunction);
    }

    SScopedContext(AScriptEngine* pEngine, asIScriptFunction* pFunction) :
        pEngine(pEngine)
    {
        Self = pEngine->GetContextPool().PrepareContext(pFunction);
    }

    ~SScopedContext()
    {
        pEngine->GetContextPool().UnprepareContext(Self);
    }

    asIScriptContext* operator->()
    {
        return Self;
    }

    int ExecuteCall()
    {
        int r = Self->Execute();
        if (r != asEXECUTION_FINISHED)
        {
            if (r == asEXECUTION_EXCEPTION)
            {
                LOG("Exception: {}\n", Self->GetExceptionString());
                LOG("Function: {}\n", Self->GetExceptionFunction()->GetDeclaration());
                LOG("Line: {}\n", Self->GetExceptionLineNumber());

                // It is possible to print more information about the location of the
                // exception, for example the call stack, values of variables, etc if
                // that is of interest.
            }
        }
        return r;
    }
};


template <typename T>
static void Destruct(T* p)
{
    p->~T();
}

template <typename T>
static typename T::ElementType* IndexOperator(T& self, int i)
{
    if ((unsigned)i >= self.NumComponents())
    {
        asIScriptContext* ctx = asGetActiveContext();
        ctx->SetException("Out of range");
        return {};
    }

    return &self[i];
}

static void ConstructFloat2Default(Float2* p)
{
    new (p) Float2(0);
}

static void ConstructFloat2XY(Float2* p, float X, float Y)
{
    new (p) Float2(X, Y);
}

static void ConstructFloat2FromFloat2(Float2* p, Float2 const& other)
{
    new (p) Float2(other);
}

//static std::string Float2ToString(Float2 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float2ToHexString(Float2 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static Float3 Float2ToFloat3(Float2 const& self, float z)
{
    return Float3(self, z);
}

static Float4 Float2ToFloat4(Float2 const& self)
{
    return Float4(self);
}

static Float4 Float2ToFloat4ZW(Float2 const& self, float z, float w)
{
    return Float4(self, z, w);
}

static void RegisterFloat2(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float2", "float X", offsetof(Float2, X));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float2", "float Y", offsetof(Float2, Y));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat2Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    //r = pEngine->RegisterObjectBehaviour("Float2", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(ConstructFloat2Val), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(ConstructFloat2XY), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2", asBEHAVE_CONSTRUCT, "void f(const Float2 &in)", asFUNCTION(ConstructFloat2FromFloat2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float2>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 &opAssign(const Float2 &in)", asMETHODPR(Float2, operator=, (const Float2&), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float &opIndex(int)", asFUNCTION(IndexOperator<Float2>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "const float &opIndex(int) const", asFUNCTION(IndexOperator<Float2>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "bool opEquals(const Float2 &in) const", asMETHOD(Float2, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opNeg() const", asMETHODPR(Float2, operator-, () const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opAdd(const Float2 &in) const", asMETHODPR(Float2, operator+, (const Float2&) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opSub(const Float2 &in) const", asMETHODPR(Float2, operator-, (const Float2&) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opMul(const Float2 &in) const", asMETHODPR(Float2, operator*, (const Float2&) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opDiv(const Float2 &in) const", asMETHODPR(Float2, operator/, (const Float2&) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opAdd(float) const", asMETHODPR(Float2, operator+, (float) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opSub(float) const", asMETHODPR(Float2, operator-, (float) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opMul(float) const", asMETHODPR(Float2, operator*, (float) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 opDiv(float) const", asMETHODPR(Float2, operator/, (float) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opAddAssign(const Float2 &in)", asMETHODPR(Float2, operator+=, (const Float2&), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opSubAssign(const Float2 &in)", asMETHODPR(Float2, operator-=, (const Float2&), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opMulAssign(const Float2 &in)", asMETHODPR(Float2, operator*=, (const Float2&), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opDivAssign(const Float2 &in)", asMETHODPR(Float2, operator/=, (const Float2&), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opAddAssign(float)", asMETHODPR(Float2, operator+=, (float), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opSubAssign(float)", asMETHODPR(Float2, operator-=, (float), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opMulAssign(float)", asMETHODPR(Float2, operator*=, (float), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2& opDivAssign(float)", asMETHODPR(Float2, operator/=, (float), Float2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float Min() const", asMETHOD(Float2, Min), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float Max() const", asMETHOD(Float2, Max), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "int MinorAxis() const", asMETHOD(Float2, MinorAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "int MajorAxis() const", asMETHOD(Float2, MajorAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "void Clear()", asMETHOD(Float2, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Abs() const", asMETHOD(Float2, Abs), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "bool CompareEps(const Float2 &in, float) const", asMETHOD(Float2, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float LengthSqr() const", asMETHOD(Float2, LengthSqr), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float Length() const", asMETHOD(Float2, Length), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float DistSqr(const Float2 &in) const", asMETHOD(Float2, DistSqr), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float Dist(const Float2 &in) const", asMETHOD(Float2, Dist), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "float NormalizeSelf()", asMETHOD(Float2, NormalizeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Normalized() const", asMETHOD(Float2, Normalized), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Floor() const", asMETHOD(Float2, Floor), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Ceil() const", asMETHOD(Float2, Ceil), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Fract() const", asMETHOD(Float2, Fract), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Sign() const", asMETHOD(Float2, Sign), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "int SignBits() const", asMETHOD(Float2, SignBits), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float2 Snap(float) const", asMETHOD(Float2, Snap), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "int NormalAxialType() const", asMETHOD(Float2, NormalAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "int NormalPositiveAxialType() const", asMETHOD(Float2, NormalPositiveAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "int VectorAxialType() const", asMETHOD(Float2, VectorAxialType), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float2", "string ToString(int=6) const", asFUNCTIONPR(Float2ToString, (Float2 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float2", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float2ToHexString, (Float2 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float3 ToFloat3(float=0) const", asFUNCTIONPR(Float2ToFloat3, (Float2 const&, float), Float3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float4 ToFloat4() const", asFUNCTIONPR(Float2ToFloat4, (Float2 const&), Float4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2", "Float4 ToFloat4ZW(float, float) const", asFUNCTIONPR(Float2ToFloat4ZW, (Float2 const&, float, float), Float4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    HK_UNUSED(r);


    // TODO?
#if 0
        template <int Index>
        constexpr T const& Get() const;
        template <int _Shuffle>
        constexpr TVector2<T> Shuffle2() const;
        template <int _Shuffle>
        constexpr TVector3<T> Shuffle3() const;
        template <int _Shuffle>
        constexpr TVector4<T> Shuffle4() const;
        constexpr Bool2 IsInfinite() const;
        constexpr Bool2 IsNan() const;
        constexpr Bool2 IsNormal() const;
        constexpr Bool2 IsDenormal() const;
        void Write(IBinaryStreamWriteInterface& _Stream) const;
        void Read(IBinaryStreamReadInterface& _Stream);
        static constexpr int   NumComponents() { return 2; }
        static TVector2 const& Zero();
#endif
}

static void ConstructFloat3Default(Float3* p)
{
    new (p) Float3(0);
}

static void ConstructFloat3XYZ(Float3* p, float X, float Y, float Z)
{
    new (p) Float3(X, Y, Z);
}

static void ConstructFloat3FromFloat3(Float3* p, Float3 const& other)
{
    new (p) Float3(other);
}

//static std::string Float3ToString(Float3 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float3ToHexString(Float3 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static Float2 Float3ToFloat2(Float3 const& self)
{
    return Float2(self);
}

static Float4 Float3ToFloat4(Float3 const& self, float w)
{
    return Float4(self, w);
}

static void RegisterFloat3(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float3", "float X", offsetof(Float3, X));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float3", "float Y", offsetof(Float3, Y));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float3", "float Z", offsetof(Float3, Z));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat3Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    //r = pEngine->RegisterObjectBehaviour("Float3", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(ConstructFloat3Val), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(ConstructFloat3XYZ), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3", asBEHAVE_CONSTRUCT, "void f(const Float3 &in)", asFUNCTION(ConstructFloat3FromFloat3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float3>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 &opAssign(const Float3 &in)", asMETHODPR(Float3, operator=, (const Float3&), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float &opIndex(int)", asFUNCTION(IndexOperator<Float3>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "const float &opIndex(int) const", asFUNCTION(IndexOperator<Float3>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "bool opEquals(const Float3 &in) const", asMETHOD(Float3, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opNeg() const", asMETHODPR(Float3, operator-, () const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opAdd(const Float3 &in) const", asMETHODPR(Float3, operator+, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opSub(const Float3 &in) const", asMETHODPR(Float3, operator-, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opMul(const Float3 &in) const", asMETHODPR(Float3, operator*, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opDiv(const Float3 &in) const", asMETHODPR(Float3, operator/, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opAdd(float) const", asMETHODPR(Float3, operator+, (float) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opSub(float) const", asMETHODPR(Float3, operator-, (float) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opMul(float) const", asMETHODPR(Float3, operator*, (float) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 opDiv(float) const", asMETHODPR(Float3, operator/, (float) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opAddAssign(const Float3 &in)", asMETHODPR(Float3, operator+=, (const Float3&), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opSubAssign(const Float3 &in)", asMETHODPR(Float3, operator-=, (const Float3&), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opMulAssign(const Float3 &in)", asMETHODPR(Float3, operator*=, (const Float3&), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opDivAssign(const Float3 &in)", asMETHODPR(Float3, operator/=, (const Float3&), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opAddAssign(float)", asMETHODPR(Float3, operator+=, (float), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opSubAssign(float)", asMETHODPR(Float3, operator-=, (float), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opMulAssign(float)", asMETHODPR(Float3, operator*=, (float), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3& opDivAssign(float)", asMETHODPR(Float3, operator/=, (float), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float Min() const", asMETHOD(Float3, Min), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float Max() const", asMETHOD(Float3, Max), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "int MinorAxis() const", asMETHOD(Float3, MinorAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "int MajorAxis() const", asMETHOD(Float3, MajorAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "void Clear()", asMETHOD(Float3, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Abs() const", asMETHOD(Float3, Abs), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "bool CompareEps(const Float3 &in, float) const", asMETHOD(Float3, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float LengthSqr() const", asMETHOD(Float3, LengthSqr), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float Length() const", asMETHOD(Float3, Length), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float DistSqr(const Float3 &in) const", asMETHOD(Float3, DistSqr), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float Dist(const Float3 &in) const", asMETHOD(Float3, Dist), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "float NormalizeSelf()", asMETHOD(Float3, NormalizeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Normalized() const", asMETHOD(Float3, Normalized), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 NormalizeFix() const", asMETHOD(Float3, NormalizeFix), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "bool FixNormal()", asMETHOD(Float3, FixNormal), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Floor() const", asMETHOD(Float3, Floor), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Ceil() const", asMETHOD(Float3, Ceil), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Fract() const", asMETHOD(Float3, Fract), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Sign() const", asMETHOD(Float3, Sign), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "int SignBits() const", asMETHOD(Float3, SignBits), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Snap(float) const", asMETHOD(Float3, Snap), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 SnapNormal(float) const", asMETHOD(Float3, SnapNormal), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "int NormalAxialType() const", asMETHOD(Float3, NormalAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "int NormalPositiveAxialType() const", asMETHOD(Float3, NormalPositiveAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "int VectorAxialType() const", asMETHOD(Float3, VectorAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float3 Perpendicular() const", asMETHOD(Float3, Perpendicular), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "void ComputeBasis(Float3 &out, Float3 &out) const", asMETHOD(Float3, ComputeBasis), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float3", "string ToString(int=6) const", asFUNCTIONPR(Float3ToString, (Float3 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float3", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float3ToHexString, (Float3 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float2 ToFloat2() const", asFUNCTIONPR(Float3ToFloat2, (Float3 const&), Float2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3", "Float4 ToFloat4(float=0) const", asFUNCTIONPR(Float3ToFloat4, (Float3 const&, float), Float4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
        template <int _Shuffle>
        constexpr TVector2<T> Shuffle2() const;
        template <int _Shuffle>
        constexpr TVector3<T> Shuffle3() const;
        template <int _Shuffle>
        constexpr TVector4<T> Shuffle4() const;
        constexpr Bool3 IsInfinite() const;
        constexpr Bool3 IsNan() const;
        constexpr Bool3 IsNormal() const;
        constexpr Bool3 IsDenormal() const;
        void Write(IBinaryStreamWriteInterface& _Stream) const;
        void Read(IBinaryStreamReadInterface& _Stream);
        static constexpr int   NumComponents();
        static TVector3 const& Zero();
#endif
}

static void ConstructFloat4Default(Float4* p)
{
    new (p) Float4(0);
}

static void ConstructFloat4XYZW(Float4* p, float X, float Y, float Z, float W)
{
    new (p) Float4(X, Y, Z, W);
}

static void ConstructFloat4FromFloat4(Float4* p, Float4 const& other)
{
    new (p) Float4(other);
}

//static std::string Float4ToString(Float4 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float4ToHexString(Float4 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static Float2 Float4ToFloat2(Float4 const& self)
{
    return Float2(self);
}

static Float3 Float4ToFloat3(Float4 const& self)
{
    return Float3(self);
}

static void RegisterFloat4(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float4", "float X", offsetof(Float4, X));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float4", "float Y", offsetof(Float4, Y));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float4", "float Z", offsetof(Float4, Z));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float4", "float W", offsetof(Float4, W));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat4Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    //r = pEngine->RegisterObjectBehaviour("Float4", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(ConstructFloat4Val), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(ConstructFloat4XYZW), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4", asBEHAVE_CONSTRUCT, "void f(const Float4 &in)", asFUNCTION(ConstructFloat4FromFloat4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 &opAssign(const Float4 &in)", asMETHODPR(Float4, operator=, (const Float4&), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float &opIndex(int)", asFUNCTION(IndexOperator<Float4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "const float &opIndex(int) const", asFUNCTION(IndexOperator<Float4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "bool opEquals(const Float4 &in) const", asMETHOD(Float4, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opNeg() const", asMETHODPR(Float4, operator-, () const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opAdd(const Float4 &in) const", asMETHODPR(Float4, operator+, (const Float4&) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opSub(const Float4 &in) const", asMETHODPR(Float4, operator-, (const Float4&) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opMul(const Float4 &in) const", asMETHODPR(Float4, operator*, (const Float4&) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opDiv(const Float4 &in) const", asMETHODPR(Float4, operator/, (const Float4&) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opAdd(float) const", asMETHODPR(Float4, operator+, (float) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opSub(float) const", asMETHODPR(Float4, operator-, (float) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opMul(float) const", asMETHODPR(Float4, operator*, (float) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 opDiv(float) const", asMETHODPR(Float4, operator/, (float) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opAddAssign(const Float4 &in)", asMETHODPR(Float4, operator+=, (const Float4&), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opSubAssign(const Float4 &in)", asMETHODPR(Float4, operator-=, (const Float4&), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opMulAssign(const Float4 &in)", asMETHODPR(Float4, operator*=, (const Float4&), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opDivAssign(const Float4 &in)", asMETHODPR(Float4, operator/=, (const Float4&), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opAddAssign(float)", asMETHODPR(Float4, operator+=, (float), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opSubAssign(float)", asMETHODPR(Float4, operator-=, (float), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opMulAssign(float)", asMETHODPR(Float4, operator*=, (float), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4& opDivAssign(float)", asMETHODPR(Float4, operator/=, (float), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float Min() const", asMETHOD(Float4, Min), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float Max() const", asMETHOD(Float4, Max), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "int MinorAxis() const", asMETHOD(Float4, MinorAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "int MajorAxis() const", asMETHOD(Float4, MajorAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "void Clear()", asMETHOD(Float4, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Abs() const", asMETHOD(Float4, Abs), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "bool CompareEps(const Float4 &in, float) const", asMETHOD(Float4, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float LengthSqr() const", asMETHOD(Float4, LengthSqr), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float Length() const", asMETHOD(Float4, Length), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float DistSqr(const Float4 &in) const", asMETHOD(Float4, DistSqr), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float Dist(const Float4 &in) const", asMETHOD(Float4, Dist), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "float NormalizeSelf()", asMETHOD(Float4, NormalizeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Normalized() const", asMETHOD(Float4, Normalized), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Floor() const", asMETHOD(Float4, Floor), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Ceil() const", asMETHOD(Float4, Ceil), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Fract() const", asMETHOD(Float4, Fract), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Sign() const", asMETHOD(Float4, Sign), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "int SignBits() const", asMETHOD(Float4, SignBits), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float4 Snap(float) const", asMETHOD(Float4, Snap), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "int NormalAxialType() const", asMETHOD(Float4, NormalAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "int NormalPositiveAxialType() const", asMETHOD(Float4, NormalPositiveAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "int VectorAxialType() const", asMETHOD(Float4, VectorAxialType), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float4", "string ToString(int=6) const", asFUNCTIONPR(Float4ToString, (Float4 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float4", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float4ToHexString, (Float4 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float3 ToFloat2() const", asFUNCTIONPR(Float4ToFloat2, (Float4 const&), Float2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4", "Float3 ToFloat3() const", asFUNCTIONPR(Float4ToFloat3, (Float4 const&), Float3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    template <int Index>
    constexpr T const& Get() const;
    template <int _Shuffle>
    constexpr TVector2<T> Shuffle2() const;
    template <int _Shuffle>
    constexpr TVector3<T> Shuffle3() const;
    template <int _Shuffle>
    constexpr TVector4<T> Shuffle4() const;    
    constexpr Bool4 IsInfinite() const;
    constexpr Bool4 IsNan() const;
    constexpr Bool4 IsNormal() const;
    constexpr Bool4 IsDenormal() const;
    void Write(IBinaryStreamWriteInterface & _Stream) const;
    void Read(IBinaryStreamReadInterface & _Stream);
    static constexpr int   NumComponents();
    static TVector4 const& Zero();
#endif
}


static void ConstructPlaneDefault(PlaneF* p)
{
    new (p) PlaneF({0, 0, 0}, 0);
}

static void ConstructPlaneABCD(PlaneF* p, float A, float B, float C, float D)
{
    new (p) PlaneF(A, B, C, D);
}

static void ConstructPlaneNormalDist(PlaneF* p, Float3 const& Normal, float Dist)
{
    new (p) PlaneF(Normal, Dist);
}

static void ConstructPlaneNormalPoint(PlaneF* p, Float3 const& Normal, Float3 const& Point)
{
    new (p) PlaneF(Normal, Point);
}

static void ConstructPlaneFromPoints(PlaneF* p, Float3 const& P0, Float3 const& P1, Float3 const& P2)
{
    new (p) PlaneF(P0, P1, P2);
}

static void ConstructPlaneFromPlane(PlaneF* p, PlaneF const& Plane)
{
    new (p) PlaneF(Plane);
}

//static std::string PlaneToString(PlaneF const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string PlaneToHexString(PlaneF const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterPlane(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Plane", "Float3 Normal", offsetof(PlaneF, Normal));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Plane", "float D", offsetof(PlaneF, D));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructPlaneDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(ConstructPlaneABCD), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, float)", asFUNCTION(ConstructPlaneNormalDist), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, const Float3 &in)", asFUNCTION(ConstructPlaneNormalPoint), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, const Float3 &in, const Float3 &in)", asFUNCTION(ConstructPlaneFromPoints), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_CONSTRUCT, "void f(const Plane &in)", asFUNCTION(ConstructPlaneFromPlane), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Plane", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<PlaneF>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "Plane opNeg() const", asMETHODPR(PlaneF, operator-, () const, PlaneF), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "Plane &opAssign(const Plane &in)", asMETHODPR(PlaneF, operator=, (const PlaneF&), PlaneF&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "bool opEquals(const Plane &in) const", asMETHOD(PlaneF, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "bool CompareEps(const Plane &in, float, float) const", asMETHOD(PlaneF, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "void Clear()", asMETHOD(PlaneF, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "void SetDist(float)", asMETHOD(PlaneF, SetDist), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "float GetDist() const", asMETHOD(PlaneF, GetDist), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "int AxialType() const", asMETHOD(PlaneF, AxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "int PositiveAxialType() const", asMETHOD(PlaneF, PositiveAxialType), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "int SignBits() const", asMETHOD(PlaneF, SignBits), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "void FromPoints(const Float3 &in, const Float3 &in, const Float3 &in)", asMETHODPR(PlaneF, FromPoints, (Float3 const&, Float3 const&, Float3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "float DistanceToPoint(const Float3 &in) const", asMETHOD(PlaneF, DistanceToPoint), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "void NormalizeSelf()", asMETHOD(PlaneF, NormalizeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "Float3 Normalized() const", asMETHOD(PlaneF, Normalized), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "Plane Snap(float, float) const", asMETHOD(PlaneF, Snap), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "Float4& ToFloat4()", asMETHODPR(PlaneF, ToFloat4, (), Float4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Plane", "const Float4& ToFloat4() const", asMETHODPR(PlaneF, ToFloat4, () const, Float4 const&), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Plane", "string ToString(int=6) const", asFUNCTIONPR(PlaneToString, (PlaneF const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Plane", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(PlaneToHexString, (PlaneF const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);

    HK_UNUSED(r);
    // TODO?
#if 0
    void Write(IBinaryStreamWriteInterface& _Stream) const;
    void Read(IBinaryStreamReadInterface& _Stream);
#endif
}

static void ConstructFloat2x2Default(Float2x2* p)
{
    new (p) Float2x2(1);
}

static void ConstructFloat2x2Floats(Float2x2* p, float _00, float _01, float _10, float _11)
{
    new (p) Float2x2(_00, _01, _10, _11);
}

static void ConstructFloat2x2FromFloat2x2(Float2x2* p, const Float2x2& v)
{
    new (p) Float2x2(v);
}

static void ConstructFloat2x2Vecs(Float2x2* p, Float2 const& _Col0, Float2 const& _Col1)
{
    new (p) Float2x2(_Col0, _Col1);
}

static Float2 Float2x2GetRow(Float2x2& self, int i)
{
    if ((unsigned)i >= 2)
    {
        asIScriptContext* ctx = asGetActiveContext();
        ctx->SetException("Out of range");
        return {};
    }

    return self.GetRow(i);
}

static Float2x2 GetDiagonal2x2(float Diagonal)
{
    return Float2x2(Diagonal);
}

static Float2x2 GetDiagonal2x2(const Float2& Diagonal)
{
    return Float2x2(Diagonal);
}

static Float3x3 Float2x2ToFloat3x3(const Float2x2& self)
{
    return Float3x3(self);
}

static Float3x4 Float2x2ToFloat3x4(const Float2x2& self)
{
    return Float3x4(self);
}

static Float4x4 Float2x2ToFloat4x4(const Float2x2& self)
{
    return Float4x4(self);
}

//static std::string Float2x2ToString(Float2x2 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float2x2ToHexString(Float2x2 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterFloat2x2(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float2x2", "Float2 Col0", offsetof(Float2x2, Col0));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float2x2", "Float2 Col1", offsetof(Float2x2, Col1));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2x2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat2x2Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2x2", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(ConstructFloat2x2Floats), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2x2", asBEHAVE_CONSTRUCT, "void f(const Float2x2& in)", asFUNCTION(ConstructFloat2x2FromFloat2x2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2x2", asBEHAVE_CONSTRUCT, "void f(const Float2& in, const Float2& in)", asFUNCTION(ConstructFloat2x2Vecs), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float2x2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float2x2>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2 &opIndex(int)", asFUNCTION(IndexOperator<Float2x2>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "const Float2 &opIndex(int) const", asFUNCTION(IndexOperator<Float2x2>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 &opAssign(const Float2x2 &in)", asMETHODPR(Float2x2, operator=, (const Float2x2&), Float2x2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "bool opEquals(const Float2x2 &in) const", asMETHOD(Float2x2, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2 GetRow(int) const", asFUNCTION(Float2x2GetRow), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "bool CompareEps(const Float2x2 &in, float) const", asMETHOD(Float2x2, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "void TransposeSelf()", asMETHOD(Float2x2, TransposeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 Transposed() const", asMETHOD(Float2x2, Transposed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "void InverseSelf()", asMETHOD(Float2x2, InverseSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 Inversed() const", asMETHOD(Float2x2, Inversed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "float Determinant() const", asMETHOD(Float2x2, Determinant), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "void Clear()", asMETHOD(Float2x2, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "void SetIdentity()", asMETHOD(Float2x2, SetIdentity), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 Scaled(const Float2 &in) const", asMETHOD(Float2x2, Scaled), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 opMul(float) const", asMETHODPR(Float2x2, operator*, (float) const, Float2x2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 opDiv(float) const", asMETHODPR(Float2x2, operator/, (float) const, Float2x2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2 opMul(const Float2 &in) const", asMETHODPR(Float2x2, operator*, (const Float2&) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2 opMul(const Float2x2 &in) const", asMETHODPR(Float2x2, operator*, (const Float2x2&) const, Float2x2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2& opMulAssign(const Float2x2 &in)", asMETHODPR(Float2x2, operator*=, (const Float2x2&), Float2x2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2& opMulAssign(float)", asMETHODPR(Float2x2, operator*=, (float), Float2x2&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float2x2& opDivAssign(float)", asMETHODPR(Float2x2, operator/=, (float), Float2x2&), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float2x2", "string ToString(int=6) const", asFUNCTIONPR(Float2x2ToString, (Float2x2 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float2x2", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float2x2ToHexString, (Float2x2 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float3x3 ToFloat3x3() const", asFUNCTIONPR(Float2x2ToFloat3x3, (Float2x2 const&), Float3x3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float3x4 ToFloat3x4() const", asFUNCTIONPR(Float2x2ToFloat3x4, (Float2x2 const&), Float3x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float2x2", "Float4x4 ToFloat4x4() const", asFUNCTIONPR(Float2x2ToFloat4x4, (Float2x2 const&), Float4x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2x2 GetScale2x2(const Float2 &in)", asFUNCTION(Float2x2::Scale), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2x2 GetRotation2x2(float)", asFUNCTION(Float2x2::Rotation), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2x2 GetDiagonal2x2(float)", asFUNCTIONPR(GetDiagonal2x2, (float), Float2x2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2x2 GetDiagonal2x2(const Float2 &in)", asFUNCTIONPR(GetDiagonal2x2, (const Float2&), Float2x2), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    void                   Write(IBinaryStreamWriteInterface & _Stream) const;
    void                   Read(IBinaryStreamReadInterface & _Stream);
#endif
}

static void ConstructFloat3x3Default(Float3x3* p)
{
    new (p) Float3x3(1);
}

static void ConstructFloat3x3Floats(Float3x3* p, float _00, float _01, float _02, float _10, float _11, float _12, float _20, float _21, float _22)
{
    new (p) Float3x3(_00, _01, _02, _10, _11, _12, _20, _21, _22);
}

static void ConstructFloat3x3FromFloat3x3(Float3x3* p, const Float3x3& v)
{
    new (p) Float3x3(v);
}

static void ConstructFloat3x3Vecs(Float3x3* p, Float3 const& _Col0, Float3 const& _Col1, Float3 const& _Col2)
{
    new (p) Float3x3(_Col0, _Col1, _Col2);
}

static Float3 Float3x3GetRow(Float3x3& self, int i)
{
    if ((unsigned)i >= 3)
    {
        asIScriptContext* ctx = asGetActiveContext();
        ctx->SetException("Out of range");
        return {};
    }

    return self.GetRow(i);
}

static Float2x2 Float3x3ToFloat2x2(const Float3x3& self)
{
    return Float2x2(self);
}

static Float3x4 Float3x3ToFloat3x4(const Float3x3& self)
{
    return Float3x4(self);
}

static Float4x4 Float3x3ToFloat4x4(const Float3x3& self)
{
    return Float4x4(self);
}

static Float3x3 GetDiagonal3x3(float Diagonal)
{
    return Float3x3(Diagonal);
}

static Float3x3 GetDiagonal3x3(const Float3& Diagonal)
{
    return Float3x3(Diagonal);
}

//static std::string Float3x3ToString(Float3x3 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float3x3ToHexString(Float3x3 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterFloat3x3(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float3x3", "Float3 Col0", offsetof(Float3x3, Col0));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float3x3", "Float3 Col1", offsetof(Float3x3, Col1));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float3x3", "Float3 Col2", offsetof(Float3x3, Col2));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat3x3Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x3", asBEHAVE_CONSTRUCT, "void f(float, float, float, float, float, float, float, float, float)", asFUNCTION(ConstructFloat3x3Floats), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x3", asBEHAVE_CONSTRUCT, "void f(const Float3x3& in)", asFUNCTION(ConstructFloat3x3FromFloat3x3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x3", asBEHAVE_CONSTRUCT, "void f(const Float3& in, const Float3& in, const Float3& in)", asFUNCTION(ConstructFloat3x3Vecs), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x3", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float3x3>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3 &opIndex(int)", asFUNCTION(IndexOperator<Float3x3>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "const Float3 &opIndex(int) const", asFUNCTION(IndexOperator<Float3x3>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 &opAssign(const Float3x3 &in)", asMETHODPR(Float3x3, operator=, (const Float3x3&), Float3x3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "bool opEquals(const Float3x3 &in) const", asMETHOD(Float3x3, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3 GetRow(int) const", asFUNCTION(Float3x3GetRow), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "bool CompareEps(const Float3x3 &in, float) const", asMETHOD(Float3x3, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "void TransposeSelf()", asMETHOD(Float3x3, TransposeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 Transposed() const", asMETHOD(Float3x3, Transposed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "void InverseSelf()", asMETHOD(Float3x3, InverseSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 Inversed() const", asMETHOD(Float3x3, Inversed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "float Determinant() const", asMETHOD(Float3x3, Determinant), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "void Clear()", asMETHOD(Float3x3, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "void SetIdentity()", asMETHOD(Float3x3, SetIdentity), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 Scaled(const Float3 &in) const", asMETHOD(Float3x3, Scaled), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 RotateAroundNormal(float, const Float3 &in) const", asMETHOD(Float3x3, RotateAroundNormal), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 RotateAroundVector(float, const Float3 &in) const", asMETHOD(Float3x3, RotateAroundVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 opMul(float) const", asMETHODPR(Float3x3, operator*, (float) const, Float3x3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 opDiv(float) const", asMETHODPR(Float3x3, operator/, (float) const, Float3x3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3 opMul(const Float3 &in) const", asMETHODPR(Float3x3, operator*, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 opMul(const Float3x3 &in) const", asMETHODPR(Float3x3, operator*, (const Float3x3&) const, Float3x3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3& opMulAssign(const Float3x3 &in)", asMETHODPR(Float3x3, operator*=, (const Float3x3&), Float3x3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3& opMulAssign(float)", asMETHODPR(Float3x3, operator*=, (float), Float3x3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3& opDivAssign(float)", asMETHODPR(Float3x3, operator/=, (float), Float3x3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x3 ViewInverseFast() const", asMETHOD(Float3x3, ViewInverseFast), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float3x3", "string ToString(int=6) const", asFUNCTIONPR(Float3x3ToString, (Float3x3 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float3x3", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float3x3ToHexString, (Float3x3 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float2x2 ToFloat2x2() const", asFUNCTIONPR(Float3x3ToFloat2x2, (Float3x3 const&), Float2x2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float3x4 ToFloat3x4() const", asFUNCTIONPR(Float3x3ToFloat3x4, (Float3x3 const&), Float3x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x3", "Float4x4 ToFloat4x4() const", asFUNCTIONPR(Float3x3ToFloat4x4, (Float3x3 const&), Float4x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetScale3x3(const Float3 &in)", asFUNCTION(Float3x3::Scale), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetRotationAroundNormal3x3(float, const Float3 &in)", asFUNCTION(Float3x3::RotationAroundNormal), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetRotationAroundVector3x3(float, const Float3 &in)", asFUNCTION(Float3x3::RotationAroundVector), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetRotationX3x3(float)", asFUNCTION(Float3x3::RotationX), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetRotationY3x3(float)", asFUNCTION(Float3x3::RotationY), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetRotationZ3x3(float)", asFUNCTION(Float3x3::RotationZ), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetDiagonal3x3(float)", asFUNCTIONPR(GetDiagonal3x3, (float), Float3x3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x3 GetDiagonal3x3(const Float3 &in)", asFUNCTIONPR(GetDiagonal3x3, (const Float3&), Float3x3), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    void                   Write(IBinaryStreamWriteInterface & _Stream) const;
    void                   Read(IBinaryStreamReadInterface & _Stream);
#endif
}

static void ConstructFloat3x4Default(Float3x4* p)
{
    new (p) Float3x4(1);
}

static void ConstructFloat3x4Floats(Float3x4* p, float _00, float _01, float _02, float _03, float _10, float _11, float _12, float _13, float _20, float _21, float _22, float _23)
{
    new (p) Float3x4(_00, _01, _02, _03, _10, _11, _12, _13, _20, _21, _22, _23);
}

static void ConstructFloat3x4FromFloat3x4(Float3x4* p, const Float3x4& v)
{
    new (p) Float3x4(v);
}

static void ConstructFloat3x4Vecs(Float3x4* p, Float4 const& _Col0, Float4 const& _Col1, Float4 const& _Col2)
{
    new (p) Float3x4(_Col0, _Col1, _Col2);
}

static Float3 Float3x4GetRow(Float3x4& self, int i)
{
    if ((unsigned)i >= 4)
    {
        asIScriptContext* ctx = asGetActiveContext();
        ctx->SetException("Out of range");
        return {};
    }

    return self.GetRow(i);
}

static Float2x2 Float3x4ToFloat2x2(const Float3x4& self)
{
    return Float2x2(self);
}

static Float3x3 Float3x4ToFloat3x3(const Float3x4& self)
{
    return Float3x3(self);
}

static Float4x4 Float3x4ToFloat4x4(const Float3x4& self)
{
    return Float4x4(self);
}

static Float3x4 GetDiagonal3x4(float Diagonal)
{
    return Float3x4(Diagonal);
}

static Float3x4 GetDiagonal3x4(const Float3& Diagonal)
{
    return Float3x4(Diagonal);
}

//static std::string Float3x4ToString(Float3x4 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float3x4ToHexString(Float3x4 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterFloat3x4(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float3x4", "Float4 Col0", offsetof(Float3x4, Col0));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float3x4", "Float4 Col1", offsetof(Float3x4, Col1));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float3x4", "Float4 Col2", offsetof(Float3x4, Col2));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat3x4Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x4", asBEHAVE_CONSTRUCT, "void f(float, float, float, float, float, float, float, float, float, float, float, float)", asFUNCTION(ConstructFloat3x4Floats), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x4", asBEHAVE_CONSTRUCT, "void f(const Float3x4& in)", asFUNCTION(ConstructFloat3x4FromFloat3x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x4", asBEHAVE_CONSTRUCT, "void f(const Float4& in, const Float4& in, const Float4& in)", asFUNCTION(ConstructFloat3x4Vecs), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float3x4", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float3x4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float4 &opIndex(int)", asFUNCTION(IndexOperator<Float3x4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "const Float4 &opIndex(int) const", asFUNCTION(IndexOperator<Float3x4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4 &opAssign(const Float3x4 &in)", asMETHODPR(Float3x4, operator=, (const Float3x4&), Float3x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "bool opEquals(const Float3x4 &in) const", asMETHOD(Float3x4, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3 GetRow(int) const", asFUNCTION(Float3x4GetRow), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "bool CompareEps(const Float3x4 &in, float) const", asMETHOD(Float3x4, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void Compose(const Float3 &in, const Float3x3 &in, const Float3 &in)", asMETHODPR(Float3x4, Compose, (Float3 const&, Float3x3 const&, Float3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void Compose(const Float3 &in, const Float3x3 &in)", asMETHODPR(Float3x4, Compose, (Float3 const&, Float3x3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void SetTranslation(const Float3 &in)", asMETHOD(Float3x4, SetTranslation), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void DecomposeAll(Float3 &out, Float3x3 &out, Float3 &out) const", asMETHOD(Float3x4, DecomposeAll), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3 DecomposeTranslation() const", asMETHOD(Float3x4, DecomposeTranslation), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x3 DecomposeRotation() const", asMETHOD(Float3x4, DecomposeRotation), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3 DecomposeScale() const", asMETHOD(Float3x4, DecomposeScale), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void DecomposeRotationAndScale(Float3x3 &out, Float3 &out) const", asMETHOD(Float3x4, DecomposeRotationAndScale), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void DecomposeNormalMatrix(Float3x3 &out) const", asMETHOD(Float3x4, DecomposeNormalMatrix), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void InverseSelf()", asMETHOD(Float3x4, InverseSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4 Inversed() const", asMETHOD(Float3x4, Inversed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "float Determinant() const", asMETHOD(Float3x4, Determinant), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void Clear()", asMETHOD(Float3x4, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "void SetIdentity()", asMETHOD(Float3x4, SetIdentity), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4 opMul(float) const", asMETHODPR(Float3x4, operator*, (float) const, Float3x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4 opDiv(float) const", asMETHODPR(Float3x4, operator/, (float) const, Float3x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3 opMul(const Float3 &in) const", asMETHODPR(Float3x4, operator*, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3 opMul(const Float2 &in) const", asMETHODPR(Float3x4, operator*, (const Float2&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float2 Mult_Float2_IgnoreZ(const Float2 &in) const", asMETHODPR(Float3x4, Mult_Float2_IgnoreZ, (Float2 const&) const, Float2), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4 opMul(const Float3x4 &in) const", asMETHODPR(Float3x4, operator*, (const Float3x4&) const, Float3x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4& opMulAssign(const Float3x4 &in)", asMETHODPR(Float3x4, operator*=, (const Float3x4&), Float3x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4& opMulAssign(float)", asMETHODPR(Float3x4, operator*=, (float), Float3x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x4& opDivAssign(float)", asMETHODPR(Float3x4, operator/=, (float), Float3x4&), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float3x4", "string ToString(int=6) const", asFUNCTIONPR(Float3x4ToString, (Float3x4 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float3x4", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float3x4ToHexString, (Float3x4 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float2x2 ToFloat2x2() const", asFUNCTIONPR(Float3x4ToFloat2x2, (Float3x4 const&), Float2x2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float3x3 ToFloat3x4() const", asFUNCTIONPR(Float3x4ToFloat3x3, (Float3x4 const&), Float3x3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float3x4", "Float4x4 ToFloat4x4() const", asFUNCTIONPR(Float3x4ToFloat4x4, (Float3x4 const&), Float4x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetTranslation3x4(const Float3 &in)", asFUNCTION(Float3x4::Translation), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetScale3x4(const Float3 &in)", asFUNCTION(Float3x4::Scale), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetRotationAroundNormal3x4(float, const Float3 &in)", asFUNCTION(Float3x4::RotationAroundNormal), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetRotationAroundVector3x4(float, const Float3 &in)", asFUNCTION(Float3x4::RotationAroundVector), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetRotationX3x4(float)", asFUNCTION(Float3x4::RotationX), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetRotationY3x4(float)", asFUNCTION(Float3x4::RotationY), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetRotationZ3x4(float)", asFUNCTION(Float3x4::RotationZ), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetDiagonal3x4(float)", asFUNCTIONPR(GetDiagonal3x4, (float), Float3x4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3x4 GetDiagonal3x4(const Float3 &in)", asFUNCTIONPR(GetDiagonal3x4, (const Float3&), Float3x4), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    void                   Write(IBinaryStreamWriteInterface & _Stream) const;
    void                   Read(IBinaryStreamReadInterface & _Stream);
#endif
}

static void ConstructFloat4x4Default(Float4x4* p)
{
    new (p) Float4x4(1);
}

static void ConstructFloat4x4Floats(Float4x4* p, float _00, float _01, float _02, float _03, float _10, float _11, float _12, float _13, float _20, float _21, float _22, float _23, float _30, float _31, float _32, float _33)
{
    new (p) Float4x4(_00, _01, _02, _03, _10, _11, _12, _13, _20, _21, _22, _23, _30, _31, _32, _33);
}

static void ConstructFloat4x4FromFloat4x4(Float4x4* p, const Float4x4& v)
{
    new (p) Float4x4(v);
}

static void ConstructFloat4x4Vecs(Float4x4* p, Float4 const& _Col0, Float4 const& _Col1, Float4 const& _Col2, Float4 const& _Col3)
{
    new (p) Float4x4(_Col0, _Col1, _Col2, _Col3);
}

static Float4 Float4x4GetRow(Float4x4& self, int i)
{
    if ((unsigned)i >= 4)
    {
        asIScriptContext* ctx = asGetActiveContext();
        ctx->SetException("Out of range");
        return {};
    }

    return self.GetRow(i);
}

static Float2x2 Float4x4ToFloat2x2(const Float4x4& self)
{
    return Float2x2(self);
}

static Float3x4 Float4x4ToFloat3x4(const Float4x4& self)
{
    return Float3x4(self);
}

static Float3x3 Float4x4ToFloat3x3(const Float4x4& self)
{
    return Float3x3(self);
}

static Float4x4 GetDiagonal4x4(float Diagonal)
{
    return Float4x4(Diagonal);
}

static Float4x4 GetDiagonal4x4(const Float4& Diagonal)
{
    return Float4x4(Diagonal);
}

//static std::string Float4x4ToString(Float4x4 const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string Float4x4ToHexString(Float4x4 const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterFloat4x4(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Float4x4", "Float4 Col0", offsetof(Float4x4, Col0));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float4x4", "Float4 Col1", offsetof(Float4x4, Col1));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float4x4", "Float4 Col2", offsetof(Float4x4, Col2));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Float4x4", "Float4 Col3", offsetof(Float4x4, Col3));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4x4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructFloat4x4Default), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4x4", asBEHAVE_CONSTRUCT, "void f(float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float)", asFUNCTION(ConstructFloat4x4Floats), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4x4", asBEHAVE_CONSTRUCT, "void f(const Float4x4& in)", asFUNCTION(ConstructFloat4x4FromFloat4x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4x4", asBEHAVE_CONSTRUCT, "void f(const Float4& in, const Float4& in, const Float4& in, const Float4& in)", asFUNCTION(ConstructFloat4x4Vecs), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Float4x4", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Float4x4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4 &opIndex(int)", asFUNCTION(IndexOperator<Float4x4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "const Float4 &opIndex(int) const", asFUNCTION(IndexOperator<Float4x4>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 &opAssign(const Float4x4 &in)", asMETHODPR(Float4x4, operator=, (const Float4x4&), Float4x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "bool opEquals(const Float4x4 &in) const", asMETHOD(Float4x4, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4 GetRow(int) const", asFUNCTION(Float4x4GetRow), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "bool CompareEps(const Float4x4 &in, float) const", asMETHOD(Float4x4, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "void TransposeSelf()", asMETHOD(Float4x4, TransposeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 Transposed() const", asMETHOD(Float4x4, Transposed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "void InverseSelf()", asMETHOD(Float4x4, InverseSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 Inversed() const", asMETHOD(Float4x4, Inversed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "float Determinant() const", asMETHOD(Float4x4, Determinant), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "void Clear()", asMETHOD(Float4x4, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "void SetIdentity()", asMETHOD(Float4x4, SetIdentity), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 Translated(const Float3 &in) const", asMETHOD(Float4x4, Translated), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 Scaled(const Float3 &in) const", asMETHOD(Float4x4, Scaled), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 RotateAroundNormal(float, const Float3 &in) const", asMETHOD(Float4x4, RotateAroundNormal), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 RotateAroundVector(float, const Float3 &in) const", asMETHOD(Float4x4, RotateAroundVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float3 TransformAsFloat3x3(const Float3 &in) const", asMETHODPR(Float4x4, TransformAsFloat3x3, (Float3 const&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float3x3 TransformAsFloat3x3(const Float3x3 &in) const", asMETHODPR(Float4x4, TransformAsFloat3x3, (Float3x3 const&) const, Float3x3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 opMul(float) const", asMETHODPR(Float4x4, operator*, (float) const, Float4x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 opDiv(float) const", asMETHODPR(Float4x4, operator/, (float) const, Float4x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4 opMul(const Float4 &in) const", asMETHODPR(Float4x4, operator*, (const Float4&) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4 opMul(const Float3 &in) const", asMETHODPR(Float4x4, operator*, (const Float3&) const, Float4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 opMul(const Float4x4 &in) const", asMETHODPR(Float4x4, operator*, (const Float4x4&) const, Float4x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4& opMulAssign(const Float4x4 &in)", asMETHODPR(Float4x4, operator*=, (const Float4x4&), Float4x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4& opMulAssign(const Float3x4 &in)", asMETHODPR(Float4x4, operator*=, (const Float3x4&), Float4x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4& opMulAssign(float)", asMETHODPR(Float4x4, operator*=, (float), Float4x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4& opDivAssign(float)", asMETHODPR(Float4x4, operator/=, (float), Float4x4&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 ViewInverseFast() const", asMETHOD(Float4x4, ViewInverseFast), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 PerspectiveProjectionInverseFast() const", asMETHOD(Float4x4, PerspectiveProjectionInverseFast), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float4x4 OrthoProjectionInverseFast() const", asMETHOD(Float4x4, OrthoProjectionInverseFast), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float4x4", "string ToString(int=6) const", asFUNCTIONPR(Float4x4ToString, (Float4x4 const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Float4x4", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(Float4x4ToHexString, (Float4x4 const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float2x2 ToFloat2x2() const", asFUNCTIONPR(Float4x4ToFloat2x2, (Float4x4 const&), Float2x2), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float3x4 ToFloat3x4() const", asFUNCTIONPR(Float4x4ToFloat3x4, (Float4x4 const&), Float3x4), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Float4x4", "Float3x3 ToFloat4x4() const", asFUNCTIONPR(Float4x4ToFloat3x3, (Float4x4 const&), Float3x3), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetTranslation4x4(const Float3 &in)", asFUNCTION(Float4x4::Translation), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetScale4x4(const Float3 &in)", asFUNCTION(Float4x4::Scale), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetRotationAroundNormal4x4(float, const Float3 &in)", asFUNCTION(Float4x4::RotationAroundNormal), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetRotationAroundVector4x4(float, const Float3 &in)", asFUNCTION(Float4x4::RotationAroundVector), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetRotationX4x4(float)", asFUNCTION(Float4x4::RotationX), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetRotationY4x4(float)", asFUNCTION(Float4x4::RotationY), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetRotationZ4x4(float)", asFUNCTION(Float4x4::RotationZ), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 LookAt(const Float3 &in, const Float3 &in, const Float3 &in)", asFUNCTION(Float4x4::LookAt), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetDiagonal4x4(float)", asFUNCTIONPR(GetDiagonal4x4, (float), Float4x4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4x4 GetDiagonal4x4(const Float4 &in)", asFUNCTIONPR(GetDiagonal4x4, (const Float4&), Float4x4), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    static Float4x4 const& ClipControl_UpperLeft_ZeroToOne();
    static Float4x4        Ortho2D(Float2 const& _Mins, Float2 const& _Maxs);
    static Float4x4        Ortho2DCC(Float2 const& _Mins, Float2 const& _Maxs);
    static Float4x4        Ortho(Float2 const& _Mins, Float2 const& _Maxs, float _ZNear, float _ZFar);
    static Float4x4        OrthoCC(Float2 const& _Mins, Float2 const& _Maxs, float _ZNear, float _ZFar);
    static Float4x4        OrthoRev(Float2 const& _Mins, Float2 const& _Maxs, float _ZNear, float _ZFar);
    static Float4x4        OrthoRevCC(Float2 const& _Mins, Float2 const& _Maxs, float _ZNear, float _ZFar);
    static Float4x4        Perspective(float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar);
    static Float4x4        Perspective(float _FovXRad, float _FovYRad, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveCC(float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveCC(float _FovXRad, float _FovYRad, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveRev(float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveRev(float _FovXRad, float _FovYRad, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveRevCC(float _FovXRad, float _Width, float _Height, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveRevCC_Y(float _FovYRad, float _Width, float _Height, float _ZNear, float _ZFar);
    static Float4x4        PerspectiveRevCC(float _FovXRad, float _FovYRad, float _ZNear, float _ZFar);
    static void            GetCubeFaceMatrices(Float4x4 & _PositiveX,
                                    Float4x4 & _NegativeX,
                                    Float4x4 & _PositiveY,
                                    Float4x4 & _NegativeY,
                                    Float4x4 & _PositiveZ,
                                    Float4x4 & _NegativeZ);
    static Float4x4 const* GetCubeFaceMatrices();

    void                   Write(IBinaryStreamWriteInterface & _Stream) const;
    void                   Read(IBinaryStreamReadInterface & _Stream);
#endif
}

static void ConstructQuatDefault(Quat* p)
{
    new (p) Quat(Quat::Identity());
}

static void ConstructQuatWXYZ(Quat* p, float W, float X, float Y, float Z)
{
    new (p) Quat(W, X, Y, Z);
}

static void ConstructQuatPYR(Quat* p, float Pitch, float Yaw, float Roll)
{
    new (p) Quat(Pitch, Yaw, Roll);
}

static void ConstructQuatFromQuat(Quat* p, Quat const& other)
{
    new (p) Quat(other);
}

//static std::string QuatToString(Quat const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string QuatToHexString(Quat const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterQuat(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Quat", "float X", offsetof(Quat, X));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Quat", "float Y", offsetof(Quat, Y));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Quat", "float Z", offsetof(Quat, Z));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Quat", "float W", offsetof(Quat, W));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Quat", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructQuatDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Quat", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(ConstructQuatWXYZ), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Quat", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(ConstructQuatPYR), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Quat", asBEHAVE_CONSTRUCT, "void f(const Quat &in)", asFUNCTION(ConstructQuatFromQuat), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Quat", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Quat>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat &opAssign(const Quat &in)", asMETHODPR(Quat, operator=, (const Quat&), Quat&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "float &opIndex(int)", asFUNCTION(IndexOperator<Quat>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "const float &opIndex(int) const", asFUNCTION(IndexOperator<Quat>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "bool opEquals(const Quat &in) const", asMETHOD(Quat, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat opNeg() const", asMETHODPR(Quat, operator-, () const, Quat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat opAdd(const Quat &in) const", asMETHODPR(Quat, operator+, (const Quat&) const, Quat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat opSub(const Quat &in) const", asMETHODPR(Quat, operator-, (const Quat&) const, Quat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat opMul(const Quat &in) const", asMETHODPR(Quat, operator*, (const Quat&) const, Quat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Float3 opMul(const Float3 &in) const", asMETHODPR(Quat, operator*, (const Float3&) const, Float3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat opMul(float) const", asMETHODPR(Quat, operator*, (float) const, Quat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat opDiv(float) const", asMETHODPR(Quat, operator/, (float) const, Quat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat& opAddAssign(const Quat &in)", asMETHODPR(Quat, operator+=, (const Quat&), Quat&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat& opSubAssign(const Quat &in)", asMETHODPR(Quat, operator-=, (const Quat&), Quat&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat& opMulAssign(const Quat &in)", asMETHODPR(Quat, operator*=, (const Quat&), Quat&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat& opMulAssign(float)", asMETHODPR(Quat, operator*=, (float), Quat&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat& opDivAssign(float)", asMETHODPR(Quat, operator/=, (float), Quat&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "bool CompareEps(const Quat &in, float) const", asMETHOD(Quat, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "float NormalizeSelf()", asMETHOD(Quat, NormalizeSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat Normalized() const", asMETHOD(Quat, Normalized), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "void InverseSelf()", asMETHOD(Quat, InverseSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat Inversed() const", asMETHOD(Quat, Inversed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "void ConjugateSelf()", asMETHOD(Quat, ConjugateSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat Conjugated() const", asMETHOD(Quat, Conjugated), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "float ComputeW() const", asMETHOD(Quat, ComputeW), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Float3 XAxis() const", asMETHOD(Quat, XAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Float3 YAxis() const", asMETHOD(Quat, YAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Float3 ZAxis() const", asMETHOD(Quat, ZAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "void SetIdentity()", asMETHOD(Quat, SetIdentity), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat RotateAroundNormal(float, const Float3 &in) const", asMETHOD(Quat, RotateAroundNormal), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Quat RotateAroundVector(float, const Float3 &in) const", asMETHOD(Quat, RotateAroundVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "void ToAngles(float &out, float &out, float &out) const", asMETHOD(Quat, ToAngles), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "void FromAngles(float, float, float)", asMETHOD(Quat, FromAngles), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Float3x3 ToMatrix3x3() const", asMETHOD(Quat, ToMatrix3x3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "Float4x4 ToMatrix4x4() const", asMETHOD(Quat, ToMatrix4x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "void FromMatrix(const Float3x3 &in)", asMETHOD(Quat, FromMatrix), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "float Pitch() const", asMETHOD(Quat, Pitch), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "float Yaw() const", asMETHOD(Quat, Yaw), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Quat", "float Roll() const", asMETHOD(Quat, Roll), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Quat", "string ToString(int=6) const", asFUNCTIONPR(QuatToString, (Quat const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Quat", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(QuatToHexString, (Quat const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Quat GetRotationAroundNormalQuat(float, const Float3 &in)", asFUNCTION(Quat::RotationAroundNormal), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Quat GetRotationAroundVectorQuat(float, const Float3 &in)", asFUNCTION(Quat::RotationAroundVector), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Quat GetRotationXQuat(float)", asFUNCTION(Quat::RotationX), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Quat GetRotationYQuat(float)", asFUNCTION(Quat::RotationY), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Quat GetRotationZQuat(float)", asFUNCTION(Quat::RotationZ), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Quat Slerp(const Quat &in, const Quat &in, float)", asFUNCTION(Math::Slerp), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    Bool4 IsInfinite() const;
    Bool4 IsNan() const;
    Bool4 IsNormal() const;
    Bool4 NotEqual( Quat const & _Other ) const;
    void Write( IBinaryStreamWriteInterface & _Stream ) const;
    void Read( IBinaryStreamReadInterface & _Stream );
    static constexpr int NumComponents();
    explicit constexpr Quat( Float4 const & _Value );
    HK_FORCEINLINE Quat operator*( float _Left, Quat const & _Right );
#endif
}

static void ConstructAnglDefault(Angl* p)
{
    new (p) Angl(0, 0, 0);
}

static void ConstructAnglPYR(Angl* p, float Pitch, float Yaw, float Roll)
{
    new (p) Angl(Pitch, Yaw, Roll);
}

static void ConstructAnglFromAngl(Angl* p, Angl const& other)
{
    new (p) Angl(other);
}

//static std::string AnglToString(Angl const& self, int Precision)
//{
//    return self.ToString(Precision).CStr();
//}

//static std::string AnglToHexString(Angl const& self, bool bLeadingZeros, bool bPrefix)
//{
//    return self.ToHexString(bLeadingZeros, bPrefix).CStr();
//}

static void RegisterAngl(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("Angl", "float Pitch", offsetof(Angl, Pitch));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Angl", "float Yaw", offsetof(Angl, Yaw));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("Angl", "float Roll", offsetof(Angl, Roll));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Angl", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructAnglDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Angl", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(ConstructAnglPYR), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Angl", asBEHAVE_CONSTRUCT, "void f(const Angl &in)", asFUNCTION(ConstructAnglFromAngl), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("Angl", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<Angl>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl &opAssign(const Angl &in)", asMETHODPR(Angl, operator=, (const Angl&), Angl&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "float &opIndex(int)", asFUNCTION(IndexOperator<Angl>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "const float &opIndex(int) const", asFUNCTION(IndexOperator<Angl>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "bool opEquals(const Angl &in) const", asMETHOD(Angl, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "bool CompareEps(const Angl &in, float) const", asMETHOD(Angl, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl opNeg() const", asMETHODPR(Angl, operator-, () const, Angl), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl opAdd(const Angl &in) const", asMETHODPR(Angl, operator+, (const Angl&) const, Angl), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl opSub(const Angl &in) const", asMETHODPR(Angl, operator-, (const Angl&) const, Angl), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl& opAddAssign(const Angl &in)", asMETHODPR(Angl, operator+=, (const Angl&), Angl&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl& opSubAssign(const Angl &in)", asMETHODPR(Angl, operator-=, (const Angl&), Angl&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "void Clear()", asMETHOD(Angl, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Quat ToQuat() const", asMETHOD(Angl, ToQuat), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Float3x3 ToMatrix3x3() const", asMETHOD(Angl, ToMatrix3x3), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Float4x4 ToMatrix4x4() const", asMETHOD(Angl, ToMatrix4x4), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "void Normalize360Self()", asMETHOD(Angl, Normalize360Self), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl Normalized360() const", asMETHOD(Angl, Normalized360), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "void Normalize180Self()", asMETHOD(Angl, Normalize180Self), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl Normalized180() const", asMETHOD(Angl, Normalized180), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Angl Delta(const Angl &in) const", asMETHOD(Angl, Delta), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "Float3& ToFloat3()", asMETHODPR(Angl, ToFloat3, (), Float3&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("Angl", "const Float3& ToFloat3() const", asMETHODPR(Angl, ToFloat3, () const, Float3 const&), asCALL_THISCALL);
    assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Angl", "string ToString(int=6) const", asFUNCTIONPR(AnglToString, (Angl const&, int), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    //r = pEngine->RegisterObjectMethod("Angl", "string ToHexString(bool=false, bool=false) const", asFUNCTIONPR(AnglToHexString, (Angl const&, bool, bool), std::string), asCALL_CDECL_OBJFIRST);
    //assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float AngleNormalize360(float)", asFUNCTION(Angl::Normalize360), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float AngleNormalize180(float)", asFUNCTION(Angl::Normalize180), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    Bool3 IsInfinite() const;
    Bool3 IsNan() const;
    Bool3 IsNormal() const;
    Bool3 NotEqual( Angl const & _Other ) const;
    void Write( IBinaryStreamWriteInterface & _Stream ) const;
    void Read( IBinaryStreamReadInterface & _Stream );
    static constexpr int NumComponents() { return 3; }
    static Angl const & Zero();
    explicit constexpr Angl( Float3 const & _Value );
    HK_FORCEINLINE Angl operator*( float _Left, Angl const & _Right )
    static byte PackByte( float _Angle );
    static uint16_t PackShort( float _Angle );
    static float UnpackByte( byte _Angle );
    static float UnpackShort( uint16_t _Angle );
#endif
}

static void ConstructTransformDefault(STransform* p)
{
    new (p) STransform(Float3(0), Quat::Identity(), Float3(1));
}

static void ConstructTransformPRS(STransform* p, Float3 const& Position, Quat const& Rotation, Float3 const& Scale)
{
    new (p) STransform(Position, Rotation, Scale);
}

static void ConstructTransformPR(STransform* p, Float3 const& Position, Quat const& Rotation)
{
    new (p) STransform(Position, Rotation);
}

static void ConstructTransformFromTransform(STransform* p, STransform const& other)
{
    new (p) STransform(other);
}

static void RegisterTransform(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("STransform", "Float3 Position", offsetof(STransform, Position));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("STransform", "Quat Rotation", offsetof(STransform, Rotation));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("STransform", "Float3 Scale", offsetof(STransform, Scale));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("STransform", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructTransformDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("STransform", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, const Quat &in, const Float3 &in)", asFUNCTION(ConstructTransformPRS), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("STransform", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, const Quat &in)", asFUNCTION(ConstructTransformPR), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("STransform", asBEHAVE_CONSTRUCT, "void f(const STransform &in)", asFUNCTION(ConstructTransformFromTransform), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("STransform", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<STransform>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "STransform &opAssign(const STransform &in)", asMETHODPR(STransform, operator=, (const STransform&), STransform&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void Clear()", asMETHOD(STransform, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void SetIdentity()", asMETHOD(STransform, SetIdentity), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void SetScale(const Float3 &in)", asMETHODPR(STransform, SetScale, (Float3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void SetScale(float, float, float)", asMETHODPR(STransform, SetScale, (float, float, float), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void SetScale(float)", asMETHODPR(STransform, SetScale, (float), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void SetAngles(const Angl &in)", asMETHODPR(STransform, SetAngles, (Angl const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void SetAngles(float, float, float)", asMETHODPR(STransform, SetAngles, (float, float, float), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Angl GetAngles() const", asMETHOD(STransform, GetAngles), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "float GetPitch() const", asMETHOD(STransform, GetPitch), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "float GetYaw() const", asMETHOD(STransform, GetYaw), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "float GetRoll() const", asMETHOD(STransform, GetRoll), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Float3 GetRightVector() const", asMETHOD(STransform, GetRightVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Float3 GetLeftVector() const", asMETHOD(STransform, GetLeftVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Float3 GetUpVector() const", asMETHOD(STransform, GetUpVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Float3 GetDownVector() const", asMETHOD(STransform, GetDownVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Float3 GetBackVector() const", asMETHOD(STransform, GetBackVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "Float3 GetForwardVector() const", asMETHOD(STransform, GetForwardVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void ComputeTransformMatrix(Float3x4 &out) const", asMETHOD(STransform, ComputeTransformMatrix), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void TurnRightFPS(float)", asMETHOD(STransform, TurnRightFPS), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void TurnLeftFPS(float)", asMETHOD(STransform, TurnLeftFPS), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void TurnUpFPS(float)", asMETHOD(STransform, TurnUpFPS), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void TurnDownFPS(float)", asMETHOD(STransform, TurnDownFPS), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void TurnAroundAxis(float, const Float3 &in)", asMETHOD(STransform, TurnAroundAxis), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void TurnAroundVector(float, const Float3 &in)", asMETHOD(STransform, TurnAroundVector), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void StepRight(float)", asMETHOD(STransform, StepRight), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void StepLeft(float)", asMETHOD(STransform, StepLeft), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void StepUp(float)", asMETHOD(STransform, StepUp), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void StepDown(float)", asMETHOD(STransform, StepDown), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void StepBack(float)", asMETHOD(STransform, StepBack), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void StepForward(float)", asMETHOD(STransform, StepForward), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void Step(const Float3 &in)", asMETHOD(STransform, Step), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "STransform Inversed() const", asMETHOD(STransform, Inversed), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "void InverseSelf()", asMETHOD(STransform, InverseSelf), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("STransform", "STransform opMul(const STransform &in) const", asMETHODPR(STransform, operator*, (const STransform&) const, STransform), asCALL_THISCALL);
    assert(r >= 0);

    HK_UNUSED(r);

// TODO?
#if 0
    void GetVectors( Float3 * _Right, Float3 * _Up, Float3 * _Back ) const;
    void Write( IBinaryStreamWriteInterface & _Stream ) const;
    void Read( IBinaryStreamReadInterface & _Stream );
#endif
}

static void ConstructAxisAlignedBoxDefault(BvAxisAlignedBox* p)
{
    new (p) BvAxisAlignedBox(BvAxisAlignedBox::Empty());
}

static void ConstructAxisAlignedBoxVecs(BvAxisAlignedBox* p, Float3 const& Mins, Float3 const& Maxs)
{
    new (p) BvAxisAlignedBox(Mins, Maxs);
}

static void ConstructAxisAlignedBoxPosRadius(BvAxisAlignedBox* p, Float3 const& Pos, float Radius)
{
    new (p) BvAxisAlignedBox(Pos, Radius);
}

static void ConstructAxisAlignedBoxFromAxisAlignedBox(BvAxisAlignedBox* p, BvAxisAlignedBox const& Box)
{
    new (p) BvAxisAlignedBox(Box);
}

static void RegisterAxisAlignedBox(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("BvAxisAlignedBox", "Float3 Mins", offsetof(BvAxisAlignedBox, Mins));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("BvAxisAlignedBox", "Float3 Maxs", offsetof(BvAxisAlignedBox, Maxs));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvAxisAlignedBox", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructAxisAlignedBoxDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvAxisAlignedBox", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, const Float3 &in)", asFUNCTION(ConstructAxisAlignedBoxVecs), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvAxisAlignedBox", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, float)", asFUNCTION(ConstructAxisAlignedBoxPosRadius), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvAxisAlignedBox", asBEHAVE_CONSTRUCT, "void f(const BvAxisAlignedBox &in)", asFUNCTION(ConstructAxisAlignedBoxFromAxisAlignedBox), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvAxisAlignedBox", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<BvAxisAlignedBox>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox &opAssign(const BvAxisAlignedBox &in)", asMETHODPR(BvAxisAlignedBox, operator=, (const BvAxisAlignedBox&), BvAxisAlignedBox&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "bool opEquals(const BvAxisAlignedBox &in) const", asMETHOD(BvAxisAlignedBox, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "bool CompareEps(const BvAxisAlignedBox &in, float) const", asMETHOD(BvAxisAlignedBox, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void Clear()", asMETHOD(BvAxisAlignedBox, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void AddPoint(const Float3 &in)", asMETHODPR(BvAxisAlignedBox, AddPoint, (Float3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void AddPoint(float, float, float)", asMETHODPR(BvAxisAlignedBox, AddPoint, (float, float, float), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void AddAABB(const BvAxisAlignedBox &in)", asMETHODPR(BvAxisAlignedBox, AddAABB, (BvAxisAlignedBox const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void AddAABB(const Float3 &in, const Float3 &in)", asMETHODPR(BvAxisAlignedBox, AddAABB, (Float3 const&, Float3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void AddSphere(const Float3 &in, float)", asMETHOD(BvAxisAlignedBox, AddSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "Float3 Center() const", asMETHOD(BvAxisAlignedBox, Center), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float Radius() const", asMETHOD(BvAxisAlignedBox, Radius), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "Float3 Size() const", asMETHOD(BvAxisAlignedBox, Size), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "Float3 HalfSize() const", asMETHOD(BvAxisAlignedBox, HalfSize), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float Width() const", asMETHOD(BvAxisAlignedBox, Width), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float Height() const", asMETHOD(BvAxisAlignedBox, Height), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float Depth() const", asMETHOD(BvAxisAlignedBox, Depth), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float Volume() const", asMETHOD(BvAxisAlignedBox, Volume), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float LongestAxisSize() const", asMETHOD(BvAxisAlignedBox, LongestAxisSize), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "float ShortestAxisSize() const", asMETHOD(BvAxisAlignedBox, ShortestAxisSize), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "void FromSphere(const Float3 &in, float)", asMETHOD(BvAxisAlignedBox, FromSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "bool IsEmpty() const", asMETHOD(BvAxisAlignedBox, IsEmpty), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox Transform(const Float3 &in, const Float3x3 &in) const", asMETHODPR(BvAxisAlignedBox, Transform, (Float3 const&, Float3x3 const&) const, BvAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox Transform(const Float3x4 &in) const", asMETHODPR(BvAxisAlignedBox, Transform, (Float3x4 const&) const, BvAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox FromOrientedBox(const Float3 &in, const Float3 &in, const Float3x3 &in) const", asMETHOD(BvAxisAlignedBox, FromOrientedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox opAdd(const Float3 &in) const", asMETHODPR(BvAxisAlignedBox, operator+, (const Float3&) const, BvAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox opSub(const Float3 &in) const", asMETHODPR(BvAxisAlignedBox, operator-, (const Float3&) const, BvAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox opMul(float) const", asMETHODPR(BvAxisAlignedBox, operator*, (float) const, BvAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox opDiv(float) const", asMETHODPR(BvAxisAlignedBox, operator/, (float) const, BvAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox& opAddAssign(const Float3 &in)", asMETHODPR(BvAxisAlignedBox, operator+=, (const Float3&), BvAxisAlignedBox&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox& opSubAssign(const Float3 &in)", asMETHODPR(BvAxisAlignedBox, operator-=, (const Float3&), BvAxisAlignedBox&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox& opMulAssign(float)", asMETHODPR(BvAxisAlignedBox, operator*=, (float), BvAxisAlignedBox&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvAxisAlignedBox", "BvAxisAlignedBox& opDivAssign(float)", asMETHODPR(BvAxisAlignedBox, operator/=, (float), BvAxisAlignedBox&), asCALL_THISCALL);
    assert(r >= 0);

    HK_UNUSED(r);

    // TODO?
#if 0
    void   GetVertices( Float3 _Vertices[8] ) const;
    static BvAxisAlignedBox const & Empty();
    void Write( IBinaryStreamWriteInterface & _Stream ) const;
    void Read( IBinaryStreamReadInterface & _Stream );
#endif
}


static void ConstructOrientedBoxDefault(BvOrientedBox* p)
{
    new (p) BvOrientedBox();
}

static void ConstructOrientedBoxVecs(BvOrientedBox* p, Float3 const& Center, Float3 const& _HalfSize)
{
    new (p) BvOrientedBox(Center, _HalfSize);
}

static void ConstructOrientedBoxFromOrientedBox(BvOrientedBox* p, BvOrientedBox const& Box)
{
    new (p) BvOrientedBox(Box);
}

static void RegisterOrientedBox(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("BvOrientedBox", "Float3 Center", offsetof(BvOrientedBox, Center));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("BvOrientedBox", "Float3 HalfSize", offsetof(BvOrientedBox, HalfSize));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("BvOrientedBox", "Float3x3 Orient", offsetof(BvOrientedBox, Orient));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvOrientedBox", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructOrientedBoxDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvOrientedBox", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, const Float3 &in)", asFUNCTION(ConstructOrientedBoxVecs), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvOrientedBox", asBEHAVE_CONSTRUCT, "void f(const BvOrientedBox &in)", asFUNCTION(ConstructOrientedBoxFromOrientedBox), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvOrientedBox", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<BvOrientedBox>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvOrientedBox", "BvOrientedBox &opAssign(const BvOrientedBox &in)", asMETHODPR(BvOrientedBox, operator=, (const BvOrientedBox&), BvOrientedBox&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvOrientedBox", "bool opEquals(const BvOrientedBox &in) const", asMETHOD(BvOrientedBox, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvOrientedBox", "void FromAxisAlignedBox(const BvAxisAlignedBox &in, const Float3 &in, const Float3x3 &in)", asMETHODPR(BvOrientedBox, FromAxisAlignedBox, (BvAxisAlignedBox const&, Float3 const&, Float3x3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvOrientedBox", "void FromAxisAlignedBoxWithPadding(const BvAxisAlignedBox &in, const Float3 &in, const Float3x3 &in, float)", asMETHODPR(BvOrientedBox, FromAxisAlignedBoxWithPadding, (BvAxisAlignedBox const&, Float3 const&, Float3x3 const&, float), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvOrientedBox", "void FromAxisAlignedBox(const BvAxisAlignedBox &in, const Float3x4 &in)", asMETHODPR(BvOrientedBox, FromAxisAlignedBox, (BvAxisAlignedBox const&, Float3x4 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvOrientedBox", "void FromAxisAlignedBoxWithPadding(const BvAxisAlignedBox &in, const Float3x4 &in, float)", asMETHODPR(BvOrientedBox, FromAxisAlignedBoxWithPadding, (BvAxisAlignedBox const&, Float3x4 const&, float), void), asCALL_THISCALL);
    assert(r >= 0);

    HK_UNUSED(r);
    // TODO?
#if 0
    void GetVertices( Float3 _Vertices[8] ) const;
#endif
}


static void ConstructSphereDefault(BvSphere* p)
{
    new (p) BvSphere(Float3(0), 0);
}

static void ConstructSphereCenterRadius(BvSphere* p, Float3 const& Center, float Radius)
{
    new (p) BvSphere(Center, Radius);
}

static void ConstructSphereFromSphere(BvSphere* p, BvSphere const& Sphere)
{
    new (p) BvSphere(Sphere);
}

static void RegisterSphere(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectProperty("BvSphere", "Float3 Center", offsetof(BvSphere, Center));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("BvSphere", "float Radius", offsetof(BvSphere, Radius));
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvSphere", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructSphereDefault), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvSphere", asBEHAVE_CONSTRUCT, "void f(const Float3 &in, float)", asFUNCTION(ConstructSphereCenterRadius), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvSphere", asBEHAVE_CONSTRUCT, "void f(const BvSphere &in)", asFUNCTION(ConstructSphereFromSphere), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("BvSphere", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct<BvSphere>), asCALL_CDECL_OBJFIRST);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere &opAssign(const BvSphere &in)", asMETHODPR(BvSphere, operator=, (const BvSphere&), BvSphere&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "bool opEquals(const BvSphere &in) const", asMETHOD(BvSphere, operator==), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "bool CompareEps(const BvSphere &in, float) const", asMETHOD(BvSphere, CompareEps), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere opAdd(const Float3 &in) const", asMETHODPR(BvSphere, operator+, (const Float3&) const, BvSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere opSub(const Float3 &in) const", asMETHODPR(BvSphere, operator-, (const Float3&) const, BvSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere opMul(float) const", asMETHODPR(BvSphere, operator*, (float) const, BvSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere opDiv(float) const", asMETHODPR(BvSphere, operator/, (float) const, BvSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere& opAddAssign(const Float3 &in)", asMETHODPR(BvSphere, operator+=, (const Float3&), BvSphere&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere& opSubAssign(const Float3 &in)", asMETHODPR(BvSphere, operator-=, (const Float3&), BvSphere&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere& opMulAssign(float)", asMETHODPR(BvSphere, operator*=, (float), BvSphere&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "BvSphere& opDivAssign(float)", asMETHODPR(BvSphere, operator/=, (float), BvSphere&), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "void Clear()", asMETHOD(BvSphere, Clear), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "void AddPoint(const Float3 &in)", asMETHODPR(BvSphere, AddPoint, (Float3 const&), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "void AddPoint(float, float, float)", asMETHODPR(BvSphere, AddPoint, (float, float, float), void), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "void AddSphere(const BvSphere &in)", asMETHOD(BvSphere, AddSphere), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "void FromAxisAlignedBox(const BvAxisAlignedBox &in)", asMETHOD(BvSphere, FromAxisAlignedBox), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectMethod("BvSphere", "float Dist(const Plane &in) const", asMETHOD(BvSphere, Dist), asCALL_THISCALL);
    assert(r >= 0);

    HK_UNUSED(r);
    // TODO?
#if 0
    explicit BvSphere( float _Radius );
    void FromPointsAverage( Float3 const * _Points, int _NumPoints );
    void FromPoints( Float3 const * _Points, int _NumPoints );
    void FromPointsAroundCenter( Float3 const & _Center, Float3 const * _Points, int _NumPoints );
#endif
}

void RegisterMath(asIScriptEngine* pEngine)
{
    int r;

    r = pEngine->RegisterObjectType("Float2", sizeof(Float2), asOBJ_VALUE | asGetTypeTraits<Float2>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Float3", sizeof(Float3), asOBJ_VALUE | asGetTypeTraits<Float3>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Float4", sizeof(Float4), asOBJ_VALUE | asGetTypeTraits<Float4>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Float2x2", sizeof(Float2x2), asOBJ_VALUE | asGetTypeTraits<Float2x2>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Float3x3", sizeof(Float3x3), asOBJ_VALUE | asGetTypeTraits<Float3x3>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Float3x4", sizeof(Float3x4), asOBJ_VALUE | asGetTypeTraits<Float3x4>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Float4x4", sizeof(Float4x4), asOBJ_VALUE | asGetTypeTraits<Float4x4>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Quat", sizeof(Quat), asOBJ_VALUE | asGetTypeTraits<Quat>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Angl", sizeof(Angl), asOBJ_VALUE | asGetTypeTraits<Angl>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("STransform", sizeof(STransform), asOBJ_VALUE | asGetTypeTraits<STransform>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("Plane", sizeof(PlaneF), asOBJ_VALUE | asGetTypeTraits<PlaneF>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("BvAxisAlignedBox", sizeof(BvAxisAlignedBox), asOBJ_VALUE | asGetTypeTraits<BvAxisAlignedBox>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("BvOrientedBox", sizeof(BvOrientedBox), asOBJ_VALUE | asGetTypeTraits<BvOrientedBox>());
    assert(r >= 0);
    r = pEngine->RegisterObjectType("BvSphere", sizeof(BvSphere), asOBJ_VALUE | asGetTypeTraits<BvSphere>());
    assert(r >= 0);

    RegisterFloat2(pEngine);
    RegisterFloat3(pEngine);
    RegisterFloat4(pEngine);
    RegisterFloat2x2(pEngine);
    RegisterFloat3x3(pEngine);
    RegisterFloat3x4(pEngine);
    RegisterFloat4x4(pEngine);
    RegisterQuat(pEngine);
    RegisterAngl(pEngine);
    RegisterTransform(pEngine);
    RegisterPlane(pEngine);
    RegisterAxisAlignedBox(pEngine);
    RegisterOrientedBox(pEngine);
    RegisterSphere(pEngine);

    r = pEngine->RegisterGlobalFunction("float Dot(const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Dot<float>, (const Float2&, const Float2&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Dot(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Dot<float>, (const Float3&, const Float3&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Dot(const Float4 &in, const Float4 &in)", asFUNCTIONPR(Math::Dot<float>, (const Float4&, const Float4&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Dot(const Plane &in, const Float3 &in)", asFUNCTIONPR(Math::Dot<float>, (const PlaneF&, const Float3&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Dot(const Float3 &in, const Plane &in)", asFUNCTIONPR(Math::Dot<float>, (const Float3&, const PlaneF&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Dot(const Plane &in, const Float4 &in)", asFUNCTIONPR(Math::Dot<float>, (const PlaneF&, const Float4&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Dot(const Float4 &in, const Plane &in)", asFUNCTIONPR(Math::Dot<float>, (const Float4&, const PlaneF&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Cross(const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Cross<float>, (const Float2&, const Float2&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Cross(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Cross<float>, (const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Reflect(const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Reflect<float>, (const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Reflect(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Reflect<float>, (const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Refract(const Float2 &in, const Float2 &in, float)", asFUNCTIONPR(Math::Refract<float>, (const Float2&, const Float2&, float), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Refract(const Float3 &in, const Float3 &in, float)", asFUNCTIONPR(Math::Refract<float>, (const Float3&, const Float3&, float), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 ProjectVector(const Float3 &in, const Float3 &in, float)", asFUNCTIONPR(Math::ProjectVector<float>, (const Float3&, const Float3&, float), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 ProjectVector(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::ProjectVector<float>, (const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Lerp(const Float2 &in, const Float2 &in, float)", asFUNCTIONPR(Math::Lerp<float>, (const Float2&, const Float2&, float), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Lerp(const Float3 &in, const Float3 &in, float)", asFUNCTIONPR(Math::Lerp<float>, (const Float3&, const Float3&, float), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Lerp(const Float4 &in, const Float4 &in, float)", asFUNCTIONPR(Math::Lerp<float>, (const Float4&, const Float4&, float), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float Bilerp(float, float, float, float, const Float2 &in)", asFUNCTIONPR(Math::Bilerp<float>, (float, float, float, float, const Float2&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Bilerp(const Float2 &in, const Float2 &in, const Float2 &in, const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Bilerp<float>, (const Float2&, const Float2&, const Float2&, const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Bilerp(const Float3 &in, const Float3 &in, const Float3 &in, const Float3 &in, const Float2 &in)", asFUNCTIONPR(Math::Bilerp<float>, (const Float3&, const Float3&, const Float3&, const Float3&, const Float2&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Bilerp(const Float4 &in, const Float4 &in, const Float4 &in, const Float4 &in, const Float2 &in)", asFUNCTIONPR(Math::Bilerp<float>, (const Float4&, const Float4&, const Float4&, const Float4&, const Float2&), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Step(const Float2 &in, float)", asFUNCTIONPR(Math::Step<float>, (const Float2&, float), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Step(const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Step<float>, (const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 SmoothStep(const Float2 &in, float, float)", asFUNCTIONPR(Math::SmoothStep<float>, (const Float2&, float, float), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 SmoothStep(const Float2 &in, const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::SmoothStep<float>, (const Float2&, const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Step(const Float3 &in, float)", asFUNCTIONPR(Math::Step<float>, (const Float3&, float), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Step(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Step<float>, (const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 SmoothStep(const Float3 &in, float, float)", asFUNCTIONPR(Math::SmoothStep<float>, (const Float3&, float, float), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 SmoothStep(const Float3 &in, const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::SmoothStep<float>, (const Float3&, const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Step(const Float4 &in, float)", asFUNCTIONPR(Math::Step<float>, (const Float4&, float), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Step(const Float4 &in, const Float4 &in)", asFUNCTIONPR(Math::Step<float>, (const Float4&, const Float4&), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 SmoothStep(const Float4 &in, float, float)", asFUNCTIONPR(Math::SmoothStep<float>, (const Float4&, float, float), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 SmoothStep(const Float4 &in, const Float4 &in, const Float4 &in)", asFUNCTIONPR(Math::SmoothStep<float>, (const Float4&, const Float4&, const Float4&), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Min(const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Min, (const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Min(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Min, (const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Min(const Float4 &in, const Float4 &in)", asFUNCTIONPR(Math::Min, (const Float4&, const Float4&), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Max(const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Max, (const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Max(const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Max, (const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Max(const Float4 &in, const Float4 &in)", asFUNCTIONPR(Math::Max, (const Float4&, const Float4&), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Clamp(const Float2 &in, const Float2 &in, const Float2 &in)", asFUNCTIONPR(Math::Clamp, (const Float2&, const Float2&, const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Clamp(const Float3 &in, const Float3 &in, const Float3 &in)", asFUNCTIONPR(Math::Clamp, (const Float3&, const Float3&, const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Clamp(const Float4 &in, const Float4 &in, const Float4 &in)", asFUNCTIONPR(Math::Clamp, (const Float4&, const Float4&, const Float4&), Float4), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float2 Saturate(const Float2 &in)", asFUNCTIONPR(Math::Saturate, (const Float2&), Float2), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 Saturate(const Float3 &in)", asFUNCTIONPR(Math::Saturate, (const Float3&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float4 Saturate(const Float4 &in)", asFUNCTIONPR(Math::Saturate, (const Float4&), Float4), asCALL_CDECL);
    assert(r >= 0);

    r = pEngine->RegisterGlobalFunction("bool BvSphereOverlapSphere( const BvSphere & in, const BvSphere & in )", asFUNCTIONPR(BvSphereOverlapSphere, (BvSphere const&, BvSphere const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvSphereOverlapPoint( const BvSphere & in, const Float3 & in )", asFUNCTIONPR(BvSphereOverlapPoint, (BvSphere const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvSphereOverlapTriangle( const BvSphere & in, const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvSphereOverlapTriangle, (BvSphere const&, Float3 const&, Float3 const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvSphereOverlapPlane( const BvSphere & in, const Plane & in )", asFUNCTIONPR(BvSphereOverlapPlane, (BvSphere const&, PlaneF const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("int BvSphereOverlapPlaneSideMask( const BvSphere & in, const Plane & in )", asFUNCTIONPR(BvSphereOverlapPlaneSideMask, (BvSphere const&, PlaneF const&), int), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapBox( const BvAxisAlignedBox & in, const BvAxisAlignedBox & in )", asFUNCTIONPR(BvBoxOverlapBox, (BvAxisAlignedBox const&, BvAxisAlignedBox const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapBox2D( const Float2 & in, const Float2 & in, const Float2 & in, const Float2 & in )", asFUNCTIONPR(BvBoxOverlapBox2D, (Float2 const&, Float2 const&, Float2 const&, Float2 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapPoint( const BvAxisAlignedBox & in, const Float3 & in )", asFUNCTIONPR(BvBoxOverlapPoint, (BvAxisAlignedBox const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapSphere( const BvAxisAlignedBox & in, const BvSphere & in )", asFUNCTIONPR(BvBoxOverlapSphere, (BvAxisAlignedBox const&, BvSphere const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapTriangle( const BvAxisAlignedBox & in, const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvBoxOverlapTriangle, (BvAxisAlignedBox const&, Float3 const&, Float3 const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapTriangle_FastApproximation( const BvAxisAlignedBox & in, const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvBoxOverlapTriangle_FastApproximation, (BvAxisAlignedBox const&, Float3 const&, Float3 const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvGetBoxIntersection( const BvAxisAlignedBox & in, const BvAxisAlignedBox & in, BvAxisAlignedBox & out )", asFUNCTIONPR(BvGetBoxIntersection, (BvAxisAlignedBox const&, BvAxisAlignedBox const&, BvAxisAlignedBox&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("int BvBoxOverlapPlaneSideMask( const Float3 & in, const Float3 & in, const Plane & in )", asFUNCTIONPR(BvBoxOverlapPlaneSideMask, (Float3 const&, Float3 const&, PlaneF const&), int), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapPlane( const Float3 & in, const Float3 & in, const Plane & in )", asFUNCTIONPR(BvBoxOverlapPlane, (Float3 const&, Float3 const&, PlaneF const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapPlane( const BvAxisAlignedBox & in, const Plane & in )", asFUNCTIONPR(BvBoxOverlapPlane, (BvAxisAlignedBox const&, PlaneF const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvBoxOverlapPlaneFast( const BvAxisAlignedBox & in, const Plane & in, int, int )", asFUNCTIONPR(BvBoxOverlapPlaneFast, (BvAxisAlignedBox const&, PlaneF const&, int, int), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("int BvBoxOverlapPlaneSideMask( const BvAxisAlignedBox & in, const Plane & in, int, int )", asFUNCTIONPR(BvBoxOverlapPlaneSideMask, (BvAxisAlignedBox const&, PlaneF const&, int, int), int), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapOrientedBox( const BvOrientedBox & in, const BvOrientedBox & in )", asFUNCTIONPR(BvOrientedBoxOverlapOrientedBox, (BvOrientedBox const&, BvOrientedBox const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapSphere( const BvOrientedBox & in, const BvSphere & in )", asFUNCTIONPR(BvOrientedBoxOverlapSphere, (BvOrientedBox const&, BvSphere const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapBox( const BvOrientedBox & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvOrientedBoxOverlapBox, (BvOrientedBox const&, Float3 const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapBox( const BvOrientedBox & in, const BvAxisAlignedBox & in )", asFUNCTIONPR(BvOrientedBoxOverlapBox, (BvOrientedBox const&, BvAxisAlignedBox const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapTriangle( const BvOrientedBox & in, const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvOrientedBoxOverlapTriangle, (BvOrientedBox const&, Float3 const&, Float3 const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapTriangle_FastApproximation( const BvOrientedBox & in, const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvOrientedBoxOverlapTriangle_FastApproximation, (BvOrientedBox const&, Float3 const&, Float3 const&, Float3 const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvOrientedBoxOverlapPlane( const BvOrientedBox & in, const Plane & in )", asFUNCTIONPR(BvOrientedBoxOverlapPlane, (BvOrientedBox const&, PlaneF const&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectSphere( const Float3 & in, const Float3 & in, const BvSphere & in, float & out, float & out )", asFUNCTIONPR(BvRayIntersectSphere, (Float3 const&, Float3 const&, BvSphere const&, float&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectSphere( const Float3 & in, const Float3 & in, const BvSphere & in, float & out )", asFUNCTIONPR(BvRayIntersectSphere, (Float3 const&, Float3 const&, BvSphere const&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectBox( const Float3 & in, const Float3 & in, const BvAxisAlignedBox & in, float & out, float & out )", asFUNCTIONPR(BvRayIntersectBox, (Float3 const&, Float3 const&, BvAxisAlignedBox const&, float&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectBox2D( const Float2 & in, const Float2 & in, const Float2 & in, const Float2 & in, float & out, float & out )", asFUNCTIONPR(BvRayIntersectBox2D, (Float2 const&, Float2 const&, Float2 const&, Float2 const&, float&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectOrientedBox( const Float3 & in, const Float3 & in, const BvOrientedBox & in, float & out, float & out )", asFUNCTIONPR(BvRayIntersectOrientedBox, (Float3 const&, Float3 const&, BvOrientedBox const&, float&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectTriangle( const Float3 & in, const Float3 & in, const Float3 & in, const Float3 & in, const Float3 & in, float & out, float & out, float & out, bool=true )", asFUNCTIONPR(BvRayIntersectTriangle, (Float3 const&, Float3 const&, Float3 const&, Float3 const&, Float3 const&, float&, float&, float&, bool), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectPlane( const Float3 & in, const Float3 & in, const Plane & in, float & out )", asFUNCTIONPR(BvRayIntersectPlane, (Float3 const&, Float3 const&, PlaneF const&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectPlaneFront( const Float3 & in, const Float3 & in, const Plane & in, float & out )", asFUNCTIONPR(BvRayIntersectPlaneFront, (Float3 const&, Float3 const&, PlaneF const&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectPlaneBack( const Float3 & in, const Float3 & in, const Plane & in, float & out )", asFUNCTIONPR(BvRayIntersectPlaneBack, (Float3 const&, Float3 const&, PlaneF const&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectElipsoid( const Float3 & in, const Float3 & in, float _Radius, float, float, float & out, float & out )", asFUNCTIONPR(BvRayIntersectElipsoid, (Float3 const&, Float3 const&, float, float, float, float&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvRayIntersectElipsoid( const Float3 & in, const Float3 & in, float _Radius, float, float, float & out )", asFUNCTIONPR(BvRayIntersectElipsoid, (Float3 const&, Float3 const&, float, float, float, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float BvShortestDistanceSqr( const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvShortestDistanceSqr, (Float3 const&, Float3 const&, Float3 const&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvDistanceSqr( const Float3 & in, const Float3 & in, const Float3 & in, float & out )", asFUNCTIONPR(BvDistanceSqr, (Float3 const&, Float3 const&, Float3 const&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvIsPointOnSegment( const Float3 & in, const Float3 & in, const Float3 & in, float )", asFUNCTIONPR(BvIsPointOnSegment, (Float3 const&, Float3 const&, Float3 const&, float), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("Float3 BvProjectPointOnLine( const Float3 & in, const Float3 & in, const Float3 & in )", asFUNCTIONPR(BvProjectPointOnLine, (Float3 const&, Float3 const&, Float3 const&), Float3), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("float BvShortestDistanceSqr( const Float2 & in, const Float2 & in, const Float2 & in )", asFUNCTIONPR(BvShortestDistanceSqr, (Float2 const&, Float2 const&, Float2 const&), float), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvDistanceSqr( const Float2 & in, const Float2 & in, const Float2 & in, float & out )", asFUNCTIONPR(BvDistanceSqr, (Float2 const&, Float2 const&, Float2 const&, float&), bool), asCALL_CDECL);
    assert(r >= 0);
    r = pEngine->RegisterGlobalFunction("bool BvIsPointOnSegment( const Float2 & in, const Float2 & in, const Float2 & in, float )", asFUNCTIONPR(BvIsPointOnSegment, (Float2 const&, Float2 const&, Float2 const&, float), bool), asCALL_CDECL);
    assert(r >= 0);

    HK_UNUSED(r);



// TODO
#if 0
    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector2<T> operator+(T _Left, TVector2<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector2<T> operator-(T _Left, TVector2<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector3<T> operator+(T _Left, TVector3<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector3<T> operator-(T _Left, TVector3<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector4<T> operator+(T _Left, TVector4<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector4<T> operator-(T _Left, TVector4<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector2<T> operator*(T _Left, TVector2<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector3<T> operator*(T _Left, TVector3<T> const& _Right);

    template <typename T, typename = TStdEnableIf<Math::IsReal<T>()>>
    constexpr TVector4<T> operator*(T _Left, TVector4<T> const& _Right);

    bool BvPointInPoly2D( Float2 const * _Points, int _NumPoints, float _PX, float _PY );
    bool BvPointInPoly2D( Float2 const * _Points, int _NumPoints, Float2 const & _Point );    
    bool BvBoxOverlapPlane( Float3 const * _BoxVertices, PlaneF const & in );
    bool BvBoxOverlapConvex( BvAxisAlignedBox const & in, PlaneF const * _Planes, int _PlaneCount );
    bool BvBoxInsideConvex( BvAxisAlignedBox const & in, PlaneF const * _Planes, int _PlaneCount );
    bool BvOrientedBoxOverlapConvex( BvOrientedBox const & b, PlaneF const * _Planes, int _PlaneCount );
    bool BvOrientedBoxInsideConvex( BvOrientedBox const & b, PlaneF const * _Planes, int _PlaneCount );
    bool BvPointInConvexHullCCW( Float3 const & InPoint, Float3 const & InNormal, Float3 const * InPoints, int InNumPoints );
    bool BvPointInConvexHullCW( Float3 const & InPoint, Float3 const & InNormal, Float3 const * InPoints, int InNumPoints );
#endif
}


AScriptEngine::AScriptEngine(AWorld* pWorld) :
    pEngine(asCreateScriptEngine()),
    ContextPool(pEngine)
{
    bHasCompileErrors = false;
    int r;


    r = pEngine->SetMessageCallback(asMETHOD(AScriptEngine, MessageCallback), this, asCALL_THISCALL);
    assert(r >= 0);

    // Register the string type
    RegisterStdString(pEngine);

    // Register the generic handle type, called 'ref' in the script
    RegisterScriptHandle(pEngine);

    // Register the weak ref template type
    RegisterScriptWeakRef(pEngine);

    RegisterMath(pEngine);

    // Register the game object. The scripts cannot create these directly, so there is no factory function.
    r = pEngine->RegisterObjectType("AActor", 0, asOBJ_REF);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("AActor", asBEHAVE_ADDREF, "void f()", asMETHOD(AActor, AddRef), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("AActor", asBEHAVE_RELEASE, "void f()", asMETHOD(AActor, RemoveRef), asCALL_THISCALL);
    assert(r >= 0);
    r = pEngine->RegisterObjectBehaviour("AActor", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(AActor, ScriptGetWeakRefFlag), asCALL_THISCALL);
    assert(r >= 0);

    r = pEngine->RegisterObjectType("SActorDamage", sizeof(SActorDamage), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Float3>());
    assert(r >= 0);

    //r = pEngine->RegisterObjectProperty("AActor", "bool bTickEvenWhenPaused", offsetof(AActor, bTickEvenWhenPaused));
    //assert(r >= 0);

    r = pEngine->RegisterObjectMethod("AActor", "void Destroy()", asMETHOD(AActor, Destroy), asCALL_THISCALL);
    assert(r >= 0);

    r = pEngine->RegisterObjectMethod("AActor", "bool get_bPendingKill() const property", asMETHOD(AActor, IsPendingKill), asCALL_THISCALL);
    assert(r >= 0);

    r = pEngine->RegisterObjectMethod("AActor", "void ApplyDamage(const SActorDamage& in)", asMETHOD(AActor, ApplyDamage), asCALL_THISCALL);
    assert(r >= 0);


    r = pEngine->RegisterObjectProperty("SActorDamage", "float Amount", offsetof(SActorDamage, Amount));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("SActorDamage", "Float3 Position", offsetof(SActorDamage, Position));
    assert(r >= 0);
    r = pEngine->RegisterObjectProperty("SActorDamage", "float Radius", offsetof(SActorDamage, Radius));
    assert(r >= 0);
    //r = pEngine->RegisterObjectProperty("SActorDamage", "AActor@+ DamageCauser", offsetof(SActorDamage, DamageCauser));
    //assert(r >= 0);




    r = pEngine->RegisterInterface("IActorScript");
    assert(r >= 0);

    r = pEngine->RegisterGlobalFunction("void PrintMessage(const string &in msg)", asFUNCTION(PrintMessage), asCALL_CDECL);
    assert(r >= 0);

    r = pEngine->RegisterObjectType("AWorld", 0, asOBJ_REF | asOBJ_NOHANDLE);
    assert(r >= 0);

    r = pEngine->RegisterGlobalProperty("AWorld world", pWorld);
    assert(r >= 0);

    HK_UNUSED(r);
}

AScriptEngine::~AScriptEngine()
{
    Scripts.Clear();

    if (pEngine)
        pEngine->ShutDownAndRelease();
}

void AScriptEngine::MessageCallback(asSMessageInfo const& msg)
{
    const char* type = "ERR ";

    switch (msg.type)
    {
        case asMSGTYPE_ERROR:
            type = "Error";
            break;
        case asMSGTYPE_WARNING:
            type = "Warning";
            break;
        case asMSGTYPE_INFORMATION:
            type = "Info";
            break;
    }

    LOG("{} ({}, {}) : {} : {}\n", msg.section, msg.row, msg.col, type, msg.message);

    if (msg.type == asMSGTYPE_ERROR)
        bHasCompileErrors = true;
}

static int LoadScript(const char* SourceFileName, const char* IncludedFrom, CScriptBuilder* pBuilder)
{
    AFileStream f;
    if (!f.OpenRead(SourceFileName))
        return -1;

    return pBuilder->AddSectionFromMemory(SourceFileName, f.AsString().CStr());
}

AActorScript* AScriptEngine::GetActorScript(AString const& ModuleName)
{
    int r;

    for (auto& pScript : Scripts) // TODO: hash
    {
        if (pScript->Module == ModuleName)
            return pScript.get();
    }

    asIScriptModule* mod = pEngine->GetModule(ModuleName.CStr(), asGM_ONLY_IF_EXISTS);
    if (mod)
    {
        // We've already attempted loading the script before, but there is no actor class
        return nullptr;
    }

    // Compile the script into the module
    CScriptBuilder builder;
    r = builder.StartNewModule(pEngine, ModuleName.CStr());
    if (r < 0)
        return nullptr;

    builder.SetIncludeCallback([](const char* SourceFileName, const char* IncludedFrom, CScriptBuilder* pBuilder, void* pUserParam)
                               { return LoadScript(SourceFileName, IncludedFrom, pBuilder); },
                               nullptr);

    r = LoadScript((ModuleName + ".as").CStr(), "", &builder);
    if (r < 0)
        return nullptr;

    r = builder.BuildModule();
    if (r < 0)
        return nullptr;

    // Cache the functions and methods that will be used
    std::unique_ptr<AActorScript> pScript = std::make_unique<AActorScript>();

    pScript->Module = ModuleName;

    // Find the class that implements the IActorScript interface
    mod               = pEngine->GetModule(ModuleName.CStr(), asGM_ONLY_IF_EXISTS);
    asITypeInfo* type = 0;
    int          tc   = mod->GetObjectTypeCount();
    for (int n = 0; n < tc; n++)
    {
        bool found = false;
        type       = mod->GetObjectTypeByIndex(n);
        int ic     = type->GetInterfaceCount();
        for (int i = 0; i < ic; i++)
        {
            if (strcmp(type->GetInterface(i)->GetName(), "IActorScript") == 0)
            {
                found = true;
                break;
            }
        }

        if (found == true)
        {
            pScript->Type = type;
            break;
        }
    }

    if (pScript->Type == 0)
    {
        LOG("Couldn't find the actor class for the type '{}'\n", ModuleName);
        return nullptr;
    }

    AString s = AString(type->GetName()) + "@ " + type->GetName() + "(AActor @)";

    pScript->M_FactoryFunc = type->GetFactoryByDecl(s.CStr());
    if (pScript->M_FactoryFunc == 0)
    {
        LOG("Couldn't find the appropriate factory for the type '{}'\n", ModuleName);
        return nullptr;
    }

    pScript->M_BeginPlay       = type->GetMethodByDecl("void BeginPlay()");
    pScript->M_Tick            = type->GetMethodByDecl("void Tick(float TimeStep)");
    pScript->M_TickPrePhysics  = type->GetMethodByDecl("void TickPrePhysics(float TimeStep)");
    pScript->M_TickPostPhysics = type->GetMethodByDecl("void TickPostPhysics(float TimeStep)");
    pScript->M_LateUpdate      = type->GetMethodByDecl("void LateUpdate(float TimeStep)");
    pScript->M_OnApplyDamage   = type->GetMethodByDecl("void OnApplyDamage(const SActorDamage& in Damage)");

    pScript->pEngine = this;

    type->SetUserData(pScript.get());

    Scripts.Add(std::move(pScript));

    return Scripts.Last().get();
}

asIScriptObject* AScriptEngine::CreateScriptInstance(AString const& ModuleName, AActor* pActor)
{
    asIScriptObject* pInstance{};

    AActorScript* pScript = GetActorScript(ModuleName);
    if (!pScript)
        return nullptr;

    SScopedContext ctx(this, pScript->M_FactoryFunc);
    ctx->SetArgObject(0, pActor);

    if (ctx.ExecuteCall() == asEXECUTION_FINISHED)
    {
        pInstance = *((asIScriptObject**)ctx->GetAddressOfReturnValue());
        pInstance->AddRef();
    }
    return pInstance;
}

AScriptContextPool::AScriptContextPool(asIScriptEngine* pEngine) :
    pEngine(pEngine)
{
}

AScriptContextPool::~AScriptContextPool()
{
    for (asIScriptContext* pContext : Contexts)
        pContext->Release();
}

asIScriptContext* AScriptContextPool::PrepareContext(asIScriptFunction* pFunction)
{
    asIScriptContext* pContext;
    if (!Contexts.IsEmpty())
    {
        pContext = Contexts.Last();
        Contexts.RemoveLast();
    }
    else
        pContext = pEngine->CreateContext();

    int r = pContext->Prepare(pFunction);
    HK_ASSERT(r >= 0);

    if (!r)
    {
        LOG("AScriptContextPool::PrepareContext: failed to prepare context '{}'\n", pFunction->GetName());
    }

    return pContext;
}

asIScriptContext* AScriptContextPool::PrepareContext(asIScriptObject* pScriptObject, asIScriptFunction* pFunction)
{
    asIScriptContext* pContext = PrepareContext(pFunction);
    pContext->SetObject(pScriptObject);
    return pContext;
}

void AScriptContextPool::UnprepareContext(asIScriptContext* pContext)
{
    pContext->Unprepare();
    Contexts.Add(pContext);
}

AActorScript* AActorScript::GetScript(asIScriptObject* pObject)
{
    return reinterpret_cast<AActorScript*>(pObject->GetObjectType()->GetUserData());
}

void AActorScript::SetProperties(asIScriptObject* pObject, TStringHashMap<AString> const& Properties)
{
    // TODO
}

bool AActorScript::SetProperty(asIScriptObject* pObject, AStringView PropertyName, AStringView PropertyValue)
{
    // TODO
    return false;
}

void AActorScript::CloneProperties(asIScriptObject* Template, asIScriptObject* Destination)
{
    // TODO
}

void AActorScript::BeginPlay(asIScriptObject* pObject)
{
    if (M_BeginPlay)
    {
        SScopedContext ctx(pEngine, pObject, M_BeginPlay);
        ctx.ExecuteCall();
    }
}

void AActorScript::Tick(asIScriptObject* pObject, float TimeStep)
{
    if (M_Tick)
    {
        SScopedContext ctx(pEngine, pObject, M_Tick);
        ctx->SetArgFloat(0, TimeStep);
        ctx.ExecuteCall();
    }
}

void AActorScript::TickPrePhysics(asIScriptObject* pObject, float TimeStep)
{
    if (M_TickPrePhysics)
    {
        SScopedContext ctx(pEngine, pObject, M_TickPrePhysics);
        ctx->SetArgFloat(0, TimeStep);
        ctx.ExecuteCall();
    }
}

void AActorScript::TickPostPhysics(asIScriptObject* pObject, float TimeStep)
{
    if (M_TickPostPhysics)
    {
        SScopedContext ctx(pEngine, pObject, M_TickPostPhysics);
        ctx->SetArgFloat(0, TimeStep);
        ctx.ExecuteCall();
    }
}

void AActorScript::LateUpdate(asIScriptObject* pObject, float TimeStep)
{
    if (M_LateUpdate)
    {
        SScopedContext ctx(pEngine, pObject, M_LateUpdate);
        ctx->SetArgFloat(0, TimeStep);
        ctx.ExecuteCall();
    }
}

void AActorScript::OnApplyDamage(asIScriptObject* pObject, SActorDamage const& Damage)
{
    if (M_OnApplyDamage)
    {
        SScopedContext ctx(pEngine, pObject, M_OnApplyDamage);
        ctx->SetArgObject(0, (void*)&Damage);
        ctx.ExecuteCall();
    }
}

void AActorScript::DrawDebug(asIScriptObject* pObject, ADebugRenderer* InRenderer)
{
    // TODO
}
