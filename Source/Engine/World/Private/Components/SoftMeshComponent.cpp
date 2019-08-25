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

// EXPEREMENTAL!!!


#include <Engine/World/Public/Components/SoftMeshComponent.h>
#include <Engine/World/Public/World.h>

#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
//#include <BulletSoftBody/btSoftBodyHelpers.h>

#include <Engine/BulletCompatibility/BulletCompatibility.h>

AN_CLASS_META( FSoftMeshComponent )

FSoftMeshComponent::FSoftMeshComponent() {
    bSoftBodySimulation = true;
    bCanEverTick = true;

    bJointsSimulatedByPhysics = true;

    //PrevTransformBasis.SetIdentity();
}

void FSoftMeshComponent::InitializeComponent() {
    Super::InitializeComponent();

    RecreateSoftBody();
}

void FSoftMeshComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    DetachAllVertices();

    if ( SoftBody ) {
        btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;
        physicsWorld->removeSoftBody( SoftBody );
        b3Destroy( SoftBody );
        SoftBody = nullptr;
    }
}

void FSoftMeshComponent::RecreateSoftBody() {

    FIndexedMesh * sourceMesh = GetMesh();
    FSkeleton * skel = GetSkeleton();

    if ( !sourceMesh || !skel || sourceMesh->SoftbodyFaces.IsEmpty() || sourceMesh->SoftbodyLinks.IsEmpty() ) {
        // TODO: Warning?
        return;
    }

    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    if ( SoftBody ) {
        physicsWorld->removeSoftBody( SoftBody );
        b3Destroy( SoftBody );
        SoftBody = nullptr;
        //PrevTransformBasis.SetIdentity();
        //PrevTransformOrigin.Clear();
    }

    constexpr bool bRandomizeConstraints = true;

    //FMeshVertex const * vertices = sourceMesh->GetVertices();
    //unsigned int const * indices = sourceMesh->GetIndices();

    btAlignedObjectArray< btVector3 > vtx;
    vtx.resize( skel->GetJoints().Size() );
    FJoint const * joints = skel->GetJoints().ToPtr();
    for ( int i = 0; i < skel->GetJoints().Size(); i++ ) {
        vtx[ i ] = btVectorToFloat3( joints[ i ].OffsetMatrix.DecomposeTranslation() );
    }

    SoftBody = b3New( btSoftBody, GetWorld()->SoftBodyWorldInfo, vtx.size(), &vtx[ 0 ], 0 );
    for ( FSoftbodyLink & link : sourceMesh->SoftbodyLinks ) {
        SoftBody->appendLink( link.Indices[0], link.Indices[1] );
    }
    for ( FSoftbodyFace & face : sourceMesh->SoftbodyFaces ) {
        SoftBody->appendFace( face.Indices[0], face.Indices[1], face.Indices[2] );
    }
    btSoftBody::Material * pm = SoftBody->appendMaterial();
    pm->m_kLST = LinearStiffness;
    pm->m_kAST = AngularStiffness;
    pm->m_kVST = VolumeStiffness;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;
    SoftBody->generateBendingConstraints( 2, pm );
    SoftBody->m_cfg.piterations = 10;
    SoftBody->m_cfg.viterations = 2;
    SoftBody->m_cfg.kVC = VelocitiesCorrection;
    SoftBody->m_cfg.kDP = DampingCoefficient;
    SoftBody->m_cfg.kDG = DragCoefficient;
    SoftBody->m_cfg.kLF = LiftCoefficient;
    SoftBody->m_cfg.kPR = Pressure;
    SoftBody->m_cfg.kVC = VolumeConversation;
    SoftBody->m_cfg.kDF = DynamicFriction;
    SoftBody->m_cfg.kMT = PoseMatching;
    SoftBody->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;

    //SoftBody->m_cfg.aeromodel = btSoftBody::eAeroModel::F_TwoSided;
    //if ( bRandomizeConstraints ) {
    //    SoftBody->randomizeConstraints();
    //}

    bool bFromFaces = false;
    if ( Mass > 0.01f ) {
        SoftBody->setTotalMass( Mass, bFromFaces );
    } else {
        SoftBody->setTotalMass( 0.01f, bFromFaces );
    }
    
    bUpdateAnchors = true;

    //UpdateAnchorPoints();

    if ( bRandomizeConstraints ) {
        SoftBody->randomizeConstraints();
    }

    physicsWorld->addSoftBody( SoftBody );

    //SoftBody->setVelocity( btVector3( 0, 0, 0 ) );
    
    //bUpdateSoftbodyTransform = true;
}

