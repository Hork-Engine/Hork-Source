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


#include "NavMesh.h"
#include <Engine/Core/Logger.h>
#include <Engine/Core/Allocators/LinearAllocator.h>
#include <Engine/Core/Compress.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Geometry/BV/BvIntersect.h>

#undef malloc
#undef free

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
#include <float.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawNavMeshBVTree("com_DrawNavMeshBVTree"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMeshNodes("com_DrawNavMeshNodes"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMesh("com_DrawNavMesh"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMeshTileBounds("com_DrawNavMeshTileBounds"s, "0"s, CVAR_CHEAT);

HK_VALIDATE_TYPE_SIZE(NavPolyRef, sizeof(dtPolyRef));

namespace
{

const int MAX_LAYERS = 255;

const bool RECAST_ENABLE_LOGGING = true;
const bool RECAST_ENABLE_TIMINGS = true;

}

struct TileCacheData
{
    byte* Data;
    int Size;
};

struct TileCompressorCallback : public dtTileCacheCompressor
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

struct DetourLinearAllocator : public dtTileCacheAlloc
{
    TLinearAllocator<> Allocator;

    void* operator new(size_t _SizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        Core::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Ptr);
    }

    void reset() override
    {
        Allocator.Reset();
    }

    void* alloc(const size_t _Size) override
    {
        return Allocator.Allocate(_Size);
    }

    void free(void*) override
    {
    }
};

struct DetourMeshProcess : public dtTileCacheMeshProcess
{
    // NavMesh connections
    TVector<Float3> OffMeshConVerts;
    TVector<float> OffMeshConRads;
    TVector<unsigned char> OffMeshConDirs;
    TVector<unsigned char> OffMeshConAreas;
    TVector<unsigned short> OffMeshConFlags;
    TVector<unsigned int> OffMeshConID;
    int OffMeshConCount;
    NavMesh* NavMesh;

    void* operator new(size_t _SizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        Core::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Ptr);
    }

    void process(struct dtNavMeshCreateParams* _Params, unsigned char* _PolyAreas, unsigned short* _PolyFlags) override
    {
        // Update poly flags from areas.
        for (int i = 0; i < _Params->polyCount; ++i)
        {
            if (_PolyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
            {
                _PolyAreas[i] = NAV_MESH_AREA_GROUND;
            }
            if (_PolyAreas[i] == NAV_MESH_AREA_GROUND ||
                _PolyAreas[i] == NAV_MESH_AREA_GRASS ||
                _PolyAreas[i] == NAV_MESH_AREA_ROAD)
            {
                _PolyFlags[i] = NAV_MESH_FLAGS_WALK;
            }
            else if (_PolyAreas[i] == NAV_MESH_AREA_WATER)
            {
                _PolyFlags[i] = NAV_MESH_FLAGS_SWIM;
            }
            else if (_PolyAreas[i] == NAV_MESH_AREA_DOOR)
            {
                _PolyFlags[i] = NAV_MESH_FLAGS_WALK | NAV_MESH_FLAGS_DOOR;
            }
        }

        BvAxisAlignedBox clip_bounds;
        rcVcopy(clip_bounds.Mins.ToPtr(), _Params->bmin);
        rcVcopy(clip_bounds.Maxs.ToPtr(), _Params->bmax);

        OffMeshConVerts.Clear();
        OffMeshConVerts.Clear();
        OffMeshConRads.Clear();
        OffMeshConDirs.Clear();
        OffMeshConAreas.Clear();
        OffMeshConFlags.Clear();
        OffMeshConID.Clear();
        OffMeshConCount = 0;

        const float MARGIN = 0.2f;

        for (int i = 0; i < NavMesh->NavMeshConnections.Size(); i++)
        {
            NavMeshConnection const& con = NavMesh->NavMeshConnections[i];

            BvAxisAlignedBox bounds = con.CalcBoundingBox();
            bounds.Mins -= MARGIN;
            bounds.Maxs += MARGIN;

            if (!BvBoxOverlapBox(clip_bounds, bounds))
            {
                // Connection is outside of clip bounds
                continue;
            }

            OffMeshConVerts.Add(con.StartPosition);
            OffMeshConVerts.Add(con.EndPosition);
            OffMeshConRads.Add(con.Radius);
            OffMeshConDirs.Add(con.bBidirectional ? DT_OFFMESH_CON_BIDIR : 0);
            OffMeshConAreas.Add(con.AreaID);
            OffMeshConFlags.Add(con.Flags);
            OffMeshConID.Add(i); // FIXME?

            OffMeshConCount++;
        }

        // Pass in off-mesh connections.
        _Params->offMeshConVerts = (float*)OffMeshConVerts.ToPtr();
        _Params->offMeshConRad = OffMeshConRads.ToPtr();
        _Params->offMeshConDir = OffMeshConDirs.ToPtr();
        _Params->offMeshConAreas = OffMeshConAreas.ToPtr();
        _Params->offMeshConFlags = OffMeshConFlags.ToPtr();
        _Params->offMeshConUserID = OffMeshConID.ToPtr();
        _Params->offMeshConCount = OffMeshConCount;
    }
};

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


namespace
{

const int MAX_POLYS = 2048;

NavPolyRef TmpPolys[MAX_POLYS];
NavPolyRef TmpPathPolys[MAX_POLYS];
alignas(16) Float3 TmpPathPoints[MAX_POLYS];
unsigned char TmpPathFlags[MAX_POLYS];

Hk::TileCompressorCallback s_TileCompressorCallback;
Hk::RecastContext s_RecastContext;

} // namespace

NavMesh::NavMesh()
{
    m_BoundingBox.Clear();
}

NavMesh::~NavMesh()
{
    Purge();
}

