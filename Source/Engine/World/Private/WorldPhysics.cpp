/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <World/Public/WorldPhysics.h>
#include <World/Public/Components/TerrainComponent.h>
#include <World/Public/Components/PhysicalBody.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/World.h>
#include <World/Public/Base/DebugRenderer.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Platform/Public/Logger.h>

#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>

#include "BulletCompatibility/BulletCompatibility.h"

#ifdef AN_COMPILER_MSVC
#pragma warning(push)
#pragma warning( disable : 4456 4828 )
#endif
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#ifdef AN_COMPILER_MSVC
#pragma warning(pop)
#endif

#define SOFT_BODY_WORLD
#define REF_AWARE

ARuntimeVariable com_DrawContactPoints( _CTS( "com_DrawContactPoints" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_DrawConstraints( _CTS( "com_DrawConstraints" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_DrawConstraintLimits( _CTS( "com_DrawConstraintLimits" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_NoPhysicsSimulation( _CTS( "com_NoPhysicsSimulation" ), _CTS( "0" ), VAR_CHEAT );

static SCollisionQueryFilter DefaultCollisionQueryFilter;

struct SCollisionFilterCallback : public btOverlapFilterCallback
{
    // Return true when pairs need collision
    bool needBroadphaseCollision( btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1 ) const override
    {
        if ( ( proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask )
             && ( proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask ) )
        {
            // FIXME: can we safe cast m_clientObject to btCollisionObject?
            btCollisionObject const * colObj0 = static_cast< btCollisionObject const * >( proxy0->m_clientObject );
            btCollisionObject const * colObj1 = static_cast< btCollisionObject const * >( proxy1->m_clientObject );

            AHitProxy const * hitProxy0 = static_cast< AHitProxy const * >(colObj0->getUserPointer());
            AHitProxy const * hitProxy1 = static_cast< AHitProxy const * >(colObj1->getUserPointer());

            if ( !hitProxy0 || !hitProxy1 ) {
                return true;
            }

            AActor * actor0 = hitProxy0->GetOwnerActor();
            AActor * actor1 = hitProxy1->GetOwnerActor();

            if ( hitProxy0->GetCollisionIgnoreActors().Find( actor1 ) != hitProxy0->GetCollisionIgnoreActors().End() ) {
                return false;
            }

            if ( hitProxy1->GetCollisionIgnoreActors().Find( actor0 ) != hitProxy1->GetCollisionIgnoreActors().End() ) {
                return false;
            }

            return true;
        }

        return false;
    }
};

static SCollisionFilterCallback CollisionFilterCallback;

extern ContactAddedCallback gContactAddedCallback;

static bool CustomMaterialCombinerCallback( btManifoldPoint & cp,
                                            const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
                                            const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 )
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

void AWorldPhysics::GenerateContactPoints( int _ContactIndex, SCollisionContact & _Contact )
{
    if ( CacheContactPoints == _ContactIndex ) {
        // Contact points already generated for this contact
        return;
    }

    CacheContactPoints = _ContactIndex;

    ContactPoints.ResizeInvalidate( _Contact.Manifold->getNumContacts() );

    bool bSwapped = static_cast< AHitProxy * >( _Contact.Manifold->getBody0()->getUserPointer() ) == _Contact.ComponentB;

    if ( ( _ContactIndex & 1 ) == 0 ) {
        // BodyA

        if ( bSwapped ) {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                SContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnA );
                contact.Normal = -btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        } else {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                SContactPoint & contact = ContactPoints[ j ];
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
                SContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnB );
                contact.Normal = btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        } else {
            for ( int j = 0; j < _Contact.Manifold->getNumContacts(); ++j ) {
                btManifoldPoint & point = _Contact.Manifold->getContactPoint( j );
                SContactPoint & contact = ContactPoints[ j ];
                contact.Position = btVectorToFloat3( point.m_positionWorldOnA );
                contact.Normal = -btVectorToFloat3( point.m_normalWorldOnB );
                contact.Distance = point.m_distance1;
                contact.Impulse = point.m_appliedImpulse;
            }
        }
    }
}

AWorldPhysics::AWorldPhysics()
{
    GravityVector = Float3( 0.0f, -9.81f, 0.0f );

    gContactAddedCallback = CustomMaterialCombinerCallback;

#if 1
    BroadphaseInterface = MakeUnique< btDbvtBroadphase >();
#else
    BroadphaseInterface = MakeUnique< btAxisSweep3 >( btVector3( -10000, -10000, -10000 ), btVector3( 10000, 10000, 10000 ) );
#endif

    //CollisionConfiguration = MakeUnique< btDefaultCollisionConfiguration >();
    CollisionConfiguration = MakeUnique< btSoftBodyRigidBodyCollisionConfiguration >();
    CollisionDispatcher = MakeUnique< btCollisionDispatcher >( CollisionConfiguration.GetObject() );
    // TODO: remove this if we don't use gimpact
    btGImpactCollisionAlgorithm::registerAlgorithm( CollisionDispatcher.GetObject() );
    ConstraintSolver = MakeUnique< btSequentialImpulseConstraintSolver >();

#ifdef SOFT_BODY_WORLD
    DynamicsWorld = MakeUnique< btSoftRigidDynamicsWorld >( CollisionDispatcher.GetObject(),
                                                            BroadphaseInterface.GetObject(),
                                                            ConstraintSolver.GetObject(),
                                                            CollisionConfiguration.GetObject(),
                                                            /* SoftBodySolver */ nullptr );
#else
    DynamicsWorld = MakeUnique< btDiscreteDynamicsWorld >( CollisionDispatcher, BroadphaseInterface, ConstraintSolver, CollisionConfiguration );
#endif

    DynamicsWorld->setGravity( btVectorToFloat3( GravityVector ) );
    DynamicsWorld->getDispatchInfo().m_useContinuous = true;
    //DynamicsWorld->getSolverInfo().m_splitImpulse = pOwnerWorld->bContactSolverSplitImpulse;
    //DynamicsWorld->getSolverInfo().m_numIterations = pOwnerWorld->NumContactSolverIterations;
    DynamicsWorld->getPairCache()->setOverlapFilterCallback( &CollisionFilterCallback );
    DynamicsWorld->setInternalTickCallback( OnPrePhysics, static_cast< void * >( this ), true );
    DynamicsWorld->setInternalTickCallback( OnPostPhysics, static_cast< void * >( this ), false );
    //DynamicsWorld->setSynchronizeAllMotionStates( true ); // TODO: check how it works

    GhostPairCallback = MakeUnique< btGhostPairCallback >();
    BroadphaseInterface->getOverlappingPairCache()->setInternalGhostPairCallback( GhostPairCallback.GetObject() );

#ifdef SOFT_BODY_WORLD
    SoftBodyWorldInfo = &DynamicsWorld->getWorldInfo();
    SoftBodyWorldInfo->m_dispatcher = CollisionDispatcher.GetObject();
    SoftBodyWorldInfo->m_broadphase = BroadphaseInterface.GetObject();
    SoftBodyWorldInfo->m_gravity = btVectorToFloat3( GravityVector );
    SoftBodyWorldInfo->air_density = ( btScalar )1.2;
    SoftBodyWorldInfo->water_density = 0;
    SoftBodyWorldInfo->water_offset = 0;
    SoftBodyWorldInfo->water_normal = btVector3( 0, 0, 0 );
    SoftBodyWorldInfo->m_sparsesdf.Initialize();
#endif
}

AWorldPhysics::~AWorldPhysics()
{
    RemoveCollisionContacts();

    DynamicsWorld.Reset();
    ConstraintSolver.Reset();
    CollisionDispatcher.Reset();
    CollisionConfiguration.Reset();
    BroadphaseInterface.Reset();
    GhostPairCallback.Reset();
}

void AWorldPhysics::RemoveCollisionContacts()
{
#ifdef REF_AWARE
    for ( int i = 0 ; i < 2 ; i++ ) {
        TPodVector< SCollisionContact > & currentContacts = CollisionContacts[ i ];
        THash<> & contactHash = ContactHash[ i ];

        for ( SCollisionContact & contact : currentContacts ) {
            contact.ActorA->RemoveRef();
            contact.ActorB->RemoveRef();
            contact.ComponentA->RemoveRef();
            contact.ComponentB->RemoveRef();
        }

        currentContacts.Clear();
        contactHash.Clear();
    }
#endif
}

void AWorldPhysics::AddPendingBody( AHitProxy * InPhysicalBody )
{
    INTRUSIVE_ADD_UNIQUE( InPhysicalBody, NextMarked, PrevMarked, PendingAddToWorldHead, PendingAddToWorldTail );
}

void AWorldPhysics::RemovePendingBody( AHitProxy * InPhysicalBody )
{
    INTRUSIVE_REMOVE( InPhysicalBody, NextMarked, PrevMarked, PendingAddToWorldHead, PendingAddToWorldTail );
}

void AWorldPhysics::AddHitProxy( AHitProxy * HitProxy )
{
    if ( !HitProxy ) {
        // Passed a null pointer
        return;
    }

    if ( HitProxy->bInWorld ) {
        // Physical body is already in world, so remove it from the world
        if ( HitProxy->GetCollisionObject() ) {
            DynamicsWorld->removeCollisionObject( HitProxy->GetCollisionObject() );
        }
        HitProxy->bInWorld = false;
    }

    if ( HitProxy->GetCollisionObject() ) {
        // Add physical body to pending list
        AddPendingBody( HitProxy );
    }
}

void AWorldPhysics::RemoveHitProxy( AHitProxy * HitProxy )
{
    if ( !HitProxy ) {
        // Passed a null pointer
        return;
    }

    // Remove physical body from pending list
    RemovePendingBody( HitProxy );

    if ( !HitProxy->bInWorld ) {
        // Physical body is not in world
        return;
    }

    DynamicsWorld->removeCollisionObject( HitProxy->GetCollisionObject() );

    HitProxy->bInWorld = false;
}

void AWorldPhysics::AddPendingBodies()
{
    AHitProxy * next;
    for ( AHitProxy * hitProxy = PendingAddToWorldHead ; hitProxy ; hitProxy = next )
    {
        next = hitProxy->NextMarked;

        hitProxy->NextMarked = hitProxy->PrevMarked = nullptr;

        if ( hitProxy->GetCollisionObject() )
        {
            AN_ASSERT( !hitProxy->bInWorld );

            btRigidBody* rigidBody = btRigidBody::upcast( hitProxy->GetCollisionObject() );
            if ( rigidBody ) {
                DynamicsWorld->addRigidBody( rigidBody, hitProxy->GetCollisionGroup(), hitProxy->GetCollisionMask() );
            }
            else {
                DynamicsWorld->addCollisionObject( hitProxy->GetCollisionObject(), hitProxy->GetCollisionGroup(), hitProxy->GetCollisionMask() );
            }
            hitProxy->bInWorld = true;
        }
    }
    PendingAddToWorldHead = PendingAddToWorldTail = nullptr;
}

void AWorldPhysics::DispatchContactAndOverlapEvents()
{
#ifdef AN_COMPILER_MSVC
#pragma warning( disable : 4456 )
#endif

    int curTickNumber = FixedTickNumber & 1;
    int prevTickNumber = ( FixedTickNumber + 1 ) & 1;

    TPodVector< SCollisionContact > & currentContacts = CollisionContacts[ curTickNumber ];
    TPodVector< SCollisionContact > & prevContacts = CollisionContacts[ prevTickNumber ];

    THash<> & contactHash = ContactHash[ curTickNumber ];
    THash<> & prevContactHash = ContactHash[ prevTickNumber ];

    SCollisionContact contact;

    SOverlapEvent overlapEvent;
    SContactEvent contactEvent;

#ifdef REF_AWARE
    for ( SCollisionContact & contact : currentContacts ) {
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

        AHitProxy * objectA = static_cast< AHitProxy * >( contactManifold->getBody0()->getUserPointer() );
        AHitProxy * objectB = static_cast< AHitProxy * >( contactManifold->getBody1()->getUserPointer() );

        if ( !objectA || !objectB ) {
            // ghost object
            continue;
        }

        if ( objectA->Id < objectB->Id ) {
            std::swap( objectA, objectB );
        }

        AActor * actorA = objectA->GetOwnerActor();
        AActor * actorB = objectB->GetOwnerActor();

        ASceneComponent * componentA = objectA->GetOwnerComponent();
        ASceneComponent * componentB = objectB->GetOwnerComponent();

        if ( actorA->IsPendingKill() || actorB->IsPendingKill() || componentA->IsPendingKill() || componentB->IsPendingKill() ) {
            // don't generate contact or overlap events for destroyed objects
            continue;
        }

        // Do not generate contact events if one of components is trigger
        bool bContactWithTrigger = objectA->IsTrigger() || objectB->IsTrigger();

        contact.bComponentADispatchContactEvents = !bContactWithTrigger && objectA->bDispatchContactEvents
            && ( objectA->E_OnBeginContact
                || objectA->E_OnEndContact
                || objectA->E_OnUpdateContact );

        contact.bComponentBDispatchContactEvents = !bContactWithTrigger && objectB->bDispatchContactEvents
            && ( objectB->E_OnBeginContact
                || objectB->E_OnEndContact
                || objectB->E_OnUpdateContact );

        contact.bComponentADispatchOverlapEvents = objectA->IsTrigger() && objectA->bDispatchOverlapEvents
            && ( objectA->E_OnBeginOverlap
                || objectA->E_OnEndOverlap
                || objectA->E_OnUpdateOverlap );

        contact.bComponentBDispatchOverlapEvents = objectB->IsTrigger() && objectB->bDispatchOverlapEvents
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

        contact.bActorADispatchOverlapEvents = objectA->IsTrigger() && objectA->bDispatchOverlapEvents
            && ( actorA->E_OnBeginOverlap
                || actorA->E_OnEndOverlap
                || actorA->E_OnUpdateOverlap );

        contact.bActorBDispatchOverlapEvents = objectB->IsTrigger() && objectB->bDispatchOverlapEvents
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
            for ( int h = contactHash.First( hash ) ; h != -1 ; h = contactHash.Next( h ) ) {
                if ( currentContacts[ h ].ComponentA->Id == objectA->Id
                    && currentContacts[ h ].ComponentB->Id == objectB->Id ) {
                    bUnique = false;
                    break;
                }
            }

            if ( bUnique ) {
#ifdef REF_AWARE
                actorA->AddRef();
                actorB->AddRef();
                objectA->AddRef();
                objectB->AddRef();
#endif
                currentContacts.Append( contact );
                contactHash.Insert( hash, currentContacts.Size() - 1 );
            }
            else {
                //GLogger.Printf( "Assertion failed: bUnique\n" );
            }
        }
    }

    // Reset cache
    CacheContactPoints = -1;

    struct SDispatchContactCondition
    {
        SContactEvent const & ContactEvent;

        SDispatchContactCondition( SContactEvent const & InContactEvent )
            : ContactEvent( InContactEvent )
        {

        }

        bool operator()() const
        {
            if ( ContactEvent.SelfActor->IsPendingKill() ) {
                return false;
            }

            if ( ContactEvent.OtherActor->IsPendingKill() ) {
                return false;
            }

            if ( !ContactEvent.SelfBody->GetOwnerComponent() ) {
                return false;
            }

            if ( !ContactEvent.OtherBody->GetOwnerComponent() ) {
                return false;
            }

            return true;
        }
    };

    struct SDispatchOverlapCondition
    {
        SOverlapEvent const & OverlapEvent;

        SDispatchOverlapCondition( SOverlapEvent const & InOverlapEvent )
            : OverlapEvent( InOverlapEvent )
        {

        }

        bool operator()() const
        {
            if ( OverlapEvent.SelfActor->IsPendingKill() ) {
                return false;
            }

            if ( OverlapEvent.OtherActor->IsPendingKill() ) {
                return false;
            }

            if ( !OverlapEvent.SelfBody->GetOwnerComponent() ) {
                return false;
            }

            if ( !OverlapEvent.OtherBody->GetOwnerComponent() ) {
                return false;
            }

            return true;
        }
    };

    // Dispatch contact and overlap events (OnBeginContact, OnBeginOverlap, OnUpdateContact, OnUpdateOverlap)
    for ( int i = 0 ; i < currentContacts.Size() ; i++ ) {
        SCollisionContact & contact = currentContacts[ i ];

        int hash = contact.Hash();
        bool bFirstContact = true;

        for ( int h = prevContactHash.First( hash ); h != -1; h = prevContactHash.Next( h ) ) {
            if ( prevContacts[ h ].ComponentA->Id == contact.ComponentA->Id
                && prevContacts[ h ].ComponentB->Id == contact.ComponentB->Id ) {
                bFirstContact = false;
                break;
            }
        }

        //if ( !contact.ActorA->IsPendingKill() )
        {
            if ( contact.bActorADispatchContactEvents ) {

                if ( contact.ActorA->E_OnBeginContact || contact.ActorA->E_OnUpdateContact ) {

                    if ( contact.ComponentA->bGenerateContactPoints ) {
                        GenerateContactPoints( i << 1, contact );

                        contactEvent.Points = ContactPoints.ToPtr();
                        contactEvent.NumPoints = ContactPoints.Size();
                    } else {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorA;
                    contactEvent.SelfBody = contact.ComponentA;
                    contactEvent.OtherActor = contact.ActorB;
                    contactEvent.OtherBody = contact.ComponentB;

                    if ( bFirstContact ) {
                        contact.ActorA->E_OnBeginContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    } else {
                        contact.ActorA->E_OnUpdateContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    }
                }

            } else if ( contact.bActorADispatchOverlapEvents ) {
                overlapEvent.SelfActor = contact.ActorA;
                overlapEvent.SelfBody = contact.ComponentA;
                overlapEvent.OtherActor = contact.ActorB;
                overlapEvent.OtherBody = contact.ComponentB;

                if ( bFirstContact ) {
                    contact.ActorA->E_OnBeginOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                } else {
                    contact.ActorA->E_OnUpdateOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                }
            }
        }

        //if ( !contact.ComponentA->IsPendingKill() )
        {
            if ( contact.bComponentADispatchContactEvents ) {

                if ( contact.ComponentA->E_OnBeginContact || contact.ComponentA->E_OnUpdateContact ) {
                    if ( contact.ComponentA->bGenerateContactPoints ) {
                        GenerateContactPoints( i << 1, contact );

                        contactEvent.Points = ContactPoints.ToPtr();
                        contactEvent.NumPoints = ContactPoints.Size();
                    } else {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorA;
                    contactEvent.SelfBody = contact.ComponentA;
                    contactEvent.OtherActor = contact.ActorB;
                    contactEvent.OtherBody = contact.ComponentB;

                    if ( bFirstContact ) {
                        contact.ComponentA->E_OnBeginContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    } else {
                        contact.ComponentA->E_OnUpdateContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    }
                }
            } else if ( contact.bComponentADispatchOverlapEvents ) {

                overlapEvent.SelfActor = contact.ActorA;
                overlapEvent.SelfBody = contact.ComponentA;
                overlapEvent.OtherActor = contact.ActorB;
                overlapEvent.OtherBody = contact.ComponentB;

                if ( bFirstContact ) {
                    contact.ComponentA->E_OnBeginOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                } else {
                    contact.ComponentA->E_OnUpdateOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                }
            }
        }

        //if ( !contact.ActorB->IsPendingKill() )
        {
            if ( contact.bActorBDispatchContactEvents ) {

                if ( contact.ActorB->E_OnBeginContact || contact.ActorB->E_OnUpdateContact ) {
                    if ( contact.ComponentB->bGenerateContactPoints ) {
                        GenerateContactPoints( ( i << 1 ) + 1, contact );

                        contactEvent.Points = ContactPoints.ToPtr();
                        contactEvent.NumPoints = ContactPoints.Size();
                    } else {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorB;
                    contactEvent.SelfBody = contact.ComponentB;
                    contactEvent.OtherActor = contact.ActorA;
                    contactEvent.OtherBody = contact.ComponentA;

                    if ( bFirstContact ) {
                        contact.ActorB->E_OnBeginContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    } else {
                        contact.ActorB->E_OnUpdateContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    }
                }
            } else if ( contact.bActorBDispatchOverlapEvents ) {
                overlapEvent.SelfActor = contact.ActorB;
                overlapEvent.SelfBody = contact.ComponentB;
                overlapEvent.OtherActor = contact.ActorA;
                overlapEvent.OtherBody = contact.ComponentA;

                if ( bFirstContact ) {
                    contact.ActorB->E_OnBeginOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                } else {
                    contact.ActorB->E_OnUpdateOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                }
            }
        }

        //if ( !contact.ComponentB->IsPendingKill() )
        {
            if ( contact.bComponentBDispatchContactEvents ) {

                if ( contact.ComponentB->E_OnBeginContact || contact.ComponentB->E_OnUpdateContact ) {
                    if ( contact.ComponentB->bGenerateContactPoints ) {
                        GenerateContactPoints( ( i << 1 ) + 1, contact );

                        contactEvent.Points = ContactPoints.ToPtr();
                        contactEvent.NumPoints = ContactPoints.Size();
                    } else {
                        contactEvent.Points = nullptr;
                        contactEvent.NumPoints = 0;
                    }

                    contactEvent.SelfActor = contact.ActorB;
                    contactEvent.SelfBody = contact.ComponentB;
                    contactEvent.OtherActor = contact.ActorA;
                    contactEvent.OtherBody = contact.ComponentA;

                    if ( bFirstContact ) {
                        contact.ComponentB->E_OnBeginContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    } else {
                        contact.ComponentB->E_OnUpdateContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
                    }
                }
            } else if ( contact.bComponentBDispatchOverlapEvents ) {

                overlapEvent.SelfActor = contact.ActorB;
                overlapEvent.SelfBody = contact.ComponentB;
                overlapEvent.OtherActor = contact.ActorA;
                overlapEvent.OtherBody = contact.ComponentA;

                if ( bFirstContact ) {
                    contact.ComponentB->E_OnBeginOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                } else {
                    contact.ComponentB->E_OnUpdateOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
                }
            }
        }
    }

    // Dispatch contact and overlap events (OnEndContact, OnEndOverlap)
    for ( int i = 0; i < prevContacts.Size(); i++ ) {
        SCollisionContact & contact = prevContacts[ i ];

        int hash = contact.Hash();
        bool bHaveContact = false;

        for ( int h = contactHash.First( hash ); h != -1; h = contactHash.Next( h ) ) {
            if ( currentContacts[ h ].ComponentA->Id == contact.ComponentA->Id
                && currentContacts[ h ].ComponentB->Id == contact.ComponentB->Id ) {
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
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ActorA->E_OnEndContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
            }

        } else if ( contact.bActorADispatchOverlapEvents ) {
            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            contact.ActorA->E_OnEndOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
        }

        if ( contact.bComponentADispatchContactEvents ) {

            if ( contact.ComponentA->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorA;
                contactEvent.SelfBody = contact.ComponentA;
                contactEvent.OtherActor = contact.ActorB;
                contactEvent.OtherBody = contact.ComponentB;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ComponentA->E_OnEndContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
            }
        } else if ( contact.bComponentADispatchOverlapEvents ) {

            overlapEvent.SelfActor = contact.ActorA;
            overlapEvent.SelfBody = contact.ComponentA;
            overlapEvent.OtherActor = contact.ActorB;
            overlapEvent.OtherBody = contact.ComponentB;

            contact.ComponentA->E_OnEndOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
        }

        if ( contact.bActorBDispatchContactEvents ) {

            if ( contact.ActorB->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ActorB->E_OnEndContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
            }
        } else if ( contact.bActorBDispatchOverlapEvents ) {
            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            contact.ActorB->E_OnEndOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
        }

        if ( contact.bComponentBDispatchContactEvents ) {

            if ( contact.ComponentB->E_OnEndContact ) {
                contactEvent.SelfActor = contact.ActorB;
                contactEvent.SelfBody = contact.ComponentB;
                contactEvent.OtherActor = contact.ActorA;
                contactEvent.OtherBody = contact.ComponentA;
                contactEvent.Points = nullptr;
                contactEvent.NumPoints = 0;

                contact.ComponentB->E_OnEndContact.DispatchConditional( SDispatchContactCondition( contactEvent ), contactEvent );
            }
        } else if ( contact.bComponentBDispatchOverlapEvents ) {

            overlapEvent.SelfActor = contact.ActorB;
            overlapEvent.SelfBody = contact.ComponentB;
            overlapEvent.OtherActor = contact.ActorA;
            overlapEvent.OtherBody = contact.ComponentA;

            contact.ComponentB->E_OnEndOverlap.DispatchConditional( SDispatchOverlapCondition( overlapEvent ), overlapEvent );
        }
    }
}

void AWorldPhysics::OnPrePhysics( btDynamicsWorld * _World, float _TimeStep )
{
    AWorldPhysics * self = static_cast< AWorldPhysics * >( _World->getWorldUserInfo() );

    //self->AddPendingBodies();

    self->PrePhysicsCallback( _TimeStep );
}

void AWorldPhysics::OnPostPhysics( btDynamicsWorld * _World, float _TimeStep )
{
    AWorldPhysics * self = static_cast< AWorldPhysics * >( _World->getWorldUserInfo() );

    self->DispatchContactAndOverlapEvents();

    self->PostPhysicsCallback( _TimeStep );
    self->FixedTickNumber++;
}

void AWorldPhysics::Simulate( float _TimeStep )
{
    AddPendingBodies();

    if ( com_NoPhysicsSimulation ) {
        return;
    }

    const float FixedTimeStep = 1.0f / PhysicsHertz;

    int maxSubSteps = Math::ToIntFast( Math::Floor( _TimeStep * PhysicsHertz ) ) + 1;
    //maxSubSteps = Math::Min( maxSubSteps, MAX_SIMULATION_SUB_STEPS );

    btContactSolverInfo & contactSolverInfo = DynamicsWorld->getSolverInfo();
    contactSolverInfo.m_numIterations = Math::Clamp( NumContactSolverIterations, 1, 256 );
    contactSolverInfo.m_splitImpulse = bContactSolverSplitImpulse;

    // Update world gravity
    if ( bGravityDirty ) {
        DynamicsWorld->setGravity( btVectorToFloat3( GravityVector ) );
        bGravityDirty = false;
    }

    // Simulation
    bDuringPhysicsUpdate = true;
    DynamicsWorld->stepSimulation( _TimeStep, maxSubSteps, FixedTimeStep );
    bDuringPhysicsUpdate = false;

#ifdef SOFT_BODY_WORLD
    SoftBodyWorldInfo->m_sparsesdf.GarbageCollect();
#endif
}

void AWorldPhysics::DrawDebug( ADebugRenderer * InRenderer )
{
    int mode = 0;
    if ( com_DrawContactPoints ) {
        mode |= btIDebugDraw::DBG_DrawContactPoints;
    }
    if ( com_DrawConstraints ) {
        mode |= btIDebugDraw::DBG_DrawConstraints;
    }
    if ( com_DrawConstraintLimits ) {
        mode |= btIDebugDraw::DBG_DrawConstraintLimits;
    }
    if ( mode ) {
        class ABulletDebugDraw : public btIDebugDraw
        {
        public:
            ADebugRenderer * Renderer;
            int DebugMode;

            void drawLine( btVector3 const & from, btVector3 const & to, btVector3 const & color ) override
            {
                Renderer->SetColor( Color4( color.x(), color.y(), color.z(), 1.0f ) );
                Renderer->DrawLine( btVectorToFloat3( from ), btVectorToFloat3( to ) );
            }

            void drawContactPoint( btVector3 const & pointOnB, btVector3 const & normalOnB, btScalar distance, int lifeTime, btVector3 const & color ) override
            {
                Renderer->SetColor( Color4( color.x(), color.y(), color.z(), 1.0f ) );
                Renderer->DrawPoint( btVectorToFloat3( pointOnB ) );
                Renderer->DrawPoint( btVectorToFloat3( normalOnB ) );
            }

            void reportErrorWarning( const char * warningString ) override
            {
            }

            void draw3dText( btVector3 const & location, const char * textString ) override
            {
            }

            void setDebugMode( int debugMode ) override
            {
                DebugMode = debugMode;
            }

            int getDebugMode() const override
            {
                return DebugMode;
            }

            void flushLines() override
            {
            }
        };
        static ABulletDebugDraw debugDrawer;

        InRenderer->SetDepthTest( false );
        debugDrawer.Renderer = InRenderer;
        debugDrawer.setDebugMode( mode );
        DynamicsWorld->setDebugDrawer( &debugDrawer );
        DynamicsWorld->debugDrawWorld();
    }
}

static bool CompareDistance( SCollisionTraceResult const & A, SCollisionTraceResult const & B )
{
    return A.Distance < B.Distance;
}

static bool FindCollisionActor( SCollisionQueryFilter const & _QueryFilter, AActor * _Actor )
{
    for ( int i = 0; i < _QueryFilter.ActorsCount; i++ ) {
        if ( _Actor == _QueryFilter.IgnoreActors[ i ] ) {
            return true;
        }
    }
    return false;
}

static bool FindCollisionBody( SCollisionQueryFilter const & _QueryFilter, ASceneComponent * _Body )
{
    for ( int i = 0; i < _QueryFilter.BodiesCount; i++ ) {
        if ( _Body->Id == _QueryFilter.IgnoreBodies[ i ]->Id ) {
            return true;
        }
    }
    return false;
}

AN_FORCEINLINE static bool NeedsCollision( SCollisionQueryFilter const & _QueryFilter, btBroadphaseProxy * _Proxy )
{
    btCollisionObject * collisionObject = static_cast< btCollisionObject * >( _Proxy->m_clientObject );

    AHitProxy * hitProxy = static_cast< AHitProxy * >( collisionObject->getUserPointer() );

    if ( !hitProxy ) {
        // something bad
        return false;
    }

    if ( FindCollisionActor( _QueryFilter, hitProxy->GetOwnerActor() ) ) {
        return false;
    }

    if ( FindCollisionBody( _QueryFilter, hitProxy->GetOwnerComponent() ) ) {
        return false;
    }

    return ( _Proxy->m_collisionFilterGroup & _QueryFilter.CollisionMask ) && _Proxy->m_collisionFilterMask;
}

#if 0
static bool CullTriangle( btCollisionObject const * Object, btCollisionWorld::LocalShapeInfo const * LocalShapeInfo, btVector3 const & HitNormal, bool bNormalInWorldSpace, int TriangleFaceCull )
{
    if ( TriangleFaceCull == COLLISION_TRIANGLE_CULL_NONE ) {
        return false;
    }

    // TODO: TERRAIN_SHAPE_PROXYTYPE

    if ( Object->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE
         || Object->getCollisionShape()->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE )
    {
        const btBvhTriangleMeshShape * trimesh;

        if ( Object->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE ) {
            trimesh = static_cast< const btScaledBvhTriangleMeshShape * >( Object->getCollisionShape() )->getChildShape();
        } else {
            trimesh = static_cast< const btBvhTriangleMeshShape * >( Object->getCollisionShape() );
        }

        const Float3 * pVertices;
        int VertexCount;
        PHY_ScalarType VertexType;
        int VertexStride;
        const unsigned int *pIndices;
        int IndexStride;
        int FaceCount;
        PHY_ScalarType IndexType;

        trimesh->getMeshInterface()->getLockedReadOnlyVertexIndexBase((const byte **)&pVertices,
                                                                      VertexCount,
                                                                      VertexType,
                                                                      VertexStride,
                                                                      (const byte **)&pIndices,
                                                                      IndexStride,
                                                                      FaceCount,
                                                                      IndexType,
                                                                      LocalShapeInfo->m_shapePart);

        AN_ASSERT(VertexType==PHY_FLOAT);
        AN_ASSERT(IndexType==PHY_INTEGER);
        AN_ASSERT(VertexStride==sizeof(Float3));
        AN_ASSERT(IndexStride==sizeof(unsigned int)*3);

        const unsigned int * triangleIndices = &pIndices[ LocalShapeInfo->m_triangleIndex * 3 ];

        Float3 v0 = pVertices[ triangleIndices[0] ];
        Float3 v1 = pVertices[ triangleIndices[1] ];
        Float3 v2 = pVertices[ triangleIndices[2] ];

        Float3 normal = Math::Cross( v1-v0, v2-v0 ).Normalized();
        btVector3 triangleNormal = Object->getWorldTransform().getBasis() * btVectorToFloat3( normal );

        btVector3 contactNormal = bNormalInWorldSpace
                ? HitNormal
                : Object->getWorldTransform().getBasis() * HitNormal;

        const float dp = btDot( contactNormal, triangleNormal );
        //const float eps = 0.001f;

        if ( TriangleFaceCull == COLLISION_TRIANGLE_CULL_BACKFACE ) {
            //return dp < 1.0f - eps;
            return dp < 0.0f;
        }

        // front face
        //return dp > -1.0f + eps;
        return dp > 0.0f;
    }

    return false;
}
#endif

struct STraceRayResultCallback : btCollisionWorld::RayResultCallback
{
    STraceRayResultCallback( SCollisionQueryFilter const * _QueryFilter, Float3 const & _RayStart, Float3 const & _RayDir, TPodVector< SCollisionTraceResult > & _Result )
        : RayLength( _RayDir.Length() )
        , RayStart( _RayStart )
        , RayDir( _RayDir )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
        , Result( _Result )
    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;

        if ( QueryFilter.bCullBackFace ) {
            m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        }

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

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btCollisionWorld::LocalRayResult & RayResult, bool bNormalInWorldSpace ) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( RayResult.m_collisionObject, RayResult.m_localShapeInfo, RayResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        const btCollisionObject * hitCollisionObject = RayResult.m_collisionObject;

        SCollisionTraceResult & hit = Result.Append();
        hit.Clear();
        hit.HitProxy = static_cast< AHitProxy * >(hitCollisionObject->getUserPointer());
        hit.Position = RayStart + RayResult.m_hitFraction * RayDir;
        hit.Normal = bNormalInWorldSpace
                ? btVectorToFloat3( RayResult.m_hitNormalLocal )
                : btVectorToFloat3( hitCollisionObject->getWorldTransform().getBasis()*RayResult.m_hitNormalLocal );
        hit.Distance = RayResult.m_hitFraction * RayLength;
        hit.Fraction = RayResult.m_hitFraction;

        return m_closestHitFraction;
    }

    float RayLength;
    Float3 RayStart;
    Float3 RayDir;
    SCollisionQueryFilter const & QueryFilter;
    TPodVector< SCollisionTraceResult > & Result;
};

struct STraceClosestRayResultCallback : btCollisionWorld::RayResultCallback
{
    STraceClosestRayResultCallback( SCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
        , m_rayFromWorld( rayFromWorld )
        , m_rayToWorld( rayToWorld )

    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;

        if ( QueryFilter.bCullBackFace ) {
            m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        }

        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btCollisionWorld::LocalRayResult & RayResult, bool bNormalInWorldSpace ) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( RayResult.m_collisionObject, RayResult.m_localShapeInfo, RayResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        // caller already does the filter on the m_closestHitFraction
        btAssert( RayResult.m_hitFraction <= m_closestHitFraction );

        m_closestHitFraction = RayResult.m_hitFraction;
        m_collisionObject = RayResult.m_collisionObject;
        m_hitNormalWorld = bNormalInWorldSpace
                ? RayResult.m_hitNormalLocal
                : m_collisionObject->getWorldTransform().getBasis()*RayResult.m_hitNormalLocal;
        m_hitPointWorld.setInterpolate3(m_rayFromWorld,m_rayToWorld,RayResult.m_hitFraction);
        return RayResult.m_hitFraction;
    }

    SCollisionQueryFilter const & QueryFilter;
    btVector3 m_rayFromWorld;
    btVector3 m_rayToWorld;
    btVector3 m_hitPointWorld;
    btVector3 m_hitNormalWorld;
};

struct STraceClosestConvexResultCallback : btCollisionWorld::ConvexResultCallback
{
    STraceClosestConvexResultCallback( SCollisionQueryFilter const * _QueryFilter )
        : m_hitCollisionObject(0)
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }


    btVector3 m_hitNormalWorld;
    btVector3 m_hitPointWorld;
    const btCollisionObject * m_hitCollisionObject;

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btCollisionWorld::LocalConvexResult & ConvexResult, bool bNormalInWorldSpace ) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( ConvexResult.m_hitCollisionObject, ConvexResult.m_localShapeInfo, ConvexResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        // caller already does the filter on the m_closestHitFraction
        btAssert( ConvexResult.m_hitFraction <= m_closestHitFraction );

        m_closestHitFraction = ConvexResult.m_hitFraction;
        m_hitCollisionObject = ConvexResult.m_hitCollisionObject;
        if ( bNormalInWorldSpace ) {
            m_hitNormalWorld = ConvexResult.m_hitNormalLocal;
        } else {
            // need to transform normal into worldspace
            m_hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis() * ConvexResult.m_hitNormalLocal;
        }
        m_hitPointWorld = ConvexResult.m_hitPointLocal;
        return ConvexResult.m_hitFraction;
    }

    SCollisionQueryFilter const & QueryFilter;
};

struct STraceConvexResultCallback : btCollisionWorld::ConvexResultCallback
{
    STraceConvexResultCallback( SCollisionQueryFilter const * _QueryFilter, float _RayLength, TPodVector< SCollisionTraceResult > & _Result )
        : RayLength( _RayLength )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
        , Result( _Result )
    {
        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btCollisionWorld::LocalConvexResult & ConvexResult, bool bNormalInWorldSpace ) override
    {
        // Ignore triangle edge collision
        //if ( CullTriangle( ConvexResult.m_hitCollisionObject, ConvexResult.m_localShapeInfo, ConvexResult.m_hitNormalLocal, bNormalInWorldSpace, QueryFilter.TriangleFaceCulling ) ) {
        //    return 1;
        //}

        const btCollisionObject * hitCollisionObject = ConvexResult.m_hitCollisionObject;

        SCollisionTraceResult & hit = Result.Append();
        hit.Clear();
        hit.HitProxy = static_cast< AHitProxy * >( hitCollisionObject->getUserPointer() );
        hit.Position = btVectorToFloat3( ConvexResult.m_hitPointLocal );
        hit.Normal = bNormalInWorldSpace
                ? btVectorToFloat3( ConvexResult.m_hitNormalLocal )
                : btVectorToFloat3( hitCollisionObject->getWorldTransform().getBasis()*ConvexResult.m_hitNormalLocal );
        hit.Distance = ConvexResult.m_hitFraction * RayLength;
        hit.Fraction = ConvexResult.m_hitFraction;

        return m_closestHitFraction;
    }

    float RayLength;
    SCollisionQueryFilter const & QueryFilter;
    TPodVector< SCollisionTraceResult > & Result;
};

bool AWorldPhysics::Trace( TPodVector< SCollisionTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    if ( !_QueryFilter ) {
        _QueryFilter = &DefaultCollisionQueryFilter;
    }

    _Result.Clear();

    Float3 rayDir = _RayEnd - _RayStart;

    STraceRayResultCallback hitResult( _QueryFilter, _RayStart, rayDir, _Result );

    DynamicsWorld->rayTest( btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ), hitResult );

    if ( _QueryFilter->bSortByDistance ) {
        std::sort( _Result.Begin(), _Result.End(), CompareDistance );
    }

    return !_Result.IsEmpty();
}

bool AWorldPhysics::TraceClosest( SCollisionTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    STraceClosestRayResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    DynamicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    _Result.Clear();

    if ( !hitResult.hasHit() ) {
        return false;
    }

    _Result.HitProxy = static_cast< AHitProxy * >(hitResult.m_collisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = ( _Result.Position - _RayStart ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldPhysics::TraceSphere( SCollisionTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    STraceClosestConvexResultCallback hitResult( _QueryFilter );

    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );

    DynamicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _RayStart ) ),
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _RayEnd ) ), hitResult );

    _Result.Clear();

    if ( !hitResult.hasHit() ) {
        return false;
    }

    _Result.HitProxy = static_cast< AHitProxy * >(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( _RayEnd - _RayStart ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldPhysics::TraceBox( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    STraceClosestConvexResultCallback hitResult( _QueryFilter );

    btBoxShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

    DynamicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( startPos ) ),
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( endPos ) ), hitResult );

    _Result.Clear();

    if ( !hitResult.hasHit() ) {
        return false;
    }

    _Result.HitProxy = static_cast< AHitProxy * >(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

// TODO: Check TraceBox2 and add TraceSphere2, TraceCylinder2 etc
bool AWorldPhysics::TraceBox2( TPodVector< SCollisionTraceResult > & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;
    float rayLength = ( endPos - startPos ).Length();

    _Result.Clear();

    STraceConvexResultCallback hitResult( _QueryFilter, rayLength, _Result );

    btBoxShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

    DynamicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( startPos ) ),
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( endPos ) ), hitResult );

    return !_Result.IsEmpty();
}

