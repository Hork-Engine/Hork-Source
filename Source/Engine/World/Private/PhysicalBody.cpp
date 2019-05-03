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

#include <Engine/World/Public/PhysicalBody.h>
#include <Engine/World/Public/World.h>

#include "BulletCompatibility/BulletCompatibility.h"
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#define PHYS_COMPARE_EPSILON 0.0001f

class FPhysicalBodyMotionState : public btMotionState {
public:
    FPhysicalBodyMotionState()
        : PrevPosition( Float3(0) )
        , PrevRotation( Quat::Identity() )
        , CenterOfMass( Float3(0) )
    {
    }

    // Overrides

    void getWorldTransform( btTransform & _WorldTransform ) const override;
    void setWorldTransform( btTransform const & _WorldTransform ) override;

    // Public members

    FPhysicalBody * PhysBody;

    mutable Float3 PrevPosition;
    mutable Quat PrevRotation;
    Float3 CenterOfMass;
};

void FPhysicalBodyMotionState::getWorldTransform( btTransform & _WorldTransform ) const {
    PrevPosition = PhysBody->GetWorldPosition();
    PrevRotation = PhysBody->GetWorldRotation();
    _WorldTransform.setOrigin( btVectorToFloat3( PrevPosition + PrevRotation * CenterOfMass ) );
    _WorldTransform.setRotation( btQuaternionToQuat( PrevRotation ) );
}

void FPhysicalBodyMotionState::setWorldTransform( btTransform const & _WorldTransform ) {
    Quat newWorldRotation = btQuaternionToQuat( _WorldTransform.getRotation() );
    Float3 newWorldPosition = btVectorToFloat3( _WorldTransform.getOrigin() ) - newWorldRotation * CenterOfMass;

#if 0
    FPhysicalBody * parentBody = 0;

    FSceneComponent * parent = PhysBody->GetParent();
    if ( parent ) {
        if ( parent->FinalClassMeta().IsSubclassOf< FPhysicalBody >() ) {
            parentBody = static_cast< FPhysicalBody * >( parent );
        }
    }

    if ( !parentBody ) {
        PhysBody->bTransformWasChangedByPhysicsEngine = true;
        PhysBody->SetWorldPosition( newWorldPosition );
        PhysBody->SetWorldRotation( newWorldRotation );
        PrevPosition = PhysBody->GetWorldPosition();
        PrevRotation = PhysBody->GetWorldRotation();
        PhysBody->bTransformWasChangedByPhysicsEngine = false;
    } else {
        FDelayedWorldTransform transform;
        transform.RigidBody = PhysBody;
        transform.ParentRigidBody = parentBody;
        transform.WorldPosition = newWorldPosition;
        transform.WorldRotation = newWorldRotation;
        PhysBody->GetWorld()->AddDelayedWorldTransform( transform );
    }
#else
    PhysBody->bTransformWasChangedByPhysicsEngine = true;
    PhysBody->SetWorldPosition( newWorldPosition );
    PhysBody->SetWorldRotation( newWorldRotation );
    PrevPosition = PhysBody->GetWorldPosition();
    PrevRotation = PhysBody->GetWorldRotation();
    PhysBody->bTransformWasChangedByPhysicsEngine = false;
#endif
}

AN_CLASS_META_NO_ATTRIBS( FPhysicalBody )

FPhysicalBody::FPhysicalBody() {
    CachedScale = Float3( 1.0f );
}

void FPhysicalBody::InitializeComponent() {
    Super::InitializeComponent();

    if ( !bNoPhysics ) {
        CreateRigidBody();
    }
}

void FPhysicalBody::DeinitializeComponent() {
    DestroyRigidBody();

    Super::DeinitializeComponent();
}

