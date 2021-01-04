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

#include <World/Public/World.h>
#include <World/Public/Actors/Pawn.h>
#include <World/Public/Timer.h>
#include <World/Public/Base/GameModuleInterface.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/PointLightComponent.h>
#include <World/Private/Render/VSD.h>
#include <GameThread/Public/EngineInstance.h>
#include <Runtime/Public/Runtime.h>
#include <Core/Public/Logger.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include "PrimitiveLinkPool.h"

AN_CLASS_META( AWorld )

AWorld * AWorld::PendingKillWorlds = nullptr;
TPodArray< AWorld * > AWorld::Worlds;

AWorld::AWorld()
    : PhysicsWorld( this )
    , RenderWorld( this )
    , NavigationMesh( this )
{
    PersistentLevel = NewObject< ALevel >();
    PersistentLevel->AddRef();
    PersistentLevel->OwnerWorld = this;
    PersistentLevel->bIsPersistent = true;
    PersistentLevel->IndexInArrayOfLevels = ArrayOfLevels.Size();
    ArrayOfLevels.Append( PersistentLevel );
    PersistentLevel->OnAddLevelToWorld();
}

AWorld::~AWorld()
{
}

void AWorld::SetPaused( bool _Paused )
{
    bPauseRequest = _Paused;
    bUnpauseRequest = !_Paused;
}

bool AWorld::IsPaused() const
{
    return bPaused;
}

void AWorld::ResetGameplayTimer()
{
    bResetGameplayTimer = true;
}

void AWorld::SetPhysicsHertz( int _Hertz )
{
    PhysicsWorld.PhysicsHertz = _Hertz;
}

void AWorld::SetContactSolverSplitImpulse( bool _SplitImpulse )
{
    PhysicsWorld.bContactSolverSplitImpulse = _SplitImpulse;
}

void AWorld::SetContactSolverIterations( int _InterationsCount )
{
    PhysicsWorld.NumContactSolverIterations = _InterationsCount;
}

void AWorld::SetGravityVector( Float3 const & _Gravity )
{
    PhysicsWorld.GravityVector = _Gravity;
    PhysicsWorld.bGravityDirty = true;
}

Float3 const & AWorld::GetGravityVector() const
{
    return PhysicsWorld.GravityVector;
}

void AWorld::BeginPlay()
{
}

void AWorld::EndPlay()
{
}

void AWorld::Destroy()
{
    if ( bPendingKill ) {
        return;
    }

    // Mark world to remove it from the game
    bPendingKill = true;
    NextPendingKillWorld = PendingKillWorlds;
    PendingKillWorlds = this;

    DestroyActors();
    KickoffPendingKillObjects();

    // Remove all levels from world including persistent level
    for ( ALevel * level : ArrayOfLevels ) {
        //if ( !level->bIsPersistent ) {
            level->OnRemoveLevelFromWorld();
        //}
        level->IndexInArrayOfLevels = -1;
        level->OwnerWorld = nullptr;
        level->RemoveRef();
    }
    ArrayOfLevels.Clear();

    EndPlay();
}

void AWorld::DestroyActors()
{
    for ( AActor * actor : Actors ) {
        actor->Destroy();
    }
}

void AWorld::BuildNavigation( SAINavigationConfig const & _NavigationConfig )
{
    NavigationMesh.Initialize( _NavigationConfig );
    NavigationMesh.Build();
}

SActorSpawnInfo::SActorSpawnInfo( uint64_t _ActorClassId )
    : SActorSpawnInfo( AActor::Factory().LookupClass( _ActorClassId ) )
{
}

SActorSpawnInfo::SActorSpawnInfo( const char * _ActorClassName )
    : SActorSpawnInfo( AActor::Factory().LookupClass( _ActorClassName ) )
{
}

void SActorSpawnInfo::SetTemplate( AActor const * _Template )
{
    AN_ASSERT( &_Template->FinalClassMeta() == ActorTypeClassMeta );
    Template = _Template;
}

