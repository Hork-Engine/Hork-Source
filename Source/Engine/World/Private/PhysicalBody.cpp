#include <Engine/World/Public/PhysicalBody.h>
#include <Engine/World/Public/World.h>

#include "BulletCompatibility/BulletCompatibility.h"
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <Bullet3Common/b3AlignedAllocator.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>

#define PHYS_COMPARE_EPSILON 0.0001f

AN_CLASS_META_NO_ATTRIBS( FCollisionBody )
AN_CLASS_META_NO_ATTRIBS( FCollisionSphere )
AN_CLASS_META_NO_ATTRIBS( FCollisionBox )
AN_CLASS_META_NO_ATTRIBS( FCollisionCylinder )
AN_CLASS_META_NO_ATTRIBS( FCollisionCone )
AN_CLASS_META_NO_ATTRIBS( FCollisionCapsule )
AN_CLASS_META_NO_ATTRIBS( FCollisionPlane )
AN_CLASS_META_NO_ATTRIBS( FCollisionConvexHull )

#define b3New( _ClassName, ... ) new (b3AlignedAlloc(sizeof(_ClassName),16)) _ClassName( __VA_ARGS__ )
#define b3Destroy( _Object ) { dtor( _Object ); b3AlignedFree( _Object ); }

template< typename T > AN_FORCEINLINE void dtor( T * object ) { object->~T(); }

btCollisionShape * FCollisionSphere::Create() {
    return b3New( btSphereShape, Radius );
}

btCollisionShape * FCollisionBox::Create() {
    return b3New( btBoxShape, btVectorToFloat3( HalfExtents ) );
}

btCollisionShape * FCollisionCylinder::Create() {
    return b3New( btCylinderShape, btVectorToFloat3( HalfExtents ) );
}

btCollisionShape * FCollisionCone::Create() {
    return b3New( btConeShape, Radius, Height );
}

btCollisionShape * FCollisionCapsule::Create() {
    return b3New( btCapsuleShape, Radius, Height );
}

btCollisionShape * FCollisionPlane::Create() {
    return b3New( btStaticPlaneShape, btVectorToFloat3( Plane.Normal ), Plane.D );
}

btCollisionShape * FCollisionConvexHull::Create() {
    return b3New( btConvexHullShape, ( btScalar const * )Vertices.ToPtr(), Vertices.Length(), sizeof( Float3 ) );
}

class FPhysicalBodyMotionState : public btMotionState {
public:
    FPhysicalBodyMotionState()
        : PrevPosition( Float3(0) )
        , PrevRotation( Quat::Identity() )
        , CenterOfMass( Float3(0) )
    {
    }

    // Overrides

    void getWorldTransform( btTransform & _WorldTransform ) const;
    void setWorldTransform( btTransform const & _WorldTransform );

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

    CreateRigidBody();
}

void FPhysicalBody::DeinitializeComponent() {
    DestroyRigidBody();

    Super::DeinitializeComponent();
}