bool AWorldPhysics::TraceCylinder( SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    STraceClosestConvexResultCallback hitResult( _QueryFilter );

    btCylinderShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

    DynamicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( startPos ) ),
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( endPos ) ), hitResult );

    _Result.Clear();

    if ( !hitResult.hasHit() ) {
        return false;
    }

    _Result.HitProxy = static_cast< AHitProxy * >(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldPhysics::TraceCapsule( SCollisionTraceResult & _Result, float _CapsuleHeight, float CapsuleRadius, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) const
{
    STraceClosestConvexResultCallback hitResult( _QueryFilter );

    btCapsuleShape shape( CapsuleRadius, _CapsuleHeight );
    shape.setMargin( 0.0f );

    DynamicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _RayStart ) ),
        btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _RayEnd ) ), hitResult );

    _Result.Clear();

    if ( !hitResult.hasHit() ) {
        return false;
    }

    _Result.HitProxy = static_cast< AHitProxy * >(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * (_RayEnd - _RayStart).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldPhysics::TraceConvex( SCollisionTraceResult & _Result, SConvexSweepTest const & _SweepTest ) const
{
    _Result.Clear();

    if ( !_SweepTest.CollisionBody->IsConvex() ) {
        GLogger.Printf( "AWorld::TraceConvex: non-convex collision body for convex trace\n" );
        return false;
    }

    TUniqueRef< btCollisionShape > shape( _SweepTest.CollisionBody->Create() );
    shape->setMargin( _SweepTest.CollisionBody->Margin );

    AN_ASSERT( shape->isConvex() );

    Float3x4 startTransform, endTransform;

    startTransform.Compose( _SweepTest.StartPosition, _SweepTest.StartRotation.ToMatrix3x3(), _SweepTest.Scale );
    endTransform.Compose(_SweepTest.EndPosition, _SweepTest.EndRotation.ToMatrix3x3(), _SweepTest.Scale);

    Float3 startPos = startTransform * _SweepTest.CollisionBody->Position;
    Float3 endPos = endTransform * _SweepTest.CollisionBody->Position;
    Quat startRot = _SweepTest.StartRotation * _SweepTest.CollisionBody->Rotation;
    Quat endRot = _SweepTest.EndRotation * _SweepTest.CollisionBody->Rotation;

    STraceClosestConvexResultCallback hitResult( &_SweepTest.QueryFilter );

    DynamicsWorld->convexSweepTest( static_cast< btConvexShape * >( shape.GetObject() ),
        btTransform( btQuaternionToQuat( startRot ), btVectorToFloat3( startPos ) ),
        btTransform( btQuaternionToQuat( endRot ), btVectorToFloat3( endPos ) ), hitResult );

    if ( !hitResult.hasHit() ) {
        return false;
    }

    _Result.HitProxy = static_cast< AHitProxy * >(hitResult.m_hitCollisionObject->getUserPointer());
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

struct SQueryCollisionObjectsCallback : public btCollisionWorld::ContactResultCallback
{
    SQueryCollisionObjectsCallback( TPodVector< AHitProxy * > & _Result, SCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override
    {
        AHitProxy * hitProxy;

        hitProxy = static_cast<AHitProxy *>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if ( hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask) ) {
            AddUnique( hitProxy );
        }

        hitProxy = static_cast<AHitProxy *>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if ( hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask) ) {
            AddUnique( hitProxy );
        }

        return 0.0f;
    }

    void AddUnique( AHitProxy * HitProxy )
    {
        for ( AHitProxy * proxy : Result ) {
            if ( proxy->Id == HitProxy->Id ) {
                return;
            }
        }
        Result.Append( HitProxy );
    }

    TPodVector< AHitProxy * > & Result;
    SCollisionQueryFilter const & QueryFilter;
};

struct SQueryCollisionCallback : public btCollisionWorld::ContactResultCallback
{
    SQueryCollisionCallback( TPodVector< SCollisionQueryResult > & _Result, SCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override
    {
        AHitProxy * hitProxy;

        hitProxy = static_cast<AHitProxy *>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if ( hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask) ) {
            AddContact( hitProxy, cp );
        }

        hitProxy = static_cast<AHitProxy *>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if ( hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask) ) {
            AddContact( hitProxy, cp );
        }

        return 0.0f;
    }

    void AddContact( AHitProxy * HitProxy, btManifoldPoint & cp )
    {
        SCollisionQueryResult & contact = Result.Append();

        contact.HitProxy = HitProxy;
        contact.Position = btVectorToFloat3( cp.m_positionWorldOnB );
        contact.Normal = btVectorToFloat3( cp.m_normalWorldOnB );
        contact.Distance = cp.m_distance1; // FIXME?
        //contact.Fraction; // FIXME

        //contact.Fraction = contact.Distance / RayLength;

        //contact.Impulse = cp.m_appliedImpulse;
    }

    TPodVector< SCollisionQueryResult > & Result;
    SCollisionQueryFilter const & QueryFilter;
};