void SActorSpawnInfo::_SetAttribute( AString const & AttributeName, AString const & AttributeValue )
{
    int hash = AttributeName.Hash();
    for ( int i = AttributeHash.First( hash ) ; i != -1 ; i = AttributeHash.Next( i ) ) {
        if ( Attributes[i].first == AttributeName ) {
            Attributes[i].second = AttributeValue;
            return;
        }
    }
    AttributeHash.Insert( hash, Attributes.Size() );
    Attributes.Append( std::make_pair( AttributeName, AttributeValue ) );
}

AActor * AWorld::SpawnActor( SActorSpawnInfo const & _SpawnParameters )
{
    AClassMeta const * classMeta = _SpawnParameters.ActorClassMeta();

    if ( !classMeta ) {
        GLogger.Printf( "AWorld::SpawnActor: invalid actor class\n" );
        return nullptr;
    }

    if ( classMeta->Factory() != &AActor::Factory() ) {
        GLogger.Printf( "AWorld::SpawnActor: not an actor class\n" );
        return nullptr;
    }

    AActor const * templateActor = _SpawnParameters.GetTemplate();

    if ( templateActor && classMeta != &templateActor->FinalClassMeta() ) {
        GLogger.Printf( "AWorld::SpawnActor: SActorSpawnInfo::Template class doesn't match meta data\n" );
        return nullptr;
    }

    AActor * actor = static_cast< AActor * >( classMeta->CreateInstance() );
    actor->AddRef();

    if ( _SpawnParameters.Instigator ) {
        actor->Instigator = _SpawnParameters.Instigator;
        actor->Instigator->AddRef();
    }

    // Add actor to world array of actors
    Actors.Append( actor );
    actor->IndexInWorldArrayOfActors = Actors.Size() - 1;
    actor->ParentWorld = this;

    actor->Level = _SpawnParameters.Level ? _SpawnParameters.Level : PersistentLevel;
    actor->Level->Actors.Append( actor );
    actor->IndexInLevelArrayOfActors = actor->Level->Actors.Size() - 1;

    actor->bInEditor = _SpawnParameters.bInEditor;

    if ( templateActor ) {

        // Clone attributes
        AClassMeta::CloneAttributes( templateActor, actor );

        // FIXME: How about to clone components?

    //    for ( AActorComponent const * templateComponent : templateActor->Components ) {
    //        if ( templateComponent->IsPendingKill() ) {
    //            continue;
    //        }

    //        AActorComponent * component;
    //        if ( templateComponent->bCreatedDuringConstruction ) {
    //            component = actor->FindComponentGUID( templateComponent->GetGUID() );
    //        } else {
    //            component = actor->AddComponent( &templateComponent->FinalClassMeta(), templateComponent->GetObjectName().CStr() );
    //        }

    //        if ( component ) {

    //            if ( templateComponent == templateActor->RootComponent ) {
    //                RootComponent = dynamic_cast< ASceneComponent * >( component );
    //            }

    //            AClassMeta::CloneAttributes( templateComponent, component );
    //        }
    //    }

        // TODO: Clone components hierarchy, etc

    } else {
        // TODO: Here create components added from the editor
        // TODO: Copy actor/component properties from the actor spawn parameters

        actor->SetAttributes( _SpawnParameters.GetAttributeHash(), _SpawnParameters.GetAttributes() );
        // FIXME: what about component attributes?
    }

    if ( actor->bInEditor ) {
        AArrayOfActorComponents tempArray = actor->Components;
        for ( AActorComponent * component : tempArray ) {
            component->OnCreateAvatar();
        }
    }

    actor->Initialize( _SpawnParameters.SpawnTransform );

    actor->bDuringConstruction = false;

    BroadcastActorSpawned( actor );

    return actor;
}