void FPhysicalBody::CreateRigidBody() {
    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    ShiftedCompoundShape = b3New( btCompoundShape );
    btCompoundShape * compoundShape = b3New( btCompoundShape );

    TPodArray< float > masses;
    masses.Resize( BodyComposition.CollisionBodies.Length() );
    int numShapes = 0;

    btTransform offset;
    for ( FCollisionBody * collisionBody : BodyComposition.CollisionBodies ) {
        btCollisionShape * shape = collisionBody->Create();

        shape->setMargin( collisionBody->Margin );
        shape->setUserPointer( collisionBody );
        collisionBody->AddRef();

        offset.setOrigin( btVectorToFloat3( collisionBody->Position ) );
        offset.setRotation( btQuaternionToQuat( collisionBody->Rotation ) );
        compoundShape->addChildShape( offset, shape );

        masses[numShapes++] = 1.0f;
    }

#if 0
    btTransform principal;
    principal.setRotation( btQuaternion::getIdentity() );
    principal.setOrigin( btVector3( 0.0f, 0.0f, 0.0f ) );

    if ( numShapes > 0 ) {
        btVector3 inertia( 0.0f, 0.0f, 0.0f );
        compoundShape->calculatePrincipalAxisTransform( masses.ToPtr(), principal, inertia );
    }

    btTransform adjusted;
    for ( int i = 0 ; i < numShapes ; i++ ) {
        adjusted = compoundShape->getChildTransform( i );
        adjusted.setOrigin( adjusted.getOrigin() - principal.getOrigin() );
        ShiftedCompoundShape->addChildShape( adjusted, compoundShape->getChildShape( i ) );
    }
#else
    btTransform adjusted;
    for ( int i = 0 ; i < numShapes ; i++ ) {
        ShiftedCompoundShape->addChildShape( compoundShape->getChildTransform( i ), compoundShape->getChildShape( i ) );
    }
#endif

    bool bUseCompound = !numShapes || numShapes > 1;
    if ( !bUseCompound ) {
        btTransform const & childTransform = ShiftedCompoundShape->getChildTransform( 0 );

        if ( !btVectorToFloat3( childTransform.getOrigin() ).CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON )
             || !btQuaternionToQuat( childTransform.getRotation() ).Compare( Quat::Identity() ) ) {
            bUseCompound = true;
        }
    }

    btVector3 localInertia( 0.0f, 0.0f, 0.0f );

    if ( Mass < 0.0f ) {
        Mass = 0.0f;
    }
    if ( Mass > 0.0f ) {
        ShiftedCompoundShape->calculateLocalInertia( Mass, localInertia );
    }

    MotionState = b3New( FPhysicalBodyMotionState );
    MotionState->CenterOfMass = Float3(0.0f);// btVectorToFloat3( principal.getOrigin() );
    MotionState->PhysBody = this;

    btRigidBody::btRigidBodyConstructionInfo constructInfo( Mass, MotionState, ShiftedCompoundShape, localInertia );

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

    RigidBody->setCollisionShape( bUseCompound ? ShiftedCompoundShape : ShiftedCompoundShape->getChildShape( 0 ) );

    //int flags = RigidBody->getFlags();
    //if ( bNoGravity ) {
    //    flags |= BT_DISABLE_WORLD_GRAVITY;
    //} else {
    //    flags &= ~BT_DISABLE_WORLD_GRAVITY;
    //}
    //RigidBody->setFlags( flags );
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
            // Use world gravity. TODO: If world gravity was changed, change rigid body gravity too
            RigidBody->setGravity( btVectorToFloat3( GetWorld()->GetGravityVector() ) );
        }
    }

    // Use world gravity. TODO: If world gravity was changed, change rigid body gravity too
    //RigidBody->setGravity( btVectorToFloat3( GetWorld()->GetGravityVector() ) );

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

    // Apply physical body position with center of mass shift
//    btTransform & worldTransform = RigidBody->getWorldTransform();
//    worldTransform.setOrigin( CenterOfMass );
//    if ( GetWorld()->IsPhysicsSimulating() ) {
//        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
//        interpolationWorldTransform.setOrigin( worldTransform.getOrigin() );
//        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
//    }

    //RigidBody->setMassProps( Mass, localInertia );
    RigidBody->updateInertiaTensor();

    physicsWorld->addRigidBody( RigidBody, CollisionLayer, CollisionMask );

    if ( Mass > 0.0f ) {
        RigidBody->activate();
    }

    b3Destroy( compoundShape );
}

void FPhysicalBody::DestroyRigidBody() {
    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    physicsWorld->removeRigidBody( RigidBody );
    b3Destroy( RigidBody );

    int numShapes = ShiftedCompoundShape->getNumChildShapes();
    for ( int i = 0 ; i < numShapes ; i++ ) {
        btCollisionShape * shape = ShiftedCompoundShape->getChildShape( i );
        static_cast< FCollisionBody * >( shape->getUserPointer() )->RemoveRef();
        b3Destroy( shape );
    }
    b3Destroy( ShiftedCompoundShape );
    b3Destroy( MotionState );
}

