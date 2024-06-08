/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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
struct TerrainTriangle
{
    Float3 Vertices[3];
    Float3 Normal;
    Float2 Texcoord;
};

class Terrain : public Resource
{
public:
    /** Navigation areas are used to gather navigation geometry.
    
    NOTE: In the future, we can create a bit mask for each terrain quad to decide which triangles should be used for navigation.
    e.g. TBitMask<> WalkableMask
    */
    TVector<BvAxisAlignedBox> NavigationAreas;

    /** Find ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const;
    /** Find ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TriangleHitResult& HitResult) const;

    bool GetTriangle(float X, float Z, TerrainTriangle& Triangle) const;
};

bool Terrain::Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const
{
    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        bool bCullBackFace;
        int IntersectionCount = 0;

        TVector<TriangleHitResult>* Result;

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) override
        {
            Float3 v0 = btVectorToFloat3(triangle[0]);
            Float3 v1 = btVectorToFloat3(triangle[1]);
            Float3 v2 = btVectorToFloat3(triangle[2]);
            float d, u, v;
            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                TriangleHitResult& hit = Result->Add();
                hit.Location = RayStart + RayDir * d;
                hit.Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                hit.UV.X = u;
                hit.UV.Y = v;
                hit.Distance = d;
                hit.Indices[0] = 0;
                hit.Indices[1] = 0;
                hit.Indices[2] = 0;
                //hit.Material = nullptr;
                IntersectionCount++;
            }
        }
    };

    if (!m_HeightfieldShape)
        return false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, m_BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    Float3 ShapeOffset = Float3(0.0f, (m_MinHeight + m_MaxHeight) * 0.5f, 0.0f);
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart = RayStartLocal;
    triangleRaycastCallback.RayDir = RayDir;
    triangleRaycastCallback.Result = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    m_HeightfieldShape->performRaycast(&triangleRaycastCallback, btVectorToFloat3(RayStartLocal), btVectorToFloat3(RayStartLocal + RayDir * Distance));

    for (int i = HitResult.Size() - triangleRaycastCallback.IntersectionCount; i < HitResult.Size(); i++)
    {
        HitResult[i].Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}

bool Terrain::RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TriangleHitResult& HitResult) const
{
#    define FIRST_INTERSECTION_IS_CLOSEST

    class ATriangleRaycastCallback : public btTriangleCallback
    {
    public:
        Float3 RayStart;
        Float3 RayDir;
        float Distance;
        bool bCullBackFace;
        int IntersectionCount = 0;

        TriangleHitResult* Result;

        void processTriangle(btVector3* triangle, int partId, int triangleIndex) override
        {
#    ifdef FIRST_INTERSECTION_IS_CLOSEST
            if (IntersectionCount == 0)
            {
                Float3 v0 = btVectorToFloat3(triangle[0]);
                Float3 v1 = btVectorToFloat3(triangle[1]);
                Float3 v2 = btVectorToFloat3(triangle[2]);
                float d, u, v;
                if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
                {
                    Result->Location = RayStart + RayDir * d;
                    Result->Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    Result->UV.X = u;
                    Result->UV.Y = v;
                    Result->Distance = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    //Result->Material = nullptr;
                    IntersectionCount++;
                }
            }
#    else
            Float3 v0 = btVectorToFloat3(triangle[0]);
            Float3 v1 = btVectorToFloat3(triangle[1]);
            Float3 v2 = btVectorToFloat3(triangle[2]);
            float d, u, v;
            if (BvRayIntersectTriangle(RayStart, RayDir, v0, v1, v2, d, u, v, bCullBackFace))
            {
                if (d < Distance)
                {
                    Result->Location = RayStart + RayDir * d;
                    Result->Normal = Math::Cross(v1 - v0, v2 - v0).Normalized();
                    Result->UV.X = u;
                    Result->UV.Y = v;
                    Result->Distance = d;
                    Result->Indices[0] = 0;
                    Result->Indices[1] = 0;
                    Result->Indices[2] = 0;
                    Result->Material = nullptr;
                    Distance = d;
                    IntersectionCount++;
                }
            }
#    endif
        }
    };

    if (!m_HeightfieldShape)
        return false;

    float boxMin, boxMax;

    Float3 invRayDir;

    invRayDir.X = 1.0f / RayDir.X;
    invRayDir.Y = 1.0f / RayDir.Y;
    invRayDir.Z = 1.0f / RayDir.Z;

    if (!BvRayIntersectBox(RayStart, invRayDir, m_BoundingBox, boxMin, boxMax) || boxMin >= Distance)
    {
        return false;
    }

    Float3 ShapeOffset = Float3(0.0f, (m_MinHeight + m_MaxHeight) * 0.5f, 0.0f);
    Float3 RayStartLocal = RayStart - ShapeOffset;

    ATriangleRaycastCallback triangleRaycastCallback;
    triangleRaycastCallback.RayStart = RayStartLocal;
    triangleRaycastCallback.RayDir = RayDir;
    triangleRaycastCallback.Distance = Distance;
    triangleRaycastCallback.Result = &HitResult;
    triangleRaycastCallback.bCullBackFace = bCullBackFace;

    m_HeightfieldShape->performRaycast(&triangleRaycastCallback, btVectorToFloat3(RayStartLocal), btVectorToFloat3(RayStartLocal + RayDir * Distance));

    //LOG( "triangleRaycastCallback.IntersectionCount {}\n", triangleRaycastCallback.IntersectionCount );

    if (triangleRaycastCallback.IntersectionCount > 0)
    {
        HitResult.Location += ShapeOffset;
    }

    return triangleRaycastCallback.IntersectionCount > 0;
}


bool Terrain::GetTriangle(float X, float Z, TerrainTriangle& Triangle) const
{
    if (!GetTriangleVertices(X, Z, Triangle.Vertices[0], Triangle.Vertices[1], Triangle.Vertices[2]))
    {
        return false;
    }

    Triangle.Normal = Math::Cross(Triangle.Vertices[1] - Triangle.Vertices[0],
                                  Triangle.Vertices[2] - Triangle.Vertices[0]);
    Triangle.Normal.NormalizeSelf();

    GetTexcoord(X, Z, Triangle.Texcoord);

    return true;
}
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

    /** Set actor to ignore collisions with this component */
    void AddCollisionIgnoreActor(Actor* _Actor);

