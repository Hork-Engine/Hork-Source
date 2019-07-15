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

#include "Level.h"
#include "CollisionEvents.h"
#include <Engine/Core/Public/Guid.h>

class FWorld;
class FPawn;
class FSceneComponent;
class FCameraComponent;
class FActorComponent;
class FTimer;

using FArrayOfActorComponents = TPodArray< FActorComponent *, 8 >;

#define AN_ACTOR( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( FActor::Factory(), _Class, _SuperClass ) \
protected: \
    ~_Class() {} \
private:


/*

FActor

Base class for all actors

*/
class ANGIE_API FActor : public FBaseObject {
    AN_ACTOR( FActor, FBaseObject )

    friend class FWorld;
    friend class FActorComponent;
public:

    // Actor events
    FContactDelegate E_OnBeginContact;
    FContactDelegate E_OnEndContact;
    FContactDelegate E_OnUpdateContact;
    FOverlapDelegate E_OnBeginOverlap;
    FOverlapDelegate E_OnEndOverlap;
    FOverlapDelegate E_OnUpdateOverlap;

    // Root component keeps component hierarchy and transform for the actor
    FSceneComponent * RootComponent;

    float LifeSpan;

    bool bCanEverTick;

    bool bTickEvenWhenPaused;

    bool bTickPrePhysics;

    bool bTickPostPhysics;

    // Actors factory
    static FObjectFactory & Factory() { static FObjectFactory ObjectFactory( "Actor factory" ); return ObjectFactory; }

    // Get actor GUID
    FGUID const & GetGUID() const { return GUID; }

    // Set actor unique name
    void SetName( FString const & _Name ) override;

    // Get actor's world
    FWorld * GetWorld() const { return ParentWorld; }

    // Get actor's level
    FLevel * GetLevel() const { return Level; }

    // Create component by it's class id
    FActorComponent * CreateComponent( uint64_t _ClassId, const char * _Name );

    // Create component by it's class name
    FActorComponent * CreateComponent( const char * _ClassName, const char * _Name );

    // Create component by it's class meta (fastest way to create component)
    FActorComponent * CreateComponent( FClassMeta const * _ClassMeta, const char * _Name );

    // Load component from document data
    FActorComponent * LoadComponent( FDocument const & _Document, int _FieldsHead );

    // Get component by it's class id
    FActorComponent * GetComponent( uint64_t _ClassId );

    // Get component by it's class name
    FActorComponent * GetComponent( const char * _ClassName );

    // Get component by it's class meta
    FActorComponent * GetComponent( FClassMeta const * _ClassMeta );

    // Get component by it's unique name
    FActorComponent * FindComponent( const char * _UniqueName );

    // Get component by it's unique GUID
    FActorComponent * FindComponentGUID( FGUID const & _GUID );

    // Create component of specified type
    template< typename ComponentType >
    ComponentType * CreateComponent( const char * _Name ) {
        return static_cast< ComponentType * >( CreateComponent( &ComponentType::ClassMeta(), _Name ) );
    }

    // Get component of specified type
    template< typename ComponentType >
    ComponentType * GetComponent() {
        return static_cast< ComponentType * >( GetComponent( &ComponentType::ClassMeta() ) );
    }

    // Get all actor components
    FArrayOfActorComponents const & GetComponents() const { return Components; }

    // Serialize actor to document data
    int Serialize( FDocument & _Doc ) override;

    // Destroy this actor
    void Destroy();

    // Is actor marked as pending kill
    bool IsPendingKill() const { return bPendingKill; }

    // Actor's instigator
    FPawn * GetInstigator() { return Instigator; }

    // Apply damage to actor
    virtual void ApplyDamage( float _DamageAmount, Float3 const & _Position, FActor * _DamageCauser );

protected:

    FActor();

    // Register actor's timer. Call this in BeginPlay or after.
    void RegisterTimer( FTimer * _Timer );

protected:

    // Called before components initialized
    virtual void PreInitializeComponents() {}

    // Called after components initialized
    virtual void PostInitializeComponents() {}

    // Called when actor enters the game
    virtual void BeginPlay() {}

    // Called only from Destroy() method
    virtual void EndPlay();

    // Notify all actors in the world when actor spawned
    virtual void OnActorSpawned( FActor * _SpawnedActor ) {}

    // Tick based on variable time step. Dependend on current frame rate.
    // One tick per frame. It is good place to update things like animation.
    virtual void Tick( float _TimeStep ) {}

    // Tick based on fixed time step. Use it to update logic and physics.
    // There may be one or several ticks per frame. Called before physics simulation.
    virtual void TickPrePhysics( float _TimeStep ) {}

    // Tick based on fixed time step. Use it to update logic based on physics simulation.
    // There may be one or several ticks per frame. Called after physics simulation.
    virtual void TickPostPhysics( float _TimeStep ) {}

    // Draw debug primitives
    virtual void DrawDebug( FDebugDraw * _DebugDraw );

private:

    void PostSpawnInitialize( FTransform const & _SpawnTransform );

    void PostActorConstruction();

    void InitializeComponents();

    // Called only from SpawnActor
    void BeginPlayComponents();

    void TickComponents( float _TimeStep );

    void DestroyComponents();

    void Clone( FActor const * _TemplateActor );

    void AddComponent( FActorComponent * _Component );

    FString GenerateComponentUniqueName( const char * _Name );

    FGUID GUID;

    // All actor components
    FArrayOfActorComponents Components;

    // Index in world array of actors
    int IndexInWorldArrayOfActors = -1;

    // Index in level array of actors
    int IndexInLevelArrayOfActors = -1;

    FWorld * ParentWorld;
    TRef< FLevel > Level;
    //TRef< FActor > Attach; // TODO: Attach actor to another actor
    FPawn * Instigator;
    bool bPendingKill;
    bool bDuringConstruction = true;
    FActor * NextPendingKillActor;

    float LifeTime;

    FTimer * Timers;
};

class ANGIE_API FViewActor : public FActor {
    AN_ACTOR( FViewActor, FActor )

    friend class FPlayerController;

protected:
    FViewActor() {}

    virtual void OnView( FCameraComponent * _Camera ) {}
};