void FPhysicalBody::RebuildComponent() {

    if ( !RigidBody ) {
        return;
    }

    int numShapes = ShiftedCompoundShape->getNumChildShapes();
    for ( int i = numShapes-1 ; i >= 0 ; i-- ) {
        btCollisionShape * shape = ShiftedCompoundShape->getChildShape( i );
        static_cast< FCollisionBody * >( shape->getUserPointer() )->RemoveRef();
        b3Destroy( shape );
    }
    b3Destroy( ShiftedCompoundShape );

    //btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    ShiftedCompoundShape = b3New( btCompoundShape );
    btCompoundShape * compoundShape = b3New( btCompoundShape );

    TPodArray< float > masses;
    masses.Resize( BodyComposition.CollisionBodies.Length() );
    numShapes = 0;

    btTransform offset;
    for ( FCollisionBody * collisionBody : BodyComposition.CollisionBodies ) {
        btCollisionShape * shape = collisionBody->Create();

        shape->setMargin( collisionBody->Margin );
        shape->setUserPointer( collisionBody );
        collisionBody->AddRef();

        offset.setOrigin( btVectorToFloat3( collisionBody->Position ) );
        offset.setRotation( btQuaternionToQuat( collisionBody->Rotation ) );
        compoundShape->addChildShape( offset, shape );

        masses[numShapes++] = 1.0f;
    }

#if 0
    btTransform principal;
    principal.setRotation( btQuaternion::getIdentity() );
    principal.setOrigin( btVector3( 0.0f, 0.0f, 0.0f ) );

    if ( numShapes > 0 ) {
        btVector3 inertia( 0.0f, 0.0f, 0.0f );
        compoundShape->calculatePrincipalAxisTransform( masses.ToPtr(), principal, inertia );
    }

    btTransform adjusted;
    for ( int i = 0 ; i < numShapes ; i++ ) {
        adjusted = compoundShape->getChildTransform( i );
        adjusted.setOrigin( adjusted.getOrigin() - principal.getOrigin() );
        ShiftedCompoundShape->addChildShape( adjusted, compoundShape->getChildShape( i ) );
    }
#else
    btTransform adjusted;
    for ( int i = 0 ; i < numShapes ; i++ ) {
        ShiftedCompoundShape->addChildShape( compoundShape->getChildTransform( i ), compoundShape->getChildShape( i ) );
    }
#endif

    bool bUseCompound = !numShapes || numShapes > 1;
    if ( !bUseCompound ) {
        btTransform const & childTransform = ShiftedCompoundShape->getChildTransform( 0 );

        if ( !btVectorToFloat3( childTransform.getOrigin() ).CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON )
             || !btQuaternionToQuat( childTransform.getRotation() ).Compare( Quat::Identity() ) ) {
            bUseCompound = true;
        }
    }

    btVector3 localInertia( 0.0f, 0.0f, 0.0f );

    if ( Mass < 0.0f ) {
        Mass = 0.0f;
    }
    if ( Mass > 0.0f ) {
        ShiftedCompoundShape->calculateLocalInertia( Mass, localInertia );
    }

    MotionState->CenterOfMass = Float3(0.0f);// btVectorToFloat3( principal.getOrigin() );

//    btRigidBody::btRigidBodyConstructionInfo constructInfo( Mass, MotionState, ShiftedCompoundShape, localInertia );

