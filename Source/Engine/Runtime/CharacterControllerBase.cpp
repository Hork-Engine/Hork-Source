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

#include "CharacterControllerBase.h"
#include "World.h"
#include "RuntimeVariable.h"
#include "BulletCompatibility.h"

#include <BulletDynamics/Dynamics/btActionInterface.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h>

ARuntimeVariable com_DrawCharacterControllerCapsule( _CTS( "com_DrawCharacterControllerCapsule" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable com_RecoverFromPenetration( _CTS( "com_RecoverFromPenetration" ), _CTS( "1" ) );
ARuntimeVariable com_UseGhostObjectSweepTest( _CTS( "com_UseGhostObjectSweepTest" ), _CTS( "1" ) );
ARuntimeVariable com_CharacterCcdPenetration( _CTS( "com_CharacterCcdPenetration" ), _CTS( "0" ) );

ATTRIBUTE_ALIGNED16( class )
ACharacterControllerActionInterface : public btActionInterface
{
public:
    BT_DECLARE_ALIGNED_ALLOCATOR();

    ACharacterControllerBase * CharacterController;

    btManifoldArray ManifoldArray;

    // btActionInterface interface
    void updateAction( btCollisionWorld * collisionWorld, btScalar deltaTime ) override
    {
        CharacterController->_Update( deltaTime );
    }

    // btActionInterface interface
    void debugDraw( btIDebugDraw * debugDrawer ) override
    {
    }
};


AN_CLASS_META( ACharacterControllerBase )

ACharacterControllerBase::ACharacterControllerBase()
{
    HitProxy = CreateInstanceOf< AHitProxy >();
    HitProxy->SetCollisionGroup( CM_CHARACTER_CONTROLLER );
    HitProxy->SetCollisionMask( CM_ALL );

    AnglePitch = 0;
    AngleYaw = 0;

    SetAbsoluteScale( true );
}

void ACharacterControllerBase::InitializeComponent()
{
    Super::InitializeComponent();

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin( btVectorToFloat3( GetWorldPosition() + Float3( 0, GetCharacterHeight() * 0.5f, 0 ) ) );

    // Just a bridge between the character controller and btActionInterface
    ActionInterface = new ACharacterControllerActionInterface;
    ActionInterface->CharacterController = this;

    btVector3 halfExtents;
    halfExtents[0] = halfExtents[2] = CapsuleRadius;
    halfExtents[1] = GetCharacterHeight() * 0.5f;
    CylinderShape = new btCylinderShape( halfExtents );

    ConvexShape = new btCapsuleShape( CapsuleRadius, CapsuleHeight );

    bNeedToUpdateCapsule = false;

    World = GetWorld()->GetPhysics().GetInternal();

    GhostObject = new btPairCachingGhostObject;
    GhostObject->setUserPointer( HitProxy.GetObject() );
    GhostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
    GhostObject->setWorldTransform( startTransform );
    GhostObject->setCollisionShape( ConvexShape );

    World->addAction( ActionInterface );

    HitProxy->Initialize( this, GhostObject );
}

void ACharacterControllerBase::DeinitializeComponent()
{
    HitProxy->Deinitialize();

    World->removeAction( ActionInterface );

    delete ActionInterface;
    delete GhostObject;
    delete ConvexShape;
    delete CylinderShape;
    
    Super::DeinitializeComponent();
}

void ACharacterControllerBase::BeginPlay()
{
    Super::BeginPlay();

    CalcYawAndPitch( AngleYaw, AnglePitch );

    // Set angles without roll
    SetWorldRotation( GetAngleQuaternion() );
}

void ACharacterControllerBase::OnTransformDirty()
{
    Super::OnTransformDirty();

    if ( IsInitialized() && !bInsideUpdate )
    {
        SetCapsuleWorldPosition( GetWorldPosition() );

        // Sync yaw and pitch with current rotation
        CalcYawAndPitch( AngleYaw, AnglePitch );
    }
}

void ACharacterControllerBase::CalcYawAndPitch( float & Yaw, float & Pitch )
{
    Float3 right = GetWorldRightVector();
    right.Y = 0.0f; // remove roll
    float len = right.NormalizeSelf();
    if ( len < 0.5f ) {
        // can't calc yaw
        right = Float3( 1, 0, 0 );
    }

    Float3 forward = GetWorldForwardVector();

    Yaw = Math::Degrees( std::atan2( -right.Z, right.X ) );
    Yaw = Angl::Normalize180( Yaw );

    Pitch = Math::Clamp( Math::Degrees( std::acos( Math::Clamp( -forward.Y, -1.0f, 1.0f ) ) ) - 90.0f, -90.0f, 90.0f );
}

void ACharacterControllerBase::SetCharacterYaw( float _Yaw )
{
    AngleYaw = Angl::Normalize180( _Yaw );
    SetWorldRotation( GetAngleQuaternion() );
}

void ACharacterControllerBase::SetCharacterPitch( float _Pitch )
{
    AnglePitch = Math::Clamp( _Pitch, -90.0f, 90.0f );
    SetWorldRotation( GetAngleQuaternion() );
}

Quat ACharacterControllerBase::GetAngleQuaternion() const
{
    float sx, sy, cx, cy;

    Math::DegSinCos( AnglePitch * 0.5f, sx, cx );
    Math::DegSinCos( AngleYaw * 0.5f, sy, cy );

    return Quat( cy*cx, cy*sx, sy*cx, -sy*sx );
}

Float3 ACharacterControllerBase::GetCenterWorldPosition() const
{
    Float3 worldPosition = GetWorldPosition();
    worldPosition[1] += GetCharacterHeight() * 0.5f;
    return worldPosition;
}

void ACharacterControllerBase::SetCollisionGroup( int _CollisionGroup )
{
    HitProxy->SetCollisionGroup( _CollisionGroup );
}

void ACharacterControllerBase::SetCollisionMask( int _CollisionMask )
{
    HitProxy->SetCollisionMask( _CollisionMask );
}

void ACharacterControllerBase::SetCollisionFilter( int _CollisionGroup, int _CollisionMask )
{
    HitProxy->SetCollisionFilter( _CollisionGroup, _CollisionMask );
}

void ACharacterControllerBase::AddCollisionIgnoreActor( AActor * _Actor )
{
    HitProxy->AddCollisionIgnoreActor( _Actor );
}

void ACharacterControllerBase::RemoveCollisionIgnoreActor( AActor * _Actor )
{
    HitProxy->RemoveCollisionIgnoreActor( _Actor );
}

void ACharacterControllerBase::UpdateCapsuleShape()
{
    if ( !bNeedToUpdateCapsule )
    {
        return;
    }

    delete ConvexShape;

    // TODO: Test this
    ConvexShape = new btCapsuleShape( CapsuleRadius, CapsuleHeight );
    GhostObject->setCollisionShape( ConvexShape );

    bNeedToUpdateCapsule = false;
}

void ACharacterControllerBase::SetCapsuleWorldPosition( Float3 const & InPosition )
{
    btTransform transform = GhostObject->getWorldTransform();
    btVector3 position = btVectorToFloat3( InPosition + Float3( 0, GetCharacterHeight()*0.5f, 0 ) );
    if ( (transform.getOrigin() - position).length2() > FLT_EPSILON ) {
        transform.setOrigin( position );
        GhostObject->setWorldTransform( transform );
    }
}

void ACharacterControllerBase::_Update( float _TimeStep )
{
    if ( !GhostObject->getBroadphaseHandle() ) {
        // Collision object was not added to the world yet
        return;
    }

    bInsideUpdate = true;

    UpdateCapsuleShape();

    Update( _TimeStep );

    bInsideUpdate = false;
}

class ACharacterControllerTraceCallback : public btCollisionWorld::ConvexResultCallback
{
    using Super = btCollisionWorld::ConvexResultCallback;

public:
    ACharacterControllerTraceCallback( btCollisionObject * _Self, Float3 const & _UpVec, float _MinSlopeDot )
        : HitProxy( nullptr )
        , Self( _Self )
        , UpVec( btVectorToFloat3( _UpVec ) )
        , MinSlopeDot( _MinSlopeDot )
    {
        m_collisionFilterGroup = _Self->getBroadphaseHandle()->m_collisionFilterGroup;
        m_collisionFilterMask = _Self->getBroadphaseHandle()->m_collisionFilterMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        if ( !Super::needsCollision( proxy0 ) ) {
            return false;
        }

        AHitProxy const * hitProxy0 = static_cast< AHitProxy const * >(Self->getUserPointer());
        AHitProxy const * hitProxy1 = static_cast< AHitProxy const * >(static_cast< btCollisionObject const * >(proxy0->m_clientObject)->getUserPointer());

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

    btScalar addSingleResult( btCollisionWorld::LocalConvexResult & result, bool normalInWorldSpace ) override
    {
        if ( result.m_hitCollisionObject == Self )
        {
            return 1;
        }

        if ( !result.m_hitCollisionObject->hasContactResponse() )
        {
            return 1;
        }

        btVector3 hitNormalWorld = normalInWorldSpace
            ? result.m_hitNormalLocal
            : result.m_hitCollisionObject->getWorldTransform().getBasis() * result.m_hitNormalLocal;

        btScalar dotUp = UpVec.dot( hitNormalWorld );
        if ( dotUp < MinSlopeDot )
        {
            return 1;
        }

        AN_ASSERT( result.m_hitFraction <= m_closestHitFraction );

        m_closestHitFraction = result.m_hitFraction;
        HitNormalWorld = hitNormalWorld;
        HitPointWorld = result.m_hitPointLocal;
        HitProxy = static_cast< AHitProxy * >( result.m_hitCollisionObject->getUserPointer() );

        return result.m_hitFraction;
    }

    btVector3 HitNormalWorld;
    btVector3 HitPointWorld;
    AHitProxy * HitProxy;

protected:
    btCollisionObject * Self;
    btVector3 UpVec;
    btScalar MinSlopeDot;
};

class ACharacterControllerTraceNoSlopeCallback : public btCollisionWorld::ConvexResultCallback
{
    using Super = btCollisionWorld::ConvexResultCallback;

public:
    ACharacterControllerTraceNoSlopeCallback( btCollisionObject * _Self )
        : HitProxy( nullptr )
        , Self( _Self )
    {
        m_collisionFilterGroup = _Self->getBroadphaseHandle()->m_collisionFilterGroup;
        m_collisionFilterMask = _Self->getBroadphaseHandle()->m_collisionFilterMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        if ( !Super::needsCollision( proxy0 ) ) {
            return false;
        }

        AHitProxy const * hitProxy0 = static_cast< AHitProxy const * >(Self->getUserPointer());
        AHitProxy const * hitProxy1 = static_cast< AHitProxy const * >(static_cast< btCollisionObject const * >(proxy0->m_clientObject)->getUserPointer());

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

    btScalar addSingleResult( btCollisionWorld::LocalConvexResult & result, bool normalInWorldSpace ) override
    {
        if ( result.m_hitCollisionObject == Self )
        {
            return 1;
        }

        if ( !result.m_hitCollisionObject->hasContactResponse() )
        {
            return 1;
        }

        btVector3 hitNormalWorld = normalInWorldSpace
            ? result.m_hitNormalLocal
            : result.m_hitCollisionObject->getWorldTransform().getBasis() * result.m_hitNormalLocal;

        AN_ASSERT( result.m_hitFraction <= m_closestHitFraction );

        m_closestHitFraction = result.m_hitFraction;
        HitNormalWorld = hitNormalWorld;
        HitPointWorld = result.m_hitPointLocal;
        HitProxy = static_cast< AHitProxy * >( result.m_hitCollisionObject->getUserPointer() );

        return result.m_hitFraction;
    }

    btVector3 HitNormalWorld;
    btVector3 HitPointWorld;
    AHitProxy * HitProxy;

protected:
    btCollisionObject * Self;
};

void ACharacterControllerBase::TraceSelf( Float3 const & Start, Float3 const & End, Float3 const & Up, float MinSlopeDot, SCharacterControllerTrace & Trace, bool bCylinder ) const
{
    ACharacterControllerTraceCallback callback( GhostObject, Up, MinSlopeDot );

    const float ccdPenetration = com_CharacterCcdPenetration.GetFloat();//World->getDispatchInfo().m_allowedCcdPenetration;

    btTransform transformStart, transformEnd;

    transformStart.setOrigin( btVectorToFloat3( Start + Float3( 0, GetCharacterHeight()*0.5f, 0 ) ) );
    transformStart.setBasis( btMatrix3x3::getIdentity() );

    transformEnd.setOrigin( btVectorToFloat3( End + Float3( 0, GetCharacterHeight()*0.5f, 0 ) ) );
    transformEnd.setBasis( btMatrix3x3::getIdentity() );
//bCylinder=true;
    btConvexShape * shape = bCylinder ? CylinderShape : ConvexShape;

    if ( com_UseGhostObjectSweepTest )
    {
        GhostObject->convexSweepTest( shape, transformStart, transformEnd, callback, ccdPenetration );
    }
    else
    {
        World->convexSweepTest( shape, transformStart, transformEnd, callback, ccdPenetration );
    }

    Trace.HitProxy = callback.HitProxy;
    Trace.Position = btVectorToFloat3( callback.HitPointWorld );
    Trace.Normal = btVectorToFloat3( callback.HitNormalWorld );
    Trace.Fraction = callback.m_closestHitFraction;

    AN_ASSERT( GhostObject->hasContactResponse() );
}

void ACharacterControllerBase::TraceSelf( Float3 const & Start, Float3 const & End, SCharacterControllerTrace & Trace, bool bCylinder ) const
{
    ACharacterControllerTraceNoSlopeCallback callback( GhostObject );

    const float ccdPenetration = com_CharacterCcdPenetration.GetFloat();//World->getDispatchInfo().m_allowedCcdPenetration;

    btTransform transformStart, transformEnd;

    transformStart.setOrigin( btVectorToFloat3( Start + Float3( 0, GetCharacterHeight()*0.5f, 0 ) ) );
    transformStart.setBasis( btMatrix3x3::getIdentity() );

    transformEnd.setOrigin( btVectorToFloat3( End + Float3( 0, GetCharacterHeight()*0.5f, 0 ) ) );
    transformEnd.setBasis( btMatrix3x3::getIdentity() );
//bCylinder=true;
    btConvexShape * shape = bCylinder ? CylinderShape : ConvexShape;

    if ( com_UseGhostObjectSweepTest )
    {
        GhostObject->convexSweepTest( shape, transformStart, transformEnd, callback, ccdPenetration );
    }
    else
    {
        World->convexSweepTest( shape, transformStart, transformEnd, callback, ccdPenetration );
    }

    Trace.HitProxy = callback.HitProxy;
    Trace.Position = btVectorToFloat3( callback.HitPointWorld );
    Trace.Normal = btVectorToFloat3( callback.HitNormalWorld );
    Trace.Fraction = callback.m_closestHitFraction;

    AN_ASSERT( GhostObject->hasContactResponse() );
}

void ACharacterControllerBase::RecoverFromPenetration( float MaxPenetrationDepth, int MaxIterations )
{
    //bool touchingContact = false;
    int numPenetrationLoops = 0;
    if ( com_RecoverFromPenetration ) {
        while ( _RecoverFromPenetration( MaxPenetrationDepth ) ) {
            numPenetrationLoops++;
            //touchingContact = true;

            if ( numPenetrationLoops > MaxIterations ) {
                GLogger.Printf( "ACharacterControllerBase::RecoverFromPenetration: couldn't recover from penetration (num iterations %d)\n", numPenetrationLoops );
                break;
            }
        }
    }
    if ( numPenetrationLoops > 0 && numPenetrationLoops <= MaxIterations ) {
        GLogger.Printf( "Recovered from penetration, %d iterations\n", numPenetrationLoops );
    }
}

static bool NeedsCollision( const btCollisionObject* body0, const btCollisionObject* body1 )
{
    // FIXME: Check collision ignore actors?

    bool collides = (body0->getBroadphaseHandle()->m_collisionFilterGroup & body1->getBroadphaseHandle()->m_collisionFilterMask) != 0;
    collides = collides && (body1->getBroadphaseHandle()->m_collisionFilterGroup & body0->getBroadphaseHandle()->m_collisionFilterMask);
    return collides;
}

bool ACharacterControllerBase::_RecoverFromPenetration( float MaxPenetrationDepth )
{
    // Note from btKinematicCharacterController:
    // Here we must refresh the overlapping paircache as the penetrating movement itself or the
    // previous recovery iteration might have used setWorldTransform and pushed us into an object
    // that is not in the previous cache contents from the last timestep, as will happen if we
    // are pushed into a new AABB overlap. Unhandled this means the next convex sweep gets stuck.
    //
    // Do this by calling the broadphase's setAabb with the moved AABB, this will update the broadphase
    // paircache and the ghostobject's internal paircache at the same time.    /BW

    btVector3 minAabb, maxAabb;
    ConvexShape->getAabb( GhostObject->getWorldTransform(), minAabb, maxAabb );
    World->getBroadphase()->setAabb( GhostObject->getBroadphaseHandle(), minAabb, maxAabb, World->getDispatcher() );

    bool penetration = false;

    World->getDispatcher()->dispatchAllCollisionPairs( GhostObject->getOverlappingPairCache(), World->getDispatchInfo(), World->getDispatcher() );

    btVector3 capsulePosition = GhostObject->getWorldTransform().getOrigin();

    btManifoldArray & manifoldArray = ActionInterface->ManifoldArray;

    //	float maxPen = 0.0f;
    for ( int i = 0; i < GhostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++ )
    {
        manifoldArray.resize( 0 );

        btBroadphasePair* collisionPair = &GhostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

        btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
        btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

        if ( (obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse()) )
            continue;

        if ( !NeedsCollision( obj0, obj1 ) )
            continue;

        if ( collisionPair->m_algorithm )
            collisionPair->m_algorithm->getAllContactManifolds( manifoldArray );

        for ( int j = 0; j < manifoldArray.size(); j++ )
        {
            btPersistentManifold* manifold = manifoldArray[j];
            float directionSign = manifold->getBody0() == GhostObject ? -1 : 1;
            for ( int p = 0; p < manifold->getNumContacts(); p++ )
            {
                btManifoldPoint const & pt = manifold->getContactPoint( p );

                float dist = pt.getDistance();

                if ( dist < -MaxPenetrationDepth )
                {
                    // TODO: cause problems on slopes, not sure if it is needed
                    //if (dist < maxPen)
                    //{
                    //	maxPen = dist;
                    //	TouchingNormal = pt.m_normalWorldOnB * directionSign;//??

                    //}
                    capsulePosition += pt.m_normalWorldOnB * directionSign * dist * 0.2f;
                    penetration = true;
                }
                else
                {
                    //GLogger.Printf("touching %f\n", dist);
                }
            }

            //manifold->clearManifold();
        }
    }

    if ( penetration ) {
        Float3 newPosition = btVectorToFloat3( capsulePosition );
        newPosition[1] -= GetCharacterHeight() * 0.5f;

        SetCapsuleWorldPosition( newPosition );

        // Update scene component world position
        SetWorldPosition( newPosition );
    }

    return penetration;
}

void ACharacterControllerBase::SlideMove( Float3 const & StartPos, Float3 const & TargetPos, float TimeStep, Float3 & FinalPos, bool * bClipped, TPodVector< SCharacterControllerContact > * pContacts )
{
    Float3 linearVelocity = ( TargetPos - StartPos ) / TimeStep;
    Float3 finalVelocity;

    SlideMove( StartPos, linearVelocity, TimeStep, FinalPos, finalVelocity, bClipped, pContacts );
}

static AN_FORCEINLINE bool FindHitNormal( Float3 const * ContactNormals, int NumContacts, Float3 const & HitNormal )
{
    for ( int i = 0 ; i < NumContacts ; i++ ) {
        if ( Math::Dot( HitNormal, ContactNormals[i] ) > 0.99f ) {
            // The same plane we hit before
            return true;
        }
    }
    return false;
}

bool ACharacterControllerBase::ClipVelocityByContactNormals( Float3 const * ContactNormals, int NumContacts, Float3 & Velocity )
{
    int i, j;

    for ( i = 0 ; i < NumContacts ; i++ ) {
        ClipVelocity( Velocity, ContactNormals[i], Velocity );

        for ( j = 0 ; j < NumContacts ; j++ ) {
            if ( j != i && Math::Dot( Velocity, ContactNormals[j] ) < 0 ) {
                break;
            }
        }
        if ( j == NumContacts ) {
            return true;
        }
    }

    if ( NumContacts != 2 ) {
        return false;
    }

    // Slide along the crease
    Float3 dir = Math::Cross( ContactNormals[0], ContactNormals[1] );
    Velocity = dir * Math::Dot( dir, Velocity );

    return true;
}

void ACharacterControllerBase::SlideMove( Float3 const & StartPos, Float3 const & LinearVelocity, float _TimeStep, Float3 & FinalPos, Float3 & FinalVelocity, bool * bClipped, TPodVector< SCharacterControllerContact > * pContacts )
{
    const int MAX_CONTACTS = 5;
    Float3 contactNormals[MAX_CONTACTS];
    int numContacts = 0;
    const int MAX_ITERATIONS = 4;    
    int iteration;
    Float3 currentVelocity = LinearVelocity;
    Float3 currentPosition = StartPos;
    float dt = _TimeStep;
    bool clipped = false;
    SCharacterControllerTrace trace;

    for ( iteration = 0 ; iteration < MAX_ITERATIONS ; iteration++ ) {
        Float3 targetPosition = currentPosition + currentVelocity * dt;

        if ( currentPosition != targetPosition ) {
            TraceSelf( currentPosition, targetPosition, trace );

            if ( !trace.HasHit() ) {
                // Moved the entire distance
                break;
            }
        }
        else {
            // Stop moving
            break;
        }

        if ( trace.Fraction > 0 ) {
            // Move only a fraction of the distance
            currentPosition = Math::Lerp( currentPosition, targetPosition, trace.Fraction );

            numContacts = 0;
        }

        // Add touched objects
        if ( pContacts && trace.HitProxy ) {
            SCharacterControllerContact & contact = pContacts->Append();
            contact.HitProxy = trace.HitProxy;
            contact.Position = trace.Position;
            contact.Normal = trace.Normal;
        }

        dt = dt - trace.Fraction * dt;

        if ( numContacts >= MAX_CONTACTS ) {
            currentVelocity.Clear();
            clipped = true;
            break;
        }

        // Find the same plane we hit before
        if ( FindHitNormal( contactNormals, numContacts, trace.Normal ) ) {
            // Nudge the velocity along the hit plane to fix epsilon issues with non-axial planes
            currentVelocity += trace.Normal * 0.03f;
            continue;
        }

        // Add contact
        contactNormals[numContacts] = trace.Normal;
        numContacts++;

        // Clip the velocity
        if ( !ClipVelocityByContactNormals( contactNormals, numContacts, currentVelocity ) ) {
            currentVelocity.Clear();
            clipped = true;
            break;
        }

        // Velocity is against the start velocity
        if ( Math::Dot( currentVelocity, LinearVelocity ) <= 0.0f ) {
            currentVelocity.Clear();
            clipped = true;
            break;
        }
    }

    FinalVelocity = currentVelocity;
    FinalPos = StartPos + FinalVelocity * _TimeStep;

    if ( bClipped ) {
        *bClipped = iteration > 0 || clipped;
    }
}

void ACharacterControllerBase::ClipVelocity( Float3 const & InVelocity, Float3 const & InNormal, Float3 & OutVelocity, float Overbounce )
{
    OutVelocity = Math::ProjectVector( InVelocity, InNormal, Overbounce );

    for ( int i = 0 ; i < 3 ; i++ ) {
        if ( Math::Abs( OutVelocity[i] ) < 0.003f ) {
            OutVelocity[i] = 0;
        }
    }
}

void ACharacterControllerBase::DrawDebug( ADebugRenderer * InRenderer )
{
    Super::DrawDebug( InRenderer );

    if ( com_DrawCharacterControllerCapsule ) {
        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( Color4::White() );
        btDrawCollisionShape( InRenderer, GhostObject->getWorldTransform(), GhostObject->getCollisionShape() );
    }
}






ATTRIBUTE_ALIGNED16( class )
AProjectileActionInterface : public btActionInterface
{
public:
    BT_DECLARE_ALIGNED_ALLOCATOR();

    AProjectileExperimental * Projectile;

    //btManifoldArray ManifoldArray;

    // btActionInterface interface
    void updateAction( btCollisionWorld * collisionWorld, btScalar deltaTime ) override
    {
        Projectile->_Update( deltaTime );
    }

    // btActionInterface interface
    void debugDraw( btIDebugDraw * debugDrawer ) override
    {
    }
};


AN_CLASS_META( AProjectileExperimental )

AProjectileExperimental::AProjectileExperimental()
{
    HitProxy = CreateInstanceOf< AHitProxy >();
    HitProxy->SetCollisionGroup( CM_PROJECTILE );
    HitProxy->SetCollisionMask( CM_ALL );
}

void AProjectileExperimental::InitializeComponent()
{
    Super::InitializeComponent();

    btTransform startTransform;
    startTransform.setOrigin( btVectorToFloat3( GetWorldPosition() ) );
    startTransform.setRotation( btQuaternionToQuat( GetWorldRotation() ) );

    // Just a bridge between the projectile and btActionInterface
    ActionInterface = new AProjectileActionInterface;
    ActionInterface->Projectile = this;

    ConvexShape = new btCapsuleShapeZ( 0.1f, 0.35f );

    World = GetWorld()->GetPhysics().GetInternal();

    //GhostObject = new btPairCachingGhostObject;
    GhostObject = new btGhostObject;
    GhostObject->setUserPointer( HitProxy.GetObject() );
    GhostObject->setCollisionFlags( btCollisionObject::CF_CHARACTER_OBJECT );
    GhostObject->setWorldTransform( startTransform );
    GhostObject->setCollisionShape( ConvexShape );

    World->addAction( ActionInterface );

    HitProxy->Initialize( this, GhostObject );
}

void AProjectileExperimental::DeinitializeComponent()
{
    HitProxy->Deinitialize();

    World->removeAction( ActionInterface );

    delete ActionInterface;
    delete GhostObject;
    delete ConvexShape;

    Super::DeinitializeComponent();
}

void AProjectileExperimental::BeginPlay()
{
    Super::BeginPlay();

    GetWorld()->E_OnPostPhysicsUpdate.Add( this, &AProjectileExperimental::HandlePostPhysicsUpdate );
}

void AProjectileExperimental::EndPlay()
{
    GetWorld()->E_OnPostPhysicsUpdate.Remove( this );
}

void AProjectileExperimental::HandlePostPhysicsUpdate( float TimeStep )
{
    ClearForces();
}

void AProjectileExperimental::ClearForces()
{
    m_totalForce.Clear();
    m_totalTorque.Clear();
}

void AProjectileExperimental::OnTransformDirty()
{
    Super::OnTransformDirty();

    if ( IsInitialized() && !bInsideUpdate )
    {
//        SetCapsuleWorldPosition( GetWorldPosition() );
        btTransform transform = GhostObject->getWorldTransform();
        transform.setOrigin( btVectorToFloat3( GetWorldPosition() ) );
        transform.setRotation( btQuaternionToQuat( GetWorldRotation() ) );
        GhostObject->setWorldTransform( transform );
    }
}

void AProjectileExperimental::SetCollisionGroup( int _CollisionGroup )
{
    HitProxy->SetCollisionGroup( _CollisionGroup );
}

void AProjectileExperimental::SetCollisionMask( int _CollisionMask )
{
    HitProxy->SetCollisionMask( _CollisionMask );
}

void AProjectileExperimental::SetCollisionFilter( int _CollisionGroup, int _CollisionMask )
{
    HitProxy->SetCollisionFilter( _CollisionGroup, _CollisionMask );
}

void AProjectileExperimental::AddCollisionIgnoreActor( AActor * _Actor )
{
    HitProxy->AddCollisionIgnoreActor( _Actor );
}

void AProjectileExperimental::RemoveCollisionIgnoreActor( AActor * _Actor )
{
    HitProxy->RemoveCollisionIgnoreActor( _Actor );
}

//void AProjectileExperimental::UpdateCapsuleShape()
//{
//    if ( !bNeedToUpdateCapsule )
//    {
//        return;
//    }
//
//    delete ConvexShape;
//
//    // TODO: Test this
//    ConvexShape = new btCapsuleShape( CapsuleRadius, CapsuleHeight );
//    GhostObject->setCollisionShape( ConvexShape );
//
//    bNeedToUpdateCapsule = false;
//}

//void AProjectileExperimental::SetCapsuleWorldPosition( Float3 const & InPosition )
//{
//    btTransform transform = GhostObject->getWorldTransform();
//    btVector3 position = btVectorToFloat3( InPosition + Float3( 0, GetCharacterHeight()*0.5f, 0 ) );
//    if ( (transform.getOrigin() - position).length2() > FLT_EPSILON ) {
//        transform.setOrigin( position );
//        GhostObject->setWorldTransform( transform );
//    }
//}

void AProjectileExperimental::_Update( float _TimeStep )
{
    if ( !GhostObject->getBroadphaseHandle() ) {
        // Collision object was not added to the world yet
        return;
    }

    bInsideUpdate = true;

    //UpdateCapsuleShape();

    Update( _TimeStep );

    bInsideUpdate = false;
}

class AProjectileTraceCallback : public btCollisionWorld::ConvexResultCallback
{
    using Super = btCollisionWorld::ConvexResultCallback;

public:
    AProjectileTraceCallback( btCollisionObject * _Self )
        : HitProxy( nullptr )
        , Self( _Self )
    {
        m_collisionFilterGroup = _Self->getBroadphaseHandle()->m_collisionFilterGroup;
        m_collisionFilterMask = _Self->getBroadphaseHandle()->m_collisionFilterMask;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override
    {
        if ( !Super::needsCollision( proxy0 ) ) {
            return false;
        }

        AHitProxy const * hitProxy0 = static_cast< AHitProxy const * >(Self->getUserPointer());
        AHitProxy const * hitProxy1 = static_cast< AHitProxy const * >(static_cast< btCollisionObject const * >(proxy0->m_clientObject)->getUserPointer());

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

    btScalar addSingleResult( btCollisionWorld::LocalConvexResult & result, bool normalInWorldSpace ) override
    {
        if ( result.m_hitCollisionObject == Self )
        {
            return 1;
        }

        if ( !result.m_hitCollisionObject->hasContactResponse() )
        {
            return 1;
        }

        btVector3 hitNormalWorld = normalInWorldSpace
            ? result.m_hitNormalLocal
            : result.m_hitCollisionObject->getWorldTransform().getBasis() * result.m_hitNormalLocal;

        AN_ASSERT( result.m_hitFraction <= m_closestHitFraction );

        m_closestHitFraction = result.m_hitFraction;
        HitNormalWorld = hitNormalWorld;
        HitPointWorld = result.m_hitPointLocal;
        HitProxy = static_cast< AHitProxy * >(result.m_hitCollisionObject->getUserPointer());

        return result.m_hitFraction;
    }

    btVector3 HitNormalWorld;
    btVector3 HitPointWorld;
    AHitProxy * HitProxy;

protected:
    btCollisionObject * Self;
};

void AProjectileExperimental::TraceSelf( Float3 const & Start, Float3 const & End, SProjectileTrace & Trace ) const
{
    AProjectileTraceCallback callback( GhostObject );

    const float ccdPenetration = 0.0f;

    btTransform transformStart, transformEnd;

    btTransform const & transform = GhostObject->getWorldTransform();

    transformStart.setOrigin( btVectorToFloat3( Start ) );
    transformStart.setBasis( transform.getBasis() );

    transformEnd.setOrigin( btVectorToFloat3( End ) );
    transformEnd.setBasis( transform.getBasis() );

    if ( com_UseGhostObjectSweepTest )
    {
        GhostObject->convexSweepTest( ConvexShape, transformStart, transformEnd, callback, ccdPenetration );
    }
    else
    {
        World->convexSweepTest( ConvexShape, transformStart, transformEnd, callback, ccdPenetration );
    }

    Trace.HitProxy = callback.HitProxy;
    Trace.Position = btVectorToFloat3( callback.HitPointWorld );
    Trace.Normal = btVectorToFloat3( callback.HitNormalWorld );
    Trace.Fraction = callback.m_closestHitFraction;

    AN_ASSERT( GhostObject->hasContactResponse() );
}

void AProjectileExperimental::TraceSelf( Float3 const & Start, Quat const & StartRot, Float3 const & End, Quat const & EndRot, SProjectileTrace & Trace ) const
{
    AProjectileTraceCallback callback( GhostObject );

    const float ccdPenetration = 0.0f;

    btTransform transformStart, transformEnd;

    transformStart.setOrigin( btVectorToFloat3( Start ) );
    transformStart.setRotation( btQuaternionToQuat( StartRot ) );

    transformEnd.setOrigin( btVectorToFloat3( End ) );
    transformEnd.setRotation( btQuaternionToQuat( EndRot ) );

    if ( com_UseGhostObjectSweepTest )
    {
        GhostObject->convexSweepTest( ConvexShape, transformStart, transformEnd, callback, ccdPenetration );
    }
    else
    {
        World->convexSweepTest( ConvexShape, transformStart, transformEnd, callback, ccdPenetration );
    }

    Trace.HitProxy = callback.HitProxy;
    Trace.Position = btVectorToFloat3( callback.HitPointWorld );
    Trace.Normal = btVectorToFloat3( callback.HitNormalWorld );
    Trace.Fraction = callback.m_closestHitFraction;

    AN_ASSERT( GhostObject->hasContactResponse() );
}

void AProjectileExperimental::DrawDebug( ADebugRenderer * InRenderer )
{
    Super::DrawDebug( InRenderer );

    //if ( com_DrawCharacterControllerCapsule ) {
        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( Color4::White() );
        btDrawCollisionShape( InRenderer, GhostObject->getWorldTransform(), GhostObject->getCollisionShape() );
    //}
}

void AProjectileExperimental::ApplyForce( Float3 const & force, Float3 const & rel_pos )
{
    ApplyCentralForce( force );
    ApplyTorque( Math::Cross( rel_pos, force/* * m_linearFactor*/ ) );
}

void AProjectileExperimental::ApplyTorque( Float3 const & torque )
{
    m_totalTorque += torque;// * m_angularFactor;
//#if defined(BT_CLAMP_VELOCITY_TO) && BT_CLAMP_VELOCITY_TO > 0
//    clampVelocity( m_totalTorque );
//#endif
}

void AProjectileExperimental::ApplyCentralForce( Float3 const & force )
{
    m_totalForce += force;// * m_linearFactor;
}

void AProjectileExperimental::Update( float _TimeStep ) {
#if 1
    if ( LinearVelocity.LengthSqr() > 0.001f ) {
        Float3 currentPosition = GetWorldPosition();

        Float3 targetPosition = currentPosition + LinearVelocity * _TimeStep;

        SProjectileTrace trace;
        TraceSelf( currentPosition, targetPosition, trace );

        currentPosition = Math::Lerp( currentPosition, targetPosition, trace.Fraction );

        SetWorldPosition( currentPosition );

        btTransform transform = GhostObject->getWorldTransform();
        transform.setOrigin( btVectorToFloat3( currentPosition ) );
        GhostObject->setWorldTransform( transform );

        if ( trace.HasHit() ) {

            OnHit.Dispatch( trace.HitProxy, trace.Position, trace.Normal );

            LinearVelocity.Clear();
        
            //AHitProxy * HitProxy;
            //Float3 Position;
            //Float3 Normal;
            //float Fraction;
        }
    }
    else {
        LinearVelocity.Clear();
    }
#else

    float Gravity = 9.8f;

    float fallVelocity = Gravity * _TimeStep;

    btVector3 inertia;
    ConvexShape->calculateLocalInertia( 1.0f, inertia );
    btVector3 m_invInertiaLocal;
    m_invInertiaLocal.setValue( inertia.x() != btScalar( 0.0 ) ? btScalar( 1.0 ) / inertia.x() : btScalar( 0.0 ),
                                inertia.y() != btScalar( 0.0 ) ? btScalar( 1.0 ) / inertia.y() : btScalar( 0.0 ),
                                inertia.z() != btScalar( 0.0 ) ? btScalar( 1.0 ) / inertia.z() : btScalar( 0.0 ) );
    btTransform m_worldTransform = GhostObject->getWorldTransform();
    btMatrix3x3 m_invInertiaTensorWorld = m_worldTransform.getBasis().scaled( m_invInertiaLocal ) * m_worldTransform.getBasis().transpose();

    //LinearVelocity += m_totalForce * (/*m_inverseMass * */_TimeStep);
    //AngularVelocity += btVectorToFloat3( m_invInertiaTensorWorld * btVectorToFloat3( m_totalTorque ) * _TimeStep );

    LinearVelocity[1] -= fallVelocity;

    if ( LinearVelocity.LengthSqr() > 0.0001f ) {
        Float3 currentPosition = GetWorldPosition();
        Quat currentRotation = GetWorldRotation();

        Float3 vel = AngularVelocity;
        float len = vel.NormalizeSelf();

        Quat q = Quat::RotationAroundNormal( len * _TimeStep, vel );

        Float3 targetPosition = currentPosition + LinearVelocity * _TimeStep;
        Quat targetRotation = q * GetWorldRotation();
        targetRotation.NormalizeSelf();

        SProjectileTrace trace;
        TraceSelf( currentPosition, currentRotation, targetPosition, targetRotation, trace );
//currentRotation = targetRotation;
        if ( !trace.HasHit() ) {
            currentPosition = targetPosition;
            currentRotation = targetRotation;
        }
        else {
            currentPosition = Math::Lerp( currentPosition, targetPosition, trace.Fraction );
            currentRotation = Quat::Slerp( currentRotation, targetRotation, trace.Fraction );
            currentRotation.NormalizeSelf();

            //Float3 v = Math::Reflect( LinearVelocity, trace.Normal );

            float backoff = Math::Dot( LinearVelocity, trace.Normal ) * (1.5f);
            LinearVelocity = LinearVelocity - trace.Normal * backoff;
            for ( int i = 0 ; i < 3 ; i++ ) {
                if ( Math::Abs( LinearVelocity[i] ) < 0.003f ) {
                    LinearVelocity[i] = 0;
                }
            }

            //Float3 velocityDelta = LinearVelocity;

            //LinearVelocity = v * 0.5f;
            //AngularVelocity = AngularVelocity * 0.5f;

            //velocityDelta = LinearVelocity - velocityDelta;

            

            Float3 force = LinearVelocity / (/*m_inverseMass * */_TimeStep);
            Float3 rel_pos = trace.Position - currentPosition;
            Float3 torque = Math::Cross( rel_pos, force/* * m_linearFactor*/ );// * m_angularFactor;

            AngularVelocity = btVectorToFloat3( m_invInertiaTensorWorld * btVectorToFloat3( torque ) * _TimeStep );



            //float backoff = Math::Dot( LinearVelocity, trace.Normal ) * (1.5f);

            //LinearVelocity = LinearVelocity - trace.Normal * backoff;
            //for ( int i = 0 ; i < 3 ; i++ ) {
            //    if ( Math::Abs( LinearVelocity[i] ) < 0.003f ) {
            //        LinearVelocity[i] = 0;
            //    }
            //}

            //if ( trace.Normal[1] > 0.7f && LinearVelocity[1] < 0.1f ) {
            //    LinearVelocity.Clear();
            //    //AngularVelocity.Clear();
            //}
        }
        //btRigidBody

        SetWorldPosition( currentPosition );
        SetWorldRotation( currentRotation );

        btTransform transform = GhostObject->getWorldTransform();
        transform.setOrigin( btVectorToFloat3( currentPosition ) );
        transform.setRotation( btQuaternionToQuat( currentRotation ) );
        GhostObject->setWorldTransform( transform );
    }
#endif
}
