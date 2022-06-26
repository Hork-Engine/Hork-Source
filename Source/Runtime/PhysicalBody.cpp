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
#include <Geometry/BV/BvIntersect.h>

#include "BulletCompatibility.h"

#define PHYS_COMPARE_EPSILON 0.0001f
#define MIN_MASS             0.001f
#define MAX_MASS             1000.0f

AConsoleVar com_DrawCollisionModel("com_DrawCollisionModel"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawCollisionShapes("com_DrawCollisionShapes"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawTriggers("com_DrawTriggers"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawBoneCollisionShapes("com_DrawBoneCollisionShapes"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawStaticCollisionBounds("com_DrawStaticCollisionBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawSimulatedCollisionBounds("com_DrawSimulatedCollisionBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawKinematicCollisionBounds("com_DrawKinematicCollisionBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawBoneCollisionBounds("com_DrawBoneCollisionBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawTriggerBounds("com_DrawTriggerBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawCenterOfMass("com_DrawCenterOfMass"s, "0"s, CVAR_CHEAT);

static constexpr bool bUseInternalEdgeUtility = true;

class AMotionState : public btMotionState
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return Platform::GetHeapAllocator<HEAP_PHYSICS>().Alloc(_SizeInBytes, 16);
    }

    void operator delete(void* _Ptr)
    {
        Platform::GetHeapAllocator<HEAP_PHYSICS>().Free(_Ptr);
    }
};

class APhysicalBodyMotionState : public AMotionState
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return Platform::GetHeapAllocator<HEAP_PHYSICS>().Alloc(_SizeInBytes, 16);
    }

    void operator delete(void* _Ptr)
    {
        Platform::GetHeapAllocator<HEAP_PHYSICS>().Free(_Ptr);
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
        LOG("APhysicalBodyMotionState::SetWorldTransform for non-simulated {}\n", Self->GetObjectName());
        return;
    }

    bDuringMotionStateUpdate = true;
    WorldRotation            = btQuaternionToQuat(_CenterOfMassTransform.getRotation());
    WorldPosition            = btVectorToFloat3(_CenterOfMassTransform.getOrigin() - _CenterOfMassTransform.getBasis() * btVectorToFloat3(CenterOfMass));
    Self->SetWorldPosition(WorldPosition);
    Self->SetWorldRotation(WorldRotation);
    bDuringMotionStateUpdate = false;
}

template <>
SEnumDef const* EnumDefinition<EMotionBehavior>()
{
    static const SEnumDef EnumDef[] = {
        {MB_STATIC, "Static"},
        {MB_SIMULATED, "Simulated"},
        {MB_KINEMATIC, "Kinematic"},
        {0, nullptr}};
    return EnumDef;
}

template <>
SEnumDef const* EnumDefinition<EAINavigationBehavior>()
{
    static const SEnumDef EnumDef[] = {
        {AI_NAVIGATION_BEHAVIOR_NONE, "None"},
        {AI_NAVIGATION_BEHAVIOR_STATIC, "Static"},
        {AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE, "Static Non Walkable"},
        {AI_NAVIGATION_BEHAVIOR_DYNAMIC, "Dynamic"},
        {AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE, "Dynamic Non Walkable"},
        {0, nullptr}};
    return EnumDef;
}

template <>
SEnumDef const* EnumDefinition<COLLISION_MASK>()
{
    static const SEnumDef EnumDef[] = { {CM_NOCOLLISION, "CM_NOCOLLISION"},
                                        {CM_WORLD_STATIC, "CM_WORLD_STATIC"},
                                        {CM_WORLD_DYNAMIC, "CM_WORLD_DYNAMIC"},
                                        {CM_WORLD, "CM_WORLD"},
                                        {CM_PAWN, "CM_PAWN"},
                                        {CM_PROJECTILE, "CM_PROJECTILE"},
                                        {CM_TRIGGER, "CM_TRIGGER"},
                                        {CM_CHARACTER_CONTROLLER, "CM_CHARACTER_CONTROLLER"},
                                        {CM_WATER, "CM_WATER"},
                                        {CM_SOLID, "CM_SOLID"},
                                        {CM_UNUSED7, "CM_UNUSED7"},
                                        {CM_UNUSED8, "CM_UNUSED8" },
                                        {CM_UNUSED9, "CM_UNUSED9"},
                                        {CM_UNUSED10, "CM_UNUSED10"},
                                        {CM_UNUSED11, "CM_UNUSED11"},
                                        {CM_UNUSED12, "CM_UNUSED12"},
                                        {CM_UNUSED13, "CM_UNUSED13"},
                                        {CM_UNUSED14, "CM_UNUSED14"},
                                        {CM_UNUSED15, "CM_UNUSED15"},
                                        {CM_UNUSED16, "CM_UNUSED16"},
                                        {CM_UNUSED17, "CM_UNUSED17"},
                                        {CM_UNUSED18, "CM_UNUSED18"},
                                        {CM_UNUSED19, "CM_UNUSED19"},
                                        {CM_UNUSED20, "CM_UNUSED20"},
                                        {CM_UNUSED21, "CM_UNUSED21"},
                                        {CM_UNUSED22, "CM_UNUSED22"},
                                        {CM_UNUSED23, "CM_UNUSED23"},
                                        {CM_UNUSED24, "CM_UNUSED24"},
                                        {CM_UNUSED25, "CM_UNUSED25"},
                                        {CM_UNUSED26, "CM_UNUSED26"},
                                        {CM_UNUSED27, "CM_UNUSED27"},
                                        {CM_UNUSED28, "CM_UNUSED28"},
                                        {CM_UNUSED29, "CM_UNUSED29"},
                                        {CM_UNUSED30, "CM_UNUSED30"},
                                        {CM_UNUSED31, "CM_UNUSED31"},
                                        {CM_ALL, "CM_ALL"},
                                        {0, nullptr}};
    return EnumDef;
}

HK_BEGIN_CLASS_META(APhysicalBody)
HK_PROPERTY(bDispatchContactEvents, SetDispatchContactEvents, ShouldDispatchContactEvents, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bDispatchOverlapEvents, SetDispatchOverlapEvents, ShouldDispatchOverlapEvents, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bGenerateContactPoints, SetGenerateContactPoints, ShouldGenerateContactPoints, HK_PROPERTY_DEFAULT)
HK_PROPERTY_DIRECT(bUseMeshCollision, HK_PROPERTY_DEFAULT)
HK_PROPERTY(MotionBehavior, SetMotionBehavior, GetMotionBehavior, HK_PROPERTY_DEFAULT)
HK_PROPERTY(AINavigationBehavior, SetAINavigationBehavior, GetAINavigationBehavior, HK_PROPERTY_DEFAULT)
HK_PROPERTY(IsTrigger, SetTrigger, IsTrigger, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bDisableGravity, SetDisableGravity, IsGravityDisabled, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bOverrideWorldGravity, SetOverrideWorldGravity, IsWorldGravityOverriden, HK_PROPERTY_DEFAULT)
HK_PROPERTY(SelfGravity, SetSelfGravity, GetSelfGravity, HK_PROPERTY_DEFAULT)
HK_PROPERTY(Mass, SetMass, GetMass, HK_PROPERTY_DEFAULT)
HK_PROPERTY(CollisionGroup, SetCollisionGroup, GetCollisionGroup, HK_PROPERTY_DEFAULT)
HK_PROPERTY(CollisionMask, SetCollisionMask, GetCollisionMask, HK_PROPERTY_DEFAULT)
HK_PROPERTY(LinearSleepingThreshold, SetLinearSleepingThreshold, GetLinearSleepingThreshold, HK_PROPERTY_DEFAULT)
HK_PROPERTY(LinearDamping, SetLinearDamping, GetLinearDamping, HK_PROPERTY_DEFAULT)
HK_PROPERTY(AngularSleepingThreshold, SetAngularSleepingThreshold, GetAngularSleepingThreshold, HK_PROPERTY_DEFAULT)
HK_PROPERTY(AngularDamping, SetAngularDamping, GetAngularDamping, HK_PROPERTY_DEFAULT)
HK_PROPERTY(Friction, SetFriction, GetFriction, HK_PROPERTY_DEFAULT)
HK_PROPERTY(AnisotropicFriction, SetAnisotropicFriction, GetAnisotropicFriction, HK_PROPERTY_DEFAULT)
HK_PROPERTY(RollingFriction, SetRollingFriction, GetRollingFriction, HK_PROPERTY_DEFAULT)
HK_PROPERTY(Restitution, SetRestitution, GetRestitution, HK_PROPERTY_DEFAULT)
HK_PROPERTY(ContactProcessingThreshold, SetContactProcessingThreshold, GetContactProcessingThreshold, HK_PROPERTY_DEFAULT)
HK_PROPERTY(CcdRadius, SetCcdRadius, GetCcdRadius, HK_PROPERTY_DEFAULT)
HK_PROPERTY(CcdMotionThreshold, SetCcdMotionThreshold, GetCcdMotionThreshold, HK_PROPERTY_DEFAULT)
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
        AAINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;
        NavigationMesh.NavigationPrimitives.Add(this);
    }
}

void APhysicalBody::DeinitializeComponent()
{
    DestroyRigidBody();

    ClearBoneCollisions();

    AAINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;
    NavigationMesh.NavigationPrimitives.Remove(this);

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
        AAINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;

        if (AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE)
        {
            NavigationMesh.NavigationPrimitives.Add(this);
        }
        else
        {
            NavigationMesh.NavigationPrimitives.Remove(this);
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
        LOG("ABoneCollisionInstance::SetWorldTransform for bone\n");
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

    TVector<SBoneCollision> const& boneCollisions = collisionModel->GetBoneCollisions();

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
                    LOG("WARNING: Set transform for non-KINEMATIC body {}\n", GetObjectName());
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
            LOG("WARNING: Set transform for non-KINEMATIC body {}\n", GetObjectName());
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

void APhysicalBody::GatherCollisionGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices) const
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

void APhysicalBody::SetCollisionGroup(COLLISION_MASK _CollisionGroup)
{
    HitProxy->SetCollisionGroup(_CollisionGroup);
}

void APhysicalBody::SetCollisionMask(COLLISION_MASK _CollisionMask)
{
    HitProxy->SetCollisionMask(_CollisionMask);
}

void APhysicalBody::SetCollisionFilter(COLLISION_MASK _CollisionGroup, COLLISION_MASK _CollisionMask)
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

                InRenderer->DrawTriangleSoup(DebugDrawCache->Vertices, DebugDrawCache->Indices, false);
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

                InRenderer->DrawTriangleSoup(DebugDrawCache->Vertices, DebugDrawCache->Indices, false);

                InRenderer->SetColor(Color4(0, 0, 0, 1));
                InRenderer->DrawTriangleSoupWireframe(DebugDrawCache->Vertices, DebugDrawCache->Indices);
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

void APhysicalBody::GatherNavigationGeometry(SNavigationGeometry& Geometry) const
{
    BvAxisAlignedBox worldBounds;

    bool bWalkable = !((AINavigationBehavior == AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE) || (AINavigationBehavior == AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE));

    //        if ( PhysicsBehavior != PB_STATIC ) {
    //            // Generate navmesh only for static geometry
    //            return;
    //        }

    GetCollisionWorldBounds(worldBounds);
    if (worldBounds.IsEmpty())
    {
        LOG("APhysicalBody::GatherNavigationGeometry: the body has no collision\n");
        return;
    }

    TVector<Float3>&       Vertices          = Geometry.Vertices;
    TVector<unsigned int>& Indices           = Geometry.Indices;
    TBitMask<>&                   WalkableTriangles = Geometry.WalkableMask;
    BvAxisAlignedBox&             ResultBoundingBox = Geometry.BoundingBox;
    BvAxisAlignedBox const*       pClipBoundingBox  = Geometry.pClipBoundingBox;

    BvAxisAlignedBox             clippedBounds;
    const Float3                 padding(0.001f);

    if (pClipBoundingBox)
    {
        if (!BvGetBoxIntersection(worldBounds, *pClipBoundingBox, clippedBounds))
        {
            return;
        }
        // Apply a little padding
        clippedBounds.Mins -= padding;
        clippedBounds.Maxs += padding;
        ResultBoundingBox.AddAABB(clippedBounds);
    }
    else
    {
        worldBounds.Mins -= padding;
        worldBounds.Maxs += padding;
        ResultBoundingBox.AddAABB(worldBounds);
    }


    TVector<Float3>       collisionVertices;
    TVector<unsigned int> collisionIndices;

    GatherCollisionGeometry(collisionVertices, collisionIndices);

    if (collisionIndices.IsEmpty())
    {
        // Try to get from mesh
        AMeshComponent const * mesh = Upcast<AMeshComponent>(this);

        if (mesh && !mesh->IsSkinnedMesh())
        {

            AIndexedMesh* indexedMesh = mesh->GetMesh();

            if (!indexedMesh->IsSkinned())
            {
                Float3x4 const& worldTransform = mesh->GetWorldTransformMatrix();

                SMeshVertex const*  srcVertices = indexedMesh->GetVertices();
                unsigned int const* srcIndices  = indexedMesh->GetIndices();

                int firstVertex   = Vertices.Size();
                int firstIndex    = Indices.Size();
                int firstTriangle = Indices.Size() / 3;

                // indexCount may be different from indexedMesh->GetIndexCount()
                int indexCount = 0;
                for (AIndexedMeshSubpart const* subpart : indexedMesh->GetSubparts())
                {
                    indexCount += subpart->GetIndexCount();
                }

                Vertices.Resize(firstVertex + indexedMesh->GetVertexCount());
                Indices.Resize(firstIndex + indexCount);
                WalkableTriangles.Resize(firstTriangle + indexCount / 3);

                Float3*       pVertices = Vertices.ToPtr() + firstVertex;
                unsigned int* pIndices  = Indices.ToPtr() + firstIndex;

                for (int i = 0; i < indexedMesh->GetVertexCount(); i++)
                {
                    pVertices[i] = worldTransform * srcVertices[i].Position;
                }

                if (pClipBoundingBox)
                {
                    // Clip triangles
                    unsigned int i0, i1, i2;
                    int          triangleNum = 0;
                    for (AIndexedMeshSubpart const* subpart : indexedMesh->GetSubparts())
                    {
                        const int numTriangles = subpart->GetIndexCount() / 3;
                        for (int i = 0; i < numTriangles; i++)
                        {
                            i0 = firstVertex + subpart->GetBaseVertex() + srcIndices[subpart->GetFirstIndex() + i * 3 + 0];
                            i1 = firstVertex + subpart->GetBaseVertex() + srcIndices[subpart->GetFirstIndex() + i * 3 + 1];
                            i2 = firstVertex + subpart->GetBaseVertex() + srcIndices[subpart->GetFirstIndex() + i * 3 + 2];

                            if (BvBoxOverlapTriangle_FastApproximation(clippedBounds, Vertices[i0], Vertices[i1], Vertices[i2]))
                            {
                                *pIndices++ = i0;
                                *pIndices++ = i1;
                                *pIndices++ = i2;

                                if (bWalkable)
                                {
                                    WalkableTriangles.Mark(firstTriangle + triangleNum);
                                }
                                triangleNum++;
                            }
                        }
                    }
                    Indices.Resize(firstIndex + triangleNum * 3);
                    WalkableTriangles.Resize(firstTriangle + triangleNum);
                }
                else
                {
                    int triangleNum = 0;
                    for (AIndexedMeshSubpart const* subpart : indexedMesh->GetSubparts())
                    {
                        const int numTriangles = subpart->GetIndexCount() / 3;
                        for (int i = 0; i < numTriangles; i++)
                        {
                            *pIndices++ = firstVertex + subpart->GetBaseVertex() + srcIndices[subpart->GetFirstIndex() + i * 3 + 0];
                            *pIndices++ = firstVertex + subpart->GetBaseVertex() + srcIndices[subpart->GetFirstIndex() + i * 3 + 1];
                            *pIndices++ = firstVertex + subpart->GetBaseVertex() + srcIndices[subpart->GetFirstIndex() + i * 3 + 2];

                            if (bWalkable)
                            {
                                WalkableTriangles.Mark(firstTriangle + triangleNum);
                            }
                            triangleNum++;
                        }
                    }
                }

                //for ( int i = 0 ; i < indexedMesh->GetVertexCount() ; i++ ) {
                //    pVertices[i].Y += 1; //debug
                //}
            }
        }
    }
    else
    {
        Float3 const*       srcVertices = collisionVertices.ToPtr();
        unsigned int const* srcIndices  = collisionIndices.ToPtr();

        int firstVertex   = Vertices.Size();
        int firstIndex    = Indices.Size();
        int firstTriangle = Indices.Size() / 3;
        int vertexCount   = collisionVertices.Size();
        int indexCount    = collisionIndices.Size();

        Vertices.Resize(firstVertex + vertexCount);
        Indices.Resize(firstIndex + indexCount);
        WalkableTriangles.Resize(firstTriangle + indexCount / 3);

        Float3*       pVertices = Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = Indices.ToPtr() + firstIndex;

        Platform::Memcpy(pVertices, srcVertices, vertexCount * sizeof(Float3));

        if (pClipBoundingBox)
        {
            // Clip triangles
            unsigned int i0, i1, i2;
            const int    numTriangles = indexCount / 3;
            int          triangleNum  = 0;
            for (int i = 0; i < numTriangles; i++)
            {
                i0 = firstVertex + srcIndices[i * 3 + 0];
                i1 = firstVertex + srcIndices[i * 3 + 1];
                i2 = firstVertex + srcIndices[i * 3 + 2];

                if (BvBoxOverlapTriangle_FastApproximation(clippedBounds, Vertices[i0], Vertices[i1], Vertices[i2]))
                {
                    *pIndices++ = i0;
                    *pIndices++ = i1;
                    *pIndices++ = i2;

                    if (bWalkable)
                    {
                        WalkableTriangles.Mark(firstTriangle + triangleNum);
                    }
                    triangleNum++;
                }
            }
            Indices.Resize(firstIndex + triangleNum * 3);
            WalkableTriangles.Resize(firstTriangle + triangleNum);
        }
        else
        {
            const int numTriangles = indexCount / 3;
            for (int i = 0; i < numTriangles; i++)
            {
                *pIndices++ = firstVertex + srcIndices[i * 3 + 0];
                *pIndices++ = firstVertex + srcIndices[i * 3 + 1];
                *pIndices++ = firstVertex + srcIndices[i * 3 + 2];

                if (bWalkable)
                {
                    WalkableTriangles.Mark(firstTriangle + i);
                }
            }
        }

        //for ( int i = 0 ; i < vertexCount ; i++ ) {
        //    pVertices[i].Y += 1; //debug
        //}
    }
}
