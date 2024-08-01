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

#include "NavMeshInterface.h"

#include <Engine/Core/Logger.h>
#include <Engine/Core/Allocators/LinearAllocator.h>
#include <Engine/Core/Compress.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Core/Containers/ArrayView.h>
#include <Engine/Core/Containers/BitMask.h>
#include <Engine/Geometry/BV/BvIntersect.h>

#include <Engine/World/Modules/NavMesh/Components/NavMeshObstacleComponent.h>
#include <Engine/World/Modules/NavMesh/Components/NavMeshAreaComponent.h>
#include <Engine/World/Modules/NavMesh/Components/OffMeshLinkComponent.h>
#include <Engine/World/Modules/Physics/Components/StaticBodyComponent.h>
#include <Engine/World/Modules/Physics/Components/HeightFieldComponent.h>

#include <Engine/GameApplication/GameApplication.h>

#include <Recast/Recast.h>
#include <Detour/DetourNavMesh.h>
#include <Detour/DetourNavMeshQuery.h>
#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourCommon.h>
#include <Detour/DetourCrowd.h>
#include <Detour/DetourTileCache.h>
#include <Detour/DetourTileCacheBuilder.h>
#include <Detour/RecastDebugDraw.h>
#include <Detour/DetourDebugDraw.h>
#include <Detour/DebugDraw.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawNavMeshBVTree("com_DrawNavMeshBVTree"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMeshNodes("com_DrawNavMeshNodes"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMesh("com_DrawNavMesh"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMeshTileBounds("com_DrawNavMeshTileBounds"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawOffMeshLinks("com_DrawOffMeshLinks"s, "0"s, CVAR_CHEAT);

HK_VALIDATE_TYPE_SIZE(NavPolyRef, sizeof(dtPolyRef));

namespace
{

const bool RECAST_ENABLE_LOGGING = true;
const bool RECAST_ENABLE_TIMINGS = true;

const int MAX_POLYS = 2048;

NavPolyRef          TmpPolys[MAX_POLYS];
NavPolyRef          TmpPathPolys[MAX_POLYS];
alignas(16) Float3  TmpPathPoints[MAX_POLYS];
unsigned char       TmpPathFlags[MAX_POLYS];

String GetErrorStr(dtStatus status)
{
    String s;
    if (status & DT_WRONG_MAGIC)
        s += "DT_WRONG_MAGIC ";
    if (status & DT_WRONG_VERSION)
        s += "DT_WRONG_VERSION ";
    if (status & DT_OUT_OF_MEMORY)
        s += "DT_OUT_OF_MEMORY ";
    if (status & DT_INVALID_PARAM)
        s += "DT_INVALID_PARAM ";
    if (status & DT_BUFFER_TOO_SMALL)
        s += "DT_BUFFER_TOO_SMALL ";
    if (status & DT_OUT_OF_NODES)
        s += "DT_OUT_OF_NODES ";
    if (status & DT_PARTIAL_RESULT)
        s += "DT_PARTIAL_RESULT ";
    if (!s.IsEmpty())
        s.Resize(s.Size() - 1);
    return s;
}

struct TileCompressorCallback final : public dtTileCacheCompressor
{
    int maxCompressedSize(const int bufferSize) override
    {
        return Core::FastLZMaxCompressedSize(bufferSize);
    }

    dtStatus compress(const unsigned char* buffer, const int bufferSize, unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) override
    {
        size_t size;
        *compressedSize = 0;
        if (!Core::FastLZCompress(compressed, &size, buffer, bufferSize))
            return DT_FAILURE;
        *compressedSize = size;
        return DT_SUCCESS;
    }

    dtStatus decompress(const unsigned char* compressed, const int compressedSize, unsigned char* buffer, const int maxBufferSize, int* bufferSize) override
    {
        size_t size;
        *bufferSize = 0;
        if (!Core::FastLZDecompress(compressed, compressedSize, buffer, &size, maxBufferSize))
            return DT_FAILURE;
        *bufferSize = size;
        return DT_SUCCESS;
    }
};

TileCompressorCallback s_TileCompressorCallback;

} // namespace

class NavigationGeometry final
{
public:
    void Clear()
    {
        m_Vertices.Clear();
        m_BoundingBox.Clear();
        m_CropBoxes.Clear();
        m_MaxCropBox.Clear();
    }

    void AddCropBox(BvAxisAlignedBox const& box)
    {
        m_CropBoxes.Add(box);
        m_MaxCropBox.AddAABB(box);
    }

    void AddTriangle(Float3 const& p0, Float3 const& p1, Float3 const& p2)
    {
        BvAxisAlignedBox triangleBounds;

        triangleBounds.Mins.X = Math::Min3(p0.X, p1.X, p2.X);
        triangleBounds.Mins.Y = Math::Min3(p0.Y, p1.Y, p2.Y);
        triangleBounds.Mins.Z = Math::Min3(p0.Z, p1.Z, p2.Z);

        triangleBounds.Maxs.X = Math::Max3(p0.X, p1.X, p2.X);
        triangleBounds.Maxs.Y = Math::Max3(p0.Y, p1.Y, p2.Y);
        triangleBounds.Maxs.Z = Math::Max3(p0.Z, p1.Z, p2.Z);

        for (BvAxisAlignedBox const& cropBox : m_CropBoxes)
        {
            // Simple fast triangle - AABB overlap test
            if (BvBoxOverlapBox(cropBox, triangleBounds))
            {
                m_Vertices.Add(p0);
                m_Vertices.Add(p1);
                m_Vertices.Add(p2);

                m_BoundingBox.AddAABB(triangleBounds);
            }
        }
    }

    void AddTriangleSoup(ArrayView<Float3> vertices, ArrayView<uint32_t> indices)
    {
        HK_ASSERT((indices.Size() % 3) == 0);
        for (uint32_t i = 0, count = indices.Size(); i < count; i += 3)
            AddTriangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
    }

    void Finalize()
    {
        // Shrink bounding box to clipping box

        for (int i = 0; i < 3; ++i)
        {
            if (m_BoundingBox.Mins[i] < m_MaxCropBox.Mins[i])
                m_BoundingBox.Mins[i] = m_MaxCropBox.Mins[i];
            if (m_BoundingBox.Maxs[i] > m_MaxCropBox.Maxs[i])
                m_BoundingBox.Maxs[i] = m_MaxCropBox.Maxs[i];
        }
    }

    bool IsEmpty() const
    {
        return m_Vertices.IsEmpty() || m_BoundingBox.IsEmpty();
    }

    BvAxisAlignedBox const& GetMaxCropBox() const { return m_MaxCropBox; }

    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    Vector<Float3> const& GetVertices() const { return m_Vertices; }

private:
    Vector<Float3>      m_Vertices;
    BvAxisAlignedBox    m_BoundingBox = BvAxisAlignedBox::Empty();
    BvAxisAlignedBox    m_MaxCropBox  = BvAxisAlignedBox::Empty();
    SmallVector<BvAxisAlignedBox, 8> m_CropBoxes;
};

struct DetourMeshProcess final : public dtTileCacheMeshProcess
{
    NavMeshInterface*       m_NavMeshInterface;
    Vector<Float3>          m_OffMeshConVerts;
    Vector<float>           m_OffMeshConRads;
    Vector<uint8_t>         m_OffMeshConDirs;
    Vector<uint8_t>         m_OffMeshConAreas;
    Vector<uint16_t>        m_OffMeshConFlags;
    Vector<uint32_t>        m_OffMeshConID;
    int                     m_OffMeshConCount;
    BvAxisAlignedBox        m_ClipBounds;

    void* operator new(size_t sizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(sizeInBytes);
    }

    void operator delete(void* ptr)
    {
        Core::GetHeapAllocator<HEAP_NAVIGATION>().Free(ptr);
    }

    void process(struct dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags) override
    {
        for (int i = 0; i < params->polyCount; ++i)
        {
            if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
                polyAreas[i] = NAV_MESH_AREA_GROUND;

#if 0
            if (polyAreas[i] == NAV_MESH_AREA_GROUND || polyAreas[i] == NAV_MESH_AREA_GRASS || polyAreas[i] == NAV_MESH_AREA_ROAD)
            {
                polyFlags[i] = NAV_MESH_FLAGS_WALK;
            }
            else if (polyAreas[i] == NAV_MESH_AREA_WATER)
            {
                polyFlags[i] = NAV_MESH_FLAGS_SWIM;
            }
            else if (polyAreas[i] == NAV_MESH_AREA_DOOR)
            {
                polyFlags[i] = NAV_MESH_FLAGS_WALK | NAV_MESH_FLAGS_DOOR;
            }
#endif
        }

        rcVcopy(m_ClipBounds.Mins.ToPtr(), params->bmin);
        rcVcopy(m_ClipBounds.Maxs.ToPtr(), params->bmax);

        m_OffMeshConVerts.Clear();
        m_OffMeshConVerts.Clear();
        m_OffMeshConRads.Clear();
        m_OffMeshConDirs.Clear();
        m_OffMeshConAreas.Clear();
        m_OffMeshConFlags.Clear();
        m_OffMeshConID.Clear();
        m_OffMeshConCount = 0;

        auto& links = m_NavMeshInterface->GetWorld()->GetComponentManager<OffMeshLinkComponent>();
        links.IterateComponents(*this);

        // Pass in off-mesh connections.
        params->offMeshConVerts = (float*)m_OffMeshConVerts.ToPtr();
        params->offMeshConRad = m_OffMeshConRads.ToPtr();
        params->offMeshConDir = m_OffMeshConDirs.ToPtr();
        params->offMeshConAreas = m_OffMeshConAreas.ToPtr();
        params->offMeshConFlags = m_OffMeshConFlags.ToPtr();
        params->offMeshConUserID = m_OffMeshConID.ToPtr();
        params->offMeshConCount = m_OffMeshConCount;
    }

