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

// EXPERIMENTAL!!!


#include "SoftMeshComponent.h"
#include "World.h"
#include "Engine.h"

#include <Core/ConsoleVar.h>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
//#include <BulletSoftBody/btSoftBodyHelpers.h>

#include "BulletCompatibility.h"

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSoftmeshFaces("com_DrawSoftmeshFaces"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(SoftMeshComponent)

SoftMeshComponent::SoftMeshComponent()
{
    bSoftBodySimulation = true;
    bCanEverTick        = true;

    m_bJointsSimulatedByPhysics = true;

    //PrevTransformBasis.SetIdentity();
}

SoftMeshComponent::~SoftMeshComponent()
{
    DetachAllVertices();
}

void SoftMeshComponent::InitializeComponent()
{
    Super::InitializeComponent();

    RecreateSoftBody();
}

void SoftMeshComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    DetachAllVertices();

    if (SoftBody)
    {
        btSoftRigidDynamicsWorld* physicsWorld = static_cast<btSoftRigidDynamicsWorld*>(GetWorld()->PhysicsSystem.GetInternal());
        physicsWorld->removeSoftBody(SoftBody);
        delete SoftBody;
        SoftBody = nullptr;
    }
}

void SoftMeshComponent::RecreateSoftBody()
{
    IndexedMesh* sourceMesh = GetMesh();

    if (!sourceMesh)
    {
        return;
    }

    TPodVector<SoftbodyLink> const& softbodyLinks = sourceMesh->GetSoftbodyLinks();
    TPodVector<SoftbodyFace> const& softbodyFaces = sourceMesh->GetSoftbodyFaces();

    if (softbodyFaces.IsEmpty() || softbodyLinks.IsEmpty())
    {
        // TODO: Warning?
        return;
    }

    btSoftRigidDynamicsWorld* physicsWorld = static_cast<btSoftRigidDynamicsWorld*>(GetWorld()->PhysicsSystem.GetInternal());

    if (SoftBody)
    {
        physicsWorld->removeSoftBody(SoftBody);
        delete SoftBody;
        SoftBody = nullptr;
        //PrevTransformBasis.SetIdentity();
        //PrevTransformOrigin.Clear();
    }

    constexpr bool bRandomizeConstraints = true;

    //MeshVertex const * vertices = sourceMesh->GetVertices();
    //unsigned int const * indices = sourceMesh->GetIndices();

    btAlignedObjectArray<btVector3> vtx;

    MeshSkin const& skin = sourceMesh->GetSkin();

    vtx.resize(skin.JointIndices.Size());

    for (int i = 0; i < skin.JointIndices.Size(); i++)
    {
        vtx[i] = btVectorToFloat3(skin.OffsetMatrices[i].DecomposeTranslation());
    }

    SoftBody = new btSoftBody(GetWorld()->PhysicsSystem.GetSoftBodyWorldInfo(), vtx.size(), &vtx[0], 0);
    for (SoftbodyLink const& link : softbodyLinks)
    {
        SoftBody->appendLink(link.Indices[0], link.Indices[1]);
    }
    for (SoftbodyFace const& face : softbodyFaces)
    {
        SoftBody->appendFace(face.Indices[0], face.Indices[1], face.Indices[2]);
    }
    btSoftBody::Material* pm = SoftBody->appendMaterial();
    pm->m_kLST               = LinearStiffness;
    pm->m_kAST               = AngularStiffness;
    pm->m_kVST               = VolumeStiffness;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;
    SoftBody->generateBendingConstraints(2, pm);
    SoftBody->m_cfg.piterations = 10;
    SoftBody->m_cfg.viterations = 2;
    SoftBody->m_cfg.kVC         = VelocitiesCorrection;
    SoftBody->m_cfg.kDP         = DampingCoefficient;
    SoftBody->m_cfg.kDG         = DragCoefficient;
    SoftBody->m_cfg.kLF         = LiftCoefficient;
    SoftBody->m_cfg.kPR         = Pressure;
    SoftBody->m_cfg.kVC         = VolumeConversation;
    SoftBody->m_cfg.kDF         = DynamicFriction;
    SoftBody->m_cfg.kMT         = PoseMatching;
    SoftBody->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;

    //SoftBody->m_cfg.aeromodel = btSoftBody::eAeroModel::F_TwoSided;
    //if ( bRandomizeConstraints ) {
    //    SoftBody->randomizeConstraints();
    //}

    bool bFromFaces = false;
    if (GetMass() > 0.01f)
    {
        SoftBody->setTotalMass(GetMass(), bFromFaces);
    }
    else
    {
        SoftBody->setTotalMass(0.01f, bFromFaces);
    }

    m_bUpdateAnchors = true;

    //UpdateAnchorPoints();

    if (bRandomizeConstraints)
    {
        SoftBody->randomizeConstraints();
    }

    physicsWorld->addSoftBody(SoftBody);

    //SoftBody->setVelocity( btVector3( 0, 0, 0 ) );

    //bUpdateSoftbodyTransform = true;
}

