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

#include "MeshComponent.h"
#include "SkinnedComponent.h"
#include "World.h"
#include <Engine/Runtime/ResourceManager.h>

#include <Engine/Core/Platform/Logger.h>
#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawMeshBounds("com_DrawMeshBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawBrushBounds("com_DrawBrushBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawIndexedMeshBVH("com_DrawIndexedMeshBVH"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(MeshComponent)

namespace
{

bool RaycastCallback(PrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<TriangleHitResult>& Hits)
{
    MeshComponent const* mesh = static_cast<MeshComponent const*>(Self->Owner);
    bool bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if (hitDistanceLocal < 0.0001f)
    {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    IndexedMesh* resource = mesh->GetMesh();

    int firstHit = Hits.Size();

    //if (mesh->bOverrideMeshMaterials)
    {
        bool ret = false;

        float boxMin, boxMax;

        Float3 invRayDir;

        invRayDir.X = 1.0f / rayDirLocal.X;
        invRayDir.Y = 1.0f / rayDirLocal.Y;
        invRayDir.Z = 1.0f / rayDirLocal.Z;

        if (!BvRayIntersectBox(rayStartLocal, invRayDir, resource->GetBoundingBox(), boxMin, boxMax) || boxMin >= hitDistanceLocal)
        {
            return false;
        }

        auto& views = mesh->GetRenderViews();

        IndexedMeshSubpartArray const& subparts = resource->GetSubparts();
        for (int i = 0; i < subparts.Size(); i++)
        {
            int first = Hits.Size();

            // Raycast subpart
            IndexedMeshSubpart* subpart = subparts[i];
            ret |= subpart->Raycast(rayStartLocal, rayDirLocal, invRayDir, hitDistanceLocal, bCullBackFaces, Hits);

            // Correct material
            int num = Hits.Size() - first;
            if (num)
            {
                MaterialInstance* material;
                if (views.IsEmpty())
                {
                    static TStaticResourceFinder<MaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
                    material = DefaultInstance.GetObject();
                }
                else
                {
                    material = views[0]->GetMaterial(i);
                }

                for (int j = 0; j < num; j++)
                {
                    Hits[first + j].Material = material;
                }
            }
        }

        if (!ret)
        {
            return false;
        }
    }
    //else
    //{
    //    if (!resource->Raycast(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hits))
    //    {
    //        return false;
    //    }
    //}

    // Convert hits to worldspace

    Float3x4 const& transform = mesh->GetWorldTransformMatrix();
    Float3x3 normalMatrix;

    transform.DecomposeNormalMatrix(normalMatrix);

    int numHits = Hits.Size() - firstHit;
    for (int i = 0; i < numHits; i++)
    {
        int hitNum = firstHit + i;
        TriangleHitResult& hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal = (normalMatrix * hitResult.Normal).Normalized();
        hitResult.Distance = (hitResult.Location - InRayStart).Length();
    }

    return true;
}

bool RaycastClosestCallback(PrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit, MeshVertex const** pVertices)
{
    MeshComponent const* mesh = static_cast<MeshComponent const*>(Self->Owner);
    bool bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if (hitDistanceLocal < 0.0001f)
    {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    IndexedMesh* resource = mesh->GetMesh();

    int subpartIndex;

    if (!resource->RaycastClosest(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hit.Location, Hit.UV, hitDistanceLocal, Hit.Indices, subpartIndex))
    {
        return false;
    }

    auto& views = mesh->GetRenderViews();
    if (views.IsEmpty())
    {
        static TStaticResourceFinder<MaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
        Hit.Material = DefaultInstance.GetObject();
    }
    else
    {
        Hit.Material = views[0]->GetMaterial(subpartIndex);
    }

    *pVertices = resource->GetVertices();

    Float3x4 const& transform = mesh->GetWorldTransformMatrix();

    // Transform hit location to world space
    Hit.Location = transform * Hit.Location;

    // Recalc hit distance in world space
    Hit.Distance = (Hit.Location - InRayStart).Length();

    Float3 const& v0 = (*pVertices)[Hit.Indices[0]].Position;
    Float3 const& v1 = (*pVertices)[Hit.Indices[1]].Position;
    Float3 const& v2 = (*pVertices)[Hit.Indices[2]].Position;

    // calc triangle vertices
    Float3 tv0 = transform * v0;
    Float3 tv1 = transform * v1;
    Float3 tv2 = transform * v2;

    // calc normal
    Hit.Normal = Math::Cross(tv1 - tv0, tv2 - tv0).Normalized();

    return true;
}

} // namespace

MeshComponent::MeshComponent()
{
    m_DrawableType = DRAWABLE_STATIC_MESH;

    m_Primitive->RaycastCallback = RaycastCallback;
    m_Primitive->RaycastClosestCallback = RaycastClosestCallback;

    m_bAllowRaycast = true;

    static TStaticResourceFinder<IndexedMesh> MeshResource("/Default/Meshes/Box"s);
    m_Mesh = MeshResource.GetObject();
    m_Bounds = m_Mesh->GetBoundingBox();

    SetUseMeshCollision(true);
}

MeshComponent::~MeshComponent()
{
    ClearRenderViews();

    m_Mesh->Listeners.Remove(this);
}

void MeshComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void MeshComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();
}

void MeshComponent::SetAllowRaycast(bool _bAllowRaycast)
{
    if (_bAllowRaycast)
    {
        m_Primitive->RaycastCallback = RaycastCallback;
        m_Primitive->RaycastClosestCallback = RaycastClosestCallback;
    }
    else
    {
        m_Primitive->RaycastCallback = nullptr;
        m_Primitive->RaycastClosestCallback = nullptr;
    }
    m_bAllowRaycast = _bAllowRaycast;
}

void MeshComponent::SetMesh(IndexedMesh* mesh)
{
    if (m_Mesh == mesh)
    {
        return;
    }

    m_Mesh->Listeners.Remove(this);

    m_Mesh = mesh;

    if (!m_Mesh)
    {
        static TStaticResourceFinder<IndexedMesh> MeshResource("/Default/Meshes/Box"s);
        m_Mesh = MeshResource.GetObject();
    }

    m_Mesh->Listeners.Add(this);

    UpdateMesh();
}

void MeshComponent::UpdateMesh()
{
    // Update bounding box
    m_Bounds = m_Mesh->GetBoundingBox();

    // Update sockets
    for (SceneSocket& socket : m_Sockets)
        socket.Definition->RemoveRef();

    TVector<SocketDef*> const& socketDef = m_Mesh->GetSockets();
    m_Sockets.ResizeInvalidate(socketDef.Size());
    for (int i = 0; i < socketDef.Size(); i++)
    {
        socketDef[i]->AddRef();
        m_Sockets[i].Definition = socketDef[i];
        m_Sockets[i].SkinnedMesh = IsSkinnedMesh() ? static_cast<SkinnedComponent*>(this) : nullptr;
    }

    // Mark to update world bounds
    UpdateWorldBounds();

    if (ShouldUseMeshCollision())
    {
        UpdatePhysicsAttribs();
    }

    m_RenderTransformMatrixFrame = 0;
}

void MeshComponent::CopyMaterialsFromMeshResource()
{
    HK_ASSERT(m_Mesh);
    SetRenderView(m_Mesh->GetDefaultRenderView());
}

void MeshComponent::ClearRenderViews()
{
    for (auto view : m_Views)
        view->RemoveRef();
    m_Views.Clear();
}

void MeshComponent::SetRenderView(MeshRenderView* renderView)
{
    ClearRenderViews();
    AddRenderView(renderView);
}

void MeshComponent::AddRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    if (m_Views.AddUnique(renderView))
        renderView->AddRef();
}

