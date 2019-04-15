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

#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Actor.h>
#include <Engine/World/Public/ActorComponent.h>
#include <Engine/World/Public/SceneComponent.h>
#include <Engine/World/Public/MeshComponent.h>
#include <Engine/World/Public/SkeletalAnimation.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/Timer.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_BEGIN_CLASS_META( FWorld )
//AN_ATTRIBUTE_( bTickEvenWhenPaused, AF_DEFAULT )
AN_END_CLASS_META()

void FActorSpawnParameters::SetTemplate( FActor const * _Template ) {
    AN_Assert( &_Template->FinalClassMeta() == ActorTypeClassMeta );
    Template = _Template;
}

FWorld::FWorld() {
    PersistentLevel = NewObject< FLevel >();
    PersistentLevel->AddRef();
    PersistentLevel->OwnerWorld = this;
    PersistentLevel->bIsPersistent = true;
    PersistentLevel->IndexInArrayOfLevels = ArrayOfLevels.Length();
    ArrayOfLevels.Append( PersistentLevel );
}

void FWorld::Destroy() {
    if ( bPendingKill ) {
        return;
    }

    // Mark world to remove them from the game
    bPendingKill = true;
    NextPendingKillWorld = GGameMaster.PendingKillWorlds;
    GGameMaster.PendingKillWorlds = this;

    DestroyActors();
    KickoffPendingKillObjects();

    // Remove all levels from world including persistent level
    for ( FLevel * level : ArrayOfLevels ) {
        if ( !level->bIsPersistent ) {
            level->OnRemoveLevelFromWorld();
        }
        level->IndexInArrayOfLevels = -1;
        level->OwnerWorld = nullptr;
        level->RemoveRef();
    }
    ArrayOfLevels.Clear();

    EndPlay();
}

void FWorld::DestroyActors() {
    for ( FActor * actor : Actors ) {
        actor->Destroy();
    }
}

FActor * FWorld::SpawnActor( FActorSpawnParameters const & _SpawnParameters ) {

    GLogger.Printf( "==== Spawn Actor ====\n" );

    FClassMeta const * classMeta = _SpawnParameters.ActorClassMeta();

    if ( !classMeta ) {
        GLogger.Printf( "FWorld::SpawnActor: invalid actor class\n" );
        return nullptr;
    }

    if ( classMeta->Factory() != &FActor::Factory() ) {
        GLogger.Printf( "FWorld::SpawnActor: not an actor class\n" );
        return nullptr;
    }

    FActor const * templateActor = _SpawnParameters.GetTemplate();

    if ( templateActor && classMeta != &templateActor->FinalClassMeta() ) {
        GLogger.Printf( "FWorld::SpawnActor: FActorSpawnParameters::Template class doesn't match meta data\n" );
        return nullptr;
    }

    FActor * actor = static_cast< FActor * >( classMeta->CreateInstance() );
    actor->AddRef();
    actor->bDuringConstruction = false;

    // Add actor to world array of actors
    Actors.Append( actor );
    actor->IndexInWorldArrayOfActors = Actors.Length() - 1;
    actor->ParentWorld = this;

    actor->Level = _SpawnParameters.Level ? _SpawnParameters.Level : PersistentLevel;
    actor->Level->Actors.Append( actor );
    actor->IndexInLevelArrayOfActors = actor->Level->Actors.Length() - 1;

    // Update actor name to make it unique
    actor->SetName( actor->Name );

    if ( templateActor ) {
        actor->Clone( templateActor );
    }

    actor->PostSpawnInitialize( _SpawnParameters.SpawnTransform );
    //actor->PostActorCreated();
    //actor->ExecuteContruction( _SpawnParameters.SpawnTransform );
    actor->PostActorConstruction();

    BroadcastActorSpawned( actor );
    actor->BeginPlayComponents();
    actor->BeginPlay();

    GLogger.Printf( "=====================\n" );
    return actor;
}