//    constructInfo.m_linearDamping;
//    constructInfo.m_angularDamping;
//    constructInfo.m_friction;
//    constructInfo.m_rollingFriction;
//    constructInfo.m_spinningFriction;
//    constructInfo.m_restitution;
//    constructInfo.m_linearSleepingThreshold;
//    constructInfo.m_angularSleepingThreshold;

    RigidBody->setMassProps( Mass, localInertia );

    RigidBody->setCollisionShape( bUseCompound ? ShiftedCompoundShape : ShiftedCompoundShape->getChildShape( 0 ) );

    //int flags = RigidBody->getFlags();
    //if ( bNoGravity ) {
    //    flags |= BT_DISABLE_WORLD_GRAVITY;
    //} else {
    //    flags &= ~BT_DISABLE_WORLD_GRAVITY;
    //}
    //RigidBody->setFlags( flags );
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
            // Use world gravity. TODO: If world gravity was changed, change rigid body gravity too
            RigidBody->setGravity( btVectorToFloat3( GetWorld()->GetGravityVector() ) );
        }
    }

    // Use world gravity. TODO: If world gravity was changed, change rigid body gravity too
    //RigidBody->setGravity( btVectorToFloat3( GetWorld()->GetGravityVector() ) );

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

    // Apply physical body position with center of mass shift
//    btTransform & worldTransform = RigidBody->getWorldTransform();
//    worldTransform.setOrigin( CenterOfMass );
//    if ( GetWorld()->IsPhysicsSimulating() ) {
//        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
//        interpolationWorldTransform.setOrigin( worldTransform.getOrigin() );
//        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
//    }

    //RigidBody->setMassProps( Mass, localInertia );
    RigidBody->updateInertiaTensor();

//    physicsWorld->addRigidBody( RigidBody, CollisionLayer, CollisionMask );

    if ( Mass > 0.0f ) {
        RigidBody->activate();
    }

    b3Destroy( compoundShape );


    //DestroyRigidBody();
    //CreateRigidBody();

    CachedScale = Float3(1);
    MarkTransformDirty();
}

void FPhysicalBody::OnTransformDirty() {
    Super::OnTransformDirty();

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

    if ( !CachedScale.CompareEps( worldScale, PHYS_COMPARE_EPSILON ) ) {

        CachedScale = worldScale;

        //foreach  shape :compoindShape { shape->setLocalScaling( btVectorToVec3( newWorldScale ) ); }

        ShiftedCompoundShape->setLocalScaling( btVectorToFloat3( worldScale ) );

        // TODO: update center of mass
    }
}

#define Activate() {if ( Mass > 0.0f ) RigidBody->activate( true );}

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
    RigidBody->setLinearVelocity( btVectorToFloat3( _Velocity ) );
    //if ( _Velocity != Float3::Zero() ) {
        Activate();
    //}
}

void FPhysicalBody::SetLinearFactor( Float3 const & _Factor ) {
    RigidBody->setLinearFactor( btVectorToFloat3( _Factor ) );
}

void FPhysicalBody::SetLinearRestThreshold( float _Threshold ) {
    RigidBody->setSleepingThresholds( _Threshold, RigidBody->getAngularSleepingThreshold() );
}

void FPhysicalBody::SetLinearDamping( float _Damping ) {
    RigidBody->setDamping( _Damping, RigidBody->getAngularDamping() );
}

void FPhysicalBody::SetAngularVelocity( Float3 const & _Velocity ) {
    RigidBody->setAngularVelocity( btVectorToFloat3( _Velocity ) );
    //if ( _Velocity != Float3::Zero() ) {
        Activate();
    //}
}

void FPhysicalBody::SetAngularFactor( Float3 const & _Factor ) {
    RigidBody->setAngularFactor( btVectorToFloat3( _Factor ) );
}

void FPhysicalBody::SetAngularRestThreshold( float _Threshold ) {
    RigidBody->setSleepingThresholds( RigidBody->getLinearSleepingThreshold(), _Threshold );
}

void FPhysicalBody::SetAngularDamping( float _Damping ) {
    RigidBody->setDamping( RigidBody->getLinearDamping(), _Damping );
}

void FPhysicalBody::SetFriction( float _Friction ) {
    RigidBody->setFriction( _Friction );
}