    void Visit(OffMeshLinkComponent& component)
    {
        auto destination = m_NavMeshInterface->GetWorld()->GetObject(component.GetDestination());
        if (!destination)
            return;

        Float3 startPoint = component.GetOwner()->GetWorldPosition();
        Float3 endPoint = destination->GetWorldPosition();

        const float MARGIN = 0.2f;

        BvAxisAlignedBox linkBounds =
        {
            {Math::Min(startPoint.X, endPoint.X), Math::Min(startPoint.Y, endPoint.Y), Math::Min(startPoint.Z, endPoint.Z)},
            {Math::Max(startPoint.X, endPoint.X), Math::Max(startPoint.Y, endPoint.Y), Math::Max(startPoint.Z, endPoint.Z)}
        };

        linkBounds.Mins -= MARGIN;
        linkBounds.Maxs += MARGIN;

        if (!BvBoxOverlapBox(m_ClipBounds, linkBounds))
        {
            // Connection is outside of clip bounds
            return;
        }

        m_OffMeshConVerts.Add(startPoint);
        m_OffMeshConVerts.Add(endPoint);
        m_OffMeshConRads.Add(component.GetRadius());
        m_OffMeshConDirs.Add(component.IsBidirectional() ? DT_OFFMESH_CON_BIDIR : 0);
        m_OffMeshConAreas.Add(component.GetAreaType());
        m_OffMeshConFlags.Add(0);
        m_OffMeshConID.Add(component.GetHandle().ToUInt32());

        m_OffMeshConCount++;
    }
};

struct DetourLinearAllocator final : public dtTileCacheAlloc
{
    LinearAllocator<> Allocator;

    void* operator new(size_t sizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(sizeInBytes);
    }

    void operator delete(void* ptr)
    {
        Core::GetHeapAllocator<HEAP_NAVIGATION>().Free(ptr);
    }

    void reset() override
    {
        Allocator.Reset();
    }

    void* alloc(const size_t size) override
    {
        return Allocator.Allocate(size);
    }

    void free(void*) override
    {
    }
};

NavQueryFilter::NavQueryFilter()
{
    for (int i = 0; i < NAV_MESH_AREA_MAX; ++i)
        m_AreaCost[i] = 1.0f;
}

NavMeshInterface::NavMeshInterface()
{
    m_AreaDesc[NAV_MESH_AREA_GROUND].Name = "Ground";
    m_AreaDesc[NAV_MESH_AREA_GROUND].Color = duRGBA(0, 255, 0, 255);
    m_AreaDesc[NAV_MESH_AREA_WATER].Name = "Water";
    m_AreaDesc[NAV_MESH_AREA_WATER].Color = duRGBA(0, 192, 255, 255);
    for (int i = 2; i < NAV_MESH_AREA_MAX; ++i)
    {
        m_AreaDesc[i].Name = "User_" + Core::ToString(i);
        m_AreaDesc[i].Color = duIntToCol(i, 255);
    }
}

void NavMeshInterface::Initialize()
{
    TickFunction tickFunc;
    tickFunc.Desc.Name.FromString("Update NavMesh");
    tickFunc.Desc.TickEvenWhenPaused = false;
    tickFunc.Group = TickGroup::PostTransform;
    tickFunc.Delegate.Bind(this, &NavMeshInterface::Update);
    tickFunc.OwnerTypeID = GetInterfaceTypeID() | (1 << 31);
    RegisterTickFunction(tickFunc);

    RegisterDebugDrawFunction({this, &NavMeshInterface::DrawDebug});
}

void NavMeshInterface::Deinitialize()
{
    Purge();
}

bool NavMeshInterface::Create()
{
    static_assert(MaxVertsPerPoly == DT_VERTS_PER_POLYGON, "Value check");

    dtStatus status;

    Purge();

    // Copy initial properties
    m_WalkableHeight = WalkableHeight;
    m_WalkableRadius = WalkableRadius;
    m_WalkableClimb = WalkableClimb;
    m_WalkableSlopeAngle = WalkableSlopeAngle;
    m_CellSize = CellSize;
    m_CellHeight = CellHeight;
    m_EdgeMaxLength = EdgeMaxLength;
    m_EdgeMaxError = EdgeMaxError;
    m_MinRegionSize = MinRegionSize;
    m_MergeRegionSize = MergeRegionSize;
    m_DetailSampleDist = DetailSampleDist;
    m_DetailSampleMaxError = DetailSampleMaxError;
    if (m_VertsPerPoly < 3)
    {
        m_VertsPerPoly = 3;
        LOG("NavMeshInterface::Create: VertsPerPoly < 3\n");
    }
    else if (m_VertsPerPoly > MaxVertsPerPoly)
    {
        m_VertsPerPoly = MaxVertsPerPoly;
        LOG("NavMeshInterface::Create: VertsPerPoly > MaxVertsPerPoly\n");
    }
    m_TileSize = TileSize;
    m_IsDynamic = IsDynamic;
    m_MaxLayers = MaxLayers;
    if (m_MaxLayers > MaxAllowedLayers)
    {
        LOG("NavMeshInterface::Create: MaxLayers > MaxAllowedLayers\n");
        m_MaxLayers = MaxAllowedLayers;
    }
    m_PartitionMethod = PartitionMethod;

    m_BoundingBox.Clear();
    for (auto& navigationVolume : NavigationVolumes)
        m_BoundingBox.AddAABB(navigationVolume);

    if (m_BoundingBox.IsEmpty())
    {
        LOG("NavMeshInterface::Create: empty bounds\n");
        return false;
    }

    int gridW, gridH;
    rcCalcGridSize(m_BoundingBox.Mins.ToPtr(), m_BoundingBox.Maxs.ToPtr(), m_CellSize, &gridW, &gridH);

    m_NumTilesX = (gridW + m_TileSize - 1) / m_TileSize;
    m_NumTilesZ = (gridH + m_TileSize - 1) / m_TileSize;

    // Max tiles and max polys affect how the tile IDs are caculated.
    // There are 22 bits available for identifying a tile and a polygon.
    const int tileBits = Math::Min<int>(Math::Log2(Math::ToGreaterPowerOfTwo((uint64_t)m_NumTilesX * m_NumTilesZ)), 14);
    const int maxTiles = 1 << tileBits;
    const int maxPolysPerTile = 1u << (22 - tileBits);

    m_TileWidth = m_TileSize * m_CellSize;

    dtNavMeshParams params = {};
    rcVcopy(params.orig, m_BoundingBox.Mins.ToPtr());
    params.tileWidth = m_TileWidth;
    params.tileHeight = m_TileWidth;
    params.maxTiles = maxTiles;
    params.maxPolys = maxPolysPerTile;

    m_NavMesh = dtAllocNavMesh();
    if (!m_NavMesh)
    {
        Purge();
        LOG("NavMeshInterface::Create: Failed on dtAllocNavMesh\n");
        return false;
    }

    status = m_NavMesh->init(&params);
    if (dtStatusFailed(status))
    {
        Purge();
        LOG("NavMeshInterface::Create: Could not initialize navmesh\n");
        return false;
    }

    m_NavQuery = dtAllocNavMeshQuery();
    if (!m_NavQuery)
    {
        Purge();
        LOG("NavMeshInterface::Create: Failed on dtAllocNavMeshQuery\n");
        return false;
    }

    const int MAX_NODES = 2048;
    status = m_NavQuery->init(m_NavMesh, MAX_NODES);
    if (dtStatusFailed(status))
    {
        Purge();
        LOG("NavMeshInterface::Create: Could not initialize navmesh query\n");
        return false;
    }

    m_MeshProcess = MakeUnique<DetourMeshProcess>();
    m_MeshProcess->m_NavMeshInterface = this;

    if (m_IsDynamic)
    {
        dtTileCacheParams tileCacheParams;
        Core::ZeroMem(&tileCacheParams, sizeof(tileCacheParams));
        rcVcopy(tileCacheParams.orig, m_BoundingBox.Mins.ToPtr());
        tileCacheParams.cs = m_CellSize;
        tileCacheParams.ch = m_CellHeight;
        tileCacheParams.width = m_TileSize;
        tileCacheParams.height = m_TileSize;
        tileCacheParams.walkableHeight = m_WalkableHeight;
        tileCacheParams.walkableRadius = m_WalkableRadius;
        tileCacheParams.walkableClimb = m_WalkableClimb;
        tileCacheParams.maxSimplificationError = m_EdgeMaxError;
        tileCacheParams.maxTiles = maxTiles * m_MaxLayers;
        tileCacheParams.maxObstacles = MaxDynamicObstacles;

        m_TileCache = dtAllocTileCache();
        if (!m_TileCache)
        {
            Purge();
            LOG("NavMeshInterface::Create: Failed on dtAllocTileCache\n");
            return false;
        }

        m_LinearAllocator = MakeUnique<DetourLinearAllocator>();

        status = m_TileCache->init(&tileCacheParams, m_LinearAllocator.RawPtr(), &s_TileCompressorCallback, m_MeshProcess.RawPtr());
        if (dtStatusFailed(status))
        {
            Purge();
            LOG("NavMeshInterface::Create: Could not initialize tile cache\n");
            return false;
        }
    }

    return true;
}