void CreateCollisionShape( FCollisionBodyComposition const & BodyComposition, Float3 const & _Scale, btCompoundShape ** _CompoundShape, Float3 * _CenterOfMass ) {
    *_CompoundShape = b3New( btCompoundShape );

    int numShapes = BodyComposition.CollisionBodies.Length();

    if ( numShapes > 0 ) {
        btVector3 centerOfMass( 0, 0, 0 );
        btVector3 scaling = btVectorToFloat3( _Scale );

        TPodArray< btTransform > shapeTransforms;
        shapeTransforms.Reserve( numShapes );

        for ( FCollisionBody * collisionBody : BodyComposition.CollisionBodies ) {
            btTransform & shapeTransform = shapeTransforms.Append();
            shapeTransform.setOrigin( btVectorToFloat3( _Scale * collisionBody->Position ) );
            shapeTransform.setRotation( btQuaternionToQuat( collisionBody->Rotation ) );
            centerOfMass += shapeTransform.getOrigin();
        }

        centerOfMass /= numShapes;

        for ( int i = 0 ; i < numShapes ; i++ ) {
            FCollisionBody * collisionBody = BodyComposition.CollisionBodies[ i ];
            btCollisionShape * shape = collisionBody->Create();
            btTransform & shapeTransform = shapeTransforms[ i ];

            shape->setMargin( collisionBody->Margin );
            shape->setUserPointer( collisionBody );
            shape->setLocalScaling( scaling );

            shapeTransform.getOrigin() -= centerOfMass;

            (*_CompoundShape)->addChildShape( shapeTransform, shape );

            collisionBody->AddRef();
        }

        *_CenterOfMass = btVectorToFloat3( centerOfMass );
    } else {
        *_CenterOfMass = Float3( 0.0f );
    }
}

static void DestroyCollisionShape( btCompoundShape * _CompoundShape ) {
    int numShapes = _CompoundShape->getNumChildShapes();
    for ( int i = numShapes-1 ; i >= 0 ; i-- ) {
        btCollisionShape * shape = _CompoundShape->getChildShape( i );
        static_cast< FCollisionBody * >( shape->getUserPointer() )->RemoveRef();
        b3Destroy( shape );
    }
    b3Destroy( _CompoundShape );
}

static void UpdateRigidBodyCollisionShape( btRigidBody * RigidBody, btCompoundShape * CompoundShape, bool bTrigger, bool bKinematicBody ) {
    int numShapes = CompoundShape->getNumChildShapes();
    bool bUseCompound = !numShapes || numShapes > 1;
    if ( !bUseCompound ) {
        btTransform const & childTransform = CompoundShape->getChildTransform( 0 );

        if ( !btVectorToFloat3( childTransform.getOrigin() ).CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON )
             || !btQuaternionToQuat( childTransform.getRotation() ).Compare( Quat::Identity() ) ) {
            bUseCompound = true;
        }
    }
    RigidBody->setCollisionShape( bUseCompound ? CompoundShape : CompoundShape->getChildShape( 0 ) );

    int collisionFlags = RigidBody->getCollisionFlags();

    if ( bTrigger ) {
        collisionFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
    } else {
        collisionFlags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
    }
    if ( bKinematicBody ) {
        collisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
    } else {
        collisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    if ( !bUseCompound && RigidBody->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE /*&& World->GetInternalEdge()*/ ) {
        collisionFlags |= btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    } else {
        collisionFlags &= ~btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    }

    RigidBody->setCollisionFlags( collisionFlags );
    RigidBody->forceActivationState( bKinematicBody ? DISABLE_DEACTIVATION : ISLAND_SLEEPING );
}

static void UpdateRigidBodyGravity( btRigidBody * RigidBody, bool bNoGravity, bool bOverrideWorldGravity, Float3 const & SelfGravity, Float3 const & WorldGravity ) {
    int flags = RigidBody->getFlags();
    if ( !bNoGravity && !bOverrideWorldGravity ) {
        flags &= ~BT_DISABLE_WORLD_GRAVITY;
    } else {
        flags |= BT_DISABLE_WORLD_GRAVITY;
    }
    RigidBody->setFlags( flags );

    if ( bNoGravity ) {
        RigidBody->setGravity( btVector3( 0.0f, 0.0f, 0.0f ) );
    } else {
        if ( bOverrideWorldGravity ) {
            // Use self gravity
            RigidBody->setGravity( btVectorToFloat3( SelfGravity ) );
        } else {
            // Use world gravity. TODO: If world gravity was changed, change rigid body gravity too?
            RigidBody->setGravity( btVectorToFloat3( WorldGravity ) );
        }
    }
}