void MeshComponent::RemoveRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    auto i = m_Views.IndexOf(renderView);
    if (i != Core::NPOS)
    {
        m_Views.Remove(i);
        renderView->RemoveRef();
    }
}

BvAxisAlignedBox MeshComponent::GetSubpartWorldBounds(int subpartIndex) const
{
    IndexedMeshSubpart const* subpart = const_cast<MeshComponent*>(this)->m_Mesh->GetSubpart(subpartIndex);
    if (!subpart)
    {
        LOG("MeshComponent::GetSubpartWorldBounds: invalid subpart index\n");
        return BvAxisAlignedBox::Empty();
    }
    return subpart->GetBoundingBox().Transform(GetWorldTransformMatrix());
}

CollisionModel* MeshComponent::GetMeshCollisionModel() const
{
    return m_Mesh->GetCollisionModel();
}

void MeshComponent::OnMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag)
{
    if (UpdateFlag == INDEXED_MESH_UPDATE_BOUNDING_BOX)
    {
        m_Bounds = m_Mesh->GetBoundingBox();
        UpdateWorldBounds();
    }
    else
    {
        UpdateMesh();
    }
}

void MeshComponent::OnPreRenderUpdate(RenderFrontendDef const* _Def)
{
    Super::OnPreRenderUpdate(_Def);

    if (m_RenderTransformMatrixFrame == _Def->FrameNumber)
        return;

    if (m_RenderTransformMatrixFrame == 0 || m_RenderTransformMatrixFrame + 1 != _Def->FrameNumber)
    {
        int i = _Def->FrameNumber;

        Float3x4 const& transform = GetWorldTransformMatrix();

        m_RenderTransformMatrix[i & 1]       = transform;
        m_RenderTransformMatrix[(i + 1) & 1] = transform;

        m_RenderTransformMatrixFrame = i;
        return;
    }

    m_RenderTransformMatrixFrame = _Def->FrameNumber;

    m_RenderTransformMatrix[m_RenderTransformMatrixFrame & 1] = GetWorldTransformMatrix();
}

void MeshComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawIndexedMeshBVH)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            m_Mesh->DrawBVH(InRenderer, GetWorldTransformMatrix());
        }
    }

    if (com_DrawMeshBounds)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            InRenderer->SetDepthTest(false);

            if (IsSkinnedMesh())
            {
                InRenderer->SetColor(Color4(0.5f, 0.5f, 1, 1));
            }
            else
            {
                InRenderer->SetColor(Color4(1, 1, 1, 1));
            }

            InRenderer->DrawAABB(m_WorldBounds);
        }
    }
}

#if 0
HK_CLASS_META( BrushComponent )

static bool BrushRaycastCallback( PrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodVector< TriangleHitResult > & Hits ) {
    BrushComponent const * brush = static_cast< BrushComponent const * >(Self->Owner);

#    if 0
    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * InRayEnd;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    IndexedMesh * resource = mesh->GetMesh();

    return resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, Hits );
#    endif

    HK_UNUSED( brush );

    LOG( "BrushRaycastCallback: todo\n" );
    return false;
}

static bool BrushRaycastClosestCallback( PrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, MeshVertex const ** pVertices, unsigned int Indices[3], TRef< MaterialInstance > & Material ) {
    BrushComponent const * brush = static_cast< BrushComponent const * >(Self->Owner);

#    if 0
    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

    // transform ray to object space
    Float3 rayStartLocal = transformInverse * InRayStart;
    Float3 rayEndLocal = transformInverse * HitLocation;
    Float3 rayDirLocal = rayEndLocal - rayStartLocal;

    float hitDistanceLocal = rayDirLocal.Length();
    if ( hitDistanceLocal < 0.0001f ) {
        return false;
    }

    rayDirLocal /= hitDistanceLocal;

    IndexedMesh * resource = mesh->GetMesh();

    if ( !resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, HitLocation, HitUV, HitDistance, Indices, Material ) ) {
        return false;
    }

    *pVertices = resource->GetVertices();

    return true;

#    endif

    HK_UNUSED( brush );

    LOG( "BrushRaycastClosestCallback: todo\n" );
    return false;
}

BrushComponent::BrushComponent() {
    primitive->RaycastCallback = BrushRaycastCallback;
    primitive->RaycastClosestCallback = BrushRaycastClosestCallback;
}

void BrushComponent::DrawDebug( DebugRenderer * InRenderer ) {
    Super::DrawDebug( InRenderer );

    if ( RVDrawBrushBounds )
    {
        if ( primitive->VisPass == InRenderer->GetVisPass() )
        {
            InRenderer->SetDepthTest( false );
            InRenderer->SetColor( Color4( 1, 0.5f, 0.5f, 1 ) );
            InRenderer->DrawAABB( WorldBounds );
        }
    }
}
#endif






HK_CLASS_META(ProceduralMeshComponent)

static bool RaycastCallback_Procedural(PrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<TriangleHitResult>& Hits)
{
    ProceduralMeshComponent const* mesh           = static_cast<ProceduralMeshComponent const*>(Self->Owner);
    bool                            bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

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

    ProceduralMesh* resource = mesh->GetMesh();
    if (!resource)
    {
        // No resource associated with procedural mesh component
        return false;
    }

    int firstHit = Hits.Size();

    if (!resource->Raycast(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hits))
    {
        return false;
    }

    MaterialInstance* material;

    auto& views = mesh->GetRenderViews();
    if (views.IsEmpty())
    {
        static TStaticResourceFinder<MaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
        material = DefaultInstance.GetObject();
    }
    else
    {
        material = views[0]->GetMaterial(0);
    }

    // Convert hits to worldspace

    Float3x4 const& transform = mesh->GetWorldTransformMatrix();
    Float3x3        normalMatrix;

    transform.DecomposeNormalMatrix(normalMatrix);

    int numHits = Hits.Size() - firstHit;

    for (int i = 0; i < numHits; i++)
    {
        int                 hitNum    = firstHit + i;
        TriangleHitResult& hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal   = (normalMatrix * hitResult.Normal).Normalized();
        hitResult.Distance = (hitResult.Location - InRayStart).Length();
        hitResult.Material = material;
    }

    return true;
}