void NavMeshInterface::Purge()
{
    dtFreeNavMeshQuery(m_NavQuery);
    m_NavQuery = nullptr;

    dtFreeNavMesh(m_NavMesh);
    m_NavMesh = nullptr;

    //dtFreeCrowd(m_Crowd);
    //m_Crowd = nullptr;

    dtFreeTileCache(m_TileCache);
    m_TileCache = nullptr;

    m_LinearAllocator.Reset();
    m_MeshProcess.Reset();

    m_NumTilesX = 0;
    m_NumTilesZ = 0;
}

void NavMeshInterface::Clear()
{
    if (!m_NavMesh)
        return;

    if (m_IsDynamic)
    {
        HK_ASSERT(m_TileCache);

        int tileCount = m_TileCache->getTileCount();
        for (int i = 0; i < tileCount; i++)
        {
            const dtCompressedTile* tile = m_TileCache->getTile(i);
            if (tile && tile->header)
            {
                m_TileCache->removeTile(m_TileCache->getTileRef(tile), nullptr, nullptr);
            }
        }
    }
    else
    {
        int tileCount = m_NavMesh->getMaxTiles();
        const dtNavMesh* navMesh = m_NavMesh;
        for (int i = 0; i < tileCount; i++)
        {
            const dtMeshTile* tile = navMesh->getTile(i);
            if (tile && tile->header)
            {
                m_NavMesh->removeTile(m_NavMesh->getTileRef(tile), nullptr, nullptr);
            }
        }
    }
}

void NavMeshInterface::ClearTile(int inX, int inZ)
{
    if (!m_NavMesh)
        return;

    if (m_IsDynamic)
    {
        HK_ASSERT(m_TileCache);

        dtCompressedTileRef compressedTiles[MaxAllowedLayers];
        int count = m_TileCache->getTilesAt(inX, inZ, compressedTiles, m_MaxLayers);
        for (int i = 0; i < count; i++)
        {
            byte* data = nullptr;
            dtStatus status = m_TileCache->removeTile(compressedTiles[i], &data, nullptr);
            if (dtStatusFailed(status))
                continue;
            dtFree(data);
        }
    }
    else
    {
        dtTileRef ref = m_NavMesh->getTileRefAt(inX, inZ, 0);
        if (ref)
            m_NavMesh->removeTile(ref, nullptr, nullptr);
    }
}

void NavMeshInterface::ClearTiles(Int2 const& inMins, Int2 const& inMaxs)
{
    if (!m_NavMesh)
        return;

    for (int z = inMins[1]; z <= inMaxs[1]; z++)
        for (int x = inMins[0]; x <= inMaxs[0]; x++)
            ClearTile(x, z);
}

bool NavMeshInterface::IsEmpty(int inX, int inZ) const
{
    return !m_NavMesh || m_NavMesh->getTileAt(inX, inZ, 0) == nullptr;
}

bool NavMeshInterface::Build()
{
    if (m_NumTilesX == 0 || m_NumTilesZ == 0)
        return false;

    return Build(Int2(0, 0), Int2(m_NumTilesX - 1, m_NumTilesZ - 1));
}

void NavMeshInterface::BuildOnNextFrame()
{
    m_BuildOnNextFrame = true;

    m_FrameNum = GetWorld()->GetTick().FixedFrameNum + 1;
}

bool NavMeshInterface::Build(Int2 const& inMins, Int2 const& inMaxs)
{
    if (!m_NavMesh)
    {
        LOG("NavMeshInterface::Build: navmesh must be initialized\n");
        return false;
    }

    Int2 clampedMins;
    Int2 clampedMaxs;

    clampedMins.X = Math::Clamp<int>(inMins.X, 0, m_NumTilesX - 1);
    clampedMins.Y = Math::Clamp<int>(inMins.Y, 0, m_NumTilesZ - 1);
    clampedMaxs.X = Math::Clamp<int>(inMaxs.X, 0, m_NumTilesX - 1);
    clampedMaxs.Y = Math::Clamp<int>(inMaxs.Y, 0, m_NumTilesZ - 1);

    unsigned int count = 0;
    for (int z = clampedMins[1]; z <= clampedMaxs[1]; z++)
        for (int x = clampedMins[0]; x <= clampedMaxs[0]; x++)
            if (BuildTile(x, z))
                count++;
    return count > 0;
}

bool NavMeshInterface::Build(BvAxisAlignedBox const& inBoundingBox)
{
    if (m_TileWidth == 0.0f)
        return false;

    Int2 mins((inBoundingBox.Mins.X - m_BoundingBox.Mins.X) / m_TileWidth,
        (inBoundingBox.Mins.Z - m_BoundingBox.Mins.Z) / m_TileWidth);
    Int2 maxs((inBoundingBox.Maxs.X - m_BoundingBox.Mins.X) / m_TileWidth,
        (inBoundingBox.Maxs.Z - m_BoundingBox.Mins.Z) / m_TileWidth);

    return Build(mins, maxs);
}

void NavMeshInterface::SetAreaCost(NAV_MESH_AREA inAreaType, float inCost)
{
    m_QueryFilter.SetAreaCost(inAreaType, inCost);
}

float NavMeshInterface::GetAreaCost(NAV_MESH_AREA inAreaType) const
{
    return m_QueryFilter.GetAreaCost(inAreaType);
}

void NavMeshInterface::Update()
{
    if (m_BuildOnNextFrame && m_FrameNum == GetWorld()->GetTick().FixedFrameNum)
    {
        m_BuildOnNextFrame = false;

        Build();
    }

    if (m_TileCache)
        m_TileCache->update(GetWorld()->GetTick().FixedTimeStep, m_NavMesh);
}

void NavMeshInterface::AddObstacle(NavMeshObstacleComponent* inObstacle)
{
    if (!m_TileCache)
        return;

    HK_ASSERT(!inObstacle->m_ObstacleRef);

    dtObstacleRef ref = 0;
    dtStatus status = DT_FAILURE;

    Float3 position = inObstacle->m_Position;
    float angle = Math::Radians(inObstacle->GetAngle());

    while (1)
    {
        switch (inObstacle->GetShape())
        {
            case NavMeshObstacleShape::Box:
                if (angle == 0.0f)
                    status = m_TileCache->addBoxObstacle((position - inObstacle->GetHalfExtents()).ToPtr(), (position + inObstacle->GetHalfExtents()).ToPtr(), &ref);
                else
                    status = m_TileCache->addBoxObstacle(position.ToPtr(), inObstacle->GetHalfExtents().ToPtr(), angle, &ref);
                break;
            case NavMeshObstacleShape::Cylinder:
            {
                Float3 offset = Float3(0, inObstacle->GetHeight() * 0.5f, 0);
                status = m_TileCache->addObstacle((position - offset).ToPtr(), inObstacle->GetRadius(), inObstacle->GetHeight(), &ref);
                break;
            }
        };

        if (!(status & DT_BUFFER_TOO_SMALL))
            break;

        m_TileCache->update(1, m_NavMesh);
    }

    if (dtStatusFailed(status))
    {
        LOG("Failed to add navmesh obstacle: {}\n", GetErrorStr(status));
        return;
    }

    inObstacle->m_ObstacleRef = ref;
}

void NavMeshInterface::RemoveObstacle(NavMeshObstacleComponent* inObstacle)
{
    if (!m_TileCache)
        return;

    if (!inObstacle->m_ObstacleRef)
        return;

    dtStatus status;

    while (1)
    {
        status = m_TileCache->removeObstacle(inObstacle->m_ObstacleRef);

        if (!(status & DT_BUFFER_TOO_SMALL))
            break;

        m_TileCache->update(1, m_NavMesh);
    }

    if (dtStatusFailed(status))
    {
        LOG("Failed to remove navmesh obstacle: {}\n", GetErrorStr(status));
        return;
    }

    inObstacle->m_ObstacleRef = 0;
}

void NavMeshInterface::UpdateObstacle(NavMeshObstacleComponent* inObstacle)
{
    RemoveObstacle(inObstacle);
    AddObstacle(inObstacle);
}

bool NavMeshInterface::CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavQueryFilter const& inFilter, NavMeshRayCastResult& outResult) const
{
    NavPolyRef polyRef;

    if (!QueryNearestPoly(inRayStart, inExtents, inFilter, polyRef))
        return false;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    auto status = m_NavQuery->raycast(polyRef, inRayStart.ToPtr(), inRayEnd.ToPtr(), &filter, &outResult.Fraction, outResult.Normal.ToPtr(), TmpPolys, nullptr, MAX_POLYS);
    if (dtStatusFailed(status))
        return false;

    return outResult.Fraction != FLT_MAX;
}

bool NavMeshInterface::CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavMeshRayCastResult& outResult) const
{
    return CastRay(inRayStart, inRayEnd, inExtents, m_QueryFilter, outResult);
}