    /** Unset actor to ignore collisions with this component */
    void RemoveCollisionIgnoreActor(Actor* _Actor);

    void SetQueryGroup(int _UserQueryGroup);

    /** Raycast the terrain */
    bool Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TVector<TriangleHitResult>& Hits) const;

    /** Raycast the terrain */
    bool RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit) const;

    /** Get X,Z coordinates in local terrain space */
    void GetLocalXZ(Float3 const& Position, float& X, float& Z) const;

    /** Get terrain triangle at specified world position */
    bool GetTriangle(Float3 const& Position, TerrainTriangle& Triangle) const;

    float SampleHeight(Float3 const& Position) const;

    void GatherCollisionGeometry(BvAxisAlignedBox const& LocalBounds, TVector<Float3>& CollisionVertices, TVector<unsigned int>& CollisionIndices) const;

    void GatherNavigationGeometry(NavigationGeometry& Geometry) const override;
};
static bool RaycastCallback(PrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TVector<TriangleHitResult>& Hits)
{
    TerrainComponent const* terrain        = static_cast<TerrainComponent const*>(Self->Owner);
    bool                     bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    Terrain* resource = terrain->GetTerrain();
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

#    if 0 // General case
    Float3x3 normalMatrix;
    transform.DecomposeNormalMatrix( normalMatrix );
#    else
    Float3x3 normalMatrix = terrain->GetWorldRotation().ToMatrix3x3();
#    endif

    int numHits = Hits.Size() - firstHit;
    for (int i = 0; i < numHits; i++)
    {
        int                 hitNum    = firstHit + i;
        TriangleHitResult& hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal   = (normalMatrix * hitResult.Normal).Normalized();

        // No need to recalc hit distance
        //hitResult.Distance = (hitResult.Location - InRayStart).Length();
    }

    return true;
}

static bool RaycastClosestCallback(PrimitiveDef const* Self,
                                   Float3 const&        InRayStart,
                                   Float3 const&        InRayEnd,
                                   TriangleHitResult&  Hit,
                                   MeshVertex const**  pVertices)
{
    TerrainComponent const* terrain        = static_cast<TerrainComponent const*>(Self->Owner);
    bool                     bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    Terrain* resource = terrain->GetTerrain();
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

#    if 0 // General case
    Float3x3 normalMatrix;
    TerrainWorldTransform.DecomposeNormalMatrix( normalMatrix );
    Hit.Normal = (normalMatrix * Hit.Normal).Normalized();
#    else
    Hit.Normal = terrain->GetWorldRotation().ToMatrix3x3() * Hit.Normal;
    Hit.Normal.NormalizeSelf();
#    endif

    return true;
}

