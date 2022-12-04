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

#include "ActorDefinition.h"
#include "Level.h"
#include "CollisionEvents.h"
#include "CameraComponent.h"

class AWorld;
class AActorComponent;
class ASceneComponent;
class AInputComponent;
class AController;

using ActorComponents = TSmallVector<AActorComponent*, 8>;

#define HK_ACTOR(_Class, _SuperClass) \
    HK_FACTORY_CLASS(AActor::Factory(), _Class, _SuperClass)

struct SActorInitializer
{
    bool bCanEverTick{};
    bool bTickEvenWhenPaused{};
    bool bTickPrePhysics{};
    bool bTickPostPhysics{};
    bool bLateUpdate{};
};

struct SActorDamage
{
    float   Amount;
    Float3  Position;
    float   Radius;
    AActor* DamageCauser;
};

constexpr float LIFESPAN_ALIVE = 0;
constexpr float LIFESPAN_DEAD  = -1;


/**

AActor

Base class for all actors

*/
class AActor : public ABaseObject
{
    HK_ACTOR(AActor, ABaseObject)

    friend class AWorld;
    friend class AController;
    friend class APhysicsSystem;

public:
    /** Actor factory */
    static AObjectFactory& Factory()
    {
        static AObjectFactory ObjectFactory("Actor factory");
        return ObjectFactory;
    }

public:
    /** You can control the lifespan of an actor by setting the LifeSpan property.
    Note that ticking must be enabled (bCanEverTick set to true). */
    float LifeSpan{LIFESPAN_ALIVE};

    AActor();

    /** Get actor's world */
    AWorld* GetWorld() const { return m_World; }

    /** Get actor's level */
    ALevel* GetLevel() const { return m_Level; }

    /** The root component is used to place an actor in the world.
    It is also used to set the actor's location during spawning. */
    ASceneComponent* GetRootComponent() const { return m_RootComponent; }

    void ResetRootComponent() { m_RootComponent = nullptr; }

    /** The pawn camera is used to setup rendering. */
    ACameraComponent* GetPawnCamera() { return m_PawnCamera; }

    /** Actor's instigator */
    AActor* GetInstigator() { return m_Instigator; }

    AController* GetController() { return m_Controller; }

    /** Create component by it's class id */
    AActorComponent* CreateComponent(uint64_t _ClassId, AStringView Name);

    /** Create component by it's class name */
    AActorComponent* CreateComponent(const char* _ClassName, AStringView Name);

    /** Create component by it's class meta (fastest way to create component) */
    AActorComponent* CreateComponent(AClassMeta const* _ClassMeta, AStringView Name);

    /** Get component by it's class id */
    AActorComponent* GetComponent(uint64_t _ClassId);

    /** Get component by it's class name */
    AActorComponent* GetComponent(const char* _ClassName);

    /** Get component by it's class meta */
    AActorComponent* GetComponent(AClassMeta const* _ClassMeta);

    /** Create component of specified type */
    template <typename ComponentType>
    ComponentType* CreateComponent(AStringView Name)
    {
        return static_cast<ComponentType*>(CreateComponent(&ComponentType::ClassMeta(), Name));
    }

    /** Get component of specified type */
    template <typename ComponentType>
    ComponentType* GetComponent()
    {
        return static_cast<ComponentType*>(GetComponent(&ComponentType::ClassMeta()));
    }

    /** Get all actor components */
    ActorComponents const& GetComponents() const { return m_Components; }

    /** Destroy self */
    void Destroy();

    /** Is actor marked as pending kill */
    bool IsPendingKill() const { return m_bPendingKill; }

    /** Apply damage to the actor */
    void ApplyDamage(SActorDamage const& Damage);

    /** Override this function to bind axes and actions to the input component */
    virtual void SetupInputComponent(AInputComponent* Input) {}

    /** Is used to register console commands. Experimental. */
    virtual void SetupRuntimeCommands() {}

    bool IsSpawning() const { return m_bSpawning; }

    bool IsInEditor() const { return m_bInEditor; }

    /** Set property value by it's public name. See actor definition. */
    bool SetPublicProperty(AStringView PublicName, AStringView Value);

    class asILockableSharedBool* ScriptGetWeakRefFlag();

