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


#include <Engine/World/Public/SoftMeshComponent.h>
#include <Engine/World/Public/World.h>

#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
//#include <BulletSoftBody/btSoftBodyHelpers.h>
#include "BulletCompatibility/BulletCompatibility.h"

AN_CLASS_META_NO_ATTRIBS( FSoftMeshComponent )

FSoftMeshComponent::FSoftMeshComponent() {
    bSoftBodySimulation = true;
    bCanEverTick = true;

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

    if ( !sourceMesh || sourceMesh->IsSkinned() ) {
        // TODO: Warning?
        return;
    }

    if ( !SoftMesh ) {
        SoftMesh = NewObject< FIndexedMesh >();
    }

    SoftMesh->Initialize( sourceMesh->GetVertexCount(),
        sourceMesh->GetIndexCount(),
        sourceMesh->GetSubparts().Length(),
        false,
        false );

    memcpy( SoftMesh->GetVertices(), sourceMesh->GetVertices(), sizeof( FMeshVertex ) * SoftMesh->GetVertexCount() );
    memcpy( SoftMesh->GetIndices(), sourceMesh->GetIndices(), sizeof( unsigned int ) * SoftMesh->GetIndexCount() );


    Float3x3 rotation = BaseTransform.DecomposeRotation();
    for ( int i = 0; i < SoftMesh->GetVertexCount(); i++ ) {
        FMeshVertex * v = SoftMesh->GetVertices() + i;

        v->Position = BaseTransform * v->Position;
        v->Normal = rotation * v->Normal;
        v->Tangent = rotation * v->Tangent;
    }
    

    for ( int i = 0; i < SoftMesh->GetSubparts().Length(); i++ ) {
        FIndexedMeshSubpart * dst = SoftMesh->GetSubpart( i );
        FIndexedMeshSubpart * src = sourceMesh->GetSubpart( i );

        dst->BaseVertex = src->BaseVertex;
        dst->FirstIndex = src->FirstIndex;
        dst->VertexCount = src->VertexCount;
        dst->IndexCount = src->IndexCount;
        dst->MaterialInstance = src->MaterialInstance;
        dst->SetBoundingBox( src->GetBoundingBox() );
    }

    SoftMesh->SetName( sourceMesh->GetName() );

    SoftMesh->SendVertexDataToGPU( SoftMesh->GetVertexCount(), 0 );
    SoftMesh->SendIndexDataToGPU( SoftMesh->GetIndexCount(), 0 );

    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    if ( SoftBody ) {
        physicsWorld->removeSoftBody( SoftBody );
        b3Destroy( SoftBody );
        SoftBody = nullptr;
        //PrevTransformBasis.SetIdentity();
        //PrevTransformOrigin.Clear();
    }

    constexpr bool bRandomizeConstraints = true;
    unsigned int maxVertexIndex = 0;

    FMeshVertex const * vertices = SoftMesh->GetVertices();
    unsigned int const * indices = SoftMesh->GetIndices();

    FIndexedMeshSubpartArray const & subparts = SoftMesh->GetSubparts();

    for ( FIndexedMeshSubpart const * subpart : subparts ) {
        for ( int i = 0; i < subpart->IndexCount; ++i ) {
            maxVertexIndex = FMath::Max( subpart->BaseVertex + indices[ subpart->FirstIndex + i ], maxVertexIndex );
        }
    }

    ++maxVertexIndex;
    TPodArray< bool > chks;
    btAlignedObjectArray< btVector3 > vtx;
    //TPodArray< btVector3 > vtx;
    chks.Resize( maxVertexIndex*maxVertexIndex );
    chks.ZeroMem();
    vtx.resize( maxVertexIndex );
    btVector3 sum;
    for ( int i = 0; i < maxVertexIndex; i++ ) {
        vtx[ i ] = btVectorToFloat3( vertices[ i ].Position );
    }

    SoftBody = b3New( btSoftBody, GetWorld()->SoftBodyWorldInfo, vtx.size(), &vtx[ 0 ], 0 );

    int idx[ 3 ];

    for ( FIndexedMeshSubpart const * subpart : subparts ) {
        for ( int i = 0; i < subpart->IndexCount; i += 3 ) {
            idx[ 0 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i ];
            idx[ 1 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i + 1 ];
            idx[ 2 ] = subpart->BaseVertex + indices[ subpart->FirstIndex + i + 2 ];

#define IDX(_x_,_y_) ((_y_)*maxVertexIndex+(_x_))

            for ( int j = 2, k = 0; k < 3; j = k++ ) {
                if ( !chks[ IDX( idx[ j ], idx[ k ] ) ] ) {
                    chks[ IDX( idx[ j ], idx[ k ] ) ] = true;
                    chks[ IDX( idx[ k ], idx[ j ] ) ] = true;
                    SoftBody->appendLink( idx[ j ], idx[ k ] );
                }
            }

#undef IDX

            SoftBody->appendFace( idx[ 0 ], idx[ 1 ], idx[ 2 ] );
        }
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

void FSoftMeshComponent::UpdateSoftbodyMesh() {
    if ( SoftMesh ) {
        FMeshVertex * vertices = SoftMesh->GetVertices();

        for ( int i = 0; i < SoftMesh->GetVertexCount(); i++ ) {
            //Float3 v = GetVertexPosition( i );

            // TODO: optimize transfomation (move to RenderFrontend?)
            //vertices[ i ].Position = PrevTransformBasis * ( v - PrevTransformOrigin );
            //vertices[ i ].Normal = PrevTransformBasis * GetVertexNormal( i );

            vertices[ i ].Position = GetVertexPosition( i );
            vertices[ i ].Normal = GetVertexNormal( i );
        }

        SoftMesh->SendVertexDataToGPU( SoftMesh->GetVertexCount(), 0 );
    }
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

            if ( binding.VertexIndex < 0 || binding.VertexIndex >= SoftMesh->GetVertexCount() ) {
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
        SoftBody->addVelocity( btVectorToFloat3( WindVelocity*_TimeStep*(FMath::Rand()*0.5f+0.5f) ) );
    }

    // TODO: This must be done in post physics tick!!!
    UpdateSoftbodyTransform();
    UpdateSoftbodyBoundingBox();

    // TODO: This must be done just before rendering if mesh is visible
    UpdateSoftbodyMesh();
    
}

void FSoftMeshComponent::DrawDebug( FDebugDraw * _DebugDraw ) {
    Super::DrawDebug( _DebugDraw );

    if ( !SoftBody ) {
        return;
    }

    _DebugDraw->SetDepthTest( true );

    // Draw AABB
    //btVector3 mins, maxs;
    //SoftBody->getCollisionShape()->getAabb( SoftBody->getWorldTransform(), mins, maxs );
    //_DebugDraw->SetColor( 1, 1, 0, 1 );
    //_DebugDraw->DrawAABB( BvAxisAlignedBox( btVectorToFloat3( mins ), btVectorToFloat3( maxs ) ) );

    // Draw faces
    _DebugDraw->SetColor( 1, 0, 0, 1 );
    for ( int i = 0; i < SoftBody->m_faces.size(); i++ ) {

        btSoftBody::Face & f = SoftBody->m_faces[ i ];

        _DebugDraw->SetColor( Float4( btVectorToFloat3( f.m_normal ) * 0.5f + 0.5f, 1.0f ) );

        _DebugDraw->DrawTriangle(
            btVectorToFloat3( f.m_n[ 0 ]->m_x ),
            btVectorToFloat3( f.m_n[ 1 ]->m_x ),
            btVectorToFloat3( f.m_n[ 2 ]->m_x ), true );
    }

}

void FSoftMeshComponent::AttachVertex( int _VertexIndex, FAnchorComponent * _Anchor ) {
    FAnchorBinding * binding = nullptr;

    for ( int i = 0; i < Anchors.Length(); i++ ) {
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
    for ( int i = 0; i < Anchors.Length(); i++ ) {
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
    for ( int i = 0; i < Anchors.Length(); i++ ) {
        Anchors[ i ].Anchor->RemoveRef();
        Anchors[ i ].Anchor->AttachCount--;
    }
    Anchors.Clear();
    bUpdateAnchors = true;
}

FAnchorComponent * FSoftMeshComponent::GetVertexAnchor( int _VertexIndex ) const {
    for ( int i = 0; i < Anchors.Length(); i++ ) {
        if ( Anchors[ i ].VertexIndex == _VertexIndex ) {
            return Anchors[ i ].Anchor;
        }
    }
    return nullptr;
}
