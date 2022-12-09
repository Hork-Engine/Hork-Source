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

#include "MeshComponent.h"
#include "SkinnedComponent.h"
#include "Actor.h"
#include "World.h"
#include "ResourceManager.h"

#include <Platform/Logger.h>
#include <Geometry/BV/BvIntersect.h>
#include <Core/ConsoleVar.h>

AConsoleVar com_DrawMeshBounds("com_DrawMeshBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawBrushBounds("com_DrawBrushBounds"s, "0"s, CVAR_CHEAT);
AConsoleVar com_DrawIndexedMeshBVH("com_DrawIndexedMeshBVH"s, "0"s, CVAR_CHEAT);

HK_CLASS_META(AMeshComponent)

namespace
{

bool RaycastCallback(SPrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<STriangleHitResult>& Hits)
{
    AMeshComponent const* mesh = static_cast<AMeshComponent const*>(Self->Owner);
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

    AIndexedMesh* resource = mesh->GetMesh();

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

        AIndexedMeshSubpartArray const& subparts = resource->GetSubparts();
        for (int i = 0; i < subparts.Size(); i++)
        {
            int first = Hits.Size();

            // Raycast subpart
            AIndexedMeshSubpart* subpart = subparts[i];
            ret |= subpart->Raycast(rayStartLocal, rayDirLocal, invRayDir, hitDistanceLocal, bCullBackFaces, Hits);

            // Correct material
            int num = Hits.Size() - first;
            if (num)
            {
                AMaterialInstance* material;
                if (views.IsEmpty())
                {
                    static TStaticResourceFinder<AMaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
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
        STriangleHitResult& hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal = (normalMatrix * hitResult.Normal).Normalized();
        hitResult.Distance = (hitResult.Location - InRayStart).Length();
    }

    return true;
}

bool RaycastClosestCallback(SPrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, STriangleHitResult& Hit, SMeshVertex const** pVertices)
{
    AMeshComponent const* mesh = static_cast<AMeshComponent const*>(Self->Owner);
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

    AIndexedMesh* resource = mesh->GetMesh();

    int subpartIndex;

    if (!resource->RaycastClosest(rayStartLocal, rayDirLocal, hitDistanceLocal, bCullBackFaces, Hit.Location, Hit.UV, hitDistanceLocal, Hit.Indices, subpartIndex))
    {
        return false;
    }

    auto& views = mesh->GetRenderViews();
    if (views.IsEmpty())
    {
        static TStaticResourceFinder<AMaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
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

AMeshComponent::AMeshComponent()
{
    DrawableType = DRAWABLE_STATIC_MESH;

    Primitive->RaycastCallback        = RaycastCallback;
    Primitive->RaycastClosestCallback = RaycastClosestCallback;

    bAllowRaycast = true;

    static TStaticResourceFinder<AIndexedMesh> MeshResource("/Default/Meshes/Box"s);
    m_Mesh = MeshResource.GetObject();
    Bounds = m_Mesh->GetBoundingBox();

    SetUseMeshCollision(true);
}

AMeshComponent::~AMeshComponent()
{
    ClearRenderViews();

    m_Mesh->Listeners.Remove(this);
}

void AMeshComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void AMeshComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();
}

void AMeshComponent::SetAllowRaycast(bool _bAllowRaycast)
{
    if (_bAllowRaycast)
    {
        Primitive->RaycastCallback        = RaycastCallback;
        Primitive->RaycastClosestCallback = RaycastClosestCallback;
    }
    else
    {
        Primitive->RaycastCallback        = nullptr;
        Primitive->RaycastClosestCallback = nullptr;
    }
    bAllowRaycast = _bAllowRaycast;
}

void AMeshComponent::SetMesh(AIndexedMesh* mesh)
{
    if (m_Mesh == mesh)
    {
        return;
    }

    m_Mesh->Listeners.Remove(this);

    m_Mesh = mesh;

    for (SceneSocket& socket : m_Sockets)
    {
        socket.SocketDef->RemoveRef();
    }

    if (!m_Mesh)
    {
        static TStaticResourceFinder<AIndexedMesh> MeshResource("/Default/Meshes/Box"s);
        m_Mesh = MeshResource.GetObject();
    }

    m_Mesh->Listeners.Add(this);

    // Update bounding box
    Bounds = m_Mesh->GetBoundingBox();

    // Update sockets
    TVector<ASocketDef*> const& socketDef = m_Mesh->GetSockets();
    m_Sockets.ResizeInvalidate(socketDef.Size());
    for (int i = 0; i < socketDef.Size(); i++)
    {
        socketDef[i]->AddRef();
        m_Sockets[i].SocketDef   = socketDef[i];
        m_Sockets[i].SkinnedMesh = IsSkinnedMesh() ? static_cast<ASkinnedComponent*>(this) : nullptr;
    }

    NotifyMeshChanged();

    // Mark to update world bounds
    UpdateWorldBounds();

    if (ShouldUseMeshCollision())
    {
        UpdatePhysicsAttribs();
    }

    m_RenderTransformMatrixFrame = 0;
}

void AMeshComponent::CopyMaterialsFromMeshResource()
{
    HK_ASSERT(m_Mesh);
    SetRenderView(m_Mesh->GetDefaultRenderView());
}

void AMeshComponent::ClearRenderViews()
{
    for (auto view : m_Views)
        view->RemoveRef();
    m_Views.Clear();
}

void AMeshComponent::SetRenderView(MeshRenderView* renderView)
{
    ClearRenderViews();
    AddRenderView(renderView);
}

void AMeshComponent::AddRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    if (m_Views.AddUnique(renderView))
        renderView->AddRef();
}

void AMeshComponent::RemoveRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    auto i = m_Views.IndexOf(renderView);
    if (i != Core::NPOS)
    {
        m_Views.Remove(i);
        renderView->RemoveRef();
    }
}

BvAxisAlignedBox AMeshComponent::GetSubpartWorldBounds(int subpartIndex) const
{
    AIndexedMeshSubpart const* subpart = const_cast<AMeshComponent*>(this)->m_Mesh->GetSubpart(subpartIndex);
    if (!subpart)
    {
        LOG("AMeshComponent::GetSubpartWorldBounds: invalid subpart index\n");
        return BvAxisAlignedBox::Empty();
    }
    return subpart->GetBoundingBox().Transform(GetWorldTransformMatrix());
}

ACollisionModel* AMeshComponent::GetMeshCollisionModel() const
{
    return m_Mesh->GetCollisionModel();
}

void AMeshComponent::NotifyMeshChanged()
{
    OnMeshChanged();
}

void AMeshComponent::OnMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag)
{
    TRef<AIndexedMesh> curMesh = m_Mesh;

    // Reset mesh
    SetMesh(nullptr);
    SetMesh(curMesh);
}

void AMeshComponent::OnPreRenderUpdate(SRenderFrontendDef const* _Def)
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

void AMeshComponent::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawIndexedMeshBVH)
    {
        if (Primitive->VisPass == InRenderer->GetVisPass())
        {
            m_Mesh->DrawBVH(InRenderer, GetWorldTransformMatrix());
        }
    }

    if (com_DrawMeshBounds)
    {
        if (Primitive->VisPass == InRenderer->GetVisPass())
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

            InRenderer->DrawAABB(WorldBounds);
        }
    }
}