void SoftMeshComponent::OnMeshChanged()
{
    if (!GetWorld())
    {
        // Component not initialized yet
        return;
    }

    RecreateSoftBody();
}

Float3 SoftMeshComponent::GetVertexPosition(int _VertexIndex) const
{
    if (SoftBody)
    {
        if (_VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size())
        {
            return btVectorToFloat3(SoftBody->m_nodes[_VertexIndex].m_x);
        }
    }
    return Float3::Zero();
}

Float3 SoftMeshComponent::GetVertexNormal(int _VertexIndex) const
{
    if (SoftBody)
    {
        if (_VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size())
        {
            return btVectorToFloat3(SoftBody->m_nodes[_VertexIndex].m_n);
        }
    }
    return Float3::Zero();
}

Float3 SoftMeshComponent::GetVertexVelocity(int _VertexIndex) const
{
    if (SoftBody)
    {
        if (_VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size())
        {
            return btVectorToFloat3(SoftBody->m_nodes[_VertexIndex].m_v);
        }
    }
    return Float3::Zero();
}

void SoftMeshComponent::SetWindVelocity(Float3 const& _Velocity)
{
    //if ( SoftBody ) {
    //    SoftBody->setWindVelocity( btVectorToFloat3( _Velocity ) );
    //}

    WindVelocity = _Velocity;
}

Float3 const& SoftMeshComponent::GetWindVelocity() const
{
    return WindVelocity;
}

void SoftMeshComponent::AddForceSoftBody(Float3 const& _Force)
{
    if (SoftBody)
    {
        SoftBody->addForce(btVectorToFloat3(_Force));
    }
}

void SoftMeshComponent::AddForceToVertex(Float3 const& _Force, int _VertexIndex)
{
    if (SoftBody && _VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size())
    {
        SoftBody->addForce(btVectorToFloat3(_Force), _VertexIndex);
    }
}

void SoftMeshComponent::UpdateSoftbodyTransform()
{
    //    if ( bUpdateSoftbodyTransform ) {
    //        btTransform transform;
    //
    //        Float3 worldPosition = GetWorldPosition();
    //        Quat worldRotation = GetWorldRotation();
    //
    //        transform.setOrigin( btVectorToFloat3( worldPosition ) );
    //        transform.setRotation( btQuaternionToQuat( worldRotation ) );
    //
    //        PrevWorldPosition = worldPosition;
    //        PrevWorldRotation = worldRotation;
    //#if 0
    //        btTransform prevTransform;
    //        prevTransform.setOrigin( btVectorToFloat3( PrevTransformOrigin ) );
    //        prevTransform.setBasis( btMatrixToFloat3x3( PrevTransformBasis ) );
    //        if ( SoftBody ) {
    //            SoftBody->transform( transform * prevTransform.inverse() );
    //        }
    //#else
    //        //if ( SoftBody ) {
    //        //    Anchor->setWorldTransform( transform );
    //        //}
    //#endif
    //
    //        PrevTransformOrigin = btVectorToFloat3( transform.getOrigin() );
    //        PrevTransformBasis = btMatrixToFloat3x3( transform.getBasis() );
    //
    //        bUpdateSoftbodyTransform = false;
    //    }
}

