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

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <btBulletDynamicsCommon.h>
#include "BulletCompatibility/BulletCompatibility.h"

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning( disable : 4456 )
#endif
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif

AN_BEGIN_CLASS_META( FWorld )
//AN_ATTRIBUTE_( bTickEvenWhenPaused, AF_DEFAULT )
AN_END_CLASS_META()

void FActorSpawnParameters::SetTemplate( FActor const * _Template ) {
    AN_Assert( &_Template->FinalClassMeta() == ActorTypeClassMeta );
    Template = _Template;
}

class FPhysicsDebugDraw : public btIDebugDraw {
public:
    FDebugDraw * DD;
    int DebugMode;

    void drawLine( btVector3 const & from, btVector3 const & to, btVector3 const & color ) override {
        DD->SetColor( color.x(), color.y(), color.z(), 1.0f );
        DD->DrawLine( btVectorToFloat3( from ), btVectorToFloat3( to ) );
    }

    void drawContactPoint( btVector3 const & pointOnB, btVector3 const & normalOnB, btScalar distance, int lifeTime, btVector3 const & color ) override {
        DD->SetColor( color.x(), color.y(), color.z(), 1.0f );
        DD->DrawPoint( btVectorToFloat3( pointOnB ) );
        DD->DrawPoint( btVectorToFloat3( normalOnB ) );
    }

    void reportErrorWarning( const char * warningString ) override {
    }

    void draw3dText( btVector3 const & location, const char * textString ) override {
    }

    void setDebugMode( int debugMode ) override {
        DebugMode = debugMode;
    }

    int getDebugMode() const override {
        return DebugMode;
    }

    void flushLines() override {
    }
};

static FPhysicsDebugDraw PhysicsDebugDraw;


FWorld::FWorld() {
    PersistentLevel = NewObject< FLevel >();
    PersistentLevel->AddRef();
    PersistentLevel->OwnerWorld = this;
    PersistentLevel->bIsPersistent = true;
    PersistentLevel->IndexInArrayOfLevels = ArrayOfLevels.Length();
    ArrayOfLevels.Append( PersistentLevel );

    GravityVector = Float3( 0.0f, -9.81f, 0.0f );

#if 0
    PhysicsBroadphase = b3New( btDbvtBroadphase ); // TODO: AxisSweep3Internal?
#else
    PhysicsBroadphase = b3New( btAxisSweep3, btVector3(-10000,-10000,-10000), btVector3(10000,10000,10000) );
#endif
    //CollisionConfiguration = b3New( btDefaultCollisionConfiguration );
    CollisionConfiguration = b3New( btSoftBodyRigidBodyCollisionConfiguration );
    CollisionDispatcher = b3New( btCollisionDispatcher, CollisionConfiguration );
    ConstraintSolver = b3New( btSequentialImpulseConstraintSolver );
    PhysicsWorld = b3New( btSoftRigidDynamicsWorld, CollisionDispatcher, PhysicsBroadphase, ConstraintSolver, CollisionConfiguration, /* SoftBodySolver */ 0 );
    PhysicsWorld->setGravity( btVectorToFloat3( GravityVector ) );
    PhysicsWorld->getDispatchInfo().m_useContinuous = true;
    PhysicsWorld->getSolverInfo().m_splitImpulse = GGameMaster.bContactSolverSplitImpulse;
    PhysicsWorld->getSolverInfo().m_numIterations = GGameMaster.NumContactSolverIterations;
    PhysicsWorld->setDebugDrawer( &PhysicsDebugDraw );
    PhysicsWorld->setInternalTickCallback( OnPrePhysics, static_cast< void * >( this ), true );
    PhysicsWorld->setInternalTickCallback( OnPostPhysics, static_cast< void * >( this ), false );

    // TODO: remove this if we don't use gimpact
    btGImpactCollisionAlgorithm::registerAlgorithm( CollisionDispatcher );

    SoftBodyWorldInfo = &PhysicsWorld->getWorldInfo();//b3New( btSoftBodyWorldInfo );
    SoftBodyWorldInfo->m_dispatcher = CollisionDispatcher;
    SoftBodyWorldInfo->m_broadphase = PhysicsBroadphase;
    SoftBodyWorldInfo->m_gravity = btVectorToFloat3( GravityVector );
    SoftBodyWorldInfo->air_density = ( btScalar )1.2;
    SoftBodyWorldInfo->water_density = 0;
    SoftBodyWorldInfo->water_offset = 0;
    SoftBodyWorldInfo->water_normal = btVector3( 0, 0, 0 );
    SoftBodyWorldInfo->m_sparsesdf.Initialize();
}