static Float3 ReadFloat3( ADocValue const * pObject, const char * _FieldName, Float3 const & _Default )
{
    ADocMember const * field = pObject->FindMember( _FieldName );
    if ( !field ) {
        return _Default;
    }

    AString s = field->GetString();

    Float3 r;
    sscanf( s.CStr(), "%f %f %f", &r.X, &r.Y, &r.Z );
    return r;
}

static Quat ReadQuat( ADocValue const * pObject, const char * _FieldName, Quat const & _Default )
{
    ADocMember const * field = pObject->FindMember( _FieldName );
    if ( !field ) {
        return _Default;
    }

    AString s = field->GetString();

    Quat r;
    sscanf( s.CStr(), "%f %f %f %f", &r.X, &r.Y, &r.Z, &r.W );
    return r;
}

AActor * AWorld::LoadActor( ADocValue const * pObject, ALevel * _Level, bool bInEditor )
{
    ADocMember const * classNameField = pObject->FindMember( "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "AWorld::LoadActor: invalid actor class\n" );
        return nullptr;
    }

    AClassMeta const * classMeta = AActor::Factory().LookupClass( classNameField->GetString().CStr() );
    if ( !classMeta ) {
        GLogger.Printf( "AWorld::LoadActor: invalid actor class \"%s\"\n", classNameField->GetString().CStr() );
        return nullptr;
    }

    AActor * actor = static_cast< AActor * >( classMeta->CreateInstance() );
    actor->AddRef();

    ADocMember const * objectNameField = pObject->FindMember( "ObjectName" );
    if ( objectNameField ) {
        actor->SetObjectName( objectNameField->GetString() );
    }

    // Add actor to world array of actors
    Actors.Append( actor );
    actor->IndexInWorldArrayOfActors = Actors.Size() - 1;
    actor->ParentWorld = this;

    actor->Level = _Level ? _Level : PersistentLevel;
    actor->Level->Actors.Append( actor );
    actor->IndexInLevelArrayOfActors = actor->Level->Actors.Size() - 1;

    actor->bInEditor = bInEditor;

    // Load actor attributes
    actor->LoadAttributes( pObject );

#if 0
    // Load components
    SDocumentField * componentsArray = pObject->FindField( "Components" );
    for ( int i = componentsArray->ValuesHead ; i != -1 ; i = pDocument->Values[ i ].Next ) {
        SDocumentValue const * componentObject = &pDocument->Values[ i ];
        if ( componentObject->Type != SDocumentValue::T_Object ) {
            continue;
        }
        actor->LoadComponent( _Document, componentObject->FieldsHead );
    }

    // Load root component
    SDocumentField * rootField = pObject->FindField( "Root" );
    if ( rootField ) {
        SDocumentValue * rootValue = &pDocument->Values[ rootField->ValuesHead ];
        ASceneComponent * root = dynamic_cast< ASceneComponent * >( actor->FindComponent( rootValue->Token.ToString().CStr() ) );
        if ( root ) {
            actor->RootComponent = root;
        }
    }
#endif

    if ( actor->bInEditor ) {
        AArrayOfActorComponents tempArray = actor->Components;
        for ( AActorComponent * component : tempArray ) {
            component->OnCreateAvatar();
        }
    }

    STransform spawnTransform;

    spawnTransform.Position = ReadFloat3( pObject, "SpawnPosition", Float3(0.0f) );
    spawnTransform.Rotation = ReadQuat( pObject, "SpawnRotation", Quat::Identity() );
    spawnTransform.Scale    = ReadFloat3( pObject, "SpawnScale", Float3(1.0f) );

    actor->Initialize( spawnTransform );

    actor->bDuringConstruction = false;

    BroadcastActorSpawned( actor );

    return actor;
}