struct SQueryActorsCallback : public btCollisionWorld::ContactResultCallback
{
    SQueryActorsCallback( TPodVector< AActor * > & _Result, SCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = CM_ALL;
        m_collisionFilterMask = QueryFilter.CollisionMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override
    {
        AHitProxy * hitProxy;

        hitProxy = static_cast<AHitProxy *>(colObj0Wrap->getCollisionObject()->getUserPointer());
        if ( hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask) ) {
            AddUnique( hitProxy->GetOwnerActor() );
        }

        hitProxy = static_cast<AHitProxy *>(colObj1Wrap->getCollisionObject()->getUserPointer());
        if ( hitProxy && (hitProxy->GetCollisionGroup() & QueryFilter.CollisionMask) ) {
            AddUnique( hitProxy->GetOwnerActor() );
        }

        return 0.0f;
    }

    void AddUnique( AActor * Actor )
    {
        if ( Result.Find( Actor ) == Result.End() ) {
            Result.Append( Actor );
        }
    }

    TPodVector< AActor * > & Result;
    SCollisionQueryFilter const & QueryFilter;
};

static void CollisionShapeContactTest( btDiscreteDynamicsWorld const * InWorld, Float3 const & InPosition, btCollisionShape * InShape, btCollisionWorld::ContactResultCallback & InCallback )
{
    TUniqueRef< btRigidBody > tempBody = MakeUnique< btRigidBody >( 0.0f, nullptr, InShape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( InPosition ) ) );
