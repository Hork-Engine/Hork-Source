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

#include "BaseObject.h"
#include "Level.h"

class FActor;
class FActorComponent;
class FDrawSurf;
class FMeshComponent;
class FSkinnedComponent;
class FTimer;
class FDebugDraw;

struct FActorSpawnParameters {
    FActorSpawnParameters() = delete;

    FActorSpawnParameters( FClassMeta const * _ActorTypeClassMeta )
        : Level( nullptr )
        , Template( nullptr )
        , ActorTypeClassMeta( _ActorTypeClassMeta )
    {
    }

    // FIXME: This might be useful from scripts
#if 0
    FActorSpawnParameters( uint64_t _ActorClassId )
        : FActorSpawnParameters( FActor::Factory().LookupClass( _ActorClassId ) )
    {
    }

    FActorSpawnParameters( const char * _ActorClassName )
        : FActorSpawnParameters( FActor::Factory().LookupClass( _ActorClassName ) )
    {
    }
#endif

    void SetTemplate( FActor const * _Template );

    FClassMeta const * ActorClassMeta() const { return ActorTypeClassMeta; }
    FActor const * GetTemplate() const { return Template; }

    FTransform SpawnTransform;

    FLevel * Level;

protected:
    FActor const * Template;    // template type meta must match ActorTypeClassMeta
    FClassMeta const * ActorTypeClassMeta;
};

template< typename ActorType >
struct TActorSpawnParameters : FActorSpawnParameters {
    TActorSpawnParameters()
        : FActorSpawnParameters( &ActorType::ClassMeta() )
    {
    }
};


#define AN_WORLD( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( FWorld::Factory(), _Class, _SuperClass ) \
protected: \
    ~_Class() {} \
private:


/*

FWorld

Defines a game map or editor/tool scene

*/
class ANGIE_API FWorld : public FBaseObject {
    AN_WORLD( FWorld, FBaseObject )

    friend class FActor;
    friend class FActorComponent;
    friend class FGameMaster;
    friend class FRenderFrontend;

public:
    // Worlds factory
    static FObjectFactory & Factory() { static FObjectFactory ObjectFactory( "World factory" ); return ObjectFactory; }

    // Spawn a new actor
    FActor * SpawnActor( FActorSpawnParameters const & _SpawnParameters );

