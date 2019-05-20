#include <Engine/World/Public/SoftMeshComponent.h>
#include <Engine/World/Public/World.h>

#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include "BulletCompatibility/BulletCompatibility.h"

AN_CLASS_META_NO_ATTRIBS( FSoftMeshComponent )

FSoftMeshComponent::FSoftMeshComponent() {
    bSoftBodySimulation = true;
}

void FSoftMeshComponent::InitializeComponent() {
    Super::InitializeComponent();

    RecreateSoftBody();
}

void FSoftMeshComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    if ( SoftBody ) {
        btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;
        physicsWorld->removeSoftBody( SoftBody );
        b3Destroy( SoftBody );
        SoftBody = nullptr;
    }
}

void FSoftMeshComponent::RecreateSoftBody() {
    btSoftRigidDynamicsWorld * physicsWorld = GetWorld()->PhysicsWorld;

    if ( SoftBody ) {
        physicsWorld->removeSoftBody( SoftBody );
        b3Destroy( SoftBody );
        SoftBody = nullptr;
    }

    FIndexedMesh const * meshResource = GetMesh();
    if ( !meshResource ) {
        return;
    }

    constexpr bool bRandomizeConstraints = true;
    unsigned int maxVertexIndex = 0;

    FMeshVertex const * vertices = meshResource->GetVertices();
    unsigned int const * indices = meshResource->GetIndices();

    FIndexedMeshSubpartArray const & subparts = meshResource->GetSubparts();

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

    btSoftBody::Material*	pm = SoftBody->appendMaterial();
    pm->m_kLST = 0.5;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;
    SoftBody->generateBendingConstraints( 2, pm );
    SoftBody->m_cfg.piterations = 2;
    SoftBody->m_cfg.kDF = 0.5;

    if ( bRandomizeConstraints ) {
        SoftBody->randomizeConstraints();
    }

    //SoftBody->randomizeConstraints();
    //SoftBody->scale( btVector3( 6, 6, 6 ) );
    SoftBody->setTotalMass( 100, true );
    
    physicsWorld->addSoftBody( SoftBody );

    SetWindVelocity( WindVelocity );
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
    if ( SoftBody ) {
        SoftBody->setWindVelocity( btVectorToFloat3( _Velocity ) );
    }

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

#if 0
Config					m_cfg;			// Configuration
SolverState				m_sst;			// Solver state
Pose					m_pose;			// Pose
void*					m_tag;			// User data
btSoftBodyWorldInfo*	m_worldInfo;	// World info
tNoteArray				m_notes;		// Notes
tNodeArray				m_nodes;		// Nodes
tLinkArray				m_links;		// Links
tFaceArray				m_faces;		// Faces
tTetraArray				m_tetras;		// Tetras
tAnchorArray			m_anchors;		// Anchors
tRContactArray			m_rcontacts;	// Rigid contacts
tSContactArray			m_scontacts;	// Soft contacts
tJointArray				m_joints;		// Joints
tMaterialArray			m_materials;	// Materials
btScalar				m_timeacc;		// Time accumulator
btVector3				m_bounds[ 2 ];	// Spatial bounds	
bool					m_bUpdateRtCst;	// Update runtime constants
btDbvt					m_ndbvt;		// Nodes tree
btDbvt					m_fdbvt;		// Faces tree
btDbvt					m_cdbvt;		// Clusters tree
tClusterArray			m_clusters;		// Clusters

btAlignedObjectArray<bool>m_clusterConnectivity;//cluster connectivity, for self-collision

btTransform			m_initialWorldTransform;

btVector3			m_windVelocity;

btScalar        m_restLengthScale;


btAlignedObjectArray<int>	m_userIndexMapping;



bool				checkLink( int node0,
    int node1 ) const;
bool				checkLink( const Node* node0,
    const Node* node1 ) const;
/* Check for existring face												*/
bool				checkFace( int node0,
    int node1,
    int node2 ) const;
/* Append material														*/
Material*			appendMaterial();
/* Append note															*/
void				appendNote( const char* text,
    const btVector3& o,
    const btVector4& c = btVector4( 1, 0, 0, 0 ),
    Node* n0 = 0,
    Node* n1 = 0,
    Node* n2 = 0,
    Node* n3 = 0 );
void				appendNote( const char* text,
    const btVector3& o,
    Node* feature );
void				appendNote( const char* text,
    const btVector3& o,
    Link* feature );
void				appendNote( const char* text,
    const btVector3& o,
    Face* feature );
/* Append node															*/
void				appendNode( const btVector3& x, btScalar m );
/* Append link															*/
void				appendLink( int model = -1, Material* mat = 0 );
void				appendLink( int node0,
    int node1,
    Material* mat = 0,
    bool bcheckexist = false );
void				appendLink( Node* node0,
    Node* node1,
    Material* mat = 0,
    bool bcheckexist = false );
/* Append face															*/
void				appendFace( int model = -1, Material* mat = 0 );
void				appendFace( int node0,
    int node1,
    int node2,
    Material* mat = 0 );
void			appendTetra( int model, Material* mat );
//
void			appendTetra( int node0,
    int node1,
    int node2,
    int node3,
    Material* mat = 0 );


/* Append anchor														*/
void				appendAnchor( int node,
    btRigidBody* body, bool disableCollisionBetweenLinkedBodies = false, btScalar influence = 1 );
void			appendAnchor( int node, btRigidBody* body, const btVector3& localPivot, bool disableCollisionBetweenLinkedBodies = false, btScalar influence = 1 );
/* Append linear joint													*/
void				appendLinearJoint( const LJoint::Specs& specs, Cluster* body0, Body body1 );
void				appendLinearJoint( const LJoint::Specs& specs, Body body = Body() );
void				appendLinearJoint( const LJoint::Specs& specs, btSoftBody* body );
/* Append linear joint													*/
void				appendAngularJoint( const AJoint::Specs& specs, Cluster* body0, Body body1 );
void				appendAngularJoint( const AJoint::Specs& specs, Body body = Body() );
void				appendAngularJoint( const AJoint::Specs& specs, btSoftBody* body );

/* Add aero force to a node of the body */
void			    addAeroForceToNode( const btVector3& windVelocity, int nodeIndex );

/* Add aero force to a face of the body */
void			    addAeroForceToFace( const btVector3& windVelocity, int faceIndex );

/* Add velocity to the entire body										*/
void				addVelocity( const btVector3& velocity );

/* Set velocity for the entire body										*/
void				setVelocity( const btVector3& velocity );

/* Add velocity to a node of the body									*/
void				addVelocity( const btVector3& velocity,
    int node );
/* Set mass																*/
void				setMass( int node,
    btScalar mass );
/* Get mass																*/
btScalar			getMass( int node ) const;
/* Get total mass														*/
btScalar			getTotalMass() const;
/* Set total mass (weighted by previous masses)							*/
void				setTotalMass( btScalar mass,
    bool fromfaces = false );
/* Set total density													*/
void				setTotalDensity( btScalar density );
/* Set volume mass (using tetrahedrons)									*/
void				setVolumeMass( btScalar mass );
/* Set volume density (using tetrahedrons)								*/
void				setVolumeDensity( btScalar density );
/* Transform															*/
void				transform( const btTransform& trs );
/* Translate															*/
void				translate( const btVector3& trs );
/* Rotate															*/
void				rotate( const btQuaternion& rot );
/* Scale																*/
void				scale( const btVector3& scl );
/* Get link resting lengths scale										*/
btScalar			getRestLengthScale();
/* Scale resting length of all springs									*/
void				setRestLengthScale( btScalar restLength );
/* Set current state as pose											*/
void				setPose( bool bvolume,
    bool bframe );
/* Set current link lengths as resting lengths							*/
void				resetLinkRestLengths();
/* Return the volume													*/
btScalar			getVolume() const;
/* Cluster count														*/
int					clusterCount() const;
/* Cluster center of mass												*/
static btVector3	clusterCom( const Cluster* cluster );
btVector3			clusterCom( int cluster ) const;
/* Cluster velocity at rpos												*/
static btVector3	clusterVelocity( const Cluster* cluster, const btVector3& rpos );
/* Cluster impulse														*/
static void			clusterVImpulse( Cluster* cluster, const btVector3& rpos, const btVector3& impulse );
static void			clusterDImpulse( Cluster* cluster, const btVector3& rpos, const btVector3& impulse );
static void			clusterImpulse( Cluster* cluster, const btVector3& rpos, const Impulse& impulse );
static void			clusterVAImpulse( Cluster* cluster, const btVector3& impulse );
static void			clusterDAImpulse( Cluster* cluster, const btVector3& impulse );
static void			clusterAImpulse( Cluster* cluster, const Impulse& impulse );
static void			clusterDCImpulse( Cluster* cluster, const btVector3& impulse );
/* Generate bending constraints based on distance in the adjency graph	*/
int					generateBendingConstraints( int distance,
    Material* mat = 0 );
/* Randomize constraints to reduce solver bias							*/
void				randomizeConstraints();
/* Release clusters														*/
void				releaseCluster( int index );
void				releaseClusters();
/* Generate clusters (K-mean)											*/
///generateClusters with k=0 will create a convex cluster for each tetrahedron or triangle
///otherwise an approximation will be used (better performance)
int					generateClusters( int k, int maxiterations = 8192 );
/* Refine																*/
void				refine( ImplicitFn* ifn, btScalar accurary, bool cut );
/* CutLink																*/
bool				cutLink( int node0, int node1, btScalar position );
bool				cutLink( const Node* node0, const Node* node1, btScalar position );

///Ray casting using rayFrom and rayTo in worldspace, (not direction!)
bool				rayTest( const btVector3& rayFrom,
    const btVector3& rayTo,
    sRayCast& results );
/* Solver presets														*/
void				setSolver( eSolverPresets::_ preset );
/* predictMotion														*/
void				predictMotion( btScalar dt );
/* solveConstraints														*/
void				solveConstraints();
/* staticSolve															*/
void				staticSolve( int iterations );
/* solveCommonConstraints												*/
static void			solveCommonConstraints( btSoftBody** bodies, int count, int iterations );
/* solveClusters														*/
static void			solveClusters( const btAlignedObjectArray<btSoftBody*>& bodies );
/* integrateMotion														*/
void				integrateMotion();
/* defaultCollisionHandlers												*/
void				defaultCollisionHandler( const btCollisionObjectWrapper* pcoWrap );
void				defaultCollisionHandler( btSoftBody* psb );


//
// Set the solver that handles this soft body
// Should not be allowed to get out of sync with reality
// Currently called internally on addition to the world
void setSoftBodySolver( btSoftBodySolver *softBodySolver )
{
    m_softBodySolver = softBodySolver;
}

//
// Return the solver that handles this soft body
// 
btSoftBodySolver *getSoftBodySolver()
{
    return m_softBodySolver;
}

//
// Return the solver that handles this soft body
// 
btSoftBodySolver *getSoftBodySolver() const
{
    return m_softBodySolver;
}


//
// ::btCollisionObject
//

virtual void getAabb( btVector3& aabbMin, btVector3& aabbMax ) const
{
    aabbMin = m_bounds[ 0 ];
    aabbMax = m_bounds[ 1 ];
}
#endif