void FWorld::OnPrePhysics( btDynamicsWorld * _World, float _TimeStep ) {
    static_cast< FWorld * >( _World->getWorldUserInfo() )->OnPrePhysics( _TimeStep );
}

void FWorld::OnPostPhysics( btDynamicsWorld * _World, float _TimeStep ) {
    static_cast< FWorld * >( _World->getWorldUserInfo() )->OnPostPhysics( _TimeStep );
}

void FWorld::SetGravityVector( Float3 const & _Gravity ) {
    GravityVector = _Gravity;
    bGravityDirty = true;
}

Float3 const & FWorld::GetGravityVector() const {
    return GravityVector;
}

void FWorld::Destroy() {
    if ( bPendingKill ) {
        return;
    }

    // Mark world to remove it from the game
    bPendingKill = true;
    NextPendingKillWorld = GGameMaster.PendingKillWorlds;
    GGameMaster.PendingKillWorlds = this;

#if 0
    for ( int i = 0 ; i < 2 ; i++ ) {
        TPodArray< FCollisionContact > & currentContacts = CollisionContacts[ i ];
        THash<> & contactHash = ContactHash[ i ];

        for ( FCollisionContact & contact : currentContacts ) {
            contact.ActorA->RemoveRef();
            contact.ActorB->RemoveRef();
            contact.ComponentA->RemoveRef();
            contact.ComponentB->RemoveRef();
        }

        currentContacts.Clear();
        contactHash.Clear();
    }
#endif

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

    b3Destroy( PhysicsWorld );
    //b3Destroy( SoftBodyWorldInfo );
    b3Destroy( ConstraintSolver );
    b3Destroy( CollisionDispatcher );
    b3Destroy( CollisionConfiguration );
    b3Destroy( PhysicsBroadphase );

    EndPlay();
}

void FWorld::DestroyActors() {
    for ( FActor * actor : Actors ) {
        actor->Destroy();
    }
}

FActor * FWorld::SpawnActor( FActorSpawnParameters const & _SpawnParameters ) {

    //GLogger.Printf( "==== Spawn Actor ====\n" );

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

    //GLogger.Printf( "=====================\n" );
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

        if ( GGameMaster.IsGamePaused() && !actor->bTickEvenWhenPaused ) {
            continue;
        }

        actor->TickComponents( _TimeStep );

        if ( actor->bCanEverTick ) {
            actor->Tick( _TimeStep );
        }

        actor->LifeTime += _TimeStep;

        if ( actor->LifeSpan > 0.0f ) {
            actor->LifeSpan -= _TimeStep;

            if ( actor->LifeSpan < 0.0f ) {
                actor->Destroy();
            }
        }
    }

    SimulatePhysics( _TimeStep );

    KickoffPendingKillObjects();

    WorldLocalTime += _TimeStep;
    if ( !GGameMaster.IsGamePaused() ) {
        WorldPlayTime += _TimeStep;
    }
}

void FWorld::OnPrePhysics( float _TimeStep ) {
    for ( FActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( actor->bCanEverTick && actor->bTickPrePhysics ) {

            if ( GGameMaster.IsGamePaused() && !actor->bTickEvenWhenPaused ) {
                continue;
            }

            //actor->TickComponentsPrePhysics( _TimeStep );
            actor->TickPrePhysics( _TimeStep );

            //actor->LifeTime += _TimeStep;
            //if ( actor->LifeSpan > 0.0f && actor->LifeTime > actor->LifeSpan ) {
            //    actor->Destroy();
            //}
        }
    }
}

static int CacheContactPoints = -1;

void FWorld::GenerateContactPoints( int _ContactIndex, FCollisionContact & _Contact ) {
    if ( CacheContactPoints == _ContactIndex ) {
        // Contact points already generated for this contact
        return;
    }

    CacheContactPoints = _ContactIndex;

    ContactPoints.ResizeInvalidate( _Contact.Manifold->getNumContacts() );

    bool bSwapped = static_cast< FPhysicalBody * >( _Contact.Manifold->getBody0()->getUserPointer() ) == _Contact.ComponentB;

    if ( ( _ContactIndex & 1 ) == 0 ) {
        // BodyA

        if ( bSwapped ) {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                FContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnA );
                contact.Normal = -btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        } else {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                FContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnB );
                contact.Normal = btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }

    } else {
        // BodyB

        if ( bSwapped ) {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                FContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnB );
                contact.Normal = btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        } else {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                FContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnA );
                contact.Normal = -btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }
    }
}