bool NavMesh::Initialize(NavMeshDesc const& inNavigationConfig)
{
    dtStatus status;

    Purge();

    if (inNavigationConfig.BoundingBox.IsEmpty())
    {
        LOG("NavMesh::Initialize: empty bounding box\n");
        return false;
    }

    m_Desc = inNavigationConfig;

    m_BoundingBox = inNavigationConfig.BoundingBox;

    if (m_Desc.VertsPerPoly < 3)
    {
        LOG("NavVertsPerPoly < 3\n");

        m_Desc.VertsPerPoly = 3;
    }
    else if (m_Desc.VertsPerPoly > DT_VERTS_PER_POLYGON)
    {
        LOG("NavVertsPerPoly > NAV_MAX_VERTS_PER_POLYGON\n");

        m_Desc.VertsPerPoly = DT_VERTS_PER_POLYGON;
    }

    if (m_Desc.MaxLayers > MAX_LAYERS)
    {
        LOG("MaxLayers > MAX_LAYERS\n");
        m_Desc.MaxLayers = MAX_LAYERS;
    }

    int grid_w, grid_h;
    rcCalcGridSize(m_BoundingBox.Mins.ToPtr(), m_BoundingBox.Maxs.ToPtr(), m_Desc.CellSize, &grid_w, &grid_h);

    m_NumTilesX = (grid_w + m_Desc.TileSize - 1) / m_Desc.TileSize;
    m_NumTilesZ = (grid_h + m_Desc.TileSize - 1) / m_Desc.TileSize;

    // Max tiles and max polys affect how the tile IDs are caculated.
    // There are 22 bits available for identifying a tile and a polygon.
    const int tile_bits = Math::Min<int>(Math::Log2(Math::ToGreaterPowerOfTwo((uint64_t)m_NumTilesX * m_NumTilesZ)), 14);
    const int max_tiles = 1 << tile_bits;
    const int max_polys_per_tile = 1u << (22 - tile_bits);

    m_TileWidth = m_Desc.TileSize * m_Desc.CellSize;

    dtNavMeshParams params = {};
    rcVcopy(params.orig, m_BoundingBox.Mins.ToPtr());
    params.tileWidth = m_TileWidth;
    params.tileHeight = m_TileWidth;
    params.maxTiles = max_tiles;
    params.maxPolys = max_polys_per_tile;

    m_NavMesh = dtAllocNavMesh();
    if (!m_NavMesh)
    {
        Purge();
        LOG("Failed on dtAllocNavMesh\n");
        return false;
    }

    status = m_NavMesh->init(&params);
    if (dtStatusFailed(status))
    {
        Purge();
        LOG("Could not initialize navmesh\n");
        return false;
    }

    m_NavQuery = dtAllocNavMeshQuery();
    if (!m_NavQuery)
    {
        Purge();
        LOG("Failed on dtAllocNavMeshQuery\n");
        return false;
    }

    const int MAX_NODES = 2048;
    status = m_NavQuery->init(m_NavMesh, MAX_NODES);
    if (dtStatusFailed(status))
    {
        Purge();
        LOG("Could not initialize navmesh query");
        return false;
    }

    if (m_Desc.bDynamicNavMesh)
    {
        dtTileCacheParams tile_cache_params;
        Core::ZeroMem(&tile_cache_params, sizeof(tile_cache_params));
        rcVcopy(tile_cache_params.orig, m_Desc.BoundingBox.Mins.ToPtr());
        tile_cache_params.cs = m_Desc.CellSize;
        tile_cache_params.ch = m_Desc.CellHeight;
        tile_cache_params.width = m_Desc.TileSize;
        tile_cache_params.height = m_Desc.TileSize;
        tile_cache_params.walkableHeight = m_Desc.WalkableHeight;
        tile_cache_params.walkableRadius = m_Desc.WalkableRadius;
        tile_cache_params.walkableClimb = m_Desc.WalkableClimb;
        tile_cache_params.maxSimplificationError = m_Desc.EdgeMaxError;
        tile_cache_params.maxTiles = max_tiles * m_Desc.MaxLayers;
        tile_cache_params.maxObstacles = m_Desc.MaxDynamicObstacles;

        m_TileCache = dtAllocTileCache();
        if (!m_TileCache)
        {
            Purge();
            LOG("Failed on dtAllocTileCache\n");
            return false;
        }

        m_LinearAllocator = MakeUnique<DetourLinearAllocator>();

        m_MeshProcess = MakeUnique<DetourMeshProcess>();
        m_MeshProcess->NavMesh = this;

        status = m_TileCache->init(&tile_cache_params, m_LinearAllocator.RawPtr(), &s_TileCompressorCallback, m_MeshProcess.RawPtr());
        if (dtStatusFailed(status))
        {
            Purge();
            LOG("Could not initialize tile cache\n");
            return false;
        }

        // TODO: Add obstacles here?
    }

    return true;
}

bool NavMesh::Build()
{
    Int2 mins(0, 0);
    Int2 maxs(m_NumTilesX - 1, m_NumTilesZ - 1);

    return Build(mins, maxs);
}

bool NavMesh::Build(Int2 const& inMins, Int2 const& inMaxs)
{
    if (!m_NavMesh)
    {
        LOG("NavMesh::Build: navmesh must be initialized\n");
        return false;
    }

    Int2 clamped_mins;
    Int2 clamped_maxs;

    clamped_mins.X = Math::Clamp<int>(inMins.X, 0, m_NumTilesX - 1);
    clamped_mins.Y = Math::Clamp<int>(inMins.Y, 0, m_NumTilesZ - 1);
    clamped_maxs.X = Math::Clamp<int>(inMaxs.X, 0, m_NumTilesX - 1);
    clamped_maxs.Y = Math::Clamp<int>(inMaxs.Y, 0, m_NumTilesZ - 1);

    unsigned int count = 0;
    for (int z = clamped_mins[1]; z <= clamped_maxs[1]; z++)
        for (int x = clamped_mins[0]; x <= clamped_maxs[0]; x++)
            if (BuildTile(x, z))
                count++;
    return count > 0;
}

bool NavMesh::Build(BvAxisAlignedBox const& inBoundingBox)
{
    Int2 mins((inBoundingBox.Mins.X - m_BoundingBox.Mins.X) / m_TileWidth,
              (inBoundingBox.Mins.Z - m_BoundingBox.Mins.Z) / m_TileWidth);
    Int2 maxs((inBoundingBox.Maxs.X - m_BoundingBox.Mins.X) / m_TileWidth,
              (inBoundingBox.Maxs.Z - m_BoundingBox.Mins.Z) / m_TileWidth);

    return Build(mins, maxs);
}

BvAxisAlignedBox NavMesh::GetTileWorldBounds(int inX, int inZ) const
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

namespace
{

// Based on rcMarkWalkableTriangles
void MarkWalkableTriangles(float inSlopeAngleDeg, Float3 const* inVertices, unsigned int const* inIndices, int inTriangleCount, int inFirstTriangle, TBitMask<> const& inWalkableMask, unsigned char* outAreas)
{
    Float3 perpendicular;
    float perpendicular_length;

    const float threshold = Math::Cos(Math::Radians(inSlopeAngleDeg));

    for (int i = 0; i < inTriangleCount; ++i)
    {
        int triangle = inFirstTriangle + i;
        if (inWalkableMask.IsMarked(triangle))
        {
            unsigned int const* tri = &inIndices[triangle * 3];

            perpendicular = Math::Cross(inVertices[tri[1]] - inVertices[tri[0]], inVertices[tri[2]] - inVertices[tri[0]]);
            perpendicular_length = perpendicular.Length();
            if (perpendicular_length > 0 && perpendicular[1] > threshold * perpendicular_length)
            {
                outAreas[i] = RC_WALKABLE_AREA;
            }
        }
    }
}

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
        s.Resize(s.Length() - 1);
    return s;
}

} // namespace