void SoftMeshComponent::UpdateSoftbodyBoundingBox()
{
    if (SoftBody)
    {
        btVector3 mins, maxs;

        //btTransform prevTransform;
        //prevTransform.setOrigin( btVectorToFloat3( PrevTransformOrigin ) );
        //prevTransform.setBasis( btMatrixToFloat3x3( PrevTransformBasis ) );
        //prevTransform = prevTransform.inverse();

        //btTransform t;
        //t.setIdentity();

        //SoftBody->getAabb( mins, maxs );
        //SoftBody->getCollisionShape()->getAabb( t, mins, maxs );
        //SoftBody->getCollisionShape()->getAabb( prevTransform, mins, maxs );

        SoftBody->getAabb(mins, maxs);

        ForceOverrideBounds(true);
        SetBoundsOverride(BvAxisAlignedBox(btVectorToFloat3(mins), btVectorToFloat3(maxs)));
    }
}

void SoftMeshComponent::UpdateAnchorPoints()
{
    if (!SoftBody)
    {
        return;
    }

    if (m_bUpdateAnchors)
    {

        auto* physicsWorld = GetWorld()->PhysicsSystem.GetInternal();

        // Remove old anchors. FIXME: is it correct?
        SoftBody->m_collisionDisabledObjects.clear();
        SoftBody->m_anchors.clear();

        // Add new anchors
        for (AnchorBinding& binding : m_Anchors)
        {

            if (binding.VertexIndex < 0 || binding.VertexIndex >= SoftBody->m_nodes.size())
            {
                continue;
            }

            btRigidBody* anchorBody = binding.Anchor->Anchor;

            if (!anchorBody)
            {

                // create rigid body

                btSphereShape* shape = new btSphereShape(0.5f);
                anchorBody           = new btRigidBody(0.0f, NULL, shape);

                physicsWorld->addRigidBody(anchorBody, 0, 0);

                int collisionFlags = anchorBody->getCollisionFlags();
                collisionFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
                collisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
                anchorBody->setCollisionFlags(collisionFlags);

                anchorBody->forceActivationState(DISABLE_DEACTIVATION);

                btTransform transform;

                Float3 worldPosition = binding.Anchor->GetWorldPosition();
                Quat   worldRotation = binding.Anchor->GetWorldRotation();

                transform.setOrigin(btVectorToFloat3(worldPosition));
                transform.setRotation(btQuaternionToQuat(worldRotation));

                anchorBody->setWorldTransform(transform);

                binding.Anchor->Anchor = anchorBody;
            }

            //SoftBody->appendAnchor( binding.VertexIndex, binding.Anchor->Anchor );
            SoftBody->appendAnchor(binding.VertexIndex, binding.Anchor->Anchor,
                                   btVector3(0, 0, 0), false, 1);

            SoftBody->setMass(binding.VertexIndex, 1);
        }

        //SoftBody->setVelocity( btVector3( 0, 0, 0 ) );

        m_bUpdateAnchors = false;
    }
}

