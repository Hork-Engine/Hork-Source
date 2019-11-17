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


#include <Engine/World/Public/WorldCollisionQuery.h>
#include <Engine/World/Public/World.h>
#include <Engine/BulletCompatibility/BulletCompatibility.h>

#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>
#include <btBulletDynamicsCommon.h>

static SCollisionQueryFilter DefaultCollisionQueryFilter;

static bool CompareDistance( SCollisionTraceResult const & A, SCollisionTraceResult const & B ) {
    return A.Distance < B.Distance;
}

static bool FindCollisionActor( SCollisionQueryFilter const & _QueryFilter, AActor * _Actor ) {
    for ( int i = 0; i < _QueryFilter.ActorsCount; i++ ) {
        if ( _Actor == _QueryFilter.IgnoreActors[ i ] ) {
            return true;
        }
    }
    return false;
}

static bool FindCollisionBody( SCollisionQueryFilter const & _QueryFilter, APhysicalBody * _Body ) {
    for ( int i = 0; i < _QueryFilter.BodiesCount; i++ ) {
        if ( _Body == _QueryFilter.IgnoreBodies[ i ] ) {
            return true;
        }
    }
    return false;
}

AN_FORCEINLINE static bool NeedsCollision( SCollisionQueryFilter const & _QueryFilter, btBroadphaseProxy * _Proxy ) {
    APhysicalBody * body = static_cast< APhysicalBody * >( static_cast< btCollisionObject * >( _Proxy->m_clientObject )->getUserPointer() );

    if ( body ) {
        if ( FindCollisionActor( _QueryFilter, body->GetParentActor() ) ) {
            return false;
        }

        if ( FindCollisionBody( _QueryFilter, body ) ) {
            return false;
        }
    } else {
        // ghost object
    }

    return ( _Proxy->m_collisionFilterGroup & _QueryFilter.CollisionMask ) && _Proxy->m_collisionFilterMask;
}

AN_FORCEINLINE static unsigned short ClampUnsignedShort( int _Value ) {
    if ( _Value < 0 ) return 0;
    if ( _Value > 0xffff ) return 0xffff;
    return _Value;
}

struct STraceRayResultCallback : btCollisionWorld::AllHitsRayResultCallback {

    STraceRayResultCallback( SCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : btCollisionWorld::AllHitsRayResultCallback( rayFromWorld, rayToWorld )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );

        m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;

        //    enum EFlags
        //    {
        //       kF_None                 = 0,
        //       kF_FilterBackfaces      = 1 << 0,
        //       kF_KeepUnflippedNormal  = 1 << 1,   // Prevents returned face normal getting flipped when a ray hits a back-facing triangle
        //         ///SubSimplexConvexCastRaytest is the default, even if kF_None is set.
        //       kF_UseSubSimplexConvexCastRaytest = 1 << 2,   // Uses an approximate but faster ray versus convex intersection algorithm
        //       kF_UseGjkConvexCastRaytest = 1 << 3,
        //       kF_Terminator        = 0xFFFFFFFF
        //    };
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    SCollisionQueryFilter const & QueryFilter;
};

struct STraceClosestRayResultCallback : btCollisionWorld::ClosestRayResultCallback {

    STraceClosestRayResultCallback( SCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : ClosestRayResultCallback( rayFromWorld, rayToWorld )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );

        m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
        m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    SCollisionQueryFilter const & QueryFilter;
};

struct STraceClosestConvexResultCallback : btCollisionWorld::ClosestConvexResultCallback {

    STraceClosestConvexResultCallback( SCollisionQueryFilter const * _QueryFilter, btVector3 const & rayFromWorld, btVector3 const & rayToWorld )
        : ClosestConvexResultCallback( rayFromWorld, rayToWorld )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    SCollisionQueryFilter const & QueryFilter;
};

bool AWorldCollisionQuery::Trace( AWorld const * _World, TPodArray< SCollisionTraceResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) {
    STraceRayResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    _World->PhysicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    _Result.Resize( hitResult.m_collisionObjects.size() );

    for ( int i = 0; i < hitResult.m_collisionObjects.size(); i++ ) {
        SCollisionTraceResult & result = _Result[ i ];
        result.Body = static_cast< APhysicalBody * >( hitResult.m_collisionObjects[ i ]->getUserPointer() );
        result.Position = btVectorToFloat3( hitResult.m_hitPointWorld[ i ] );
        result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld[ i ] );
        result.Distance = ( result.Position - _RayStart ).Length();
        result.Fraction = hitResult.m_closestHitFraction;
    }

    //if ( bForcePerTriangleCheck ) {
        // TODO: check intersection with triangles!
    //}

    if ( !_QueryFilter ) {
        _QueryFilter = &DefaultCollisionQueryFilter;
    }

    if ( _QueryFilter->bSortByDistance ) {
        StdSort( _Result.Begin(), _Result.End(), CompareDistance );
    }

    return !_Result.IsEmpty();
}