bool NavMesh::BuildTile(int inX, int inZ)
{
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

    BvAxisAlignedBox tile_bounds;
    BvAxisAlignedBox tile_bounds_with_pad;

    HK_ASSERT(m_NavMesh);

    RemoveTile(inX, inZ);

    tile_bounds = GetTileWorldBounds(inX, inZ);

    rcConfig config = {};
    config.cs = m_Desc.CellSize;
    config.ch = m_Desc.CellHeight;
    config.walkableSlopeAngle = m_Desc.WalkableSlopeAngle;
    config.walkableHeight = (int)Math::Ceil(m_Desc.WalkableHeight / config.ch);
    config.walkableClimb = (int)Math::Floor(m_Desc.WalkableClimb / config.ch);
    config.walkableRadius = (int)Math::Ceil(m_Desc.WalkableRadius / config.cs);
    config.maxEdgeLen = (int)(m_Desc.EdgeMaxLength / m_Desc.CellSize);
    config.maxSimplificationError = m_Desc.EdgeMaxError;
    config.minRegionArea = (int)rcSqr(m_Desc.MinRegionSize);        // Note: area = size*size
    config.mergeRegionArea = (int)rcSqr(m_Desc.MergeRegionSize); // Note: area = size*size
    config.detailSampleDist = m_Desc.DetailSampleDist < 0.9f ? 0 : m_Desc.CellSize * m_Desc.DetailSampleDist;
    config.detailSampleMaxError = m_Desc.CellHeight * m_Desc.DetailSampleMaxError;
    config.tileSize = m_Desc.TileSize;
    config.borderSize = config.walkableRadius + 3; // radius + padding
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.maxVertsPerPoly = m_Desc.VertsPerPoly;

    rcVcopy(config.bmin, tile_bounds.Mins.ToPtr());
    rcVcopy(config.bmax, tile_bounds.Maxs.ToPtr());

    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    for (int i = 0; i < 3; i++)
    {
        tile_bounds_with_pad.Mins[i] = config.bmin[i];
        tile_bounds_with_pad.Maxs[i] = config.bmax[i];
    }

    NavigationGeometry geometry;
    geometry.pClipBoundingBox = &tile_bounds_with_pad;
    geometry.BoundingBox.Clear();
    GatherNavigationGeometry(geometry);

    // Empty tile
    if (geometry.BoundingBox.IsEmpty() || geometry.Indices.IsEmpty())
        return true;

    //---------TEST-----------
    //walkableMask.Resize(indices.Size()/3);
    //walkableMask.MarkAll();
    //------------------------

    //rcVcopy( config.bmin, geometry.BoundingBox.Mins.ToPtr() );
    //rcVcopy( config.bmax, geometry.BoundingBox.Maxs.ToPtr() );
    config.bmin[1] = geometry.BoundingBox.Mins.Y;
    config.bmax[1] = geometry.BoundingBox.Maxs.Y;
    tile_bounds_with_pad = geometry.BoundingBox;

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

    int triangle_count = geometry.Indices.Size() / 3;

    // Allocate array that can hold triangle area types.
    // If you have multiple meshes you need to process, allocate
    // and array which can hold the max number of triangles you need to process.
    unsigned char* triangle_area_types = (unsigned char*)Core::GetHeapAllocator<HEAP_TEMP>().Alloc(triangle_count, 16, MALLOC_ZERO);

    // Find triangles which are walkable based on their slope and rasterize them.
    // If your input data is multiple meshes, you can transform them here, calculate
    // the are type for each of the meshes and rasterize them.
    MarkWalkableTriangles(config.walkableSlopeAngle, geometry.Vertices.ToPtr(), geometry.Indices.ToPtr(), triangle_count, 0, geometry.WalkableMask, triangle_area_types);

    bool rasterized = rcRasterizeTriangles(&s_RecastContext,
                                           (float const*)geometry.Vertices.ToPtr(),
                                           geometry.Vertices.Size(),
                                           (int const*)geometry.Indices.ToPtr(),
                                           triangle_area_types,
                                           triangle_count,
                                           *temporal.Heightfield,
                                           config.walkableClimb);

    Core::GetHeapAllocator<HEAP_TEMP>().Free(triangle_area_types);

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
        LOG("NavMesh::Build: Failed on rcErodeWalkableArea\n");
        return false;
    }
#if 1
    BvAxisAlignedBox area_bounds;
    for (int area_num = 0; area_num < NavigationAreas.Size(); ++area_num)
    {
        NavMeshArea const& area = NavigationAreas[area_num];
        rcCompactHeightfield const& chf = *temporal.CompactHeightfield;

        area_bounds = area.CalcBoundingBox();
        if (area_bounds.IsEmpty())
        {
            // Invalid bounding box
            continue;
        }

        if (!BvBoxOverlapBox(tile_bounds_with_pad, area_bounds))
        {
            // Area is outside of tile bounding box
            continue;
        }

        // The next code is based on rcMarkBoxArea and rcMarkConvexPolyArea
        int minx = (int)((area_bounds.Mins[0] - chf.bmin[0]) / chf.cs);
        int miny = (int)((area_bounds.Mins[1] - chf.bmin[1]) / chf.ch);
        int minz = (int)((area_bounds.Mins[2] - chf.bmin[2]) / chf.cs);
        int maxx = (int)((area_bounds.Maxs[0] - chf.bmin[0]) / chf.cs);
        int maxy = (int)((area_bounds.Maxs[1] - chf.bmin[1]) / chf.ch);
        int maxz = (int)((area_bounds.Maxs[2] - chf.bmin[2]) / chf.cs);

        if (maxx < 0) continue;
        if (minx >= chf.width) continue;
        if (maxz < 0) continue;
        if (minz >= chf.height) continue;

        if (minx < 0) minx = 0;
        if (maxx >= chf.width) maxx = chf.width - 1;
        if (minz < 0) minz = 0;
        if (maxz >= chf.height) maxz = chf.height - 1;

        switch (area.Shape)
        {
            case NAV_MESH_AREA_SHAPE_BOX:
                for (int z = minz; z <= maxz; ++z)
                {
                    for (int x = minx; x <= maxx; ++x)
                    {
                        const rcCompactCell& c = chf.cells[x + z * chf.width];
                        for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
                        {
                            rcCompactSpan& s = chf.spans[i];
                            if ((int)s.y >= miny && (int)s.y <= maxy)
                            {
                                if (chf.areas[i] != RC_NULL_AREA)
                                    chf.areas[i] = area.AreaID;
                            }
                        }
                    }
                }
                break;
            case NAV_MESH_AREA_SHAPE_CONVEX_VOLUME:
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
                                p[0] = chf.bmin[0] + (x + 0.5f) * chf.cs;
                                p[1] = chf.bmin[2] + (z + 0.5f) * chf.cs;

                                if (PointInPoly2D(area.NumConvexVolumeVerts, area.ConvexVolume[0].ToPtr(), p))
                                {
                                    chf.areas[i] = area.AreaID;
                                }
                            }
                        }
                    }
                }
                break;
        }
    }