//AString AWorld::GenerateActorUniqueName( const char * _Name )
//{
//    // TODO: optimize!
//    if ( !FindActor( _Name ) ) {
//        return _Name;
//    }
//    int uniqueNumber = 0;
//    AString uniqueName;
//    do {
//        uniqueName.Resize( 0 );
//        uniqueName.Concat( _Name );
//        uniqueName.Concat( Int( ++uniqueNumber ).CStr() );
//    } while ( FindActor( uniqueName.CStr() ) != nullptr );
//    return uniqueName;
//}

//AActor * AWorld::FindActor( const char * _UniqueName )
//{
//    // TODO: Use hash!
//    for ( AActor * actor : Actors ) {
//        if ( !actor->GetName().Icmp( _UniqueName ) ) {
//            return actor;
//        }
//    }
//    return nullptr;
//}

void AWorld::BroadcastActorSpawned( AActor * _SpawnedActor )
{
    E_OnActorSpawned.Dispatch( _SpawnedActor );
}

void AWorld::UpdatePauseStatus()
{
    if ( bPauseRequest ) {
        bPauseRequest = false;
        bPaused = true;
        GLogger.Printf( "Game paused\n" );
    } else if ( bUnpauseRequest ) {
        bUnpauseRequest = false;
        bPaused = false;
        GLogger.Printf( "Game unpaused\n" );
    }
}

void AWorld::UpdateTimers( float _TimeStep )
{
    bDuringTimerTick = true;
    int i=0;
    for ( ATimer * timer = TimerList ; timer ; timer = timer->Next ) {

        if ( timer->GetRefCount() > 1 ) {
            timer->Tick( _TimeStep );
            i++;
        } else {
            // Timer has no owner, unregister it
            timer->Unregister();
        }
    }
    //GLogger.Printf( "Timers in world %d\n", i );
    bDuringTimerTick = false;

    for ( STimerCmd & cmd : TimerCmd )
    {
        if ( cmd.Command == STimerCmd::ADD )
        {
            INTRUSIVE_ADD_UNIQUE( cmd.TimerCb, Next, Prev, TimerList, TimerListTail );
        }
        else
        {
            INTRUSIVE_REMOVE( cmd.TimerCb, Next, Prev, TimerList, TimerListTail );
        }
    }
    TimerCmd.Clear();
}

void AWorld::UpdateActors( float _TimeStep )
{
    for ( AActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( bPaused && !actor->bTickEvenWhenPaused ) {
            continue;
        }

        actor->TickComponents( _TimeStep );

        if ( actor->bCanEverTick ) {
            actor->Tick( _TimeStep );
        }
    }
}

void AWorld::UpdateActorsPrePhysics( float _TimeStep )
{
    for ( AActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( actor->bCanEverTick && actor->bTickPrePhysics ) {
            //actor->TickComponentsPrePhysics( _TimeStep );
            actor->TickPrePhysics( _TimeStep );
        }
    }
}

void AWorld::UpdateActorsPostPhysics( float _TimeStep )
{
    for ( AActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( actor->bCanEverTick && actor->bTickPostPhysics ) {

            //actor->TickComponentsPostPhysics( _TimeStep );
            actor->TickPostPhysics( _TimeStep );

        }

        // Update actor life span
        actor->LifeTime += _TimeStep;

        if ( actor->LifeSpan != 0.0f ) {
            actor->LifeSpan -= _TimeStep;

            if ( actor->LifeSpan <= 0.0f ) {
                actor->Destroy();
            }
        }
    }
}

void AWorld::UpdateLevels( float _TimeStep )
{
    for ( ALevel * level : ArrayOfLevels ) {
        level->Tick( _TimeStep );
    }
}

void AWorld::OnPrePhysics( float _TimeStep )
{
    GameplayTimeMicro = GameplayTimeMicroAfterTick;

    // Tick actors
    UpdateActorsPrePhysics( _TimeStep );
}

void AWorld::OnPostPhysics( float _TimeStep )
{
    UpdateActorsPostPhysics( _TimeStep );

    if ( bResetGameplayTimer ) {
        bResetGameplayTimer = false;
        GameplayTimeMicroAfterTick = 0;
    } else {
        GameplayTimeMicroAfterTick += (double)_TimeStep * 1000000.0;
    }
}

