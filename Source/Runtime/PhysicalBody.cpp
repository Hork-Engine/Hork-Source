/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "PhysicalBody.h"
#include "World.h"
#include "DebugRenderer.h"

#include <Platform/Logger.h>
#include <Core/ConsoleVar.h>

#include "BulletCompatibility.h"

#define PHYS_COMPARE_EPSILON 0.0001f
#define MIN_MASS             0.001f
#define MAX_MASS             1000.0f

AConsoleVar com_DrawCollisionModel(_CTS("com_DrawCollisionModel"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawCollisionShapes(_CTS("com_DrawCollisionShapes"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawTriggers(_CTS("com_DrawTriggers"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawBoneCollisionShapes(_CTS("com_DrawBoneCollisionShapes"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawStaticCollisionBounds(_CTS("com_DrawStaticCollisionBounds"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawSimulatedCollisionBounds(_CTS("com_DrawSimulatedCollisionBounds"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawKinematicCollisionBounds(_CTS("com_DrawKinematicCollisionBounds"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawBoneCollisionBounds(_CTS("com_DrawBoneCollisionBounds"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawTriggerBounds(_CTS("com_DrawTriggerBounds"), _CTS("0"), CVAR_CHEAT);
AConsoleVar com_DrawCenterOfMass(_CTS("com_DrawCenterOfMass"), _CTS("0"), CVAR_CHEAT);

static constexpr bool bUseInternalEdgeUtility = true;

class AMotionState : public btMotionState
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return GZoneMemory.Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        GZoneMemory.Free(_Ptr);
    }
};

class APhysicalBodyMotionState : public AMotionState
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return GZoneMemory.Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        GZoneMemory.Free(_Ptr);
    }

    APhysicalBodyMotionState() :
        WorldPosition(Float3(0)), WorldRotation(Quat::Identity()), CenterOfMass(Float3(0))
    {
    }

    // Overrides

    void getWorldTransform(btTransform& _CenterOfMassTransform) const override;
    void setWorldTransform(btTransform const& _CenterOfMassTransform) override;

    // Public members

    APhysicalBody* Self;
    mutable Float3 WorldPosition;
    mutable Quat   WorldRotation;
    Float3         CenterOfMass;
    bool           bDuringMotionStateUpdate = false;
};

void APhysicalBodyMotionState::getWorldTransform(btTransform& _CenterOfMassTransform) const
{
    WorldPosition = Self->GetWorldPosition();
    WorldRotation = Self->GetWorldRotation();

    _CenterOfMassTransform.setRotation(btQuaternionToQuat(WorldRotation));
    _CenterOfMassTransform.setOrigin(btVectorToFloat3(WorldPosition) + _CenterOfMassTransform.getBasis() * btVectorToFloat3(CenterOfMass));
}

void APhysicalBodyMotionState::setWorldTransform(btTransform const& _CenterOfMassTransform)
{
    if (Self->MotionBehavior != MB_SIMULATED)
    {
        GLogger.Printf("APhysicalBodyMotionState::SetWorldTransform for non-simulated %s\n", Self->GetObjectNameCStr());
        return;
    }

    bDuringMotionStateUpdate = true;
    WorldRotation            = btQuaternionToQuat(_CenterOfMassTransform.getRotation());
    WorldPosition            = btVectorToFloat3(_CenterOfMassTransform.getOrigin() - _CenterOfMassTransform.getBasis() * btVectorToFloat3(CenterOfMass));
    Self->SetWorldPosition(WorldPosition);
    Self->SetWorldRotation(WorldRotation);
    bDuringMotionStateUpdate = false;
}

HK_FORCEINLINE void SetAttributeFromString(EMotionBehavior& Attribute, AString const& String)
{
    if (String == "STATIC")
    {
        Attribute = MB_STATIC;
    }
    else if (String == "SIMULATED")
    {
        Attribute = MB_SIMULATED;
    }
    else
    {
        Attribute = MB_KINEMATIC;
    }
}

void SetAttributeToString(EMotionBehavior Attribute, AString& String)
{
    switch (Attribute)
    {
        case MB_STATIC:
            String = "STATIC";
            break;
        case MB_SIMULATED:
            String = "SIMULATED";
            break;
        default:
            String = "KINEMATIC";
            break;
    }
}

HK_FORCEINLINE void SetAttributeFromString(EAINavigationBehavior& Attribute, AString const& String)
{
    if (String == "NONE")
    {
        Attribute = AI_NAVIGATION_BEHAVIOR_NONE;
    }
    else if (String == "STATIC")
    {
        Attribute = AI_NAVIGATION_BEHAVIOR_STATIC;
    }
    else if (String == "STATIC NON WALKABLE")
    {
        Attribute = AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE;
    }
    else if (String == "DYNAMIC")
    {
        Attribute = AI_NAVIGATION_BEHAVIOR_DYNAMIC;
    }
    else if (String == "DYNAMIC NON WALKABLE")
    {
        Attribute = AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE;
    }
    else
    {
        Attribute = AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE;
    }
}

void SetAttributeToString(EAINavigationBehavior Attribute, AString& String)
{
    switch (Attribute)
    {
        case AI_NAVIGATION_BEHAVIOR_NONE:
            String = "NONE";
            break;
        case AI_NAVIGATION_BEHAVIOR_STATIC:
            String = "STATIC";
            break;
        case AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE:
            String = "STATIC_NON_WALKABLE";
            break;
        case AI_NAVIGATION_BEHAVIOR_DYNAMIC:
            String = "DYNAMIC";
            break;
        case AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE:
            String = "DYNAMIC_NON_WALKABLE";
            break;
        default:
            String = "DYNAMIC_NON_WALKABLE";
            break;
    }
}

HK_BEGIN_CLASS_META(APhysicalBody)
HK_ATTRIBUTE(bDispatchContactEvents, bool, SetDispatchContactEvents, ShouldDispatchContactEvents, AF_DEFAULT)
HK_ATTRIBUTE(bDispatchOverlapEvents, bool, SetDispatchOverlapEvents, ShouldDispatchOverlapEvents, AF_DEFAULT)
HK_ATTRIBUTE(bGenerateContactPoints, bool, SetGenerateContactPoints, ShouldGenerateContactPoints, AF_DEFAULT)
HK_ATTRIBUTE_(bUseMeshCollision, AF_DEFAULT)
HK_ATTRIBUTE(MotionBehavior, EMotionBehavior, SetMotionBehavior, GetMotionBehavior, AF_DEFAULT)
HK_ATTRIBUTE(AINavigationBehavior, EAINavigationBehavior, SetAINavigationBehavior, GetAINavigationBehavior, AF_DEFAULT)
HK_ATTRIBUTE(IsTrigger, bool, SetTrigger, IsTrigger, AF_DEFAULT)
HK_ATTRIBUTE(DisableGravity, bool, SetDisableGravity, IsGravityDisabled, AF_DEFAULT)
HK_ATTRIBUTE(OverrideWorldGravity, bool, SetOverrideWorldGravity, IsWorldGravityOverriden, AF_DEFAULT)
HK_ATTRIBUTE(SelfGravity, Float3, SetSelfGravity, GetSelfGravity, AF_DEFAULT)
HK_ATTRIBUTE(Mass, float, SetMass, GetMass, AF_DEFAULT)
HK_ATTRIBUTE(CollisionGroup, int, SetCollisionGroup, GetCollisionGroup, AF_DEFAULT)
HK_ATTRIBUTE(CollisionMask, int, SetCollisionMask, GetCollisionMask, AF_DEFAULT)
HK_ATTRIBUTE(LinearSleepingThreshold, float, SetLinearSleepingThreshold, GetLinearSleepingThreshold, AF_DEFAULT)
HK_ATTRIBUTE(LinearDamping, float, SetLinearDamping, GetLinearDamping, AF_DEFAULT)
HK_ATTRIBUTE(AngularSleepingThreshold, float, SetAngularSleepingThreshold, GetAngularSleepingThreshold, AF_DEFAULT)
HK_ATTRIBUTE(AngularDamping, float, SetAngularDamping, GetAngularDamping, AF_DEFAULT)
HK_ATTRIBUTE(Friction, float, SetFriction, GetFriction, AF_DEFAULT)
HK_ATTRIBUTE(AnisotropicFriction, Float3, SetAnisotropicFriction, GetAnisotropicFriction, AF_DEFAULT)
HK_ATTRIBUTE(RollingFriction, float, SetRollingFriction, GetRollingFriction, AF_DEFAULT)
HK_ATTRIBUTE(Restitution, float, SetRestitution, GetRestitution, AF_DEFAULT)
HK_ATTRIBUTE(ContactProcessingThreshold, float, SetContactProcessingThreshold, GetContactProcessingThreshold, AF_DEFAULT)
HK_ATTRIBUTE(CcdRadius, float, SetCcdRadius, GetCcdRadius, AF_DEFAULT)
HK_ATTRIBUTE(CcdMotionThreshold, float, SetCcdMotionThreshold, GetCcdMotionThreshold, AF_DEFAULT)
HK_END_CLASS_META()

APhysicalBody::APhysicalBody()
{
    HitProxy = CreateInstanceOf<AHitProxy>();
}

bool APhysicalBody::ShouldHaveCollisionBody() const
{
    if (bSoftBodySimulation)
    {
        return false;
    }

    if (!HitProxy->GetCollisionGroup())
    {
        return false;
    }

    if (IsInEditor())
    {
        return false;
    }

    ACollisionModel const* collisionModel = GetCollisionModel();
    if (!collisionModel)
    {
        return false;
    }

    if (collisionModel->IsEmpty())
    {
        return false;
    }

    return true;
}

void APhysicalBody::InitializeComponent()
{
    Super::InitializeComponent();

    if (ShouldHaveCollisionBody())
    {
        CreateRigidBody();
    }

    CreateBoneCollisions();

    if (AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE)
    {
        AAINavigationMesh& NavigationMesh = GetWorld()->GetNavigationMesh();
        NavigationMesh.AddNavigationGeometry(this);
    }
}

void APhysicalBody::DeinitializeComponent()
{
    DestroyRigidBody();

    ClearBoneCollisions();

    AAINavigationMesh& NavigationMesh = GetWorld()->GetNavigationMesh();
    NavigationMesh.RemoveNavigationGeometry(this);

    Super::DeinitializeComponent();
}

void APhysicalBody::SetMotionBehavior(EMotionBehavior _MotionBehavior)
{
    if (MotionBehavior == _MotionBehavior)
    {
        return;
    }

    MotionBehavior = _MotionBehavior;

    UpdatePhysicsAttribs();
}

void APhysicalBody::SetAINavigationBehavior(EAINavigationBehavior _AINavigationBehavior)
{
    if (AINavigationBehavior == _AINavigationBehavior)
    {
        return;
    }

    AINavigationBehavior = _AINavigationBehavior;

    if (IsInitialized())
    {
        AAINavigationMesh& NavigationMesh = GetWorld()->GetNavigationMesh();

        NavigationMesh.RemoveNavigationGeometry(this);

        if (AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE)
        {
            NavigationMesh.AddNavigationGeometry(this);
        }
    }
}

class ABoneCollisionInstance : public AMotionState
{
public:
    void getWorldTransform(btTransform& _CenterOfMassTransform) const override
    {
        Float3x4 jointTransform = Self->GetWorldTransformMatrix() * Self->_GetJointTransform(HitProxy->GetJointIndex());

        Float3 position = jointTransform.DecomposeTranslation();
        Quat   rotation;

        rotation.FromMatrix(jointTransform.DecomposeRotation());

        Float3 localPosition = Self->CachedScale * OffsetPosition;

        _CenterOfMassTransform.setRotation(btQuaternionToQuat(rotation * OffsetRotation));
        _CenterOfMassTransform.setOrigin(btVectorToFloat3(position) + _CenterOfMassTransform.getBasis() * btVectorToFloat3(localPosition));
    }

    void setWorldTransform(btTransform const& _CenterOfMassTransform) override
    {
        GLogger.Printf("ABoneCollisionInstance::SetWorldTransform for bone\n");
    }

    APhysicalBody* Self;

    TRef<AHitProxy> HitProxy;
    Float3          OffsetPosition;
    Quat            OffsetRotation;
};

void APhysicalBody::ClearBoneCollisions()
{
    for (int i = 0; i < BoneCollisionInst.Size(); i++)
    {
        ABoneCollisionInstance* boneCollision = BoneCollisionInst[i];

        btCollisionObject* colObject = boneCollision->HitProxy->GetCollisionObject();
        btCollisionShape*  shape     = colObject->getCollisionShape();

        boneCollision->HitProxy->Deinitialize();

        delete colObject;
        delete shape;
        delete boneCollision;
    }

    BoneCollisionInst.Clear();
}

void APhysicalBody::UpdateBoneCollisions()
{
    if (!IsInitialized())
    {
        return;
    }

    CreateBoneCollisions();
}

void APhysicalBody::CreateBoneCollisions()
{
    ClearBoneCollisions();

    ACollisionModel const* collisionModel = GetCollisionModel();

    if (!collisionModel)
    {
        return;
    }

    btRigidBody::btRigidBodyConstructionInfo constructInfo(0.0f, nullptr, nullptr);

    TStdVector<SBoneCollision> const& boneCollisions = collisionModel->GetBoneCollisions();

    BoneCollisionInst.Resize(boneCollisions.Size());
    for (int i = 0; i < boneCollisions.Size(); i++)
    {
        ABoneCollisionInstance*           boneCollision = new ABoneCollisionInstance;
        TUniqueRef<ACollisionBody> const& collisionBody = boneCollisions[i].CollisionBody;

        BoneCollisionInst[i] = boneCollision;

        boneCollision->Self           = this;
        boneCollision->OffsetPosition = collisionBody->Position;
        boneCollision->OffsetRotation = collisionBody->Rotation;
        boneCollision->HitProxy       = CreateInstanceOf<AHitProxy>();
        boneCollision->HitProxy->SetCollisionMask(boneCollisions[i].CollisionMask);
        boneCollision->HitProxy->SetCollisionGroup(boneCollisions[i].CollisionGroup);
        boneCollision->HitProxy->SetJointIndex(boneCollisions[i].JointIndex);

        btCollisionShape* shape = collisionBody->Create(CachedScale);
        shape->setMargin(collisionBody->Margin);

        constructInfo.m_motionState    = boneCollision;
        constructInfo.m_collisionShape = shape;

        int collisionFlags = btCollisionObject::CF_KINEMATIC_OBJECT;

        btRigidBody* rigidBody = new btRigidBody(constructInfo);
        rigidBody->setCollisionFlags(collisionFlags);
        rigidBody->forceActivationState(DISABLE_DEACTIVATION);
        rigidBody->setUserPointer(boneCollision->HitProxy.GetObject());

        boneCollision->HitProxy->Initialize(this, rigidBody);
    }
}

void APhysicalBody::SetCollisionModel(ACollisionModel* _CollisionModel)
{
    if (IsSame(CollisionModel, _CollisionModel))
    {
        return;
    }

    CollisionModel = _CollisionModel;

    UpdatePhysicsAttribs();
    UpdateBoneCollisions();
}

ACollisionModel* APhysicalBody::GetCollisionModel() const
{
    return bUseMeshCollision ? GetMeshCollisionModel() : CollisionModel;
}

void APhysicalBody::SetUseMeshCollision(bool _bUseMeshCollision)
{
    if (bUseMeshCollision == _bUseMeshCollision)
    {
        return;
    }

    bUseMeshCollision = _bUseMeshCollision;

    UpdatePhysicsAttribs();
    UpdateBoneCollisions();
}

void APhysicalBody::SetCollisionFlags()
{
    int collisionFlags = RigidBody->getCollisionFlags();

    if (HitProxy->IsTrigger())
    {
        collisionFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
    }
    if (MotionBehavior == MB_KINEMATIC)
    {
        collisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    if (MotionBehavior == MB_STATIC)
    {
        collisionFlags |= btCollisionObject::CF_STATIC_OBJECT;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_STATIC_OBJECT;
    }
    if (bUseInternalEdgeUtility && CollisionInstance->GetCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
    {
        collisionFlags |= btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    }

    RigidBody->setCollisionFlags(collisionFlags);
    RigidBody->forceActivationState(MotionBehavior == MB_KINEMATIC ? DISABLE_DEACTIVATION : ISLAND_SLEEPING);
}

void APhysicalBody::SetRigidBodyGravity()
{
    Float3 const& worldGravity = GetWorld()->GetGravityVector();

    int flags = RigidBody->getFlags();

    if (bDisableGravity || bOverrideWorldGravity)
    {
        flags |= BT_DISABLE_WORLD_GRAVITY;
    }
    else
    {
        flags &= ~BT_DISABLE_WORLD_GRAVITY;
    }

    RigidBody->setFlags(flags);

    if (bDisableGravity)
    {
        RigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
    }
    else if (bOverrideWorldGravity)
    {
        // Use self gravity
        RigidBody->setGravity(btVectorToFloat3(SelfGravity));
    }
    else
    {
        // Use world gravity
        RigidBody->setGravity(btVectorToFloat3(worldGravity));
    }
}

void APhysicalBody::CreateRigidBody()
{
    HK_ASSERT(MotionState == nullptr);
    HK_ASSERT(RigidBody == nullptr);
    HK_ASSERT(CollisionInstance == nullptr);

    CachedScale = GetWorldScale();

    MotionState       = new APhysicalBodyMotionState;
    MotionState->Self = this;

    CollisionInstance         = GetCollisionModel()->Instantiate(CachedScale);
    MotionState->CenterOfMass = CollisionInstance->GetCenterOfMass();

    Float3 localInertia(0.0f, 0.0f, 0.0f);
    float  mass = 0;

    if (MotionBehavior == MB_SIMULATED)
    {
        mass         = Math::Clamp(Mass, MIN_MASS, MAX_MASS);
        localInertia = CollisionInstance->CalculateLocalInertia(mass);
    }

    btRigidBody::btRigidBodyConstructionInfo constructInfo(mass, MotionState, CollisionInstance->GetCollisionShape(), btVectorToFloat3(localInertia));
    constructInfo.m_linearDamping   = LinearDamping;
    constructInfo.m_angularDamping  = AngularDamping;
    constructInfo.m_friction        = Friction;
    constructInfo.m_rollingFriction = RollingFriction;
    //    constructInfo.m_spinningFriction;
    constructInfo.m_restitution              = Restitution;
    constructInfo.m_linearSleepingThreshold  = LinearSleepingThreshold;
    constructInfo.m_angularSleepingThreshold = AngularSleepingThreshold;

    RigidBody = new btRigidBody(constructInfo);
    RigidBody->setUserPointer(HitProxy.GetObject());

    SetCollisionFlags();
    SetRigidBodyGravity();

    HitProxy->Initialize(this, RigidBody);

    ActivatePhysics();

    // Update dynamic attributes
    SetLinearFactor(LinearFactor);
    SetAngularFactor(AngularFactor);
    SetAnisotropicFriction(AnisotropicFriction);
    SetContactProcessingThreshold(ContactProcessingThreshold);
    SetCcdRadius(CcdRadius);
    SetCcdMotionThreshold(CcdMotionThreshold);

    UpdateDebugDrawCache();
}

void APhysicalBody::DestroyRigidBody()
{
    if (!RigidBody)
    {
        // Rigid body wasn't created
        return;
    }

    HitProxy->Deinitialize();

    delete RigidBody;
    RigidBody = nullptr;

    CollisionInstance.Reset();

    delete MotionState;
    MotionState = nullptr;

    UpdateDebugDrawCache();
}

void APhysicalBody::UpdatePhysicsAttribs()
{
    if (!IsInitialized())
    {
        return;
    }

    if (!ShouldHaveCollisionBody())
    {
        DestroyRigidBody();
        return;
    }

    if (!RigidBody)
    {
        CreateRigidBody();
        return;
    }

    btTransform const& centerOfMassTransform = RigidBody->getWorldTransform();
    Float3             position              = btVectorToFloat3(centerOfMassTransform.getOrigin() - centerOfMassTransform.getBasis() * btVectorToFloat3(MotionState->CenterOfMass));

    CachedScale = GetWorldScale();

    CollisionInstance         = GetCollisionModel()->Instantiate(CachedScale);
    MotionState->CenterOfMass = CollisionInstance->GetCenterOfMass();

    float mass = Math::Clamp(Mass, MIN_MASS, MAX_MASS);
    if (MotionBehavior == MB_SIMULATED)
    {
        Float3 localInertia = CollisionInstance->CalculateLocalInertia(mass);

        RigidBody->setMassProps(mass, btVectorToFloat3(localInertia));
    }
    else
    {
        RigidBody->setMassProps(0.0f, btVector3(0, 0, 0));
    }

    // Intertia tensor is based on transform orientation and mass props, so we need to update it too
    RigidBody->updateInertiaTensor();

    RigidBody->setCollisionShape(CollisionInstance->GetCollisionShape());

    SetCollisionFlags();

    // Update position with new center of mass
    SetCenterOfMassPosition(position);

    HitProxy->UpdateBroadphase(); // FIXME: is it need?

    SetRigidBodyGravity();

    ActivatePhysics();

    UpdateDebugDrawCache();

    // Update dynamic attributes
    //    SetLinearFactor( LinearFactor );
    //    SetAngularFactor( AngularFactor );
    //    SetAnisotropicFriction( AnisotropicFriction );
    //    SetContactProcessingThreshold( ContactProcessingThreshold );
    //    SetCcdRadius( CcdRadius );
    //    SetCcdMotionThreshold( CcdMotionThreshold );
}

void APhysicalBody::OnTransformDirty()
{
    Super::OnTransformDirty();

    if (RigidBody)
    {
        if (!MotionState->bDuringMotionStateUpdate)
        {
            if (MotionBehavior != MB_KINEMATIC)
            {
                Float3 position = GetWorldPosition();
                Quat   rotation = GetWorldRotation();

                if (rotation != MotionState->WorldRotation)
                {
                    MotionState->WorldRotation = rotation;
                    SetCenterOfMassRotation(rotation);
                }
                if (position != MotionState->WorldPosition)
                {
                    MotionState->WorldPosition = position;
                    SetCenterOfMassPosition(position);
                }

                if (!IsInEditor())
                {
                    GLogger.Printf("WARNING: Set transform for non-KINEMATIC body %s\n", GetObjectNameCStr());
                }
            }
        }

        int numBodies = CollisionInstance->GetCollisionBodiesCount();

        if ((numBodies > 0 /*|| !BoneCollisionInst.IsEmpty()*/) && !CachedScale.CompareEps(GetWorldScale(), PHYS_COMPARE_EPSILON))
        {
            UpdatePhysicsAttribs();

            //UpdateBoneCollisions();
        }

        UpdateDebugDrawCache();
    }
    else
    {
        if (MotionBehavior != MB_KINEMATIC && !GetOwnerActor()->IsSpawning() && !IsInEditor())
        {
            GLogger.Printf("WARNING: Set transform for non-KINEMATIC body %s\n", GetObjectNameCStr());
        }
    }

    //if ( SoftBody && !bUpdateSoftbodyTransform ) {
    //    if ( !PrevWorldPosition.CompareEps( GetWorldPosition(), PHYS_COMPARE_EbPSILON )
    //        || !PrevWorldRotation.CompareEps( GetWorldRotation(), PHYS_COMPARE_EPSILON ) ) {
    //        bUpdateSoftbodyTransform = true;

    //        // TODO: add to dirty list?
    //    }
    //}
}

void APhysicalBody::SetCenterOfMassPosition(Float3 const& _Position)
{
    HK_ASSERT(RigidBody);

    btTransform& centerOfMassTransform = RigidBody->getWorldTransform();
    centerOfMassTransform.setOrigin(btVectorToFloat3(_Position) + centerOfMassTransform.getBasis() * btVectorToFloat3(MotionState->CenterOfMass));

    if (GetWorld()->IsDuringPhysicsUpdate())
    {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setOrigin(centerOfMassTransform.getOrigin());
        RigidBody->setInterpolationWorldTransform(interpolationWorldTransform);
    }

    ActivatePhysics();
}

void APhysicalBody::SetCenterOfMassRotation(Quat const& _Rotation)
{
    HK_ASSERT(RigidBody);

    btTransform& centerOfMassTransform = RigidBody->getWorldTransform();

    btVector3 bodyPrevPosition = centerOfMassTransform.getOrigin() - centerOfMassTransform.getBasis() * btVectorToFloat3(MotionState->CenterOfMass);

    centerOfMassTransform.setRotation(btQuaternionToQuat(_Rotation));

    if (!MotionState->CenterOfMass.CompareEps(Float3::Zero(), PHYS_COMPARE_EPSILON))
    {
        centerOfMassTransform.setOrigin(bodyPrevPosition + centerOfMassTransform.getBasis() * btVectorToFloat3(MotionState->CenterOfMass));
    }

    if (GetWorld()->IsDuringPhysicsUpdate())
    {
        btTransform interpolationWorldTransform = RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setBasis(centerOfMassTransform.getBasis());
        if (!MotionState->CenterOfMass.CompareEps(Float3::Zero(), PHYS_COMPARE_EPSILON))
        {
            interpolationWorldTransform.setOrigin(centerOfMassTransform.getOrigin());
        }
        RigidBody->setInterpolationWorldTransform(interpolationWorldTransform);
    }

    // Intertia tensor is based on transform orientation and mass props, so we need to update it too
    RigidBody->updateInertiaTensor();

    ActivatePhysics();
}

void APhysicalBody::SetLinearVelocity(Float3 const& _Velocity)
{
    if (RigidBody)
    {
        RigidBody->setLinearVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }

    if (SoftBody)
    {
        SoftBody->setVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::AddLinearVelocity(Float3 const& _Velocity)
{
    if (RigidBody)
    {
        RigidBody->setLinearVelocity(RigidBody->getLinearVelocity() + btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }

    if (SoftBody)
    {
        SoftBody->addVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::SetLinearFactor(Float3 const& _Factor)
{
    if (RigidBody)
    {
        RigidBody->setLinearFactor(btVectorToFloat3(_Factor));
    }

    LinearFactor = _Factor;
}

void APhysicalBody::SetLinearSleepingThreshold(float _Threshold)
{
    if (RigidBody)
    {
        RigidBody->setSleepingThresholds(_Threshold, AngularSleepingThreshold);
    }

    LinearSleepingThreshold = _Threshold;
}

void APhysicalBody::SetLinearDamping(float _Damping)
{
    if (RigidBody)
    {
        RigidBody->setDamping(_Damping, AngularDamping);
    }

    LinearDamping = _Damping;
}

void APhysicalBody::SetAngularVelocity(Float3 const& _Velocity)
{
    if (RigidBody)
    {
        RigidBody->setAngularVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::AddAngularVelocity(Float3 const& _Velocity)
{
    if (RigidBody)
    {
        RigidBody->setAngularVelocity(RigidBody->getAngularVelocity() + btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void APhysicalBody::SetAngularFactor(Float3 const& _Factor)
{
    if (RigidBody)
    {
        RigidBody->setAngularFactor(btVectorToFloat3(_Factor));
    }

    AngularFactor = _Factor;
}

void APhysicalBody::SetAngularSleepingThreshold(float _Threshold)
{
    if (RigidBody)
    {
        RigidBody->setSleepingThresholds(LinearSleepingThreshold, _Threshold);
    }

    AngularSleepingThreshold = _Threshold;
}

void APhysicalBody::SetAngularDamping(float _Damping)
{
    if (RigidBody)
    {
        RigidBody->setDamping(LinearDamping, _Damping);
    }

    AngularDamping = _Damping;
}

void APhysicalBody::SetFriction(float _Friction)
{
    if (RigidBody)
    {
        RigidBody->setFriction(_Friction);
    }

    if (SoftBody)
    {
        SoftBody->setFriction(_Friction);
    }

    Friction = _Friction;
}

void APhysicalBody::SetAnisotropicFriction(Float3 const& _Friction)
{
    if (RigidBody)
    {
        RigidBody->setAnisotropicFriction(btVectorToFloat3(_Friction));
    }

    if (SoftBody)
    {
        SoftBody->setAnisotropicFriction(btVectorToFloat3(_Friction));
    }

    AnisotropicFriction = _Friction;
}

void APhysicalBody::SetRollingFriction(float _Friction)
{
    if (RigidBody)
    {
        RigidBody->setRollingFriction(_Friction);
    }

    if (SoftBody)
    {
        SoftBody->setRollingFriction(_Friction);
    }

    RollingFriction = _Friction;
}

void APhysicalBody::SetRestitution(float _Restitution)
{
    if (RigidBody)
    {
        RigidBody->setRestitution(_Restitution);
    }

    if (SoftBody)
    {
        SoftBody->setRestitution(_Restitution);
    }

    Restitution = _Restitution;
}

void APhysicalBody::SetContactProcessingThreshold(float _Threshold)
{
    if (RigidBody)
    {
        RigidBody->setContactProcessingThreshold(_Threshold);
    }

    if (SoftBody)
    {
        SoftBody->setContactProcessingThreshold(_Threshold);
    }

    ContactProcessingThreshold = _Threshold;
}

void APhysicalBody::SetCcdRadius(float _Radius)
{
    CcdRadius = Math::Max(_Radius, 0.0f);

    if (RigidBody)
    {
        RigidBody->setCcdSweptSphereRadius(CcdRadius);
    }

    if (SoftBody)
    {
        SoftBody->setCcdSweptSphereRadius(CcdRadius);
    }
}

void APhysicalBody::SetCcdMotionThreshold(float _Threshold)
{
    CcdMotionThreshold = Math::Max(_Threshold, 0.0f);

    if (RigidBody)
    {
        RigidBody->setCcdMotionThreshold(CcdMotionThreshold);
    }

    if (SoftBody)
    {
        SoftBody->setCcdMotionThreshold(CcdMotionThreshold);
    }
}

Float3 APhysicalBody::GetLinearVelocity() const
{
    return RigidBody ? btVectorToFloat3(RigidBody->getLinearVelocity()) : Float3::Zero();
}

Float3 const& APhysicalBody::GetLinearFactor() const
{
    return LinearFactor;
}

Float3 APhysicalBody::GetVelocityAtPoint(Float3 const& _Position) const
{
    return RigidBody ? btVectorToFloat3(RigidBody->getVelocityInLocalPoint(btVectorToFloat3(_Position - MotionState->CenterOfMass))) : Float3::Zero();
}

float APhysicalBody::GetLinearSleepingThreshold() const
{
    return LinearSleepingThreshold;
}

float APhysicalBody::GetLinearDamping() const
{
    return LinearDamping;
}

Float3 APhysicalBody::GetAngularVelocity() const
{
    return RigidBody ? btVectorToFloat3(RigidBody->getAngularVelocity()) : Float3::Zero();
}

Float3 const& APhysicalBody::GetAngularFactor() const
{
    return AngularFactor;
}

float APhysicalBody::GetAngularSleepingThreshold() const
{
    return AngularSleepingThreshold;
}

float APhysicalBody::GetAngularDamping() const
{
    return AngularDamping;
}

float APhysicalBody::GetFriction() const
{
    return Friction;
}

Float3 const& APhysicalBody::GetAnisotropicFriction() const
{
    return AnisotropicFriction;
}

float APhysicalBody::GetRollingFriction() const
{
    return RollingFriction;
}

float APhysicalBody::GetRestitution() const
{
    return Restitution;
}

float APhysicalBody::GetContactProcessingThreshold() const
{
    return ContactProcessingThreshold;
}

float APhysicalBody::GetCcdRadius() const
{
    return CcdRadius;
}

float APhysicalBody::GetCcdMotionThreshold() const
{
    return CcdMotionThreshold;
}

Float3 const& APhysicalBody::GetCenterOfMass() const
{
    return MotionState ? MotionState->CenterOfMass : Float3::Zero();
}

Float3 APhysicalBody::GetCenterOfMassWorldPosition() const
{
    return RigidBody ? btVectorToFloat3(RigidBody->getWorldTransform().getOrigin()) : GetWorldPosition();
}

void APhysicalBody::ActivatePhysics()
{
    if (MotionBehavior == MB_SIMULATED)
    {
        if (RigidBody)
        {
            RigidBody->activate(true);
        }
    }

    if (SoftBody)
    {
        SoftBody->activate(true);
    }
}

bool APhysicalBody::IsPhysicsActive() const
{
    if (RigidBody)
    {
        return RigidBody->isActive();
    }

    if (SoftBody)
    {
        return SoftBody->isActive();
    }

    return false;
}

void APhysicalBody::ClearForces()
{
    if (RigidBody)
    {
        RigidBody->clearForces();
    }
}

void APhysicalBody::ApplyCentralForce(Float3 const& _Force)
{
    if (RigidBody && _Force != Float3::Zero())
    {
        ActivatePhysics();
        RigidBody->applyCentralForce(btVectorToFloat3(_Force));
    }
}

void APhysicalBody::ApplyForce(Float3 const& _Force, Float3 const& _Position)
{
    if (RigidBody && _Force != Float3::Zero())
    {
        ActivatePhysics();
        RigidBody->applyForce(btVectorToFloat3(_Force), btVectorToFloat3(_Position - MotionState->CenterOfMass));
    }
}

void APhysicalBody::ApplyTorque(Float3 const& _Torque)
{
    if (RigidBody && _Torque != Float3::Zero())
    {
        ActivatePhysics();
        RigidBody->applyTorque(btVectorToFloat3(_Torque));
    }
}

void APhysicalBody::ApplyCentralImpulse(Float3 const& _Impulse)
{
    if (RigidBody && _Impulse != Float3::Zero())
    {
        ActivatePhysics();
        RigidBody->applyCentralImpulse(btVectorToFloat3(_Impulse));
    }
}

void APhysicalBody::ApplyImpulse(Float3 const& _Impulse, Float3 const& _Position)
{
    if (RigidBody && _Impulse != Float3::Zero())
    {
        ActivatePhysics();
        RigidBody->applyImpulse(btVectorToFloat3(_Impulse), btVectorToFloat3(_Position - MotionState->CenterOfMass));
    }
}

void APhysicalBody::ApplyTorqueImpulse(Float3 const& _Torque)
{
    if (RigidBody && _Torque != Float3::Zero())
    {
        ActivatePhysics();
        RigidBody->applyTorqueImpulse(btVectorToFloat3(_Torque));
    }
}

void APhysicalBody::GetCollisionBodiesWorldBounds(TPodVector<BvAxisAlignedBox>& _BoundingBoxes) const
{
    if (!CollisionInstance)
    {
        _BoundingBoxes.Clear();
        return;
    }

    CollisionInstance->GetCollisionBodiesWorldBounds(GetWorldPosition(), GetWorldRotation(), _BoundingBoxes);
}

void APhysicalBody::GetCollisionWorldBounds(BvAxisAlignedBox& _BoundingBox) const
{
    if (!CollisionInstance)
    {
        _BoundingBox.Clear();
        return;
    }

    CollisionInstance->GetCollisionWorldBounds(GetWorldPosition(), GetWorldRotation(), _BoundingBox);
}

void APhysicalBody::GetCollisionBodyWorldBounds(int _Index, BvAxisAlignedBox& _BoundingBox) const
{
    if (!CollisionInstance)
    {
        _BoundingBox.Clear();
        return;
    }

    CollisionInstance->GetCollisionBodyWorldBounds(_Index, GetWorldPosition(), GetWorldRotation(), _BoundingBox);
}

void APhysicalBody::GetCollisionBodyLocalBounds(int _Index, BvAxisAlignedBox& _BoundingBox) const
{
    if (!CollisionInstance)
    {
        _BoundingBox.Clear();
        return;
    }

    CollisionInstance->GetCollisionBodyLocalBounds(_Index, _BoundingBox);
}

float APhysicalBody::GetCollisionBodyMargin(int _Index) const
{
    if (!CollisionInstance)
    {
        return 0;
    }

    return CollisionInstance->GetCollisionBodyMargin(_Index);
}

int APhysicalBody::GetCollisionBodiesCount() const
{
    if (!CollisionInstance)
    {
        return 0;
    }

    return CollisionInstance->GetCollisionBodiesCount();
}

void APhysicalBody::GatherCollisionGeometry(TPodVectorHeap<Float3>& _Vertices, TPodVectorHeap<unsigned int>& _Indices) const
{
    ACollisionModel const* collisionModel = GetCollisionModel();
    if (!collisionModel)
    {
        return;
    }

    collisionModel->GatherGeometry(_Vertices, _Indices, GetWorldTransformMatrix());
}

void APhysicalBody::SetTrigger(bool _Trigger)
{
    if (HitProxy->IsTrigger() == _Trigger)
    {
        return;
    }

    HitProxy->SetTrigger(_Trigger);

    UpdatePhysicsAttribs();
}

void APhysicalBody::SetDisableGravity(bool _DisableGravity)
{
    if (bDisableGravity == _DisableGravity)
    {
        return;
    }

    bDisableGravity = _DisableGravity;

    UpdatePhysicsAttribs();
}

void APhysicalBody::SetOverrideWorldGravity(bool _OverrideWorldGravity)
{
    if (bOverrideWorldGravity == _OverrideWorldGravity)
    {
        return;
    }

    bOverrideWorldGravity = _OverrideWorldGravity;

    UpdatePhysicsAttribs();
}

void APhysicalBody::SetSelfGravity(Float3 const& _SelfGravity)
{
    if (SelfGravity == _SelfGravity)
    {
        return;
    }

    SelfGravity = _SelfGravity;

    UpdatePhysicsAttribs();
}

void APhysicalBody::SetMass(float _Mass)
{
    if (Mass == _Mass)
    {
        return;
    }

    Mass = _Mass;

    UpdatePhysicsAttribs();
}

void APhysicalBody::SetCollisionGroup(int _CollisionGroup)
{
    HitProxy->SetCollisionGroup(_CollisionGroup);
}

void APhysicalBody::SetCollisionMask(int _CollisionMask)
{
    HitProxy->SetCollisionMask(_CollisionMask);
}

void APhysicalBody::SetCollisionFilter(int _CollisionGroup, int _CollisionMask)
{
    HitProxy->SetCollisionFilter(_CollisionGroup, _CollisionMask);
}

void APhysicalBody::AddCollisionIgnoreActor(AActor* _Actor)
{
    HitProxy->AddCollisionIgnoreActor(_Actor);
}

void APhysicalBody::RemoveCollisionIgnoreActor(AActor* _Actor)
{
    HitProxy->RemoveCollisionIgnoreActor(_Actor);
}

void APhysicalBody::CollisionContactQuery(TPodVector<AHitProxy*>& _Result) const
{
    HitProxy->CollisionContactQuery(_Result);
}

void APhysicalBody::CollisionContactQueryActor(TPodVector<AActor*>& _Result) const
{
    HitProxy->CollisionContactQueryActor(_Result);
}

void APhysicalBody::UpdateDebugDrawCache()
{
    if (DebugDrawCache)
    {
        DebugDrawCache->bDirty = true;
    }
}

void APhysicalBody::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawCollisionModel || com_DrawTriggers)
    {
        if (!DebugDrawCache)
        {
            DebugDrawCache         = MakeUnique<SDebugDrawCache>();
            DebugDrawCache->bDirty = true;
        }

        if (DebugDrawCache->bDirty)
        {
            DebugDrawCache->Vertices.Clear();
            DebugDrawCache->Indices.Clear();
            DebugDrawCache->bDirty = false;
            GatherCollisionGeometry(DebugDrawCache->Vertices, DebugDrawCache->Indices);
        }

        InRenderer->SetDepthTest(false);

        if (HitProxy->IsTrigger())
        {
            if (com_DrawTriggers)
            {
                InRenderer->SetColor(Color4(0, 1, 0, 0.5f));

                InRenderer->DrawTriangleSoup(DebugDrawCache->Vertices.ToPtr(), DebugDrawCache->Vertices.Size(), sizeof(Float3),
                                             DebugDrawCache->Indices.ToPtr(), DebugDrawCache->Indices.Size(), false);
            }
        }
        else
        {
            if (com_DrawCollisionModel)
            {
                switch (MotionBehavior)
                {
                    case MB_STATIC: {
                        //uint32_t h = Core::Hash((const char*)&Id,sizeof(Id));
                        //Color4 c;
                        //c.SetDWord( h );
                        //c.SetAlpha( 0.1f );
                        //InRenderer->SetColor( c );
                        InRenderer->SetColor(Color4(0.5f, 0.5f, 0.5f, 0.1f));
                        break;
                    }
                    case MB_SIMULATED:
                        InRenderer->SetColor(Color4(1, 0.5f, 0.5f, 0.1f));
                        break;
                    case MB_KINEMATIC:
                        InRenderer->SetColor(Color4(0.5f, 0.5f, 1, 0.1f));
                        break;
                }

                InRenderer->DrawTriangleSoup(DebugDrawCache->Vertices.ToPtr(), DebugDrawCache->Vertices.Size(), sizeof(Float3),
                                             DebugDrawCache->Indices.ToPtr(), DebugDrawCache->Indices.Size(), false);

                InRenderer->SetColor(Color4(0, 0, 0, 1));
                InRenderer->DrawTriangleSoupWireframe(DebugDrawCache->Vertices.ToPtr(), sizeof(Float3),
                                                      DebugDrawCache->Indices.ToPtr(), DebugDrawCache->Indices.Size());
            }
        }
    }

    if (HitProxy->IsTrigger() && com_DrawTriggerBounds)
    {
        TPodVector<BvAxisAlignedBox> boundingBoxes;

        GetCollisionBodiesWorldBounds(boundingBoxes);

        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(1, 0, 1, 1));
        for (BvAxisAlignedBox const& bb : boundingBoxes)
        {
            InRenderer->DrawAABB(bb);
        }
    }
    else
    {
        if (MotionBehavior == MB_STATIC && com_DrawStaticCollisionBounds)
        {
            TPodVector<BvAxisAlignedBox> boundingBoxes;

            GetCollisionBodiesWorldBounds(boundingBoxes);

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
            for (BvAxisAlignedBox const& bb : boundingBoxes)
            {
                InRenderer->DrawAABB(bb);
            }
        }

        if (MotionBehavior == MB_SIMULATED && com_DrawSimulatedCollisionBounds)
        {
            TPodVector<BvAxisAlignedBox> boundingBoxes;

            GetCollisionBodiesWorldBounds(boundingBoxes);

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(IsPhysicsActive() ? Color4(0.1f, 1.0f, 0.1f, 1) : Color4(0.3f, 0.3f, 0.3f, 1));
            for (BvAxisAlignedBox const& bb : boundingBoxes)
            {
                InRenderer->DrawAABB(bb);
            }
        }

        if (MotionBehavior == MB_KINEMATIC && com_DrawKinematicCollisionBounds)
        {
            TPodVector<BvAxisAlignedBox> boundingBoxes;

            GetCollisionBodiesWorldBounds(boundingBoxes);

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(0.5f, 0.5f, 1, 1));
            for (BvAxisAlignedBox const& bb : boundingBoxes)
            {
                InRenderer->DrawAABB(bb);
            }
        }
    }

    if (com_DrawBoneCollisionBounds)
    {
        btVector3 mins, maxs;
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(1.0f, 1.0f, 0.0f, 1));
        for (ABoneCollisionInstance* boneCollision : BoneCollisionInst)
        {
            btCollisionObject* colObject = boneCollision->HitProxy->GetCollisionObject();
            btCollisionShape*  shape     = colObject->getCollisionShape();

            shape->getAabb(colObject->getWorldTransform(), mins, maxs);

            InRenderer->DrawAABB(BvAxisAlignedBox(btVectorToFloat3(mins), btVectorToFloat3(maxs)));
        }
    }

    if (com_DrawBoneCollisionShapes)
    {
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(1.0f, 1.0f, 0.0f, 1));
        for (ABoneCollisionInstance* boneCollision : BoneCollisionInst)
        {
            btCollisionObject* colObject = boneCollision->HitProxy->GetCollisionObject();

            btDrawCollisionShape(InRenderer, colObject->getWorldTransform(), colObject->getCollisionShape());
        }
    }

    if (com_DrawCenterOfMass)
    {
        if (RigidBody)
        {
            Float3 centerOfMass = GetCenterOfMassWorldPosition();

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 0, 0, 1));
            InRenderer->DrawBox(centerOfMass, Float3(0.02f));
        }
    }

    if (com_DrawCollisionShapes)
    {
        if (RigidBody)
        {
            InRenderer->SetDepthTest(false);
            btDrawCollisionObject(InRenderer, RigidBody);
        }
    }
}