#endif
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

    if (m_Desc.RecastPartitionMethod == NAV_MESH_PARTITION_WATERSHED)
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
    }
    else if (m_Desc.RecastPartitionMethod == NAV_MESH_PARTITION_MONOTONE)
    {
        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distancefield.
        if (!rcBuildRegionsMonotone(&s_RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea, config.mergeRegionArea))
        {
            LOG("Could not build monotone regions\n");
            return false;
        }
    }
    else
    { // RECAST_PARTITION_LAYERS
        // Partition the walkable surface into simple regions without holes.
        if (!rcBuildLayerRegions(&s_RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea))
        {
            LOG("Could not build layer regions\n");
            return false;
        }
    }

    if (m_Desc.bDynamicNavMesh)
    {
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

        TileCacheData cache_data[MAX_LAYERS];

        int num_layers = Math::Min(temporal.LayerSet->nlayers, MAX_LAYERS);
        int num_valid_layers = 0;
        for (int i = 0; i < num_layers; i++)
        {
            TileCacheData* tile = &cache_data[i];
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

            num_valid_layers++;
        }

        int cached_layer_count = 0;
        //int cache_compressed_size = 0;
        //int cache_raw_size = 0;
        //const int layer_size = dtAlign4(sizeof(dtTileCacheLayerHeader)) + m_Desc.NavTileSize * m_Desc.NavTileSize * num_layers;
        for (int i = 0; i < num_valid_layers; i++)
        {
            dtCompressedTileRef ref;
            dtStatus status = m_TileCache->addTile(cache_data[i].Data, cache_data[i].Size, DT_COMPRESSEDTILE_FREE_DATA, &ref);
            if (dtStatusFailed(status))
            {
                dtFree(cache_data[i].Data);
                cache_data[i].Data = nullptr;
                continue;
            }

            status = m_TileCache->buildNavMeshTile(ref, m_NavMesh);
            if (dtStatusFailed(status))
                LOG("Failed to build navmesh tile {}\n", GetErrorStr(status));

            cached_layer_count++;
            //cache_compressed_size += cache_data[i].Size;
            //cache_raw_size += layer_size;
        }

        if (cached_layer_count == 0)
            return false;

        //        int build_peak_memory_usage = LinearAllocator->High;
        //        const float compression_ratio = (float)cache_compressed_size / (float)(cache_raw_size+1);

        //        int time_memory_usage = 0;
        //        const dtNavMesh * mesh = m_NavMesh;
        //        for ( int i = 0 ; i < mesh->getMaxTiles() ; ++i )
        //        {
        //            const dtMeshTile * tile = mesh->getTile( i );
        //            if (tile->header)
        //                time_memory_usage += tile->dataSize;
        //        }
        //        GLogger.Printf( "Processed navigation data:\n"
        //                        "Total memory usage: %.1f kB\n"
        //                        "Cache compressed size: %.1f kB\n"
        //                        "Cache raw size: %.1f kB\n"
        //                        "Cache compression ratio: %.1f%%\n"
        //                        "Cache layers count: %d\n"
        //                        "Cache layers per tile: %.1f\n"
        //                        "Build peak memory usage: %d kB\n",
        //                        time_memory_usage/1024.0f,
        //                        cache_compressed_size/1024.0f,
        //                        cache_raw_size/1024.0f,
        //                        compression_ratio*100.0f,
        //                        cacheayerCount,
        //                        (float)cached_layer_count/gridSize,
        //                        build_peak_memory_usage
        //                        );
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
        // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

        // Update poly flags from areas.
        static_assert(NAV_MESH_AREA_GROUND == RC_WALKABLE_AREA, "Navmesh area id static check");
        for (int i = 0; i < temporal.PolyMesh->npolys; ++i)
        {
            if (temporal.PolyMesh->areas[i] == NAV_MESH_AREA_GROUND ||
                temporal.PolyMesh->areas[i] == NAV_MESH_AREA_GRASS ||
                temporal.PolyMesh->areas[i] == NAV_MESH_AREA_ROAD)
            {
                temporal.PolyMesh->flags[i] = NAV_MESH_FLAGS_WALK;
            }
            else if (temporal.PolyMesh->areas[i] == NAV_MESH_AREA_WATER)
            {
                temporal.PolyMesh->flags[i] = NAV_MESH_FLAGS_SWIM;
            }
            else if (temporal.PolyMesh->areas[i] == NAV_MESH_AREA_DOOR)
            {
                temporal.PolyMesh->flags[i] = NAV_MESH_FLAGS_WALK | NAV_MESH_FLAGS_DOOR;
            }
        }

        const float MARGIN = 0.2f;

        TVector<Float3> offmesh_con_verts;
        TVector<float> offmesh_con_rads;
        TVector<unsigned char> offmesh_con_dirs;
        TVector<unsigned char> offmesh_con_areas;
        TVector<unsigned short> offmesh_con_flags;
        TVector<unsigned int> offmesh_con_id;
        int offmesh_con_count = 0;

        for (int i = 0; i < NavMeshConnections.Size(); i++)
        {
            NavMeshConnection const& con = NavMeshConnections[i];

            BvAxisAlignedBox bounds = con.CalcBoundingBox();
            bounds.Mins -= MARGIN;
            bounds.Maxs += MARGIN;

            // Connection is outside of tile bounding box
            if (!BvBoxOverlapBox(tile_bounds_with_pad, bounds))
                continue;

            offmesh_con_verts.Add(con.StartPosition);
            offmesh_con_verts.Add(con.EndPosition);
            offmesh_con_rads.Add(con.Radius);
            offmesh_con_dirs.Add(con.bBidirectional ? DT_OFFMESH_CON_BIDIR : 0);
            offmesh_con_areas.Add(con.AreaID);
            offmesh_con_flags.Add(con.Flags);
            offmesh_con_id.Add(i); // FIXME?

            offmesh_con_count++;
        }

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
        params.offMeshConVerts  = (float*)offmesh_con_verts.ToPtr();
        params.offMeshConRad    = offmesh_con_rads.ToPtr();
        params.offMeshConDir    = offmesh_con_dirs.ToPtr();
        params.offMeshConAreas  = offmesh_con_areas.ToPtr();
        params.offMeshConFlags  = offmesh_con_flags.ToPtr();
        params.offMeshConUserID = offmesh_con_id.ToPtr();
        params.offMeshConCount  = offmesh_con_count;
        params.walkableHeight   = m_Desc.WalkableHeight;
        params.walkableRadius   = m_Desc.WalkableRadius;
        params.walkableClimb    = m_Desc.WalkableClimb;
        params.tileX            = inX;
        params.tileY            = inZ;
        rcVcopy(params.bmin, temporal.PolyMesh->bmin);
        rcVcopy(params.bmax, temporal.PolyMesh->bmax);
        params.cs               = config.cs;
        params.ch               = config.ch;
        params.buildBvTree      = true;

        unsigned char* nav_data = 0;
        int nav_data_size = 0;

        if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size))
        {
            if (params.vertCount >= 0xffff)
                LOG("vertCount >= 0xffff\n");

            LOG("Could not build navmesh tile\n");
            return false;
        }

        dtStatus status = m_NavMesh->addTile(nav_data, nav_data_size, DT_TILE_FREE_DATA, 0, nullptr);
        if (dtStatusFailed(status))
        {
            dtFree(nav_data);
            LOG("Could not add tile to navmesh\n");
            return false;
        }
    }

    return true;
}