FActor * FWorld::LoadActor( FDocument const & _Document, int _FieldsHead, FLevel * _Level ) {

    GLogger.Printf( "==== Load Actor ====\n" );

    FDocumentField * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "FWorld::LoadActor: invalid actor class\n" );
        return nullptr;
    }

    FDocumentValue * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    FClassMeta const * classMeta = FActor::Factory().LookupClass( classNameValue->Token.ToString().ToConstChar() );
    if ( !classMeta ) {
        GLogger.Printf( "FWorld::LoadActor: invalid actor class \"%s\"\n", classNameValue->Token.ToString().ToConstChar() );
        return nullptr;
    }

    FActor * actor = static_cast< FActor * >( classMeta->CreateInstance() );
    actor->AddRef();
    actor->bDuringConstruction = false;

    // Add actor to world array of actors
    Actors.Append( actor );
    actor->IndexInWorldArrayOfActors = Actors.Length() - 1;
    actor->ParentWorld = this;

    actor->Level = _Level ? _Level : PersistentLevel;
    actor->Level->Actors.Append( actor );
    actor->IndexInLevelArrayOfActors = actor->Level->Actors.Length() - 1;

    // Update actor name to make it unique
    actor->SetName( actor->Name );

    // Load actor attributes
    actor->LoadAttributes( _Document, _FieldsHead );

    // Load components
    FDocumentField * componentsArray = _Document.FindField( _FieldsHead, "Components" );
    for ( int i = componentsArray->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
        FDocumentValue const * componentObject = &_Document.Values[ i ];
        if ( componentObject->Type != FDocumentValue::T_Object ) {
            continue;
        }
        actor->LoadComponent( _Document, componentObject->FieldsHead );
    }

    // Load root component
    FDocumentField * rootField = _Document.FindField( _FieldsHead, "Root" );
    if ( rootField ) {
        FDocumentValue * rootValue = &_Document.Values[ rootField->ValuesHead ];
        FSceneComponent * root = dynamic_cast< FSceneComponent * >( actor->FindComponent( rootValue->Token.ToString().ToConstChar() ) );
        if ( root ) {
            actor->RootComponent = root;
        }
    }

    //actor->PostActorCreated();
    //actor->ExecuteContruction( _SpawnParameters.SpawnTransform );
    actor->PostActorConstruction();
    BroadcastActorSpawned( actor );
    actor->BeginPlayComponents();
    actor->BeginPlay();

    GLogger.Printf( "=====================\n" );
    return actor;
}

FString FWorld::GenerateActorUniqueName( const char * _Name ) {
    // TODO: optimize!
    if ( !FindActor( _Name ) ) {
        return _Name;
    }
    int uniqueNumber = 0;
    FString uniqueName;
    do {
        uniqueName.Resize( 0 );
        uniqueName.Concat( _Name );
        uniqueName.Concat( Int( ++uniqueNumber ).ToConstChar() );
    } while ( FindActor( uniqueName.ToConstChar() ) != nullptr );
    return uniqueName;
}

FActor * FWorld::FindActor( const char * _UniqueName ) {
    // TODO: Use hash!
    for ( FActor * actor : Actors ) {
        if ( !actor->GetName().Icmp( _UniqueName ) ) {
            return actor;
        }
    }
    return nullptr;
}

void FWorld::BroadcastActorSpawned( FActor * _SpawnedActor ) {
    for ( FActor * actor : Actors ) {
        if ( actor != _SpawnedActor ) {
            actor->OnActorSpawned( _SpawnedActor );
        }
    }
}

void FWorld::BeginPlay() {
    GLogger.Printf( "FWorld::BeginPlay()\n" );
}

void FWorld::EndPlay() {
    GLogger.Printf( "FWorld::EndPlay()\n" );
}

void FWorld::Tick( float _TimeStep ) {
    //if ( GGameMaster.IsGamePaused() && !bTickEvenWhenPaused ) {
    //    return;
    //}

    // Tick all timers
    for ( FTimer * timer = TimerList ; timer ; timer = timer->Next ) {
        timer->Tick( _TimeStep );
    }

    // Tick actors
    for ( FActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( actor->bCanEverTick ) {

            if ( GGameMaster.IsGamePaused() && !actor->bTickEvenWhenPaused ) {
                continue;
            }

            actor->TickComponents( _TimeStep );
            actor->Tick( _TimeStep );

            actor->LifeTime += _TimeStep;
            if ( actor->LifeSpan > 0.0f && actor->LifeTime > actor->LifeSpan ) {
                actor->Destroy();
            }
        }
    }

    KickoffPendingKillObjects();

    WorldLocalTime += _TimeStep;
    if ( !GGameMaster.IsGamePaused() ) {
        WorldPlayTime += _TimeStep;
    }
}