static void EvaluateRaycastResult(PrimitiveDef*       Self,
                                  Level const*        LightingLevel,
                                  MeshVertex const*   pVertices,
                                  MeshVertexUV const* pLightmapVerts,
                                  int                  LightmapBlock,
                                  unsigned int const*  pIndices,
                                  Float3 const&        HitLocation,
                                  Float2 const&        HitUV,
                                  Float3*              Vertices,
                                  Float2&              TexCoord,
                                  Float3&              LightmapSample)
{
    TerrainComponent* terrain = static_cast<TerrainComponent*>(Self->Owner);

    TerrainTriangle triangle;

    terrain->GetTriangle(HitLocation, triangle);

    Vertices[0]    = triangle.Vertices[0];
    Vertices[1]    = triangle.Vertices[1];
    Vertices[2]    = triangle.Vertices[2];
    TexCoord       = triangle.Texcoord;
    LightmapSample = Float3(0.0f);
}

TerrainComponent::TerrainComponent()
{
    m_HitProxy = NewObj<HitProxy>();

    Primitive                         = VisibilitySystem::AllocatePrimitive();
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

TerrainComponent::~TerrainComponent()
{
    VisibilitySystem::DeallocatePrimitive(Primitive);

    if (m_Terrain)
    {
        m_Terrain->Listeners.Remove(this);
    }
}

void TerrainComponent::SetVisible(bool _Visible)
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

bool TerrainComponent::IsVisible() const
{
    return !!(Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE);
}

void TerrainComponent::SetHiddenInLightPass(bool _HiddenInLightPass)
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

bool TerrainComponent::IsHiddenInLightPass() const
{
    return !(Primitive->QueryGroup & VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS);
}

void TerrainComponent::SetQueryGroup(int _UserQueryGroup)
{
    Primitive->QueryGroup |= VSD_QUERY_MASK(_UserQueryGroup & 0xffff0000);
}

bool TerrainComponent::Raycast(Float3 const& InRayStart, Float3 const& InRayEnd, TVector<TriangleHitResult>& Hits) const
{
    if (!Primitive->RaycastCallback)
    {
        return false;
    }

    Hits.Clear();

    return Primitive->RaycastCallback(Primitive, InRayStart, InRayEnd, Hits);
}

bool TerrainComponent::RaycastClosest(Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit) const
{
    if (!Primitive->RaycastCallback)
    {
        return false;
    }

    return Primitive->RaycastClosestCallback(Primitive, InRayStart, InRayEnd, Hit, nullptr);
}

void TerrainComponent::GetLocalXZ(Float3 const& InPosition, float& X, float& Z) const
{
    // position in terrain space
    Float3 localPosition = TerrainWorldTransformInv * InPosition;

    X = localPosition.X;
    Z = localPosition.Z;
}

bool TerrainComponent::GetTriangle(Float3 const& InPosition, TerrainTriangle& Triangle) const
{
    if (!m_Terrain)
    {
        return false;
    }

    // position in terrain space
    Float3 localPosition = TerrainWorldTransformInv * InPosition;

    if (!m_Terrain->GetTriangle(localPosition.X, localPosition.Z, Triangle))
    {
        return false;
    }

    // Convert triangle to world space
    Triangle.Vertices[0] = TerrainWorldTransform * Triangle.Vertices[0];
    Triangle.Vertices[1] = TerrainWorldTransform * Triangle.Vertices[1];
    Triangle.Vertices[2] = TerrainWorldTransform * Triangle.Vertices[2];

#    if 0 // General case
    Float3x3 normalMatrix;
    TerrainWorldTransform.DecomposeNormalMatrix( normalMatrix );
    Triangle.Normal = (normalMatrix * Triangle.Normal).Normalized();
#    else
    Triangle.Normal = (GetWorldRotation().ToMatrix3x3() * Triangle.Normal).Normalized();
#    endif

    return true;
}

float TerrainComponent::SampleHeight(Float3 const& InPosition) const
{
    if (!m_Terrain)
    {
        return 0.0f;
    }

    float x, z;
    GetLocalXZ(InPosition, x, z);
    return m_Terrain->SampleHeight(x, z);
}

void TerrainComponent::GatherCollisionGeometry(BvAxisAlignedBox const& LocalBounds, TVector<Float3>& CollisionVertices, TVector<unsigned int>& CollisionIndices) const
{
    if (!m_Terrain)
        return;

    int firstVert = CollisionVertices.Size();
    m_Terrain->GatherGeometry(LocalBounds, CollisionVertices, CollisionIndices);

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

void TerrainComponent::GatherNavigationGeometry(NavigationGeometry& Geometry) const
{
    if (!m_Terrain)
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

    TVector<BvAxisAlignedBox> const& areas = m_Terrain->NavigationAreas;

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

        Core::Memcpy(pVertices, srcVertices, vertexCount * sizeof(Float3));

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
#endif