void FPhysicalBody::SetAnisotropicFriction( Float3 const & _Friction ) {
    RigidBody->setAnisotropicFriction( btVectorToFloat3( _Friction ) );
}

void FPhysicalBody::SetRollingFriction( float _Friction ) {
    RigidBody->setRollingFriction( _Friction );
}

void FPhysicalBody::SetRestitution( float _Restitution ) {
    RigidBody->setRestitution( _Restitution );
}

void FPhysicalBody::SetContactProcessingThreshold( float _Threshold ) {
    RigidBody->setContactProcessingThreshold( _Threshold );
}

void FPhysicalBody::SetCcdRadius( float _Radius ) {
    RigidBody->setCcdSweptSphereRadius( FMath::Max( _Radius, 0.0f ) );
}

void FPhysicalBody::SetCcdMotionThreshold( float _Threshold ) {
    RigidBody->setCcdMotionThreshold( FMath::Max( _Threshold, 0.0f ) );
}

Float3 FPhysicalBody::GetLinearVelocity() const {
    return btVectorToFloat3( RigidBody->getLinearVelocity() );
}

Float3 FPhysicalBody::GetLinearFactor() const {
    return btVectorToFloat3( RigidBody->getLinearFactor() );
}

Float3 FPhysicalBody::GetVelocityAtPoint( Float3 const & _Position ) const {
    return btVectorToFloat3( RigidBody->getVelocityInLocalPoint( btVectorToFloat3( _Position - MotionState->CenterOfMass ) ) );
}

float FPhysicalBody::GetLinearRestThreshold() const {
    return RigidBody->getLinearSleepingThreshold();
}

float FPhysicalBody::GetLinearDamping() const {
    return RigidBody->getLinearDamping();
}

Float3 FPhysicalBody::GetAngularVelocity() const {
    return btVectorToFloat3( RigidBody->getAngularVelocity() );
}

Float3 FPhysicalBody::GetAngularFactor() const {
    return btVectorToFloat3( RigidBody->getAngularFactor() );
}

float FPhysicalBody::GetAngularRestThreshold() const {
    return RigidBody->getAngularSleepingThreshold();
}

float FPhysicalBody::GetAngularDamping() const {
    return RigidBody->getAngularDamping();
}

float FPhysicalBody::GetFriction() const {
    return RigidBody->getFriction();
}

Float3 FPhysicalBody::GetAnisotropicFriction() const {
    return btVectorToFloat3( RigidBody->getAnisotropicFriction() );
}

float FPhysicalBody::GetRollingFriction() const {
    return RigidBody->getRollingFriction();
}

float FPhysicalBody::GetRestitution() const {
    if ( bNoPhysics ) {
        return 0.0f;
    }

    return RigidBody->getRestitution();
}

float FPhysicalBody::GetContactProcessingThreshold() const {
    if ( bNoPhysics ) {
        return 0.0f;
    }

    return RigidBody->getContactProcessingThreshold();
}

float FPhysicalBody::GetCcdRadius() const {
    if ( bNoPhysics ) {
        return 0.0f;
    }

    return RigidBody->getCcdSweptSphereRadius();
}

float FPhysicalBody::GetCcdMotionThreshold() const {
    if ( bNoPhysics ) {
        return 0.0f;
    }

    return RigidBody->getCcdMotionThreshold();
}

bool FPhysicalBody::IsActive() const {
    if ( bNoPhysics ) {
        return false;
    }

    return RigidBody->isActive();
}

void FPhysicalBody::ClearForces() {
    if ( bNoPhysics ) {
        return;
    }

    RigidBody->clearForces();
}

void FPhysicalBody::ApplyCentralForce( Float3 const & _Force ) {
    if ( bNoPhysics ) {
        return;
    }

    if ( _Force != Float3::Zero() ) {
        Activate();
        RigidBody->applyCentralForce( btVectorToFloat3( _Force ) );
    }
}

