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

// EXPERIMENTAL!!!


#include "SoftMeshComponent.h"
#include "World.h"
#include <Engine/Runtime/Engine.h>

#include <Engine/Core/ConsoleVar.h>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
//#include <BulletSoftBody/btSoftBodyHelpers.h>

#include <Engine/Runtime/BulletCompatibility.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawSoftmeshFaces("com_DrawSoftmeshFaces"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(SoftMeshComponent)

SoftMeshComponent::SoftMeshComponent()
{
    m_bCanEverTick = true;

    m_bSoftBodySimulation = true;
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

    if (m_SoftBody)
    {
        btSoftRigidDynamicsWorld* physicsWorld = static_cast<btSoftRigidDynamicsWorld*>(GetWorld()->PhysicsSystem.GetInternal());
        physicsWorld->removeSoftBody(m_SoftBody);
        delete m_SoftBody;
        m_SoftBody = nullptr;
    }
}

void SoftMeshComponent::RecreateSoftBody()
{
    IndexedMesh* sourceMesh = GetMesh();

    if (!sourceMesh)
    {
        return;
    }

    TVector<SoftbodyLink> const& softbodyLinks = sourceMesh->GetSoftbodyLinks();
    TVector<SoftbodyFace> const& softbodyFaces = sourceMesh->GetSoftbodyFaces();

    if (softbodyFaces.IsEmpty() || softbodyLinks.IsEmpty())
    {
        // TODO: Warning?
        return;
    }

    btSoftRigidDynamicsWorld* physicsWorld = static_cast<btSoftRigidDynamicsWorld*>(GetWorld()->PhysicsSystem.GetInternal());

    if (m_SoftBody)
    {
        physicsWorld->removeSoftBody(m_SoftBody);
        delete m_SoftBody;
        m_SoftBody = nullptr;
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

    m_SoftBody = new btSoftBody(GetWorld()->PhysicsSystem.GetSoftBodyWorldInfo(), vtx.size(), &vtx[0], 0);
    for (SoftbodyLink const& link : softbodyLinks)
    {
        m_SoftBody->appendLink(link.Indices[0], link.Indices[1]);
    }
    for (SoftbodyFace const& face : softbodyFaces)
    {
        m_SoftBody->appendFace(face.Indices[0], face.Indices[1], face.Indices[2]);
    }
    btSoftBody::Material* pm = m_SoftBody->appendMaterial();
    pm->m_kLST               = LinearStiffness;
    pm->m_kAST               = AngularStiffness;
    pm->m_kVST               = VolumeStiffness;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;
    m_SoftBody->generateBendingConstraints(2, pm);
    m_SoftBody->m_cfg.piterations = 10;
    m_SoftBody->m_cfg.viterations = 2;
    m_SoftBody->m_cfg.kVC = VelocitiesCorrection;
    m_SoftBody->m_cfg.kDP = DampingCoefficient;
    m_SoftBody->m_cfg.kDG = DragCoefficient;
    m_SoftBody->m_cfg.kLF = LiftCoefficient;
    m_SoftBody->m_cfg.kPR = Pressure;
    m_SoftBody->m_cfg.kVC = VolumeConversation;
    m_SoftBody->m_cfg.kDF = DynamicFriction;
    m_SoftBody->m_cfg.kMT = PoseMatching;
    m_SoftBody->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;

    //m_SoftBody->m_cfg.aeromodel = btSoftBody::eAeroModel::F_TwoSided;
    //if ( bRandomizeConstraints ) {
    //    m_SoftBody->randomizeConstraints();
    //}

    bool bFromFaces = false;
    if (GetMass() > 0.01f)
    {
        m_SoftBody->setTotalMass(GetMass(), bFromFaces);
    }
    else
    {
        m_SoftBody->setTotalMass(0.01f, bFromFaces);
    }

    m_bUpdateAnchors = true;

    //UpdateAnchorPoints();

    if (bRandomizeConstraints)
    {
        m_SoftBody->randomizeConstraints();
    }

    physicsWorld->addSoftBody(m_SoftBody);

    //m_SoftBody->setVelocity( btVector3( 0, 0, 0 ) );

    //bUpdateSoftbodyTransform = true;
}

void SoftMeshComponent::UpdateMesh()
{
    Super::UpdateMesh();

    if (!GetWorld())
    {
        // Component not initialized yet
        return;
    }

    RecreateSoftBody();
}

Float3 SoftMeshComponent::GetVertexPosition(int _VertexIndex) const
{
    if (m_SoftBody)
    {
        if (_VertexIndex >= 0 && _VertexIndex < m_SoftBody->m_nodes.size())
        {
            return btVectorToFloat3(m_SoftBody->m_nodes[_VertexIndex].m_x);
        }
    }
    return Float3::Zero();
}

Float3 SoftMeshComponent::GetVertexNormal(int _VertexIndex) const
{
    if (m_SoftBody)
    {
        if (_VertexIndex >= 0 && _VertexIndex < m_SoftBody->m_nodes.size())
        {
            return btVectorToFloat3(m_SoftBody->m_nodes[_VertexIndex].m_n);
        }
    }
    return Float3::Zero();
}

Float3 SoftMeshComponent::GetVertexVelocity(int _VertexIndex) const
{
    if (m_SoftBody)
    {
        if (_VertexIndex >= 0 && _VertexIndex < m_SoftBody->m_nodes.size())
        {
            return btVectorToFloat3(m_SoftBody->m_nodes[_VertexIndex].m_v);
        }
    }
    return Float3::Zero();
}

void SoftMeshComponent::SetWindVelocity(Float3 const& _Velocity)
{
    //if ( m_SoftBody ) {
    //    m_SoftBody->setWindVelocity( btVectorToFloat3( _Velocity ) );
    //}

    m_WindVelocity = _Velocity;
}

Float3 const& SoftMeshComponent::GetWindVelocity() const
{
    return m_WindVelocity;
}

void SoftMeshComponent::AddForceSoftBody(Float3 const& _Force)
{
    if (m_SoftBody)
    {
        m_SoftBody->addForce(btVectorToFloat3(_Force));
    }
}

void SoftMeshComponent::AddForceToVertex(Float3 const& _Force, int _VertexIndex)
{
    if (m_SoftBody && _VertexIndex >= 0 && _VertexIndex < m_SoftBody->m_nodes.size())
    {
        m_SoftBody->addForce(btVectorToFloat3(_Force), _VertexIndex);
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
    //        if ( m_SoftBody ) {
    //            m_SoftBody->transform( transform * prevTransform.inverse() );
    //        }
    //#else
    //        //if ( m_SoftBody ) {
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
    if (m_SoftBody)
    {
        btVector3 mins, maxs;

        //btTransform prevTransform;
        //prevTransform.setOrigin( btVectorToFloat3( PrevTransformOrigin ) );
        //prevTransform.setBasis( btMatrixToFloat3x3( PrevTransformBasis ) );
        //prevTransform = prevTransform.inverse();

        //btTransform t;
        //t.setIdentity();

        //m_SoftBody->getAabb( mins, maxs );
        //m_SoftBody->getCollisionShape()->getAabb( t, mins, maxs );
        //m_SoftBody->getCollisionShape()->getAabb( prevTransform, mins, maxs );

        m_SoftBody->getAabb(mins, maxs);

        ForceOverrideBounds(true);
        SetBoundsOverride(BvAxisAlignedBox(btVectorToFloat3(mins), btVectorToFloat3(maxs)));
    }
}

void SoftMeshComponent::UpdateAnchorPoints()
{
    if (!m_SoftBody)
    {
        return;
    }

    if (m_bUpdateAnchors)
    {

        auto* physicsWorld = GetWorld()->PhysicsSystem.GetInternal();

        // Remove old anchors. FIXME: is it correct?
        m_SoftBody->m_collisionDisabledObjects.clear();
        m_SoftBody->m_anchors.clear();

        // Add new anchors
        for (AnchorBinding& binding : m_Anchors)
        {

            if (binding.VertexIndex < 0 || binding.VertexIndex >= m_SoftBody->m_nodes.size())
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

            //m_SoftBody->appendAnchor( binding.VertexIndex, binding.Anchor->Anchor );
            m_SoftBody->appendAnchor(binding.VertexIndex, binding.Anchor->Anchor,
                                   btVector3(0, 0, 0), false, 1);

            m_SoftBody->setMass(binding.VertexIndex, 1);
        }

        //m_SoftBody->setVelocity( btVector3( 0, 0, 0 ) );

        m_bUpdateAnchors = false;
    }
}

void SoftMeshComponent::TickComponent(float _TimeStep)
{
    Super::TickComponent(_TimeStep);

    UpdateAnchorPoints();

    // TODO: This must be done in pre physics tick!!!
    if (m_SoftBody)
    {
        //m_SoftBody->addVelocity( btVectorToFloat3( m_WindVelocity*_TimeStep*(Math::Rand()*0.5f+0.5f) ) );

        btVector3 vel = btVectorToFloat3(m_WindVelocity * _TimeStep);

        MersenneTwisterRand& rng = GEngine->Rand; // TODO: Use random from game session

        for (int i = 0, ni = m_SoftBody->m_nodes.size(); i < ni; ++i) m_SoftBody->addVelocity(vel * (rng.GetFloat() * 0.5f + 0.5f), i);
    }

    // TODO: This must be done in post physics tick!!!
    UpdateSoftbodyTransform();
    UpdateSoftbodyBoundingBox();

    m_bUpdateAbsoluteTransforms = true;
}

void SoftMeshComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (!m_SoftBody)
    {
        return;
    }

    // Draw AABB
    //btVector3 mins, maxs;
    //m_SoftBody->getCollisionShape()->getAabb( m_SoftBody->getWorldTransform(), mins, maxs );
    //InRenderer->SetDepthTest( true );
    //InRenderer->SetColor( 1, 1, 0, 1 );
    //InRenderer->DrawAABB( BvAxisAlignedBox( btVectorToFloat3( mins ), btVectorToFloat3( maxs ) ) );

    // Draw faces
    if (com_DrawSoftmeshFaces)
    {
        InRenderer->SetDepthTest(true);
        InRenderer->SetColor(Color4(1, 0, 0, 1));
        for (int i = 0; i < m_SoftBody->m_faces.size(); i++)
        {

            btSoftBody::Face& f = m_SoftBody->m_faces[i];

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
