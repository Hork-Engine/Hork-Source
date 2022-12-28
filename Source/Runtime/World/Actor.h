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

#include <Runtime/ActorDefinition.h>
#include <Runtime/CollisionEvents.h>
#include <Runtime/Level.h>

#include "CameraComponent.h"

class asILockableSharedBool;
class asIScriptObject;

HK_NAMESPACE_BEGIN

class World;
class ActorComponent;
class SceneComponent;
class InputComponent;
class Actor_Controller;
class DebugRenderer;

using ActorComponents = TSmallVector<ActorComponent*, 8>;

#define HK_ACTOR(_Class, _SuperClass) \
    HK_FACTORY_CLASS(Actor::Factory(), _Class, _SuperClass)

struct ActorInitializer
{
    bool bCanEverTick{};
    bool bTickEvenWhenPaused{};
    bool bTickPrePhysics{};
    bool bTickPostPhysics{};
    bool bLateUpdate{};
};

struct ActorDamage
{
    float Amount;
    Float3 Position;
    float Radius;
    Actor* DamageCauser;
};

constexpr float LIFESPAN_ALIVE = 0;
constexpr float LIFESPAN_DEAD = -1;


/**

Actor

Base class for all actors

*/
class Actor : public BaseObject
{
    HK_ACTOR(Actor, BaseObject)

    friend class World;
    friend class Actor_Controller;
    friend class PhysicsSystem;

public:
    /** Actor factory */
    static ObjectFactory& Factory()
    {
        static ObjectFactory ObjectFactory("Actor factory");
        return ObjectFactory;
    }

public:
    /** You can control the lifespan of an actor by setting the LifeSpan property.
    Note that ticking must be enabled (bCanEverTick set to true). */
    float LifeSpan{LIFESPAN_ALIVE};

    Actor();

    /** Get actor's world */
    World* GetWorld() const { return m_World; }

    /** Get actor's level */
    Level* GetLevel() const { return m_Level; }

    /** The root component is used to place an actor in the world.
    It is also used to set the actor's location during spawning. */
    void SetRootComponent(SceneComponent* rootComponent);

    /** The root component is used to place an actor in the world.
    It is also used to set the actor's location during spawning. */
    SceneComponent* GetRootComponent() const { return m_RootComponent; }

    void ResetRootComponent() { m_RootComponent = nullptr; }

    /** The pawn camera is used to setup rendering. */
    CameraComponent* GetPawnCamera() { return m_PawnCamera; }

    /** Actor's instigator */
    Actor* GetInstigator() { return m_Instigator; }

    Actor_Controller* GetController() { return m_Controller; }

    /** Create component by it's class id */
    ActorComponent* CreateComponent(uint64_t _ClassId, StringView Name);

    /** Create component by it's class name */
    ActorComponent* CreateComponent(const char* _ClassName, StringView Name);

    /** Create component by it's class meta (fastest way to create component) */
    ActorComponent* CreateComponent(ClassMeta const* _ClassMeta, StringView Name);

    /** Get component by it's class id */
    ActorComponent* GetComponent(uint64_t _ClassId);

    /** Get component by it's class name */
    ActorComponent* GetComponent(const char* _ClassName);

    /** Get component by it's class meta */
    ActorComponent* GetComponent(ClassMeta const* _ClassMeta);

    /** Create component of specified type */
    template <typename ComponentType>
    ComponentType* CreateComponent(StringView Name)
    {
        return static_cast<ComponentType*>(CreateComponent(&ComponentType::GetClassMeta(), Name));
    }

    /** Get component of specified type */
    template <typename ComponentType>
    ComponentType* GetComponent()
    {
        return static_cast<ComponentType*>(GetComponent(&ComponentType::GetClassMeta()));
    }

    /** Get all actor components */
    ActorComponents const& GetComponents() const { return m_Components; }

    /** Destroy self */
    void Destroy();

    /** Is actor marked as pending kill */
    bool IsPendingKill() const { return m_bPendingKill; }

    /** Apply damage to the actor */
    void ApplyDamage(ActorDamage const& Damage);

    /** Override this function to bind axes and actions to the input component */
    virtual void SetupInputComponent(InputComponent* Input) {}

    /** Is used to register console commands. Experimental. */
    virtual void SetupRuntimeCommands() {}

    bool IsSpawning() const { return m_bSpawning; }

    bool IsInEditor() const { return m_bInEditor; }

    /** Set property value by it's public name. See actor definition. */
    bool SetPublicProperty(StringView PublicName, StringView Value);

    asILockableSharedBool* ScriptGetWeakRefFlag();

    /** Set object debug/editor or ingame name */
    void SetObjectName(StringView Name) { m_Name = Name; }

    /** Get object debug/editor or ingame name */
    String const& GetObjectName() const { return m_Name; }

protected:
    // Actor events
    ContactDelegate E_OnBeginContact;
    ContactDelegate E_OnEndContact;
    ContactDelegate E_OnUpdateContact;
    OverlapDelegate E_OnBeginOverlap;
    OverlapDelegate E_OnEndOverlap;
    OverlapDelegate E_OnUpdateOverlap;

    /** The root component is used to place an actor in the world.
    It is also used to set the actor's location during spawning. */
    SceneComponent* m_RootComponent{};

    /** The pawn camera is used to setup rendering. */
    TWeakRef<CameraComponent> m_PawnCamera;

    /** Called after constructor. Note that the actor is not yet in the world.
    The actor appears in the world only after spawn and just before calling BeginPlay().
    Spawning occurs at the beginning of the next frame.
    NOTE: Here you can subscribe to actor events. For example, to react to a collision,
    you need to subscribe to "Begin Contact" event like this:
        E_OnBeginContact.Add(this, &MyActorController::HandleBeginContact)
    In the script, you just need to declare the OnBeginContact method and similarly with other events.
    */
    virtual void Initialize(ActorInitializer& Initializer) {}

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

    virtual void OnApplyDamage(ActorDamage const& Damage) {}

    /** Draw debug primitives */
    virtual void DrawDebug(DebugRenderer* InRenderer) {}

    /** Called before components initialized */
    virtual void PreInitializeComponents() {}

    /** Called after components initialized */
    virtual void PostInitializeComponents() {}

    /** Called during level loading */
    virtual void SetLevelGeometry(LevelGeometry const& geometry) {}

    class WorldTimer* AddTimer(TCallback<void()> const& Callback);
    void RemoveTimer(WorldTimer* Timer);
    void RemoveAllTimers();

private:
    void AddComponent(ActorComponent* Component, StringView Name);
    void CallBeginPlay();
    void CallTick(float TimeStep);
    void CallTickPrePhysics(float TimeStep);
    void CallTickPostPhysics(float TimeStep);
    void CallLateUpdate(float TimeStep);
    void CallDrawDebug(DebugRenderer* InRenderer);

private:
    World* m_World{};
    TWeakRef<Level> m_Level;
    ActorComponents m_Components;
    TRef<ActorDefinition> m_pActorDef;
    Actor* m_Instigator{};
    Actor_Controller* m_Controller{};
    asIScriptObject* m_ScriptModule{};
    asILockableSharedBool* m_pWeakRefFlag{};
    String m_Name;

    int m_ComponentLocalIdGen{};

    /** Index in world array of actors */
    int m_IndexInWorldArrayOfActors{-1};

    /** Index in level array of actors */
    int m_IndexInLevelArrayOfActors{-1};

    Actor* m_NextSpawnActor{};
    Actor* m_NextPendingKillActor{};

    WorldTimer* m_TimerList{};
    WorldTimer* m_TimerListTail{};

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

HK_NAMESPACE_END