bool NavMeshInterface::GetTileLocation(Float3 const& inPosition, int& outTileX, int& outTileY) const
{
    if (!m_NavMesh)
    {
        outTileX = outTileY = 0;
        return false;
    }

    m_NavMesh->calcTileLoc(inPosition.ToPtr(), &outTileX, &outTileY);
    return true;
}

bool NavMeshInterface::QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPolyRef& outNearestPolyRef) const
{
    outNearestPolyRef = 0;

    if (!m_NavQuery)
        return false;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    auto status = m_NavQuery->findNearestPoly(inPosition.ToPtr(), inExtents.ToPtr(), &filter, &outNearestPolyRef, nullptr);
    if (dtStatusFailed(status) || !outNearestPolyRef)
        return false;

    return true;
}

bool NavMeshInterface::QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavPolyRef& outNearestPolyRef) const
{
    return QueryNearestPoly(inPosition, inExtents, m_QueryFilter, outNearestPolyRef);
}

bool NavMeshInterface::QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef& outNearestPointRef) const
{
    outNearestPointRef.PolyRef = 0;
    outNearestPointRef.Position.Clear();

    if (!m_NavQuery)
        return false;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    auto status = m_NavQuery->findNearestPoly(inPosition.ToPtr(), inExtents.ToPtr(), &filter, &outNearestPointRef.PolyRef, outNearestPointRef.Position.ToPtr());
    if (dtStatusFailed(status) || !outNearestPointRef.PolyRef)
        return false;

    return true;
}

bool NavMeshInterface::QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavPointRef& outNearestPointRef) const
{
    return QueryNearestPoint(inPosition, inExtents, m_QueryFilter, outNearestPointRef);
}

namespace
{
    float NavRandom()
    {
        return GameApplication::GetRandom().GetFloat();
    }
} // namespace

bool NavMeshInterface::QueryRandomPoint(NavQueryFilter const& inFilter, NavPointRef& outRandomPointRef) const
{
    outRandomPointRef.PolyRef = 0;
    outRandomPointRef.Position.Clear();

    if (!m_NavQuery)
        return false;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    auto status = m_NavQuery->findRandomPoint(&filter, NavRandom, &outRandomPointRef.PolyRef, outRandomPointRef.Position.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::QueryRandomPoint(NavPointRef& outRandomPointRef) const
{
    return QueryRandomPoint(m_QueryFilter, outRandomPointRef);
}

bool NavMeshInterface::QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef& outRandomPointRef) const
{
    NavPointRef pointRef;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    if (!QueryNearestPoly(inPosition, inExtents, inFilter, pointRef.PolyRef))
        return false;

    pointRef.Position = inPosition;

    return QueryRandomPointAroundCircle(pointRef, inRadius, inFilter, outRandomPointRef);
}

bool NavMeshInterface::QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavPointRef& outRandomPointRef) const
{
    return QueryRandomPointAroundCircle(inPosition, inRadius, inExtents, m_QueryFilter, outRandomPointRef);
}

bool NavMeshInterface::QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavPointRef& outRandomPointRef) const
{
    outRandomPointRef.PolyRef = 0;
    outRandomPointRef.Position.Clear();

    if (!m_NavQuery)
        return false;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    dtStatus status = m_NavQuery->findRandomPointAroundCircle(inPointRef.PolyRef, inPointRef.Position.ToPtr(), inRadius, &filter, NavRandom, &outRandomPointRef.PolyRef, outRandomPointRef.Position.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavPointRef& outRandomPointRef) const
{
    return QueryRandomPointAroundCircle(inPointRef, inRadius, m_QueryFilter, outRandomPointRef);
}

bool NavMeshInterface::QueryClosestPointOnPoly(NavPointRef const& inPointRef, Float3& outPoint, bool* outOverPolygon) const
{
    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->closestPointOnPoly(inPointRef.PolyRef, inPointRef.Position.ToPtr(), outPoint.ToPtr(), outOverPolygon);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::QueryClosestPointOnPolyBoundary(NavPointRef const& inPointRef, Float3& outPoint) const
{
    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->closestPointOnPolyBoundary(inPointRef.PolyRef, inPointRef.Position.ToPtr(), outPoint.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavQueryFilter const& inFilter, NavPolyRef* outVisited, int& outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const
{
    if (!m_NavQuery)
        return false;

    inMaxVisitedSize = Math::Max(inMaxVisitedSize, 0);

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    dtStatus status = m_NavQuery->moveAlongSurface(inPointRef.PolyRef, inPointRef.Position.ToPtr(), inDestination.ToPtr(), &filter, outResultPos.ToPtr(), outVisited, &outVisitedCount, inMaxVisitedSize);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavPolyRef* outVisited, int& outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const
{
    return MoveAlongSurface(inPointRef, inDestination, m_QueryFilter, outVisited, outVisitedCount, inMaxVisitedSize, outResultPos);
}

bool NavMeshInterface::MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, NavQueryFilter const& inFilter, int inMaxVisitedSize, Float3& outResultPos) const
{
    NavPointRef pointRef;

    m_LastVisitedPolys.Clear();

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    if (!QueryNearestPoly(inPosition, inExtents, inFilter, pointRef.PolyRef))
        return false;

    pointRef.Position = inPosition;

    m_LastVisitedPolys.Resize(Math::Max(inMaxVisitedSize, 0));

    int visited_count = 0;
    if (!MoveAlongSurface(pointRef, inDestination, inFilter, m_LastVisitedPolys.ToPtr(), visited_count, m_LastVisitedPolys.Size(), outResultPos))
    {
        m_LastVisitedPolys.Clear();
        return false;
    }

    m_LastVisitedPolys.Resize(visited_count);

    return true;
}

bool NavMeshInterface::MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, int inMaxVisitedSize, Float3& outResultPos) const
{
    return MoveAlongSurface(inPosition, inDestination, inExtents, m_QueryFilter, inMaxVisitedSize, outResultPos);
}

bool NavMeshInterface::FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavQueryFilter const& inFilter, NavPolyRef* outPath, int& outPathCount, const int inMaxPath) const
{
    outPathCount = 0;

    if (!m_NavQuery)
        return false;

    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    dtStatus status = m_NavQuery->findPath(inStartRef.PolyRef, inEndRef.PolyRef, inStartRef.Position.ToPtr(), inEndRef.Position.ToPtr(), &filter, outPath, &outPathCount, inMaxPath);
    if (dtStatusFailed(status))
    {
        outPathCount = 0;
        return false;
    }

    return true;
}

bool NavMeshInterface::FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavPolyRef* outPath, int& outPathCount, const int inMaxPath) const
{
    return FindPath(inStartRef, inEndRef, m_QueryFilter, outPath, outPathCount, inMaxPath);
}

bool NavMeshInterface::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, Vector<NavMeshPathPoint>& outPathPoints) const
{
    NavPointRef startRef;
    NavPointRef endRef;

    if (!QueryNearestPoly(inStartPos, inExtents, inFilter, startRef.PolyRef))
        return false;

    if (!QueryNearestPoly(inEndPos, inExtents, inFilter, endRef.PolyRef))
        return false;

    startRef.Position = inStartPos;
    endRef.Position = inEndPos;

    int numPolys;
    if (!FindPath(startRef, endRef, inFilter, TmpPolys, numPolys, MAX_POLYS))
        return false;

    Float3 closestLocalEnd = inEndPos;

    if (TmpPolys[numPolys - 1] != endRef.PolyRef)
        m_NavQuery->closestPointOnPoly(TmpPolys[numPolys - 1], inEndPos.ToPtr(), closestLocalEnd.ToPtr(), 0);

    int pathLen;
    m_NavQuery->findStraightPath(inStartPos.ToPtr(), closestLocalEnd.ToPtr(), TmpPolys, numPolys, TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &pathLen, MAX_POLYS);

    outPathPoints.Resize(pathLen);
    for (int i = 0; i < pathLen; ++i)
    {
        outPathPoints[i].Position = TmpPathPoints[i];
        outPathPoints[i].Flags = NavMeshPathFlags(TmpPathFlags[i]);
    }

    return true;
}

bool NavMeshInterface::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, Vector<NavMeshPathPoint>& outPathPoints) const
{
    return FindPath(inStartPos, inEndPos, inExtents, m_QueryFilter, outPathPoints);
}

bool NavMeshInterface::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, Vector<Float3>& outPathPoints) const
{
    NavPointRef startRef;
    NavPointRef endRef;

    if (!QueryNearestPoly(inStartPos, inExtents, inFilter, startRef.PolyRef))
        return false;

    if (!QueryNearestPoly(inEndPos, inExtents, inFilter, endRef.PolyRef))
        return false;

    startRef.Position = inStartPos;
    endRef.Position = inEndPos;

    int numPolys;
    if (!FindPath(startRef, endRef, inFilter, TmpPolys, numPolys, MAX_POLYS))
        return false;

    Float3 closestLocalEnd = inEndPos;

    if (TmpPolys[numPolys - 1] != endRef.PolyRef)
        m_NavQuery->closestPointOnPoly(TmpPolys[numPolys - 1], inEndPos.ToPtr(), closestLocalEnd.ToPtr(), 0);

    int pathLen;
    m_NavQuery->findStraightPath(inStartPos.ToPtr(), closestLocalEnd.ToPtr(), TmpPolys, numPolys, TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &pathLen, MAX_POLYS);

    outPathPoints.Resize(pathLen);
    Core::Memcpy(outPathPoints.ToPtr(), TmpPathPoints, sizeof(Float3) * pathLen);

    return true;
}

