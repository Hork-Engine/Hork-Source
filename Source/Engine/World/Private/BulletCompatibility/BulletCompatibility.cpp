/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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


#include "BulletCompatibility.h"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btMultiSphereShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btConvexPointCloudShape.h>
#include <BulletCollision/CollisionShapes/btConvexPolyhedron.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btStridingMeshInterface.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

// TODO: replace btIDebugDraw by ADebugRenderer
void btDrawCollisionShape( ADebugRenderer * InRenderer, const btTransform& worldTransform, const btCollisionShape* shape )
{
    class ABulletDebugDraw : public btIDebugDraw
    {
    public:
        ADebugRenderer * Renderer;

        void drawLine( btVector3 const & from, btVector3 const & to, btVector3 const & color ) override
        {
            Renderer->DrawLine( btVectorToFloat3( from ), btVectorToFloat3( to ) );
        }

        void drawContactPoint( btVector3 const & pointOnB, btVector3 const & normalOnB, btScalar distance, int lifeTime, btVector3 const & color ) override
        {
        }

        void reportErrorWarning( const char * warningString ) override
        {
        }

        void draw3dText( btVector3 const & location, const char * textString ) override
        {
        }

        void setDebugMode( int debugMode ) override
        {
        }

        int getDebugMode() const override
        {
            return 0;
        }

        void flushLines() override
        {
        }
    };
    static ABulletDebugDraw debugDrawer;
    static btVector3 dummy(0,0,0);

    debugDrawer.Renderer = InRenderer;

    // Draw a small simplex at the center of the object
    debugDrawer.drawTransform(worldTransform, 0.1f);

    if (shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
    {
        const btCompoundShape* compoundShape = static_cast<const btCompoundShape*>(shape);
        for (int i = compoundShape->getNumChildShapes() - 1; i >= 0; i--)
        {
            btTransform childTrans = compoundShape->getChildTransform(i);
            const btCollisionShape* colShape = compoundShape->getChildShape(i);
            btDrawCollisionShape( InRenderer, worldTransform * childTrans, colShape );
        }
    }
    else
    {
        switch (shape->getShapeType())
        {
        case BOX_SHAPE_PROXYTYPE:
        {
            const btBoxShape* boxShape = static_cast<const btBoxShape*>(shape);
            btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();
            debugDrawer.drawBox(-halfExtents, halfExtents, worldTransform, dummy);
            break;
        }

        case SPHERE_SHAPE_PROXYTYPE:
        {
            const btSphereShape* sphereShape = static_cast<const btSphereShape*>(shape);
            btScalar radius = sphereShape->getMargin();  //radius doesn't include the margin, so draw with margin
            debugDrawer.drawSphere(radius, worldTransform, dummy);
            break;
        }
        case MULTI_SPHERE_SHAPE_PROXYTYPE:
        {
            const btMultiSphereShape* multiSphereShape = static_cast<const btMultiSphereShape*>(shape);

            btTransform childTransform;
            childTransform.setIdentity();

            for (int i = multiSphereShape->getSphereCount() - 1; i >= 0; i--)
            {
                childTransform.setOrigin(multiSphereShape->getSpherePosition(i));
                debugDrawer.drawSphere(multiSphereShape->getSphereRadius(i), worldTransform * childTransform, dummy);
            }

            break;
        }
        case CAPSULE_SHAPE_PROXYTYPE:
        {
            const btCapsuleShape* capsuleShape = static_cast<const btCapsuleShape*>(shape);

            btScalar radius = capsuleShape->getRadius();
            btScalar halfHeight = capsuleShape->getHalfHeight();

            int upAxis = capsuleShape->getUpAxis();
            debugDrawer.drawCapsule(radius, halfHeight, upAxis, worldTransform, dummy);
            break;
        }
        case CONE_SHAPE_PROXYTYPE:
        {
            const btConeShape* coneShape = static_cast<const btConeShape*>(shape);
            btScalar radius = coneShape->getRadius();  //+coneShape->getMargin();
            btScalar height = coneShape->getHeight();  //+coneShape->getMargin();

            int upAxis = coneShape->getConeUpIndex();
            debugDrawer.drawCone(radius, height, upAxis, worldTransform, dummy);
            break;
        }
        case CYLINDER_SHAPE_PROXYTYPE:
        {
            const btCylinderShape* cylinder = static_cast<const btCylinderShape*>(shape);
            int upAxis = cylinder->getUpAxis();
            btScalar radius = cylinder->getRadius();
            btScalar halfHeight = cylinder->getHalfExtentsWithMargin()[upAxis];
            debugDrawer.drawCylinder(radius, halfHeight, upAxis, worldTransform, dummy);
            break;
        }

        case STATIC_PLANE_PROXYTYPE:
        {
            // Static plane shape is not really used in Angie Engine
            const btStaticPlaneShape* staticPlaneShape = static_cast<const btStaticPlaneShape*>(shape);
            btScalar planeConst = staticPlaneShape->getPlaneConstant();
            const btVector3& planeNormal = staticPlaneShape->getPlaneNormal();
            debugDrawer.drawPlane(planeNormal, planeConst, worldTransform, dummy);
            break;
        }
        default:
        {
            /// for polyhedral shapes
            if (shape->isPolyhedral())
            {
                btPolyhedralConvexShape* polyshape = (btPolyhedralConvexShape*)shape;

                int i;
                if (polyshape->getConvexPolyhedron())
                {
                    const btConvexPolyhedron* poly = polyshape->getConvexPolyhedron();
                    for (i = 0; i < poly->m_faces.size(); i++)
                    {
//                        btVector3 centroid(0, 0, 0);
                        int numVerts = poly->m_faces[i].m_indices.size();
                        if (numVerts)
                        {
                            int lastV = poly->m_faces[i].m_indices[numVerts - 1];
                            for (int v = 0; v < poly->m_faces[i].m_indices.size(); v++)
                            {
                                int curVert = poly->m_faces[i].m_indices[v];
//                                centroid += poly->m_vertices[curVert];

                                InRenderer->DrawLine( btVectorToFloat3( worldTransform * poly->m_vertices[lastV] ),
                                                      btVectorToFloat3( worldTransform * poly->m_vertices[curVert] ) );
                                lastV = curVert;
                            }
                        }
//                        centroid *= btScalar(1.f) / btScalar(numVerts);
//                        if (debugDrawer.getDebugMode() & btIDebugDraw::DBG_DrawNormals)
//                        {
//                            btVector3 normalColor(1, 1, 0);
//                            btVector3 faceNormal(poly->m_faces[i].m_plane[0], poly->m_faces[i].m_plane[1], poly->m_faces[i].m_plane[2]);
//                            debugDrawer.drawLine(worldTransform * centroid, worldTransform * (centroid + faceNormal), normalColor);
//                        }
                    }
                }
                else
                {
                    for (i = 0; i < polyshape->getNumEdges(); i++)
                    {
                        btVector3 a, b;
                        polyshape->getEdge(i, a, b);
                        btVector3 wa = worldTransform * a;
                        btVector3 wb = worldTransform * b;
                        InRenderer->DrawLine( btVectorToFloat3( wa ), btVectorToFloat3( wb ) );
                    }
                }
            }

            // FIXME: This is too slow to visualize at real time
#if 0
            if (shape->isConcave())
            {
                btConcaveShape* concaveMesh = (btConcaveShape*)shape;

                ///@todo pass camera, for some culling? no -> we are not a graphics lib
                btVector3 aabbMax(btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT));
                btVector3 aabbMin(btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT));

                DebugDrawcallback drawCallback(debugDrawer, worldTransform, dummy);
                concaveMesh->processAllTriangles(&drawCallback, aabbMin, aabbMax);
            }

            if (shape->getShapeType() == CONVEX_TRIANGLEMESH_SHAPE_PROXYTYPE)
            {
                btConvexTriangleMeshShape* convexMesh = (btConvexTriangleMeshShape*)shape;
                //todo: pass camera for some culling
                btVector3 aabbMax(btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT));
                btVector3 aabbMin(btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT));
                //DebugDrawcallback drawCallback;
                DebugDrawcallback drawCallback(debugDrawer, worldTransform, dummy);
                convexMesh->getMeshInterface()->InternalProcessAllTriangles(&drawCallback, aabbMin, aabbMax);
            }
#endif
        }
        }
    }
}

void btDrawCollisionObject( ADebugRenderer * InRenderer, btCollisionObject * CollisionObject )
{
    AColor4 color( 0.3f, 0.3f, 0.3f );

    switch ( CollisionObject->getActivationState() )
    {
    case ACTIVE_TAG:
        color = AColor4( 1, 1, 1 );
        break;
    case ISLAND_SLEEPING:
        color = AColor4( 0, 1, 0 );
        break;
    case WANTS_DEACTIVATION:
        color = AColor4( 0, 1, 1 );
        break;
    case DISABLE_DEACTIVATION:
        color = AColor4( 1, 0, 0 );
        break;
    case DISABLE_SIMULATION:
        color = AColor4( 1, 1, 0 );
        break;
    default:
        break;
    };

    btVector3 customColor;
    if ( CollisionObject->getCustomDebugColor( customColor ) ) {
        color[0] = customColor[0];
        color[1] = customColor[1];
        color[2] = customColor[2];
    }

    InRenderer->SetColor( color );

    btDrawCollisionShape( InRenderer, CollisionObject->getWorldTransform(), CollisionObject->getCollisionShape() );
}