void SoftMeshComponent::TickComponent(float _TimeStep)
{
    Super::TickComponent(_TimeStep);

    UpdateAnchorPoints();

    // TODO: This must be done in pre physics tick!!!
    if (SoftBody)
    {
        //SoftBody->addVelocity( btVectorToFloat3( WindVelocity*_TimeStep*(Math::Rand()*0.5f+0.5f) ) );

        btVector3 vel = btVectorToFloat3(WindVelocity * _TimeStep);

        MersenneTwisterRand& rng = GEngine->Rand;

        for (int i = 0, ni = SoftBody->m_nodes.size(); i < ni; ++i) SoftBody->addVelocity(vel * (rng.GetFloat() * 0.5f + 0.5f), i);
    }

    // TODO: This must be done in post physics tick!!!
    UpdateSoftbodyTransform();
    UpdateSoftbodyBoundingBox();

    m_bUpdateAbsoluteTransforms = true;
}

void SoftMeshComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (!SoftBody)
    {
        return;
    }

    // Draw AABB
    //btVector3 mins, maxs;
    //SoftBody->getCollisionShape()->getAabb( SoftBody->getWorldTransform(), mins, maxs );
    //InRenderer->SetDepthTest( true );
    //InRenderer->SetColor( 1, 1, 0, 1 );
    //InRenderer->DrawAABB( BvAxisAlignedBox( btVectorToFloat3( mins ), btVectorToFloat3( maxs ) ) );

    // Draw faces
    if (com_DrawSoftmeshFaces)
    {
        InRenderer->SetDepthTest(true);
        InRenderer->SetColor(Color4(1, 0, 0, 1));
        for (int i = 0; i < SoftBody->m_faces.size(); i++)
        {

            btSoftBody::Face& f = SoftBody->m_faces[i];

            Color4 color;
            color[0] = f.m_normal[0] * 0.5f + 0.5f;
            color[1] = f.m_normal[1] * 0.5f + 0.5f;
            color[2] = f.m_normal[2] * 0.5f + 0.5f;
            color[3] = 1.0f;

            InRenderer->SetColor(color);

            InRenderer->DrawTriangle(
                btVectorToFloat3(f.m_n[0]->m_x),
                btVectorToFloat3(f.m_n[1]->m_x),
                btVectorToFloat3(f.m_n[2]->m_x), true);
        }
    }
}

void SoftMeshComponent::AttachVertex(int _VertexIndex, AnchorComponent* _Anchor)
{
    AnchorBinding* binding = nullptr;

    for (int i = 0; i < m_Anchors.Size(); i++)
    {
        if (m_Anchors[i].VertexIndex == _VertexIndex)
        {
            binding = &m_Anchors[i];
            break;
        }
    }

    if (binding)
    {
        binding->Anchor->RemoveRef();
        binding->Anchor->AttachCount--;
    }
    else
    {
        binding = &m_Anchors.Add();
    }

    binding->VertexIndex = _VertexIndex;
    binding->Anchor      = _Anchor;
    _Anchor->AttachCount++;

    _Anchor->AddRef();

    m_bUpdateAnchors = true;
}

void SoftMeshComponent::DetachVertex(int _VertexIndex)
{
    for (int i = 0; i < m_Anchors.Size(); i++)
    {
        if (m_Anchors[i].VertexIndex == _VertexIndex)
        {
            m_Anchors[i].Anchor->RemoveRef();
            m_Anchors[i].Anchor->AttachCount--;
            m_Anchors.Remove(i);
            break;
        }
    }

    m_bUpdateAnchors = true;
}

void SoftMeshComponent::DetachAllVertices()
{
    for (int i = 0; i < m_Anchors.Size(); i++)
    {
        m_Anchors[i].Anchor->RemoveRef();
        m_Anchors[i].Anchor->AttachCount--;
    }
    m_Anchors.Clear();
    m_bUpdateAnchors = true;
}

AnchorComponent* SoftMeshComponent::GetVertexAnchor(int _VertexIndex) const
{
    for (int i = 0; i < m_Anchors.Size(); i++)
    {
        if (m_Anchors[i].VertexIndex == _VertexIndex)
        {
            return m_Anchors[i].Anchor;
        }
    }
    return nullptr;
}

HK_NAMESPACE_END