void FWorld::KickoffPendingKillObjects() {
    while ( PendingKillComponents ) {
        FActorComponent * component = PendingKillComponents;
        FActorComponent * nextComponent;

        PendingKillComponents = nullptr;

        while ( component ) {
            nextComponent = component->NextPendingKillComponent;

            // FIXME: Call component->EndPlay here?

            // Remove component from actor array of components
            FActor * parent = component->ParentActor;
            if ( parent /*&& !parent->IsPendingKill()*/ ) {
                parent->Components[ component->ComponentIndex ] = parent->Components[ parent->Components.Length() - 1 ];
                parent->Components[ component->ComponentIndex ]->ComponentIndex = component->ComponentIndex;
                parent->Components.RemoveLast();
            }
            component->ComponentIndex = -1;
            component->ParentActor = nullptr;
            component->RemoveRef();

            component = nextComponent;
        }
    }

    while ( PendingKillActors ) {
        FActor * actor = PendingKillActors;
        FActor * nextActor;

        PendingKillActors = nullptr;

        while ( actor ) {
            nextActor = actor->NextPendingKillActor;

            // FIXME: Call actor->EndPlay here?

            // Remove actor from world array of actors
            Actors[ actor->IndexInWorldArrayOfActors ] = Actors[ Actors.Length() - 1 ];
            Actors[ actor->IndexInWorldArrayOfActors ]->IndexInWorldArrayOfActors = actor->IndexInWorldArrayOfActors;
            Actors.RemoveLast();
            actor->IndexInWorldArrayOfActors = -1;
            actor->ParentWorld = nullptr;

            // Remove actor from level array of actors
            FLevel * level = actor->Level;
            level->Actors[ actor->IndexInLevelArrayOfActors ] = level->Actors[ level->Actors.Length() - 1 ];
            level->Actors[ actor->IndexInLevelArrayOfActors ]->IndexInLevelArrayOfActors = actor->IndexInLevelArrayOfActors;
            level->Actors.RemoveLast();
            actor->IndexInLevelArrayOfActors = -1;
            actor->Level = nullptr;

            actor->RemoveRef();

            actor = nextActor;
        }
    }
}
//#include <unordered_set>
int FWorld::Serialize( FDocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    if ( !Actors.IsEmpty() ) {
        int actors = _Doc.AddArray( object, "Actors" );

//        std::unordered_set< std::string > precacheStrs;

        for ( FActor * actor : Actors ) {
            if ( actor->IsPendingKill() ) {
                continue;
            }
            int actorObject = actor->Serialize( _Doc );
            _Doc.AddValueToField( actors, actorObject );

//            FClassMeta const & classMeta = actor->FinalClassMeta();

//            for ( FPrecacheMeta const * precache = classMeta.GetPrecacheList() ; precache ; precache = precache->Next() ) {
//                precacheStrs.insert( precache->GetResourcePath() );
//            }
//            // TODO: precache all objects, not only actors
        }

//        if ( !precacheStrs.empty() ) {
//            int precache = _Doc.AddArray( object, "Precache" );
//            for ( auto it : precacheStrs ) {
//                const std::string & s = it;

//                _Doc.AddValueToField( precache, _Doc.CreateStringValue( _Doc.ProxyBuffer.NewString( s.c_str() ).ToConstChar() ) );
//            }
//        }
    }

    return object;
}

