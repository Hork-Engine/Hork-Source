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

#include "TerrainComponent.h"
#include "World.h"

#include <Core/ConsoleVar.h>
#include <Geometry/BV/BvIntersect.h>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "BulletCompatibility.h"

AConsoleVar com_DrawTerrainBounds("com_DrawTerrainBounds"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(ATerrainComponent)

static bool RaycastCallback(SPrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<STriangleHitResult>& Hits)
{
    ATerrainComponent const* terrain        = static_cast<ATerrainComponent const*>(Self->Owner);
    bool                     bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    ATerrain* resource = terrain->GetTerrain();
    if (!resource)
    {
        return false;
    }

    Float3x4 const& transformInverse = terrain->GetTerrainWorldTransformInversed();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal   = transformInverse * InRayEnd;
    Float3 rayDirLocal   = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if (hitDistanceLocal < 0.0001f)
    {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    int firstHit = Hits.Size();

    if (!resource->Raycast(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hits))
    {
        return false;
    }

    // Convert hits to worldspace and find closest hit

    Float3x4 const& transform = terrain->GetTerrainWorldTransform();

#if 0 // General case
    Float3x3 normalMatrix;
    transform.DecomposeNormalMatrix( normalMatrix );
#else
    Float3x3 normalMatrix = terrain->GetWorldRotation().ToMatrix3x3();
#endif

    int numHits = Hits.Size() - firstHit;
    for (int i = 0; i < numHits; i++)
    {
        int                 hitNum    = firstHit + i;
        STriangleHitResult& hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal   = (normalMatrix * hitResult.Normal).Normalized();

        // No need to recalc hit distance
        //hitResult.Distance = (hitResult.Location - InRayStart).Length();
    }

    return true;
}

static bool RaycastClosestCallback(SPrimitiveDef const* Self,
                                   Float3 const&        InRayStart,
                                   Float3 const&        InRayEnd,
                                   STriangleHitResult&  Hit,
                                   SMeshVertex const**  pVertices)
{
    ATerrainComponent const* terrain        = static_cast<ATerrainComponent const*>(Self->Owner);
    bool                     bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    ATerrain* resource = terrain->GetTerrain();
    if (!resource)
    {
        return false;
    }

    Float3x4 const& transformInverse = terrain->GetTerrainWorldTransformInversed();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal   = transformInverse * InRayEnd;
    Float3 rayDirLocal   = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if (hitDistanceLocal < 0.0001f)
    {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    if (!resource->RaycastClosest(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hit))
    {
        return false;
    }

    if (pVertices)
    {
        *pVertices = nullptr;
    }

    // Transform hit location to world space
    Hit.Location = terrain->GetTerrainWorldTransform() * Hit.Location;

    // No need to recalc hit distance
    //Hit.Distance = (Hit.Location - InRayStart).Length();

#if 0 // General case
    Float3x3 normalMatrix;
    TerrainWorldTransform.DecomposeNormalMatrix( normalMatrix );
    Hit.Normal = (normalMatrix * Hit.Normal).Normalized();
#else
    Hit.Normal = terrain->GetWorldRotation().ToMatrix3x3() * Hit.Normal;
    Hit.Normal.NormalizeSelf();
#endif

    return true;
}

static void EvaluateRaycastResult(SPrimitiveDef*       Self,
                                  ALevel const*        LightingLevel,
                                  SMeshVertex const*   pVertices,
                                  SMeshVertexUV const* pLightmapVerts,
                                  int                  LightmapBlock,
                                  unsigned int const*  pIndices,
                                  Float3 const&        HitLocation,
                                  Float2 const&        HitUV,
                                  Float3*              Vertices,
                                  Float2&              TexCoord,
                                  Float3&              LightmapSample)
{
    ATerrainComponent* terrain = static_cast<ATerrainComponent*>(Self->Owner);

    STerrainTriangle triangle;

    terrain->GetTriangle(HitLocation, triangle);

    Vertices[0]    = triangle.Vertices[0];
    Vertices[1]    = triangle.Vertices[1];
    Vertices[2]    = triangle.Vertices[2];
    TexCoord       = triangle.Texcoord;
    LightmapSample = Float3(0.0f);
}

ATerrainComponent::ATerrainComponent()
{
    HitProxy = NewObj<AHitProxy>();

    Primitive                         = AVisibilitySystem::AllocatePrimitive();
    Primitive->Owner                  = this;
    Primitive->Type                   = VSD_PRIMITIVE_BOX;
    Primitive->VisGroup               = VISIBILITY_GROUP_TERRAIN;
    Primitive->QueryGroup             = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS /* | VSD_QUERY_MASK_SHADOW_CAST*/;
    Primitive->bIsOutdoor             = true;
    Primitive->RaycastCallback        = RaycastCallback;
    Primitive->RaycastClosestCallback = RaycastClosestCallback;
    Primitive->EvaluateRaycastResult  = EvaluateRaycastResult;

    bAllowRaycast = true;

    TerrainWorldTransform.SetIdentity();
    TerrainWorldTransformInv.SetIdentity();
}

ATerrainComponent::~ATerrainComponent()
{
    AVisibilitySystem::DeallocatePrimitive(Primitive);

    if (Terrain)
    {
        Terrain->RemoveListener(this);
    }
}

void ATerrainComponent::SetVisible(bool _Visible)
{
    if (_Visible)
    {
        Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    }
    else
    {
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}

bool ATerrainComponent::IsVisible() const
{
    return !!(Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE);
}

void ATerrainComponent::SetHiddenInLightPass(bool _HiddenInLightPass)
{
    if (_HiddenInLightPass)
    {
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
    else
    {
        Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
        Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS;
    }
}

bool ATerrainComponent::IsHiddenInLightPass() const
{
    return !(Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS);
}

void ATerrainComponent::SetQueryGroup(int _UserQueryGroup)
{
    Primitive->QueryGroup |= VSD_QUERY_MASK(_UserQueryGroup & 0xffff0000);
}

void ATerrainComponent::SetTwoSidedSurface(bool bTwoSidedSurface)
{
    if (bTwoSidedSurface)
    {
        Primitive->Flags |= SURF_TWOSIDED;
    }
    else
    {
        Primitive->Flags &= ~SURF_TWOSIDED;
    }
}

uint8_t ATerrainComponent::GetSurfaceFlags() const
{
    return Primitive->Flags;
}

void ATerrainComponent::AddTerrainPhysics()
{
    if (IsInEditor())
    {
        // Do not add/remove physics for objects in editor
        return;
    }

    if (!Terrain)
    {
        // No terrain resource assigned to component
        return;
    }

    HK_ASSERT(RigidBody == nullptr);

    float verticalOffset = (Terrain->GetMinHeight() + Terrain->GetMaxHeight()) * 0.5f;

    Float3   worldPosition = TerrainWorldTransform * Float3(0.0f, verticalOffset, 0.0f);
    Float3x3 worldRotation = GetWorldRotation().ToMatrix3x3();

    btRigidBody::btRigidBodyConstructionInfo contructInfo(0.0f, nullptr, Terrain->GetHeightfieldShape());
    contructInfo.m_startWorldTransform.setOrigin(btVectorToFloat3(worldPosition));
    contructInfo.m_startWorldTransform.setBasis(btMatrixToFloat3x3(worldRotation.Transposed()));

    RigidBody = new btRigidBody(contructInfo);
    RigidBody->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT /*| btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT*/);
    RigidBody->setUserPointer(HitProxy.GetObject());

    HitProxy->Initialize(this, RigidBody);
}

void ATerrainComponent::RemoveTerrainPhysics()
{
    if (IsInEditor())
    {
        // Do not add/remove physics for objects in editor
        return;
    }

    if (RigidBody == nullptr)
    {
        return;
    }

    HitProxy->Deinitialize();

    delete RigidBody;
    RigidBody = nullptr;
}

void ATerrainComponent::InitializeComponent()
{
    Super::InitializeComponent();

    UpdateTransform();

    AddTerrainPhysics();

    GetWorld()->VisibilitySystem.AddPrimitive(Primitive);

    AAINavigationMesh & NavigationMesh = GetWorld()->NavigationMesh;
    NavigationMesh.NavigationPrimitives.Add(this);
}

void ATerrainComponent::DeinitializeComponent()
{
    AAINavigationMesh& NavigationMesh = GetWorld()->NavigationMesh;
    NavigationMesh.NavigationPrimitives.Remove(this);

    if (Terrain)
    {
        Terrain->RemoveListener(this);
    }

    RemoveTerrainPhysics();

    GetWorld()->VisibilitySystem.RemovePrimitive(Primitive);

    Super::DeinitializeComponent();
}

void ATerrainComponent::SetTerrain(ATerrain* InTerrain)
{
    if (Terrain)
    {
        Terrain->RemoveListener(this);
    }

    Terrain = InTerrain;

    if (Terrain)
    {
        Terrain->AddListener(this);
    }

    if (IsInitialized())
    {
        // Keep terrain physics in sync with terrain resource
        RemoveTerrainPhysics();
        AddTerrainPhysics();

        UpdateWorldBounds();
    }
}

void ATerrainComponent::OnTerrainModified()
{
    if (IsInitialized())
    {
        // Keep terrain physics in sync with terrain resource
        RemoveTerrainPhysics();
        AddTerrainPhysics();

        UpdateWorldBounds();
    }
}

void ATerrainComponent::SetAllowRaycast(bool _AllowRaycast)
{
    if (_AllowRaycast)
    {
        Primitive->RaycastCallback        = RaycastCallback;
        Primitive->RaycastClosestCallback = RaycastClosestCallback;
    }
    else
    {
        Primitive->RaycastCallback        = nullptr;
        Primitive->RaycastClosestCallback = nullptr;
    }
    bAllowRaycast = _AllowRaycast;
}

bool ATerrainComponent::Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<STriangleHitResult>& Hits) const
{
    if (!Primitive->RaycastCallback)
    {
        return false;
    }

    Hits.Clear();

    return Primitive->RaycastCallback(Primitive, InRayStart, InRayEnd, Hits);
}

bool ATerrainComponent::RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, STriangleHitResult& Hit) const
{
    if (!Primitive->RaycastCallback)
    {
        return false;
    }

    return Primitive->RaycastClosestCallback(Primitive, InRayStart, InRayEnd, Hit, nullptr);
}

void ATerrainComponent::UpdateTransform()
{
    Float3   worldPosition = GetWorldPosition();
    Float3x3 worldRotation = GetWorldRotation().ToMatrix3x3();

    // Terrain transform without scale
    TerrainWorldTransform.Compose(worldPosition, worldRotation);

    // Terrain inversed transform
    TerrainWorldTransformInv = TerrainWorldTransform.Inversed();

    UpdateWorldBounds();
}

void ATerrainComponent::UpdateWorldBounds()
{
    if (!Terrain)
    {
        return;
    }

    Primitive->Box = Terrain->GetBoundingBox().Transform(TerrainWorldTransform);

    // NOTE: Terrain is always in outdoor area. So we don't need to update primitive.
}

void ATerrainComponent::OnTransformDirty()
{
    Super::OnTransformDirty();

    UpdateTransform();

    if (!IsInEditor())
    {
        LOG("WARNING: Set transform for terrain {}\n", GetObjectName());
    }

    // Update rigid body transform
    if (Terrain && RigidBody)
    {
        btTransform worldTransform;
        Float3      worldPosition = TerrainWorldTransform * Float3(0.0f, (Terrain->GetMinHeight() + Terrain->GetMaxHeight()) * 0.5f, 0.0f);
        Float3x3    worldRotation = GetWorldRotation().ToMatrix3x3();

        worldTransform.setOrigin(btVectorToFloat3(worldPosition));
        worldTransform.setBasis(btMatrixToFloat3x3(worldRotation.Transposed()));
        RigidBody->setWorldTransform(worldTransform);
    }
}

void ATerrainComponent::GetLocalXZ(Float3 const& InPosition, float& X, float& Z) const
{
    // position in terrain space
    Float3 localPosition = TerrainWorldTransformInv * InPosition;

    X = localPosition.X;
    Z = localPosition.Z;
}

bool ATerrainComponent::GetTriangle(Float3 const& InPosition, STerrainTriangle& Triangle) const
{
    if (!Terrain)
    {
        return false;
    }

    // position in terrain space
    Float3 localPosition = TerrainWorldTransformInv * InPosition;

    if (!Terrain->GetTriangle(localPosition.X, localPosition.Z, Triangle))
    {
        return false;
    }

    // Convert triangle to world space
    Triangle.Vertices[0] = TerrainWorldTransform * Triangle.Vertices[0];
    Triangle.Vertices[1] = TerrainWorldTransform * Triangle.Vertices[1];
    Triangle.Vertices[2] = TerrainWorldTransform * Triangle.Vertices[2];

#if 0 // General case
    Float3x3 normalMatrix;
    TerrainWorldTransform.DecomposeNormalMatrix( normalMatrix );
    Triangle.Normal = (normalMatrix * Triangle.Normal).Normalized();
#else
    Triangle.Normal = (GetWorldRotation().ToMatrix3x3() * Triangle.Normal).Normalized();
#endif

    return true;
}

float ATerrainComponent::SampleHeight(Float3 const& InPosition) const
{
    if (!Terrain)
    {
        return 0.0f;
    }

    float x, z;
    GetLocalXZ(InPosition, x, z);
    return Terrain->SampleHeight(x, z);
}

void ATerrainComponent::SetCollisionGroup(COLLISION_MASK _CollisionGroup)
{
    HitProxy->SetCollisionGroup(_CollisionGroup);
}

void ATerrainComponent::SetCollisionMask(COLLISION_MASK _CollisionMask)
{
    HitProxy->SetCollisionMask(_CollisionMask);
}

void ATerrainComponent::SetCollisionFilter(COLLISION_MASK _CollisionGroup, COLLISION_MASK _CollisionMask)
{
    HitProxy->SetCollisionFilter(_CollisionGroup, _CollisionMask);
}

void ATerrainComponent::AddCollisionIgnoreActor(AActor* _Actor)
{
    HitProxy->AddCollisionIgnoreActor(_Actor);
}

void ATerrainComponent::RemoveCollisionIgnoreActor(AActor* _Actor)
{
    HitProxy->RemoveCollisionIgnoreActor(_Actor);
}

void ATerrainComponent::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawTerrainBounds && Terrain)
    {
        if (Primitive->VisPass == InRenderer->GetVisPass())
        {
            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(1, 0, 0, 1));
            InRenderer->DrawAABB(Primitive->Box);
        }
    }
}

