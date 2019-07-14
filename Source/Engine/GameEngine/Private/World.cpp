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

#include <Engine/GameEngine/Public/World.h>
#include <Engine/GameEngine/Public/Actor.h>
#include <Engine/GameEngine/Public/Pawn.h>
#include <Engine/GameEngine/Public/ActorComponent.h>
#include <Engine/GameEngine/Public/SceneComponent.h>
#include <Engine/GameEngine/Public/MeshComponent.h>
#include <Engine/GameEngine/Public/SkeletalAnimation.h>
#include <Engine/GameEngine/Public/GameEngine.h>
#include <Engine/GameEngine/Public/Timer.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Public/BV/BvIntersect.h>

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
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

AN_FORCEINLINE static unsigned short ClampUnsignedShort( int _Value ) {
    if ( _Value < 0 ) return 0;
    if ( _Value > 0xffff ) return 0xffff;
    return _Value;
}

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

struct FCollisionFilterCallback : public btOverlapFilterCallback {

    // Return true when pairs need collision
    bool needBroadphaseCollision( btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1 ) const override {

        if ( ( proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask )
             && ( proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask ) )
        {
            // FIXME: can we safe cast m_clientObject to btCollisionObject?

            btCollisionObject const * colObj0 = reinterpret_cast< btCollisionObject const * >( proxy0->m_clientObject );
            btCollisionObject const * colObj1 = reinterpret_cast< btCollisionObject const * >( proxy1->m_clientObject );

            FPhysicalBody const * body0 = ( FPhysicalBody const * )colObj0->getUserPointer();
            FPhysicalBody const * body1 = ( FPhysicalBody const * )colObj1->getUserPointer();

            if ( !body0 || !body1 ) {
                GLogger.Printf( "Null body\n" );
                return true;
            }

//            FPhysicalBody * body0 = static_cast< FPhysicalBody * >( static_cast< btCollisionObject * >( proxy0->m_clientObject )->getUserPointer() );
//            FPhysicalBody * body1 = static_cast< FPhysicalBody * >( static_cast< btCollisionObject * >( proxy1->m_clientObject )->getUserPointer() );

            if ( body0->CollisionIgnoreActors.Find( body1->GetParentActor() ) != body0->CollisionIgnoreActors.End() ) {
                return false;
            }

            if ( body1->CollisionIgnoreActors.Find( body0->GetParentActor() ) != body1->CollisionIgnoreActors.End() ) {
                return false;
            }

            return true;
        }

        return false;
    }
};

static FCollisionFilterCallback CollisionFilterCallback;

#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

extern ContactAddedCallback gContactAddedCallback;

static bool CustomMaterialCombinerCallback( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0,
                                            int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 )
{
    const int normalAdjustFlags = 0
    //| BT_TRIANGLE_CONVEX_BACKFACE_MODE
    //| BT_TRIANGLE_CONCAVE_DOUBLE_SIDED //double sided options are experimental, single sided is recommended
    //| BT_TRIANGLE_CONVEX_DOUBLE_SIDED
        ;

    btAdjustInternalEdgeContacts( cp, colObj1Wrap, colObj0Wrap, partId1, index1, normalAdjustFlags );

    cp.m_combinedFriction = btManifoldResult::calculateCombinedFriction( colObj0Wrap->getCollisionObject(), colObj1Wrap->getCollisionObject() );
    cp.m_combinedRestitution = btManifoldResult::calculateCombinedRestitution( colObj0Wrap->getCollisionObject(), colObj1Wrap->getCollisionObject() );

    return true;
}