void AWorld::UpdatePhysics( float _TimeStep )
{
    if ( bPaused ) {
        return;
    }

    PhysicsWorld.Simulate( _TimeStep );

    E_OnPostPhysicsUpdate.Dispatch( _TimeStep );
}

void AWorld::UpdateSkinning()
{
    for ( ASkinnedComponent * skinnedMesh = RenderWorld.GetSkinnedMeshes() ; skinnedMesh ; skinnedMesh = skinnedMesh->GetNextSkinnedMesh() ) {
        skinnedMesh->UpdateBounds();
    }
}

void AWorld::Tick( float _TimeStep )
{
    GameRunningTimeMicro = GameRunningTimeMicroAfterTick;
    GameplayTimeMicro = GameplayTimeMicroAfterTick;

    UpdatePauseStatus();

    // Tick timers. FIXME: move to PrePhysicsTick?
    UpdateTimers( _TimeStep );

    // Tick actors
    UpdateActors( _TimeStep );

    // Tick physics
    UpdatePhysics( _TimeStep );

    // Tick navigation
    NavigationMesh.Update( _TimeStep );

    // Tick skinning
    UpdateSkinning();

    // Tick levels
    UpdateLevels( _TimeStep );

    KickoffPendingKillObjects();

    uint64_t frameDuration = (double)_TimeStep * 1000000;
    GameRunningTimeMicroAfterTick += frameDuration;
}

bool AWorld::Raycast( SWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) const
{
    return GEngine->GetVSD()->RaycastTriangles( const_cast< AWorld * >( this ), _Result, _RayStart, _RayEnd, _Filter );
}

bool AWorld::RaycastBounds( TPodArray< SBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) const
{
    return GEngine->GetVSD()->RaycastBounds( const_cast< AWorld * >( this ), _Result, _RayStart, _RayEnd, _Filter );
}

bool AWorld::RaycastClosest( SWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) const
{
    return GEngine->GetVSD()->RaycastClosest( const_cast< AWorld * >( this ), _Result, _RayStart, _RayEnd, _Filter );
}

bool AWorld::RaycastClosestBounds( SBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) const
{
    return GEngine->GetVSD()->RaycastClosestBounds( const_cast< AWorld * >( this ), _Result, _RayStart, _RayEnd, _Filter );
}

void AWorld::QueryVisiblePrimitives( TPodArray< SPrimitiveDef * > & VisPrimitives, TPodArray< SSurfaceDef * > & VisSurfs, int * VisPass, SVisibilityQuery const & InQuery )
{
    GEngine->GetVSD()->QueryVisiblePrimitives( this, VisPrimitives, VisSurfs, VisPass, InQuery );
}

void AWorld::ApplyRadialDamage( float _DamageAmount, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter )
{
    TPodArray< AActor * > damagedActors;
    SActorDamage damage;

    QueryActors( damagedActors, _Position, _Radius, _QueryFilter );

    damage.Amount = _DamageAmount;
    damage.Position = _Position;
    damage.Radius = _Radius;
    damage.DamageCauser = nullptr;

    for ( AActor * damagedActor : damagedActors ) {
        damagedActor->ApplyDamage( damage );
    }
}