void FWorld::OnPostPhysics( float _TimeStep ) {

    DispatchContactAndOverlapEvents();

    // TODO: Post physics ticks


    PhysicsTickNumber++;
}

void FWorld::DispatchContactAndOverlapEvents() {

#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4456 )
#endif

    int curTickNumber = PhysicsTickNumber & 1;
    int prevTickNumber = ( PhysicsTickNumber + 1 ) & 1;

    TPodArray< FCollisionContact > & currentContacts = CollisionContacts[ curTickNumber ];
    TPodArray< FCollisionContact > & prevContacts = CollisionContacts[ prevTickNumber ];

    THash<> & contactHash = ContactHash[ curTickNumber ];
    THash<> & prevContactHash = ContactHash[ prevTickNumber ];

    FCollisionContact contact;

    FOverlapEvent overlapEvent;
    FContactEvent contactEvent;

    //GLogger.Printf( "PhysicsTickNumber %d\n", PhysicsTickNumber );

#if 0
    for ( int i = 0 ; i < currentContacts.Length() ; i++ ) {
        FCollisionContact & contact = currentContacts[ i ];

        contact.ActorA->RemoveRef();
        contact.ActorB->RemoveRef();
        contact.ComponentA->RemoveRef();
        contact.ComponentB->RemoveRef();
    }
#endif

    contactHash.Clear();
    currentContacts.Clear();

    int numManifolds = CollisionDispatcher->getNumManifolds();
    for ( int i = 0; i < numManifolds; i++ ) {
        btPersistentManifold * contactManifold = CollisionDispatcher->getManifoldByIndexInternal( i );

        if ( !contactManifold->getNumContacts() ) {
            continue;
        }

        FPhysicalBody * objectA = static_cast< FPhysicalBody * >( contactManifold->getBody0()->getUserPointer() );
        FPhysicalBody * objectB = static_cast< FPhysicalBody * >( contactManifold->getBody1()->getUserPointer() );

        if ( objectA < objectB ) {
            FCore::SwapArgs( objectA, objectB );
        }

        FActor * actorA = objectA->GetParentActor();
        FActor * actorB = objectB->GetParentActor();

        if ( actorA->IsPendingKill() || actorB->IsPendingKill() || objectA->IsPendingKill() || objectB->IsPendingKill() ) {
            continue;
        }

#if 0
        if ( objectA->Mass <= 0.0f && objectB->Mass <= 0.0f ) {
            // Static vs Static
            GLogger.Printf( "Static vs Static %s %s\n", objectA->GetName().ToConstChar()
                            , objectB->GetName().ToConstChar() );
            continue;
        }

        if ( objectA->bTrigger && objectB->bTrigger ) {
            // Trigger vs Trigger
            GLogger.Printf( "Trigger vs Tsrigger %s %s\n", objectA->GetName().ToConstChar()
                            , objectB->GetName().ToConstChar() );
            continue;
        }

        if ( objectA->Mass <= 0.0f && objectB->bTrigger ) {
            // Static vs Trigger
            GLogger.Printf( "Static vs Trigger %s %s\n", objectA->GetName().ToConstChar()
                , objectB->GetName().ToConstChar() );
            continue;
        }

        if ( objectB->Mass <= 0.0f && objectA->bTrigger ) {
            // Static vs Trigger
            GLogger.Printf( "Static vs Trigger %s %s\n", objectB->GetName().ToConstChar()
                , objectA->GetName().ToConstChar() );
            continue;
        }
#endif

        bool bContactWithTrigger = objectA->bTrigger || objectB->bTrigger; // Do not generate contact events if one of components is trigger

        //GLogger.Printf( "Contact %s %s\n", objectA->GetName().ToConstChar()
        //                , objectB->GetName().ToConstChar() );

        contact.bComponentADispatchContactEvents = !bContactWithTrigger && objectA->bDispatchContactEvents
            && ( objectA->E_OnBeginContact
                || objectA->E_OnEndContact
                || objectA->E_OnUpdateContact );

        contact.bComponentBDispatchContactEvents = !bContactWithTrigger && objectB->bDispatchContactEvents
            && ( objectB->E_OnBeginContact
                || objectB->E_OnEndContact
                || objectB->E_OnUpdateContact );

        contact.bComponentADispatchOverlapEvents = objectA->bTrigger && objectA->bDispatchOverlapEvents
            && ( objectA->E_OnBeginOverlap
                || objectA->E_OnEndOverlap
                || objectA->E_OnUpdateOverlap );

        contact.bComponentBDispatchOverlapEvents = objectB->bTrigger && objectB->bDispatchOverlapEvents
            && ( objectB->E_OnBeginOverlap
                || objectB->E_OnEndOverlap
                || objectB->E_OnUpdateOverlap );

        contact.bActorADispatchContactEvents = !bContactWithTrigger && objectA->bDispatchContactEvents
            && ( actorA->E_OnBeginContact
                || actorA->E_OnEndContact
                || actorA->E_OnUpdateContact );

        contact.bActorBDispatchContactEvents = !bContactWithTrigger && objectB->bDispatchContactEvents
            && ( actorB->E_OnBeginContact
                || actorB->E_OnEndContact
                || actorB->E_OnUpdateContact );

        contact.bActorADispatchOverlapEvents = objectA->bTrigger && objectA->bDispatchOverlapEvents
            && ( actorA->E_OnBeginOverlap
                || actorA->E_OnEndOverlap
                || actorA->E_OnUpdateOverlap );

        contact.bActorBDispatchOverlapEvents = objectB->bTrigger && objectB->bDispatchOverlapEvents
           && ( actorB->E_OnBeginOverlap
                || actorB->E_OnEndOverlap
                || actorB->E_OnUpdateOverlap );

        if ( contact.bComponentADispatchContactEvents
            || contact.bComponentBDispatchContactEvents
            || contact.bComponentADispatchOverlapEvents
            || contact.bComponentBDispatchOverlapEvents
            || contact.bActorADispatchContactEvents
            || contact.bActorBDispatchContactEvents
            || contact.bActorADispatchOverlapEvents
            || contact.bActorBDispatchOverlapEvents ) {

            contact.ActorA = actorA;
            contact.ActorB = actorB;
            contact.ComponentA = objectA;
            contact.ComponentB = objectB;
            contact.Manifold = contactManifold;

            int hash = contact.Hash();

            bool bUnique = true;
#if 1//#ifdef AN_DEBUG
            for ( int h = contactHash.First( hash ) ; h != -1 ; h = contactHash.Next( h ) ) {
                if ( currentContacts[ h ].ComponentA == objectA
                    && currentContacts[ h ].ComponentB == objectB ) {
                    bUnique = false;
                    break;
                }
            }
            AN_Assert( bUnique );
#endif
            if ( bUnique ) {

#if 0
                actorA->AddRef();
                actorB->AddRef();
                objectA->AddRef();
                objectB->AddRef();
#endif

                currentContacts.Append( contact );
                contactHash.Insert( hash, currentContacts.Length() - 1 );
            }
        }
    }

    // Reset cache
    CacheContactPoints = -1;

    // Dispatch contact and overlap events (OnBeginContact, OnBeginOverlap, OnUpdateContact, OnUpdateOverlap)
    for ( int i = 0 ; i < currentContacts.Length() ; i++ ) {
        FCollisionContact & contact = currentContacts[ i ];

        int hash = contact.Hash();
        bool bFirstContact = true;

        for ( int h = prevContactHash.First( hash ); h != -1; h = prevContactHash.Next( h ) ) {
            if ( prevContacts[ h ].ComponentA == contact.ComponentA
                && prevContacts[ h ].ComponentB == contact.ComponentB ) {
                bFirstContact = false;
                break;
            }
        }

        if ( contact.bActorADispatchContactEvents ) {

            if ( contact.ActorA->E_OnBeginContact || contact.ActorA->E_OnUpdateContact ) {

                if ( contact.ComponentA->bGenerateContactPoints ) {
                    GenerateContactPoints( i << 1, contact );

                    contactEvent.Points = ContactPoints.ToPtr();
                    contactEvent.NumPoints = ContactPoints.Length();
                } else {
                    contactEvent.Points = NULL;
                    contactEvent.NumPoints = 0;
                }

                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;

                if ( bFirstContact ) {
                    contact.ActorA->E_OnBeginContact.Dispatch( contactEvent );
                } else {
                    contact.ActorA->E_OnUpdateContact.Dispatch( contactEvent );
                }
            }

        } else if ( contact.bActorADispatchOverlapEvents ) {
            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            if ( bFirstContact ) {
                contact.ActorA->E_OnBeginOverlap.Dispatch( overlapEvent );
            } else {
                contact.ActorA->E_OnUpdateOverlap.Dispatch( overlapEvent );
            }
        }

        if ( contact.bComponentADispatchContactEvents ) {

            if ( contact.ComponentA->E_OnBeginContact || contact.ComponentA->E_OnUpdateContact ) {
                if ( contact.ComponentA->bGenerateContactPoints ) {
                    GenerateContactPoints( i << 1, contact );

                    contactEvent.Points = ContactPoints.ToPtr();
                    contactEvent.NumPoints = ContactPoints.Length();
                } else {
                    contactEvent.Points = NULL;
                    contactEvent.NumPoints = 0;
                }

                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;

                if ( bFirstContact ) {
                    contact.ComponentA->E_OnBeginContact.Dispatch( contactEvent );
                } else {
                    contact.ComponentA->E_OnUpdateContact.Dispatch( contactEvent );
                }
            }
        } else if ( contact.bComponentADispatchOverlapEvents ) {

            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            if ( bFirstContact ) {
                contact.ComponentA->E_OnBeginOverlap.Dispatch( overlapEvent );
            } else {
                contact.ComponentA->E_OnUpdateOverlap.Dispatch( overlapEvent );
            }
        }

        if ( contact.bActorBDispatchContactEvents ) {

            if ( contact.ActorB->E_OnBeginContact || contact.ActorB->E_OnUpdateContact ) {
                if ( contact.ComponentB->bGenerateContactPoints ) {
                    GenerateContactPoints( ( i << 1 ) + 1, contact );

                    contactEvent.Points = ContactPoints.ToPtr();
                    contactEvent.NumPoints = ContactPoints.Length();
                } else {
                    contactEvent.Points = NULL;
                    contactEvent.NumPoints = 0;
                }                

                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;

                if ( bFirstContact ) {
                    contact.ActorB->E_OnBeginContact.Dispatch( contactEvent );
                } else {
                    contact.ActorB->E_OnUpdateContact.Dispatch( contactEvent );
                }
            }
        } else if ( contact.bActorBDispatchOverlapEvents ) {
            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            if ( bFirstContact ) {
                contact.ActorB->E_OnBeginOverlap.Dispatch( overlapEvent );
            } else {
                contact.ActorB->E_OnUpdateOverlap.Dispatch( overlapEvent );
            }
        }

        if ( contact.bComponentBDispatchContactEvents ) {

            if ( contact.ComponentB->E_OnBeginContact || contact.ComponentB->E_OnUpdateContact ) {
                if ( contact.ComponentB->bGenerateContactPoints ) {
                    GenerateContactPoints( ( i << 1 ) + 1, contact );

                    contactEvent.Points = ContactPoints.ToPtr();
                    contactEvent.NumPoints = ContactPoints.Length();
                } else {
                    contactEvent.Points = NULL;
                    contactEvent.NumPoints = 0;
                }

                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;

                if ( bFirstContact ) {
                    contact.ComponentB->E_OnBeginContact.Dispatch( contactEvent );
                } else {
                    contact.ComponentB->E_OnUpdateContact.Dispatch( contactEvent );
                }
            }
        } else if ( contact.bComponentBDispatchOverlapEvents ) {

            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            if ( bFirstContact ) {
                contact.ComponentB->E_OnBeginOverlap.Dispatch( overlapEvent );
            } else {
                contact.ComponentB->E_OnUpdateOverlap.Dispatch( overlapEvent );
            }
        }
    }

    // Dispatch contact and overlap events (OnEndContact, OnEndOverlap)
    for ( int i = 0; i < prevContacts.Length(); i++ ) {
        FCollisionContact & contact = prevContacts[ i ];

        int hash = contact.Hash();
        bool bHaveContact = false;

        for ( int h = contactHash.First( hash ); h != -1; h = contactHash.Next( h ) ) {
            if ( currentContacts[ h ].ComponentA == contact.ComponentA
                && currentContacts[ h ].ComponentB == contact.ComponentB ) {
                bHaveContact = true;
                break;
            }
        }

        if ( bHaveContact ) {
            continue;
        }

        if ( contact.bActorADispatchContactEvents ) {

            if ( contact.ActorA->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;
                contactEvent.Points = NULL;
                contactEvent.NumPoints = 0;

                contact.ActorA->E_OnEndContact.Dispatch( contactEvent );
            }

        } else if ( contact.bActorADispatchOverlapEvents ) {
            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            contact.ActorA->E_OnEndOverlap.Dispatch( overlapEvent );
        }

        if ( contact.bComponentADispatchContactEvents ) {

            if ( contact.ComponentA->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;
                contactEvent.Points = NULL;
                contactEvent.NumPoints = 0;

                contact.ComponentA->E_OnEndContact.Dispatch( contactEvent );
            }
        } else if ( contact.bComponentADispatchOverlapEvents ) {

            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            contact.ComponentA->E_OnEndOverlap.Dispatch( overlapEvent );
        }

        if ( contact.bActorBDispatchContactEvents ) {

            if ( contact.ActorB->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;
                contactEvent.Points = NULL;
                contactEvent.NumPoints = 0;

                contact.ActorB->E_OnEndContact.Dispatch( contactEvent  );
            }
        } else if ( contact.bActorBDispatchOverlapEvents ) {
            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            contact.ActorB->E_OnEndOverlap.Dispatch( overlapEvent );
        }

        if ( contact.bComponentBDispatchContactEvents ) {

            if ( contact.ComponentB->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;
                contactEvent.Points = NULL;
                contactEvent.NumPoints = 0;

                contact.ComponentB->E_OnEndContact.Dispatch( contactEvent );
            }
        } else if ( contact.bComponentBDispatchOverlapEvents ) {

            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            contact.ComponentB->E_OnEndOverlap.Dispatch( overlapEvent );
        }
    }
}

