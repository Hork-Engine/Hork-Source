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
    asIScriptFunction*   M_BeginPlay;
    asIScriptFunction*   M_EndPlay;
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

    AScriptContextPool                     ContextPool;
    TStdVector<std::unique_ptr<AActorScript>> Scripts;
};