void AWorld::KickoffPendingKillObjects()
{
    while ( PendingKillComponents ) {
        AActorComponent * component = PendingKillComponents;
        AActorComponent * nextComponent;

        PendingKillComponents = nullptr;

        while ( component ) {
            nextComponent = component->NextPendingKillComponent;

            // FIXME: Call component->EndPlay here?

            // Remove component from actor array of components
            AActor * parent = component->OwnerActor;
            if ( parent /*&& !parent->IsPendingKill()*/ ) {
                parent->Components[ component->ComponentIndex ] = parent->Components[ parent->Components.Size() - 1 ];
                parent->Components[ component->ComponentIndex ]->ComponentIndex = component->ComponentIndex;
                parent->Components.RemoveLast();
            }
            component->ComponentIndex = -1;
            component->OwnerActor = nullptr;
            component->RemoveRef();

            component = nextComponent;
        }
    }

    while ( PendingKillActors ) {
        AActor * actor = PendingKillActors;
        AActor * nextActor;

        PendingKillActors = nullptr;

        while ( actor ) {
            nextActor = actor->NextPendingKillActor;

            // FIXME: Call actor->EndPlay here?

            // Remove actor from world array of actors
            Actors[ actor->IndexInWorldArrayOfActors ] = Actors[ Actors.Size() - 1 ];
            Actors[ actor->IndexInWorldArrayOfActors ]->IndexInWorldArrayOfActors = actor->IndexInWorldArrayOfActors;
            Actors.RemoveLast();
            actor->IndexInWorldArrayOfActors = -1;
            actor->ParentWorld = nullptr;

//            // Remove actor from level array of actors
//            ALevel * level = actor->Level;
//            level->Actors[ actor->IndexInLevelArrayOfActors ] = level->Actors[ level->Actors.Size() - 1 ];
//            level->Actors[ actor->IndexInLevelArrayOfActors ]->IndexInLevelArrayOfActors = actor->IndexInLevelArrayOfActors;
//            level->Actors.RemoveLast();
//            actor->IndexInLevelArrayOfActors = -1;
//            actor->Level = nullptr;

            actor->RemoveRef();

            actor = nextActor;
        }
    }
}

TRef< ADocObject > AWorld::Serialize()
{
    TRef< ADocObject > object = Super::Serialize();

    if ( !Actors.IsEmpty() ) {
        ADocMember * actors = object->AddArray( "Actors" );

//        std::unordered_set< std::string > precacheStrs;

        for ( AActor * actor : Actors ) {
            if ( actor->IsPendingKill() ) {
                continue;
            }
            TRef< ADocObject > actorObject = actor->Serialize();
            actors->AddValue( actorObject );

//            AClassMeta const & classMeta = actor->FinalClassMeta();

//            for ( APrecacheMeta const * precache = classMeta.GetPrecacheList() ; precache ; precache = precache->Next() ) {
//                precacheStrs.insert( precache->GetResourcePath() );
//            }
//            // TODO: precache all objects, not only actors
        }

//        if ( !precacheStrs.empty() ) {
//            int precache = pDocument->AddArray( object, "Precache" );
//            for ( auto it : precacheStrs ) {
//                const std::string & s = it;

//                pDocument->AddValueToField( precache, pDocument->CreateStringValue( s.c_str() ) );
//            }
//        }
    }

    return object;
}

void AWorld::AddLevel( ALevel * _Level )
{
    if ( _Level->IsPersistentLevel() ) {
        GLogger.Printf( "AWorld::AddLevel: Can't add persistent level\n" );
        return;
    }

    if ( _Level->OwnerWorld == this ) {
        // Already in world
        return;
    }

    if ( _Level->OwnerWorld ) {
        _Level->OwnerWorld->RemoveLevel( _Level );
    }

    _Level->OwnerWorld = this;
    _Level->IndexInArrayOfLevels = ArrayOfLevels.Size();
    _Level->AddRef();
    _Level->OnAddLevelToWorld();
    ArrayOfLevels.Append( _Level );
}