FWorld::FWorld() {
    PersistentLevel = NewObject< FLevel >();
    PersistentLevel->AddRef();
    PersistentLevel->OwnerWorld = this;
    PersistentLevel->bIsPersistent = true;
    PersistentLevel->IndexInArrayOfLevels = ArrayOfLevels.Length();
    ArrayOfLevels.Append( PersistentLevel );

    GravityVector = Float3( 0.0f, -9.81f, 0.0f );

    gContactAddedCallback = CustomMaterialCombinerCallback;

#if 0
    PhysicsBroadphase = b3New( btDbvtBroadphase ); // TODO: AxisSweep3Internal?
#else
    PhysicsBroadphase = b3New( btAxisSweep3, btVector3(-10000,-10000,-10000), btVector3(10000,10000,10000) );
#endif
    //CollisionConfiguration = b3New( btDefaultCollisionConfiguration );
    CollisionConfiguration = b3New( btSoftBodyRigidBodyCollisionConfiguration );
    CollisionDispatcher = b3New( btCollisionDispatcher, CollisionConfiguration );
    // TODO: remove this if we don't use gimpact
    btGImpactCollisionAlgorithm::registerAlgorithm( CollisionDispatcher );
    ConstraintSolver = b3New( btSequentialImpulseConstraintSolver );
    PhysicsWorld = b3New( btSoftRigidDynamicsWorld, CollisionDispatcher, PhysicsBroadphase, ConstraintSolver, CollisionConfiguration, /* SoftBodySolver */ 0 );
    PhysicsWorld->setGravity( btVectorToFloat3( GravityVector ) );
    PhysicsWorld->getDispatchInfo().m_useContinuous = true;
    PhysicsWorld->getSolverInfo().m_splitImpulse = bContactSolverSplitImpulse;
    PhysicsWorld->getSolverInfo().m_numIterations = NumContactSolverIterations;
    PhysicsWorld->getPairCache()->setOverlapFilterCallback( &CollisionFilterCallback );
    PhysicsWorld->setDebugDrawer( &PhysicsDebugDraw );
    PhysicsWorld->setInternalTickCallback( OnPrePhysics, static_cast< void * >( this ), true );
    PhysicsWorld->setInternalTickCallback( OnPostPhysics, static_cast< void * >( this ), false );
    //PhysicsWorld->setSynchronizeAllMotionStates( true ); // TODO: check how it works

    SoftBodyWorldInfo = &PhysicsWorld->getWorldInfo();
    SoftBodyWorldInfo->m_dispatcher = CollisionDispatcher;
    SoftBodyWorldInfo->m_broadphase = PhysicsBroadphase;
    SoftBodyWorldInfo->m_gravity = btVectorToFloat3( GravityVector );
    SoftBodyWorldInfo->air_density = ( btScalar )1.2;
    SoftBodyWorldInfo->water_density = 0;
    SoftBodyWorldInfo->water_offset = 0;
    SoftBodyWorldInfo->water_normal = btVector3( 0, 0, 0 );
    SoftBodyWorldInfo->m_sparsesdf.Initialize();
}

void FWorld::SetPaused( bool _Paused ) {
    bPauseRequest = _Paused;
    bUnpauseRequest = !_Paused;
}

bool FWorld::IsPaused() const {
    return bPaused;
}