void FWorld::SimulatePhysics( float _TimeStep ) {
    if ( GGameMaster.IsGamePaused() ) {
        return;
    }

    const float FixedTimeStep = 1.0f / GGameMaster.PhysicsHertz;
    int numSimulationSteps = int( _TimeStep * GGameMaster.PhysicsHertz ) + 1.0f;
    //numSimulationSteps = FMath::Min( numSimulationSteps, MAX_SIMULATION_STEPS );

    btContactSolverInfo & contactSolverInfo = PhysicsWorld->getSolverInfo();
    contactSolverInfo.m_numIterations = GGameMaster.NumContactSolverIterations;
    if ( contactSolverInfo.m_numIterations < 1 ) {
        contactSolverInfo.m_numIterations = 1;
    }
    else if ( contactSolverInfo.m_numIterations > 256 ) {
        contactSolverInfo.m_numIterations = 256;
    }
    contactSolverInfo.m_splitImpulse = GGameMaster.bContactSolverSplitImpulse;

    if ( bGravityDirty ) {
        PhysicsWorld->setGravity( btVectorToFloat3( GravityVector ) );
        bGravityDirty = false;
    }

    bDuringPhysicsUpdate = true;

    if ( GGameMaster.bEnablePhysicsInterpolation ) {
        TimeAccumulation = 0;
        PhysicsWorld->stepSimulation( _TimeStep, numSimulationSteps, FixedTimeStep );
    } else {
        TimeAccumulation += _TimeStep;
        while ( TimeAccumulation >= FixedTimeStep && numSimulationSteps > 0 ) {
            PhysicsWorld->stepSimulation( FixedTimeStep, 0, FixedTimeStep );
            TimeAccumulation -= FixedTimeStep;
            --numSimulationSteps;
        }
    }

    bDuringPhysicsUpdate = false;

    SoftBodyWorldInfo->m_sparsesdf.GarbageCollect();
}