void FSoftMeshComponent::OnMeshChanged() {
    if ( !GetWorld() ) {
        // Component not initialized yet
        return;
    }

    RecreateSoftBody();
}

Float3 FSoftMeshComponent::GetVertexPosition( int _VertexIndex ) const {
    if ( SoftBody ) {
        if ( _VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size() ) {
            return btVectorToFloat3( SoftBody->m_nodes[ _VertexIndex ].m_x );
        }
    }
    return Float3::Zero();
}

Float3 FSoftMeshComponent::GetVertexNormal( int _VertexIndex ) const {
    if ( SoftBody ) {
        if ( _VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size() ) {
            return btVectorToFloat3( SoftBody->m_nodes[ _VertexIndex ].m_n );
        }
    }
    return Float3::Zero();
}

Float3 FSoftMeshComponent::GetVertexVelocity( int _VertexIndex ) const {
    if ( SoftBody ) {
        if ( _VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size() ) {
            return btVectorToFloat3( SoftBody->m_nodes[ _VertexIndex ].m_v );
        }
    }
    return Float3::Zero();
}

void FSoftMeshComponent::SetWindVelocity( Float3 const & _Velocity ) {
    //if ( SoftBody ) {
    //    SoftBody->setWindVelocity( btVectorToFloat3( _Velocity ) );
    //}

    WindVelocity = _Velocity;
}

Float3 const & FSoftMeshComponent::GetWindVelocity() const {
    return WindVelocity;
}

void FSoftMeshComponent::AddForceSoftBody( Float3 const & _Force ) {
    if ( SoftBody ) {
        SoftBody->addForce( btVectorToFloat3( _Force ) );
    }
}

void FSoftMeshComponent::AddForceToVertex( Float3 const & _Force, int _VertexIndex ) {
    if ( SoftBody && _VertexIndex >= 0 && _VertexIndex < SoftBody->m_nodes.size() ) {
        SoftBody->addForce( btVectorToFloat3( _Force ), _VertexIndex );
    }
}

