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

#pragma once

#include "SceneComponent.h"
#include "PhysicalBody.h"
#include <World/Public/Terrain.h>
#include <World/Public/Level.h>

class ATerrainComponent : public ASceneComponent
{
    AN_COMPONENT( ATerrainComponent, ASceneComponent )

public:
    AHitProxy * GetHitProxy() const
    {
        return HitProxy;
    }

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    void SetDispatchContactEvents( bool bDispatch )
    {
        HitProxy->bDispatchContactEvents = bDispatch;
    }

    bool ShouldDispatchContactEvents() const
    {
        return HitProxy->bDispatchContactEvents;
    }

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    void SetGenerateContactPoints( bool bGenerate )
    {
        HitProxy->bGenerateContactPoints = bGenerate;
    }

    bool ShouldGenerateContactPoints() const
    {
        return HitProxy->bGenerateContactPoints;
    }

    /** Set collision group/layer. See ECollisionMask. */
    void SetCollisionGroup( int _CollisionGroup );

    /** Get collision group. See ECollisionMask. */
    int GetCollisionGroup() const { return HitProxy->GetCollisionGroup(); }

    /** Set collision mask. See ECollisionMask. */
    void SetCollisionMask( int _CollisionMask );

    /** Get collision mask. See ECollisionMask. */
    int GetCollisionMask() const { return HitProxy->GetCollisionMask(); }

    /** Set collision group and mask. See ECollisionMask. */
    void SetCollisionFilter( int _CollisionGroup, int _CollisionMask );

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor( AActor * _Actor );

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor( AActor * _Actor );

    /** Set terrain resource */
    void SetTerrain( ATerrain * InTerrain );

    /** Get terrain resource */
    ATerrain * GetTerrain() const { return Terrain; }

    /** Visibility group to filter drawables during rendering */
    void SetVisibilityGroup( int InVisibilityGroup );

    int GetVisibilityGroup() const;

    void SetVisible( bool _Visible );

    bool IsVisible() const;

    /** Set hidden during main render pass */
    void SetHiddenInLightPass( bool _HiddenInLightPass );

    bool IsHiddenInLightPass() const;

    void SetQueryGroup( int _UserQueryGroup );

    void SetTwoSidedSurface( bool bTwoSidedSurface );

    uint8_t GetSurfaceFlags() const;

    SPrimitiveDef const * GetPrimitive() const { return &Primitive; }

    /** Allow raycasting */
    void SetAllowRaycast( bool _AllowRaycast );

    bool IsRaycastAllowed() const { return bAllowRaycast; }

    /** Raycast the terrain */
    bool Raycast( Float3 const & InRayStart, Float3 const & InRayEnd, TPodVector< STriangleHitResult > & Hits ) const;

    /** Raycast the terrain */
    bool RaycastClosest( Float3 const & InRayStart, Float3 const & InRayEnd, STriangleHitResult & Hit ) const;

    /** Get X,Z coordinates in local terrain space */
    void GetLocalXZ( Float3 const & Position, float & X, float & Z ) const;

    /** Get terrain triangle at specified world position */
    bool GetTerrainTriangle( Float3 const & Position, STerrainTriangle & Triangle ) const;

    /** Get world transform matrix. Terrain world transform has no scale. */
    Float3x4 const & GetTerrainWorldTransform() const { return TerrainWorldTransform; }

    /** Get world transform matrix inversed. Terrain world transform has no scale. */
    Float3x4 const & GetTerrainWorldTransformInversed() const { return TerrainWorldTransformInv; }

    /** Get bounding box */
    BvAxisAlignedBox const & GetWorldBounds() const { return Primitive.Box; }

    /** Internal rigid body */
    class btRigidBody * GetRigidBody() const { return RigidBody; }

protected:
    ATerrainComponent();

    void InitializeComponent() override;

    void DeinitializeComponent() override;

    void OnTransformDirty() override;

    void DrawDebug( ADebugRenderer * InRenderer ) override;

    void UpdateTransform();
    void UpdateWorldBounds();

    void AddTerrainPhysics();
    void RemoveTerrainPhysics();

    // Terrain resource
    TRef< ATerrain > Terrain;
    // Collision hit proxy
    TRef< AHitProxy > HitProxy;
    // Internal rigid body
    btRigidBody * RigidBody;
    // VSD primitive
    SPrimitiveDef Primitive;
    // Cached world transform
    Float3x4 TerrainWorldTransform;
    // Cached world transform inversed
    Float3x4 TerrainWorldTransformInv;
    // Allow raycast flag
    bool bAllowRaycast : 1;
};