void AWorld::RemoveLevel( ALevel * _Level )
{
    if ( !_Level ) {
        return;
    }

    if ( _Level->IsPersistentLevel() ) {
        GLogger.Printf( "AWorld::AddLevel: Can't remove persistent level\n" );
        return;
    }

    if ( _Level->OwnerWorld != this ) {
        GLogger.Printf( "AWorld::AddLevel: level is not in world\n" );
        return;
    }

    _Level->OnRemoveLevelFromWorld();

    ArrayOfLevels[ _Level->IndexInArrayOfLevels ] = ArrayOfLevels[ ArrayOfLevels.Size() - 1 ];
    ArrayOfLevels[ _Level->IndexInArrayOfLevels ]->IndexInArrayOfLevels = _Level->IndexInArrayOfLevels;
    ArrayOfLevels.RemoveLast();

    _Level->OwnerWorld = nullptr;
    _Level->IndexInArrayOfLevels = -1;
    _Level->RemoveRef();
}

void AWorld::AddTimer( ATimer * _Timer )
{
    _Timer->AddRef();
    if ( bDuringTimerTick )
    {
        GLogger.Printf( "AWorld::AddTimer: Add pending\n" );
        TimerCmd.Append( { STimerCmd::ADD, _Timer } );
    }
    else
    {
        GLogger.Printf( "AWorld::AddTimer: Add now\n" );
        INTRUSIVE_ADD_UNIQUE( _Timer, Next, Prev, TimerList, TimerListTail );
    }
}

void AWorld::RemoveTimer( ATimer * _Timer )
{
    _Timer->RemoveRef();
    if ( bDuringTimerTick )
    {
        GLogger.Printf( "AWorld::RemoveTimer: Remove pending\n" );
        TimerCmd.Append( { STimerCmd::REMOVE, _Timer } );
    }
    else
    {
        GLogger.Printf( "AWorld::RemoveTimer: Remove now\n" );
        INTRUSIVE_REMOVE( _Timer, Next, Prev, TimerList, TimerListTail );
    }
}

void AWorld::DrawDebug( ADebugRenderer * InRenderer )
{
    for ( ALevel * level : ArrayOfLevels ) {
        level->DrawDebug( InRenderer );
    }

    GEngine->GetVSD()->DrawDebug( InRenderer );

    for ( AActor * actor : Actors ) {
        actor->DrawDebug( InRenderer );
    }

    RenderWorld.DrawDebug( InRenderer );

    PhysicsWorld.DrawDebug( InRenderer );

    NavigationMesh.DrawDebug( InRenderer );
}

AWorld * AWorld::CreateWorld()
{
    AWorld * world = CreateInstanceOf< AWorld >();

    world->AddRef();

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Size() - 1;

    world->BeginPlay();

    return world;
}

void AWorld::DestroyWorlds()
{
    for ( AWorld * world : Worlds ) {
        world->Destroy();
    }
}

void AWorld::KickoffPendingKillWorlds()
{
    while ( PendingKillWorlds ) {
        AWorld * world = PendingKillWorlds;
        AWorld * nextWorld;

        PendingKillWorlds = nullptr;

        while ( world ) {
            nextWorld = world->NextPendingKillWorld;

            // FIXME: Call world->EndPlay here?

            // Remove world from game array of worlds
            Worlds[ world->IndexInGameArrayOfWorlds ] = Worlds[ Worlds.Size() - 1 ];
            Worlds[ world->IndexInGameArrayOfWorlds ]->IndexInGameArrayOfWorlds = world->IndexInGameArrayOfWorlds;
            Worlds.RemoveLast();
            world->IndexInGameArrayOfWorlds = -1;
            world->RemoveRef();

            world = nextWorld;
        }
    }

    if ( Worlds.IsEmpty() ) {
        Worlds.Free();
    }
}

void AWorld::UpdateWorlds( float _TimeStep )
{
    for ( AWorld * world : Worlds ) {
        if ( world->IsPendingKill() ) {
            continue;
        }
        world->Tick( _TimeStep );
    }

    KickoffPendingKillWorlds();

    GPrimitiveLinkPool.CleanupEmptyBlocks();
}

void AWorld::SetGlobalIrradianceMap( int Index )
{
    GlobalIrradianceMap = Index;
}

void AWorld::SetGlobalReflectionMap( int Index )
{
    GlobalReflectionMap = Index;
}