//    tempBody->activate();
    btDiscreteDynamicsWorld * physWorld = const_cast< btDiscreteDynamicsWorld * >( InWorld );
//    physWorld->addRigidBody( tempBody );
    physWorld->contactTest( tempBody.GetObject(), InCallback );
//    physWorld->removeRigidBody( tempBody );
}

void AWorldPhysics::QueryHitProxies_Sphere( TPodVector< AHitProxy * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter ) const
{
    SQueryCollisionObjectsCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    CollisionShapeContactTest( DynamicsWorld.GetObject(), _Position, &shape, callback );
}

void AWorldPhysics::QueryHitProxies_Box( TPodVector< AHitProxy * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter ) const
{
    SQueryCollisionObjectsCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    CollisionShapeContactTest( DynamicsWorld.GetObject(), _Position, &shape, callback );
}

void AWorldPhysics::QueryHitProxies( TPodVector< AHitProxy * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter ) const
{
    QueryHitProxies_Box( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}

void AWorldPhysics::QueryActors_Sphere( TPodVector< AActor * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter ) const
{
    SQueryActorsCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    CollisionShapeContactTest( DynamicsWorld.GetObject(), _Position, &shape, callback );
}

void AWorldPhysics::QueryActors_Box( TPodVector< AActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter ) const
{
    SQueryActorsCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    CollisionShapeContactTest( DynamicsWorld.GetObject(), _Position, &shape, callback );
}

void AWorldPhysics::QueryActors( TPodVector< AActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter ) const
{
    QueryActors_Box( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}

void AWorldPhysics::QueryCollision_Sphere( TPodVector< SCollisionQueryResult > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter ) const
{
    SQueryCollisionCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    CollisionShapeContactTest( DynamicsWorld.GetObject(), _Position, &shape, callback );
}

void AWorldPhysics::QueryCollision_Box( TPodVector< SCollisionQueryResult > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter ) const
{
    SQueryCollisionCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    CollisionShapeContactTest( DynamicsWorld.GetObject(), _Position, &shape, callback );
}

void AWorldPhysics::QueryCollision( TPodVector< SCollisionQueryResult > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter ) const
{
    QueryCollision_Box( _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}