bool NavMeshInterface::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, Vector<Float3>& outPathPoints) const
{
    return FindPath(inStartPos, inEndPos, inExtents, m_QueryFilter, outPathPoints);
}

bool NavMeshInterface::FindStraightPath(Float3 const& inStartPos, Float3 const& inEndPos, NavPolyRef const* inPath, int inPathSize, Float3* outStraightPath, NavMeshPathFlags* outStraightPathFlags, NavPolyRef* outStraightPathRefs, int& outStraightPathCount, int inMaxStraightPath, NavMeshCrossings inStraightPathCrossing) const
{
    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->findStraightPath(inStartPos.ToPtr(), inEndPos.ToPtr(), inPath, inPathSize, (float*)outStraightPath, reinterpret_cast<uint8_t*>(outStraightPathFlags), outStraightPathRefs, &outStraightPathCount, inMaxStraightPath, (int)inStraightPathCrossing);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const
{
    dtQueryFilter filter(inFilter.GetAreaCosts().ToPtr(), inFilter.GetAreaMask());

    dtStatus status = m_NavQuery->findDistanceToWall(inPointRef.PolyRef, inPointRef.Position.ToPtr(), inRadius, &filter, &outHitResult.Distance, outHitResult.Position.ToPtr(), outHitResult.Normal.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMeshInterface::CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavMeshHitResult& outHitResult) const
{
    return CalcDistanceToWall(inPointRef, inRadius, m_QueryFilter, outHitResult);
}

bool NavMeshInterface::CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const
{
    NavPointRef pointRef;

    if (!QueryNearestPoly(inPosition, inExtents, inFilter, pointRef.PolyRef))
        return false;

    pointRef.Position = inPosition;

    return CalcDistanceToWall(pointRef, inRadius, inFilter, outHitResult);
}

bool NavMeshInterface::CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavMeshHitResult& outHitResult) const
{
    return CalcDistanceToWall(inPosition, inRadius, inExtents, m_QueryFilter, outHitResult);
}

bool NavMeshInterface::GetHeight(NavPointRef const& inPointRef, float& outHeight) const
{
    if (!m_NavQuery)
    {
        outHeight = 0;
        return false;
    }

    dtStatus status = m_NavQuery->getPolyHeight(inPointRef.PolyRef, inPointRef.Position.ToPtr(), &outHeight);
    if (dtStatusFailed(status))
    {
        outHeight = 0;
        return false;
    }

    return true;
}

bool NavMeshInterface::GetOffMeshConnectionPolyEndPoints(NavPolyRef inPrevRef, NavPolyRef inPolyRef, Float3& outStartPos, Float3& outEndPos) const
{
    if (!m_NavMesh)
        return false;

    dtStatus status = m_NavMesh->getOffMeshConnectionPolyEndPoints(inPrevRef, inPolyRef, outStartPos.ToPtr(), outEndPos.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

BvAxisAlignedBox NavMeshInterface::GetTileWorldBounds(int inX, int inZ) const
{
    return
    {
        {
            m_BoundingBox.Mins[0] + inX * m_TileWidth,
            m_BoundingBox.Mins[1],
            m_BoundingBox.Mins[2] + inZ * m_TileWidth},
        {
            m_BoundingBox.Mins[0] + (inX + 1) * m_TileWidth,
            m_BoundingBox.Maxs[1],
            m_BoundingBox.Mins[2] + (inZ + 1) * m_TileWidth
        }
    };
}

void duDebugDrawOffMeshCons(struct duDebugDraw* dd, const dtNavMesh& mesh)
{
    if (!dd) return;

    dd->depthMask(false);
    for (int tileIdx = 0; tileIdx < mesh.getMaxTiles(); ++tileIdx)
    {
        const dtMeshTile* tile = mesh.getTile(tileIdx);
        if (!tile->header) continue;

        dd->begin(DU_DRAW_LINES, 2.0f);
        for (int polyIdx = 0; polyIdx < tile->header->polyCount; ++polyIdx)
        {
            const dtPoly* p = &tile->polys[polyIdx];
            if (p->getType() != DT_POLYTYPE_OFFMESH_CONNECTION) // Skip regular polys.
                continue;

            unsigned int col = duDarkenCol(duTransCol(dd->areaToCol(p->getArea()), 220));

            const dtOffMeshConnection* con = &tile->offMeshCons[polyIdx - tile->header->offMeshBase];
            const float* va = &tile->verts[p->verts[0] * 3];
            const float* vb = &tile->verts[p->verts[1] * 3];

            // Check to see if start and end end-points have links.
            bool startSet = false;
            bool endSet = false;
            for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
            {
                if (tile->links[k].edge == 0)
                    startSet = true;
                if (tile->links[k].edge == 1)
                    endSet = true;
            }

            unsigned int col2;

            // End points and their on-mesh locations.
            dd->vertex(va[0], va[1], va[2], col);
            dd->vertex(con->pos[0], con->pos[1], con->pos[2], col);
            col2 = startSet ? col : duRGBA(220, 32, 16, 196);
            duAppendCircle(dd, con->pos[0], con->pos[1] + 0.1f, con->pos[2], con->rad, col2);

            dd->vertex(vb[0], vb[1], vb[2], col);
            dd->vertex(con->pos[3], con->pos[4], con->pos[5], col);
            col2 = endSet ? col : duRGBA(220, 32, 16, 196);
            duAppendCircle(dd, con->pos[3], con->pos[4] + 0.1f, con->pos[5], con->rad, col2);

            // End point vertices.
            dd->vertex(con->pos[0], con->pos[1], con->pos[2], duRGBA(0, 48, 64, 196));
            dd->vertex(con->pos[0], con->pos[1] + 0.2f, con->pos[2], duRGBA(0, 48, 64, 196));

            dd->vertex(con->pos[3], con->pos[4], con->pos[5], duRGBA(0, 48, 64, 196));
            dd->vertex(con->pos[3], con->pos[4] + 0.2f, con->pos[5], duRGBA(0, 48, 64, 196));

            // Connection arc.
            duAppendArc(dd, con->pos[0], con->pos[1], con->pos[2], con->pos[3], con->pos[4], con->pos[5], 0.25f,
                (con->flags & 1) ? 0.6f : 0, 0.6f, col);
        }
        dd->end();
    }
    dd->depthMask(true);
}

void NavMeshInterface::DrawDebug(DebugRenderer& renderer)
{
    struct DebugDrawCallback final : public duDebugDraw
    {
        DebugRenderer*        DD;
        NavMeshInterface::AreaDesc* AreaDesc;
        Float3                AccumVertices[3];
        int                   AccumIndex;
        duDebugDrawPrimitives Primitive;

        DebugDrawCallback()
        {
            AccumIndex = 0;
        }

        void depthMask(bool state) override
        {
            //DD->SetDepthTest(state);
        }

        void texture(bool state) override {}

        void begin(duDebugDrawPrimitives prim, float size = 1.0f) override
        {
            Primitive  = prim;
            AccumIndex = 0;
        }

        void vertex(const float* pos, unsigned int color) override
        {
            vertex(pos[0], pos[1], pos[2], color);
        }

        void vertex(const float x, const float y, const float z, unsigned int color) override
        {
            DD->SetColor(color);

            switch (Primitive)
            {
            case DU_DRAW_POINTS:
                DD->DrawPoint(Float3(x, y, z));
                break;
            case DU_DRAW_LINES:
                if (AccumIndex > 0)
                {
                    DD->DrawLine(AccumVertices[0], Float3(x, y, z));
                    AccumIndex = 0;
                }
                else
                {
                    AccumVertices[AccumIndex++] = Float3(x, y, z);
                }
                break;
            case DU_DRAW_TRIS:
                if (AccumIndex > 1)
                {
                    DD->DrawTriangle(AccumVertices[0], AccumVertices[1], Float3(x, y, z));
                    AccumIndex = 0;
                }
                else
                {
                    AccumVertices[AccumIndex++] = Float3(x, y, z);
                }
                break;
            case DU_DRAW_QUADS:
                if (AccumIndex > 2)
                {
                    DD->DrawTriangle(AccumVertices[0], AccumVertices[1], AccumVertices[2]);
                    DD->DrawTriangle(AccumVertices[2], Float3(x, y, z), AccumVertices[0]);
                    AccumIndex = 0;
                }
                else
                {
                    AccumVertices[AccumIndex++] = Float3(x, y, z);
                }
                break;
            }
        }

        void vertex(const float* pos, unsigned int color, const float* uv) override
        {
            vertex(pos, color);
        }

        void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override
        {
            vertex(x, y, z, color);
        }

        void end() override {}

        unsigned int areaToCol(unsigned int area) override
        {
            HK_IF_NOT_ASSERT(area < NAV_MESH_AREA_MAX)
            {
                return 0xffffffff;
            }
            return AreaDesc[area].Color;
        }
    };

    if (!m_NavMesh)
        return;

    DebugDrawCallback callback;
    callback.AreaDesc = m_AreaDesc;
    callback.DD = &renderer;

    renderer.SetDepthTest(true);

    if (com_DrawNavMeshBVTree)
        duDebugDrawNavMeshBVTree(&callback, *m_NavMesh);

    if (com_DrawNavMeshNodes)
        duDebugDrawNavMeshNodes(&callback, *m_NavQuery);

    if (com_DrawNavMesh)
        duDebugDrawNavMesh(&callback, *m_NavMesh, 0);

    if (com_DrawOffMeshLinks)
        duDebugDrawOffMeshCons(&callback, *m_NavMesh);

    if (com_DrawNavMeshTileBounds)
    {
        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 1, 1, 1));

        dtNavMesh const& mesh = *m_NavMesh;
        for (int tileIdx = 0; tileIdx < mesh.getMaxTiles(); ++tileIdx)
        {
            const dtMeshTile* tile = mesh.getTile(tileIdx);
            if (!tile->header) continue;

            renderer.DrawAABB(BvAxisAlignedBox(*(Float3*)&tile->header->bmin[0], *(Float3*)&tile->header->bmax[0]));
        }
    }
}

void NavMeshInterface::GatherNavigationGeometry(NavigationGeometry& navGeometry)
{
    auto& physics = GetWorld()->GetInterface<PhysicsInterface>();

    Vector<Float3> tempVertices;
    Vector<uint32_t> tempIndices;

    auto& cropBox = navGeometry.GetMaxCropBox();

    ShapeOverlapFilter filter;
    filter.BroadphaseLayers
        .AddLayer(BroadphaseLayer::Static);

    Vector<PhysBodyID> queryResult;
    physics.OverlapBoxMinMax(cropBox.Mins, cropBox.Maxs, queryResult, filter);

    for (PhysBodyID bodyID : queryResult)
    {
        tempVertices.Clear();
        tempIndices.Clear();

        if (auto staticBody = physics.TryGetComponent<StaticBodyComponent>(bodyID))
        {
            staticBody->GatherGeometry(tempVertices, tempIndices);
        }
        else if (auto heightField = physics.TryGetComponent<HeightFieldComponent>(bodyID))
        {
            heightField->GatherGeometry(cropBox, tempVertices, tempIndices);
        }

        navGeometry.AddTriangleSoup(tempVertices, tempIndices);
    }
}

namespace
{

    // Based on rcMarkWalkableTriangles
#if 0
    void MarkWalkableTriangles(float inSlopeAngleDeg, Float3 const* inVertices, unsigned int const* inIndices, int inTriangleCount, int inFirstTriangle, BitMask<> const& inWalkableMask, unsigned char* outAreas)
    {
        Float3 perpendicular;
        float perpendicularLength;

        const float threshold = Math::Cos(Math::Radians(inSlopeAngleDeg));

        for (int i = 0; i < inTriangleCount; ++i)
        {
            int triangle = inFirstTriangle + i;
            if (inWalkableMask.IsMarked(triangle))
            {
                unsigned int const* tri = &inIndices[triangle * 3];

                perpendicular = Math::Cross(inVertices[tri[1]] - inVertices[tri[0]], inVertices[tri[2]] - inVertices[tri[0]]);
                perpendicularLength = perpendicular.Length();
                if (perpendicularLength > 0 && perpendicular[1] > threshold * perpendicularLength)
                {
                    outAreas[i] = RC_WALKABLE_AREA;
                }
            }
        }
    }
#else
    void MarkWalkableTriangles(float inSlopeAngleDeg, Float3 const* inVertices, int inTriangleCount, int inFirstTriangle/*, BitMask<> const& inWalkableMask*/, unsigned char* outAreas)
    {
        Float3 perpendicular;
        float perpendicularLength;

        const float threshold = Math::Cos(Math::Radians(inSlopeAngleDeg));

        for (int i = 0; i < inTriangleCount; ++i)
        {
            int triangle = inFirstTriangle + i;
            //if (inWalkableMask.IsMarked(triangle))
            {
                Float3 const* tri = &inVertices[triangle * 3];

                perpendicular = Math::Cross(tri[1] - tri[0], tri[2] - tri[0]);
                perpendicularLength = perpendicular.Length();
                if (perpendicularLength > 0 && perpendicular[1] > threshold * perpendicularLength)
                {
                    outAreas[i] = RC_WALKABLE_AREA;
                }
            }
        }
    }
#endif

    bool PointInPoly2D(int nvert, const float* verts, const float* p)
    {
        int i, j;
        bool c = false;
        for (i = 0, j = nvert - 1; i < nvert; j = i++)
        {
            const float* vi = &verts[i * 2];
            const float* vj = &verts[j * 2];
            if (((vi[1] > p[1]) != (vj[1] > p[1])) &&
                (p[0] < (vj[0] - vi[0]) * (p[1] - vi[1]) / (vj[1] - vi[1]) + vi[0]))
                c = !c;
        }
        return c;
    }

} // namespace

bool NavMeshInterface::BuildTile(int inX, int inZ)
{
    class RecastContext : public rcContext
    {
    public:
        RecastContext()
        {
            enableLog(RECAST_ENABLE_LOGGING);
            enableTimer(RECAST_ENABLE_TIMINGS);
        }

    protected:
        // Virtual functions override
        void doResetLog() override {}
        void doLog(const rcLogCategory category, const char* msg, const int len) override
        {
            switch (category)
            {
            case RC_LOG_PROGRESS:
                LOG(msg);
                break;
            case RC_LOG_WARNING:
                LOG(msg);
                break;
            case RC_LOG_ERROR:
                LOG(msg);
                break;
            default:
                LOG(msg);
                break;
            }
        }
        void doResetTimers() override {}
        void doStartTimer(const rcTimerLabel label) override {}
        void doStopTimer(const rcTimerLabel label) override {}
        int doGetAccumulatedTime(const rcTimerLabel label) const override { return -1; }
    };

    struct TemportalData
    {
        rcHeightfield*         Heightfield;
        rcCompactHeightfield*  CompactHeightfield;
        rcContourSet*          ContourSet;
        rcPolyMesh*            PolyMesh;
        rcPolyMeshDetail*      PolyMeshDetail;
        rcHeightfieldLayerSet* LayerSet;

        TemportalData()
        {
            Core::ZeroMem(this, sizeof(*this));
        }

        ~TemportalData()
        {
            rcFreeHeightField(Heightfield);
            rcFreeCompactHeightfield(CompactHeightfield);
            rcFreeContourSet(ContourSet);
            rcFreePolyMesh(PolyMesh);
            rcFreePolyMeshDetail(PolyMeshDetail);
            rcFreeHeightfieldLayerSet(LayerSet);
        }
    };

    static RecastContext s_RecastContext;

    HK_ASSERT(m_NavMesh);

    ClearTile(inX, inZ);

    rcConfig config = {};
    config.cs = m_CellSize;
    config.ch = m_CellHeight;
    config.walkableSlopeAngle = m_WalkableSlopeAngle;
    config.walkableHeight = (int)Math::Ceil(m_WalkableHeight / config.ch);
    config.walkableClimb = (int)Math::Floor(m_WalkableClimb / config.ch);
    config.walkableRadius = (int)Math::Ceil(m_WalkableRadius / config.cs);
    config.maxEdgeLen = (int)(m_EdgeMaxLength / m_CellSize);
    config.maxSimplificationError = m_EdgeMaxError;
    config.minRegionArea = (int)rcSqr(m_MinRegionSize);        // Note: area = size*size
    config.mergeRegionArea = (int)rcSqr(m_MergeRegionSize); // Note: area = size*size
    config.detailSampleDist = m_DetailSampleDist < 0.9f ? 0 : m_CellSize * m_DetailSampleDist;
    config.detailSampleMaxError = m_CellHeight * m_DetailSampleMaxError;
    config.tileSize = m_TileSize;
    config.borderSize = config.walkableRadius + 3; // radius + padding
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.maxVertsPerPoly = m_VertsPerPoly;

    BvAxisAlignedBox tileBounds = GetTileWorldBounds(inX, inZ);
    BvAxisAlignedBox tileBoundsWithPad = tileBounds;

    tileBoundsWithPad.Mins.X -= config.borderSize * config.cs;
    tileBoundsWithPad.Mins.Z -= config.borderSize * config.cs;
    tileBoundsWithPad.Maxs.X += config.borderSize * config.cs;
    tileBoundsWithPad.Maxs.Z += config.borderSize * config.cs;

    NavigationGeometry geometry;

    BvAxisAlignedBox intersection;
    for (auto& navigationVolume : NavigationVolumes)
    {
        if (BvGetBoxIntersection(tileBoundsWithPad, navigationVolume, intersection))
            geometry.AddCropBox(intersection);
    }

    if (geometry.GetMaxCropBox().IsEmpty())
        return true;

    GatherNavigationGeometry(geometry);

    geometry.Finalize();

    // Empty tile
    if (geometry.IsEmpty())
        return true;

    tileBoundsWithPad.Mins.Y = geometry.GetBoundingBox().Mins.Y;
    tileBoundsWithPad.Maxs.Y = geometry.GetBoundingBox().Maxs.Y;
    
    rcVcopy(config.bmin, tileBoundsWithPad.Mins.ToPtr());
    rcVcopy(config.bmax, tileBoundsWithPad.Maxs.ToPtr());

    auto& vertices = geometry.GetVertices();

    TemportalData temporal;

    // Allocate voxel heightfield where we rasterize our input data to.
    temporal.Heightfield = rcAllocHeightfield();
    if (!temporal.Heightfield)
    {
        LOG("Failed on rcAllocHeightfield\n");
        return false;
    }

    if (!rcCreateHeightfield(&s_RecastContext, *temporal.Heightfield, config.width, config.height,
        config.bmin, config.bmax, config.cs, config.ch))
    {
        LOG("Failed on rcCreateHeightfield\n");
        return false;
    }

    int triangleCount = vertices.Size() / 3;

    // Allocate array that can hold triangle area types.
    unsigned char* triangleAreaTypes = (unsigned char*)Core::GetHeapAllocator<HEAP_TEMP>().Alloc(triangleCount, 16, MALLOC_ZERO);

    // Find triangles which are walkable based on their slope and rasterize them.
    MarkWalkableTriangles(config.walkableSlopeAngle, vertices.ToPtr(), triangleCount, 0, triangleAreaTypes);

    bool rasterized = rcRasterizeTriangles(&s_RecastContext, &vertices.ToPtr()->X, triangleAreaTypes, triangleCount, *temporal.Heightfield, config.walkableClimb);

    Core::GetHeapAllocator<HEAP_TEMP>().Free(triangleAreaTypes);

    if (!rasterized)
    {
        LOG("Failed on rcRasterizeTriangles\n");
        return false;
    }

    // Filter walkables surfaces.

    // Once all geoemtry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    rcFilterLowHangingWalkableObstacles(&s_RecastContext, config.walkableClimb, *temporal.Heightfield);
    rcFilterLedgeSpans(&s_RecastContext, config.walkableHeight, config.walkableClimb, *temporal.Heightfield);
    rcFilterWalkableLowHeightSpans(&s_RecastContext, config.walkableHeight, *temporal.Heightfield);

    // Partition walkable surface to simple regions.
    // Compact the heightfield so that it is faster to handle from now on.
    // This will result more cache coherent data as well as the neighbours
    // between walkable cells will be calculated.
    temporal.CompactHeightfield = rcAllocCompactHeightfield();
    if (!temporal.CompactHeightfield)
    {
        LOG("Failed on rcAllocCompactHeightfield\n");
        return false;
    }

    if (!rcBuildCompactHeightfield(&s_RecastContext, config.walkableHeight, config.walkableClimb, *temporal.Heightfield, *temporal.CompactHeightfield))
    {
        LOG("Failed on rcBuildCompactHeightfield\n");
        return false;
    }

    // Erode the walkable area by agent radius.
    if (!rcErodeWalkableArea(&s_RecastContext, config.walkableRadius, *temporal.CompactHeightfield))
    {
        LOG("NavMeshInterface::Build: Failed on rcErodeWalkableArea\n");
        return false;
    }

    struct Visitor
    {
        rcCompactHeightfield& chf;
        BvAxisAlignedBox tileBoundsWithPad;

        Visitor(rcCompactHeightfield& chf, BvAxisAlignedBox const& tileBoundsWithPad) : chf(chf), tileBoundsWithPad(tileBoundsWithPad) {}

        void Visit(NavMeshAreaComponent& area)
        {
            BvAxisAlignedBox areaBounds = area.CalcBoundingBox();
            if (areaBounds.IsEmpty())
            {
                // Invalid bounding box
                return;
            }

            if (!BvBoxOverlapBox(tileBoundsWithPad, areaBounds))
            {
                // Area is outside of tile bounding box
                return;
            }

            switch (area.GetShape())
            {
                case NavMeshAreaShape::Box:
                    rcMarkBoxArea(&s_RecastContext, areaBounds.Mins.ToPtr(), areaBounds.Maxs.ToPtr(), area.GetAreaType(), chf);
                    break;
                case NavMeshAreaShape::Cylinder:
                {
                    Float3 worldPosition = area.GetOwner()->GetWorldPosition();
                    float height = area.GetHeight();
                    worldPosition.Y -= height * 0.5f;
                    rcMarkCylinderArea(&s_RecastContext, worldPosition.ToPtr(), area.GetCylinderRadius(), height, area.GetAreaType(), chf);
                    break;
                }
                case NavMeshAreaShape::ConvexVolume:
                {
                    // The next code is based on rcMarkConvexPolyArea
                    int minx = (int)((areaBounds.Mins[0] - chf.bmin[0]) / chf.cs);
                    int miny = (int)((areaBounds.Mins[1] - chf.bmin[1]) / chf.ch);
                    int minz = (int)((areaBounds.Mins[2] - chf.bmin[2]) / chf.cs);
                    int maxx = (int)((areaBounds.Maxs[0] - chf.bmin[0]) / chf.cs);
                    int maxy = (int)((areaBounds.Maxs[1] - chf.bmin[1]) / chf.ch);
                    int maxz = (int)((areaBounds.Maxs[2] - chf.bmin[2]) / chf.cs);

                    if (maxx < 0) return;
                    if (minx >= chf.width) return;
                    if (maxz < 0) return;
                    if (minz >= chf.height) return;

                    if (minx < 0) minx = 0;
                    if (maxx >= chf.width) maxx = chf.width - 1;
                    if (minz < 0) minz = 0;
                    if (maxz >= chf.height) maxz = chf.height - 1;

                    Float3 worldPosition = area.GetOwner()->GetWorldPosition();

                    for (int z = minz; z <= maxz; ++z)
                    {
                        for (int x = minx; x <= maxx; ++x)
                        {
                            const rcCompactCell& c = chf.cells[x + z * chf.width];
                            for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
                            {
                                rcCompactSpan& s = chf.spans[i];
                                if (chf.areas[i] == RC_NULL_AREA)
                                    continue;
                                if ((int)s.y >= miny && (int)s.y <= maxy)
                                {
                                    float p[2];
                                    p[0] = chf.bmin[0] + (x + 0.5f) * chf.cs - worldPosition.X;
                                    p[1] = chf.bmin[2] + (z + 0.5f) * chf.cs - worldPosition.Z;

                                    auto& contour = area.GetVolumeContour();

                                    if (PointInPoly2D(contour.Size(), contour[0].ToPtr(), p))
                                    {
                                        chf.areas[i] = area.GetAreaType();
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
    };

    Visitor visitor(*temporal.CompactHeightfield, tileBoundsWithPad);
    auto& areas = GetWorld()->GetComponentManager<NavMeshAreaComponent>();
    areas.IterateComponents(visitor);

    // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
    // There are 3 partitioning methods, each with some pros and cons:
    // 1) Watershed partitioning
    //   - the classic Recast partitioning
    //   - creates the nicest tessellation
    //   - usually slowest
    //   - partitions the heightfield into nice regions without holes or overlaps
    //   - the are some corner cases where this method creates produces holes and overlaps
    //      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
    //      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
    //   * generally the best choice if you precompute the navmesh, use this if you have large open areas
    // 2) Monotone partioning
    //   - fastest
    //   - partitions the heightfield into regions without holes and overlaps (guaranteed)
    //   - creates long thin polygons, which sometimes causes paths with detours
    //   * use this if you want fast navmesh generation
    // 3) Layer partitoining
    //   - quite fast
    //   - partitions the heighfield into non-overlapping regions
    //   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
    //   - produces better triangles than monotone partitioning
    //   - does not have the corner cases of watershed partitioning
    //   - can be slow and create a bit ugly tessellation (still better than monotone)
    //     if you have large open areas with small obstacles (not a problem if you use tiles)
    //   * good choice to use for tiled navmesh with medium and small sized tiles

    switch (m_PartitionMethod)
    {
        case NavMeshPartition::Watershed:
        {
            // Prepare for region partitioning, by calculating distance field along the walkable surface.
            if (!rcBuildDistanceField(&s_RecastContext, *temporal.CompactHeightfield))
            {
                LOG("Could not build distance field\n");
                return false;
            }

            // Partition the walkable surface into simple regions without holes.
            if (!rcBuildRegions(&s_RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea, config.mergeRegionArea))
            {
                LOG("Could not build watershed regions\n");
                return false;
            }
            break;
        }
        case NavMeshPartition::Monotone:
        {
            // Partition the walkable surface into simple regions without holes.
            // Monotone partitioning does not need distancefield.
            if (!rcBuildRegionsMonotone(&s_RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea, config.mergeRegionArea))
            {
                LOG("Could not build monotone regions\n");
                return false;
            }
            break;
        }
        default:
        {
            // Partition the walkable surface into simple regions without holes.
            if (!rcBuildLayerRegions(&s_RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea))
            {
                LOG("Could not build layer regions\n");
                return false;
            }
        }
    }

    if (m_IsDynamic)
    {
        // Add obstacles inside tile
        struct ObstacleVisitor
        {
            NavMeshInterface* m_Interface;
            BvAxisAlignedBox m_TileBounds;

            void Visit(NavMeshObstacleComponent& obstacle)
            {
                Float3 const& position = obstacle.GetOwner()->GetWorldPosition();
                float radiusSqr = obstacle.GetRadius();

                if (m_TileBounds.GetSquareDistanceToPoint(position) < radiusSqr*radiusSqr)
                {
                    m_Interface->RemoveObstacle(&obstacle);
                    m_Interface->AddObstacle(&obstacle);
                }
            }
        };
        ObstacleVisitor obstacleVisitor;
        obstacleVisitor.m_Interface = this;
        obstacleVisitor.m_TileBounds = tileBounds;
        auto& obstacles = GetWorld()->GetComponentManager<NavMeshObstacleComponent>();
        obstacles.IterateComponents(obstacleVisitor);

        temporal.LayerSet = rcAllocHeightfieldLayerSet();
        if (!temporal.LayerSet)
        {
            LOG("Failed on rcAllocHeightfieldLayerSet\n");
            return false;
        }

        if (!rcBuildHeightfieldLayers(&s_RecastContext, *temporal.CompactHeightfield, config.borderSize, config.walkableHeight, *temporal.LayerSet))
        {
            LOG("Failed on rcBuildHeightfieldLayers\n");
            return false;
        }

        struct TileCacheData
        {
            byte* Data;
            int Size;
        };

        TileCacheData cacheData[MaxAllowedLayers];

        int numLayers = Math::Min(temporal.LayerSet->nlayers, MaxAllowedLayers);
        int numValidLayers = 0;
        for (int i = 0; i < numLayers; ++i)
        {
            TileCacheData* tile = &cacheData[i];
            rcHeightfieldLayer const* layer = &temporal.LayerSet->layers[i];

            dtTileCacheLayerHeader header;
            header.magic   = DT_TILECACHE_MAGIC;
            header.version = DT_TILECACHE_VERSION;
            header.tx      = inX;
            header.ty      = inZ;
            header.tlayer  = i;
            dtVcopy(header.bmin, layer->bmin);
            dtVcopy(header.bmax, layer->bmax);
            header.width  = (unsigned char)layer->width;
            header.height = (unsigned char)layer->height;
            header.minx   = (unsigned char)layer->minx;
            header.maxx   = (unsigned char)layer->maxx;
            header.miny   = (unsigned char)layer->miny;
            header.maxy   = (unsigned char)layer->maxy;
            header.hmin   = (unsigned short)layer->hmin;
            header.hmax   = (unsigned short)layer->hmax;

            dtStatus status = dtBuildTileCacheLayer(&s_TileCompressorCallback, &header, layer->heights, layer->areas, layer->cons, &tile->Data, &tile->Size);
            if (dtStatusFailed(status))
            {
                LOG("Failed on dtBuildTileCacheLayer\n");
                break;
            }

            numValidLayers++;
        }

        int cachedLayerCount = 0;
        //int cacheCompressedSize = 0;
        //int cacheRawSize = 0;
        //const int layerSize = dtAlign4(sizeof(dtTileCacheLayerHeader)) + m_TileSize * m_TileSize * numLayers;
        for (int i = 0; i < numValidLayers; ++i)
        {
            dtCompressedTileRef ref;
            dtStatus status = m_TileCache->addTile(cacheData[i].Data, cacheData[i].Size, DT_COMPRESSEDTILE_FREE_DATA, &ref);
            if (dtStatusFailed(status))
            {
                dtFree(cacheData[i].Data);
                cacheData[i].Data = nullptr;
                continue;
            }

            status = m_TileCache->buildNavMeshTile(ref, m_NavMesh);
            if (dtStatusFailed(status))
                LOG("Failed to build navmesh tile: {}\n", GetErrorStr(status));

            cachedLayerCount++;
            //cacheCompressedSize += cacheData[i].Size;
            //cacheRawSize += layerSize;
        }

        if (cachedLayerCount == 0)
            return false;

        //int buildPeakMemoryUsage = m_LinearAllocator->High;
        //const float compressionRatio = (float)cacheCompressedSize / (float)(cacheRawSize + 1);

        //int tileMemoryUsage = 0;
        //const dtNavMesh* mesh = m_NavMesh;
        //for (int i = 0; i < mesh->getMaxTiles(); ++i)
        //{
        //    const dtMeshTile* tile = mesh->getTile(i);
        //    if (tile->header)
        //        tileMemoryUsage += tile->dataSize;
        //}

        //auto gridSize = gridW * gridH;
        //LOG("Processed navigation data:\n"
        //    "Tile memory usage: {:1f} kB\n"
        //    "Cache compressed size: {:1f} kB\n"
        //    "Cache raw size: {:1f} kB\n"
        //    "Cache compression ratio: {:1f}%%\n"
        //    "Cache layers count: {}\n"
        //    "Cache layers per tile: {:1f}\n"
        //    "Build peak memory usage: {} kB\n",
        //    tileMemoryUsage / 1024.0f,
        //    cacheCompressedSize / 1024.0f,
        //    cacheRawSize / 1024.0f,
        //    compressionRatio * 100.0f,
        //    cachedLayerCount,
        //    (float)cachedLayerCount / gridSize,
        //    buildPeakMemoryUsage);
    }
    else
    {
        temporal.ContourSet = rcAllocContourSet();
        if (!temporal.ContourSet)
        {
            LOG("Failed on rcAllocContourSet\n");
            return false;
        }

        // Trace and simplify region contours.

        // Create contours.
        if (!rcBuildContours(&s_RecastContext, *temporal.CompactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *temporal.ContourSet))
        {
            LOG("Could not create contours\n");
            return false;
        }

        temporal.PolyMesh = rcAllocPolyMesh();
        if (!temporal.PolyMesh)
        {
            LOG("Failed on rcAllocPolyMesh\n");
            return false;
        }

        // Build polygon navmesh from the contours.
        if (!rcBuildPolyMesh(&s_RecastContext, *temporal.ContourSet, config.maxVertsPerPoly, *temporal.PolyMesh))
        {
            LOG("Could not triangulate contours\n");
            return false;
        }

        if (!temporal.PolyMesh->nverts || !temporal.PolyMesh->npolys)
        {
            // no data to build tile
            return true;
        }

        temporal.PolyMeshDetail = rcAllocPolyMeshDetail();
        if (!temporal.PolyMeshDetail)
        {
            LOG("Failed on rcAllocPolyMeshDetail\n");
            return false;
        }

        // Create detail mesh which allows to access approximate height on each polygon.
        if (!rcBuildPolyMeshDetail(&s_RecastContext,
            *temporal.PolyMesh,
            *temporal.CompactHeightfield,
            config.detailSampleDist,
            config.detailSampleMaxError,
            *temporal.PolyMeshDetail))
        {
            LOG("Could not build detail mesh\n");
            return false;
        }

        // At this point the navigation mesh data is ready
        // Create Detour data from poly mesh.

        dtNavMeshCreateParams params = {};
        params.verts            = temporal.PolyMesh->verts;
        params.vertCount        = temporal.PolyMesh->nverts;
        params.polys            = temporal.PolyMesh->polys;
        params.polyAreas        = temporal.PolyMesh->areas;
        params.polyFlags        = temporal.PolyMesh->flags;
        params.polyCount        = temporal.PolyMesh->npolys;
        params.nvp              = temporal.PolyMesh->nvp;
        params.detailMeshes     = temporal.PolyMeshDetail->meshes;
        params.detailVerts      = temporal.PolyMeshDetail->verts;
        params.detailVertsCount = temporal.PolyMeshDetail->nverts;
        params.detailTris       = temporal.PolyMeshDetail->tris;
        params.detailTriCount   = temporal.PolyMeshDetail->ntris;
        params.walkableHeight   = m_WalkableHeight;
        params.walkableRadius   = m_WalkableRadius;
        params.walkableClimb    = m_WalkableClimb;
        params.tileX            = inX;
        params.tileY            = inZ;
        rcVcopy(params.bmin, temporal.PolyMesh->bmin);
        rcVcopy(params.bmax, temporal.PolyMesh->bmax);
        params.cs               = config.cs;
        params.ch               = config.ch;
        params.buildBvTree      = true;

        m_MeshProcess->process(&params, temporal.PolyMesh->areas, temporal.PolyMesh->flags);

        unsigned char* navData = 0;
        int navDataSize = 0;

        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            if (params.vertCount >= 0xffff)
                LOG("Could not build navmesh tile - too many vertices\n");
            else if (params.nvp > DT_VERTS_PER_POLYGON || !params.vertCount || !params.verts || !params.polyCount || !params.polys)
                LOG("Could not build navmesh tile - invalid parameters\n");
            else
                LOG("Could not build navmesh tile - out of memory\n");
            return false;
        }

        dtStatus status = m_NavMesh->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr);
        if (dtStatusFailed(status))
        {
            dtFree(navData);
            LOG("Could not add tile to navmesh\n");
            return false;
        }
    }

    return true;
}

void NavMeshInterface::RegisterArea(NAV_MESH_AREA inAreaType, StringView inName, Color4 const& inVisualizeColor)
{
    HK_IF_NOT_ASSERT(inAreaType < NAV_MESH_AREA_MAX) { return; }

    m_AreaDesc[inAreaType].Name = inName;
    m_AreaDesc[inAreaType].Color = inVisualizeColor.GetDWord();
}

NAV_MESH_AREA NavMeshInterface::GetAreaType(StringView inName) const
{
    uint8_t i = 0;
    for (AreaDesc const& desc : m_AreaDesc)
    {
        if (desc.Name == inName)
            return NAV_MESH_AREA(i);
        ++i;
    }
    LOG("Warning: Undefined area type {}\n", inName);
    return NAV_MESH_AREA_GROUND;
}

String NavMeshInterface::GetAreaName(NAV_MESH_AREA inAreaType) const
{
    HK_IF_NOT_ASSERT(inAreaType < NAV_MESH_AREA_MAX) { return {}; }

    return m_AreaDesc[inAreaType].Name;
}

HK_NAMESPACE_END
