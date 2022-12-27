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


#include "AINavigationMesh.h"
#include "World.h"
#include "MeshComponent.h"
#include "TerrainComponent.h"
#include "Engine.h"
#include <Platform/Logger.h>
#include <Platform/Memory/LinearAllocator.h>
#include <Core/Compress.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <Core/ConsoleVar.h>
#include <Geometry/BV/BvIntersect.h>

#undef malloc
#undef free

#include <Recast.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourCrowd.h>
#include <DetourNavMeshBuilder.h>
#include <DetourTileCache.h>
#include <DetourCommon.h>
#include <DetourTileCacheBuilder.h>
#include <RecastDebugDraw.h>
#include <DetourDebugDraw.h>
#include <DebugDraw.h>
#include <float.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawNavMeshBVTree("com_DrawNavMeshBVTree"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMeshNodes("com_DrawNavMeshNodes"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMesh("com_DrawNavMesh"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawNavMeshTileBounds("com_DrawNavMeshTileBounds"s, "0"s, CVAR_CHEAT);

HK_VALIDATE_TYPE_SIZE(NavPolyRef, sizeof(dtPolyRef));

static const int  MAX_LAYERS            = 255;
static const bool RECAST_ENABLE_LOGGING = true;
static const bool RECAST_ENABLE_TIMINGS = true;

enum
{
    MAX_POLYS = 2048
};
static NavPolyRef TmpPolys[MAX_POLYS];
static NavPolyRef TmpPathPolys[MAX_POLYS];
alignas(16) static Float3 TmpPathPoints[MAX_POLYS];
static unsigned char TmpPathFlags[MAX_POLYS];

struct TileCacheData
{
    byte* Data;
    int   Size;
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
        {
            return DT_FAILURE;
        }

        *compressedSize = size;

        return DT_SUCCESS;
    }

    dtStatus decompress(const unsigned char* compressed, const int compressedSize, unsigned char* buffer, const int maxBufferSize, int* bufferSize) override
    {
        size_t size;

        *bufferSize = 0;

        if (!Core::FastLZDecompress(compressed, compressedSize, buffer, &size, maxBufferSize))
        {
            return DT_FAILURE;
        }

        *bufferSize = size;

        return DT_SUCCESS;
    }
};

static TileCompressorCallback TileCompressorCallback;

struct DetourLinearAllocator : public dtTileCacheAlloc
{
    TLinearAllocator<> Allocator;

    void* operator new(size_t _SizeInBytes)
    {
        return Platform::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        Platform::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Ptr);
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
    TPodVector<Float3>         OffMeshConVerts;
    TPodVector<float>          OffMeshConRads;
    TPodVector<unsigned char>  OffMeshConDirs;
    TPodVector<unsigned char>  OffMeshConAreas;
    TPodVector<unsigned short> OffMeshConFlags;
    TPodVector<unsigned int>   OffMeshConId;
    int                        OffMeshConCount;
    AINavigationMesh*          NavMesh;

