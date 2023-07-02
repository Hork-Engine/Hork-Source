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
#pragma once

#include "HitProxy.h"

#include <Engine/Runtime/Terrain.h>
#include <Engine/Runtime/AINavigationMesh.h>

class btRigidBody;

HK_NAMESPACE_BEGIN

class TerrainComponent : public SceneComponent, private NavigationPrimitive, public TerrainResourceListener
{
    HK_COMPONENT(TerrainComponent, SceneComponent)

public:
    HitProxy* GetHitProxy() const
    {
        return m_HitProxy;
    }

    /** Dispatch contact events (OnBeginContact, OnUpdateContact, OnEndContact) */
    void SetDispatchContactEvents(bool bDispatch)
    {
        m_HitProxy->bDispatchContactEvents = bDispatch;
    }

    bool ShouldDispatchContactEvents() const
    {
        return m_HitProxy->bDispatchContactEvents;
    }

    /** Generate contact points for contact events. Use with bDispatchContactEvents. */
    void SetGenerateContactPoints(bool bGenerate)
    {
        m_HitProxy->bGenerateContactPoints = bGenerate;
    }

    bool ShouldGenerateContactPoints() const
    {
        return m_HitProxy->bGenerateContactPoints;
    }

    /** Set collision group/layer. See COLLISION_MASK. */
    void SetCollisionGroup(COLLISION_MASK _CollisionGroup);

    /** Get collision group. See COLLISION_MASK. */
    COLLISION_MASK GetCollisionGroup() const { return m_HitProxy->GetCollisionGroup(); }

    /** Set collision mask. See COLLISION_MASK. */
    void SetCollisionMask(COLLISION_MASK _CollisionMask);

    /** Get collision mask. See COLLISION_MASK. */
    COLLISION_MASK GetCollisionMask() const { return m_HitProxy->GetCollisionMask(); }

    /** Set collision group and mask. See COLLISION_MASK. */
    void SetCollisionFilter(COLLISION_MASK _CollisionGroup, COLLISION_MASK _CollisionMask);

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(Actor* _Actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(Actor* _Actor);

    /** Set terrain resource */
    void SetTerrain(Terrain* terrain);

    /** Get terrain resource */
    Terrain* GetTerrain() const { return m_Terrain; }

    void SetVisible(bool _Visible);

    bool IsVisible() const;

    /** Set hidden during main render pass */
    void SetHiddenInLightPass(bool _HiddenInLightPass);

    bool IsHiddenInLightPass() const;

    void SetQueryGroup(int _UserQueryGroup);

    void SetTwoSidedSurface(bool bTwoSidedSurface);

    uint8_t GetSurfaceFlags() const;

    /** Allow raycasting */
    void SetAllowRaycast(bool _AllowRaycast);

    bool IsRaycastAllowed() const { return bAllowRaycast; }

    /** Raycast the terrain */
    bool Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TVector<TriangleHitResult>& Hits) const;

    /** Raycast the terrain */
    bool RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit) const;

    /** Get X,Z coordinates in local terrain space */
    void GetLocalXZ(Float3 const& Position, float& X, float& Z) const;

    /** Get terrain triangle at specified world position */
    bool GetTriangle(Float3 const& Position, TerrainTriangle& Triangle) const;

    float SampleHeight(Float3 const& Position) const;

    /** Get world transform matrix. Terrain world transform has no scale. */
    Float3x4 const& GetTerrainWorldTransform() const { return TerrainWorldTransform; }

    /** Get world transform matrix inversed. Terrain world transform has no scale. */
    Float3x4 const& GetTerrainWorldTransformInversed() const { return TerrainWorldTransformInv; }

    /** Get bounding box */
    BvAxisAlignedBox const& GetWorldBounds() const { return Primitive->Box; }

    void GatherCollisionGeometry(BvAxisAlignedBox const& LocalBounds, TVector<Float3>& CollisionVertices, TVector<unsigned int>& CollisionIndices) const;

    void GatherNavigationGeometry(NavigationGeometry& Geometry) const override;

    /** Internal rigid body */
    btRigidBody* GetRigidBody() const { return m_RigidBody; }

protected:
    TerrainComponent();
    ~TerrainComponent();

    void InitializeComponent() override;

    void DeinitializeComponent() override;

    void OnTransformDirty() override;

    void DrawDebug(DebugRenderer* InRenderer) override;

    void OnTerrainResourceUpdate(TERRAIN_UPDATE_FLAG UpdateFlag) override;

    void UpdateTransform();
    void UpdateWorldBounds();

    void AddTerrainPhysics();
    void RemoveTerrainPhysics();

    // Terrain resource
    TRef<Terrain> m_Terrain;
    // Collision hit proxy
    TRef<HitProxy> m_HitProxy;
    // Internal rigid body
    btRigidBody* m_RigidBody{};
    // VSD primitive
    PrimitiveDef* Primitive{};
    // Cached world transform
    Float3x4 TerrainWorldTransform;
    // Cached world transform inversed
    Float3x4 TerrainWorldTransformInv;
    // Allow raycast flag
    bool bAllowRaycast : 1;
};

HK_NAMESPACE_END
#endif