bool AWorldCollisionQuery::TraceClosest( AWorld const * _World, SCollisionTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) {
    STraceClosestRayResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    _World->PhysicsWorld->rayTest( hitResult.m_rayFromWorld, hitResult.m_rayToWorld, hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< APhysicalBody * >( hitResult.m_collisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = ( _Result.Position - _RayStart ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldCollisionQuery::TraceSphere( AWorld const * _World, SCollisionTraceResult & _Result, float _Radius, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) {
    STraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( _RayStart ), btVectorToFloat3( _RayEnd ) );

    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );

    _World->PhysicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexFromWorld ),
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexToWorld ), hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< APhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( _RayEnd - _RayStart ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldCollisionQuery::TraceBox( AWorld const * _World, SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) {
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    STraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    btBoxShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

    _World->PhysicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexFromWorld ),
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexToWorld ), hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< APhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldCollisionQuery::TraceCylinder( AWorld const * _World, SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) {
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    STraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    btCylinderShape shape( btVectorToFloat3( halfExtents ) );
    shape.setMargin( 0.0f );

    _World->PhysicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexFromWorld ),
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexToWorld ), hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< APhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldCollisionQuery::TraceCapsule( AWorld const * _World, SCollisionTraceResult & _Result, Float3 const & _Mins, Float3 const & _Maxs, Float3 const & _RayStart, Float3 const & _RayEnd, SCollisionQueryFilter const * _QueryFilter ) {
    Float3 boxPosition = ( _Maxs + _Mins ) * 0.5f;
    Float3 halfExtents = ( _Maxs - _Mins ) * 0.5f;
    Float3 startPos = boxPosition + _RayStart;
    Float3 endPos = boxPosition + _RayEnd;

    STraceClosestConvexResultCallback hitResult( _QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    float radius = Math::Max( halfExtents[0], halfExtents[2] );

    btCapsuleShape shape( radius, (halfExtents[1]-radius)*2.0f );
    shape.setMargin( 0.0f );

    _World->PhysicsWorld->convexSweepTest( &shape,
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexFromWorld ),
        btTransform( btQuaternion::getIdentity(), hitResult.m_convexToWorld ), hitResult );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< APhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

bool AWorldCollisionQuery::TraceConvex( AWorld const * _World, SCollisionTraceResult & _Result, SConvexSweepTest const & _SweepTest ) {

    if ( !_SweepTest.CollisionBody->IsConvex() ) {
        GLogger.Printf( "AWorld::TraceConvex: non-convex collision body for convex trace\n" );
        _Result.Clear();
        return false;
    }

    btCollisionShape * shape = _SweepTest.CollisionBody->Create();
    shape->setMargin( _SweepTest.CollisionBody->Margin );

    AN_Assert( shape->isConvex() );

    Float3x4 startTransform, endTransform;

    startTransform.Compose( _SweepTest.StartPosition, _SweepTest.StartRotation.ToMatrix(), _SweepTest.Scale );
    endTransform.Compose( _SweepTest.EndPosition, _SweepTest.EndRotation.ToMatrix(), _SweepTest.Scale );

    Float3 startPos = startTransform * _SweepTest.CollisionBody->Position;
    Float3 endPos = endTransform * _SweepTest.CollisionBody->Position;
    Quat startRot = _SweepTest.StartRotation * _SweepTest.CollisionBody->Rotation;
    Quat endRot = _SweepTest.EndRotation * _SweepTest.CollisionBody->Rotation;

    STraceClosestConvexResultCallback hitResult( &_SweepTest.QueryFilter, btVectorToFloat3( startPos ), btVectorToFloat3( endPos ) );

    _World->PhysicsWorld->convexSweepTest( static_cast< btConvexShape * >( shape ),
        btTransform( btQuaternionToQuat( startRot ), hitResult.m_convexFromWorld ),
        btTransform( btQuaternionToQuat( endRot ), hitResult.m_convexToWorld ), hitResult );

    b3Destroy( shape );

    if ( !hitResult.hasHit() ) {
        _Result.Clear();
        return false;
    }

    _Result.Body = static_cast< APhysicalBody * >( hitResult.m_hitCollisionObject->getUserPointer() );
    _Result.Position = btVectorToFloat3( hitResult.m_hitPointWorld );
    _Result.Normal = btVectorToFloat3( hitResult.m_hitNormalWorld );
    _Result.Distance = hitResult.m_closestHitFraction * ( endPos - startPos ).Length();
    _Result.Fraction = hitResult.m_closestHitFraction;
    return true;
}

struct SQueryPhysicalBodiesCallback : public btCollisionWorld::ContactResultCallback {
    SQueryPhysicalBodiesCallback( TPodArray< APhysicalBody * > & _Result, SCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        APhysicalBody * body;

        body = reinterpret_cast< APhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body );
        }

        body = reinterpret_cast< APhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body );
        }

        return 0.0f;
    }

    TPodArray< APhysicalBody * > & Result;
    SCollisionQueryFilter const & QueryFilter;
};