void FPhysicalBody::ApplyForce( Float3 const & _Force, Float3 const & _Position ) {
    if ( bNoPhysics ) {
        return;
    }

    if ( _Force != Float3::Zero() ) {
        Activate();
        RigidBody->applyForce( btVectorToFloat3( _Force ), btVectorToFloat3( _Position - MotionState->CenterOfMass ) );
    }
}

void FPhysicalBody::ApplyTorque( Float3 const & _Torque ) {
    if ( bNoPhysics ) {
        return;
    }

    if ( _Torque != Float3::Zero() ) {
        Activate();
        RigidBody->applyTorque( btVectorToFloat3( _Torque ) );
    }
}

void FPhysicalBody::ApplyCentralImpulse( Float3 const & _Impulse ) {
    if ( bNoPhysics ) {
        return;
    }

    if ( _Impulse != Float3::Zero() ) {
        Activate();
        RigidBody->applyCentralImpulse( btVectorToFloat3( _Impulse ) );
    }
}

void FPhysicalBody::ApplyImpulse( Float3 const & _Impulse, Float3 const & _Position ) {
    if ( bNoPhysics ) {
        return;
    }

    if ( _Impulse != Float3::Zero() ) {
        Activate();
        RigidBody->applyImpulse( btVectorToFloat3( _Impulse ), btVectorToFloat3( _Position - MotionState->CenterOfMass ) );
    }
}

void FPhysicalBody::ApplyTorqueImpulse( Float3 const & _Torque ) {
    if ( _Torque != Float3::Zero() ) {
        Activate();
        RigidBody->applyTorqueImpulse( btVectorToFloat3( _Torque ) );
    }
}

#if 0
void FPhysicalBody::SetGravity( Float3 const & _Gravity ) {
    if ( _Gravity != SelfGravity ) {
        SelfGravity = _Gravity;
        UpdateGravity();
    }
}

void FPhysicalBody::EnableGravity( bool _Enable ) {
    if ( _Enable != EnableGraivity ) {
        EnableGraivity = _Enable;
        UpdateGravity();
    }
}

void FPhysicalBody::SetKinematic( bool _Enable ) {
    if ( _Enable != Kinematic ) {
        Kinematic = _Enable;
        AddBodyToWorld();
    }
}

void FPhysicalBody::SetTrigger( bool _Enable ) {
    if ( _Enable != Trigger ) {
        Trigger = _Enable;
        AddBodyToWorld();
    }
}

void FPhysicalBody::SetCollisionLayer( unsigned short _Layer ) {
    if ( _Layer != CollisionLayer ) {
        CollisionLayer = _Layer;
        AddBodyToWorld();
    }
}

void FPhysicalBody::SetCollisionMask( unsigned short _Mask ) {
    if ( _Mask != CollisionMask ) {
        CollisionMask = _Mask;
        AddBodyToWorld();
    }
}

void FPhysicalBody::SetCollisionLayerAndMask( unsigned short _Layer, unsigned short _Mask ) {
    if ( _Layer != CollisionLayer || _Mask != CollisionMask ) {
        CollisionLayer = _Layer;
        CollisionMask = _Mask;
        AddBodyToWorld();
    }
}
#endif

// TODO: check correctness
void FPhysicalBody::GetCollisionBodiesWorldBounds( TPodArray< BvAxisAlignedBox > & _BoundingBoxes ) const {
    if ( bNoPhysics ) {
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

        worldTransform.Compose( rigidBodyPosition, rigidBodyRotation.ToMatrix(), Float3(1.0f)/*GetWorldScale()*/ );

        int numShapes = ShiftedCompoundShape->getNumChildShapes();

        _BoundingBoxes.ResizeInvalidate( numShapes );

        for ( int i = 0 ; i < numShapes ; i++ ) {
            btCompoundShapeChild & shape = ShiftedCompoundShape->getChildList()[ i ];

            //FCollisionBody * collisionBody = static_cast< FCollisionBody * >( shape->getUserPointer() )->RemoveRef();

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

//#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
//btscaled
