/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#if 0
#include "PhysicalBody.h"
#include "World.h"

#include <Engine/Runtime/DebugRenderer.h>
#include <Engine/Runtime/BulletCompatibility.h>

#include <Engine/Core/Application/Logger.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

#define PHYS_COMPARE_EPSILON 0.0001f
#define MIN_MASS             0.001f
#define MAX_MASS             1000.0f

ConsoleVar com_DrawCollisionModel("com_DrawCollisionModel"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCollisionShapes("com_DrawCollisionShapes"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawTriggers("com_DrawTriggers"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawBoneCollisionShapes("com_DrawBoneCollisionShapes"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawStaticCollisionBounds("com_DrawStaticCollisionBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawSimulatedCollisionBounds("com_DrawSimulatedCollisionBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawKinematicCollisionBounds("com_DrawKinematicCollisionBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawBoneCollisionBounds("com_DrawBoneCollisionBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawTriggerBounds("com_DrawTriggerBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawCenterOfMass("com_DrawCenterOfMass"s, "0"s, CVAR_CHEAT);

static constexpr bool bUseInternalEdgeUtility = true;

class MotionState : public btMotionState
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Alloc(_SizeInBytes, 16);
    }

    void operator delete(void* _Ptr)
    {
        Core::GetHeapAllocator<HEAP_PHYSICS>().Free(_Ptr);
    }
};

class PhysicalBodyMotionState : public MotionState
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Alloc(_SizeInBytes, 16);
    }

    void operator delete(void* _Ptr)
    {
        Core::GetHeapAllocator<HEAP_PHYSICS>().Free(_Ptr);
    }

    PhysicalBodyMotionState() :
        WorldPosition(Float3(0)), WorldRotation(Quat::Identity()), CenterOfMass(Float3(0))
    {
    }

    // Overrides

    void getWorldTransform(btTransform& _CenterOfMassTransform) const override;
    void setWorldTransform(btTransform const& _CenterOfMassTransform) override;

    // Public members

    PhysicalBody* Self;
    mutable Float3 WorldPosition;
    mutable Quat WorldRotation;
    Float3 CenterOfMass;
    bool bDuringMotionStateUpdate = false;
};

void PhysicalBodyMotionState::getWorldTransform(btTransform& _CenterOfMassTransform) const
{
    WorldPosition = Self->GetWorldPosition();
    WorldRotation = Self->GetWorldRotation();

    _CenterOfMassTransform.setRotation(btQuaternionToQuat(WorldRotation));
    _CenterOfMassTransform.setOrigin(btVectorToFloat3(WorldPosition) + _CenterOfMassTransform.getBasis() * btVectorToFloat3(CenterOfMass));
}

void PhysicalBodyMotionState::setWorldTransform(btTransform const& _CenterOfMassTransform)
{
    if (Self->m_MotionBehavior != MB_SIMULATED)
    {
        LOG("PhysicalBodyMotionState::SetWorldTransform for non-simulated {}\n", Self->GetObjectName());
        return;
    }

    bDuringMotionStateUpdate = true;
    WorldRotation = btQuaternionToQuat(_CenterOfMassTransform.getRotation());
    WorldPosition = btVectorToFloat3(_CenterOfMassTransform.getOrigin() - _CenterOfMassTransform.getBasis() * btVectorToFloat3(CenterOfMass));
    Self->SetWorldPosition(WorldPosition);
    Self->SetWorldRotation(WorldRotation);
    bDuringMotionStateUpdate = false;
}

template <>
EnumDef const* EnumDefinition<MOTION_BEHAVIOR>()
{
    static const EnumDef EnumDef[] = {
        {MB_STATIC, "Static"},
        {MB_SIMULATED, "Simulated"},
        {MB_KINEMATIC, "Kinematic"},
        {0, nullptr}};
    return EnumDef;
}

template <>
EnumDef const* EnumDefinition<AI_NAVIGATION_BEHAVIOR>()
{
    static const EnumDef EnumDef[] = {
        {AI_NAVIGATION_BEHAVIOR_NONE, "None"},
        {AI_NAVIGATION_BEHAVIOR_STATIC, "Static"},
        {AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE, "Static Non Walkable"},
        {AI_NAVIGATION_BEHAVIOR_DYNAMIC, "Dynamic"},
        {AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE, "Dynamic Non Walkable"},
        {0, nullptr}};
    return EnumDef;
}

template <>
EnumDef const* EnumDefinition<COLLISION_MASK>()
{
    static const EnumDef EnumDef[] = {{CM_NOCOLLISION, "CM_NOCOLLISION"},
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
                                      {CM_UNUSED8, "CM_UNUSED8"},
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

HK_BEGIN_CLASS_META(PhysicalBody)
HK_PROPERTY(bDispatchContactEvents, SetDispatchContactEvents, ShouldDispatchContactEvents, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bDispatchOverlapEvents, SetDispatchOverlapEvents, ShouldDispatchOverlapEvents, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bGenerateContactPoints, SetGenerateContactPoints, ShouldGenerateContactPoints, HK_PROPERTY_DEFAULT)
HK_PROPERTY(bUseMeshCollision, SetUseMeshCollision, ShouldUseMeshCollision, HK_PROPERTY_DEFAULT)
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

PhysicalBody::PhysicalBody()
{
    m_HitProxy = NewObj<HitProxy>();
}

bool PhysicalBody::ShouldHaveCollisionBody() const
{
    if (m_bSoftBodySimulation)
    {
        return false;
    }

    if (!m_HitProxy->GetCollisionGroup())
    {
        return false;
    }

    if (IsInEditor())
    {
        return false;
    }

    CollisionModel const* collisionModel = GetCollisionModel();
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

void PhysicalBody::InitializeComponent()
{
    Super::InitializeComponent();

    if (ShouldHaveCollisionBody())
    {
        CreateRigidBody();
    }

    CreateBoneCollisions();

    if (m_AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE)
    {
        AINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;
        NavigationMesh.NavigationPrimitives.Add(this);
    }
}

void PhysicalBody::DeinitializeComponent()
{
    DestroyRigidBody();

    ClearBoneCollisions();

    AINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;
    NavigationMesh.NavigationPrimitives.Remove(this);

    Super::DeinitializeComponent();
}

void PhysicalBody::SetMotionBehavior(MOTION_BEHAVIOR _MotionBehavior)
{
    if (m_MotionBehavior == _MotionBehavior)
    {
        return;
    }

    m_MotionBehavior = _MotionBehavior;

    UpdatePhysicsAttribs();
}

void PhysicalBody::SetAINavigationBehavior(AI_NAVIGATION_BEHAVIOR _AINavigationBehavior)
{
    if (m_AINavigationBehavior == _AINavigationBehavior)
    {
        return;
    }

    m_AINavigationBehavior = _AINavigationBehavior;

    if (IsInitialized())
    {
        AINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;

        if (m_AINavigationBehavior != AI_NAVIGATION_BEHAVIOR_NONE)
        {
            NavigationMesh.NavigationPrimitives.Add(this);
        }
        else
        {
            NavigationMesh.NavigationPrimitives.Remove(this);
        }
    }
}

class BoneCollisionInstance : public MotionState
{
public:
    void getWorldTransform(btTransform& _CenterOfMassTransform) const override
    {
        Float3x4 jointTransform = Self->GetWorldTransformMatrix() * Self->_GetJointTransform(pObject->GetJointIndex());

        Float3 position = jointTransform.DecomposeTranslation();
        Quat rotation;

        rotation.FromMatrix(jointTransform.DecomposeRotation());

        Float3 localPosition = Self->m_CachedScale * OffsetPosition;

        _CenterOfMassTransform.setRotation(btQuaternionToQuat(rotation * OffsetRotation));
        _CenterOfMassTransform.setOrigin(btVectorToFloat3(position) + _CenterOfMassTransform.getBasis() * btVectorToFloat3(localPosition));
    }

    void setWorldTransform(btTransform const& _CenterOfMassTransform) override
    {
        LOG("BoneCollisionInstance::SetWorldTransform for bone\n");
    }

    PhysicalBody* Self;

    TRef<HitProxy> pObject;
    Float3 OffsetPosition;
    Quat OffsetRotation;
};

void PhysicalBody::ClearBoneCollisions()
{
    for (int i = 0; i < m_BoneCollisionInst.Size(); i++)
    {
        BoneCollisionInstance* boneCollision = m_BoneCollisionInst[i];

        btCollisionObject* colObject = boneCollision->pObject->GetCollisionObject();
        btCollisionShape* shape = colObject->getCollisionShape();

        boneCollision->pObject->Deinitialize();

        delete colObject;
        delete shape;
        delete boneCollision;
    }

    m_BoneCollisionInst.Clear();
}

void PhysicalBody::UpdateBoneCollisions()
{
    if (!IsInitialized())
    {
        return;
    }

    CreateBoneCollisions();
}

void PhysicalBody::CreateBoneCollisions()
{
    ClearBoneCollisions();

    CollisionModel const* collisionModel = GetCollisionModel();

    if (!collisionModel)
    {
        return;
    }

    btRigidBody::btRigidBodyConstructionInfo constructInfo(0.0f, nullptr, nullptr);

    TVector<BoneCollision> const& boneCollisions = collisionModel->GetBoneCollisions();

    m_BoneCollisionInst.Resize(boneCollisions.Size());
    for (int i = 0; i < boneCollisions.Size(); i++)
    {
        BoneCollisionInstance* boneCollision = new BoneCollisionInstance;
        TUniqueRef<CollisionBody> const& collisionBody = boneCollisions[i].Body;

        m_BoneCollisionInst[i] = boneCollision;

        boneCollision->Self = this;
        boneCollision->OffsetPosition = collisionBody->Position;
        boneCollision->OffsetRotation = collisionBody->Rotation;
        boneCollision->pObject = NewObj<HitProxy>();
        boneCollision->pObject->SetCollisionMask(boneCollisions[i].CollisionMask);
        boneCollision->pObject->SetCollisionGroup(boneCollisions[i].CollisionGroup);
        boneCollision->pObject->SetJointIndex(boneCollisions[i].JointIndex);

        btCollisionShape* shape = collisionBody->Create(m_CachedScale);
        shape->setMargin(collisionBody->Margin);

        constructInfo.m_motionState = boneCollision;
        constructInfo.m_collisionShape = shape;

        int collisionFlags = btCollisionObject::CF_KINEMATIC_OBJECT;

        btRigidBody* rigidBody = new btRigidBody(constructInfo);
        rigidBody->setCollisionFlags(collisionFlags);
        rigidBody->forceActivationState(DISABLE_DEACTIVATION);
        rigidBody->setUserPointer(boneCollision->pObject.GetObject());

        boneCollision->pObject->Initialize(this, rigidBody);
    }
}

void PhysicalBody::SetCollisionModel(CollisionModel* collisionModel)
{
    if (m_CollisionModel == collisionModel)
    {
        return;
    }

    m_CollisionModel = collisionModel;

    UpdatePhysicsAttribs();
    UpdateBoneCollisions();
}

CollisionModel* PhysicalBody::GetCollisionModel() const
{
    return m_bUseMeshCollision ? GetMeshCollisionModel() : m_CollisionModel;
}

void PhysicalBody::SetUseMeshCollision(bool _bUseMeshCollision)
{
    if (m_bUseMeshCollision == _bUseMeshCollision)
    {
        return;
    }

    m_bUseMeshCollision = _bUseMeshCollision;

    UpdatePhysicsAttribs();
    UpdateBoneCollisions();
}

void PhysicalBody::SetCollisionFlags()
{
    int collisionFlags = m_RigidBody->getCollisionFlags();

    if (m_HitProxy->IsTrigger())
    {
        collisionFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
    }
    if (m_MotionBehavior == MB_KINEMATIC)
    {
        collisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    if (m_MotionBehavior == MB_STATIC)
    {
        collisionFlags |= btCollisionObject::CF_STATIC_OBJECT;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_STATIC_OBJECT;
    }
    if (bUseInternalEdgeUtility && m_CollisionInstance->GetCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
    {
        collisionFlags |= btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    }
    else
    {
        collisionFlags &= ~btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
    }

    m_RigidBody->setCollisionFlags(collisionFlags);
    m_RigidBody->forceActivationState(m_MotionBehavior == MB_KINEMATIC ? DISABLE_DEACTIVATION : ISLAND_SLEEPING);
}

void PhysicalBody::SetRigidBodyGravity()
{
    Float3 const& worldGravity = GetWorld()->GetGravityVector();

    int flags = m_RigidBody->getFlags();

    if (m_bDisableGravity || m_bOverrideWorldGravity)
    {
        flags |= BT_DISABLE_WORLD_GRAVITY;
    }
    else
    {
        flags &= ~BT_DISABLE_WORLD_GRAVITY;
    }

    m_RigidBody->setFlags(flags);

    if (m_bDisableGravity)
    {
        m_RigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
    }
    else if (m_bOverrideWorldGravity)
    {
        // Use self gravity
        m_RigidBody->setGravity(btVectorToFloat3(m_SelfGravity));
    }
    else
    {
        // Use world gravity
        m_RigidBody->setGravity(btVectorToFloat3(worldGravity));
    }
}

void PhysicalBody::CreateRigidBody()
{
    HK_ASSERT(m_MotionState == nullptr);
    HK_ASSERT(m_RigidBody == nullptr);
    HK_ASSERT(m_CollisionInstance == nullptr);

    m_CachedScale = GetWorldScale();

    m_MotionState = new PhysicalBodyMotionState;
    m_MotionState->Self = this;

    m_CollisionInstance = GetCollisionModel()->Instantiate(m_CachedScale);
    m_MotionState->CenterOfMass = m_CollisionInstance->GetCenterOfMass();

    Float3 localInertia(0.0f, 0.0f, 0.0f);
    float mass = 0;

    if (m_MotionBehavior == MB_SIMULATED)
    {
        mass = Math::Clamp(m_Mass, MIN_MASS, MAX_MASS);
        localInertia = m_CollisionInstance->CalculateLocalInertia(mass);
    }

    btRigidBody::btRigidBodyConstructionInfo constructInfo(mass, m_MotionState, m_CollisionInstance->GetCollisionShape(), btVectorToFloat3(localInertia));
    constructInfo.m_linearDamping = m_LinearDamping;
    constructInfo.m_angularDamping = m_AngularDamping;
    constructInfo.m_friction = m_Friction;
    constructInfo.m_rollingFriction = m_RollingFriction;
    //    constructInfo.m_spinningFriction;
    constructInfo.m_restitution = m_Restitution;
    constructInfo.m_linearSleepingThreshold = m_LinearSleepingThreshold;
    constructInfo.m_angularSleepingThreshold = m_AngularSleepingThreshold;

    m_RigidBody = new btRigidBody(constructInfo);
    m_RigidBody->setUserPointer(m_HitProxy.GetObject());

    SetCollisionFlags();
    SetRigidBodyGravity();

    m_HitProxy->Initialize(this, m_RigidBody);

    ActivatePhysics();

    // Update dynamic attributes
    SetLinearFactor(m_LinearFactor);
    SetAngularFactor(m_AngularFactor);
    SetAnisotropicFriction(m_AnisotropicFriction);
    SetContactProcessingThreshold(m_ContactProcessingThreshold);
    SetCcdRadius(m_CcdRadius);
    SetCcdMotionThreshold(m_CcdMotionThreshold);

    UpdateDebugDrawCache();
}

void PhysicalBody::DestroyRigidBody()
{
    if (!m_RigidBody)
    {
        // Rigid body wasn't created
        return;
    }

    m_HitProxy->Deinitialize();

    delete m_RigidBody;
    m_RigidBody = nullptr;

    m_CollisionInstance.Reset();

    delete m_MotionState;
    m_MotionState = nullptr;

    UpdateDebugDrawCache();
}

void PhysicalBody::UpdatePhysicsAttribs()
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

    if (!m_RigidBody)
    {
        CreateRigidBody();
        return;
    }

    btTransform const& centerOfMassTransform = m_RigidBody->getWorldTransform();
    Float3 position = btVectorToFloat3(centerOfMassTransform.getOrigin() - centerOfMassTransform.getBasis() * btVectorToFloat3(m_MotionState->CenterOfMass));

    m_CachedScale = GetWorldScale();

    m_CollisionInstance = GetCollisionModel()->Instantiate(m_CachedScale);
    m_MotionState->CenterOfMass = m_CollisionInstance->GetCenterOfMass();

    float mass = Math::Clamp(m_Mass, MIN_MASS, MAX_MASS);
    if (m_MotionBehavior == MB_SIMULATED)
    {
        Float3 localInertia = m_CollisionInstance->CalculateLocalInertia(mass);

        m_RigidBody->setMassProps(mass, btVectorToFloat3(localInertia));
    }
    else
    {
        m_RigidBody->setMassProps(0.0f, btVector3(0, 0, 0));
    }

    // Intertia tensor is based on transform orientation and mass props, so we need to update it too
    m_RigidBody->updateInertiaTensor();

    m_RigidBody->setCollisionShape(m_CollisionInstance->GetCollisionShape());

    SetCollisionFlags();

    // Update position with new center of mass
    SetCenterOfMassPosition(position);

    m_HitProxy->UpdateBroadphase(); // FIXME: is it need?

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

void PhysicalBody::OnTransformDirty()
{
    Super::OnTransformDirty();

    if (m_RigidBody)
    {
        if (!m_MotionState->bDuringMotionStateUpdate)
        {
            if (m_MotionBehavior != MB_KINEMATIC)
            {
                Float3 position = GetWorldPosition();
                Quat rotation = GetWorldRotation();

                if (rotation != m_MotionState->WorldRotation)
                {
                    m_MotionState->WorldRotation = rotation;
                    SetCenterOfMassRotation(rotation);
                }
                if (position != m_MotionState->WorldPosition)
                {
                    m_MotionState->WorldPosition = position;
                    SetCenterOfMassPosition(position);
                }

                if (!IsInEditor())
                {
                    LOG("WARNING: Set transform for non-KINEMATIC body {}\n", GetObjectName());
                }
            }
        }

        int numBodies = m_CollisionInstance->GetCollisionBodiesCount();

        if ((numBodies > 0 /*|| !BoneCollisionInst.IsEmpty()*/) && !m_CachedScale.CompareEps(GetWorldScale(), PHYS_COMPARE_EPSILON))
        {
            UpdatePhysicsAttribs();

            //UpdateBoneCollisions();
        }

        UpdateDebugDrawCache();
    }
    else
    {
        if (m_MotionBehavior != MB_KINEMATIC && !GetOwnerActor()->IsSpawning() && !IsInEditor())
        {
            LOG("WARNING: Set transform for non-KINEMATIC body {}\n", GetObjectName());
        }
    }

    //if ( m_SoftBody && !bUpdateSoftbodyTransform ) {
    //    if ( !PrevWorldPosition.CompareEps( GetWorldPosition(), PHYS_COMPARE_EbPSILON )
    //        || !PrevWorldRotation.CompareEps( GetWorldRotation(), PHYS_COMPARE_EPSILON ) ) {
    //        bUpdateSoftbodyTransform = true;

    //        // TODO: add to dirty list?
    //    }
    //}
}

void PhysicalBody::SetCenterOfMassPosition(Float3 const& _Position)
{
    HK_ASSERT(m_RigidBody);

    btTransform& centerOfMassTransform = m_RigidBody->getWorldTransform();
    centerOfMassTransform.setOrigin(btVectorToFloat3(_Position) + centerOfMassTransform.getBasis() * btVectorToFloat3(m_MotionState->CenterOfMass));

    if (GetWorld()->IsDuringPhysicsUpdate())
    {
        btTransform interpolationWorldTransform = m_RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setOrigin(centerOfMassTransform.getOrigin());
        m_RigidBody->setInterpolationWorldTransform(interpolationWorldTransform);
    }

    ActivatePhysics();
}

void PhysicalBody::SetCenterOfMassRotation(Quat const& _Rotation)
{
    HK_ASSERT(m_RigidBody);

    btTransform& centerOfMassTransform = m_RigidBody->getWorldTransform();

    btVector3 bodyPrevPosition = centerOfMassTransform.getOrigin() - centerOfMassTransform.getBasis() * btVectorToFloat3(m_MotionState->CenterOfMass);

    centerOfMassTransform.setRotation(btQuaternionToQuat(_Rotation));

    if (!m_MotionState->CenterOfMass.CompareEps(Float3::Zero(), PHYS_COMPARE_EPSILON))
    {
        centerOfMassTransform.setOrigin(bodyPrevPosition + centerOfMassTransform.getBasis() * btVectorToFloat3(m_MotionState->CenterOfMass));
    }

    if (GetWorld()->IsDuringPhysicsUpdate())
    {
        btTransform interpolationWorldTransform = m_RigidBody->getInterpolationWorldTransform();
        interpolationWorldTransform.setBasis(centerOfMassTransform.getBasis());
        if (!m_MotionState->CenterOfMass.CompareEps(Float3::Zero(), PHYS_COMPARE_EPSILON))
        {
            interpolationWorldTransform.setOrigin(centerOfMassTransform.getOrigin());
        }
        m_RigidBody->setInterpolationWorldTransform(interpolationWorldTransform);
    }

    // Intertia tensor is based on transform orientation and mass props, so we need to update it too
    m_RigidBody->updateInertiaTensor();

    ActivatePhysics();
}

void PhysicalBody::SetLinearVelocity(Float3 const& _Velocity)
{
    if (m_RigidBody)
    {
        m_RigidBody->setLinearVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }

    if (m_SoftBody)
    {
        m_SoftBody->setVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void PhysicalBody::AddLinearVelocity(Float3 const& _Velocity)
{
    if (m_RigidBody)
    {
        m_RigidBody->setLinearVelocity(m_RigidBody->getLinearVelocity() + btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }

    if (m_SoftBody)
    {
        m_SoftBody->addVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void PhysicalBody::SetLinearFactor(Float3 const& _Factor)
{
    if (m_RigidBody)
    {
        m_RigidBody->setLinearFactor(btVectorToFloat3(_Factor));
    }

    m_LinearFactor = _Factor;
}

void PhysicalBody::SetLinearSleepingThreshold(float _Threshold)
{
    if (m_RigidBody)
    {
        m_RigidBody->setSleepingThresholds(_Threshold, m_AngularSleepingThreshold);
    }

    m_LinearSleepingThreshold = _Threshold;
}

void PhysicalBody::SetLinearDamping(float _Damping)
{
    if (m_RigidBody)
    {
        m_RigidBody->setDamping(_Damping, m_AngularDamping);
    }

    m_LinearDamping = _Damping;
}

void PhysicalBody::SetAngularVelocity(Float3 const& _Velocity)
{
    if (m_RigidBody)
    {
        m_RigidBody->setAngularVelocity(btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void PhysicalBody::AddAngularVelocity(Float3 const& _Velocity)
{
    if (m_RigidBody)
    {
        m_RigidBody->setAngularVelocity(m_RigidBody->getAngularVelocity() + btVectorToFloat3(_Velocity));
        if (_Velocity != Float3::Zero())
        {
            ActivatePhysics();
        }
    }
}

void PhysicalBody::SetAngularFactor(Float3 const& _Factor)
{
    if (m_RigidBody)
    {
        m_RigidBody->setAngularFactor(btVectorToFloat3(_Factor));
    }

    m_AngularFactor = _Factor;
}

void PhysicalBody::SetAngularSleepingThreshold(float _Threshold)
{
    if (m_RigidBody)
    {
        m_RigidBody->setSleepingThresholds(m_LinearSleepingThreshold, _Threshold);
    }

    m_AngularSleepingThreshold = _Threshold;
}

void PhysicalBody::SetAngularDamping(float _Damping)
{
    if (m_RigidBody)
    {
        m_RigidBody->setDamping(m_LinearDamping, _Damping);
    }

    m_AngularDamping = _Damping;
}

void PhysicalBody::SetFriction(float _Friction)
{
    if (m_RigidBody)
    {
        m_RigidBody->setFriction(_Friction);
    }

    if (m_SoftBody)
    {
        m_SoftBody->setFriction(_Friction);
    }

    m_Friction = _Friction;
}

void PhysicalBody::SetAnisotropicFriction(Float3 const& _Friction)
{
    if (m_RigidBody)
    {
        m_RigidBody->setAnisotropicFriction(btVectorToFloat3(_Friction));
    }

    if (m_SoftBody)
    {
        m_SoftBody->setAnisotropicFriction(btVectorToFloat3(_Friction));
    }

    m_AnisotropicFriction = _Friction;
}

void PhysicalBody::SetRollingFriction(float _Friction)
{
    if (m_RigidBody)
    {
        m_RigidBody->setRollingFriction(_Friction);
    }

    if (m_SoftBody)
    {
        m_SoftBody->setRollingFriction(_Friction);
    }

    m_RollingFriction = _Friction;
}

void PhysicalBody::SetRestitution(float _Restitution)
{
    if (m_RigidBody)
    {
        m_RigidBody->setRestitution(_Restitution);
    }

    if (m_SoftBody)
    {
        m_SoftBody->setRestitution(_Restitution);
    }

    m_Restitution = _Restitution;
}

void PhysicalBody::SetContactProcessingThreshold(float _Threshold)
{
    if (m_RigidBody)
    {
        m_RigidBody->setContactProcessingThreshold(_Threshold);
    }

    if (m_SoftBody)
    {
        m_SoftBody->setContactProcessingThreshold(_Threshold);
    }

    m_ContactProcessingThreshold = _Threshold;
}

void PhysicalBody::SetCcdRadius(float _Radius)
{
    m_CcdRadius = Math::Max(_Radius, 0.0f);

    if (m_RigidBody)
    {
        m_RigidBody->setCcdSweptSphereRadius(m_CcdRadius);
    }

    if (m_SoftBody)
    {
        m_SoftBody->setCcdSweptSphereRadius(m_CcdRadius);
    }
}

void PhysicalBody::SetCcdMotionThreshold(float _Threshold)
{
    m_CcdMotionThreshold = Math::Max(_Threshold, 0.0f);

    if (m_RigidBody)
    {
        m_RigidBody->setCcdMotionThreshold(m_CcdMotionThreshold);
    }

    if (m_SoftBody)
    {
        m_SoftBody->setCcdMotionThreshold(m_CcdMotionThreshold);
    }
}

Float3 PhysicalBody::GetLinearVelocity() const
{
    return m_RigidBody ? btVectorToFloat3(m_RigidBody->getLinearVelocity()) : Float3::Zero();
}

Float3 const& PhysicalBody::GetLinearFactor() const
{
    return m_LinearFactor;
}

Float3 PhysicalBody::GetVelocityAtPoint(Float3 const& _Position) const
{
    return m_RigidBody ? btVectorToFloat3(m_RigidBody->getVelocityInLocalPoint(btVectorToFloat3(_Position - m_MotionState->CenterOfMass))) : Float3::Zero();
}

float PhysicalBody::GetLinearSleepingThreshold() const
{
    return m_LinearSleepingThreshold;
}

float PhysicalBody::GetLinearDamping() const
{
    return m_LinearDamping;
}

Float3 PhysicalBody::GetAngularVelocity() const
{
    return m_RigidBody ? btVectorToFloat3(m_RigidBody->getAngularVelocity()) : Float3::Zero();
}

Float3 const& PhysicalBody::GetAngularFactor() const
{
    return m_AngularFactor;
}

float PhysicalBody::GetAngularSleepingThreshold() const
{
    return m_AngularSleepingThreshold;
}

float PhysicalBody::GetAngularDamping() const
{
    return m_AngularDamping;
}

float PhysicalBody::GetFriction() const
{
    return m_Friction;
}

Float3 const& PhysicalBody::GetAnisotropicFriction() const
{
    return m_AnisotropicFriction;
}

float PhysicalBody::GetRollingFriction() const
{
    return m_RollingFriction;
}

float PhysicalBody::GetRestitution() const
{
    return m_Restitution;
}

float PhysicalBody::GetContactProcessingThreshold() const
{
    return m_ContactProcessingThreshold;
}

float PhysicalBody::GetCcdRadius() const
{
    return m_CcdRadius;
}

float PhysicalBody::GetCcdMotionThreshold() const
{
    return m_CcdMotionThreshold;
}

Float3 const& PhysicalBody::GetCenterOfMass() const
{
    return m_MotionState ? m_MotionState->CenterOfMass : Float3::Zero();
}

Float3 PhysicalBody::GetCenterOfMassWorldPosition() const
{
    return m_RigidBody ? btVectorToFloat3(m_RigidBody->getWorldTransform().getOrigin()) : GetWorldPosition();
}

void PhysicalBody::ActivatePhysics()
{
    if (m_MotionBehavior == MB_SIMULATED)
    {
        if (m_RigidBody)
        {
            m_RigidBody->activate(true);
        }
    }

    if (m_SoftBody)
    {
        m_SoftBody->activate(true);
    }
}

bool PhysicalBody::IsPhysicsActive() const
{
    if (m_RigidBody)
    {
        return m_RigidBody->isActive();
    }

    if (m_SoftBody)
    {
        return m_SoftBody->isActive();
    }

    return false;
}

void PhysicalBody::ClearForces()
{
    if (m_RigidBody)
    {
        m_RigidBody->clearForces();
    }
}

void PhysicalBody::ApplyCentralForce(Float3 const& _Force)
{
    if (m_RigidBody && _Force != Float3::Zero())
    {
        ActivatePhysics();
        m_RigidBody->applyCentralForce(btVectorToFloat3(_Force));
    }
}

void PhysicalBody::ApplyForce(Float3 const& _Force, Float3 const& _Position)
{
    if (m_RigidBody && _Force != Float3::Zero())
    {
        ActivatePhysics();
        m_RigidBody->applyForce(btVectorToFloat3(_Force), btVectorToFloat3(_Position - m_MotionState->CenterOfMass));
    }
}

void PhysicalBody::ApplyTorque(Float3 const& _Torque)
{
    if (m_RigidBody && _Torque != Float3::Zero())
    {
        ActivatePhysics();
        m_RigidBody->applyTorque(btVectorToFloat3(_Torque));
    }
}

void PhysicalBody::ApplyCentralImpulse(Float3 const& _Impulse)
{
    if (m_RigidBody && _Impulse != Float3::Zero())
    {
        ActivatePhysics();
        m_RigidBody->applyCentralImpulse(btVectorToFloat3(_Impulse));
    }
}

void PhysicalBody::ApplyImpulse(Float3 const& _Impulse, Float3 const& _Position)
{
    if (m_RigidBody && _Impulse != Float3::Zero())
    {
        ActivatePhysics();
        m_RigidBody->applyImpulse(btVectorToFloat3(_Impulse), btVectorToFloat3(_Position - m_MotionState->CenterOfMass));
    }
}

void PhysicalBody::ApplyTorqueImpulse(Float3 const& _Torque)
{
    if (m_RigidBody && _Torque != Float3::Zero())
    {
        ActivatePhysics();
        m_RigidBody->applyTorqueImpulse(btVectorToFloat3(_Torque));
    }
}

void PhysicalBody::GetCollisionBodiesWorldBounds(TVector<BvAxisAlignedBox>& _BoundingBoxes) const
{
    if (!m_CollisionInstance)
    {
        _BoundingBoxes.Clear();
        return;
    }

    m_CollisionInstance->GetCollisionBodiesWorldBounds(GetWorldPosition(), GetWorldRotation(), _BoundingBoxes);
}

void PhysicalBody::GetCollisionWorldBounds(BvAxisAlignedBox& _BoundingBox) const
{
    if (!m_CollisionInstance)
    {
        _BoundingBox.Clear();
        return;
    }

    m_CollisionInstance->GetCollisionWorldBounds(GetWorldPosition(), GetWorldRotation(), _BoundingBox);
}

void PhysicalBody::GetCollisionBodyWorldBounds(int index, BvAxisAlignedBox& _BoundingBox) const
{
    if (!m_CollisionInstance)
    {
        _BoundingBox.Clear();
        return;
    }

    m_CollisionInstance->GetCollisionBodyWorldBounds(index, GetWorldPosition(), GetWorldRotation(), _BoundingBox);
}

void PhysicalBody::GetCollisionBodyLocalBounds(int index, BvAxisAlignedBox& _BoundingBox) const
{
    if (!m_CollisionInstance)
    {
        _BoundingBox.Clear();
        return;
    }

    m_CollisionInstance->GetCollisionBodyLocalBounds(index, _BoundingBox);
}

float PhysicalBody::GetCollisionBodyMargin(int index) const
{
    if (!m_CollisionInstance)
    {
        return 0;
    }

    return m_CollisionInstance->GetCollisionBodyMargin(index);
}

int PhysicalBody::GetCollisionBodiesCount() const
{
    if (!m_CollisionInstance)
    {
        return 0;
    }

    return m_CollisionInstance->GetCollisionBodiesCount();
}

void PhysicalBody::GatherCollisionGeometry(TVector<Float3>& _Vertices, TVector<unsigned int>& _Indices) const
{
    CollisionModel const* collisionModel = GetCollisionModel();
    if (!collisionModel)
    {
        return;
    }

    collisionModel->GatherGeometry(_Vertices, _Indices, GetWorldTransformMatrix());
}

void PhysicalBody::SetTrigger(bool _Trigger)
{
    if (m_HitProxy->IsTrigger() == _Trigger)
    {
        return;
    }

    m_HitProxy->SetTrigger(_Trigger);

    UpdatePhysicsAttribs();
}

void PhysicalBody::SetDisableGravity(bool _DisableGravity)
{
    if (m_bDisableGravity == _DisableGravity)
    {
        return;
    }

    m_bDisableGravity = _DisableGravity;

    UpdatePhysicsAttribs();
}

void PhysicalBody::SetOverrideWorldGravity(bool _OverrideWorldGravity)
{
    if (m_bOverrideWorldGravity == _OverrideWorldGravity)
    {
        return;
    }

    m_bOverrideWorldGravity = _OverrideWorldGravity;

    UpdatePhysicsAttribs();
}

void PhysicalBody::SetSelfGravity(Float3 const& _SelfGravity)
{
    if (m_SelfGravity == _SelfGravity)
    {
        return;
    }

    m_SelfGravity = _SelfGravity;

    UpdatePhysicsAttribs();
}

void PhysicalBody::SetMass(float _Mass)
{
    if (m_Mass == _Mass)
    {
        return;
    }

    m_Mass = _Mass;

    UpdatePhysicsAttribs();
}

void PhysicalBody::SetCollisionGroup(COLLISION_MASK _CollisionGroup)
{
    m_HitProxy->SetCollisionGroup(_CollisionGroup);
}

void PhysicalBody::SetCollisionMask(COLLISION_MASK _CollisionMask)
{
    m_HitProxy->SetCollisionMask(_CollisionMask);
}

void PhysicalBody::SetCollisionFilter(COLLISION_MASK _CollisionGroup, COLLISION_MASK _CollisionMask)
{
    m_HitProxy->SetCollisionFilter(_CollisionGroup, _CollisionMask);
}

void PhysicalBody::AddCollisionIgnoreActor(Actor* _Actor)
{
    m_HitProxy->AddCollisionIgnoreActor(_Actor);
}

void PhysicalBody::RemoveCollisionIgnoreActor(Actor* _Actor)
{
    m_HitProxy->RemoveCollisionIgnoreActor(_Actor);
}

void PhysicalBody::CollisionContactQuery(TVector<HitProxy*>& _Result) const
{
    m_HitProxy->CollisionContactQuery(_Result);
}

void PhysicalBody::CollisionContactQueryActor(TVector<Actor*>& _Result) const
{
    m_HitProxy->CollisionContactQueryActor(_Result);
}

void PhysicalBody::UpdateDebugDrawCache()
{
    if (m_DebugDrawCache)
    {
        m_DebugDrawCache->bDirty = true;
    }
}

void PhysicalBody::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if ((com_DrawCollisionModel || com_DrawTriggers) && m_RigidBody)
    {
        if (!m_DebugDrawCache)
        {
            m_DebugDrawCache = MakeUnique<DebugDrawCache>();
            m_DebugDrawCache->bDirty = true;
        }

        if (m_DebugDrawCache->bDirty)
        {
            m_DebugDrawCache->Vertices.Clear();
            m_DebugDrawCache->Indices.Clear();
            m_DebugDrawCache->bDirty = false;
            GatherCollisionGeometry(m_DebugDrawCache->Vertices, m_DebugDrawCache->Indices);
        }

        InRenderer->SetDepthTest(false);

        if (m_HitProxy->IsTrigger())
        {
            if (com_DrawTriggers)
            {
                InRenderer->SetColor(Color4(0, 1, 0, 0.5f));

                InRenderer->DrawTriangleSoup(m_DebugDrawCache->Vertices, m_DebugDrawCache->Indices, false);
            }
        }
        else
        {
            if (com_DrawCollisionModel)
            {
                switch (m_MotionBehavior)
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

                InRenderer->DrawTriangleSoup(m_DebugDrawCache->Vertices, m_DebugDrawCache->Indices, false);

                InRenderer->SetColor(Color4(0, 0, 0, 1));
                InRenderer->DrawTriangleSoupWireframe(m_DebugDrawCache->Vertices, m_DebugDrawCache->Indices);
            }
        }
    }

    if (m_HitProxy->IsTrigger() && com_DrawTriggerBounds)
    {
        TVector<BvAxisAlignedBox> boundingBoxes;

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
        if (m_MotionBehavior == MB_STATIC && com_DrawStaticCollisionBounds)
        {
            TVector<BvAxisAlignedBox> boundingBoxes;

            GetCollisionBodiesWorldBounds(boundingBoxes);

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
            for (BvAxisAlignedBox const& bb : boundingBoxes)
            {
                InRenderer->DrawAABB(bb);
            }
        }

        if (m_MotionBehavior == MB_SIMULATED && com_DrawSimulatedCollisionBounds)
        {
            TVector<BvAxisAlignedBox> boundingBoxes;

            GetCollisionBodiesWorldBounds(boundingBoxes);

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(IsPhysicsActive() ? Color4(0.1f, 1.0f, 0.1f, 1) : Color4(0.3f, 0.3f, 0.3f, 1));
            for (BvAxisAlignedBox const& bb : boundingBoxes)
            {
                InRenderer->DrawAABB(bb);
            }
        }

        if (m_MotionBehavior == MB_KINEMATIC && com_DrawKinematicCollisionBounds)
        {
            TVector<BvAxisAlignedBox> boundingBoxes;

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
        for (BoneCollisionInstance* boneCollision : m_BoneCollisionInst)
        {
            btCollisionObject* colObject = boneCollision->pObject->GetCollisionObject();
            btCollisionShape* shape = colObject->getCollisionShape();

            shape->getAabb(colObject->getWorldTransform(), mins, maxs);

            InRenderer->DrawAABB(BvAxisAlignedBox(btVectorToFloat3(mins), btVectorToFloat3(maxs)));
        }
    }

    if (com_DrawBoneCollisionShapes)
    {
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(1.0f, 1.0f, 0.0f, 1));
        for (BoneCollisionInstance* boneCollision : m_BoneCollisionInst)
        {
            btCollisionObject* colObject = boneCollision->pObject->GetCollisionObject();

            btDrawCollisionShape(InRenderer, colObject->getWorldTransform(), colObject->getCollisionShape());
        }
    }

    if (com_DrawCenterOfMass)
    {
        if (m_RigidBody)
        {
            Float3 centerOfMass = GetCenterOfMassWorldPosition();

            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 0, 0, 1));
            InRenderer->DrawBox(centerOfMass, Float3(0.02f));
        }
    }

    if (com_DrawCollisionShapes)
    {
        if (m_RigidBody)
        {
            InRenderer->SetDepthTest(false);
            btDrawCollisionObject(InRenderer, m_RigidBody);
        }
    }
}

void PhysicalBody::GatherNavigationGeometry(NavigationGeometry& Geometry) const
{
    #if 0 // todo
    BvAxisAlignedBox worldBounds;

    bool bWalkable = !((m_AINavigationBehavior == AI_NAVIGATION_BEHAVIOR_STATIC_NON_WALKABLE) || (m_AINavigationBehavior == AI_NAVIGATION_BEHAVIOR_DYNAMIC_NON_WALKABLE));

    //        if ( PhysicsBehavior != PB_STATIC ) {
    //            // Generate navmesh only for static geometry
    //            return;
    //        }

    GetCollisionWorldBounds(worldBounds);
    if (worldBounds.IsEmpty())
    {
        LOG("PhysicalBody::GatherNavigationGeometry: the body has no collision\n");
        return;
    }

    TVector<Float3>& Vertices = Geometry.Vertices;
    TVector<unsigned int>& Indices = Geometry.Indices;
    TBitMask<>& WalkableTriangles = Geometry.WalkableMask;
    BvAxisAlignedBox& ResultBoundingBox = Geometry.BoundingBox;
    BvAxisAlignedBox const* pClipBoundingBox = Geometry.pClipBoundingBox;

    BvAxisAlignedBox clippedBounds;
    const Float3 padding(0.001f);

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


    TVector<Float3> collisionVertices;
    TVector<unsigned int> collisionIndices;

    GatherCollisionGeometry(collisionVertices, collisionIndices);

    if (collisionIndices.IsEmpty())
    {
        // Try to get from mesh
        MeshComponent const* mesh = Upcast<MeshComponent>(this);

        if (mesh && !mesh->IsSkinnedMesh())
        {
            IndexedMesh* indexedMesh = mesh->GetMesh();

            if (!indexedMesh->IsSkinned())
            {
                Float3x4 const& worldTransform = mesh->GetWorldTransformMatrix();

                MeshVertex const* srcVertices = indexedMesh->GetVertices();
                unsigned int const* srcIndices = indexedMesh->GetIndices();

                int firstVertex = Vertices.Size();
                int firstIndex = Indices.Size();
                int firstTriangle = Indices.Size() / 3;

                // indexCount may be different from indexedMesh->GetIndexCount()
                int indexCount = 0;
                for (IndexedMeshSubpart const* subpart : indexedMesh->GetSubparts())
                {
                    indexCount += subpart->GetIndexCount();
                }

                Vertices.Resize(firstVertex + indexedMesh->GetVertexCount());
                Indices.Resize(firstIndex + indexCount);
                WalkableTriangles.Resize(firstTriangle + indexCount / 3);

                Float3* pVertices = Vertices.ToPtr() + firstVertex;
                unsigned int* pIndices = Indices.ToPtr() + firstIndex;

                for (int i = 0; i < indexedMesh->GetVertexCount(); i++)
                {
                    pVertices[i] = worldTransform * srcVertices[i].Position;
                }

                if (pClipBoundingBox)
                {
                    // Clip triangles
                    unsigned int i0, i1, i2;
                    int triangleNum = 0;
                    for (IndexedMeshSubpart const* subpart : indexedMesh->GetSubparts())
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
                    for (IndexedMeshSubpart const* subpart : indexedMesh->GetSubparts())
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
        Float3 const* srcVertices = collisionVertices.ToPtr();
        unsigned int const* srcIndices = collisionIndices.ToPtr();

        int firstVertex = Vertices.Size();
        int firstIndex = Indices.Size();
        int firstTriangle = Indices.Size() / 3;
        int vertexCount = collisionVertices.Size();
        int indexCount = collisionIndices.Size();

        Vertices.Resize(firstVertex + vertexCount);
        Indices.Resize(firstIndex + indexCount);
        WalkableTriangles.Resize(firstTriangle + indexCount / 3);

        Float3* pVertices = Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices = Indices.ToPtr() + firstIndex;

        Core::Memcpy(pVertices, srcVertices, vertexCount * sizeof(Float3));

        if (pClipBoundingBox)
        {
            // Clip triangles
            unsigned int i0, i1, i2;
            const int numTriangles = indexCount / 3;
            int triangleNum = 0;
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

    #endif
}

HK_NAMESPACE_END
#endif
