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

#pragma once

#include <Core/String.h>
#include <Containers/Vector.h>
#include <Containers/Hash.h>

class asIScriptEngine;
class asIScriptContext;
class asIScriptObject;
class asIScriptFunction;
class asITypeInfo;
struct asSMessageInfo;

HK_NAMESPACE_BEGIN

class Actor;
class World;

class ScriptContextPool
{
public:
    ScriptContextPool(asIScriptEngine* pEngine);
    virtual ~ScriptContextPool();

    asIScriptContext* PrepareContext(asIScriptFunction* pFunction);
    asIScriptContext* PrepareContext(asIScriptObject* pScriptObject, asIScriptFunction* pFunction);
    void              UnprepareContext(asIScriptContext* pContext);

private:
    asIScriptEngine*              m_pEngine;
    TPodVector<asIScriptContext*> m_Contexts;
};

class ActorScript
{
public:
    ActorScript() {}

    static ActorScript* GetScript(asIScriptObject* pObject);

    static void SetProperties(asIScriptObject* pObject, TStringHashMap<String> const& Properties);
    static bool SetProperty(asIScriptObject* pObject, StringView PropertyName, StringView PropertyValue);    
    static void CloneProperties(asIScriptObject* Template, asIScriptObject* Destination);

    String const& GetModule() const { return Module; }

    void BeginPlay(asIScriptObject* pObject);
    void Tick(asIScriptObject* pObject, float TimeStep);
    void TickPrePhysics(asIScriptObject* pObject, float TimeStep);
    void TickPostPhysics(asIScriptObject* pObject, float TimeStep);
    void LateUpdate(asIScriptObject* pObject, float TimeStep);
    void OnApplyDamage(asIScriptObject* pObject, struct ActorDamage const& Damage);
    void DrawDebug(asIScriptObject* pObject, class DebugRenderer* InRenderer);

//private:
    String              Module;
    asITypeInfo*         Type{};
    asIScriptFunction*   M_FactoryFunc{};
    asIScriptFunction*   M_BeginPlay{};
    asIScriptFunction*   M_Tick{};
    asIScriptFunction*   M_TickPrePhysics{};
    asIScriptFunction*   M_TickPostPhysics{};
    asIScriptFunction*   M_LateUpdate{};
    asIScriptFunction*   M_OnApplyDamage{};
    class ScriptEngine* pEngine{};

    friend class ScriptEngine;
};

class ScriptEngine
{
public:
    ScriptEngine(World* pWorld);
    ~ScriptEngine();

    asIScriptObject* CreateScriptInstance(String const& ModuleName, Actor* pActor);

    ScriptContextPool& GetContextPool() { return m_ContextPool; }

    bool bHasCompileErrors;

protected:
    void MessageCallback(asSMessageInfo const& msg);

    ActorScript* GetActorScript(String const& ModuleName);

    asIScriptEngine* m_pEngine;

    ScriptContextPool                     m_ContextPool;
    TVector<std::unique_ptr<ActorScript>> m_Scripts;
};

HK_NAMESPACE_END