    /** Set object debug/editor or ingame name */
    void SetObjectName(AStringView Name) { m_Name = Name; }

    /** Get object debug/editor or ingame name */
    AString const& GetObjectName() const { return m_Name; }

protected:
    // Actor events
    AContactDelegate E_OnBeginContact;
    AContactDelegate E_OnEndContact;
    AContactDelegate E_OnUpdateContact;
    AOverlapDelegate E_OnBeginOverlap;
    AOverlapDelegate E_OnEndOverlap;
    AOverlapDelegate E_OnUpdateOverlap;

    /** The root component is used to place an actor in the world.
    It is also used to set the actor's location during spawning. */
    ASceneComponent* m_RootComponent{};

    /** The pawn camera is used to setup rendering. */
    TWeakRef<ACameraComponent> m_PawnCamera;

    /** Called after constructor. Note that the actor is not yet in the world.
    The actor appears in the world only after spawn and just before calling BeginPlay().
    Spawning occurs at the beginning of the next frame.
    NOTE: Here you can subscribe to actor events. For example, to react to a collision,
    you need to subscribe to "Begin Contact" event like this:
        E_OnBeginContact.Add(this, &MyActorController::HandleBeginContact)
    In the script, you just need to declare the OnBeginContact method and similarly with other events.
    */
    virtual void Initialize(SActorInitializer& Initializer) {}

    /** Called when the actor enters the game */
    virtual void BeginPlay() {}

    /** Tick based on variable time step. Depends on the current frame rate.
    One tick per frame. This is a good place to update things like animation. */
    virtual void Tick(float TimeStep) {}

    /** Tick based on fixed time step. Use it to update logic and physics.
    There can be zero or more ticks per frame. Called before physics simulation. */
    virtual void TickPrePhysics(float TimeStep) {}

    /** Tick based on fixed time step. Use it to update logic based on physics simulation.
    There can be zero or more ticks per frame. Called after physics simulation. */
    virtual void TickPostPhysics(float TimeStep) {}

    /** Tick based on variable time step. Depends on the current frame rate.
    One tick per frame. Called at the end of a frame. */
    virtual void LateUpdate(float TimeStep) {}

    virtual void OnInputLost() {}

    virtual void OnApplyDamage(SActorDamage const& Damage) {}

    /** Draw debug primitives */
    virtual void DrawDebug(ADebugRenderer* InRenderer) {}

    /** Called before components initialized */
    virtual void PreInitializeComponents() {}

    /** Called after components initialized */
    virtual void PostInitializeComponents() {}

    class ATimer* AddTimer(TCallback<void()> const& Callback);
    void          RemoveTimer(ATimer* Timer);
    void          RemoveAllTimers();

private:
    void AddComponent(AActorComponent* Component, AStringView Name);
    void CallBeginPlay();
    void CallTick(float TimeStep);
    void CallTickPrePhysics(float TimeStep);
    void CallTickPostPhysics(float TimeStep);
    void CallLateUpdate(float TimeStep);
    void CallDrawDebug(ADebugRenderer* InRenderer);

private:
    AWorld*                      m_World{};
    TWeakRef<ALevel>             m_Level;
    ActorComponents              m_Components;
    TRef<AActorDefinition>       m_pActorDef;
    AActor*                      m_Instigator{};
    AController*                 m_Controller{};
    class asIScriptObject*       m_ScriptModule{};
    class asILockableSharedBool* m_pWeakRefFlag{};
    AString                      m_Name;

    int m_ComponentLocalIdGen{};

    /** Index in world array of actors */
    int m_IndexInWorldArrayOfActors{-1};

    /** Index in level array of actors */
    int m_IndexInLevelArrayOfActors{-1};

    AActor* m_NextSpawnActor{};
    AActor* m_NextPendingKillActor{};

    ATimer* m_TimerList{};
    ATimer* m_TimerListTail{};

    float m_LifeTime{0.0f};

    bool m_bCanEverTick{};
    bool m_bTickEvenWhenPaused{};
    bool m_bTickPrePhysics{};
    bool m_bTickPostPhysics{};
    bool m_bLateUpdate{};
    bool m_bSpawning{true};
    bool m_bPendingKill{};
    bool m_bInEditor{};
};