static bool RaycastClosestCallback_Procedural(PrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TriangleHitResult& Hit, MeshVertex const** pVertices)
{
    ProceduralMeshComponent const* mesh           = static_cast<ProceduralMeshComponent const*>(Self->Owner);
    bool                            bCullBackFaces = !(Self->Flags & SURF_TWOSIDED);

    Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

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

    ProceduralMesh* resource = mesh->GetMesh();
    if (!resource)
    {
        // No resource associated with procedural mesh component
        return false;
    }

    if (!resource->RaycastClosest(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hit.Location, Hit.UV, hitDistanceLocal, Hit.Indices))
    {
        return false;
    }

    auto& views = mesh->GetRenderViews();
    if (views.IsEmpty())
    {
        static TStaticResourceFinder<MaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
        Hit.Material = DefaultInstance.GetObject();
    }
    else
    {
        Hit.Material = views[0]->GetMaterial(0);
    }

    *pVertices = resource->VertexCache.ToPtr();

    Float3x4 const& transform = mesh->GetWorldTransformMatrix();

    // Transform hit location to world space
    Hit.Location = transform * Hit.Location;

    // Recalc hit distance in world space
    Hit.Distance = (Hit.Location - InRayStart).Length();

    Float3 const& v0 = (*pVertices)[Hit.Indices[0]].Position;
    Float3 const& v1 = (*pVertices)[Hit.Indices[1]].Position;
    Float3 const& v2 = (*pVertices)[Hit.Indices[2]].Position;

    // calc triangle vertices
    Float3 tv0 = transform * v0;
    Float3 tv1 = transform * v1;
    Float3 tv2 = transform * v2;

    // calc normal
    Hit.Normal = Math::Cross(tv1 - tv0, tv2 - tv0).Normalized();

    return true;
}

ProceduralMeshComponent::ProceduralMeshComponent()
{
    m_DrawableType = DRAWABLE_PROCEDURAL_MESH;

    m_Primitive->RaycastCallback = RaycastCallback_Procedural;
    m_Primitive->RaycastClosestCallback = RaycastClosestCallback_Procedural;

    m_bAllowRaycast = true;

    //LightmapOffset.Z = LightmapOffset.W = 1;

    //static TStaticResourceFinder< IndexedMesh > MeshResource("/Default/Meshes/Box"s);
    //m_Mesh = MeshResource.GetObject();
    //Bounds = m_Mesh->GetBoundingBox();
}

ProceduralMeshComponent::~ProceduralMeshComponent()
{
    ClearRenderViews();
}

void ProceduralMeshComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void ProceduralMeshComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();
}

void ProceduralMeshComponent::SetAllowRaycast(bool _bAllowRaycast)
{
    if (_bAllowRaycast)
    {
        m_Primitive->RaycastCallback = RaycastCallback_Procedural;
        m_Primitive->RaycastClosestCallback = RaycastClosestCallback_Procedural;
    }
    else
    {
        m_Primitive->RaycastCallback = nullptr;
        m_Primitive->RaycastClosestCallback = nullptr;
    }
    m_bAllowRaycast = _bAllowRaycast;
}

void ProceduralMeshComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawMeshBounds)
    {
        if (m_Primitive->VisPass == InRenderer->GetVisPass())
        {
            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(0.5f, 1, 0.5f, 1));
            InRenderer->DrawAABB(m_WorldBounds);
        }
    }
}

void ProceduralMeshComponent::SetMesh(ProceduralMesh* mesh)
{
    m_Mesh = mesh;
}

ProceduralMesh* ProceduralMeshComponent::GetMesh() const
{
    return m_Mesh;
}

void ProceduralMeshComponent::ClearRenderViews()
{
    for (auto view : m_Views)
        view->RemoveRef();
    m_Views.Clear();
}

void ProceduralMeshComponent::SetRenderView(MeshRenderView* renderView)
{
    ClearRenderViews();
    AddRenderView(renderView);
}

void ProceduralMeshComponent::AddRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    if (m_Views.AddUnique(renderView))
        renderView->AddRef();
}

void ProceduralMeshComponent::RemoveRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    auto i = m_Views.IndexOf(renderView);
    if (i != Core::NPOS)
    {
        m_Views.Remove(i);
        renderView->RemoveRef();
    }
}

void ProceduralMeshComponent::OnPreRenderUpdate(RenderFrontendDef const* _Def)
{
    Super::OnPreRenderUpdate(_Def);

    if (m_RenderTransformMatrixFrame == _Def->FrameNumber)
        return;

    if (m_RenderTransformMatrixFrame == 0 || m_RenderTransformMatrixFrame + 1 != _Def->FrameNumber)
    {
        int i = _Def->FrameNumber;

        Float3x4 const& transform = GetWorldTransformMatrix();

        m_RenderTransformMatrix[i & 1]     = transform;
        m_RenderTransformMatrix[(i + 1) & 1] = transform;

        m_RenderTransformMatrixFrame = i;
        return;
    }

    m_RenderTransformMatrixFrame = _Def->FrameNumber;

    m_RenderTransformMatrix[m_RenderTransformMatrixFrame & 1] = GetWorldTransformMatrix();
}

HK_NAMESPACE_END
