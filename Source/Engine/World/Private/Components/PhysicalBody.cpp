/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Components/PhysicalBody.h>
#include <World/Public/World.h>
#include <World/Public/Base/DebugRenderer.h>
#include <Core/Public/Logger.h>

#include "../BulletCompatibility/BulletCompatibility.h"

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#define PHYS_COMPARE_EPSILON 0.0001f
#define MIN_MASS 0.001f
#define MAX_MASS 1000.0f

ARuntimeVariable RVDrawCollisionModel( _CTS( "DrawCollisionModel" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawStaticCollisionBounds( _CTS( "DrawStaticCollisionBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawDynamicCollisionBounds( _CTS( "DrawDynamicCollisionBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawKinematicCollisionBounds( _CTS( "DrawKinematicCollisionBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawTriggerBounds( _CTS( "DrawTriggerBounds" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawCenterOfMass( _CTS( "DrawCenterOfMass" ), _CTS( "0" ), VAR_CHEAT );

static constexpr bool bUseInternalEdgeUtility = true;

//static bool bDuringMotionStateUpdate = false;

class SPhysicalBodyMotionState : public btMotionState {
public:
    SPhysicalBodyMotionState()
        : WorldPosition( Float3(0) )
        , WorldRotation( Quat::Identity() )
        , CenterOfMass( Float3(0) )
    {
    }

    // Overrides

    void getWorldTransform( btTransform & _CenterOfMassTransform ) const override;
    void setWorldTransform( btTransform const & _CenterOfMassTransform ) override;

    // Public members

    APhysicalBody * Self;
    mutable Float3 WorldPosition;
    mutable Quat WorldRotation;
    Float3 CenterOfMass;
    bool bDuringMotionStateUpdate = false;
};

void SPhysicalBodyMotionState::getWorldTransform( btTransform & _CenterOfMassTransform ) const {
    WorldPosition = Self->GetWorldPosition();
    WorldRotation = Self->GetWorldRotation();

    _CenterOfMassTransform.setRotation( btQuaternionToQuat( WorldRotation ) );
    _CenterOfMassTransform.setOrigin( btVectorToFloat3( WorldPosition ) + _CenterOfMassTransform.getBasis() * btVectorToFloat3( CenterOfMass ) );
}

void SPhysicalBodyMotionState::setWorldTransform( btTransform const & _CenterOfMassTransform ) {
    if ( Self->PhysicsBehavior == PB_DYNAMIC )
    {
        bDuringMotionStateUpdate = true;
        WorldRotation = btQuaternionToQuat( _CenterOfMassTransform.getRotation() );
        WorldPosition = btVectorToFloat3( _CenterOfMassTransform.getOrigin() - _CenterOfMassTransform.getBasis() * btVectorToFloat3( CenterOfMass ) );
        Self->SetWorldPosition( WorldPosition );
        Self->SetWorldRotation( WorldRotation );
        bDuringMotionStateUpdate = false;
    } else {
        GLogger.Printf( "SPhysicalBodyMotionState::setWorldTransform for non-dynamic %s\n", Self->GetObjectNameCStr() );
    }
}

AN_CLASS_META( APhysicalBody )

#define HasCollisionBody() ( !bSoftBodySimulation && GetBodyComposition().NumCollisionBodies() > 0 && ( /*PhysicsSimulation == PS_DYNAMIC || bTrigger || bKinematicBody || */CollisionGroup ) )

APhysicalBody::APhysicalBody() {
    CachedScale = Float3( 1.0f );
}

void APhysicalBody::InitializeComponent() {
    Super::InitializeComponent();

    if ( HasCollisionBody() ) {
        CreateRigidBody();
    }

    if ( AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE )
    {
        AAINavigationMesh & NavigationMesh = GetWorld()->GetNavigationMesh();
        NavigationMesh.AddNavigationGeometry( this );
    }
}

void APhysicalBody::DeinitializeComponent() {
    DestroyRigidBody();

    AAINavigationMesh & NavigationMesh = GetWorld()->GetNavigationMesh();
    NavigationMesh.RemoveNavigationGeometry( this );

    Super::DeinitializeComponent();
}

void APhysicalBody::SetPhysicsBehavior( EPhysicsBehavior _PhysicsBehavior ) {
    if ( PhysicsBehavior == _PhysicsBehavior )
    {
        return;
    }

    PhysicsBehavior = _PhysicsBehavior;

    if ( IsInitialized() )
    {
        UpdatePhysicsAttribs();
    }
}

void APhysicalBody::SetAINavigationBehavior( EAINavigationBehavior _AINavigationBehavior ) {
    if ( AINavigationBehavior == _AINavigationBehavior )
    {
        return;
    }

    AINavigationBehavior = _AINavigationBehavior;

    if ( IsInitialized() )
    {
        AAINavigationMesh & NavigationMesh = GetWorld()->GetNavigationMesh();

        NavigationMesh.RemoveNavigationGeometry( this );

        if ( AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE )
        {
            NavigationMesh.AddNavigationGeometry( this );
        }
    }
}

ACollisionBodyComposition const & APhysicalBody::GetBodyComposition() const {
    return bUseDefaultBodyComposition ? DefaultBodyComposition() : BodyComposition;
}

static void UpdateRigidBodyCollisionShape( btCollisionObject * RigidBody, btCompoundShape * CompoundShape, bool bTrigger, EPhysicsBehavior _PhysicsBehavior ) {
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
    if ( _PhysicsBehavior == PB_KINEMATIC ) {
        collisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
    } else {
        collisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    if ( _PhysicsBehavior == PB_STATIC ) {
        collisionFlags |= btCollisionObject::CF_STATIC_OBJECT;
    } else {
        collisionFlags &= ~btCollisionObject::CF_STATIC_OBJECT;
    }
    if ( !bUseCompound && RigidBody->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE && bUseInternalEdgeUtility ) {
        collisionFlags |= btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    } else {
        collisionFlags &= ~btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    }

    RigidBody->setCollisionFlags( collisionFlags );
    RigidBody->forceActivationState( _PhysicsBehavior == PB_KINEMATIC ? DISABLE_DEACTIVATION : ISLAND_SLEEPING );
}

static void UpdateRigidBodyGravity( btRigidBody * RigidBody, bool bDisableGravity, bool bOverrideWorldGravity, Float3 const & SelfGravity, Float3 const & WorldGravity ) {
    int flags = RigidBody->getFlags();

    if ( bDisableGravity || bOverrideWorldGravity ) {
        flags |= BT_DISABLE_WORLD_GRAVITY;
    } else {
        flags &= ~BT_DISABLE_WORLD_GRAVITY;
    }

    RigidBody->setFlags( flags );

    if ( bDisableGravity ) {
        RigidBody->setGravity( btVector3( 0.0f, 0.0f, 0.0f ) );
    } else if ( bOverrideWorldGravity ) {
        // Use self gravity
        RigidBody->setGravity( btVectorToFloat3( SelfGravity ) );
    } else {
        // Use world gravity
        RigidBody->setGravity( btVectorToFloat3( WorldGravity ) );
    }
}

void APhysicalBody::CreateRigidBody() {
    //btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    AN_ASSERT( MotionState == nullptr );
    AN_ASSERT( RigidBody == nullptr );
    AN_ASSERT( CompoundShape == nullptr );

    CachedScale = GetWorldScale();

    MotionState = b3New( SPhysicalBodyMotionState );
    MotionState->Self = this;

    CreateCollisionShape( GetBodyComposition(), CachedScale, &CompoundShape, &MotionState->CenterOfMass );

    btVector3 localInertia( 0.0f, 0.0f, 0.0f );

    float mass = Math::Clamp( Mass, MIN_MASS, MAX_MASS );
    if ( PhysicsBehavior == PB_DYNAMIC ) {
        CompoundShape->calculateLocalInertia( mass, localInertia );
    }

    btRigidBody::btRigidBodyConstructionInfo constructInfo( PhysicsBehavior == PB_DYNAMIC ? mass : 0.0f, MotionState, CompoundShape, localInertia );

    constructInfo.m_linearDamping = LinearDamping;
    constructInfo.m_angularDamping = AngularDamping;
    constructInfo.m_friction = Friction;
    constructInfo.m_rollingFriction = RollingFriction;
//    constructInfo.m_spinningFriction;
    constructInfo.m_restitution = Restitution;
    constructInfo.m_linearSleepingThreshold = LinearSleepingThreshold;
    constructInfo.m_angularSleepingThreshold = AngularSleepingThreshold;

    RigidBody = b3New( btRigidBody, constructInfo );
    RigidBody->setUserPointer( this );

    UpdateRigidBodyCollisionShape( RigidBody, CompoundShape, bTrigger, PhysicsBehavior );

    // transform already setup in motion state
#if 0
    Quat worldRotation = GetWorldRotation();
    Float3 worldPosition = GetWorldPosition();

    btTransform & centerOfMassTransform = RigidBody->getWorldTransform();

    centerOfMassTransform.setRotation( btQuaternionToQuat( worldRotation ) );
    centerOfMassTransform.setOrigin( btVectorToFloat3( worldPosition ) + centerOfMassTransform.getBasis() * btVectorToFloat3( MotionState->CenterOfMass ) );
#endif
    RigidBody->updateInertiaTensor();

    AddPhysicalBodyToWorld();

    UpdateRigidBodyGravity( RigidBody, bDisableGravity, bOverrideWorldGravity, SelfGravity, GetWorld()->GetGravityVector() );

    ActivatePhysics();

    // Update dynamic attributes
    SetLinearFactor( LinearFactor );
    SetAngularFactor( AngularFactor );
    SetAnisotropicFriction( AnisotropicFriction );
    SetContactProcessingThreshold( ContactProcessingThreshold );
    SetCcdRadius( CcdRadius );
    SetCcdMotionThreshold( CcdMotionThreshold );
}

void APhysicalBody::DestroyRigidBody() {
    if ( RigidBody ) {
        RemovePhysicalBodyFromWorld();

        b3Destroy( RigidBody );
        RigidBody = nullptr;

        DestroyCollisionShape( CompoundShape );
        CompoundShape = nullptr;

        b3Destroy( MotionState );
        MotionState = nullptr;
    }
}

void APhysicalBody::AddPhysicalBodyToWorld() {
    GetWorld()->GetPhysicsWorld().AddPhysicalBody( this );
}

void APhysicalBody::RemovePhysicalBodyFromWorld() {
    GetWorld()->GetPhysicsWorld().RemovePhysicalBody( this );
}

void APhysicalBody::UpdatePhysicsAttribs() {
    if ( !GetWorld() ) {
        // Called before initialization
        return;
    }

    if ( !HasCollisionBody() ) {
        DestroyRigidBody();
        return;
    }

    if ( !RigidBody ) {
        CreateRigidBody();
        return;
    }

    //btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    btTransform const & centerOfMassTransform = RigidBody->getWorldTransform();
    Float3 position = btVectorToFloat3( centerOfMassTransform.getOrigin() - centerOfMassTransform.getBasis() * btVectorToFloat3( MotionState->CenterOfMass ) );

    CachedScale = GetWorldScale();

    DestroyCollisionShape( CompoundShape );
    CreateCollisionShape( GetBodyComposition(), CachedScale, &CompoundShape, &MotionState->CenterOfMass );

    btVector3 localInertia( 0.0f, 0.0f, 0.0f );

    float mass = Math::Clamp( Mass, MIN_MASS, MAX_MASS );
    if ( PhysicsBehavior == PB_DYNAMIC ) {
        CompoundShape->calculateLocalInertia( mass, localInertia );
    }

    RigidBody->setMassProps( PhysicsBehavior == PB_DYNAMIC ? mass : 0.0f, localInertia );

    UpdateRigidBodyCollisionShape( RigidBody, CompoundShape, bTrigger, PhysicsBehavior );

    // Update position with new center of mass
    SetCenterOfMassPosition( position );

    RigidBody->updateInertiaTensor();

    AddPhysicalBodyToWorld();

    UpdateRigidBodyGravity( RigidBody, bDisableGravity, bOverrideWorldGravity, SelfGravity, GetWorld()->GetGravityVector() );

    ActivatePhysics();

    // Update dynamic attributes
//    SetLinearFactor( LinearFactor );
//    SetAngularFactor( AngularFactor );
//    SetAnisotropicFriction( AnisotropicFriction );
//    SetContactProcessingThreshold( ContactProcessingThreshold );
//    SetCcdRadius( CcdRadius );
//    SetCcdMotionThreshold( CcdMotionThreshold );
}

void APhysicalBody::OnTransformDirty() {
    Super::OnTransformDirty();

    if ( RigidBody ) {
        if ( !MotionState->bDuringMotionStateUpdate ) {

            if ( PhysicsBehavior != PB_KINEMATIC )
            {
                Float3 position = GetWorldPosition();
                Quat rotation = GetWorldRotation();

                if ( rotation != MotionState->WorldRotation ) {
                    MotionState->WorldRotation = rotation;
                    SetCenterOfMassRotation( rotation );
                }
                if ( position != MotionState->WorldPosition ) {
                    MotionState->WorldPosition = position;
                    SetCenterOfMassPosition( position );
                }

                GLogger.Printf( "Set transform for STATIC or DYNAMIC phys body %s\n", GetObjectNameCStr() );
            }
        }

        int numShapes = CompoundShape->getNumChildShapes();
        if ( numShapes > 0 && !CachedScale.CompareEps( GetWorldScale(), PHYS_COMPARE_EPSILON ) ) {
            UpdatePhysicsAttribs();
        }
    }

    //if ( SoftBody && !bUpdateSoftbodyTransform ) {
    //    if ( !PrevWorldPosition.CompareEps( GetWorldPosition(), PHYS_COMPARE_EPSILON )
    //        || !PrevWorldRotation.CompareEps( GetWorldRotation(), PHYS_COMPARE_EPSILON ) ) {
    //        bUpdateSoftbodyTransform = true;

    //        // TODO: add to dirty list?
    //    }
    //}
}

void APhysicalBody::SetCenterOfMassPosition( Float3 const & _Position ) {
    btTransform & centerOfMassTransform = RigidBody->getWorldTransform();
    centerOfMassTransform.setOrigin( btVectorToFloat3( _Position ) + centerOfMassTransform.getBasis() * btVectorToFloat3( MotionState->CenterOfMass ) );

    if ( GetWorld()->IsDuringPhysicsUpdate() ) {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setOrigin( centerOfMassTransform.getOrigin() );
        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
    }

    ActivatePhysics();
}

void APhysicalBody::SetCenterOfMassRotation( Quat const & _Rotation ) {
    btTransform & centerOfMassTransform = RigidBody->getWorldTransform();

    btVector3 bodyPrevPosition = centerOfMassTransform.getOrigin() - centerOfMassTransform.getBasis() * btVectorToFloat3( MotionState->CenterOfMass );

    centerOfMassTransform.setRotation( btQuaternionToQuat( _Rotation ) );

    if ( !MotionState->CenterOfMass.CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON ) ) {
        centerOfMassTransform.setOrigin( bodyPrevPosition + centerOfMassTransform.getBasis() * btVectorToFloat3( MotionState->CenterOfMass ) );
    }

    if ( GetWorld()->IsDuringPhysicsUpdate() ) {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setBasis( centerOfMassTransform.getBasis() );
        if ( !MotionState->CenterOfMass.CompareEps( Float3::Zero(), PHYS_COMPARE_EPSILON ) ) {
            interpolationWorldTransform.setOrigin( centerOfMassTransform.getOrigin() );
        }
        RigidBody->setInterpolationWorldTransform( interpolationWorldTransform );
    }

    RigidBody->updateInertiaTensor();

    ActivatePhysics();
}

void APhysicalBody::SetLinearVelocity( Float3 const & _Velocity ) {
    if ( RigidBody ) {
        RigidBody->setLinearVelocity( btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            ActivatePhysics();
        }
    }

    if ( SoftBody ) {
        SoftBody->setVelocity( btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::AddLinearVelocity( Float3 const & _Velocity ) {
    if ( RigidBody ) {
        RigidBody->setLinearVelocity( RigidBody->getLinearVelocity() + btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            ActivatePhysics();
        }
    }

    if ( SoftBody ) {
        SoftBody->addVelocity( btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::SetLinearFactor( Float3 const & _Factor ) {
    if ( RigidBody ) {
        RigidBody->setLinearFactor( btVectorToFloat3( _Factor ) );
    }

    LinearFactor = _Factor;
}

void APhysicalBody::SetLinearSleepingThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setSleepingThresholds( _Threshold, AngularSleepingThreshold );
    }

    LinearSleepingThreshold = _Threshold;
}

void APhysicalBody::SetLinearDamping( float _Damping ) {
    if ( RigidBody ) {
        RigidBody->setDamping( _Damping, AngularDamping );
    }

    LinearDamping = _Damping;
}

void APhysicalBody::SetAngularVelocity( Float3 const & _Velocity ) {
    if ( RigidBody ) {
        RigidBody->setAngularVelocity( btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::AddAngularVelocity( Float3 const & _Velocity ) {
    if ( RigidBody ) {
        RigidBody->setAngularVelocity( RigidBody->getAngularVelocity() + btVectorToFloat3( _Velocity ) );
        if ( _Velocity != Float3::Zero() ) {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::SetAngularFactor( Float3 const & _Factor ) {
    if ( RigidBody ) {
        RigidBody->setAngularFactor( btVectorToFloat3( _Factor ) );
    }

    AngularFactor = _Factor;
}

void APhysicalBody::SetAngularSleepingThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setSleepingThresholds( LinearSleepingThreshold, _Threshold );
    }

    AngularSleepingThreshold = _Threshold;
}

void APhysicalBody::SetAngularDamping( float _Damping ) {
    if ( RigidBody ) {
        RigidBody->setDamping( LinearDamping, _Damping );
    }

    AngularDamping = _Damping;
}

void APhysicalBody::SetFriction( float _Friction ) {
    if ( RigidBody ) {
        RigidBody->setFriction( _Friction );
    }

    if ( SoftBody ) {
        SoftBody->setFriction( _Friction );
    }

    Friction = _Friction;
}

void APhysicalBody::SetAnisotropicFriction( Float3 const & _Friction ) {
    if ( RigidBody ) {
        RigidBody->setAnisotropicFriction( btVectorToFloat3( _Friction ) );
    }

    if ( SoftBody ) {
        SoftBody->setAnisotropicFriction( btVectorToFloat3( _Friction ) );
    }

    AnisotropicFriction = _Friction;
}

void APhysicalBody::SetRollingFriction( float _Friction ) {
    if ( RigidBody ) {
        RigidBody->setRollingFriction( _Friction );
    }

    if ( SoftBody ) {
        SoftBody->setRollingFriction( _Friction );
    }

    RollingFriction = _Friction;
}

void APhysicalBody::SetRestitution( float _Restitution ) {
    if ( RigidBody ) {
        RigidBody->setRestitution( _Restitution );
    }

    if ( SoftBody ) {
        SoftBody->setRestitution( _Restitution );
    }

    Restitution = _Restitution;
}

void APhysicalBody::SetContactProcessingThreshold( float _Threshold ) {
    if ( RigidBody ) {
        RigidBody->setContactProcessingThreshold( _Threshold );
    }

    if ( SoftBody ) {
        SoftBody->setContactProcessingThreshold( _Threshold );
    }

    ContactProcessingThreshold = _Threshold;
}

void APhysicalBody::SetCcdRadius( float _Radius ) {
    CcdRadius = Math::Max( _Radius, 0.0f );

    if ( RigidBody ) {
        RigidBody->setCcdSweptSphereRadius( CcdRadius );
    }

    if ( SoftBody ) {
        SoftBody->setCcdSweptSphereRadius( CcdRadius );
    }
}

void APhysicalBody::SetCcdMotionThreshold( float _Threshold ) {
    CcdMotionThreshold = Math::Max( _Threshold, 0.0f );

    if ( RigidBody ) {
        RigidBody->setCcdMotionThreshold( CcdMotionThreshold );
    }

    if ( SoftBody ) {
        SoftBody->setCcdMotionThreshold( CcdMotionThreshold );
    }
}

Float3 APhysicalBody::GetLinearVelocity() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getLinearVelocity() ) : Float3::Zero();
}

Float3 const & APhysicalBody::GetLinearFactor() const {
    return LinearFactor;
}

Float3 APhysicalBody::GetVelocityAtPoint( Float3 const & _Position ) const {
    return RigidBody ? btVectorToFloat3( RigidBody->getVelocityInLocalPoint( btVectorToFloat3( _Position - MotionState->CenterOfMass ) ) ) : Float3::Zero();
}

float APhysicalBody::GetLinearSleepingThreshold() const {
    return LinearSleepingThreshold;
}

float APhysicalBody::GetLinearDamping() const {
    return LinearDamping;
}

Float3 APhysicalBody::GetAngularVelocity() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getAngularVelocity() ) : Float3::Zero();
}

Float3 const & APhysicalBody::GetAngularFactor() const {
    return AngularFactor;
}

float APhysicalBody::GetAngularSleepingThreshold() const {
    return AngularSleepingThreshold;
}

float APhysicalBody::GetAngularDamping() const {
    return AngularDamping;
}

float APhysicalBody::GetFriction() const {
    return Friction;
}

Float3 const & APhysicalBody::GetAnisotropicFriction() const {
    return AnisotropicFriction;
}

float APhysicalBody::GetRollingFriction() const {
    return RollingFriction;
}

float APhysicalBody::GetRestitution() const {
    return Restitution;
}

float APhysicalBody::GetContactProcessingThreshold() const {
    return ContactProcessingThreshold;
}

float APhysicalBody::GetCcdRadius() const {
    return CcdRadius;
}

float APhysicalBody::GetCcdMotionThreshold() const {
    return CcdMotionThreshold;
}

Float3 const & APhysicalBody::GetCenterOfMass() const {
    return MotionState ? MotionState->CenterOfMass : Float3::Zero();
}

Float3 APhysicalBody::GetCenterOfMassWorldPosition() const {
    return RigidBody ? btVectorToFloat3( RigidBody->getWorldTransform().getOrigin() ) : GetWorldPosition();
}

void APhysicalBody::ActivatePhysics() {
    if ( PhysicsBehavior == PB_DYNAMIC ) {
        if ( RigidBody ) {
            RigidBody->activate( true );
        }
    }

    if ( SoftBody ) {
        SoftBody->activate( true );
    }
}

bool APhysicalBody::IsPhysicsActive() const {
    
    if ( RigidBody ) {
        return RigidBody->isActive();
    }

    if ( SoftBody ) {
        return SoftBody->isActive();
    }

    return false;
}

void APhysicalBody::ClearForces() {
    if ( RigidBody ) {
        RigidBody->clearForces();
    }
}

void APhysicalBody::ApplyCentralForce( Float3 const & _Force ) {
    if ( RigidBody && _Force != Float3::Zero() ) {
        ActivatePhysics();
        RigidBody->applyCentralForce( btVectorToFloat3( _Force ) );
    }
}

void APhysicalBody::ApplyForce( Float3 const & _Force, Float3 const & _Position ) {
    if ( RigidBody && _Force != Float3::Zero() ) {
        ActivatePhysics();
        RigidBody->applyForce( btVectorToFloat3( _Force ), btVectorToFloat3( _Position - MotionState->CenterOfMass ) );
    }
}

void APhysicalBody::ApplyTorque( Float3 const & _Torque ) {
    if ( RigidBody && _Torque != Float3::Zero() ) {
        ActivatePhysics();
        RigidBody->applyTorque( btVectorToFloat3( _Torque ) );
    }
}

void APhysicalBody::ApplyCentralImpulse( Float3 const & _Impulse ) {
    if ( RigidBody && _Impulse != Float3::Zero() ) {
        ActivatePhysics();
        RigidBody->applyCentralImpulse( btVectorToFloat3( _Impulse ) );
    }
}

void APhysicalBody::ApplyImpulse( Float3 const & _Impulse, Float3 const & _Position ) {
    if ( RigidBody && _Impulse != Float3::Zero() ) {
        ActivatePhysics();
        RigidBody->applyImpulse( btVectorToFloat3( _Impulse ), btVectorToFloat3( _Position - MotionState->CenterOfMass ) );
    }
}

void APhysicalBody::ApplyTorqueImpulse( Float3 const & _Torque ) {
    if ( RigidBody && _Torque != Float3::Zero() ) {
        ActivatePhysics();
        RigidBody->applyTorqueImpulse( btVectorToFloat3( _Torque ) );
    }
}

void APhysicalBody::GetCollisionBodiesWorldBounds( TPodArray< BvAxisAlignedBox > & _BoundingBoxes ) const {
    if ( !RigidBody ) {
        _BoundingBoxes.Clear();
    } else {
        btVector3 mins, maxs;

        int numShapes = CompoundShape->getNumChildShapes();

        _BoundingBoxes.ResizeInvalidate( numShapes );

        for ( int i = 0 ; i < numShapes ; i++ ) {
            btCompoundShapeChild & shape = CompoundShape->getChildList()[ i ];

            shape.m_childShape->getAabb( RigidBody->getWorldTransform() * shape.m_transform, mins, maxs );

            _BoundingBoxes[i].Mins = btVectorToFloat3( mins );
            _BoundingBoxes[i].Maxs = btVectorToFloat3( maxs );
        }
    }
}

void APhysicalBody::GetCollisionWorldBounds( BvAxisAlignedBox & _BoundingBox ) const {
    _BoundingBox.Clear();

    if ( !RigidBody ) {
        return;
    }

    btVector3 mins, maxs;

    int numShapes = CompoundShape->getNumChildShapes();

    for ( int i = 0 ; i < numShapes ; i++ ) {
        btCompoundShapeChild & shape = CompoundShape->getChildList()[ i ];

        shape.m_childShape->getAabb( RigidBody->getWorldTransform() * shape.m_transform, mins, maxs );

        _BoundingBox.AddAABB( btVectorToFloat3( mins ), btVectorToFloat3( maxs ) );
    }
}

void APhysicalBody::GetCollisionBodyWorldBounds( int _Index, BvAxisAlignedBox & _BoundingBox ) const {
    if ( !RigidBody || _Index >= CompoundShape->getNumChildShapes() ) {
        _BoundingBox.Clear();
        return;
    }

    btVector3 mins, maxs;

    btCompoundShapeChild & shape = CompoundShape->getChildList()[ _Index ];

    shape.m_childShape->getAabb( RigidBody->getWorldTransform() * shape.m_transform, mins, maxs );

    _BoundingBox.Mins = btVectorToFloat3( mins );
    _BoundingBox.Maxs = btVectorToFloat3( maxs );
}

void APhysicalBody::GetCollisionBodyLocalBounds( int _Index, BvAxisAlignedBox & _BoundingBox ) const {
    if ( !RigidBody || _Index >= CompoundShape->getNumChildShapes() ) {
        _BoundingBox.Clear();
        return;
    }

    btVector3 mins, maxs;

    btCompoundShapeChild & shape = CompoundShape->getChildList()[ _Index ];

    shape.m_childShape->getAabb( shape.m_transform, mins, maxs );

    _BoundingBox.Mins = btVectorToFloat3( mins );
    _BoundingBox.Maxs = btVectorToFloat3( maxs );
}

float APhysicalBody::GetCollisionBodyMargin( int _Index ) const {
    if ( !RigidBody || _Index >= CompoundShape->getNumChildShapes() ) {
        return 0;
    }

    btCompoundShapeChild & shape = CompoundShape->getChildList()[ _Index ];

    return shape.m_childShape->getMargin();
}

int APhysicalBody::GetCollisionBodiesCount() const {
    if ( !RigidBody ) {
        return 0;
    }

    return CompoundShape->getNumChildShapes();
}

void APhysicalBody::CreateCollisionModel( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) {
    ACollisionBodyComposition const & collisionBody = GetBodyComposition();

    int firstVertex = _Vertices.Size();

    collisionBody.CreateGeometry( _Vertices, _Indices );

    int numVertices = _Vertices.Size() - firstVertex;

    if ( numVertices > 0 )
    {
        Float3 * pVertices = _Vertices.ToPtr() + firstVertex;

        Float3x4 const & worldTransofrm = GetWorldTransformMatrix();
        for ( int i = 0 ; i < numVertices ; i++, pVertices++ ) {
            *pVertices = worldTransofrm * (*pVertices);
        }
    }
}

struct SContactQueryCallback : public btCollisionWorld::ContactResultCallback {
    SContactQueryCallback( TPodArray< APhysicalBody * > & _Result, int _CollisionMask, APhysicalBody const * _Self )
        : Result( _Result )
        , CollisionMask( _CollisionMask )
        , Self( _Self )
    {
        _Result.Clear();
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        APhysicalBody * body;

        body = reinterpret_cast< APhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && body != Self && Result.Find( body ) == Result.End() && ( body->GetCollisionGroup() & CollisionMask ) ) {
            Result.Append( body );
        }

        body = reinterpret_cast< APhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && body != Self && Result.Find( body ) == Result.End() && ( body->GetCollisionGroup() & CollisionMask ) ) {
            Result.Append( body );
        }

        return 0.0f;
    }

    TPodArray< APhysicalBody * > & Result;
    int CollisionMask;
    APhysicalBody const * Self;
};

struct SContactQueryActorCallback : public btCollisionWorld::ContactResultCallback {
    SContactQueryActorCallback( TPodArray< AActor * > & _Result, int _CollisionMask, AActor const * _Self )
        : Result( _Result )
        , CollisionMask( _CollisionMask )
        , Self( _Self )
    {
        _Result.Clear();
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        APhysicalBody * body;

        body = reinterpret_cast< APhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && body->GetParentActor() != Self && Result.Find( body->GetParentActor() ) == Result.End() && ( body->GetCollisionGroup() & CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        body = reinterpret_cast< APhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && body->GetParentActor() != Self && Result.Find( body->GetParentActor() ) == Result.End() && ( body->GetCollisionGroup() & CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        return 0.0f;
    }

    TPodArray< AActor * > & Result;
    int CollisionMask;
    AActor const * Self;
};

void APhysicalBody::CollisionContactQuery( TPodArray< APhysicalBody * > & _Result ) const {
    SContactQueryCallback callback( _Result, CollisionMask, this );

    if ( !RigidBody )
    {
        GLogger.Printf( "APhysicalBody::CollisionContactQuery: The object has no rigid body\n" );
        return;
    }

    if ( !bInWorld )
    {
        GLogger.Printf( "APhysicalBody::CollisionContactQuery: The body is not in world\n" );
        return;
    }

    GetWorld()->GetDynamicsWorld()->contactTest( RigidBody, callback );
}

void APhysicalBody::CollisionContactQueryActor( TPodArray< AActor * > & _Result ) const {
    SContactQueryActorCallback callback( _Result, CollisionMask, GetParentActor() );

    if ( !RigidBody )
    {
        GLogger.Printf( "APhysicalBody::CollisionContactQueryActor: The object has no rigid body\n" );
        return;
    }

    if ( !bInWorld )
    {
        GLogger.Printf( "APhysicalBody::CollisionContactQueryActor: The body is not in world\n" );
        return;
    }

    GetWorld()->GetDynamicsWorld()->contactTest( RigidBody, callback );
}

void APhysicalBody::BeginPlay() {
    Super::BeginPlay();
}

void APhysicalBody::EndPlay() {
    for ( AActor * actor : CollisionIgnoreActors ) {
        actor->RemoveRef();
    }

    CollisionIgnoreActors.Clear();

    Super::EndPlay();
}

void APhysicalBody::SetTrigger( bool _Trigger )
{
    if ( bTrigger == _Trigger )
    {
        return;
    }

    bTrigger = _Trigger;

    if ( IsInitialized() )
    {
        UpdatePhysicsAttribs();
    }
}

void APhysicalBody::SetDisableGravity( bool _DisableGravity )
{
    if ( bDisableGravity == _DisableGravity )
    {
        return;
    }

    bDisableGravity = _DisableGravity;

    if ( IsInitialized() )
    {
        UpdatePhysicsAttribs();
    }
}

void APhysicalBody::SetOverrideWorldGravity( bool _OverrideWorldGravity )
{
    if ( bOverrideWorldGravity == _OverrideWorldGravity )
    {
        return;
    }

    bOverrideWorldGravity = _OverrideWorldGravity;

    if ( IsInitialized() )
    {
        UpdatePhysicsAttribs();
    }
}

void APhysicalBody::SetSelfGravity( Float3 const & _SelfGravity )
{
    if ( SelfGravity == _SelfGravity )
    {
        return;
    }

    SelfGravity = _SelfGravity;

    if ( IsInitialized() )
    {
        UpdatePhysicsAttribs();
    }
}


void APhysicalBody::SetMass( float _Mass )
{
    if ( Mass == _Mass )
    {
        return;
    }

    Mass = _Mass;

    if ( IsInitialized() )
    {
        UpdatePhysicsAttribs();
    }
}

void APhysicalBody::SetCollisionGroup( int _CollisionGroup ) {
    if ( CollisionGroup == _CollisionGroup )
    {
        return;
    }

    CollisionGroup = _CollisionGroup;

    if ( IsInitialized() )
    {
        // Re-add rigid body to physics world
        AddPhysicalBodyToWorld();
    }
}

void APhysicalBody::SetCollisionMask( int _CollisionMask ) {
    if ( CollisionMask == _CollisionMask )
    {
        return;
    }

    CollisionMask = _CollisionMask;

    if ( IsInitialized() )
    {
        // Re-add rigid body to physics world
        AddPhysicalBodyToWorld();
    }
}

void APhysicalBody::SetCollisionFilter( int _CollisionGroup, int _CollisionMask ) {
    if ( CollisionGroup == _CollisionGroup && CollisionMask == _CollisionMask )
    {
        return;
    }

    CollisionGroup = _CollisionGroup;
    CollisionMask = _CollisionMask;

    if ( IsInitialized() )
    {
        // Re-add rigid body to physics world
        AddPhysicalBodyToWorld();
    }
}

void APhysicalBody::AddCollisionIgnoreActor( AActor * _Actor ) {
    if ( !_Actor ) {
        return;
    }
    if ( CollisionIgnoreActors.Find( _Actor ) == CollisionIgnoreActors.End() ) {
        CollisionIgnoreActors.Append( _Actor );
        _Actor->AddRef();

        // Re-add rigid body to physics world
        AddPhysicalBodyToWorld();
    }
}

void APhysicalBody::RemoveCollisionIgnoreActor( AActor * _Actor ) {
    if ( !_Actor ) {
        return;
    }
    auto it = CollisionIgnoreActors.Find( _Actor );
    if ( it != CollisionIgnoreActors.End() ) {
        AActor * actor = *it;

        actor->RemoveRef();

        CollisionIgnoreActors.RemoveSwap( it - CollisionIgnoreActors.Begin() );

        // Re-add rigid body to physics world
        AddPhysicalBodyToWorld();
    }
}

void APhysicalBody::DrawDebug( ADebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawCollisionModel ) {
        TPodArray< Float3 > collisionVertices;
        TPodArray< unsigned int > collisionIndices;

        CreateCollisionModel( collisionVertices, collisionIndices );

        InRenderer->SetDepthTest(true);

        switch ( PhysicsBehavior ) {
        case PB_STATIC:
            InRenderer->SetColor( AColor4( 0.5f, 0.5f, 0.5f, 1 ) );
            break;
        case PB_DYNAMIC:
            InRenderer->SetColor( AColor4( 1, 0.5f, 0.5f, 1 ) );
            break;
        case PB_KINEMATIC:
            InRenderer->SetColor( AColor4( 0.5f, 0.5f, 1, 1 ) );
            break;
        }

        InRenderer->DrawTriangleSoup(collisionVertices.ToPtr(),collisionVertices.Size(),sizeof(Float3),collisionIndices.ToPtr(),collisionIndices.Size(),false);
        InRenderer->DrawTriangleSoupWireframe( collisionVertices.ToPtr(), sizeof(Float3), collisionIndices.ToPtr(), collisionIndices.Size() );
    }

    if ( bTrigger && RVDrawTriggerBounds ) {
        TPodArray< BvAxisAlignedBox > boundingBoxes;

        GetCollisionBodiesWorldBounds( boundingBoxes );

        InRenderer->SetDepthTest( false );
        InRenderer->SetColor( AColor4( 1, 0, 1, 1 ) );
        for ( BvAxisAlignedBox const & bb : boundingBoxes ) {
            InRenderer->DrawAABB( bb );
        }
    } else {
        if ( PhysicsBehavior == PB_STATIC && RVDrawStaticCollisionBounds ) {
            TPodArray< BvAxisAlignedBox > boundingBoxes;

            GetCollisionBodiesWorldBounds( boundingBoxes );

            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 0.5f, 0.5f, 0.5f, 1 ) );
            for ( BvAxisAlignedBox const & bb : boundingBoxes ) {
                InRenderer->DrawAABB( bb );
            }
        }

        if ( PhysicsBehavior == PB_DYNAMIC && RVDrawDynamicCollisionBounds ) {
            TPodArray< BvAxisAlignedBox > boundingBoxes;

            GetCollisionBodiesWorldBounds( boundingBoxes );

            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( IsPhysicsActive() ? AColor4( 0.1f, 1.0f, 0.1f, 1 ) : AColor4( 0.3f, 0.3f, 0.3f, 1 ) );
            for ( BvAxisAlignedBox const & bb : boundingBoxes ) {
                InRenderer->DrawAABB( bb );
            }
        }

        if ( PhysicsBehavior == PB_KINEMATIC && RVDrawKinematicCollisionBounds ) {
            TPodArray< BvAxisAlignedBox > boundingBoxes;

            GetCollisionBodiesWorldBounds( boundingBoxes );

            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 0.5f, 0.5f, 1, 1 ) );
            for ( BvAxisAlignedBox const & bb : boundingBoxes ) {
                InRenderer->DrawAABB( bb );
            }
        }
    }

    if ( RVDrawCenterOfMass ) {
        if ( RigidBody ) {
            Float3 centerOfMass = GetCenterOfMassWorldPosition();

            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( AColor4( 1, 0, 0, 1 ) );
            InRenderer->DrawBox( centerOfMass, Float3( 0.02f ) );
        }
    }
}