AN_FORCEINLINE static unsigned short ClampUnsignedShort( int _Value ) {
    if ( _Value < 0 ) return 0;
    if ( _Value > 0xffff ) return 0xffff;
    return _Value;
}

void FPhysicalBody::CreateRigidBody() {
    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    AN_Assert( MotionState == nullptr );
    AN_Assert( RigidBody == nullptr );
    AN_Assert( CompoundShape == nullptr );

    CachedScale = GetWorldScale();

    MotionState = b3New( FPhysicalBodyMotionState );
    MotionState->PhysBody = this;

    CreateCollisionShape( bUseDefaultBodyComposition ? DefaultBodyComposition() : BodyComposition, CachedScale, &CompoundShape, &MotionState->CenterOfMass );

    btVector3 localInertia( 0.0f, 0.0f, 0.0f );

    if ( Mass < 0.0f ) {
        Mass = 0.0f;
    }
    if ( Mass > 0.0f ) {
        CompoundShape->calculateLocalInertia( Mass, localInertia );
    }

    btRigidBody::btRigidBodyConstructionInfo constructInfo( Mass, MotionState, CompoundShape, localInertia );

//    constructInfo.m_linearDamping;
//    constructInfo.m_angularDamping;
//    constructInfo.m_friction;
//    constructInfo.m_rollingFriction;
//    constructInfo.m_spinningFriction;
//    constructInfo.m_restitution;
//    constructInfo.m_linearSleepingThreshold;
//    constructInfo.m_angularSleepingThreshold;

    RigidBody = b3New( btRigidBody, constructInfo );
    RigidBody->setUserPointer( this );

    UpdateRigidBodyCollisionShape( RigidBody, CompoundShape, bTrigger, bKinematicBody );

    btTransform & transform = RigidBody->getWorldTransform();
    transform.setOrigin( btVectorToFloat3( MotionState->CenterOfMass ) );

    if ( GetWorld()->IsPhysicsSimulating() ) {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setOrigin( transform.getOrigin() );
        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
    }

    RigidBody->updateInertiaTensor();

    physicsWorld->addRigidBody( RigidBody, ClampUnsignedShort( CollisionLayer ), ClampUnsignedShort( CollisionMask ) );

    UpdateRigidBodyGravity( RigidBody, bNoGravity, bOverrideWorldGravity, SelfGravity, GetWorld()->GetGravityVector() );

    Activate();
}

void FPhysicalBody::DestroyRigidBody() {
    if ( RigidBody ) {
        btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

        physicsWorld->removeRigidBody( RigidBody );
        b3Destroy( RigidBody );
        RigidBody = nullptr;

        DestroyCollisionShape( CompoundShape );
        CompoundShape = nullptr;

        b3Destroy( MotionState );
        MotionState = nullptr;
    }
}

void FPhysicalBody::RebuildRigidBody() {
    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    if ( bNoPhysics ) {
        DestroyRigidBody();
        return;
    }

    if ( !RigidBody ) {
        CreateRigidBody();
        return;
    }

    CachedScale = GetWorldScale();

    btTransform const & bodyTransform = RigidBody->getWorldTransform();
    Float3 position = btVectorToFloat3( bodyTransform.getOrigin() ) - btQuaternionToQuat( bodyTransform.getRotation() ) * MotionState->CenterOfMass;

    DestroyCollisionShape( CompoundShape );
    CreateCollisionShape( bUseDefaultBodyComposition ? DefaultBodyComposition() : BodyComposition, CachedScale, &CompoundShape, &MotionState->CenterOfMass );

    btVector3 localInertia( 0.0f, 0.0f, 0.0f );

    if ( Mass < 0.0f ) {
        Mass = 0.0f;
    }
    if ( Mass > 0.0f ) {
        CompoundShape->calculateLocalInertia( Mass, localInertia );
    }

    // Update position with new center of mass
    UpdatePhysicalBodyPosition( position );

    //RigidBody->setMassProps( Mass, localInertia );

    UpdateRigidBodyCollisionShape( RigidBody, CompoundShape, bTrigger, bKinematicBody );

    RigidBody->updateInertiaTensor();

    physicsWorld->removeRigidBody( RigidBody );
    physicsWorld->addRigidBody( RigidBody, ClampUnsignedShort( CollisionLayer ), ClampUnsignedShort( CollisionMask ) );

    UpdateRigidBodyGravity( RigidBody, bNoGravity, bOverrideWorldGravity, SelfGravity, GetWorld()->GetGravityVector() );

    RigidBody->setMassProps( Mass, localInertia );

    Activate();
}

