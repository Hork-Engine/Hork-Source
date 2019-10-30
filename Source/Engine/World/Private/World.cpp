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
#include <Engine/World/Public/Actors/Pawn.h>
#include <Engine/World/Public/Components/SkinnedComponent.h>
#include <Engine/World/Public/Components/DirectionalLightComponent.h>
#include <Engine/World/Public/Components/PointLightComponent.h>
#include <Engine/World/Public/Components/SpotLightComponent.h>
#include <Engine/World/Public/Timer.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>
#include <Engine/Core/Public/BV/BvIntersect.h>
#include <Engine/Runtime/Public/Runtime.h>
#include <Engine/Base/Public/GameModuleInterface.h>
#include "ShadowCascade.h"

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <btBulletDynamicsCommon.h>

#include <Engine/BulletCompatibility/BulletCompatibility.h>

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

FRuntimeVariable RVDrawMeshBounds( _CTS( "DrawMeshBounds" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVDrawRootComponentAxis( _CTS( "DrawRootComponentAxis" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVDrawCollisionShapeWireframe( _CTS( "DrawCollisionShapeWireframe" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVDrawContactPoints( _CTS( "DrawContactPoints" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVDrawConstraints( _CTS( "DrawConstraints" ), _CTS( "0" ), VAR_CHEAT );
FRuntimeVariable RVDrawConstraintLimits( _CTS( "DrawConstraintLimits" ), _CTS( "0" ), VAR_CHEAT );

AN_FORCEINLINE static unsigned short ClampUnsignedShort( int _Value ) {
    if ( _Value < 0 ) return 0;
    if ( _Value > 0xffff ) return 0xffff;
    return _Value;
}

void FActorSpawnInfo::SetTemplate( FActor const * _Template ) {
    AN_Assert( &_Template->FinalClassMeta() == ActorTypeClassMeta );
    Template = _Template;
}

class FPhysicsDebugDraw : public btIDebugDraw {
public:
    FDebugDraw * DD;
    int DebugMode;

    void drawLine( btVector3 const & from, btVector3 const & to, btVector3 const & color ) override {
        DD->SetColor( FColor4( color.x(), color.y(), color.z(), 1.0f ) );
        DD->DrawLine( btVectorToFloat3( from ), btVectorToFloat3( to ) );
    }

    void drawContactPoint( btVector3 const & pointOnB, btVector3 const & normalOnB, btScalar distance, int lifeTime, btVector3 const & color ) override {
        DD->SetColor( FColor4( color.x(), color.y(), color.z(), 1.0f ) );
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

FWorld * FWorld::PendingKillWorlds = nullptr;

FWorld::FWorld() {
    PersistentLevel = NewObject< FLevel >();
    PersistentLevel->AddRef();
    PersistentLevel->OwnerWorld = this;
    PersistentLevel->bIsPersistent = true;
    PersistentLevel->IndexInArrayOfLevels = ArrayOfLevels.Size();
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
    NextPendingKillWorld = PendingKillWorlds;
    PendingKillWorlds = this;

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

FActorSpawnInfo::FActorSpawnInfo( uint64_t _ActorClassId )
    : FActorSpawnInfo( FActor::Factory().LookupClass( _ActorClassId ) )
{
}

FActorSpawnInfo::FActorSpawnInfo( const char * _ActorClassName )
    : FActorSpawnInfo( FActor::Factory().LookupClass( _ActorClassName ) )
{
}

FActor * FWorld::SpawnActor( FActorSpawnInfo const & _SpawnParameters ) {

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
    actor->IndexInWorldArrayOfActors = Actors.Size() - 1;
    actor->ParentWorld = this;

    actor->Level = _SpawnParameters.Level ? _SpawnParameters.Level : PersistentLevel;
    actor->Level->Actors.Append( actor );
    actor->IndexInLevelArrayOfActors = actor->Level->Actors.Size() - 1;

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

static Float3 ReadFloat3( FDocument const & _Document, int _FieldsHead, const char * _FieldName, Float3 const & _Default ) {
    FDocumentField * field = _Document.FindField( _FieldsHead, _FieldName );
    if ( !field ) {
        return _Default;
    }

    FDocumentValue * value = &_Document.Values[ field->ValuesHead ];

    Float3 r;
    sscanf( value->Token.ToString().ToConstChar(), "%f %f %f", &r.X, &r.Y, &r.Z );
    return r;
}

static Quat ReadQuat( FDocument const & _Document, int _FieldsHead, const char * _FieldName, Quat const & _Default ) {
    FDocumentField * field = _Document.FindField( _FieldsHead, _FieldName );
    if ( !field ) {
        return _Default;
    }

    FDocumentValue * value = &_Document.Values[ field->ValuesHead ];

    Quat r;
    sscanf( value->Token.ToString().ToConstChar(), "%f %f %f %f", &r.X, &r.Y, &r.Z, &r.W );
    return r;
}

FActor * FWorld::LoadActor( FDocument const & _Document, int _FieldsHead, FLevel * _Level ) {

    //GLogger.Printf( "==== Load Actor ====\n" );

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
    actor->IndexInWorldArrayOfActors = Actors.Size() - 1;
    actor->ParentWorld = this;

    actor->Level = _Level ? _Level : PersistentLevel;
    actor->Level->Actors.Append( actor );
    actor->IndexInLevelArrayOfActors = actor->Level->Actors.Size() - 1;

    // Update actor name to make it unique
    actor->SetName( actor->Name );

    // Load actor attributes
    actor->LoadAttributes( _Document, _FieldsHead );

#if 0
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
#endif

    FTransform spawnTransform;

    spawnTransform.Position = ReadFloat3( _Document, _FieldsHead, "SpawnPosition", Float3(0.0f) );
    spawnTransform.Rotation = ReadQuat( _Document, _FieldsHead, "SpawnRotation", Quat::Identity() );
    spawnTransform.Scale    = ReadFloat3( _Document, _FieldsHead, "SpawnScale", Float3(1.0f) );

    actor->PostSpawnInitialize( spawnTransform );

    //actor->PostActorCreated();
    //actor->ExecuteContruction( _SpawnParameters.SpawnTransform );
    actor->PostActorConstruction();
    BroadcastActorSpawned( actor );
    actor->BeginPlayComponents();
    actor->BeginPlay();

    //GLogger.Printf( "=====================\n" );
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
    E_OnActorSpawned.Dispatch( _SpawnedActor );
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
            StdSwap( objectA, objectB );
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
                contactHash.Insert( hash, currentContacts.Size() - 1 );
            }
        }
    }

    // Reset cache
    CacheContactPoints = -1;

    // Dispatch contact and overlap events (OnBeginContact, OnBeginOverlap, OnUpdateContact, OnUpdateOverlap)
    for ( int i = 0 ; i < currentContacts.Size() ; i++ ) {
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
                    contactEvent.NumPoints = ContactPoints.Size();
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
                    contactEvent.NumPoints = ContactPoints.Size();
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
                    contactEvent.NumPoints = ContactPoints.Size();
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
                    contactEvent.NumPoints = ContactPoints.Size();
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
    for ( int i = 0; i < prevContacts.Size(); i++ ) {
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

    int numSimulationSteps = FMath::Floor( _TimeStep * PhysicsHertz ) + 1.0f;
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
                parent->Components[ component->ComponentIndex ] = parent->Components[ parent->Components.Size() - 1 ];
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
            Actors[ actor->IndexInWorldArrayOfActors ] = Actors[ Actors.Size() - 1 ];
            Actors[ actor->IndexInWorldArrayOfActors ]->IndexInWorldArrayOfActors = actor->IndexInWorldArrayOfActors;
            Actors.RemoveLast();
            actor->IndexInWorldArrayOfActors = -1;
            actor->ParentWorld = nullptr;

            // Remove actor from level array of actors
            FLevel * level = actor->Level;
            level->Actors[ actor->IndexInLevelArrayOfActors ] = level->Actors[ level->Actors.Size() - 1 ];
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
    _Level->IndexInArrayOfLevels = ArrayOfLevels.Size();
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

    ArrayOfLevels[ _Level->IndexInArrayOfLevels ] = ArrayOfLevels[ ArrayOfLevels.Size() - 1 ];
    ArrayOfLevels[ _Level->IndexInArrayOfLevels ]->IndexInArrayOfLevels = _Level->IndexInArrayOfLevels;
    ArrayOfLevels.RemoveLast();

    _Level->OwnerWorld = nullptr;
    _Level->IndexInArrayOfLevels = -1;
    _Level->RemoveRef();
}

void FWorld::AddMesh( FMeshComponent * _Mesh ) {
    if ( IntrusiveIsInList( _Mesh, Next, Prev, MeshList, MeshListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Mesh, Next, Prev, MeshList, MeshListTail );
}

void FWorld::RemoveMesh( FMeshComponent * _Mesh ) {
    IntrusiveRemoveFromList( _Mesh, Next, Prev, MeshList, MeshListTail );
}

void FWorld::AddSkinnedMesh( FSkinnedComponent * _Skeleton ) {
    if ( IntrusiveIsInList( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void FWorld::RemoveSkinnedMesh( FSkinnedComponent * _Skeleton ) {
    IntrusiveRemoveFromList( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void FWorld::AddDirectionalLight( FDirectionalLightComponent * _Light ) {
    if ( IntrusiveIsInList( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void FWorld::RemoveDirectionalLight( FDirectionalLightComponent * _Light ) {
    IntrusiveRemoveFromList( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void FWorld::AddPointLight( FPointLightComponent * _Light ) {
    if ( IntrusiveIsInList( _Light, Next, Prev, PointLightList, PointLightListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Light, Next, Prev, PointLightList, PointLightListTail );
}

void FWorld::RemovePointLight( FPointLightComponent * _Light ) {
    IntrusiveRemoveFromList( _Light, Next, Prev, PointLightList, PointLightListTail );
}

void FWorld::AddSpotLight( FSpotLightComponent * _Light ) {
    if ( IntrusiveIsInList( _Light, Next, Prev, SpotLightList, SpotLightListTail ) ) {
        AN_Assert( 0 );
        return;
    }

    IntrusiveAddToList( _Light, Next, Prev, SpotLightList, SpotLightListTail );
}

void FWorld::RemoveSpotLight( FSpotLightComponent * _Light ) {
    IntrusiveRemoveFromList( _Light, Next, Prev, SpotLightList, SpotLightListTail );
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

void FWorld::DrawDebug( FDebugDraw * _DebugDraw, int _FrameNumber ) {

    if ( DebugDrawFrame == _FrameNumber ) {
        // Debug commands was already generated for this frame
        return;
    }

    DebugDrawFrame = _FrameNumber;

    FirstDebugDrawCommand = _DebugDraw->CommandsCount();

    _DebugDraw->SplitCommands();

    for ( FLevel * level : ArrayOfLevels ) {
        level->DrawDebug( _DebugDraw );
    }

    _DebugDraw->SetDepthTest( true );

    _DebugDraw->SetColor( FColor4( 1,1,1,1 ) );

    if ( RVDrawMeshBounds ) {
        for ( FMeshComponent * component = MeshList ; component ; component = component->GetNextMesh() ) {

            _DebugDraw->DrawAABB( component->GetWorldBounds() );
        }
    }

    for ( FActor * actor : Actors ) {
        actor->DrawDebug( _DebugDraw );

        if ( RVDrawRootComponentAxis ) {
            if ( actor->RootComponent ) {
                _DebugDraw->SetDepthTest( false );
                _DebugDraw->DrawAxis( actor->RootComponent->GetWorldTransformMatrix(), false );
            }
        }
    }

    _DebugDraw->SetDepthTest( false );
    PhysicsDebugDraw.DD = _DebugDraw;

    int Mode = 0;
    if ( RVDrawCollisionShapeWireframe ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawWireframe;
    }
    //if ( RVDrawCollisionShapeAABBs ) {
    //    Mode |= FPhysicsDebugDraw::DBG_DrawAabb;
    //}
    if ( RVDrawContactPoints ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawContactPoints;
    }
    if ( RVDrawConstraints ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawConstraints;
    }
    if ( RVDrawConstraintLimits ) {
        Mode |= FPhysicsDebugDraw::DBG_DrawConstraintLimits;
    }
    //if ( RVDrawCollisionShapeNormals ) {
    //    Mode |= FPhysicsDebugDraw::DBG_DrawNormals;
    //}

    PhysicsDebugDraw.setDebugMode( Mode );
    PhysicsWorld->debugDrawWorld();

    DebugDrawCommandCount = _DebugDraw->CommandsCount() - FirstDebugDrawCommand;

    //GLogger.Printf( "DebugDrawCommandCount %d\n", DebugDrawCommandCount );
}

void FWorld::RenderFrontend_AddInstances( FRenderFrontendDef * _Def ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();
    FRenderView * view = _Def->View;

    for ( FLevel * level : ArrayOfLevels ) {
        level->RenderFrontend_AddInstances( _Def );
    }

    // Add directional lights
    for ( FDirectionalLightComponent * light = DirectionalLightList ; light ; light = light->Next ) {

        if ( view->NumDirectionalLights > MAX_DIRECTIONAL_LIGHTS ) {
            GLogger.Printf( "MAX_DIRECTIONAL_LIGHTS hit\n" );
            break;
        }

        if ( !light->IsEnabled() ) {
            continue;
        }

        FDirectionalLightDef * lightDef = (FDirectionalLightDef *)GRuntime.AllocFrameMem( sizeof( FDirectionalLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->DirectionalLights.Append( lightDef );

        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Matrix = light->GetWorldRotation().ToMatrix();
        lightDef->MaxShadowCascades = light->GetMaxShadowCascades();
        lightDef->RenderMask = light->RenderingGroup;
        lightDef->NumCascades = 0;  // this will be calculated later
        lightDef->FirstCascade = 0; // this will be calculated later
        lightDef->bCastShadow = light->bCastShadow;

        view->NumDirectionalLights++;
    }


    // Add point lights
    for ( FPointLightComponent * light = PointLightList ; light ; light = light->Next ) {

        if ( !light->IsEnabled() ) {
            continue;
        }

        // TODO: cull light

        FLightDef * lightDef = (FLightDef *)GRuntime.AllocFrameMem( sizeof( FLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->Lights.Append( lightDef );

        lightDef->bSpot = false;
        lightDef->BoundingBox = light->GetWorldBounds();
        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Position = light->GetWorldPosition();
        lightDef->RenderMask = light->RenderingGroup;
        lightDef->InnerRadius = light->GetInnerRadius();
        lightDef->OuterRadius = light->GetOuterRadius();
        lightDef->OBBTransformInverse = light->GetOBBTransformInverse();

        view->NumLights++;
    }


    // Add spot lights
    for ( FSpotLightComponent * light = SpotLightList ; light ; light = light->Next ) {

        if ( !light->IsEnabled() ) {
            continue;
        }

        // TODO: cull light

        FLightDef * lightDef = (FLightDef *)GRuntime.AllocFrameMem( sizeof( FLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->Lights.Append( lightDef );

        lightDef->bSpot = true;
        lightDef->BoundingBox = light->GetWorldBounds();
        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Position = light->GetWorldPosition();
        lightDef->RenderMask = light->RenderingGroup;        
        lightDef->InnerRadius = light->GetInnerRadius();
        lightDef->OuterRadius = light->GetOuterRadius();
        lightDef->InnerConeAngle = light->GetInnerConeAngle();
        lightDef->OuterConeAngle = light->GetOuterConeAngle();
        lightDef->SpotDirection = light->GetWorldDirection();
        lightDef->SpotExponent = light->GetSpotExponent();
        lightDef->OBBTransformInverse = light->GetOBBTransformInverse();

        view->NumLights++;
    }

    void Voxelize( FRenderFrame * Frame, FRenderView * RV );

    Voxelize( frameData, view );
}

void FWorld::RenderFrontend_AddDirectionalShadowmapInstances( FRenderFrontendDef * _Def ) {

    CreateDirectionalLightCascades( GRuntime.GetFrameData(), _Def->View );

    if ( !_Def->View->NumShadowMapCascades ) {
        return;
    }

    // Create shadow instances

    for ( FMeshComponent * component = MeshList ; component ; component = component->GetNextMesh() ) {

        if ( !component->bCastShadow ) {
            continue;
        }

        // TODO: Perform culling for each shadow cascade, set CascadeMask

        //if ( component->RenderMark == _Def->VisMarker ) {
        //    return;
        //}

        if ( (component->RenderingGroup & _Def->RenderingMask) == 0 ) {
        //    component->RenderMark = _Def->VisMarker;
            continue;
        }

//        if ( component->VSDPasses & VSD_PASS_FACE_CULL ) {
//            // TODO: bTwoSided and bFrontSided must came from component
//            const bool bTwoSided = false;
//            const bool bFrontSided = true;
//            const float EPS = 0.25f;
//
//            if ( !bTwoSided ) {
//                PlaneF const & plane = component->FacePlane;
//                float d = _Def->View->ViewPosition.Dot( plane.Normal );
//
//                bool bFaceCull = false;
//
//                if ( bFrontSided ) {
//                    if ( d < -plane.D - EPS ) {
//                        bFaceCull = true;
//                    }
//                } else {
//                    if ( d > -plane.D + EPS ) {
//                        bFaceCull = true;
//                    }
//                }
//
//                if ( bFaceCull ) {
//                    component->RenderMark = _Def->VisMarker;
//#ifdef DEBUG_TRAVERSING_COUNTERS
//                    Dbg_CulledByDotProduct++;
//#endif
//                    return;
//                }
//            }
//        }

//        if ( component->VSDPasses & VSD_PASS_BOUNDS ) {
//
//            // TODO: use SSE cull
//            BvAxisAlignedBox const & bounds = component->GetWorldBounds();
//
//            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
//#ifdef DEBUG_TRAVERSING_COUNTERS
//                Dbg_CulledBySurfaceBounds++;
//#endif
//                return;
//            }
//        }

//        component->RenderMark = _Def->VisMarker;

        //if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        //    bool bVisible;
        //    component->RenderFrontend_CustomVisibleStep( _Def, bVisible );

        //    if ( !bVisible ) {
        //        return;
        //    }
        //}

        //if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        //    bool bVisible = component->VisMarker == _Def->VisMarker;
        //    if ( !bVisible ) {
        //        return;
        //    }
        //}

        Float3x4 const * instanceMatrix;

        FIndexedMesh * mesh = component->GetMesh();
        if ( !mesh ) {
            // TODO: default mesh?
            continue;
        }

        size_t skeletonOffset = 0;
        size_t skeletonSize = 0;
        if ( mesh->IsSkinned() && component->IsSkinnedMesh() ) {
            FSkinnedComponent * skeleton = static_cast< FSkinnedComponent * >(component);

            // TODO: Если кости уже были обновлены на этом кадре, то не нужно их обновлять снова!
            skeleton->UpdateJointTransforms( skeletonOffset, skeletonSize );
        }

        if ( component->bNoTransform ) {
            instanceMatrix = &Float3x4::Identity();
        } else {
            instanceMatrix = &component->GetWorldTransformMatrix();
        }

        FIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

        for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

            // FIXME: check subpart bounding box here

            FIndexedMeshSubpart * subpart = subparts[subpartIndex];

            FMaterialInstance * materialInstance = component->GetMaterialInstance( subpartIndex );
            AN_Assert( materialInstance );

            FMaterial * material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if ( material->GetGPUResource()->bNoCastShadow ) {
                continue;
            }

            FMaterialFrameData * materialInstanceFrameData = materialInstance->RenderFrontend_Update( _Def->VisMarker );

            // Add render instance
            FShadowRenderInstance * instance = (FShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( FShadowRenderInstance ) );
            if ( !instance ) {
                break;
            }

            GRuntime.GetFrameData()->ShadowInstances.Append( instance );

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;
            instance->VertexBuffer = mesh->GetVertexBufferGPU();
            instance->IndexBuffer = mesh->GetIndexBufferGPU();
            instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

            if ( component->bUseDynamicRange ) {
                instance->IndexCount = component->DynamicRangeIndexCount;
                instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
                instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
            } else {
                instance->IndexCount = subpart->GetIndexCount();
                instance->StartIndexLocation = subpart->GetFirstIndex();
                instance->BaseVertexLocation = subpart->GetBaseVertex() + component->SubpartBaseVertexOffset;
            }

            instance->SkeletonOffset = skeletonOffset;
            instance->SkeletonSize = skeletonSize;
            instance->WorldTransformMatrix = *instanceMatrix;
            instance->CascadeMask = 0xffff; // TODO: Calculate!!!

            _Def->View->ShadowInstanceCount++;

            _Def->ShadowMapPolyCount += instance->IndexCount / 3;

            if ( component->bUseDynamicRange ) {
                // If component uses dynamic range, mesh has actually one subpart
                break;
            }
        }
    }
}


TPodArray< FWorld * > FWorld::Worlds;

FWorld * FWorld::CreateWorld() {
    FWorld * world = CreateInstanceOf< FWorld >();

    world->AddRef();

    // Add world to game array of worlds
    Worlds.Append( world );
    world->IndexInGameArrayOfWorlds = Worlds.Size() - 1;

    world->BeginPlay();

    return world;
}

void FWorld::DestroyWorlds() {
    for ( FWorld * world : Worlds ) {
        world->Destroy();
    }
}

void FWorld::KickoffPendingKillWorlds() {
    while ( PendingKillWorlds ) {
        FWorld * world = PendingKillWorlds;
        FWorld * nextWorld;

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
}

void FWorld::UpdateWorlds( IGameModule * _GameModule, float _TimeStep ) {
    _GameModule->OnPreGameTick( _TimeStep );
    for ( FWorld * world : Worlds ) {
        if ( world->IsPendingKill() ) {
            continue;
        }
        world->Tick( _TimeStep );
    }
    _GameModule->OnPostGameTick( _TimeStep );

    KickoffPendingKillWorlds();

    FSpatialObject::_UpdateSurfaceAreas();
}