void NavMesh::RemoveTile(int inX, int inZ)
{
    if (!m_NavMesh)
        return;

    if (m_Desc.bDynamicNavMesh)
    {
        HK_ASSERT(m_TileCache);

        dtCompressedTileRef compressed_tiles[MAX_LAYERS];
        int count = m_TileCache->getTilesAt(inX, inZ, compressed_tiles, m_Desc.MaxLayers);
        for (int i = 0; i < count; i++)
        {
            byte* data = nullptr;
            dtStatus status = m_TileCache->removeTile(compressed_tiles[i], &data, nullptr);
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

void NavMesh::RemoveTiles()
{
    if (!m_NavMesh)
        return;

    if (m_Desc.bDynamicNavMesh)
    {
        HK_ASSERT(m_TileCache);

        int tile_count = m_TileCache->getTileCount();
        for (int i = 0; i < tile_count; i++)
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
        int tile_count = m_NavMesh->getMaxTiles();
        const dtNavMesh* nav_mesh = m_NavMesh;
        for (int i = 0; i < tile_count; i++)
        {
            const dtMeshTile* tile = nav_mesh->getTile(i);
            if (tile && tile->header)
            {
                m_NavMesh->removeTile(m_NavMesh->getTileRef(tile), nullptr, nullptr);
            }
        }
    }
}

void NavMesh::RemoveTiles(Int2 const& inMins, Int2 const& inMaxs)
{
    if (!m_NavMesh)
        return;

    for (int z = inMins[1]; z <= inMaxs[1]; z++)
        for (int x = inMins[0]; x <= inMaxs[0]; x++)
            RemoveTile(x, z);
}

bool NavMesh::IsTileExsist(int inX, int inZ) const
{
    return m_NavMesh ? m_NavMesh->getTileAt(inX, inZ, 0) != nullptr : false;
}

void NavMesh::AddObstacle(NavMeshObstacle* inObstacle)
{
    if (!m_TileCache)
        return;

    dtObstacleRef ref;
    dtStatus status = DT_FAILURE;

    // TODO:
    //while (m_TileCache->isObstacleQueueFull())
    //    m_TileCache->update( 1, m_NavMesh );

    switch (inObstacle->Shape)
    {
        case NAV_MESH_OBSTACLE_BOX:
            status = m_TileCache->addBoxObstacle((inObstacle->Position - inObstacle->HalfExtents).ToPtr(),
                                                 (inObstacle->Position + inObstacle->HalfExtents).ToPtr(), &ref);
            break;
        case NAV_MESH_OBSTACLE_CYLINDER:
            while (1)
            {
                status = m_TileCache->addObstacle(inObstacle->Position.ToPtr(), inObstacle->Radius, inObstacle->Height, &ref);
                if (!(status & DT_BUFFER_TOO_SMALL))
                    break;
                m_TileCache->update(1, m_NavMesh);
            }
            break;
    }

    if (dtStatusFailed(status))
    {
        LOG("Failed to add navmesh obstacle\n");
        if (status & DT_OUT_OF_MEMORY)
            LOG("DT_OUT_OF_MEMORY\n");
        return;
    }
    LOG("AddObstacle: {}\n", ref);
    inObstacle->ObstacleRef = ref;
}

void NavMesh::RemoveObstacle(NavMeshObstacle* inObstacle)
{
    if (!m_TileCache)
        return;

    if (!inObstacle->ObstacleRef)
        return;

    // TODO:
    //while (m_TileCache->isObstacleQueueFull())
    //    m_TileCache->update( 1, m_NavMesh );

    dtStatus status;

    while (1)
    {
        status = m_TileCache->removeObstacle(inObstacle->ObstacleRef);
        if (!(status & DT_BUFFER_TOO_SMALL))
            break;
        m_TileCache->update(1, m_NavMesh);
    }

    if (dtStatusFailed(status))
    {
        LOG("Failed to remove navmesh obstacle\n");
        return;
    }

    inObstacle->ObstacleRef = 0;
}

void NavMesh::UpdateObstacle(NavMeshObstacle* inObstacle)
{
    if (!inObstacle->ObstacleRef)
    {
        LOG("NavMesh::UpdateObstacle: obstacle is not in navmesh\n");
        return;
    }

    RemoveObstacle(inObstacle);
    AddObstacle(inObstacle);
}

void NavMesh::Purge()
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

struct DebugDrawCallback : public duDebugDraw
{
    DebugRenderer*        DD;
    Float3                AccumVertices[3];
    int                   AccumIndex;
    duDebugDrawPrimitives Primitive;

    DebugDrawCallback()
    {
        AccumIndex = 0;
    }

    void depthMask(bool state) override
    {
        DD->SetDepthTest(state);
    }

    void texture(bool state) override
    {
    }

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

    void end() override
    {
    }
};

void NavMesh::DrawDebug(DebugRenderer* inRenderer)
{
    if (!m_NavMesh)
        return;

    DebugDrawCallback callback;
    callback.DD = inRenderer;

    if (com_DrawNavMeshBVTree)
        duDebugDrawNavMeshBVTree(&callback, *m_NavMesh);

    if (com_DrawNavMeshNodes)
        duDebugDrawNavMeshNodes(&callback, *m_NavQuery);

    if (com_DrawNavMesh)
        duDebugDrawNavMeshWithClosedList(&callback, *m_NavMesh, *m_NavQuery, DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST | DU_DRAWNAVMESH_COLOR_TILES);
    //duDebugDrawNavMeshPolysWithFlags( &callback, *m_NavMesh, NAV_MESH_FLAGS_DISABLED/*NAV_MESH_FLAGS_DISABLED*/, duRGBA(0,0,0,128));
    //duDebugDrawNavMeshPolysWithFlags( &callback, *m_NavMesh, 0/*NAV_MESH_FLAGS_DISABLED*/, duRGBA(0,0,0,128));


#if 0
    if ( RVDrawNavMeshTileBounds && m_TileCache ) {
        float bmin[3], bmax[3];
        for ( int i = 0 ; i < m_TileCache->getTileCount() ; ++i ) {
            const dtCompressedTile* tile = m_TileCache->getTile(i);

            if ( !tile->header )
                continue;

            m_TileCache->calcTightTileBounds( tile->header, bmin, bmax );

            const unsigned int col = duIntToCol( i, 255 );
            const float pad = m_TileCache->getParams()->cs * 0.1f;

            duDebugDrawBoxWire( &callback, bmin[0]-pad, bmin[1]-pad,bmin[2]-pad,
                bmax[0]+pad, bmax[1]+pad,bmax[2]+pad, col, 2.0f);
        }
    }
#endif

    if (com_DrawNavMeshTileBounds)
    {
        inRenderer->SetDepthTest(false);
        inRenderer->SetColor(Color4(1, 1, 1, 1));
        for (int z = 0; z < m_NumTilesZ; z++)
            for (int x = 0; x < m_NumTilesX; x++)
                if (IsTileExsist(x, z))
                    inRenderer->DrawAABB(GetTileWorldBounds(x, z));
    }

#if 0
    static TVector< Float3 > vertices;
    static TVector< unsigned int > indices;
    static BvAxisAlignedBox dummyBoundingBox;
    static TBitMask<> walkableMask;
    static bool gen=false;
    if (!gen) {
        GatherNavigationGeometry( vertices, indices, walkableMask, dummyBoundingBox, &BoundingBox );
        //gen=true;
    }
    inRenderer->SetDepthTest(true);
    inRenderer->DrawTriangleSoup(vertices,indices,false);
#endif
}

class NavQueryFilterPrivate : public dtQueryFilter
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return Core::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        Core::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Ptr);
    }
};