void FWorld::ResetGameplayTimer() {
    bResetGameplayTimer = true;
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
    NextPendingKillWorld = GGameEngine.PendingKillWorlds;
    GGameEngine.PendingKillWorlds = this;

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

    if ( _SpawnParameters.Instigator ) {
        actor->Instigator = _SpawnParameters.Instigator;
        actor->Instigator->AddRef();
    }

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

    //UpdatePauseStatus();
    if ( bPauseRequest ) {
        bPauseRequest = false;
        bPaused = true;
        GLogger.Printf( "Game paused\n" );
    } else if ( bUnpauseRequest ) {
        bUnpauseRequest = false;
        bPaused = false;
        GLogger.Printf( "Game unpaused\n" );
    }

    //UpdateWorldTime();
    GameRunningTimeMicro = GameRunningTimeMicroAfterTick;
    GameplayTimeMicro = GameplayTimeMicroAfterTick;

    //UpdateTimers();
    // Tick all timers. TODO: move timer tick to PrePhysicsTick?
    for ( FTimer * timer = TimerList ; timer ; timer = timer->Next ) {
        timer->Tick( this, _TimeStep );
    }

    //UpdateActors();
    // Tick actors
    for ( FActor * actor : Actors ) {
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

    SimulatePhysics( _TimeStep );

    // Update levels
    for ( FLevel * level : ArrayOfLevels ) {
        level->Tick( _TimeStep );
    }

    KickoffPendingKillObjects();

    uint64_t frameDuration = (double)_TimeStep * 1000000;
    GameRunningTimeMicroAfterTick += frameDuration;
}

void FWorld::AddPhysicalBody( FPhysicalBody * _PhysicalBody ) {
    if ( !IntrusiveIsInList( _PhysicalBody, NextMarked, PrevMarked, PendingAddToWorldHead, PendingAddToWorldTail ) ) {
        IntrusiveAddToList( _PhysicalBody, NextMarked, PrevMarked, PendingAddToWorldHead, PendingAddToWorldTail );
    }
}

void FWorld::RemovePhysicalBody( FPhysicalBody * _PhysicalBody ) {
    if ( IntrusiveIsInList( _PhysicalBody, NextMarked, PrevMarked, PendingAddToWorldHead, PendingAddToWorldTail ) ) {
        IntrusiveRemoveFromList( _PhysicalBody, NextMarked, PrevMarked, PendingAddToWorldHead, PendingAddToWorldTail );
    }
}

void FWorld::OnPrePhysics( float _TimeStep ) {

    GameplayTimeMicro = GameplayTimeMicroAfterTick;

    // Add physical bodies
    FPhysicalBody * next;
    for ( FPhysicalBody * body = PendingAddToWorldHead ; body ; body = next ) {
        next = body->NextMarked;

        body->NextMarked = body->PrevMarked = nullptr;

        if ( body->RigidBody ) {
            AN_Assert( !body->bInWorld );
            PhysicsWorld->addRigidBody( body->RigidBody, ClampUnsignedShort( body->CollisionGroup ), ClampUnsignedShort( body->CollisionMask ) );
            body->bInWorld = true;
        }
    }
    PendingAddToWorldHead = PendingAddToWorldTail = nullptr;

    // Tick actors
    for ( FActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( actor->bCanEverTick && actor->bTickPrePhysics ) {
            //actor->TickComponentsPrePhysics( _TimeStep );
            actor->TickPrePhysics( _TimeStep );
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

    for ( FActor * actor : Actors ) {
        if ( actor->IsPendingKill() ) {
            continue;
        }

        if ( actor->bCanEverTick && actor->bTickPostPhysics ) {

            //actor->TickComponentsPostPhysics( _TimeStep );
            actor->TickPostPhysics( _TimeStep );

        }

        // Update actor life span
        actor->LifeTime += _TimeStep;

        if ( actor->LifeSpan > 0.0f ) {
            actor->LifeSpan -= _TimeStep;

            if ( actor->LifeSpan < 0.0f ) {
                actor->Destroy();
            }
        }
    }

    //WorldLocalTime += _TimeStep;

    FixedTickNumber++;

    if ( bResetGameplayTimer ) {
        bResetGameplayTimer = false;
        GameplayTimeMicroAfterTick = 0;
    } else {
        GameplayTimeMicroAfterTick += (double)_TimeStep * 1000000.0;
    }
}

void FWorld::DispatchContactAndOverlapEvents() {

#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4456 )
#endif

    int curTickNumber = FixedTickNumber & 1;
    int prevTickNumber = ( FixedTickNumber + 1 ) & 1;

    TPodArray< FCollisionContact > & currentContacts = CollisionContacts[ curTickNumber ];
    TPodArray< FCollisionContact > & prevContacts = CollisionContacts[ prevTickNumber ];

    THash<> & contactHash = ContactHash[ curTickNumber ];
    THash<> & prevContactHash = ContactHash[ prevTickNumber ];

    FCollisionContact contact;

    FOverlapEvent overlapEvent;
    FContactEvent contactEvent;

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

        if ( !objectA || !objectB ) {
            // ghost object
            continue;
        }

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
    if ( bPaused ) {
        return;
    }

    const float FixedTimeStep = 1.0f / PhysicsHertz;

    int numSimulationSteps = floor( _TimeStep * PhysicsHertz ) + 1.0f;
    //numSimulationSteps = FMath::Min( numSimulationSteps, MAX_SIMULATION_STEPS );

    btContactSolverInfo & contactSolverInfo = PhysicsWorld->getSolverInfo();
    contactSolverInfo.m_numIterations = FMath::Clamp( NumContactSolverIterations, 1, 256 );
    contactSolverInfo.m_splitImpulse = bContactSolverSplitImpulse;

    if ( bGravityDirty ) {
        PhysicsWorld->setGravity( btVectorToFloat3( GravityVector ) );
        bGravityDirty = false;
    }

    bDuringPhysicsUpdate = true;

    if ( bEnablePhysicsInterpolation ) {
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

bool FWorld::Raycast( FWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter ) {
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;
    Float3 rayStartLocal;
    Float3 rayEndLocal;
    Float3 rayDirLocal;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    for ( FMeshComponent * mesh = MeshList ; mesh ; mesh = mesh->Next ) {

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        FIndexedMesh * resource = mesh->GetMesh();
        if ( !resource ) {
            continue;
        }

        if ( !FMath::Intersects( mesh->GetWorldBounds(), _RayStart, invRayDir ) ) {
            continue;
        }

        Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

        // transform ray to object space
        rayStartLocal = transformInverse * _RayStart;
        rayEndLocal = transformInverse * _RayEnd;
        rayDirLocal = rayEndLocal - rayStartLocal;

        float hitDistanceLocal = rayDirLocal.Length();
        if ( hitDistanceLocal < 0.0001f ) {
            continue;
        }

        rayDirLocal /= hitDistanceLocal;

        int firstHit = _Result.Hits.Length();

        if ( resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, _Result.Hits ) ) {

            FWorldRaycastEntity & raycastEntity = _Result.Entities.Append();

            raycastEntity.Object = mesh;
            raycastEntity.FirstHit = firstHit;
            raycastEntity.LastHit = _Result.Hits.Length();
            raycastEntity.ClosestHit = raycastEntity.FirstHit;

            // Convert hits to worldspace and find closest hit

            Float3x4 const & transform = mesh->GetWorldTransformMatrix();
            Float3x3 normalMatrix;

            transform.DecomposeNormalMatrix( normalMatrix );

            for ( int i = raycastEntity.FirstHit ; i < raycastEntity.LastHit ; i++ ) {
                FTriangleHitResult & hitResult = _Result.Hits[i];

                hitResult.HitLocation = transform * hitResult.HitLocation;
                hitResult.HitNormal = ( normalMatrix * hitResult.HitNormal ).Normalized();
                hitResult.HitDistance = (hitResult.HitLocation - _RayStart).Length();

                if ( hitResult.HitDistance < _Result.Hits[ raycastEntity.ClosestHit ].HitDistance ) {
                    raycastEntity.ClosestHit = i;
                }
            }
        }
    }

    if ( _Result.Entities.IsEmpty() ) {
        return false;
    }

    if ( _Filter->bSortByDistance ) {
        _Result.Sort();
    }

    return true;
}

bool FWorld::RaycastAABB( TPodArray< FBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter ) {
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float boxMin, boxMax;

    for ( FMeshComponent * mesh = MeshList ; mesh ; mesh = mesh->Next ) {

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        FIndexedMesh * resource = mesh->GetMesh();
        if ( !resource ) {
            continue;
        }

        if ( !FMath::Intersects( mesh->GetWorldBounds(), _RayStart, invRayDir, boxMin, boxMax ) ) {
            continue;
        }

        FBoxHitResult & hitResult = _Result.Append();

        hitResult.Object = mesh;
        hitResult.HitLocationMin = _RayStart + rayDir * boxMin;
        hitResult.HitLocationMax = _RayStart + rayDir * boxMax;
        hitResult.HitDistanceMin = boxMin;
        hitResult.HitDistanceMax = boxMax;
    }

    if ( _Result.IsEmpty() ) {
        return false;
    }

    if ( _Filter->bSortByDistance ) {
        struct FSortHit {
            bool operator() ( FBoxHitResult const & _A, FBoxHitResult const & _B ) {
                return ( _A.HitDistanceMin < _B.HitDistanceMin );
            }
        } SortHit;

        StdSort( _Result.ToPtr(), _Result.ToPtr() + _Result.Length(), SortHit );
    }

    return true;
}

bool FWorld::RaycastClosest( FWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter ) {
    FMeshComponent * hitObject = nullptr;
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;
    Float3 rayStartLocal;
    Float3 rayEndLocal;
    Float3 rayDirLocal;
    Float3 hitLocation;
    Float2 hitUV;
    float hitDistance;
    unsigned int indices[3];
    TRefHolder< FMaterialInstance > material;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float boxMin, boxMax;

    hitDistance = rayLength;
    hitLocation = _RayEnd;

    for ( FMeshComponent * mesh = MeshList ; mesh ; mesh = mesh->Next ) {

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        FIndexedMesh * resource = mesh->GetMesh();
        if ( !resource ) {
            continue;
        }

        if ( !FMath::Intersects( mesh->GetWorldBounds(), _RayStart, invRayDir, boxMin, boxMax ) ) {
            continue;
        }

        if ( boxMin > hitDistance ) {
            continue;
        }

        Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

        // transform ray to object space
        rayStartLocal = transformInverse * _RayStart;
        rayEndLocal = transformInverse * hitLocation;
        rayDirLocal = rayEndLocal - rayStartLocal;

        float hitDistanceLocal = rayDirLocal.Length();
        if ( hitDistanceLocal < 0.0001f ) {
            continue;
        }

        rayDirLocal /= hitDistanceLocal;

        if ( resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, hitLocation, hitUV, hitDistance, indices, material ) ) {
            hitObject = mesh;

            // transform hit location to world space
            hitLocation = hitObject->GetWorldTransformMatrix() * hitLocation;

            // recalc hit distance in world space
            hitDistance = (hitLocation - _RayStart).Length();

            // hit is close enough to stop ray casting?
            if ( hitDistance < 0.0001f ) {
                break;
            }
        }
    }

    if ( !hitObject ) {
        return false;
    }

    FIndexedMesh * resource = hitObject->GetMesh();
    FMeshVertex * vertices = resource->GetVertices();
    Float3x4 const & transform = hitObject->GetWorldTransformMatrix();

    Float3 const & v0 = vertices[indices[0]].Position;
    Float3 const & v1 = vertices[indices[1]].Position;
    Float3 const & v2 = vertices[indices[2]].Position;

    // calc triangle vertices
    _Result.Vertices[0] = transform * v0;
    _Result.Vertices[1] = transform * v1;
    _Result.Vertices[2] = transform * v2;

    // calc hit normal
#if 1
    _Result.Normal = (_Result.Vertices[1]-_Result.Vertices[0]).Cross( _Result.Vertices[2]-_Result.Vertices[0] ).Normalized();
#else
    Float3x3 normalMat;
    transform.DecomposeNormalMatrix( normalMat );
    _Result.Normal = (normalMat * (v1-v0).Cross( v2-v0 )).Normalized();
#endif

    _Result.Object = hitObject;
    _Result.Position = hitLocation;
    _Result.Distance = hitDistance;
    _Result.Fraction = hitDistance / rayLength;
    _Result.TriangleIndices[0] = indices[0];
    _Result.TriangleIndices[1] = indices[1];
    _Result.TriangleIndices[2] = indices[2];
    _Result.Material = material;

    // calc hit UV
    Float2 const & uv0 = vertices[indices[0]].TexCoord;
    Float2 const & uv1 = vertices[indices[1]].TexCoord;
    Float2 const & uv2 = vertices[indices[2]].TexCoord;
    _Result.Texcoord = uv0 * hitUV[0] + uv1 * hitUV[1] + uv2 * ( 1.0f - hitUV[0] - hitUV[1] );
    _Result.UV = hitUV;

    return true;
}

bool FWorld::RaycastClosestAABB( FBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FWorldRaycastFilter * _Filter ) {
    FMeshComponent * hitObject = nullptr;
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;
    float hitDistanceMin;
    float hitDistanceMax;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float boxMin, boxMax;

    hitDistanceMin = rayLength;
    hitDistanceMax = rayLength;

    for ( FMeshComponent * mesh = MeshList ; mesh ; mesh = mesh->Next ) {

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        FIndexedMesh * resource = mesh->GetMesh();
        if ( !resource ) {
            continue;
        }

        if ( !FMath::Intersects( mesh->GetWorldBounds(), _RayStart, invRayDir, boxMin, boxMax ) ) {
            continue;
        }

        if ( boxMin > hitDistanceMin ) {
            continue;
        }

        hitObject = mesh;
        hitDistanceMin = boxMin;
        hitDistanceMax = boxMax;

        // hit is close enough to stop ray casting?
        if ( hitDistanceMin < 0.0001f ) {
            break;
        }
    }

    if ( !hitObject ) {
        return false;
    }

    _Result.Object = hitObject;
    _Result.HitLocationMin = _RayStart + rayDir * hitDistanceMin;
    _Result.HitLocationMax = _RayStart + rayDir * hitDistanceMax;
    _Result.HitDistanceMin = hitDistanceMin;
    _Result.HitDistanceMax = hitDistanceMax;
    //_Result.HitFractionMin = hitDistanceMin / rayLength;
    //_Result.HitFractionMax = hitDistanceMax / rayLength;

    return true;
}

static bool CompareDistance( FTraceResult const & A, FTraceResult const & B ) {
    return A.Distance < B.Distance;
}

static bool FindCollisionActor( FCollisionQueryFilter const & _QueryFilter, FActor * _Actor ) {
    for ( int i = 0; i < _QueryFilter.ActorsCount; i++ ) {
        if ( _Actor == _QueryFilter.IgnoreActors[ i ] ) {
            return true;
        }
    }
    return false;
}

static bool FindCollisionBody( FCollisionQueryFilter const & _QueryFilter, FPhysicalBody * _Body ) {
    for ( int i = 0; i < _QueryFilter.BodiesCount; i++ ) {
        if ( _Body == _QueryFilter.IgnoreBodies[ i ] ) {
            return true;
        }
    }
    return false;
}

AN_FORCEINLINE bool NeedsCollision( FCollisionQueryFilter const & _QueryFilter, btBroadphaseProxy * _Proxy ) {
    FPhysicalBody * body = static_cast< FPhysicalBody * >( static_cast< btCollisionObject * >( _Proxy->m_clientObject )->getUserPointer() );

    if ( body ) {
        if ( FindCollisionActor( _QueryFilter, body->GetParentActor() ) ) {
            return false;
        }

        if ( FindCollisionBody( _QueryFilter, body ) ) {
            return false;
        }
    } else {
        // ghost object
    }

    return ( _Proxy->m_collisionFilterGroup & _QueryFilter.CollisionMask ) && _Proxy->m_collisionFilterMask;
}

static FCollisionQueryFilter DefaultCollisionQueryFilter;

struct FTraceRayResultCallback : btCollisionWorld::AllHitsRayResultCallback {

    FTraceRayResultCallback( FCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : btCollisionWorld::AllHitsRayResultCallback( rayFromWorld, rayToWorld )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );

        m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

        //    enum EFlags
        //    {
        //       kF_None                 = 0,
        //       kF_FilterBackfaces      = 1 << 0,
        //       kF_KeepUnflippedNormal  = 1 << 1,   // Prevents returned face normal getting flipped when a ray hits a back-facing triangle
        //         ///SubSimplexConvexCastRaytest is the default, even if kF_None is set.
        //       kF_UseSubSimplexConvexCastRaytest = 1 << 2,   // Uses an approximate but faster ray versus convex intersection algorithm
        //       kF_UseGjkConvexCastRaytest = 1 << 3,
        //       kF_Terminator        = 0xFFFFFFFF
        //    };
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    FCollisionQueryFilter const & QueryFilter;
};

struct FTraceClosestRayResultCallback : btCollisionWorld::ClosestRayResultCallback {

    FTraceClosestRayResultCallback( FCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : ClosestRayResultCallback( rayFromWorld, rayToWorld )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );

        m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    FCollisionQueryFilter const & QueryFilter;
};

struct FTraceClosestConvexResultCallback : btCollisionWorld::ClosestConvexResultCallback {

    FTraceClosestConvexResultCallback( FCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : ClosestConvexResultCallback( rayFromWorld, rayToWorld )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    FCollisionQueryFilter const & QueryFilter;
};

bool FWorld::Trace( TPodArray< FTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter ) const {
    FTraceRayResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    PhysicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    _Result.Resize( hitResult.m_collisionObjects.size() );

    for ( int i = 0; i < hitResult.m_collisionObjects.size(); i++ ) {
        FTraceResult & result = _Result[ i ];
        result.Body = static_cast< FPhysicalBody * >( hitResult.m_collisionObjects[ i ]->getUserPointer() );
        result.Position = btVectorToFloat3( hitResult.m_hitPointWorld[ i ] );
        result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld[ i ] );
        result.Distance = ( result.Position - _RayStart ).Length();
        result.Fraction = hitResult.m_closestHitFraction;
    }

    //if ( bForcePerTriangleCheck ) {
        // TODO: check intersection with triangles!
    //}

    if ( !_QueryFilter ) {
        _QueryFilter = &DefaultCollisionQueryFilter;
    }

    if ( _QueryFilter->bSortByDistance ) {
        StdSort( _Result.Begin(), _Result.End(), CompareDistance );
    }

    return !_Result.IsEmpty();
}

bool FWorld::TraceClosest( FTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter ) const {
    FTraceClosestRayResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    PhysicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< FPhysicalBody * >( hitResult.m_collisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = ( _Result.Position - _RayStart ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::TraceSphere( FTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter ) const {
    FTraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );

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
    _Result.Distance = hitResult.m_closestHitFraction * ( _RayEnd - _RayStart ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::TraceBox( FTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter ) const {
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    FTraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    btBoxShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

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
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::TraceCylinder( FTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter ) const {
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    FTraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    btCylinderShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

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
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::TraceCapsule( FTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, FCollisionQueryFilter const * _QueryFilter ) const {
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    FTraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    float radius = FMath::Max( halfExtents[0], halfExtents[2] );

    btCapsuleShape shape( radius, (halfExtents[1]-radius)*2.0f );
    shape.setMargin( 0.0f );

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
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool FWorld::TraceConvex( FTraceResult & _Result, FConvexSweepTest const & _SweepTest ) const {

    if ( !_SweepTest.CollisionBody->IsConvex() ) {
        GLogger.Printf( "FWorld::TraceConvex: non-convex collision body for convex trace\n" );
        _Result.Clear();
        return false;
    }

    btCollisionShape * shape = _SweepTest.CollisionBody->Create();
    shape->setMargin( _SweepTest.CollisionBody->Margin );

    AN_Assert( shape->isConvex() );

    Float3x4 startTransform, endTransform;

    startTransform.Compose( _SweepTest.StartPosition, _SweepTest.StartRotation.ToMatrix(), _SweepTest.Scale );
    endTransform.Compose( _SweepTest.EndPosition, _SweepTest.EndRotation.ToMatrix(), _SweepTest.Scale );

    Float3 startPos = startTransform * _SweepTest.CollisionBody->Position;
    Float3 endPos = endTransform * _SweepTest.CollisionBody->Position;
    Quat startRot = _SweepTest.StartRotation * _SweepTest.CollisionBody->Rotation;
    Quat endRot = _SweepTest.EndRotation * _SweepTest.CollisionBody->Rotation;

    FTraceClosestConvexResultCallback hitResult( &_SweepTest.QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

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
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

struct FQueryPhysicalBodiesCallback : public btCollisionWorld::ContactResultCallback {
    FQueryPhysicalBodiesCallback( TPodArray< FPhysicalBody * > & _Result, FCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        FPhysicalBody * body;
        
        body = reinterpret_cast< FPhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body );
        }

        body = reinterpret_cast< FPhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body );
        }

        return 0.0f;
    }

    TPodArray< FPhysicalBody * > & Result;
    FCollisionQueryFilter const & QueryFilter;
};

struct FQueryActorsCallback : public btCollisionWorld::ContactResultCallback {
    FQueryActorsCallback( TPodArray< FActor * > & _Result, FCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        FPhysicalBody * body;

        body = reinterpret_cast< FPhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body->GetParentActor() ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        body = reinterpret_cast< FPhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body->GetParentActor() ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        return 0.0f;
    }

    TPodArray< FActor * > & Result;
    FCollisionQueryFilter const & QueryFilter;
};

void FWorld::QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter ) const {
    FQueryPhysicalBodiesCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryActors( TPodArray< FActor * > & _Result, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter ) const {
    FQueryActorsCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, FCollisionQueryFilter const * _QueryFilter ) const {
    FQueryPhysicalBodiesCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryActors( TPodArray< FActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, FCollisionQueryFilter const * _QueryFilter ) const {
    FQueryActorsCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    PhysicsWorld->addRigidBody( tempBody );
    PhysicsWorld->contactTest( tempBody, callback );
    PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void FWorld::QueryPhysicalBodies( TPodArray< FPhysicalBody * > & _Result, BvAxisAlignedBox const & _BoundingBox, FCollisionQueryFilter const * _QueryFilter ) const {
    QueryPhysicalBodies( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}

void FWorld::QueryActors( TPodArray< FActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, FCollisionQueryFilter const * _QueryFilter ) const {
    QueryActors( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}

void FWorld::ApplyRadialDamage( float _DamageAmount, Float3 const & _Position, float _Radius, FCollisionQueryFilter const * _QueryFilter ) {
    TPodArray< FActor * > damagedActors;
    QueryActors( damagedActors, _Position, _Radius, _QueryFilter );
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

    if ( DebugDrawFrame == GGameEngine.GetFrameNumber() ) {
        // Debug commands was already generated for this frame
        return;
    }

    DebugDrawFrame = GGameEngine.GetFrameNumber();

    FirstDebugDrawCommand = _DebugDraw->CommandsCount();

    _DebugDraw->SplitCommands();

    for ( FLevel * level : ArrayOfLevels ) {
        level->DrawDebug( _DebugDraw );
    }

    _DebugDraw->SetDepthTest( true );

    _DebugDraw->SetColor(1,1,1,1);

    if ( GDebugDrawFlags.bDrawMeshBounds ) {
        for ( FMeshComponent * component = MeshList ; component ; component = component->GetNextMesh() ) {

            _DebugDraw->DrawAABB( component->GetWorldBounds() );
        }
    }

    for ( FActor * actor : Actors ) {
        actor->DrawDebug( _DebugDraw );

        if ( GDebugDrawFlags.bDrawRootComponentAxis ) {
            if ( actor->RootComponent ) {
                _DebugDraw->SetDepthTest( false );
                _DebugDraw->DrawAxis( actor->RootComponent->GetWorldTransformMatrix(), false );
            }
        }
    }

    _DebugDraw->SetDepthTest( false );
    PhysicsDebugDraw.DD = _DebugDraw;

    int Mode = 0;
    if ( GDebugDrawFlags.bDrawCollisionShapeWireframe ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawWireframe;
    }
    //if ( GDebugDrawFlags.bDrawCollisionShapeAABBs ) {
    //    Mode |= FPhysicsDebugDraw::DBG_DrawAabb;
    //}
    if ( GDebugDrawFlags.bDrawContactPoints ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawContactPoints;
    }
    if ( GDebugDrawFlags.bDrawConstraints ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawConstraints;
    }
    if ( GDebugDrawFlags.bDrawConstraintLimits ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawConstraintLimits;
    }
    //if ( GDebugDrawFlags.bDrawCollisionShapeNormals ) {
    //    Mode |= FPhysicsDebugDraw::DBG_DrawNormals;
    //}

    PhysicsDebugDraw.setDebugMode( Mode );
    PhysicsWorld->debugDrawWorld();

    DebugDrawCommandCount = _DebugDraw->CommandsCount() - FirstDebugDrawCommand;

    //GLogger.Printf( "DebugDrawCommandCount %d\n", DebugDrawCommandCount );
}