void ATerrainComponent::GatherCollisionGeometry(BvAxisAlignedBox const& LocalBounds, TVector<Float3>& CollisionVertices, TVector<unsigned int>& CollisionIndices) const
{
    if (!Terrain)
        return;

    int firstVert = CollisionVertices.Size();
    Terrain->GatherGeometry(LocalBounds, CollisionVertices, CollisionIndices);

    int numVerts = CollisionVertices.Size() - firstVert;
    if (numVerts)
    {
        Float3x4 const& transformMatrix = GetWorldTransformMatrix();
        for (int i = 0 ; i < numVerts ; i++)
        {
            CollisionVertices[firstVert + i] = transformMatrix * CollisionVertices[firstVert + i];
        }
    }
}

void ATerrainComponent::GatherNavigationGeometry(SNavigationGeometry& Geometry) const
{
    if (!Terrain)
    {
        return;
    }

    TVector<Float3>       collisionVertices;
    TVector<unsigned int> collisionIndices;

    TVector<Float3>&       Vertices          = Geometry.Vertices;
    TVector<unsigned int>& Indices           = Geometry.Indices;
    TBitMask<>&                   WalkableTriangles = Geometry.WalkableMask;
    BvAxisAlignedBox&             ResultBoundingBox = Geometry.BoundingBox;
    BvAxisAlignedBox const*       pClipBoundingBox  = Geometry.pClipBoundingBox;

    Float3x4 const& worldTransform    = GetWorldTransformMatrix();
    Float3x4        worldTransformInv = worldTransform.Inversed();

    TPodVector<BvAxisAlignedBox> const& areas = Terrain->NavigationAreas;

    // Gather terrain geometry from navigation areas
    for (BvAxisAlignedBox const& areaBounds : areas)
    {
        collisionVertices.Clear();
        collisionIndices.Clear();

        if (pClipBoundingBox)
        {
            Float3 center   = pClipBoundingBox->Center();
            Float3 halfSize = pClipBoundingBox->HalfSize();

            BvOrientedBox obb;
            obb.FromAxisAlignedBox(areaBounds, worldTransform);

            // Early cut off - bounding boxes are not overlap
            if (!BvOrientedBoxOverlapBox(obb, center, halfSize))
            {
                continue;
            }

            BvAxisAlignedBox clippedAreaBounds;

            // Transform clipping box to local terrain space and calc intersection
            BvAxisAlignedBox localClip = pClipBoundingBox->Transform(worldTransformInv);
            if (!BvGetBoxIntersection(areaBounds, localClip, clippedAreaBounds))
            {
                // This should not be happen, but just in case
                continue;
            }

            GatherCollisionGeometry(clippedAreaBounds, collisionVertices, collisionIndices);
        }
        else
        {
            GatherCollisionGeometry(areaBounds, collisionVertices, collisionIndices);
        }

        if (collisionIndices.IsEmpty())
        {
            continue;
        }

        Float3 const*       srcVertices = collisionVertices.ToPtr();
        unsigned int const* srcIndices  = collisionIndices.ToPtr();

        int firstVertex   = Vertices.Size();
        int firstIndex    = Indices.Size();
        int firstTriangle = Indices.Size() / 3;
        int vertexCount   = collisionVertices.Size();
        int indexCount    = collisionIndices.Size();

        Vertices.Resize(firstVertex + vertexCount);
        Indices.Resize(firstIndex + indexCount);
        WalkableTriangles.Resize(firstTriangle + indexCount / 3);

        Float3*       pVertices = Vertices.ToPtr() + firstVertex;
        unsigned int* pIndices  = Indices.ToPtr() + firstIndex;

        Platform::Memcpy(pVertices, srcVertices, vertexCount * sizeof(Float3));

        if (pClipBoundingBox)
        {
            // Clip triangles
            unsigned int i0, i1, i2;
            const int    numTriangles = indexCount / 3;
            int          triangleNum  = 0;
            for (int i = 0; i < numTriangles; i++)
            {
                i0 = firstVertex + srcIndices[i * 3 + 0];
                i1 = firstVertex + srcIndices[i * 3 + 1];
                i2 = firstVertex + srcIndices[i * 3 + 2];

                if (BvBoxOverlapTriangle_FastApproximation(*pClipBoundingBox, Vertices[i0], Vertices[i1], Vertices[i2]))
                {
                    *pIndices++ = i0;
                    *pIndices++ = i1;
                    *pIndices++ = i2;

                    ResultBoundingBox.AddPoint(Vertices[i0]);
                    ResultBoundingBox.AddPoint(Vertices[i1]);
                    ResultBoundingBox.AddPoint(Vertices[i2]);

                    WalkableTriangles.Mark(firstTriangle + triangleNum);

                    triangleNum++;
                }
            }
            Indices.Resize(firstIndex + triangleNum * 3);
            WalkableTriangles.Resize(firstTriangle + triangleNum);
        }
        else
        {
            const int numTriangles = indexCount / 3;
            for (int i = 0; i < numTriangles; i++, pIndices += 3)
            {
                pIndices[0] = firstVertex + srcIndices[i * 3 + 0];
                pIndices[1] = firstVertex + srcIndices[i * 3 + 1];
                pIndices[2] = firstVertex + srcIndices[i * 3 + 2];

                ResultBoundingBox.AddPoint(Vertices[pIndices[0]]);
                ResultBoundingBox.AddPoint(Vertices[pIndices[1]]);
                ResultBoundingBox.AddPoint(Vertices[pIndices[2]]);

                WalkableTriangles.Mark(firstTriangle + i);
            }
        }
    }
}