#if 0
HK_CLASS_META( ABrushComponent )

static bool BrushRaycastCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodVector< STriangleHitResult > & Hits ) {
    ABrushComponent const * brush = static_cast< ABrushComponent const * >(Self->Owner);

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

    AIndexedMesh * resource = mesh->GetMesh();

    return resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, Hits );
#    endif

    HK_UNUSED( brush );

    LOG( "BrushRaycastCallback: todo\n" );
    return false;
}

static bool BrushRaycastClosestCallback( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, SMeshVertex const ** pVertices, unsigned int Indices[3], TRef< AMaterialInstance > & Material ) {
    ABrushComponent const * brush = static_cast< ABrushComponent const * >(Self->Owner);

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

    AIndexedMesh * resource = mesh->GetMesh();

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

ABrushComponent::ABrushComponent() {
    primitive->RaycastCallback = BrushRaycastCallback;
    primitive->RaycastClosestCallback = BrushRaycastClosestCallback;
}

void ABrushComponent::DrawDebug( ADebugRenderer * InRenderer ) {
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






HK_CLASS_META(AProceduralMeshComponent)

static bool RaycastCallback_Procedural(SPrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, TPodVector<STriangleHitResult>& Hits)
{
    AProceduralMeshComponent const* mesh           = static_cast<AProceduralMeshComponent const*>(Self->Owner);
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

    AProceduralMesh* resource = mesh->GetMesh();
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

    AMaterialInstance* material;

    auto& views = mesh->GetRenderViews();
    if (views.IsEmpty())
    {
        static TStaticResourceFinder<AMaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
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
        STriangleHitResult& hitResult = Hits[hitNum];

        hitResult.Location = transform * hitResult.Location;
        hitResult.Normal   = (normalMatrix * hitResult.Normal).Normalized();
        hitResult.Distance = (hitResult.Location - InRayStart).Length();
        hitResult.Material = material;
    }

    return true;
}

static bool RaycastClosestCallback_Procedural(SPrimitiveDef const* Self, Float3 const& InRayStart, Float3 const& InRayEnd, STriangleHitResult& Hit, SMeshVertex const** pVertices)
{
    AProceduralMeshComponent const* mesh           = static_cast<AProceduralMeshComponent const*>(Self->Owner);
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

    AProceduralMesh* resource = mesh->GetMesh();
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
        static TStaticResourceFinder<AMaterialInstance> DefaultInstance("/Default/MaterialInstance/Default"s);
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

AProceduralMeshComponent::AProceduralMeshComponent()
{
    DrawableType = DRAWABLE_PROCEDURAL_MESH;

    Primitive->RaycastCallback        = RaycastCallback_Procedural;
    Primitive->RaycastClosestCallback = RaycastClosestCallback_Procedural;

    bAllowRaycast = true;

    //LightmapOffset.Z = LightmapOffset.W = 1;

    //static TStaticResourceFinder< AIndexedMesh > MeshResource("/Default/Meshes/Box"s);
    //m_Mesh = MeshResource.GetObject();
    //Bounds = m_Mesh->GetBoundingBox();
}

AProceduralMeshComponent::~AProceduralMeshComponent()
{
    ClearRenderViews();
}

void AProceduralMeshComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void AProceduralMeshComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();
}

void AProceduralMeshComponent::SetAllowRaycast(bool _bAllowRaycast)
{
    if (_bAllowRaycast)
    {
        Primitive->RaycastCallback        = RaycastCallback_Procedural;
        Primitive->RaycastClosestCallback = RaycastClosestCallback_Procedural;
    }
    else
    {
        Primitive->RaycastCallback        = nullptr;
        Primitive->RaycastClosestCallback = nullptr;
    }
    bAllowRaycast = _bAllowRaycast;
}

void AProceduralMeshComponent::DrawDebug(ADebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawMeshBounds)
    {
        if (Primitive->VisPass == InRenderer->GetVisPass())
        {
            InRenderer->SetDepthTest(false);
            InRenderer->SetColor(Color4(0.5f, 1, 0.5f, 1));
            InRenderer->DrawAABB(WorldBounds);
        }
    }
}

void AProceduralMeshComponent::SetMesh(AProceduralMesh* mesh)
{
    m_Mesh = mesh;
}

AProceduralMesh* AProceduralMeshComponent::GetMesh() const
{
    return m_Mesh;
}

void AProceduralMeshComponent::ClearRenderViews()
{
    for (auto view : m_Views)
        view->RemoveRef();
    m_Views.Clear();
}

void AProceduralMeshComponent::SetRenderView(MeshRenderView* renderView)
{
    ClearRenderViews();
    AddRenderView(renderView);
}

void AProceduralMeshComponent::AddRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    if (m_Views.AddUnique(renderView))
        renderView->AddRef();
}

void AProceduralMeshComponent::RemoveRenderView(MeshRenderView* renderView)
{
    HK_ASSERT(renderView);

    auto i = m_Views.IndexOf(renderView);
    if (i != Core::NPOS)
    {
        m_Views.Remove(i);
        renderView->RemoveRef();
    }
}

void AProceduralMeshComponent::OnPreRenderUpdate(SRenderFrontendDef const* _Def)
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