NavQueryFilter::NavQueryFilter()
{
    m_Filter = MakeUnique<NavQueryFilterPrivate>();
}

NavQueryFilter::~NavQueryFilter()
{
}

void NavQueryFilter::SetAreaCost(int inAreaID, float inCost)
{
    m_Filter->setAreaCost(inAreaID, inCost);
}

float NavQueryFilter::GetAreaCost(int inAreaID) const
{
    return m_Filter->getAreaCost(inAreaID);
}

void NavQueryFilter::SetIncludeFlags(unsigned short inFlags)
{
    m_Filter->setIncludeFlags(inFlags);
}

unsigned short NavQueryFilter::GetIncludeFlags() const
{
    return m_Filter->getIncludeFlags();
}

void NavQueryFilter::SetExcludeFlags(unsigned short inFlags)
{
    m_Filter->setExcludeFlags(inFlags);
}

unsigned short NavQueryFilter::GetExcludeFlags() const
{
    return m_Filter->getExcludeFlags();
}

bool NavMesh::CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavMeshRayCastResult& outResult, NavQueryFilter const& inFilter) const
{
    NavPolyRef poly_ref;

    if (!QueryNearestPoly(inRayStart, inExtents, &poly_ref))
        return false;

    auto status = m_NavQuery->raycast(poly_ref, inRayStart.ToPtr(), inRayEnd.ToPtr(), inFilter.m_Filter.RawPtr(), &outResult.Fraction, outResult.Normal.ToPtr(), TmpPolys, nullptr, MAX_POLYS);
    if (dtStatusFailed(status))
        return false;

    return outResult.Fraction != FLT_MAX;
}

bool NavMesh::CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavMeshRayCastResult& outResult) const
{
    return CastRay(inRayStart, inRayEnd, inExtents, outResult, QueryFilter);
}

bool NavMesh::QueryTileLocaction(Float3 const& inPosition, int* outTileX, int* outTileY) const
{
    if (!m_NavMesh)
    {
        *outTileX = *outTileY = 0;
        return false;
    }

    m_NavMesh->calcTileLoc(inPosition.ToPtr(), outTileX, outTileY);
    return true;
}

bool NavMesh::QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPolyRef* outNearestPolyRef) const
{
    *outNearestPolyRef = 0;

    if (!m_NavQuery)
        return false;

    auto status = m_NavQuery->findNearestPoly(inPosition.ToPtr(), inExtents.ToPtr(), inFilter.m_Filter.RawPtr(), outNearestPolyRef, NULL);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavPolyRef* outNearestPolyRef) const
{
    return QueryNearestPoly(inPosition, inExtents, QueryFilter, outNearestPolyRef);
}