void FPhysicalBody::OnTransformDirty() {
    Super::OnTransformDirty();

    if ( RigidBody ) {
        if ( !bKinematicBody && !bTransformWasChangedByPhysicsEngine ) {

            Float3 position = GetWorldPosition();
            Quat rotation = GetWorldRotation();

            if ( rotation != MotionState->PrevRotation ) {
                MotionState->PrevRotation = rotation;
                UpdatePhysicalBodyRotation( rotation );
            }
            if ( position != MotionState->PrevPosition ) {
                MotionState->PrevPosition = position;
                UpdatePhysicalBodyPosition( position );
            }
        }

        Float3 worldScale = GetWorldScale();
        int numShapes = CompoundShape->getNumChildShapes();

        if ( numShapes > 0 && !CachedScale.CompareEps( worldScale, PHYS_COMPARE_EPSILON ) ) {

            CachedScale = worldScale;

            btVector3 scaling = btVectorToFloat3( worldScale );
#if 1
            for ( int i = 0 ; i < numShapes ; i++ ) {
                btCollisionShape * shape = CompoundShape->getChildShape( i );
                shape->setLocalScaling( scaling );

                // TODO: readd shape to shifted compound shape with origin WorldScale * shape->Position, update center of mass
            }
#else
            CompoundShape->setLocalScaling( scaling );
#endif
        }
    }
}

void FPhysicalBody::UpdatePhysicalBodyPosition( Float3 const & _Position ) {
    btTransform & transform = RigidBody->getWorldTransform();
    transform.setOrigin( btVectorToFloat3( _Position + btQuaternionToQuat( transform.getRotation() ) * MotionState->CenterOfMass ) );

    if ( GetWorld()->IsPhysicsSimulating() ) {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setOrigin( transform.getOrigin() );
        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
    }

    Activate();
}

void FPhysicalBody::UpdatePhysicalBodyRotation( Quat const & _Rotation ) {
    btTransform & transform = RigidBody->getWorldTransform();

    Float3 bodyPrevPosition = btVectorToFloat3( transform.getOrigin() ) - btQuaternionToQuat( transform.getRotation() ) * MotionState->CenterOfMass;

    transform.setRotation( btQuaternionToQuat( _Rotation ) );

    if ( !MotionState->CenterOfMass.CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON ) ) {
        transform.setOrigin( btVectorToFloat3( bodyPrevPosition + _Rotation * MotionState->CenterOfMass ) );
    }

    if ( GetWorld()->IsPhysicsSimulating() ) {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setRotation( transform.getRotation() );
        if ( !MotionState->CenterOfMass.CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON ) ) {
            interpolationWorldTransform.setOrigin( transform.getOrigin() );
        }
        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
    }

    RigidBody->updateInertiaTensor();

    Activate();
}

void FPhysicalBody::SetLinearVelocity( Float3 const & _Velocity ) {
    if ( RigidBody ) {
        RigidBody->setLinearVelocity( btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            Activate();
        }
    }
}

void FPhysicalBody::SetLinearFactor( Float3 const & _Factor ) {
    if ( RigidBody ) {
        RigidBody->setLinearFactor( btVectorToFloat3( _Factor ) );
    }
}

void FPhysicalBody::SetLinearSleepingThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setSleepingThresholds( _Threshold, RigidBody->getAngularSleepingThreshold() );
    }
}

void FPhysicalBody::SetLinearDamping( float _Damping ) {
    if ( RigidBody ) {
        RigidBody->setDamping( _Damping, RigidBody->getAngularDamping() );
    }
}

void FPhysicalBody::SetAngularVelocity( Float3 const & _Velocity ) {
    if ( RigidBody ) {
        RigidBody->setAngularVelocity( btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            Activate();
        }
    }
}

void FPhysicalBody::SetAngularFactor( Float3 const & _Factor ) {
    if ( RigidBody ) {
        RigidBody->setAngularFactor( btVectorToFloat3( _Factor ) );
    }
}

