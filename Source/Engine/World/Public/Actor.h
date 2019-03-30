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
#include <Engine/Core/Public/Guid.h>

class FWorld;
class FSceneComponent;
class FTimer;
class FCameraComponent;

#define AN_ACTOR( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( FActor::Factory(), _Class, _SuperClass ) \
protected: \
    ~_Class() {} \
private:

class FActorComponent;

/*

FActor

Base class for all actors

*/
class ANGIE_API FActor : public FBaseObject {
    AN_ACTOR( FActor, FBaseObject )

    friend class FWorld;
    friend class FActorComponent;
public:
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
    TPodArray< FActorComponent * > const & GetComponents() const { return Components; }

    // Serialize actor to document data
    int Serialize( FDocument & _Doc ) override;

    // Destroy this actor
    void Destroy();

    // Is actor marked as pending kill
    bool IsPendingKill() const { return bPendingKill; }

    void RegisterTimer( FTimer * _Timer );

#if 0
    void AddTickPrerequisiteActor( FActor * _Actor );
    void AddTickPrerequisiteComponent( FActorComponent * _Component );
    void SetActorTickEnabled( bool _Enabled );
    void RegisterActorTickFunction( FTickFunction & _TickFunction );
    PrimaryTick PrimaryActorTick;
#endif

    // Root component, keeps component hierarchy and transform for the actor
    FSceneComponent * RootComponent;

    // Point when actor life ends
    float LifeSpan;

    bool bCanEverTick;
    bool bTickEvenWhenPaused;

protected:

    // [World friend]
    FActor();

    // [World friend, overridable]
    //virtual void PostActorCreated() {}

    // [local, overridable]
    //virtual void OnContruction( FTransform const & _Transform ) {}

    // [local, overridable]
    virtual void PreInitializeComponents() {}

    // [local, overridable]
    virtual void PostInitializeComponents() {}

    // [World friend, overridable]
    virtual void BeginPlay() {}

    // [World friend, overridable]
    // Called only from Destroy() method
    virtual void EndPlay() {}

    // [World friend, overridable]
    virtual void OnActorSpawned( FActor * _SpawnedActor ) {}

    // [World friend, overridable]
    virtual void Tick( float _TimeStep ) {}

private:

    //void Destroy() override final;

    // [World friend]
    void PostSpawnInitialize( FTransform const & _SpawnTransform );

    // [World friend]
    //bool ExecuteContruction( FTransform const & _Transform );

    // [World friend]
    void PostActorConstruction();

    // [local]
    void InitializeComponents();

    // [World friend]
    // Called only from SpawnActor
    void BeginPlayComponents();

    // [World friend]
    void TickComponents( float _TimeStep );

    void DestroyComponents();

    void Clone( FActor const * _TemplateActor );

    void AddComponent( FActorComponent * _Component );

    FString GenerateComponentUniqueName( const char * _Name );

    FGUID GUID;

    // All actor components
    // [ActorComponent friend]
    TPodArray< FActorComponent * > Components;

    // Index in world array of actors
    int IndexInWorldArrayOfActors = -1;

    // Index in level array of actors
    int IndexInLevelArrayOfActors = -1;

    FWorld * ParentWorld;
    TRefHolder< FLevel > Level;
    bool bPendingKill;
    bool bDuringConstruction = true;
    FActor * NextPendingKillActor;

    float LifeTime;

    FTimer * Timers;
};

#if 0
enum ETickGroup {
    TG_PrePhysics,
    TG_DuringPhysics,
    TG_PostPhysics,
    TG_PostUpdateWork
};

struct FPrimaryTick {
    bool bCanEverTick;
    bool bTickEvenWhenPaused;
    ETickGroup TickGroup;
};

class FActor;
class FActorComponent;

struct FTickFunction {
    bool bTickEvenWhenPaused;
    ETickGroup TickGroup;
    void AddTickPrerequisite( FActor * _Actor );
    void AddTickPrerequisite( FActorComponent * _ActorComponent );
    TCallback< void( float ) > ExecuteTick;
};

class FActorRenderState {
public:

    bool bDirty;
};

class FActorPhysicsState {
public:
};
#endif

class ANGIE_API FViewActor : public FActor {
    AN_ACTOR( FViewActor, FActor )

    friend class FPlayerController;

protected:
    FViewActor() {}

    virtual void OnView( FCameraComponent * _Camera ) {}
};