bool NavMesh::QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef* outNearestPointRef) const
{
    outNearestPointRef->PolyRef = 0;
    outNearestPointRef->Position.Clear();

    if (!m_NavQuery)
        return false;
    
    auto status = m_NavQuery->findNearestPoly(inPosition.ToPtr(), inExtents.ToPtr(), inFilter.m_Filter.RawPtr(), &outNearestPointRef->PolyRef, outNearestPointRef->Position.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavPointRef* outNearestPointRef) const
{
    return QueryNearestPoint(inPosition, inExtents, QueryFilter, outNearestPointRef);
}

namespace
{
float NavRandom()
{
    return GameApplication::GetRandom().GetFloat();
}
} // namespace

bool NavMesh::QueryRandomPoint(NavQueryFilter const& inFilter, NavPointRef* outRandomPointRef) const
{
    outRandomPointRef->PolyRef = 0;
    outRandomPointRef->Position.Clear();

    if (!m_NavQuery)
        return false;

    auto status = m_NavQuery->findRandomPoint(inFilter.m_Filter.RawPtr(), NavRandom, &outRandomPointRef->PolyRef, outRandomPointRef->Position.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::QueryRandomPoint(NavPointRef* outRandomPointRef) const
{
    return QueryRandomPoint(QueryFilter, outRandomPointRef);
}

bool NavMesh::QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef* outRandomPointRef) const
{
    NavPointRef point_ref;

    if (!QueryNearestPoly(inPosition, inExtents, inFilter, &point_ref.PolyRef))
        return false;

    point_ref.Position = inPosition;

    return QueryRandomPointAroundCircle(point_ref, inRadius, inFilter, outRandomPointRef);
}

bool NavMesh::QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavPointRef* outRandomPointRef) const
{
    return QueryRandomPointAroundCircle(inPosition, inRadius, inExtents, QueryFilter, outRandomPointRef);
}

bool NavMesh::QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavPointRef* outRandomPointRef) const
{
    outRandomPointRef->PolyRef = 0;
    outRandomPointRef->Position.Clear();

    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->findRandomPointAroundCircle(inPointRef.PolyRef, inPointRef.Position.ToPtr(), inRadius, inFilter.m_Filter.RawPtr(), NavRandom, &outRandomPointRef->PolyRef, outRandomPointRef->Position.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavPointRef* outRandomPointRef) const
{
    return QueryRandomPointAroundCircle(inPointRef, inRadius, QueryFilter, outRandomPointRef);
}

bool NavMesh::QueryClosestPointOnPoly(NavPointRef const& inPointRef, Float3* outPoint, bool* outOverPolygon) const
{
    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->closestPointOnPoly(inPointRef.PolyRef, inPointRef.Position.ToPtr(), outPoint->ToPtr(), outOverPolygon);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::QueryClosestPointOnPolyBoundary(NavPointRef const& inPointRef, Float3* outPoint) const
{
    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->closestPointOnPolyBoundary(inPointRef.PolyRef, inPointRef.Position.ToPtr(), outPoint->ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavQueryFilter const& inFilter, NavPolyRef* outVisited, int* outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const
{
    if (!m_NavQuery)
        return false;

    inMaxVisitedSize = Math::Max(inMaxVisitedSize, 0);

    dtStatus status = m_NavQuery->moveAlongSurface(inPointRef.PolyRef, inPointRef.Position.ToPtr(), inDestination.ToPtr(), inFilter.m_Filter.RawPtr(), outResultPos.ToPtr(), outVisited, outVisitedCount, inMaxVisitedSize);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavPolyRef* outVisited, int* outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const
{
    return MoveAlongSurface(inPointRef, inDestination, QueryFilter, outVisited, outVisitedCount, inMaxVisitedSize, outResultPos);
}

bool NavMesh::MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, NavQueryFilter const& inFilter, int inMaxVisitedSize, Float3& outResultPos) const
{
    NavPointRef point_ref;

    m_LastVisitedPolys.Clear();

    if (!QueryNearestPoly(inPosition, inExtents, inFilter, &point_ref.PolyRef))
        return false;

    point_ref.Position = inPosition;

    m_LastVisitedPolys.Resize(Math::Max(inMaxVisitedSize, 0));

    int visited_count = 0;
    if (!MoveAlongSurface(point_ref, inDestination, inFilter, m_LastVisitedPolys.ToPtr(), &visited_count, m_LastVisitedPolys.Size(), outResultPos))
    {
        m_LastVisitedPolys.Clear();
        return false;
    }

    m_LastVisitedPolys.Resize(visited_count);

    return true;
}

bool NavMesh::MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, int inMaxVisitedSize, Float3& outResultPos) const
{
    return MoveAlongSurface(inPosition, inDestination, inExtents, QueryFilter, inMaxVisitedSize, outResultPos);
}

bool NavMesh::FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavQueryFilter const& inFilter, NavPolyRef* outPath, int* outPathCount, const int inMaxPath) const
{
    *outPathCount = 0;

    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->findPath(inStartRef.PolyRef, inEndRef.PolyRef, inStartRef.Position.ToPtr(), inEndRef.Position.ToPtr(), inFilter.m_Filter.RawPtr(), outPath, outPathCount, inMaxPath);
    if (dtStatusFailed(status))
    {
        *outPathCount = 0;
        return false;
    }

    return true;
}

bool NavMesh::FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavPolyRef* outPath, int* outPathCount, const int inMaxPath) const
{
    return FindPath(inStartRef, inEndRef, QueryFilter, outPath, outPathCount, inMaxPath);
}

bool NavMesh::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, TVector<NavMeshPathPoint>& outPathPoints) const
{
    NavPointRef start_ref;
    NavPointRef end_ref;

    if (!QueryNearestPoly(inStartPos, inExtents, inFilter, &start_ref.PolyRef))
        return false;

    if (!QueryNearestPoly(inEndPos, inExtents, inFilter, &end_ref.PolyRef))
        return false;

    start_ref.Position = inStartPos;
    end_ref.Position = inEndPos;

    int num_polys;
    if (!FindPath(start_ref, end_ref, inFilter, TmpPolys, &num_polys, MAX_POLYS))
        return false;

    Float3 closest_local_end = inEndPos;

    // Если не удалось проложить полный путь, установить конечную точку ближайшую к указанной конечной точке
    if (TmpPolys[num_polys - 1] != end_ref.PolyRef)
        m_NavQuery->closestPointOnPoly(TmpPolys[num_polys - 1], inEndPos.ToPtr(), closest_local_end.ToPtr(), 0);

    int path_len;
    m_NavQuery->findStraightPath(inStartPos.ToPtr(), closest_local_end.ToPtr(), TmpPolys, num_polys, TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &path_len, MAX_POLYS);

    outPathPoints.Resize(path_len);
    for (int i = 0; i < path_len; ++i)
    {
        outPathPoints[i].Position = TmpPathPoints[i];
        outPathPoints[i].Flags = TmpPathFlags[i];
    }

    return true;
}

bool NavMesh::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, TVector<NavMeshPathPoint>& outPathPoints) const
{
    return FindPath(inStartPos, inEndPos, inExtents, QueryFilter, outPathPoints);
}

bool NavMesh::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, TVector<Float3>& outPathPoints) const
{
    NavPointRef start_ref;
    NavPointRef end_ref;

    if (!QueryNearestPoly(inStartPos, inExtents, inFilter, &start_ref.PolyRef))
        return false;

    if (!QueryNearestPoly(inEndPos, inExtents, inFilter, &end_ref.PolyRef))
        return false;

    start_ref.Position = inStartPos;
    end_ref.Position = inEndPos;

    int num_polys;
    if (!FindPath(start_ref, end_ref, inFilter, TmpPolys, &num_polys, MAX_POLYS))
        return false;

    Float3 closest_local_end = inEndPos;

    // Если не удалось проложить полный путь, установить конечную точку ближайшую к указанной конечной точке
    if (TmpPolys[num_polys - 1] != end_ref.PolyRef)
        m_NavQuery->closestPointOnPoly(TmpPolys[num_polys - 1], inEndPos.ToPtr(), closest_local_end.ToPtr(), 0);

    int path_len;
    m_NavQuery->findStraightPath(inStartPos.ToPtr(), closest_local_end.ToPtr(), TmpPolys, num_polys, TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &path_len, MAX_POLYS);

    outPathPoints.Resize(path_len);
    Core::Memcpy(outPathPoints.ToPtr(), TmpPathPoints, sizeof(Float3) * path_len);

    return true;
}