void FPhysicalBody::SetAngularSleepingThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setSleepingThresholds( RigidBody->getLinearSleepingThreshold(), _Threshold );
    }
}

void FPhysicalBody::SetAngularDamping( float _Damping ) {
    if ( RigidBody ) {
        RigidBody->setDamping( RigidBody->getLinearDamping(), _Damping );
    }
}

void FPhysicalBody::SetFriction( float _Friction ) {
    if ( RigidBody ) {
        RigidBody->setFriction( _Friction );
    }
}

void FPhysicalBody::SetAnisotropicFriction( Float3 const & _Friction ) {
    if ( RigidBody ) {
        RigidBody->setAnisotropicFriction( btVectorToFloat3( _Friction ) );
    }
}

void FPhysicalBody::SetRollingFriction( float _Friction ) {
    if ( RigidBody ) {
        RigidBody->setRollingFriction( _Friction );
    }
}

void FPhysicalBody::SetRestitution( float _Restitution ) {
    if ( RigidBody ) {
        RigidBody->setRestitution( _Restitution );
    }
}

void FPhysicalBody::SetContactProcessingThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setContactProcessingThreshold( _Threshold );
    }
}

void FPhysicalBody::SetCcdRadius( float _Radius ) {
    if ( RigidBody ) {
        RigidBody->setCcdSweptSphereRadius( FMath::Max( _Radius, 0.0f ) );
    }
}

void FPhysicalBody::SetCcdMotionThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setCcdMotionThreshold( FMath::Max( _Threshold, 0.0f ) );
    }
}

Float3 FPhysicalBody::GetLinearVelocity() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getLinearVelocity() ) : Float3::Zero();
}

Float3 FPhysicalBody::GetLinearFactor() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getLinearFactor() ) : Float3::Zero();
}

Float3 FPhysicalBody::GetVelocityAtPoint( Float3 const & _Position ) const {
    return RigidBody ? btVectorToFloat3( RigidBody->getVelocityInLocalPoint( btVectorToFloat3( _Position - MotionState->CenterOfMass ) ) ) : Float3::Zero();
}

float FPhysicalBody::GetLinearSleepingThreshold() const {
    return RigidBody ? RigidBody->getLinearSleepingThreshold() : 0;
}

float FPhysicalBody::GetLinearDamping() const {
    return RigidBody ? RigidBody->getLinearDamping() : 0;
}

Float3 FPhysicalBody::GetAngularVelocity() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getAngularVelocity() ) : Float3::Zero();
}

Float3 FPhysicalBody::GetAngularFactor() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getAngularFactor() ) : Float3::Zero();
}

float FPhysicalBody::GetAngularSleepingThreshold() const {
    return RigidBody ? RigidBody->getAngularSleepingThreshold() : 0;
}

float FPhysicalBody::GetAngularDamping() const {
    return RigidBody ? RigidBody->getAngularDamping() : 0;
}

float FPhysicalBody::GetFriction() const {
    return RigidBody ? RigidBody->getFriction() : 0;
}

Float3 FPhysicalBody::GetAnisotropicFriction() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getAnisotropicFriction() ) : Float3::Zero();
}

float FPhysicalBody::GetRollingFriction() const {
    return RigidBody ? RigidBody->getRollingFriction() : 0;
}

float FPhysicalBody::GetRestitution() const {
    return RigidBody ? RigidBody->getRestitution() : 0;
}

float FPhysicalBody::GetContactProcessingThreshold() const {
    return RigidBody ? RigidBody->getContactProcessingThreshold() : 0;
}

float FPhysicalBody::GetCcdRadius() const {
    return RigidBody ? RigidBody->getCcdSweptSphereRadius() : 0;
}

float FPhysicalBody::GetCcdMotionThreshold() const {
    return RigidBody ? RigidBody->getCcdMotionThreshold() : 0;
}

void FPhysicalBody::Activate() {
    if ( RigidBody && Mass > 0.0f ) {
        RigidBody->activate( true );
    }
}

bool FPhysicalBody::IsActive() const {
    return RigidBody ? RigidBody->isActive() : false;
}