    // Spawn a new actor
    template< typename ActorType >
    ActorType * SpawnActor( TActorSpawnParameters< ActorType > const & _SpawnParameters ) {
        FActorSpawnParameters const & spawnParameters = _SpawnParameters;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    // Spawn a new actor
    template< typename ActorType >
    ActorType * SpawnActor( FLevel * _Level = nullptr ) {
        TActorSpawnParameters< ActorType > spawnParameters;
        spawnParameters.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    // Spawn a new actor
    template< typename ActorType >
    ActorType * SpawnActor( FTransform const & _SpawnTransform, FLevel * _Level = nullptr ) {
        TActorSpawnParameters< ActorType > spawnParameters;
        spawnParameters.SpawnTransform = _SpawnTransform;
        spawnParameters.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    // Spawn a new actor
    template< typename ActorType >
    ActorType * SpawnActor( Float3 const & _Position, Quat const & _Rotation, FLevel * _Level = nullptr ) {
        TActorSpawnParameters< ActorType > spawnParameters;
        spawnParameters.SpawnTransform.Position = _Position;
        spawnParameters.SpawnTransform.Rotation = _Rotation;
        spawnParameters.Level = _Level;
        return static_cast< ActorType * >( SpawnActor( spawnParameters ) );
    }

    // Load actor from document data
    FActor * LoadActor( FDocument const & _Document, int _FieldsHead, FLevel * _Level = nullptr );

    FActor * FindActor( const char * _UniqueName );

    TPodArray< FActor * > const & GetActors() const { return Actors; }

    // Serialize world to document data
    int Serialize( FDocument & _Doc );

    // Destroy this world
    void Destroy();

    // Destroy all actors in world
    void DestroyActors();

    // Add level to world
    void AddLevel( FLevel * _Level );

    // Remove level from world
    void RemoveLevel( FLevel * _Level );

    FLevel * GetPersistentLevel() { return PersistentLevel; }

    TPodArray< FLevel * > const & GetArrayOfLevels() const { return ArrayOfLevels; }

    FString GenerateActorUniqueName( const char * _Name );

    float GetWorldLocalTime() const { return WorldLocalTime; }
    float GetWorldPlayTime() const { return WorldPlayTime; }

    void RegisterMesh( FMeshComponent * _Mesh );
    void UnregisterMesh( FMeshComponent * _Mesh );

    void RegisterSkinnedMesh( FSkinnedComponent * _Skeleton );
    void UnregisterSkinnedMesh( FSkinnedComponent * _Skeleton );

    FMeshComponent * GetMeshList() { return MeshList; }

    //bool bTickEvenWhenPaused;

protected:
    // [FGameMaster friend, overridable]
    virtual void BeginPlay();

    // [overridable]
    // Called only from Destroy() method
    virtual void EndPlay();

    virtual void Tick( float _TimeStep );

    FWorld();

private:
    void BroadcastActorSpawned( FActor * _SpawnedActor );

    void RegisterTimer( FTimer * _Timer );      // friend FActor
    void UnregisterTimer( FTimer * _Timer );    // friend FActor

    void KickoffPendingKillObjects();

    void DrawDebug( FDebugDraw * _DebugDraw );
    int GetFirstDebugDrawCommand() const { return FirstDebugDrawCommand; }
    int GetDebugDrawCommandCount() const { return DebugDrawCommandCount; }

    TPodArray< FActor * > Actors;

    FMeshComponent * MeshList; // FRenderFrontend friend
    FMeshComponent * MeshListTail;
    FSkinnedComponent * SkinnedMeshList;
    FSkinnedComponent * SkinnedMeshListTail;

    float WorldLocalTime;
    float WorldPlayTime;

    FTimer * TimerList;
    FTimer * TimerListTail;

    int VisFrame;
    int FirstDebugDrawCommand;
    int DebugDrawCommandCount;
    int DebugDrawFrame;

    int IndexInGameArrayOfWorlds = -1; // friend FGameMaster

    bool bPendingKill;

    FActor * PendingKillActors; // friend FActor
    FActorComponent * PendingKillComponents; // friend FActorComponent

    FWorld * NextPendingKillWorld;

    TRefHolder< FLevel > PersistentLevel;
    TPodArray< FLevel * > ArrayOfLevels;
};


#include "Actor.h"
#include "ActorComponent.h"

/*

TActorIterator

Iterate world actors

Example:
for ( TActorIterator< FMyActor > it( GetWorld() ) ; it ; ++it ) {
    FMyActor * actor = *it;
    ...
}

*/
template< typename T >
struct TActorIterator {
    explicit TActorIterator( FWorld * _World )
        : Actors( _World->GetActors() ), i(0)
    {
        Next();
    }

    operator bool() const {
        return Actor != nullptr;
    }

    T * operator++() {
        Next();
        return Actor;
    }

    T * operator++(int) {
        T * a = Actor;
        Next();
        return a;
    }

    T * operator*() const {
        return Actor;
    }

    T * operator->() const {
        return Actor;
    }

    void Next() {
        FActor * a;
        while ( i < Actors.Length() ) {
            a = Actors[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                Actor = static_cast< T * >( a );
                return;
            }
        }
        Actor = nullptr;
    }

private:
    TPodArray< FActor * > const & Actors;
    T * Actor;
    int i;
};

/*

TActorIterator2

Iterate world actors

Example:

TActorIterator2< FMyActor > it( this );
for ( FMyActor * actor = it.First() ; actor ; actor = it.Next() ) {
    ...
}

*/
template< typename T >
struct TActorIterator2 {
    explicit TActorIterator2( FWorld * _World )
        : Actors( _World->GetActors() ), i( 0 )
    {
    }

    T * First() {
        i = 0;
        return Next();
    }

    T * Next() {
        FActor * a;
        while ( i < Actors.Length() ) {
            a = Actors[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                return static_cast< T * >( a );
            }
        }
        return nullptr;
    }

private:
    TPodArray< FActor * > const & Actors;
    int i;
};

/*

TComponentIterator

Iterate actor components

Example:
for ( TComponentIterator< FMyComponent > it( actor ) ; it ; ++it ) {
    FMyComponent * component = *it;
    ...
}

*/
template< typename T >
struct TComponentIterator {
    explicit TComponentIterator( FActor * _Actor )
        : Components( _Actor->GetComponents() ), i(0)
    {
        Next();
    }

    operator bool() const {
        return Component != nullptr;
    }

    T * operator++() {
        Next();
        return Component;
    }

    T * operator++(int) {
        T * a = Component;
        Next();
        return a;
    }

    T * operator*() const {
        return Component;
    }

    T * operator->() const {
        return Component;
    }

    void Next() {
        FActorComponent * a;
        while ( i < Components.Length() ) {
            a = Components[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                Component = static_cast< T * >( a );
                return;
            }
        }
        Component = nullptr;
    }

private:
    TPodArray< FActorComponent * > const & Components;
    T * Component;
    int i;
};

/*

TComponentIterator2

Iterate actor components

Example:

TComponentIterator2< FMyComponent > it( this );
for ( FMyComponent * component = it.First() ; component ; component = it.Next() ) {
    ...
}

*/
template< typename T >
struct TComponentIterator2 {
    explicit TComponentIterator2( FActor * _Actor )
        : Components( _Actor->GetComponents() ), i( 0 )
    {
    }

    T * First() {
        i = 0;
        return Next();
    }

    T * Next() {
        FActorComponent * a;
        while ( i < Components.Length() ) {
            a = Components[i++];
            if ( a->IsPendingKill() ) {
                continue;
            }
            if ( &a->FinalClassMeta() == &T::ClassMeta() ) {
                return static_cast< T * >( a );
            }
        }
        return nullptr;
    }

private:
    TPodArray< FActorComponent * > const & Components;
    int i;
};