static bool CompareDistance( FRaycastResult const & A, FRaycastResult const & B ) {
    return A.Distance < B.Distance;
}

AN_FORCEINLINE static unsigned short ClampUnsignedShort( int _Value ) {
    if ( _Value < 0 ) return 0;
    if ( _Value > 0xffff ) return 0xffff;
    return _Value;
}

bool FWorld::Raycast( TPodArray< FRaycastResult > & _Result, RayF const & _Ray, float _Dist, int _CollisionMask ) const {
    btCollisionWorld::AllHitsRayResultCallback hitResult( btVectorToFloat3( _Ray.Start ), btVectorToFloat3( _Ray.Start + FMath::Clamp< float >( _Dist, 0, MAX_RAYCAST_DISTANCE ) * _Ray.Dir ) );
    hitResult.m_collisionFilterGroup = ( short )0xffff;
    hitResult.m_collisionFilterMask = ClampUnsignedShort( _CollisionMask );
    
    PhysicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    _Result.Resize( hitResult.m_collisionObjects.size() );

    for ( int i = 0; i < hitResult.m_collisionObjects.size(); i++ ) {
        FRaycastResult & result = _Result[ i ];
        result.Body = static_cast< FPhysicalBody * >( hitResult.m_collisionObjects[ i ]->getUserPointer() );
        result.Position = btVectorToFloat3( hitResult.m_hitPointWorld[ i ] );
        result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld[ i ] );
        result.Distance = ( result.Position - _Ray.Start ).Length();
        result.HitFraction = hitResult.m_closestHitFraction;
    }

    StdSort( _Result.Begin(), _Result.End(), CompareDistance );

    return !_Result.IsEmpty();
}