bool NavMesh::FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, TVector<Float3>& outPathPoints) const
{
    return FindPath(inStartPos, inEndPos, inExtents, QueryFilter, outPathPoints);
}

bool NavMesh::FindStraightPath(Float3 const& inStartPos, Float3 const& inEndPos, NavPolyRef const* inPath, int inPathSize, Float3* outStraightPath, unsigned char* outStraightPathFlags, NavPolyRef* outStraightPathRefs, int* outStraightPathCount, int inMaxStraightPath, NAV_MESH_STRAIGHTPATH_CROSSING inStraightPathCrossing) const
{
    if (!m_NavQuery)
        return false;

    dtStatus status = m_NavQuery->findStraightPath(inStartPos.ToPtr(), inEndPos.ToPtr(), inPath, inPathSize, (float*)outStraightPath, outStraightPathFlags, outStraightPathRefs, outStraightPathCount, inMaxStraightPath, (int)inStraightPathCrossing);
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const
{
    dtStatus status = m_NavQuery->findDistanceToWall(inPointRef.PolyRef, inPointRef.Position.ToPtr(), inRadius, inFilter.m_Filter.RawPtr(), &outHitResult.Distance, outHitResult.Position.ToPtr(), outHitResult.Normal.ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

bool NavMesh::CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavMeshHitResult& outHitResult) const
{
    return CalcDistanceToWall(inPointRef, inRadius, QueryFilter, outHitResult);
}

bool NavMesh::CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const
{
    NavPointRef point_ref;

    if (!QueryNearestPoly(inPosition, inExtents, inFilter, &point_ref.PolyRef))
        return false;

    point_ref.Position = inPosition;

    return CalcDistanceToWall(point_ref, inRadius, inFilter, outHitResult);
}

bool NavMesh::CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavMeshHitResult& outHitResult) const
{
    return CalcDistanceToWall(inPosition, inRadius, inExtents, QueryFilter, outHitResult);
}

bool NavMesh::GetHeight(NavPointRef const& inPointRef, float* outHeight) const
{
    if (!m_NavQuery)
    {
        *outHeight = 0;
        return false;
    }

    dtStatus status = m_NavQuery->getPolyHeight(inPointRef.PolyRef, inPointRef.Position.ToPtr(), outHeight);
    if (dtStatusFailed(status))
    {
        *outHeight = 0;
        return false;
    }

    return true;
}

bool NavMesh::GetOffMeshConnectionPolyEndPoints(NavPolyRef inPrevRef, NavPolyRef inPolyRef, Float3* outStartPos, Float3* outEndPos) const
{
    if (!m_NavMesh)
        return false;

    dtStatus status = m_NavMesh->getOffMeshConnectionPolyEndPoints(inPrevRef, inPolyRef, outStartPos->ToPtr(), outEndPos->ToPtr());
    if (dtStatusFailed(status))
        return false;

    return true;
}

void NavMesh::Update(float inTimeStep)
{
    if (m_TileCache)
        m_TileCache->update(inTimeStep, m_NavMesh);
}

#if 0
// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary,
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
int NavigationMeshComponent::FixupShortcuts( NavPolyRef * _Path, int inNPath ) const {
    if ( _NPath < 3 ) {
        return _NPath;
    }

    // Get connected polygons
    const int maxNeis = 16;
    NavPolyRef neis[maxNeis];
    int nneis = 0;

    const dtMeshTile * tile = 0;
    const dtPoly * poly = 0;
    if ( dtStatusFailed( m_NavQuery->getAttachedNavMesh()->getTileAndPolyByRef( _Path[0], &tile, &poly ) ) ) {
        return _NPath;
    }

    for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
    {
        const dtLink* link = &tile->links[k];
        if (link->ref != 0)
        {
            if (nneis < maxNeis)
                neis[nneis++] = link->ref;
        }
    }

    // If any of the neighbour polygons is within the next few polygons
    // in the path, short cut to that polygon directly.
    const int maxLookAhead = 6;
    int cut = 0;
    for (int i = Math::Min(maxLookAhead, _NPath) - 1; i > 1 && cut == 0; i--) {
        for (int j = 0; j < nneis; j++)
        {
            if (_Path[i] == neis[j]) {
                cut = i;
                break;
            }
        }
    }
    if (cut > 1)
    {
        int offset = cut-1;
        _NPath -= offset;
        for (int i = 1; i < _NPath; i++)
            _Path[i] = _Path[i+offset];
    }

    return _NPath;
}

int NavigationMeshComponent::FixupCorridor( NavPolyRef * _Path, const int inNPath, const int inMaxPath, const NavPolyRef * _Visited, const int inNVisited ) const {
    int furthestPath = -1;
    int furthestVisited = -1;

    // Find furthest common polygon.
    for (int i = _NPath-1; i >= 0; --i)
    {
        bool found = false;
        for (int j = _NVisited-1; j >= 0; --j)
        {
            if (_Path[i] == _Visited[j])
            {
                furthestPath = i;
                furthestVisited = j;
                found = true;
            }
        }
        if (found)
            break;
    }

    // If no intersection found just return current path.
    if (furthestPath == -1 || furthestVisited == -1)
        return _NPath;

    // Concatenate paths.

    // Adjust beginning of the buffer to include the visited.
    const int req = _NVisited - furthestVisited;
    const int orig = Math::Min(furthestPath+1, _NPath);
    int size = Math::Max(0, _NPath-orig);
    if (req+size > inMaxPath)
        size = inMaxPath-req;
    if (size)
        Core::Memmove(_Path+req, _Path+orig, size*sizeof(NavPolyRef));

    // Store visited
    for (int i = 0; i < req; ++i)
        _Path[i] = _Visited[(_NVisited-1)-i];

    return req+size;
}
#endif

//    Crowd = dtAllocCrowd();
//    if ( !Crowd ) {
//        Purge();
//        LOG( "Failed on dtAllocCrowd\n" );
//        return false;
//    }


void NavMesh::GatherNavigationGeometry(NavigationGeometry& Geometry)
{
    for (TListIterator<NavigationPrimitive> it(NavigationPrimitives) ; it ; ++it)
    {
        it->GatherNavigationGeometry(Geometry);
    }
}

HK_NAMESPACE_END