void FPhysicalBody::ClearForces() {
    if ( RigidBody ) {
        RigidBody->clearForces();
    }
}

void FPhysicalBody::ApplyCentralForce( Float3 const & _Force ) {
    if ( RigidBody && _Force != Float3::Zero() ) {
        Activate();
        RigidBody->applyCentralForce( btVectorToFloat3( _Force ) );
    }
}

void FPhysicalBody::ApplyForce( Float3 const & _Force, Float3 const & _Position ) {
    if ( RigidBody && _Force != Float3::Zero() ) {
        Activate();
        RigidBody->applyForce( btVectorToFloat3( _Force ), btVectorToFloat3( _Position - MotionState->CenterOfMass ) );
    }
}

void FPhysicalBody::ApplyTorque( Float3 const & _Torque ) {
    if ( RigidBody && _Torque != Float3::Zero() ) {
        Activate();
        RigidBody->applyTorque( btVectorToFloat3( _Torque ) );
    }
}

void FPhysicalBody::ApplyCentralImpulse( Float3 const & _Impulse ) {
    if ( RigidBody && _Impulse != Float3::Zero() ) {
        Activate();
        RigidBody->applyCentralImpulse( btVectorToFloat3( _Impulse ) );
    }
}

void FPhysicalBody::ApplyImpulse( Float3 const & _Impulse, Float3 const & _Position ) {
    if ( RigidBody && _Impulse != Float3::Zero() ) {
        Activate();
        RigidBody->applyImpulse( btVectorToFloat3( _Impulse ), btVectorToFloat3( _Position - MotionState->CenterOfMass ) );
    }
}

void FPhysicalBody::ApplyTorqueImpulse( Float3 const & _Torque ) {
    if ( RigidBody && _Torque != Float3::Zero() ) {
        Activate();
        RigidBody->applyTorqueImpulse( btVectorToFloat3( _Torque ) );
    }
}

// TODO: check correctness
void FPhysicalBody::GetCollisionBodiesWorldBounds( TPodArray< BvAxisAlignedBox > & _BoundingBoxes ) const {
    if ( !RigidBody ) {
        _BoundingBoxes.Clear();
    } else {
        Float3x4 worldTransform;
        Float3 shapeWorldPosition;
        Quat shapeWorldRotation;
        btTransform shapeWorldTransform;
        btVector3 mins, maxs;
        btTransform const & Transform = RigidBody->getWorldTransform();
        Float3 rigidBodyPosition = btVectorToFloat3( Transform.getOrigin() ) - btQuaternionToQuat( Transform.getRotation() ) * MotionState->CenterOfMass;
        Quat rigidBodyRotation = btQuaternionToQuat( Transform.getRotation() );

        worldTransform.Compose( rigidBodyPosition, rigidBodyRotation.ToMatrix(), GetWorldScale() );

        int numShapes = CompoundShape->getNumChildShapes();

        _BoundingBoxes.ResizeInvalidate( numShapes );

        for ( int i = 0 ; i < numShapes ; i++ ) {
            btCompoundShapeChild & shape = CompoundShape->getChildList()[ i ];

            shapeWorldPosition = worldTransform * btVectorToFloat3( shape.m_transform.getOrigin() );
            shapeWorldRotation.FromMatrix( worldTransform.DecomposeRotation() );
            shapeWorldRotation = shapeWorldRotation * btQuaternionToQuat( shape.m_transform.getRotation() );
            shapeWorldTransform.setRotation( btQuaternionToQuat( shapeWorldRotation ) );
            shapeWorldTransform.setOrigin( btVectorToFloat3( shapeWorldPosition ) );

            shape.m_childShape->getAabb( shapeWorldTransform, mins, maxs );

            _BoundingBoxes[i].Mins = btVectorToFloat3( mins );
            _BoundingBoxes[i].Maxs = btVectorToFloat3( maxs );
        }
    }
}

void FPhysicalBody::EndPlay() {
    E_OnBeginContact.UnsubscribeAll();
    E_OnEndContact.UnsubscribeAll();
    E_OnUpdateContact.UnsubscribeAll();
    E_OnBeginOverlap.UnsubscribeAll();
    E_OnEndOverlap.UnsubscribeAll();
    E_OnUpdateOverlap.UnsubscribeAll();
}
