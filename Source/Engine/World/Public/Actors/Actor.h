/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/Level.h>
#include <World/Public/CollisionEvents.h>
#include <Core/Public/Guid.h>

class AWorld;
class APawn;
class AActorComponent;
class ASceneComponent;

using AArrayOfActorComponents = TPodArray< AActorComponent *, 8 >;

#define AN_ACTOR( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( AActor::Factory(), _Class, _SuperClass ) \
protected: \
    ~_Class() {} \
private:


struct SActorDamage
{
    float Amount;
    Float3 Position;
    float Radius;
    AActor * DamageCauser;
};


/**

AActor

Base class for all actors

*/
class AActor : public ABaseObject {
    AN_ACTOR( AActor, ABaseObject )

    friend class AWorld;

public:

    // Actor events
    AContactDelegate E_OnBeginContact;
    AContactDelegate E_OnEndContact;
    AContactDelegate E_OnUpdateContact;
    AOverlapDelegate E_OnBeginOverlap;
    AOverlapDelegate E_OnEndOverlap;
    AOverlapDelegate E_OnUpdateOverlap;

    /** Root component keeps component hierarchy and transform for the actor */
    ASceneComponent * RootComponent = nullptr;

    float LifeSpan = 0.0f;

    bool bTickEvenWhenPaused = false;

    bool bTickPrePhysics = false;

    bool bTickPostPhysics = false;

    /** Actors factory */
    static AObjectFactory & Factory() { static AObjectFactory ObjectFactory( "Actor factory" ); return ObjectFactory; }

    // Get actor GUID
    //AGUID const & GetGUID() const { return GUID; }

    /** Get actor's world */
    AWorld * GetWorld() const { return ParentWorld; }

    /** Get actor's level */
    ALevel * GetLevel() const { return Level; }

    /** Create component by it's class id */
    AActorComponent * CreateComponent( uint64_t _ClassId, const char * _Name );

    /** Create component by it's class name */
    AActorComponent * CreateComponent( const char * _ClassName, const char * _Name );

    /** Create component by it's class meta (fastest way to create component) */
    AActorComponent * CreateComponent( AClassMeta const * _ClassMeta, const char * _Name );

    /** Get component by it's class id */
    AActorComponent * GetComponent( uint64_t _ClassId );

    /** Get component by it's class name */
    AActorComponent * GetComponent( const char * _ClassName );

    /** Get component by it's class meta */
    AActorComponent * GetComponent( AClassMeta const * _ClassMeta );

    /** Create component of specified type */
    template< typename ComponentType >
    ComponentType * CreateComponent( const char * _Name ) {
        return static_cast< ComponentType * >( CreateComponent( &ComponentType::ClassMeta(), _Name ) );
    }

    /** Get component of specified type */
    template< typename ComponentType >
    ComponentType * GetComponent() {
        return static_cast< ComponentType * >( GetComponent( &ComponentType::ClassMeta() ) );
    }

    /** Get all actor components */
    AArrayOfActorComponents const & GetComponents() const { return Components; }

    /** Serialize actor to document data */
    TRef< ADocObject > Serialize() override;

    /** Destroy self */
    void Destroy();

    /** Is actor marked as pending kill */
    bool IsPendingKill() const { return bPendingKill; }

    /** Actor's instigator */
    APawn * GetInstigator() { return Instigator; }

    /** Apply damage to the actor */
    virtual void ApplyDamage( SActorDamage const & Damage );

    bool IsDuringConstruction() const { return bDuringConstruction; }

    /** Actor spawned for editing */
    bool IsInEditor() const { return bInEditor; }

protected:

    bool bCanEverTick = false;

    AActor();

protected:

    /** Called before components initialized */
    virtual void PreInitializeComponents() {}

    /** Called after components initialized */
    virtual void PostInitializeComponents() {}

    /** Called when actor enters the game */
    virtual void BeginPlay();

    /** Called only from Destroy() method */
    virtual void EndPlay();

    /** Tick based on variable time step. Dependend on current frame rate.
    One tick per frame. It is good place to update things like animation. */
    virtual void Tick( float _TimeStep ) {}

    /** Tick based on fixed time step. Use it to update logic and physics.
    There may be one or several ticks per frame. Called before physics simulation. */
    virtual void TickPrePhysics( float _TimeStep ) {}

    /** Tick based on fixed time step. Use it to update logic based on physics simulation.
    There may be one or several ticks per frame. Called after physics simulation. */
    virtual void TickPostPhysics( float _TimeStep ) {}

    /** Draw debug primitives */
    virtual void DrawDebug( ADebugRenderer * InRenderer );

private:

    void Initialize( STransform const & _SpawnTransform );

    void InitializeComponents();

    void BeginPlayComponents();

    void TickComponents( float _TimeStep );

    void DestroyComponents();

    void AddComponent( AActorComponent * _Component );

    //AGUID GUID;

    /** All actor components */
    AArrayOfActorComponents Components;

    /** Index in world array of actors */
    int IndexInWorldArrayOfActors = -1;

    /** Index in level array of actors */
    int IndexInLevelArrayOfActors = -1;

    AActor * NextPendingKillActor = nullptr;

    AWorld * ParentWorld = nullptr;

    //TRef< ALevel > Level;
    TWeakRef< ALevel > Level;

    //TWeakRef< AActor > Attach; // TODO: Attach actor to another actor

    APawn * Instigator = nullptr;

    float LifeTime = 0.0f;

    bool bPendingKill = false;
    bool bDuringConstruction = true;
    bool bInEditor = false;
};