bool FWorld::RaycastClosest( FRaycastResult & _Result, RayF const & _Ray, float _Dist, int _CollisionMask ) const {
    btCollisionWorld::ClosestRayResultCallback hitResult( btVectorToFloat3( _Ray.Start ), btVectorToFloat3( _Ray.Start + FMath::Clamp< float >( _Dist, 0, MAX_RAYCAST_DISTANCE ) * _Ray.Dir ) );
    hitResult.m_collisionFilterGroup = ( short )0xffff;
    hitResult.m_collisionFilterMask = ClampUnsignedShort( _CollisionMask );

    PhysicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< FPhysicalBody * >( hitResult.m_collisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = ( _Result.Position - _Ray.Start ).Length();
    _Result.HitFraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::RaycastSphere( FRaycastResult & _Result, float _Radius, RayF const & _Ray, float _Dist, int _CollisionMask ) const {
    btSphereShape shape( _Radius );
    float dist = FMath::Clamp< float >( _Dist, 0, MAX_RAYCAST_DISTANCE );
    Float3 rayEnd = _Ray.Start + dist * _Ray.Dir;

    btCollisionWorld::ClosestConvexResultCallback hitResult( btVectorToFloat3( _Ray.Start ), btVectorToFloat3( rayEnd ) );
    hitResult.m_collisionFilterGroup = ( short )0xffff;
    hitResult.m_collisionFilterMask = ClampUnsignedShort( _CollisionMask );

    PhysicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexFromWorld ),
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexToWorld ), hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< FPhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    //_Result.Distance = hitResult.m_closestHitFraction * ( rayEnd - _Ray.Start ).Length();
    _Result.Distance = hitResult.m_closestHitFraction * dist;
    _Result.HitFraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::RaycastConvex( FRaycastResult & _Result, FConvexCast const & _ConvexCast ) const {

    if ( !_ConvexCast.CollisionBody->IsConvex() ) {
        GLogger.Printf( "FWorld::RaycastConvex: non-convex collision body for convex raycast\n" );
        _Result.Clear();
        return false;
    }

    btCollisionShape * shape = _ConvexCast.CollisionBody->Create();

    AN_Assert( shape->isConvex() );

    Float3x4 startTransform, endTransform;

    startTransform.Compose( _ConvexCast.StartPosition, _ConvexCast.StartRotation.ToMatrix(), _ConvexCast.Scale );
    endTransform.Compose( _ConvexCast.EndPosition, _ConvexCast.EndRotation.ToMatrix(), _ConvexCast.Scale );

    Float3 startPos = startTransform * _ConvexCast.CollisionBody->Position;
    Float3 endPos = endTransform * _ConvexCast.CollisionBody->Position;
    Quat startRot = _ConvexCast.StartRotation * _ConvexCast.CollisionBody->Rotation;
    Quat endRot = _ConvexCast.EndRotation * _ConvexCast.CollisionBody->Rotation;

    btCollisionWorld::ClosestConvexResultCallback hitResult( btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );
    hitResult.m_collisionFilterGroup = ( short )0xffff;
    hitResult.m_collisionFilterMask = ClampUnsignedShort( _ConvexCast.CollisionMask );

    PhysicsWorld->convexSweepTest( static_cast< btConvexShape * >( shape ),
        btTransform( btQuaternionToQuat( startRot ), hitResult.m_convexFromWorld ),
        btTransform( btQuaternionToQuat( endRot ), hitResult.m_convexToWorld ), hitResult );

    b3Destroy( shape );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< FPhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.HitFraction = hitResult.m_closestHitFraction;
    return true;
}

struct FQueryPhysicalBodiesCallback : public btCollisionWorld::ContactResultCallback {
    FQueryPhysicalBodiesCallback( TPodArray< FPhysicalBody * > & _Result, int _CollisionMask )
        : Result( _Result )
        , CollisionMask( _CollisionMask )
    {
        _Result.Clear();
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        FPhysicalBody * body;
        
        body = reinterpret_cast< FPhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body ) == Result.End() && ( body->CollisionGroup & CollisionMask ) ) {
            Result.Append( body );
        }

        body = reinterpret_cast< FPhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body ) == Result.End() && ( body->CollisionGroup & CollisionMask ) ) {
            Result.Append( body );
        }

        return 0.0f;
    }

    TPodArray< FPhysicalBody * > & Result;
    int CollisionMask;
};