struct SQueryActorsCallback : public btCollisionWorld::ContactResultCallback {
    SQueryActorsCallback( TPodArray< AActor * > & _Result, SCollisionQueryFilter const * _QueryFilter )
        : Result( _Result )
        , QueryFilter( _QueryFilter ? *_QueryFilter : DefaultCollisionQueryFilter )
    {
        _Result.Clear();

        m_collisionFilterGroup = ( short )0xffff;
        m_collisionFilterMask = ClampUnsignedShort( QueryFilter.CollisionMask );
    }

    bool needsCollision( btBroadphaseProxy* proxy0 ) const override {
        return NeedsCollision( QueryFilter, proxy0 );
    }

    btScalar addSingleResult( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1 ) override {
        APhysicalBody * body;

        body = reinterpret_cast< APhysicalBody * >( colObj0Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body->GetParentActor() ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        body = reinterpret_cast< APhysicalBody * >( colObj1Wrap->getCollisionObject()->getUserPointer() );
        if ( body && Result.Find( body->GetParentActor() ) == Result.End() && ( body->CollisionGroup & QueryFilter.CollisionMask ) ) {
            Result.Append( body->GetParentActor() );
        }

        return 0.0f;
    }

    TPodArray< AActor * > & Result;
    SCollisionQueryFilter const & QueryFilter;
};

void AWorldCollisionQuery::QueryPhysicalBodies( AWorld const * _World, TPodArray< APhysicalBody * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter ) {
    SQueryPhysicalBodiesCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    _World->PhysicsWorld->addRigidBody( tempBody );
    _World->PhysicsWorld->contactTest( tempBody, callback );
    _World->PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void AWorldCollisionQuery::QueryActors( AWorld const * _World, TPodArray< AActor * > & _Result, Float3 const & _Position, float _Radius, SCollisionQueryFilter const * _QueryFilter ) {
    SQueryActorsCallback callback( _Result, _QueryFilter );
    btSphereShape shape( _Radius );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    _World->PhysicsWorld->addRigidBody( tempBody );
    _World->PhysicsWorld->contactTest( tempBody, callback );
    _World->PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void AWorldCollisionQuery::QueryPhysicalBodies( AWorld const * _World, TPodArray< APhysicalBody * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter ) {
    SQueryPhysicalBodiesCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    _World->PhysicsWorld->addRigidBody( tempBody );
    _World->PhysicsWorld->contactTest( tempBody, callback );
    _World->PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void AWorldCollisionQuery::QueryActors( AWorld const * _World, TPodArray< AActor * > & _Result, Float3 const & _Position, Float3 const & _HalfExtents, SCollisionQueryFilter const * _QueryFilter ) {
    SQueryActorsCallback callback( _Result, _QueryFilter );
    btBoxShape shape( btVectorToFloat3( _HalfExtents ) );
    shape.setMargin( 0.0f );
    btRigidBody * tempBody = b3New( btRigidBody, 1.0f, nullptr, &shape );
    tempBody->setWorldTransform( btTransform( btQuaternion::getIdentity(), btVectorToFloat3( _Position ) ) );
    tempBody->activate();
    _World->PhysicsWorld->addRigidBody( tempBody );
    _World->PhysicsWorld->contactTest( tempBody, callback );
    _World->PhysicsWorld->removeRigidBody( tempBody );
    b3Destroy( tempBody );
}

void AWorldCollisionQuery::QueryPhysicalBodies( AWorld const * _World, TPodArray< APhysicalBody * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter ) {
    QueryPhysicalBodies( _World, _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}

void AWorldCollisionQuery::QueryActors( AWorld const * _World, TPodArray< AActor * > & _Result, BvAxisAlignedBox const & _BoundingBox, SCollisionQueryFilter const * _QueryFilter ) {
    QueryActors( _World, _Result, _BoundingBox.Center(), _BoundingBox.HalfSize(), _QueryFilter );
}