void FSoftMeshComponent::UpdateSoftbodyTransform() {
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

void FSoftMeshComponent::UpdateSoftbodyBoundingBox() {
    if ( SoftBody ) {
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

        SoftBody->getAabb( mins, maxs );

        ForceOverrideBounds( true );
        SetBoundsOverride( BvAxisAlignedBox( btVectorToFloat3( mins ), btVectorToFloat3( maxs ) ) );
    }
}

void FSoftMeshComponent::UpdateAnchorPoints() {
    if ( !SoftBody ) {
        return;
    }

    if ( bUpdateAnchors ) {

        btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

        // Remove old anchors. FIXME: is it correct?
        SoftBody->m_collisionDisabledObjects.clear();
        SoftBody->m_anchors.clear();

        // Add new anchors
        for ( FAnchorBinding & binding : Anchors ) {

            if ( binding.VertexIndex < 0 || binding.VertexIndex >= SoftBody->m_nodes.size() ) {
                continue;
            }

            btRigidBody * anchorBody = binding.Anchor->Anchor;

            if ( !anchorBody ) {

                // create rigid body

                anchorBody = b3New( btRigidBody, 0.0f, NULL, b3New( btSphereShape, 0.5f ) );

                physicsWorld->addRigidBody( anchorBody, 0, 0 );

                int collisionFlags = anchorBody->getCollisionFlags();
                collisionFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
                collisionFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
                anchorBody->setCollisionFlags( collisionFlags );

                anchorBody->forceActivationState( DISABLE_DEACTIVATION );

                btTransform transform;

                Float3 worldPosition = binding.Anchor->GetWorldPosition();
                Quat worldRotation = binding.Anchor->GetWorldRotation();

                transform.setOrigin( btVectorToFloat3( worldPosition ) );
                transform.setRotation( btQuaternionToQuat( worldRotation ) );

                anchorBody->setWorldTransform( transform );

                binding.Anchor->Anchor = anchorBody;
            }

            //SoftBody->appendAnchor( binding.VertexIndex, binding.Anchor->Anchor );
            SoftBody->appendAnchor( binding.VertexIndex, binding.Anchor->Anchor,
                btVector3(0,0,0), false, 1 );

            SoftBody->setMass( binding.VertexIndex, 1 );
        }

        //SoftBody->setVelocity( btVector3( 0, 0, 0 ) );

        bUpdateAnchors = false;
    }
}

void FSoftMeshComponent::BeginPlay() {
}

void FSoftMeshComponent::TickComponent( float _TimeStep ) {
    Super::TickComponent( _TimeStep );

    UpdateAnchorPoints();

	// TODO: This must be done in pre physics tick!!!
    if ( SoftBody ) {
        //SoftBody->addVelocity( btVectorToFloat3( WindVelocity*_TimeStep*(FMath::Rand()*0.5f+0.5f) ) );

        btVector3 vel = btVectorToFloat3(WindVelocity*_TimeStep);

        for(int i=0,ni=SoftBody->m_nodes.size();i<ni;++i) SoftBody->addVelocity(vel*(FMath::Rand()*0.5f+0.5f),i);
    }

    // TODO: This must be done in post physics tick!!!
    UpdateSoftbodyTransform();
    UpdateSoftbodyBoundingBox();

    bUpdateAbsoluteTransforms = true;
}

void FSoftMeshComponent::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    if ( !SoftBody ) {
        return;
    }

    // Draw AABB
    //btVector3 mins, maxs;
    //SoftBody->getCollisionShape()->getAabb( SoftBody->getWorldTransform(), mins, maxs );
    //_DebugDraw->SetDepthTest( true );
    //_DebugDraw->SetColor( 1, 1, 0, 1 );
    //_DebugDraw->DrawAABB( BvAxisAlignedBox( btVectorToFloat3( mins ), btVectorToFloat3( maxs ) ) );

    // Draw faces
    if ( GDebugDrawFlags.bDrawSoftmeshFaces ) {
        _DebugDraw->SetDepthTest( true );
        _DebugDraw->SetColor( FColor4( 1, 0, 0, 1 ) );
        for ( int i = 0; i < SoftBody->m_faces.size(); i++ ) {

            btSoftBody::Face & f = SoftBody->m_faces[ i ];

            _DebugDraw->SetColor( Float4( btVectorToFloat3( f.m_normal ) * 0.5f + 0.5f, 1.0f ) );

            _DebugDraw->DrawTriangle(
                btVectorToFloat3( f.m_n[ 0 ]->m_x ),
                btVectorToFloat3( f.m_n[ 1 ]->m_x ),
                btVectorToFloat3( f.m_n[ 2 ]->m_x ), true );
        }
    }
}

void FSoftMeshComponent::AttachVertex( int _VertexIndex, FAnchorComponent * _Anchor ) {
    FAnchorBinding * binding = nullptr;

    for ( int i = 0; i < Anchors.Size(); i++ ) {
        if ( Anchors[ i ].VertexIndex == _VertexIndex ) {
            binding = &Anchors[ i ];
            break;
        }
    }

    if ( binding ) {
        binding->Anchor->RemoveRef();
        binding->Anchor->AttachCount--;
    } else {
        binding = &Anchors.Append();
    }

    binding->VertexIndex = _VertexIndex;
    binding->Anchor = _Anchor;
    _Anchor->AttachCount++;

    _Anchor->AddRef();

    bUpdateAnchors = true;
}

void FSoftMeshComponent::DetachVertex( int _VertexIndex ) {
    for ( int i = 0; i < Anchors.Size(); i++ ) {
        if ( Anchors[ i ].VertexIndex == _VertexIndex ) {
            Anchors[ i ].Anchor->RemoveRef();
            Anchors[ i ].Anchor->AttachCount--;
            Anchors.Remove( i );
            break;
        }
    }

    bUpdateAnchors = true;
}

void FSoftMeshComponent::DetachAllVertices() {
    for ( int i = 0; i < Anchors.Size(); i++ ) {
        Anchors[ i ].Anchor->RemoveRef();
        Anchors[ i ].Anchor->AttachCount--;
    }
    Anchors.Clear();
    bUpdateAnchors = true;
}

FAnchorComponent * FSoftMeshComponent::GetVertexAnchor( int _VertexIndex ) const {
    for ( int i = 0; i < Anchors.Size(); i++ ) {
        if ( Anchors[ i ].VertexIndex == _VertexIndex ) {
            return Anchors[ i ].Anchor;
        }
    }
    return nullptr;
}