void FWorld::AddLevel( FLevel * _Level ) {
    if ( _Level->IsPersistentLevel() ) {
        GLogger.Printf( "FWorld::AddLevel: Can't add persistent level\n" );
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
    _Level->IndexInArrayOfLevels = ArrayOfLevels.Length();
    _Level->AddRef();
    _Level->OnAddLevelToWorld();
    ArrayOfLevels.Append( _Level );
}

void FWorld::RemoveLevel( FLevel * _Level ) {
    if ( !_Level ) {
        return;
    }

    if ( _Level->IsPersistentLevel() ) {
        GLogger.Printf( "FWorld::AddLevel: Can't remove persistent level\n" );
        return;
    }

    if ( _Level->OwnerWorld != this ) {
        GLogger.Printf( "FWorld::AddLevel: level is not in world\n" );
        return;
    }

    _Level->OnRemoveLevelFromWorld();

    ArrayOfLevels[ _Level->IndexInArrayOfLevels ] = ArrayOfLevels[ ArrayOfLevels.Length() - 1 ];
    ArrayOfLevels[ _Level->IndexInArrayOfLevels ]->IndexInArrayOfLevels = _Level->IndexInArrayOfLevels;
    ArrayOfLevels.RemoveLast();

    _Level->OwnerWorld = nullptr;
    _Level->IndexInArrayOfLevels = -1;
    _Level->RemoveRef();
}

void FWorld::RegisterMesh( FMeshComponent * _Mesh ) {
    if ( IntrusiveIsInList( _Mesh, Next, Prev, MeshList, MeshListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Mesh, Next, Prev, MeshList, MeshListTail );
}

void FWorld::UnregisterMesh( FMeshComponent * _Mesh ) {
    IntrusiveRemoveFromList( _Mesh, Next, Prev, MeshList, MeshListTail );
}

void FWorld::RegisterSkinnedMesh( FSkinnedComponent * _Skeleton ) {
    if ( IntrusiveIsInList( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void FWorld::UnregisterSkinnedMesh( FSkinnedComponent * _Skeleton ) {
    IntrusiveRemoveFromList( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void FWorld::RegisterTimer( FTimer * _Timer ) {
    if ( IntrusiveIsInList( _Timer, Next, Prev, TimerList, TimerListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Timer, Next, Prev, TimerList, TimerListTail );
}

void FWorld::UnregisterTimer( FTimer * _Timer ) {
    IntrusiveRemoveFromList( _Timer, Next, Prev, TimerList, TimerListTail );
}

void FWorld::DrawDebug( FDebugDraw * _DebugDraw ) {

    if ( DebugDrawFrame == GGameMaster.GetFrameNumber() ) {
        // Debug commands was already generated for this frame
        return;
    }

    DebugDrawFrame = GGameMaster.GetFrameNumber();

    FirstDebugDrawCommand = _DebugDraw->CommandsCount();

    _DebugDraw->SplitCommands();

    for ( FLevel * level : ArrayOfLevels ) {
        level->DrawDebug( _DebugDraw );
    }

    _DebugDraw->SetDepthTest( true );

    _DebugDraw->SetColor(1,1,1,1);
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_STATIC_BOUNDS ) {
        for ( FMeshComponent * component = MeshList
              ; component ; component = component->GetNextMesh() ) {

            _DebugDraw->DrawAABB( component->GetWorldBounds() );
        }
    //}

    for ( FActor * actor : Actors ) {
        actor->DrawDebug( _DebugDraw );
    }

    //TPodArray< FMeshVertex > Vertices;
    //TPodArray< unsigned int > Indices;
    //BvAxisAlignedBox  Bounds;
    //FCylinderShape::CreateMesh( Vertices, Indices, Bounds, 10, 20, 1, 16 );
    //_DebugDraw->SetColor( 0,1,0,0.1f);
    //_DebugDraw->DrawTriangleSoup( &Vertices.ToPtr()->Position, Vertices.Length(), sizeof(FMeshVertex), Indices.ToPtr(), Indices.Length() );
    //_DebugDraw->SetColor( 1,1,0,0.1f);
    //_DebugDraw->DrawPoints( &Vertices.ToPtr()->Position, Vertices.Length(), sizeof(FMeshVertex) );
    //_DebugDraw->DrawCircleFilled( Float3(0), Float3(0,1,0), 10.0f );
    //_DebugDraw->SetColor( 0,0,1,1);
    //_DebugDraw->DrawCylinder( Float3(0), Float3x3::Identity(), 10, 20 );
    //_DebugDraw->SetColor( 1,0,0,1);
    //_DebugDraw->DrawDottedLine( Float3(0,0,0), Float3(100,0,0), 1.0f );
    //_DebugDraw->SetColor( 0,1,0,1);
    //_DebugDraw->DrawDottedLine( Float3(0,0,0), Float3(0,100,0), 1.0f );
    //_DebugDraw->SetColor( 0,0,1,1);
    //_DebugDraw->DrawDottedLine( Float3(0,0,0), Float3(0,0,100), 1.0f );

    DebugDrawCommandCount = _DebugDraw->CommandsCount() - FirstDebugDrawCommand;

    //GLogger.Printf( "DebugDrawCommandCount %d\n", DebugDrawCommandCount );
}
