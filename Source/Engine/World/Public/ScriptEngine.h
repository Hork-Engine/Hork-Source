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

#include <Core/Public/String.h>
#include <Containers/Public/PodVector.h>
#include <Containers/Public/StdVector.h>

class AActor;
class AWorld;

class asIScriptEngine;
class asIScriptContext;
class asIScriptObject;
class asIScriptFunction;
class asITypeInfo;
struct asSMessageInfo;

class AScriptContextPool
{
public:
    AScriptContextPool(asIScriptEngine* pEngine);
    virtual ~AScriptContextPool();

    asIScriptContext* PrepareContext(asIScriptFunction* pFunction);
    asIScriptContext* PrepareContext(asIScriptObject* pScriptObject, asIScriptFunction* pFunction);
    void              UnprepareContext(asIScriptContext* pContext);

private:
    asIScriptEngine* pEngine;
    TPodVector<asIScriptContext*> Contexts;
};

class AActorScript
{
public:
    AActorScript() {}

    static AActorScript* GetScript(asIScriptObject* pScriptInstance);

    AString const& GetModule() const { return Module; }

    void BeginPlay(asIScriptObject* pScriptInstance);
    void EndPlay(asIScriptObject* pScriptInstance);
    void Tick(asIScriptObject* pScriptInstance, float TimeStep);
    void TickPrePhysics(asIScriptObject* pScriptInstance, float TimeStep);
    void TickPostPhysics(asIScriptObject* pScriptInstance, float TimeStep);
    void ApplyDamage(asIScriptObject* pScriptInstance, struct SActorDamage const& Damage);

private:
    AString              Module;
    asITypeInfo*         Type{};
    asIScriptFunction*   M_FactoryFunc{};
    asIScriptFunction*   M_BeginPlay{};
    asIScriptFunction*   M_EndPlay{};
    asIScriptFunction*   M_Tick{};
    asIScriptFunction*   M_TickPrePhysics{};
    asIScriptFunction*   M_TickPostPhysics{};
    asIScriptFunction*   M_ApplyDamage{};
    class AScriptEngine* pEngine{};

    friend class AScriptEngine;
};

class AScriptEngine
{
public:
    AScriptEngine(AWorld* pWorld);
    ~AScriptEngine();

    asIScriptObject* CreateScriptInstance(AString const& ModuleName, AActor* pActor);

    AScriptContextPool& GetContextPool() { return ContextPool; }

    bool bHasCompileErrors;

protected:
    void MessageCallback(asSMessageInfo const& msg);

    AActorScript* GetActorScript(AString const& ModuleName);

    asIScriptEngine* pEngine;

    AScriptContextPool                        ContextPool;
    TStdVector<std::unique_ptr<AActorScript>> Scripts;
};