    void* operator new(size_t _SizeInBytes)
    {
        return Platform::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        Platform::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Ptr);
    }

    void process(struct dtNavMeshCreateParams* _Params, unsigned char* _PolyAreas, unsigned short* _PolyFlags) override
    {

        // Update poly flags from areas.
        for (int i = 0; i < _Params->polyCount; ++i)
        {
            if (_PolyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
            {
                _PolyAreas[i] = AI_NAV_MESH_AREA_GROUND;
            }
            if (_PolyAreas[i] == AI_NAV_MESH_AREA_GROUND ||
                _PolyAreas[i] == AI_NAV_MESH_AREA_GRASS ||
                _PolyAreas[i] == AI_NAV_MESH_AREA_ROAD)
            {
                _PolyFlags[i] = AI_NAV_MESH_FLAGS_WALK;
            }
            else if (_PolyAreas[i] == AI_NAV_MESH_AREA_WATER)
            {
                _PolyFlags[i] = AI_NAV_MESH_FLAGS_SWIM;
            }
            else if (_PolyAreas[i] == AI_NAV_MESH_AREA_DOOR)
            {
                _PolyFlags[i] = AI_NAV_MESH_FLAGS_WALK | AI_NAV_MESH_FLAGS_DOOR;
            }
        }

        BvAxisAlignedBox clipBounds;
        rcVcopy((float*)clipBounds.Mins.ToPtr(), _Params->bmin);
        rcVcopy((float*)clipBounds.Maxs.ToPtr(), _Params->bmax);

        OffMeshConVerts.Clear();
        OffMeshConVerts.Clear();
        OffMeshConRads.Clear();
        OffMeshConDirs.Clear();
        OffMeshConAreas.Clear();
        OffMeshConFlags.Clear();
        OffMeshConId.Clear();

        BvAxisAlignedBox conBoundingBox;
        const float      margin = 0.2f;
        OffMeshConCount         = 0;
        for (int i = 0; i < NavMesh->NavMeshConnections.Size(); i++)
        {
            AINavMeshConnection const& con = NavMesh->NavMeshConnections[i];

            con.CalcBoundingBox(conBoundingBox);
            conBoundingBox.Mins -= margin;
            conBoundingBox.Maxs += margin;

            if (!BvBoxOverlapBox(clipBounds, conBoundingBox))
            {
                // Connection is outside of clip bounds
                continue;
            }

            OffMeshConVerts.Add(con.StartPosition);
            OffMeshConVerts.Add(con.EndPosition);
            OffMeshConRads.Add(con.Radius);
            OffMeshConDirs.Add(con.bBidirectional ? DT_OFFMESH_CON_BIDIR : 0);
            OffMeshConAreas.Add(con.AreaId);
            OffMeshConFlags.Add(con.Flags);
            OffMeshConId.Add(i); // FIXME?

            OffMeshConCount++;
        }

        // Pass in off-mesh connections.
        _Params->offMeshConVerts  = (float*)OffMeshConVerts.ToPtr();
        _Params->offMeshConRad    = OffMeshConRads.ToPtr();
        _Params->offMeshConDir    = OffMeshConDirs.ToPtr();
        _Params->offMeshConAreas  = OffMeshConAreas.ToPtr();
        _Params->offMeshConFlags  = OffMeshConFlags.ToPtr();
        _Params->offMeshConUserID = OffMeshConId.ToPtr();
        _Params->offMeshConCount  = OffMeshConCount;
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
    int  doGetAccumulatedTime(const rcTimerLabel label) const override { return -1; }
};

static RecastContext RecastContext;

AINavigationMesh::AINavigationMesh()
{
    m_BoundingBox.Clear();
}

AINavigationMesh::~AINavigationMesh()
{
    Purge();
}

bool AINavigationMesh::Initialize(AINavigationConfig const& _NavigationConfig)
{
    dtStatus status;

    Purge();

    if (_NavigationConfig.BoundingBox.IsEmpty())
    {
        LOG("AINavigationMesh::Initialize: empty bounding box\n");
        return false;
    }

    Initial = _NavigationConfig;

    m_BoundingBox = _NavigationConfig.BoundingBox;

    if (Initial.VertsPerPoly < 3)
    {
        LOG("NavVertsPerPoly < 3\n");

        Initial.VertsPerPoly = 3;
    }
    else if (Initial.VertsPerPoly > DT_VERTS_PER_POLYGON)
    {
        LOG("NavVertsPerPoly > NAV_MAX_VERTS_PER_POLYGON\n");

        Initial.VertsPerPoly = DT_VERTS_PER_POLYGON;
    }

    if (Initial.MaxLayers > MAX_LAYERS)
    {
        LOG("MaxLayers > MAX_LAYERS\n");
        Initial.MaxLayers = MAX_LAYERS;
    }

    int gridWidth, gridHeight;

    rcCalcGridSize((const float*)m_BoundingBox.Mins.ToPtr(),
                   (const float*)m_BoundingBox.Maxs.ToPtr(),
                   Initial.CellSize, &gridWidth, &gridHeight);

    m_NumTilesX = (gridWidth + Initial.TileSize - 1) / Initial.TileSize;
    m_NumTilesZ = (gridHeight + Initial.TileSize - 1) / Initial.TileSize;

    // Max tiles and max polys affect how the tile IDs are caculated.
    // There are 22 bits available for identifying a tile and a polygon.
    uint64_t           powOf2          = Math::ToGreaterPowerOfTwo(m_NumTilesX * m_NumTilesZ);
    const unsigned int tileBits        = Math::Min(Math::Log2(powOf2), 14);
    const unsigned int maxTiles        = 1 << tileBits;
    const unsigned int maxPolysPerTile = 1u << (22 - tileBits);

    m_TileWidth = Initial.TileSize * Initial.CellSize;

    dtNavMeshParams params;
    Platform::ZeroMem(&params, sizeof(params));
    rcVcopy(params.orig, (const float*)m_BoundingBox.Mins.ToPtr());
    params.tileWidth  = m_TileWidth;
    params.tileHeight = m_TileWidth;
    params.maxTiles   = maxTiles;
    params.maxPolys   = maxPolysPerTile;

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
    status              = m_NavQuery->init(m_NavMesh, MAX_NODES);
    if (dtStatusFailed(status))
    {
        Purge();
        LOG("Could not initialize navmesh query");
        return false;
    }

    if (Initial.bDynamicNavMesh)
    {
        // Create tile cache

        dtTileCacheParams tileCacheParams;
        Platform::ZeroMem(&tileCacheParams, sizeof(tileCacheParams));
        rcVcopy(tileCacheParams.orig, (const float*)Initial.BoundingBox.Mins.ToPtr());
        tileCacheParams.cs                     = Initial.CellSize;
        tileCacheParams.ch                     = Initial.CellHeight;
        tileCacheParams.width                  = Initial.TileSize;
        tileCacheParams.height                 = Initial.TileSize;
        tileCacheParams.walkableHeight         = Initial.WalkableHeight;
        tileCacheParams.walkableRadius         = Initial.WalkableRadius;
        tileCacheParams.walkableClimb          = Initial.WalkableClimb;
        tileCacheParams.maxSimplificationError = Initial.EdgeMaxError;
        tileCacheParams.maxTiles               = maxTiles * Initial.MaxLayers;
        tileCacheParams.maxObstacles           = Initial.MaxDynamicObstacles;

        m_TileCache = dtAllocTileCache();
        if (!m_TileCache)
        {
            Purge();
            LOG("Failed on dtAllocTileCache\n");
            return false;
        }

        m_LinearAllocator = MakeUnique<DetourLinearAllocator>();

        m_MeshProcess          = MakeUnique<DetourMeshProcess>();
        m_MeshProcess->NavMesh = this;

        status = m_TileCache->init(&tileCacheParams, m_LinearAllocator.GetObject(), &TileCompressorCallback, m_MeshProcess.GetObject());
        if (dtStatusFailed(status))
        {
            Purge();
            LOG("Could not initialize tile cache\n");
            return false;
        }

        // TODO: Add obstacles here?
    }

    //bNavigationDirty = true;

    return true;
}

bool AINavigationMesh::Build()
{
    Int2 regionMins(0, 0);
    Int2 regionMaxs(m_NumTilesX - 1, m_NumTilesZ - 1);

    //bNavigationDirty = false;

    return BuildTiles(regionMins, regionMaxs);
}

bool AINavigationMesh::Build(Int2 const& _Mins, Int2 const& _Maxs)
{
    Int2 regionMins;
    Int2 regionMaxs;

    regionMins.X = Math::Clamp<int>(_Mins.X, 0, m_NumTilesX - 1);
    regionMins.Y = Math::Clamp<int>(_Mins.Y, 0, m_NumTilesZ - 1);
    regionMaxs.X = Math::Clamp<int>(_Maxs.X, 0, m_NumTilesX - 1);
    regionMaxs.Y = Math::Clamp<int>(_Maxs.Y, 0, m_NumTilesZ - 1);

    return BuildTiles(regionMins, regionMaxs);
}

bool AINavigationMesh::Build(BvAxisAlignedBox const& _BoundingBox)
{
    Int2 mins((_BoundingBox.Mins.X - m_BoundingBox.Mins.X) / m_TileWidth,
              (_BoundingBox.Mins.Z - m_BoundingBox.Mins.Z) / m_TileWidth);
    Int2 maxs((_BoundingBox.Maxs.X - m_BoundingBox.Mins.X) / m_TileWidth,
              (_BoundingBox.Maxs.Z - m_BoundingBox.Mins.Z) / m_TileWidth);

    return Build(mins, maxs);
}

void AINavigationMesh::GetTileWorldBounds(int _X, int _Z, BvAxisAlignedBox& _BoundingBox) const
{
    _BoundingBox.Mins[0] = m_BoundingBox.Mins[0] + _X * m_TileWidth;
    _BoundingBox.Mins[1] = m_BoundingBox.Mins[1];
    _BoundingBox.Mins[2] = m_BoundingBox.Mins[2] + _Z * m_TileWidth;

    _BoundingBox.Maxs[0] = m_BoundingBox.Mins[0] + (_X + 1) * m_TileWidth;
    _BoundingBox.Maxs[1] = m_BoundingBox.Maxs[1];
    _BoundingBox.Maxs[2] = m_BoundingBox.Mins[2] + (_Z + 1) * m_TileWidth;
}

bool AINavigationMesh::BuildTiles(Int2 const& _Mins, Int2 const& _Maxs)
{
    if (!m_NavMesh)
    {
        LOG("AINavigationMesh::BuildTiles: navmesh must be initialized\n");
        return false;
    }

    unsigned int totalBuilt = 0;
    for (int z = _Mins[1]; z <= _Maxs[1]; z++)
    {
        for (int x = _Mins[0]; x <= _Maxs[0]; x++)
        {
            if (BuildTile(x, z))
            {
                totalBuilt++;
            }
        }
    }
    return totalBuilt > 0;
}

// Based on rcMarkWalkableTriangles
static void MarkWalkableTriangles(float WalkableSlopeAngle, Float3 const* Vertices, unsigned int const* Indices, int NumTriangles, int FirstTriangle, TBitMask<> const& WalkableMask, unsigned char* Areas)
{
    Float3 perpendicular;
    float  perpendicularLength;

    const float WalkableThreshold = cosf(Math::Radians(WalkableSlopeAngle));

    for (int i = 0; i < NumTriangles; ++i)
    {
        int triangleNum = FirstTriangle + i;
        if (WalkableMask.IsMarked(triangleNum))
        {
            unsigned int const* tri = &Indices[triangleNum * 3];

            perpendicular       = Math::Cross(Vertices[tri[1]] - Vertices[tri[0]], Vertices[tri[2]] - Vertices[tri[0]]);
            perpendicularLength = perpendicular.Length();
            if (perpendicularLength > 0 && perpendicular[1] > WalkableThreshold * perpendicularLength)
            {
                Areas[i] = RC_WALKABLE_AREA;
            }
        }
    }
}

static bool PointInPoly2D(int nvert, const float* verts, const float* p)
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

static String GetErrorStr(dtStatus status)
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

bool AINavigationMesh::BuildTile(int _X, int _Z)
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
            Platform::ZeroMem(this, sizeof(*this));
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

    BvAxisAlignedBox tileWorldBounds;
    BvAxisAlignedBox tileWorldBoundsWithPadding;

    HK_ASSERT(m_NavMesh);

    RemoveTile(_X, _Z);

    GetTileWorldBounds(_X, _Z, tileWorldBounds);

    rcConfig config;
    Platform::ZeroMem(&config, sizeof(config));
    config.cs                     = Initial.CellSize;
    config.ch                     = Initial.CellHeight;
    config.walkableSlopeAngle     = Initial.WalkableSlopeAngle;
    config.walkableHeight         = (int)Math::Ceil(Initial.WalkableHeight / config.ch);
    config.walkableClimb          = (int)Math::Floor(Initial.WalkableClimb / config.ch);
    config.walkableRadius         = (int)Math::Ceil(Initial.WalkableRadius / config.cs);
    config.maxEdgeLen             = (int)(Initial.EdgeMaxLength / Initial.CellSize);
    config.maxSimplificationError = Initial.EdgeMaxError;
    config.minRegionArea          = (int)rcSqr(Initial.MinRegionSize);   // Note: area = size*size
    config.mergeRegionArea        = (int)rcSqr(Initial.MergeRegionSize); // Note: area = size*size
    config.detailSampleDist       = Initial.DetailSampleDist < 0.9f ? 0 : Initial.CellSize * Initial.DetailSampleDist;
    config.detailSampleMaxError   = Initial.CellHeight * Initial.DetailSampleMaxError;
    config.tileSize               = Initial.TileSize;
    config.borderSize             = config.walkableRadius + 3; // radius + padding
    config.width                  = config.tileSize + config.borderSize * 2;
    config.height                 = config.tileSize + config.borderSize * 2;
    config.maxVertsPerPoly        = Initial.VertsPerPoly;

    rcVcopy(config.bmin, (const float*)tileWorldBounds.Mins.ToPtr());
    rcVcopy(config.bmax, (const float*)tileWorldBounds.Maxs.ToPtr());

    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    for (int i = 0; i < 3; i++)
    {
        tileWorldBoundsWithPadding.Mins[i] = config.bmin[i];
        tileWorldBoundsWithPadding.Maxs[i] = config.bmax[i];
    }

    NavigationGeometry geometry;
    geometry.pClipBoundingBox = &tileWorldBoundsWithPadding;
    geometry.BoundingBox.Clear();
    GatherNavigationGeometry(geometry);

    if (geometry.BoundingBox.IsEmpty() || geometry.Indices.IsEmpty())
    {
        // Empty tile
        return true;
    }
    //---------TEST-----------
    //walkableMask.Resize(indices.Size()/3);
    //walkableMask.MarkAll();
    //------------------------

    //rcVcopy( config.bmin, (const float *)geometry.BoundingBox.Mins.ToPtr() );
    //rcVcopy( config.bmax, (const float *)geometry.BoundingBox.Maxs.ToPtr() );
    config.bmin[1]             = geometry.BoundingBox.Mins.Y;
    config.bmax[1]             = geometry.BoundingBox.Maxs.Y;
    tileWorldBoundsWithPadding = geometry.BoundingBox;

    TemportalData temporal;

    // Allocate voxel heightfield where we rasterize our input data to.
    temporal.Heightfield = rcAllocHeightfield();
    if (!temporal.Heightfield)
    {
        LOG("Failed on rcAllocHeightfield\n");
        return false;
    }

    if (!rcCreateHeightfield(&RecastContext, *temporal.Heightfield, config.width, config.height,
                             config.bmin, config.bmax, config.cs, config.ch))
    {
        LOG("Failed on rcCreateHeightfield\n");
        return false;
    }

    int trianglesCount = geometry.Indices.Size() / 3;

    // Allocate array that can hold triangle area types.
    // If you have multiple meshes you need to process, allocate
    // and array which can hold the max number of triangles you need to process.
    unsigned char* triangleAreaTypes = (unsigned char*)Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(trianglesCount, 16, MALLOC_ZERO);

    // Find triangles which are walkable based on their slope and rasterize them.
    // If your input data is multiple meshes, you can transform them here, calculate
    // the are type for each of the meshes and rasterize them.
    MarkWalkableTriangles(config.walkableSlopeAngle, geometry.Vertices.ToPtr(), geometry.Indices.ToPtr(), trianglesCount, 0, geometry.WalkableMask, triangleAreaTypes);

    bool rasterized = rcRasterizeTriangles(&RecastContext,
                                           (float const*)geometry.Vertices.ToPtr(),
                                           geometry.Vertices.Size(),
                                           (int const*)geometry.Indices.ToPtr(),
                                           triangleAreaTypes,
                                           trianglesCount,
                                           *temporal.Heightfield,
                                           config.walkableClimb);

    Platform::GetHeapAllocator<HEAP_TEMP>().Free(triangleAreaTypes);

    if (!rasterized)
    {
        LOG("Failed on rcRasterizeTriangles\n");
        return false;
    }

    // Filter walkables surfaces.

    // Once all geoemtry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    rcFilterLowHangingWalkableObstacles(&RecastContext, config.walkableClimb, *temporal.Heightfield);
    rcFilterLedgeSpans(&RecastContext, config.walkableHeight, config.walkableClimb, *temporal.Heightfield);
    rcFilterWalkableLowHeightSpans(&RecastContext, config.walkableHeight, *temporal.Heightfield);

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

    if (!rcBuildCompactHeightfield(&RecastContext, config.walkableHeight, config.walkableClimb, *temporal.Heightfield, *temporal.CompactHeightfield))
    {
        LOG("Failed on rcBuildCompactHeightfield\n");
        return false;
    }

    // Erode the walkable area by agent radius.
    if (!rcErodeWalkableArea(&RecastContext, config.walkableRadius, *temporal.CompactHeightfield))
    {
        LOG("AINavigationMesh::Build: Failed on rcErodeWalkableArea\n");
        return false;
    }
#if 1
    BvAxisAlignedBox areaBoundingBox;
    for (int areaNum = 0; areaNum < NavigationAreas.Size(); ++areaNum)
    {
        AINavigationArea const&    area = NavigationAreas[areaNum];
        rcCompactHeightfield const& chf  = *temporal.CompactHeightfield;

        area.CalcBoundingBox(areaBoundingBox);

        if (areaBoundingBox.IsEmpty())
        {
            // Invalid bounding box
            continue;
        }

        if (!BvBoxOverlapBox(tileWorldBoundsWithPadding, areaBoundingBox))
        {
            // Area is outside of tile bounding box
            continue;
        }

        // The next code is based on rcMarkBoxArea and rcMarkConvexPolyArea
        int minx = (int)((areaBoundingBox.Mins[0] - chf.bmin[0]) / chf.cs);
        int miny = (int)((areaBoundingBox.Mins[1] - chf.bmin[1]) / chf.ch);
        int minz = (int)((areaBoundingBox.Mins[2] - chf.bmin[2]) / chf.cs);
        int maxx = (int)((areaBoundingBox.Maxs[0] - chf.bmin[0]) / chf.cs);
        int maxy = (int)((areaBoundingBox.Maxs[1] - chf.bmin[1]) / chf.ch);
        int maxz = (int)((areaBoundingBox.Maxs[2] - chf.bmin[2]) / chf.cs);

        if (maxx < 0) continue;
        if (minx >= chf.width) continue;
        if (maxz < 0) continue;
        if (minz >= chf.height) continue;

        if (minx < 0) minx = 0;
        if (maxx >= chf.width) maxx = chf.width - 1;
        if (minz < 0) minz = 0;
        if (maxz >= chf.height) maxz = chf.height - 1;

        if (area.Shape == AI_NAV_MESH_AREA_SHAPE_CONVEX_VOLUME)
        {
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

                            if (PointInPoly2D(area.NumConvexVolumeVerts, (float*)area.ConvexVolume[0].ToPtr(), p))
                            {
                                chf.areas[i] = area.AreaId;
                            }
                        }
                    }
                }
            }
        }
        else
        {
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
                                chf.areas[i] = area.AreaId;
                        }
                    }
                }
            }
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

    if (Initial.RecastPartitionMethod == AI_NAV_MESH_PARTITION_WATERSHED)
    {
        // Prepare for region partitioning, by calculating distance field along the walkable surface.
        if (!rcBuildDistanceField(&RecastContext, *temporal.CompactHeightfield))
        {
            LOG("Could not build distance field\n");
            return false;
        }

        // Partition the walkable surface into simple regions without holes.
        if (!rcBuildRegions(&RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea, config.mergeRegionArea))
        {
            LOG("Could not build watershed regions\n");
            return false;
        }
    }
    else if (Initial.RecastPartitionMethod == AI_NAV_MESH_PARTITION_MONOTONE)
    {
        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distancefield.
        if (!rcBuildRegionsMonotone(&RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea, config.mergeRegionArea))
        {
            LOG("Could not build monotone regions\n");
            return false;
        }
    }
    else
    { // RECAST_PARTITION_LAYERS
        // Partition the walkable surface into simple regions without holes.
        if (!rcBuildLayerRegions(&RecastContext, *temporal.CompactHeightfield, config.borderSize /*0*/, config.minRegionArea))
        {
            LOG("Could not build layer regions\n");
            return false;
        }
    }

    if (Initial.bDynamicNavMesh)
    {
        temporal.LayerSet = rcAllocHeightfieldLayerSet();
        if (!temporal.LayerSet)
        {
            LOG("Failed on rcAllocHeightfieldLayerSet\n");
            return false;
        }

        if (!rcBuildHeightfieldLayers(&RecastContext, *temporal.CompactHeightfield, config.borderSize, config.walkableHeight, *temporal.LayerSet))
        {
            LOG("Failed on rcBuildHeightfieldLayers\n");
            return false;
        }

        TileCacheData cacheData[MAX_LAYERS];

        int numLayers      = Math::Min(temporal.LayerSet->nlayers, MAX_LAYERS);
        int numValidLayers = 0;
        for (int i = 0; i < numLayers; i++)
        {
            TileCacheData*           tile  = &cacheData[i];
            rcHeightfieldLayer const* layer = &temporal.LayerSet->layers[i];

            dtTileCacheLayerHeader header;
            header.magic   = DT_TILECACHE_MAGIC;
            header.version = DT_TILECACHE_VERSION;
            header.tx      = _X;
            header.ty      = _Z;
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

            dtStatus status = dtBuildTileCacheLayer(&TileCompressorCallback, &header, layer->heights, layer->areas, layer->cons, &tile->Data, &tile->Size);
            if (dtStatusFailed(status))
            {
                LOG("Failed on dtBuildTileCacheLayer\n");
                break;
            }

            numValidLayers++;
        }

        int cacheLayerCount = 0;
        //int cacheCompressedSize = 0;
        //int cacheRawSize = 0;
        //const int layerBufferSize = dtAlign4( sizeof( dtTileCacheLayerHeader ) ) + Initial.NavTileSize * Initial.NavTileSize * numLayers;
        for (int i = 0; i < numValidLayers; i++)
        {
            dtCompressedTileRef ref;
            dtStatus            status = m_TileCache->addTile(cacheData[i].Data, cacheData[i].Size, DT_COMPRESSEDTILE_FREE_DATA, &ref);
            if (dtStatusFailed(status))
            {
                dtFree(cacheData[i].Data);
                cacheData[i].Data = nullptr;
                continue;
            }

            status = m_TileCache->buildNavMeshTile(ref, m_NavMesh);
            if (dtStatusFailed(status))
            {
                LOG("Failed to build navmesh tile {}\n", GetErrorStr(status));
            }

            cacheLayerCount++;
            //            cacheCompressedSize += cacheData[i].Size;
            //            cacheRawSize += layerBufferSize;
        }

        if (cacheLayerCount == 0)
        {
            return false;
        }


        //        int cacheBuildMemUsage = LinearAllocator->High;
        //        const float compressionRatio = (float)cacheCompressedSize / (float)(cacheRawSize+1);

        //        int totalMemoryUsage = 0;
        //        const dtNavMesh * mesh = m_NavMesh;
        //        for ( int i = 0 ; i < mesh->getMaxTiles() ; ++i ) {
        //            const dtMeshTile * tile = mesh->getTile( i );
        //            if ( tile->header ) {
        //                totalMemoryUsage += tile->dataSize;
        //            }
        //        }

        //        GLogger.Printf( "Processed navigation data:\n"
        //                        "Total memory usage for m_NavMesh: %.1f kB\n"
        //                        "Cache compressed size: %.1f kB\n"
        //                        "Cache raw size: %.1f kB\n"
        //                        "Cache compression ratio: %.1f%%\n"
        //                        "Cache layers count: %d\n"
        //                        "Cache layers per tile: %.1f\n"
        //                        "Build peak memory usage: %d kB\n",
        //                        totalMemoryUsage/1024.0f,
        //                        cacheCompressedSize/1024.0f,
        //                        cacheRawSize/1024.0f,
        //                        compressionRatio*100.0f,
        //                        cacheayerCount,
        //                        (float)cacheLayerCount/gridSize,
        //                        cacheBuildMemUsage
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
        if (!rcBuildContours(&RecastContext, *temporal.CompactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *temporal.ContourSet))
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
        if (!rcBuildPolyMesh(&RecastContext, *temporal.ContourSet, config.maxVertsPerPoly, *temporal.PolyMesh))
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
        if (!rcBuildPolyMeshDetail(&RecastContext,
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
        static_assert(AI_NAV_MESH_AREA_GROUND == RC_WALKABLE_AREA, "Navmesh area id static check");
        for (int i = 0; i < temporal.PolyMesh->npolys; ++i)
        {

            if (temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_GROUND ||
                temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_GRASS ||
                temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_ROAD)
            {

                temporal.PolyMesh->flags[i] = AI_NAV_MESH_FLAGS_WALK;
            }
            else if (temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_WATER)
            {

                temporal.PolyMesh->flags[i] = AI_NAV_MESH_FLAGS_SWIM;
            }
            else if (temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_DOOR)
            {

                temporal.PolyMesh->flags[i] = AI_NAV_MESH_FLAGS_WALK | AI_NAV_MESH_FLAGS_DOOR;
            }
        }

        BvAxisAlignedBox           conBoundingBox;
        const float                margin = 0.2f;
        TPodVector<Float3>         offMeshConVerts;
        TPodVector<float>          offMeshConRads;
        TPodVector<unsigned char>  offMeshConDirs;
        TPodVector<unsigned char>  offMeshConAreas;
        TPodVector<unsigned short> offMeshConFlags;
        TPodVector<unsigned int>   offMeshConId;
        int                        offMeshConCount = 0;
        for (int i = 0; i < NavMeshConnections.Size(); i++)
        {
            AINavMeshConnection const& con = NavMeshConnections[i];

            con.CalcBoundingBox(conBoundingBox);
            conBoundingBox.Mins -= margin;
            conBoundingBox.Maxs += margin;

            if (!BvBoxOverlapBox(tileWorldBoundsWithPadding, conBoundingBox))
            {
                // Connection is outside of tile bounding box
                continue;
            }

            offMeshConVerts.Add(con.StartPosition);
            offMeshConVerts.Add(con.EndPosition);
            offMeshConRads.Add(con.Radius);
            offMeshConDirs.Add(con.bBidirectional ? DT_OFFMESH_CON_BIDIR : 0);
            offMeshConAreas.Add(con.AreaId);
            offMeshConFlags.Add(con.Flags);
            offMeshConId.Add(i); // FIXME?

            offMeshConCount++;
        }

        // Create Detour data from poly mesh.

        dtNavMeshCreateParams params;
        Platform::ZeroMem(&params, sizeof(params));
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
        params.offMeshConVerts  = (float*)offMeshConVerts.ToPtr();
        params.offMeshConRad    = offMeshConRads.ToPtr();
        params.offMeshConDir    = offMeshConDirs.ToPtr();
        params.offMeshConAreas  = offMeshConAreas.ToPtr();
        params.offMeshConFlags  = offMeshConFlags.ToPtr();
        params.offMeshConUserID = offMeshConId.ToPtr();
        params.offMeshConCount  = offMeshConCount;
        params.walkableHeight   = Initial.WalkableHeight;
        params.walkableRadius   = Initial.WalkableRadius;
        params.walkableClimb    = Initial.WalkableClimb;
        params.tileX            = _X;
        params.tileY            = _Z;
        rcVcopy(params.bmin, temporal.PolyMesh->bmin);
        rcVcopy(params.bmax, temporal.PolyMesh->bmax);
        params.cs          = config.cs;
        params.ch          = config.ch;
        params.buildBvTree = true;

        unsigned char* navData     = 0;
        int            navDataSize = 0;

        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {

            if (params.vertCount >= 0xffff)
            {
                LOG("vertCount >= 0xffff\n");
            }

            LOG("Could not build navmesh tile\n");
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

void AINavigationMesh::RemoveTile(int _X, int _Z)
{
    if (!m_NavMesh)
    {
        return;
    }

    if (Initial.bDynamicNavMesh)
    {

        HK_ASSERT(m_TileCache);

        dtCompressedTileRef compressedTiles[MAX_LAYERS];
        int                 count = m_TileCache->getTilesAt(_X, _Z, compressedTiles, Initial.MaxLayers);
        for (int i = 0; i < count; i++)
        {
            byte*    data   = nullptr;
            dtStatus status = m_TileCache->removeTile(compressedTiles[i], &data, nullptr);
            if (dtStatusFailed(status))
            {
                continue;
            }
            dtFree(data);
        }
    }
    else
    {

        dtTileRef ref = m_NavMesh->getTileRefAt(_X, _Z, 0);
        if (ref)
        {
            m_NavMesh->removeTile(ref, nullptr, nullptr);
        }
    }
}

void AINavigationMesh::RemoveTiles()
{
    if (!m_NavMesh)
    {
        return;
    }

    if (Initial.bDynamicNavMesh)
    {
        HK_ASSERT(m_TileCache);

        int numTiles = m_TileCache->getTileCount();
        for (int i = 0; i < numTiles; i++)
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
        int              numTiles = m_NavMesh->getMaxTiles();
        const dtNavMesh* navMesh  = m_NavMesh;
        for (int i = 0; i < numTiles; i++)
        {
            const dtMeshTile* tile = navMesh->getTile(i);
            if (tile && tile->header)
            {
                m_NavMesh->removeTile(m_NavMesh->getTileRef(tile), nullptr, nullptr);
            }
        }
    }
}

void AINavigationMesh::RemoveTiles(Int2 const& _Mins, Int2 const& _Maxs)
{
    if (!m_NavMesh)
    {
        return;
    }
    for (int z = _Mins[1]; z <= _Maxs[1]; z++)
    {
        for (int x = _Mins[0]; x <= _Maxs[0]; x++)
        {
            RemoveTile(x, z);
        }
    }
}

bool AINavigationMesh::IsTileExsist(int _X, int _Z) const
{
    return m_NavMesh ? m_NavMesh->getTileAt(_X, _Z, 0) != nullptr : false;
}

void AINavigationMesh::AddObstacle(AINavMeshObstacle* _Obstacle)
{
    if (!m_TileCache)
    {
        return;
    }

    dtObstacleRef ref;
    dtStatus      status;

    // TODO:
    //while ( m_TileCache->isObstacleQueueFull() ) {
    //    m_TileCache->update( 1, m_NavMesh );
    //}

    if (_Obstacle->Shape == AI_NAV_MESH_OBSTACLE_BOX)
    {
        Float3 mins = _Obstacle->Position - _Obstacle->HalfExtents;
        Float3 maxs = _Obstacle->Position + _Obstacle->HalfExtents;
        status      = m_TileCache->addBoxObstacle((const float*)mins.ToPtr(), (const float*)maxs.ToPtr(), &ref);
    }
    else
    {
        while (1)
        {
            status = m_TileCache->addObstacle((const float*)_Obstacle->Position.ToPtr(), _Obstacle->Radius, _Obstacle->Height, &ref);

            if (status & DT_BUFFER_TOO_SMALL)
            {
                m_TileCache->update(1, m_NavMesh);
                continue;
            }

            break;
        }
    }

    if (dtStatusFailed(status))
    {
        LOG("Failed to add navmesh obstacle\n");
        if (status & DT_OUT_OF_MEMORY)
        {
            LOG("DT_OUT_OF_MEMORY\n");
        }
        return;
    }
    LOG("AddObstacle: {}\n", ref);
    _Obstacle->ObstacleRef = ref;
}

void AINavigationMesh::RemoveObstacle(AINavMeshObstacle* _Obstacle)
{
    if (!m_TileCache)
    {
        return;
    }

    if (!_Obstacle->ObstacleRef)
    {
        return;
    }

    // TODO:
    //while ( m_TileCache->isObstacleQueueFull() ) {
    //    m_TileCache->update( 1, m_NavMesh );
    //}

    dtStatus status;

    while (1)
    {
        status = m_TileCache->removeObstacle(_Obstacle->ObstacleRef);

        if (status & DT_BUFFER_TOO_SMALL)
        {
            m_TileCache->update(1, m_NavMesh);
            continue;
        }

        break;
    }

    if (dtStatusFailed(status))
    {
        LOG("Failed to remove navmesh obstacle\n");
        return;
    }

    _Obstacle->ObstacleRef = 0;
}

void AINavigationMesh::UpdateObstacle(AINavMeshObstacle* _Obstacle)
{
    if (!_Obstacle->ObstacleRef)
    {
        LOG("AINavigationMesh::UpdateObstacle: obstacle is not in navmesh\n");
        return;
    }

    RemoveObstacle(_Obstacle);
    AddObstacle(_Obstacle);
}

void AINavigationMesh::Purge()
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
    DebugRenderer*       DD;
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

void AINavigationMesh::DrawDebug(DebugRenderer* InRenderer)
{
    if (!m_NavMesh)
    {
        return;
    }

    DebugDrawCallback callback;
    callback.DD = InRenderer;

    if (com_DrawNavMeshBVTree)
    {
        duDebugDrawNavMeshBVTree(&callback, *m_NavMesh);
    }

    if (com_DrawNavMeshNodes)
    {
        duDebugDrawNavMeshNodes(&callback, *m_NavQuery);
    }

    if (com_DrawNavMesh)
    {
        duDebugDrawNavMeshWithClosedList(&callback, *m_NavMesh, *m_NavQuery, DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST | DU_DRAWNAVMESH_COLOR_TILES);
    }
    //duDebugDrawNavMeshPolysWithFlags( &callback, *m_NavMesh, AI_NAV_MESH_FLAGS_DISABLED/*AI_NAV_MESH_FLAGS_DISABLED*/, duRGBA(0,0,0,128));
    //duDebugDrawNavMeshPolysWithFlags( &callback, *m_NavMesh, 0/*AI_NAV_MESH_FLAGS_DISABLED*/, duRGBA(0,0,0,128));


#if 0
    if ( RVDrawNavMeshTileBounds && m_TileCache ) {
        float bmin[3], bmax[3];
        for ( int i = 0 ; i < m_TileCache->getTileCount() ; ++i ) {
            const dtCompressedTile* tile = m_TileCache->getTile(i);

            if ( !tile->header ) {
                continue;
            }

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
        BvAxisAlignedBox boundingBox;
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(1, 1, 1, 1));
        for (int z = 0; z < m_NumTilesZ; z++)
        {
            for (int x = 0; x < m_NumTilesX; x++)
            {
                if (IsTileExsist(x, z))
                {
                    GetTileWorldBounds(x, z, boundingBox);

                    InRenderer->DrawAABB(boundingBox);
                }
            }
        }
    }

#if 0
    static TPodVector< Float3 > vertices;
    static TPodVector< unsigned int > indices;
    static BvAxisAlignedBox dummyBoundingBox;
    static TBitMask<> walkableMask;
    static bool gen=false;
    if (!gen) {
        GatherNavigationGeometry( vertices, indices, walkableMask, dummyBoundingBox, &BoundingBox );
        //gen=true;
    }
    InRenderer->SetDepthTest(true);
    InRenderer->DrawTriangleSoup(vertices,indices,false);
#endif
}

class NavQueryFilterPrivate : public dtQueryFilter
{
public:
    void* operator new(size_t _SizeInBytes)
    {
        return Platform::GetHeapAllocator<HEAP_NAVIGATION>().Alloc(_SizeInBytes);
    }

    void operator delete(void* _Ptr)
    {
        Platform::GetHeapAllocator<HEAP_NAVIGATION>().Free(_Ptr);
    }
};

NavQueryFilter::NavQueryFilter()
{
    Filter = MakeUnique<NavQueryFilterPrivate>();
}

NavQueryFilter::~NavQueryFilter()
{
}

void NavQueryFilter::SetAreaCost(int _AreaId, float _Cost)
{
    Filter->setAreaCost(_AreaId, _Cost);
}

float NavQueryFilter::GetAreaCost(int _AreaId) const
{
    return Filter->getAreaCost(_AreaId);
}

void NavQueryFilter::SetIncludeFlags(unsigned short _Flags)
{
    Filter->setIncludeFlags(_Flags);
}

unsigned short NavQueryFilter::GetIncludeFlags() const
{
    return Filter->getIncludeFlags();
}

void NavQueryFilter::SetExcludeFlags(unsigned short _Flags)
{
    Filter->setExcludeFlags(_Flags);
}

unsigned short NavQueryFilter::GetExcludeFlags() const
{
    return Filter->getExcludeFlags();
}

bool AINavigationMesh::Trace(AINavigationTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, Float3 const& _Extents, NavQueryFilter const& _Filter) const
{
    NavPolyRef StartRef;

    if (!QueryNearestPoly(_RayStart, _Extents, &StartRef))
    {
        _Result.Clear();
        return false;
    }

    int NumPolys;

    _Result.HitFraction = Math::MaxValue<float>();

    m_NavQuery->raycast(StartRef, (const float*)_RayStart.ToPtr(), (const float*)_RayEnd.ToPtr(), _Filter.Filter.GetObject(), &_Result.HitFraction, (float*)_Result.Normal.ToPtr(), TmpPolys, &NumPolys, MAX_POLYS);

    bool bHasHit = _Result.HitFraction != Math::MaxValue<float>();
    if (!bHasHit)
    {
        _Result.Clear();
        return false;
    }

    _Result.Position = _RayStart + (_RayEnd - _RayStart) * _Result.HitFraction;
    _Result.Distance = (_Result.Position - _RayStart).Length();

    return true;
}

bool AINavigationMesh::Trace(AINavigationTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, Float3 const& _Extents) const
{
    return Trace(_Result, _RayStart, _RayEnd, _Extents, QueryFilter);
}

bool AINavigationMesh::QueryTileLocaction(Float3 const& _Position, int* _TileX, int* _TileY) const
{
    if (!m_NavMesh)
    {
        *_TileX = *_TileY = 0;
        return false;
    }

    m_NavMesh->calcTileLoc((float*)_Position.ToPtr(), _TileX, _TileY);
    return true;
}

bool AINavigationMesh::QueryNearestPoly(Float3 const& _Position, Float3 const& _Extents, NavQueryFilter const& _Filter, NavPolyRef* _NearestPolyRef) const
{
    *_NearestPolyRef = 0;

    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->findNearestPoly((const float*)_Position.ToPtr(), (const float*)_Extents.ToPtr(), _Filter.Filter.GetObject(), _NearestPolyRef, NULL);
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::QueryNearestPoly(Float3 const& _Position, Float3 const& _Extents, NavPolyRef* _NearestPolyRef) const
{
    return QueryNearestPoly(_Position, _Extents, QueryFilter, _NearestPolyRef);
}

bool AINavigationMesh::QueryNearestPoint(Float3 const& _Position, Float3 const& _Extents, NavQueryFilter const& _Filter, NavPointRef* _NearestPointRef) const
{
    _NearestPointRef->PolyRef = 0;
    _NearestPointRef->Position.Clear();

    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->findNearestPoly((const float*)_Position.ToPtr(), (const float*)_Extents.ToPtr(), _Filter.Filter.GetObject(), &_NearestPointRef->PolyRef, (float*)_NearestPointRef->Position.ToPtr());
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::QueryNearestPoint(Float3 const& _Position, Float3 const& _Extents, NavPointRef* _NearestPointRef) const
{
    return QueryNearestPoint(_Position, _Extents, QueryFilter, _NearestPointRef);
}

// Function returning a random number [0..1).
static float NavRandom()
{
    const float range = 1.0f - FLT_EPSILON;
    return GEngine->Rand.GetFloat() * range;
}

bool AINavigationMesh::QueryRandomPoint(NavQueryFilter const& _Filter, NavPointRef* _RandomPointRef) const
{
    _RandomPointRef->PolyRef = 0;
    _RandomPointRef->Position.Clear();

    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->findRandomPoint(_Filter.Filter.GetObject(), NavRandom, &_RandomPointRef->PolyRef, (float*)_RandomPointRef->Position.ToPtr());
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::QueryRandomPoint(NavPointRef* _RandomPointRef) const
{
    return QueryRandomPoint(QueryFilter, _RandomPointRef);
}

bool AINavigationMesh::QueryRandomPointAroundCircle(Float3 const& _Position, float _Radius, Float3 const& _Extents, NavQueryFilter const& _Filter, NavPointRef* _RandomPointRef) const
{
    NavPointRef StartRef;

    if (!QueryNearestPoly(_Position, _Extents, _Filter, &StartRef.PolyRef))
    {
        return false;
    }

    StartRef.Position = _Position;

    return QueryRandomPointAroundCircle(StartRef, _Radius, _Filter, _RandomPointRef);
}

bool AINavigationMesh::QueryRandomPointAroundCircle(Float3 const& _Position, float _Radius, Float3 const& _Extents, NavPointRef* _RandomPointRef) const
{
    return QueryRandomPointAroundCircle(_Position, _Radius, _Extents, QueryFilter, _RandomPointRef);
}

bool AINavigationMesh::QueryRandomPointAroundCircle(NavPointRef const& _StartRef, float _Radius, NavQueryFilter const& _Filter, NavPointRef* _RandomPointRef) const
{
    _RandomPointRef->PolyRef = 0;
    _RandomPointRef->Position.Clear();

    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->findRandomPointAroundCircle(_StartRef.PolyRef, (const float*)_StartRef.Position.ToPtr(), _Radius, _Filter.Filter.GetObject(), NavRandom, &_RandomPointRef->PolyRef, (float*)_RandomPointRef->Position.ToPtr());
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::QueryRandomPointAroundCircle(NavPointRef const& _StartRef, float _Radius, NavPointRef* _RandomPointRef) const
{
    return QueryRandomPointAroundCircle(_StartRef, _Radius, QueryFilter, _RandomPointRef);
}

bool AINavigationMesh::QueryClosestPointOnPoly(NavPointRef const& _PointRef, Float3* _Point, bool* _OverPolygon) const
{
    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->closestPointOnPoly(_PointRef.PolyRef, (const float*)_PointRef.Position.ToPtr(), (float*)_Point->ToPtr(), _OverPolygon);
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::QueryClosestPointOnPolyBoundary(NavPointRef const& _PointRef, Float3* _Point) const
{
    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->closestPointOnPolyBoundary(_PointRef.PolyRef, (const float*)_PointRef.Position.ToPtr(), (float*)_Point->ToPtr());
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::MoveAlongSurface(NavPointRef const& _StartRef, Float3 const& _Destination, NavQueryFilter const& _Filter, NavPolyRef* _Visited, int* _VisitedCount, int _MaxVisitedSize, Float3& _ResultPos) const
{
    if (!m_NavQuery)
    {
        return false;
    }

    _MaxVisitedSize = Math::Max(_MaxVisitedSize, 0);

    dtStatus Status = m_NavQuery->moveAlongSurface(_StartRef.PolyRef, (const float*)_StartRef.Position.ToPtr(), (const float*)_Destination.ToPtr(), _Filter.Filter.GetObject(), (float*)_ResultPos.ToPtr(), _Visited, _VisitedCount, _MaxVisitedSize);
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::MoveAlongSurface(NavPointRef const& _StartRef, Float3 const& _Destination, NavPolyRef* _Visited, int* _VisitedCount, int _MaxVisitedSize, Float3& _ResultPos) const
{
    return MoveAlongSurface(_StartRef, _Destination, QueryFilter, _Visited, _VisitedCount, _MaxVisitedSize, _ResultPos);
}

bool AINavigationMesh::MoveAlongSurface(Float3 const& _Position, Float3 const& _Destination, Float3 const& _Extents, NavQueryFilter const& _Filter, int _MaxVisitedSize, Float3& _ResultPos) const
{
    NavPointRef StartRef;

    m_LastVisitedPolys.Clear();

    if (!QueryNearestPoly(_Position, _Extents, _Filter, &StartRef.PolyRef))
    {
        return false;
    }

    StartRef.Position = _Position;

    m_LastVisitedPolys.ResizeInvalidate(Math::Max(_MaxVisitedSize, 0));

    int VisitedCount = 0;

    if (!MoveAlongSurface(StartRef, _Destination, _Filter, m_LastVisitedPolys.ToPtr(), &VisitedCount, m_LastVisitedPolys.Size(), _ResultPos))
    {
        m_LastVisitedPolys.Clear();
        return false;
    }

    m_LastVisitedPolys.Resize(VisitedCount);

    return true;
}

bool AINavigationMesh::MoveAlongSurface(Float3 const& _Position, Float3 const& _Destination, Float3 const& _Extents, int _MaxVisitedSize, Float3& _ResultPos) const
{
    return MoveAlongSurface(_Position, _Destination, _Extents, QueryFilter, _MaxVisitedSize, _ResultPos);
}

bool AINavigationMesh::FindPath(NavPointRef const& _StartRef, NavPointRef const& _EndRef, NavQueryFilter const& _Filter, NavPolyRef* _Path, int* _PathCount, const int _MaxPath) const
{
    *_PathCount = 0;

    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->findPath(_StartRef.PolyRef, _EndRef.PolyRef, (const float*)_StartRef.Position.ToPtr(), (const float*)_EndRef.Position.ToPtr(), _Filter.Filter.GetObject(), _Path, _PathCount, _MaxPath);
    if (dtStatusFailed(Status))
    {
        *_PathCount = 0;
        return false;
    }

    return true;
}

bool AINavigationMesh::FindPath(NavPointRef const& _StartRef, NavPointRef const& _EndRef, NavPolyRef* _Path, int* _PathCount, const int _MaxPath) const
{
    return FindPath(_StartRef, _EndRef, QueryFilter, _Path, _PathCount, _MaxPath);
}

bool AINavigationMesh::FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, NavQueryFilter const& _Filter, TPodVector<AINavigationPathPoint>& _PathPoints) const
{
    NavPointRef StartRef;
    NavPointRef EndRef;

    if (!QueryNearestPoly(_StartPos, _Extents, _Filter, &StartRef.PolyRef))
    {
        return false;
    }

    if (!QueryNearestPoly(_EndPos, _Extents, _Filter, &EndRef.PolyRef))
    {
        return false;
    }

    StartRef.Position = _StartPos;
    EndRef.Position   = _EndPos;

    int NumPolys;

    if (!FindPath(StartRef, EndRef, _Filter, TmpPolys, &NumPolys, MAX_POLYS))
    {
        return false;
    }

    Float3 ClosestLocalEnd = _EndPos;

    // Если не удалось проложить полный путь, установить конечную точку ближайшую к указанной конечной точке
    if (TmpPolys[NumPolys - 1] != EndRef.PolyRef)
    {
        m_NavQuery->closestPointOnPoly(TmpPolys[NumPolys - 1], (const float*)_EndPos.ToPtr(), (float*)ClosestLocalEnd.ToPtr(), 0);
    }

    int PathLength;

    m_NavQuery->findStraightPath((const float*)_StartPos.ToPtr(), (const float*)ClosestLocalEnd.ToPtr(), TmpPolys, NumPolys, (float*)TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &PathLength, MAX_POLYS);

    _PathPoints.ResizeInvalidate(PathLength);

    for (int i = 0; i < PathLength; ++i)
    {
        _PathPoints[i].Position = TmpPathPoints[i];
        _PathPoints[i].Flags    = TmpPathFlags[i];
    }

    return true;
}

bool AINavigationMesh::FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, TPodVector<AINavigationPathPoint>& _PathPoints) const
{
    return FindPath(_StartPos, _EndPos, _Extents, QueryFilter, _PathPoints);
}

bool AINavigationMesh::FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, NavQueryFilter const& _Filter, TPodVector<Float3>& _PathPoints) const
{
    NavPointRef StartRef;
    NavPointRef EndRef;

    if (!QueryNearestPoly(_StartPos, _Extents, _Filter, &StartRef.PolyRef))
    {
        return false;
    }

    if (!QueryNearestPoly(_EndPos, _Extents, _Filter, &EndRef.PolyRef))
    {
        return false;
    }

    StartRef.Position = _StartPos;
    EndRef.Position   = _EndPos;

    int NumPolys;

    if (!FindPath(StartRef, EndRef, _Filter, TmpPolys, &NumPolys, MAX_POLYS))
    {
        return false;
    }

    Float3 ClosestLocalEnd = _EndPos;

    // Если не удалось проложить полный путь, установить конечную точку ближайшую к указанной конечной точке
    if (TmpPolys[NumPolys - 1] != EndRef.PolyRef)
    {
        m_NavQuery->closestPointOnPoly(TmpPolys[NumPolys - 1], (const float*)_EndPos.ToPtr(), (float*)ClosestLocalEnd.ToPtr(), 0);
    }

    int PathLength;

    m_NavQuery->findStraightPath((const float*)_StartPos.ToPtr(), (const float*)ClosestLocalEnd.ToPtr(), TmpPolys, NumPolys, (float*)TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &PathLength, MAX_POLYS);

    _PathPoints.ResizeInvalidate(PathLength);

    Platform::Memcpy(_PathPoints.ToPtr(), TmpPathPoints, sizeof(Float3) * PathLength);

    return true;
}

bool AINavigationMesh::FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, TPodVector<Float3>& _PathPoints) const
{
    return FindPath(_StartPos, _EndPos, _Extents, QueryFilter, _PathPoints);
}

bool AINavigationMesh::FindStraightPath(Float3 const& _StartPos, Float3 const& _EndPos, NavPolyRef const* _Path, int _PathSize, Float3* _StraightPath, unsigned char* _StraightPathFlags, NavPolyRef* _StraightPathRefs, int* _StraightPathCount, int _MaxStraightPath, AI_NAV_MESH_STRAIGHTPATH_CROSSING _StraightPathCrossing) const
{
    if (!m_NavQuery)
    {
        return false;
    }

    dtStatus Status = m_NavQuery->findStraightPath((const float*)_StartPos.ToPtr(), (const float*)_EndPos.ToPtr(), _Path, _PathSize, (float*)_StraightPath, _StraightPathFlags, _StraightPathRefs, _StraightPathCount, _MaxStraightPath, (int)_StraightPathCrossing);
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::CalcDistanceToWall(NavPointRef const& _StartRef, float _Radius, NavQueryFilter const& _Filter, AINavigationHitResult& _HitResult) const
{
    dtStatus Status = m_NavQuery->findDistanceToWall(_StartRef.PolyRef, (const float*)_StartRef.Position.ToPtr(), _Radius, _Filter.Filter.GetObject(), &_HitResult.Distance, (float*)_HitResult.Position.ToPtr(), (float*)_HitResult.Normal.ToPtr());
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

bool AINavigationMesh::CalcDistanceToWall(NavPointRef const& _StartRef, float _Radius, AINavigationHitResult& _HitResult) const
{
    return CalcDistanceToWall(_StartRef, _Radius, QueryFilter, _HitResult);
}

bool AINavigationMesh::CalcDistanceToWall(Float3 const& _Position, float _Radius, Float3 const& _Extents, NavQueryFilter const& _Filter, AINavigationHitResult& _HitResult) const
{
    NavPointRef StartRef;

    if (!QueryNearestPoly(_Position, _Extents, _Filter, &StartRef.PolyRef))
    {
        return false;
    }

    StartRef.Position = _Position;

    return CalcDistanceToWall(StartRef, _Radius, _Filter, _HitResult);
}

bool AINavigationMesh::CalcDistanceToWall(Float3 const& _Position, float _Radius, Float3 const& _Extents, AINavigationHitResult& _HitResult) const
{
    return CalcDistanceToWall(_Position, _Radius, _Extents, QueryFilter, _HitResult);
}

bool AINavigationMesh::GetHeight(NavPointRef const& _PointRef, float* _Height) const
{
    if (!m_NavQuery)
    {
        *_Height = 0;
        return false;
    }

    dtStatus Status = m_NavQuery->getPolyHeight(_PointRef.PolyRef, (const float*)_PointRef.Position.ToPtr(), _Height);
    if (dtStatusFailed(Status))
    {
        *_Height = 0;
        return false;
    }

    return true;
}

bool AINavigationMesh::GetOffMeshConnectionPolyEndPoints(NavPolyRef _PrevRef, NavPolyRef _PolyRef, Float3* _StartPos, Float3* _EndPos) const
{
    if (!m_NavMesh)
    {
        return false;
    }

    dtStatus Status = m_NavMesh->getOffMeshConnectionPolyEndPoints(_PrevRef, _PolyRef, (float*)_StartPos->ToPtr(), (float*)_EndPos->ToPtr());
    if (dtStatusFailed(Status))
    {
        return false;
    }

    return true;
}

void AINavigationMesh::Update(float _TimeStep)
{
    if (m_TileCache)
    {
        m_TileCache->update(_TimeStep, m_NavMesh);
    }
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
int NavigationMeshComponent::FixupShortcuts( NavPolyRef * _Path, int _NPath ) const {
    if ( _NPath < 3 ) {
        return _NPath;
    }

    // Get connected polygons
    static const int maxNeis = 16;
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
    static const int maxLookAhead = 6;
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

int NavigationMeshComponent::FixupCorridor( NavPolyRef * _Path, const int _NPath, const int _MaxPath, const NavPolyRef * _Visited, const int _NVisited ) const {
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
    if (req+size > _MaxPath)
        size = _MaxPath-req;
    if (size)
        Platform::Memmove(_Path+req, _Path+orig, size*sizeof(NavPolyRef));

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


void AINavigationMesh::GatherNavigationGeometry(NavigationGeometry& Geometry)
{
    for (TListIterator<NavigationPrimitive> it(NavigationPrimitives) ; it ; ++it)
    {
        it->GatherNavigationGeometry(Geometry);
    }
}

HK_NAMESPACE_END