struct FQueryActorsCallback : public btCollisionWorld::ContactResultCallback {
    FQueryActorsCallback( TPodArray< FActor * > & _Result, int _CollisionMask )
        : Result( _Result )
        , CollisionMask( _CollisionMask )
    {
        _Result.Clear();
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        FPhysicalBody * body;

        body = reinterpret_cast< FPhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body->GetParentActor() ) == Result.End() && ( body->CollisionGroup & CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        body = reinterpret_cast< FPhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body->GetParentActor() ) == Result.End() && ( body->CollisionGroup & CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        return 0.0f;
    }

    TPodArray< FActor * > & Result;
    int CollisionMask;
};

void FWorld::QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, float _Radius, int _CollisionMask ) const {
    FQueryPhysicalBodiesCallback callback( _Result, _CollisionMask );
    btSphereShape shape( _Radius );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryActors( TPodArray< FActor * > & _Result, Float3 const & _Position, float _Radius, int _CollisionMask ) const {
    FQueryActorsCallback callback( _Result, _CollisionMask );
    btSphereShape shape( _Radius );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, int _CollisionMask ) const {
    FQueryPhysicalBodiesCallback callback( _Result, _CollisionMask );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryActors( TPodArray< FActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, int _CollisionMask ) const {
    FQueryActorsCallback callback( _Result, _CollisionMask );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, BvAxisAlignedBox const & _BoundingBox, int _CollisionMask ) const {
    QueryPhysicalBodies( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _CollisionMask );
}

void FWorld::QueryActors( TPodArray< FActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, int _CollisionMask ) const {
    QueryActors( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _CollisionMask );
}

void FWorld::ApplyRadialDamage( float _DamageAmount, Float3 const & _Position, float _Radius, int _CollisionMask ) {
    TPodArray< FActor * > damagedActors;
    QueryActors( damagedActors, _Position, _Radius, _CollisionMask );
    for ( FActor * damagedActor : damagedActors ) {
        damagedActor->ApplyDamage( _DamageAmount, _Position, nullptr );
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

        if ( actor->RootComponent ) {
            _DebugDraw->SetDepthTest( false );
            _DebugDraw->DrawAxis( actor->RootComponent->GetWorldTransformMatrix(), false );
        }
    }

    _DebugDraw->SetDepthTest( false );
    PhysicsDebugDraw.DD = _DebugDraw;

    int Mode = 0;
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_COLLISION_SHAPES_WIREFRANE ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawWireframe;
    //}
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_COLLISION_SHAPE_AABBs ) {
    //    Mode |= FPhysicsDebugDraw::DBG_DrawAabb;
    //}
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_CONTACT_POINTS ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawContactPoints;
    //}
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_CONSTRAINTS ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawConstraints;
    //}
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_CONSTRAINT_LIMITS ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawConstraintLimits;
    //}
    //if ( _DebugDrawFlags & EDebugDrawFlags::DRAW_COLLISION_SHAPE_NORMALS ) {
    //    Mode |= FPhysicsDebugDraw::DBG_DrawNormals;
    //}

    PhysicsDebugDraw.setDebugMode( Mode );
    PhysicsWorld->debugDrawWorld();

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
