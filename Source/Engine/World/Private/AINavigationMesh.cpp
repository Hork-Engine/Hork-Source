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


#include <Engine/World/Public/AINavigationMesh.h>
#include <Engine/World/Public/Level.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/FastLZCompressor.h>
#include <Engine/Core/Public/BV/BvIntersect.h>

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

#include <cfloat> // FLT_MAX

ARuntimeVariable RVDrawNavMeshBVTree( _CTS( "DrawNavMeshBVTree" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawNavMeshNodes( _CTS( "DrawNavMeshNodes" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawNavMeshWithClosedList( _CTS( "DrawNavMeshWithClosedList" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVDrawNavMeshTileBounds( _CTS( "DrawNavMeshTileBounds" ), _CTS( "0" ), VAR_CHEAT );

static_assert( sizeof( SNavPolyRef ) == sizeof( dtPolyRef ), "Type sizeof check" );

static const int MAX_LAYERS = 255;
static const bool RECAST_ENABLE_LOGGING = true;
static const bool RECAST_ENABLE_TIMINGS = true;

enum { MAX_POLYS = 2048 };
static SNavPolyRef TmpPolys[ MAX_POLYS ];
static SNavPolyRef TmpPathPolys[ MAX_POLYS ];
static Float3 TmpPathPoints[ MAX_POLYS ];
static unsigned char TmpPathFlags[ MAX_POLYS ];

struct STileCacheData {
    byte * Data;
    int Size;
};

struct STileCompressorCallback : public dtTileCacheCompressor {
    int maxCompressedSize( const int bufferSize ) override {
        return AFastLZCompressor::CalcAppropriateCompressedDataSize( bufferSize );
    }

    dtStatus compress(const unsigned char* buffer, const int bufferSize, unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) override {
        AFastLZCompressor compressor;

        size_t size;

        if ( !compressor.CompressData( buffer, bufferSize, compressed, size ) ) {
            return DT_FAILURE;
        }

        *compressedSize = size;

        return DT_SUCCESS;
    }

    dtStatus decompress(const unsigned char* compressed, const int compressedSize, unsigned char* buffer, const int maxBufferSize, int* bufferSize) override {
        AFastLZCompressor compressor;

        size_t size;

        if ( !compressor.DecompressData( compressed, compressedSize, buffer, size, maxBufferSize ) ) {
            return DT_FAILURE;
        }

        *bufferSize = size;

        return DT_SUCCESS;
    }
};

static STileCompressorCallback TileCompressorCallback;

struct ADetourLinearAllocator : public dtTileCacheAlloc {
    byte * Data;
    int Capacity;
    int Top;
    int High;

    ADetourLinearAllocator( const int _Capacity ) : Data(0), Capacity(0), Top(0), High(0) {
        Data = ( byte * )GZoneMemory.Alloc( _Capacity, 1 );
        Capacity = _Capacity;
    }

    ~ADetourLinearAllocator() {
        GZoneMemory.Dealloc( Data );
    }

    void reset() override {
        High = Math::Max( High, Top );
        Top = 0;
    }

    void * alloc( const size_t _Size ) override {
        if ( Top + _Size > Capacity ) {
            GLogger.Printf( "ADetourLinearAllocator: overflowed\n" );
            return 0;
        }
        byte * ptr = Data + Top;
        Top += _Size;
        return ptr;
    }

    void free( void * ) override {
    }
};

struct ADetourMeshProcess : public dtTileCacheMeshProcess {
    // NavMesh connections
    TPodArray< Float3 >         OffMeshConVerts;
    TPodArray< float >          OffMeshConRads;
    TPodArray< unsigned char >  OffMeshConDirs;
    TPodArray< unsigned char >  OffMeshConAreas;
    TPodArray< unsigned short > OffMeshConFlags;
    TPodArray< unsigned int >   OffMeshConId;
    int                         OffMeshConCount;
    ALevel *                    OwnerLevel;

    void process( struct dtNavMeshCreateParams * _Params, unsigned char * _PolyAreas, unsigned short * _PolyFlags ) override {

        // Update poly flags from areas.
        for ( int i = 0 ; i < _Params->polyCount ; ++i ) {
            if ( _PolyAreas[i] == DT_TILECACHE_WALKABLE_AREA ) {
                _PolyAreas[i] = AI_NAV_MESH_AREA_GROUND;
            }
            if ( _PolyAreas[i] == AI_NAV_MESH_AREA_GROUND ||
                 _PolyAreas[i] == AI_NAV_MESH_AREA_GRASS ||
                 _PolyAreas[i] == AI_NAV_MESH_AREA_ROAD ) {
                _PolyFlags[i] = AI_NAV_MESH_FLAGS_WALK;
            }
            else if ( _PolyAreas[i] == AI_NAV_MESH_AREA_WATER ) {
                _PolyFlags[i] = AI_NAV_MESH_FLAGS_SWIM;
            }
            else if ( _PolyAreas[i] == AI_NAV_MESH_AREA_DOOR ) {
                _PolyFlags[i] = AI_NAV_MESH_FLAGS_WALK | AI_NAV_MESH_FLAGS_DOOR;
            }
        }

        BvAxisAlignedBox clipBounds;
        rcVcopy( (float *)clipBounds.Mins.ToPtr(), _Params->bmin );
        rcVcopy( (float *)clipBounds.Maxs.ToPtr(), _Params->bmax );

        OffMeshConVerts.Clear();
        OffMeshConVerts.Clear();
        OffMeshConRads.Clear();
        OffMeshConDirs.Clear();
        OffMeshConAreas.Clear();
        OffMeshConFlags.Clear();
        OffMeshConId.Clear();

        BvAxisAlignedBox conBoundingBox;
        const float margin = 0.2f;
        OffMeshConCount = 0;
        for ( int i = 0 ; i < OwnerLevel->NavMeshConnections.Size() ; i++ ) {
            SAINavMeshConnection const & con = OwnerLevel->NavMeshConnections[i];

            con.CalcBoundingBox( conBoundingBox );
            conBoundingBox.Mins -= margin;
            conBoundingBox.Maxs += margin;

            if ( !BvBoxOverlapBox( clipBounds, conBoundingBox ) ) {
                // Connection is outside of clip bounds
                continue;
            }

            OffMeshConVerts.Append( con.StartPosition );
            OffMeshConVerts.Append( con.EndPosition );
            OffMeshConRads.Append( con.Radius );
            OffMeshConDirs.Append( con.bBidirectional ? DT_OFFMESH_CON_BIDIR : 0 );
            OffMeshConAreas.Append( con.AreaId );
            OffMeshConFlags.Append( con.Flags );
            OffMeshConId.Append( i ); // FIXME?

            OffMeshConCount++;
        }

        // Pass in off-mesh connections.
        _Params->offMeshConVerts = (float *)OffMeshConVerts.ToPtr();
        _Params->offMeshConRad = OffMeshConRads.ToPtr();
        _Params->offMeshConDir = OffMeshConDirs.ToPtr();
        _Params->offMeshConAreas = OffMeshConAreas.ToPtr();
        _Params->offMeshConFlags = OffMeshConFlags.ToPtr();
        _Params->offMeshConUserID = OffMeshConId.ToPtr();
        _Params->offMeshConCount = OffMeshConCount;
    }
};

class ARecastContext : public rcContext {
public:
    ARecastContext() {
        enableLog( RECAST_ENABLE_LOGGING );
        enableTimer( RECAST_ENABLE_TIMINGS );
    }

protected:
    // Virtual functions override
    void doResetLog() override {}
    void doLog( const rcLogCategory category, const char * msg, const int len ) override {
        switch ( category ) {
        case RC_LOG_PROGRESS:
            GLogger.Printf( "%s", msg );
            break;
        case RC_LOG_WARNING:
            GLogger.Printf( "%s", msg );
            break;
        case RC_LOG_ERROR:
            GLogger.Printf( "%s", msg );
            break;
        default:
            GLogger.Printf( "%s", msg );
            break;
        }
    }
    void doResetTimers() override {}
    void doStartTimer( const rcTimerLabel label ) override {}
    void doStopTimer( const rcTimerLabel label ) override {}
    int doGetAccumulatedTime( const rcTimerLabel label ) const override { return -1; }
};

static ARecastContext RecastContext;

AAINavigationMesh::AAINavigationMesh() {
    NavQuery = nullptr;
    NavMesh = nullptr;
    //Crowd = nullptr;
    TileCache = nullptr;
    LinearAllocator = nullptr;
    MeshProcess = nullptr;
    NumTilesX = 0;
    NumTilesZ = 0;
    BoundingBox.Clear();
}

AAINavigationMesh::~AAINavigationMesh() {
    Purge();
}

bool AAINavigationMesh::Initialize( ALevel * _OwnerLevel, SAINavMeshInitial const & _Initial ) {
    dtStatus status;

    Purge();

    OwnerLevel = _OwnerLevel;

    if ( _Initial.BoundingBox.IsEmpty() ) {
        GLogger.Printf( "AAINavigationMesh::Initialize: empty bounding box\n" );
        return false;
    }

    Initial = _Initial;

    BoundingBox = _Initial.BoundingBox;

    if ( Initial.NavVertsPerPoly < 3 ) {
        GLogger.Printf( "NavVertsPerPoly < 3\n" );

        Initial.NavVertsPerPoly = 3;
    } else  if ( Initial.NavVertsPerPoly > DT_VERTS_PER_POLYGON ) {
        GLogger.Printf( "NavVertsPerPoly > NAV_MAX_VERTS_PER_POLYGON\n" );

        Initial.NavVertsPerPoly = DT_VERTS_PER_POLYGON;
    }

    if ( Initial.MaxLayers > MAX_LAYERS ) {
        GLogger.Printf( "MaxLayers > MAX_LAYERS\n" );
        Initial.MaxLayers = MAX_LAYERS;
    }

    int gridWidth, gridHeight;

    rcCalcGridSize( (const float * )BoundingBox.Mins.ToPtr(),
                    (const float * )BoundingBox.Maxs.ToPtr(),
                    Initial.NavCellSize, &gridWidth, &gridHeight );

    NumTilesX = ( gridWidth + Initial.NavTileSize - 1 ) / Initial.NavTileSize;
    NumTilesZ = ( gridHeight + Initial.NavTileSize - 1 ) / Initial.NavTileSize;

    // Max tiles and max polys affect how the tile IDs are caculated.
    // There are 22 bits available for identifying a tile and a polygon.
    const unsigned int tileBits = Math::Min< unsigned int >( UInt( NumTilesX * NumTilesZ ).ToGreaterPowerOfTwo().Log2(), 14 );
    const unsigned int maxTiles = 1 << tileBits;
    const unsigned int maxPolysPerTile = 1u << (22 - tileBits);

    TileWidth = Initial.NavTileSize * Initial.NavCellSize;

    dtNavMeshParams params;
    ZeroMem( &params, sizeof( params ) );
    rcVcopy( params.orig, (const float * )BoundingBox.Mins.ToPtr() );
    params.tileWidth = TileWidth;
    params.tileHeight = TileWidth;
    params.maxTiles = maxTiles;
    params.maxPolys = maxPolysPerTile;

    NavMesh = dtAllocNavMesh();
    if ( !NavMesh ) {
        Purge();
        GLogger.Printf( "Failed on dtAllocNavMesh\n" );
        return false;
    }

    status = NavMesh->init( &params );
    if ( dtStatusFailed( status ) ) {
        Purge();
        GLogger.Printf( "Could not initialize navmesh\n" );
        return false;
    }

    NavQuery = dtAllocNavMeshQuery();
    if ( !NavQuery ) {
        Purge();
        GLogger.Printf( "Failed on dtAllocNavMeshQuery\n" );
        return false;
    }

    const int MAX_NODES = 2048;
    status = NavQuery->init( NavMesh, MAX_NODES );
    if ( dtStatusFailed( status ) ) {
        Purge();
        GLogger.Printf( "Could not initialize navmesh query" );
        return false;
    }

    if ( Initial.bDynamicNavMesh ) {
        // Create tile cache

        dtTileCacheParams tileCacheParams;
        ZeroMem( &tileCacheParams, sizeof( tileCacheParams ) );
        rcVcopy( tileCacheParams.orig, (const float *)Initial.BoundingBox.Mins.ToPtr() );
        tileCacheParams.cs = Initial.NavCellSize;
        tileCacheParams.ch = Initial.NavCellHeight;
        tileCacheParams.width = Initial.NavTileSize;
        tileCacheParams.height = Initial.NavTileSize;
        tileCacheParams.walkableHeight = Initial.NavWalkableHeight;
        tileCacheParams.walkableRadius = Initial.NavWalkableRadius;
        tileCacheParams.walkableClimb = Initial.NavWalkableClimb;
        tileCacheParams.maxSimplificationError = Initial.NavEdgeMaxError;
        tileCacheParams.maxTiles = maxTiles * Initial.MaxLayers;
        tileCacheParams.maxObstacles = Initial.MaxDynamicObstacles;

        TileCache = dtAllocTileCache();
        if ( !TileCache ) {
            Purge();
            GLogger.Printf( "Failed on dtAllocTileCache\n" );
            return false;
        }

        const size_t MaxLinearAllocatorCapacity = 32 << 10; // 32 KB

        LinearAllocator = new ( GZoneMemory.Alloc( sizeof( ADetourLinearAllocator ), 1 ) ) ADetourLinearAllocator( MaxLinearAllocatorCapacity );

        MeshProcess = new ( GZoneMemory.Alloc( sizeof( ADetourMeshProcess ), 1 ) ) ADetourMeshProcess();
        MeshProcess->OwnerLevel = OwnerLevel;

        status = TileCache->init( &tileCacheParams, LinearAllocator, &TileCompressorCallback, MeshProcess );
        if ( dtStatusFailed( status ) ) {
            Purge();
            GLogger.Printf( "Could not initialize tile cache\n" );
            return false;
        }

        // TODO: Add obstacles here?
    }

    return true;
}

bool AAINavigationMesh::Build() {
    Int2 regionMins( 0, 0 );
    Int2 regionMaxs( NumTilesX - 1, NumTilesZ - 1 );

    return BuildTiles( regionMins, regionMaxs );
}

bool AAINavigationMesh::Build( Int2 const & _Mins, Int2 const & _Maxs ) {
    Int2 regionMins;
    Int2 regionMaxs;

    regionMins.X = Math::Clamp< int >( _Mins.X, 0, NumTilesX - 1 );
    regionMins.Y = Math::Clamp< int >( _Mins.Y, 0, NumTilesZ - 1 );
    regionMaxs.X = Math::Clamp< int >( _Maxs.X, 0, NumTilesX - 1 );
    regionMaxs.Y = Math::Clamp< int >( _Maxs.Y, 0, NumTilesZ - 1 );

    return BuildTiles( regionMins, regionMaxs );
}

bool AAINavigationMesh::Build( BvAxisAlignedBox const & _BoundingBox ) {
    Int2 mins( ( _BoundingBox.Mins.X - BoundingBox.Mins.X ) / TileWidth,
               ( _BoundingBox.Mins.Z - BoundingBox.Mins.Z ) / TileWidth );
    Int2 maxs( ( _BoundingBox.Maxs.X - BoundingBox.Mins.X ) / TileWidth,
               ( _BoundingBox.Maxs.Z - BoundingBox.Mins.Z ) / TileWidth );

    return Build( mins, maxs );
}

void AAINavigationMesh::GetTileWorldBounds( int _X, int _Z, BvAxisAlignedBox & _BoundingBox ) const {
    _BoundingBox.Mins[0] = BoundingBox.Mins[0] + _X * TileWidth;
    _BoundingBox.Mins[1] = BoundingBox.Mins[1];
    _BoundingBox.Mins[2] = BoundingBox.Mins[2] + _Z * TileWidth;

    _BoundingBox.Maxs[0] = BoundingBox.Mins[0] + ( _X + 1 ) * TileWidth;
    _BoundingBox.Maxs[1] = BoundingBox.Maxs[1];
    _BoundingBox.Maxs[2] = BoundingBox.Mins[2] + ( _Z + 1 ) * TileWidth;
}

bool AAINavigationMesh::BuildTiles( Int2 const & _Mins, Int2 const & _Maxs ) {
    if ( !NavMesh ) {
        GLogger.Printf( "AAINavigationMesh::BuildTiles: navmesh must be initialized\n" );
        return false;
    }

    unsigned int totalBuilt = 0;
    for ( int z = _Mins[1] ; z <= _Maxs[1] ; z++ ) {
        for ( int x = _Mins[0] ; x <= _Maxs[0] ; x++ ) {
            if ( BuildTile( x, z ) ) {
                totalBuilt++;
            }
        }
    }
    return totalBuilt > 0;
}

// Based on rcMarkWalkableTriangles
static void MarkWalkableTriangles( const float _WalkableSlopeAngle, Float3 const * _Vertices, unsigned int const * _Indices, int _NumTriangles, int _FirstTriangle, TBitMask<> const & _WalkableMask, unsigned char * _Areas ) {
    Float3 perpendicular;
    float perpendicularLength;

    const float WalkableThreshold = cosf( Math::Radians( _WalkableSlopeAngle ) );

    for ( int i = 0 ; i < _NumTriangles ; ++i ) {
        int triangleNum = _FirstTriangle + i;
        if ( _WalkableMask.IsMarked( triangleNum ) ) {
            unsigned int const * tri = &_Indices[ triangleNum * 3 ];

            perpendicular = (_Vertices[tri[1]] - _Vertices[tri[0]]).Cross( _Vertices[tri[2]] - _Vertices[tri[0]] );
            perpendicularLength = perpendicular.Length();
            if ( perpendicularLength > 0 && perpendicular[1] > WalkableThreshold * perpendicularLength ) {
                _Areas[i] = RC_WALKABLE_AREA;
            }
        }
    }
}

static int PointInPoly2D( int nvert, const float * verts, const float * p ) {
    int i, j, c = 0;
    for (i = 0, j = nvert-1; i < nvert; j = i++) {
        const float* vi = &verts[i*2];
        const float* vj = &verts[j*2];
        if (((vi[1] > p[1]) != (vj[1] > p[1])) &&
            (p[0] < (vj[0]-vi[0]) * (p[1]-vi[1]) / (vj[1]-vi[1]) + vi[0]) )
            c = !c;
    }
    return c;
}

bool AAINavigationMesh::BuildTile( int _X, int _Z ) {
    struct TemportalData {
        rcHeightfield * Heightfield;
        rcCompactHeightfield * CompactHeightfield;
        rcContourSet * ContourSet;
        rcPolyMesh * PolyMesh;
        rcPolyMeshDetail * PolyMeshDetail;
        rcHeightfieldLayerSet * LayerSet;

        TemportalData() {
            ZeroMem( this, sizeof( *this ) );
        }

        ~TemportalData() {
            rcFreeHeightField( Heightfield );
            rcFreeCompactHeightfield( CompactHeightfield );
            rcFreeContourSet( ContourSet );
            rcFreePolyMesh( PolyMesh );
            rcFreePolyMeshDetail( PolyMeshDetail );
            rcFreeHeightfieldLayerSet( LayerSet );
        }
    };

    BvAxisAlignedBox tileWorldBounds;
    BvAxisAlignedBox tileWorldBoundsWithPadding;

    AN_Assert( NavMesh );

    RemoveTile( _X, _Z );

    GetTileWorldBounds( _X, _Z, tileWorldBounds );

    rcConfig config;
    ZeroMem( &config, sizeof( config ) );
    config.cs = Initial.NavCellSize;
    config.ch = Initial.NavCellHeight;
    config.walkableSlopeAngle = Initial.NavWalkableSlopeAngle;
    config.walkableHeight = (int)Math::Ceil( Initial.NavWalkableHeight / config.ch );
    config.walkableClimb = (int)Math::Floor( Initial.NavWalkableClimb / config.ch );
    config.walkableRadius = (int)Math::Ceil( Initial.NavWalkableRadius / config.cs );
    config.maxEdgeLen = (int)(Initial.NavEdgeMaxLength / Initial.NavCellSize);
    config.maxSimplificationError = Initial.NavEdgeMaxError;
    config.minRegionArea = (int)rcSqr( Initial.NavMinRegionSize );		// Note: area = size*size
    config.mergeRegionArea = (int)rcSqr( Initial.NavMergeRegionSize );	// Note: area = size*size
    config.detailSampleDist = Initial.NavDetailSampleDist < 0.9f ? 0 : Initial.NavCellSize * Initial.NavDetailSampleDist;
    config.detailSampleMaxError = Initial.NavCellHeight * Initial.NavDetailSampleMaxError;
    config.tileSize = Initial.NavTileSize;
    config.borderSize = config.walkableRadius + 3; // radius + padding
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.maxVertsPerPoly = Initial.NavVertsPerPoly;

    rcVcopy( config.bmin, (const float *)tileWorldBounds.Mins.ToPtr() );
    rcVcopy( config.bmax, (const float *)tileWorldBounds.Maxs.ToPtr() );

    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    for ( int i = 0 ; i < 3 ; i++ ) {
        tileWorldBoundsWithPadding.Mins[i] = config.bmin[i];
        tileWorldBoundsWithPadding.Maxs[i] = config.bmax[i];
    }

    TPodArray< Float3 > vertices;
    TPodArray< unsigned int > indices;
    BvAxisAlignedBox boundingBox;
    TBitMask<> walkableMask;

    OwnerLevel->GenerateSourceNavMesh( vertices, indices, walkableMask, boundingBox, &tileWorldBoundsWithPadding );

    if ( boundingBox.IsEmpty() || indices.IsEmpty() ) {
        // Empty tile
        return true;
    }

    //rcVcopy( config.bmin, (const float *)boundingBox.Mins.ToPtr() );
    //rcVcopy( config.bmax, (const float *)boundingBox.Maxs.ToPtr() );
    config.bmin[1] = boundingBox.Mins.Y;
    config.bmax[1] = boundingBox.Maxs.Y;
    tileWorldBoundsWithPadding = boundingBox;

    TemportalData temporal;

    // Allocate voxel heightfield where we rasterize our input data to.
    temporal.Heightfield = rcAllocHeightfield();
    if ( !temporal.Heightfield ) {
        GLogger.Printf( "Failed on rcAllocHeightfield\n" );
        return false;
    }

    if ( !rcCreateHeightfield( &RecastContext, *temporal.Heightfield, config.width, config.height,
                               config.bmin, config.bmax, config.cs, config.ch ) ) {
        GLogger.Printf( "Failed on rcCreateHeightfield\n" );
        return false;
    }

    int trianglesCount = indices.Size() / 3;

    int hunkMark = GHunkMemory.SetHunkMark();

    // Allocate array that can hold triangle area types.
    // If you have multiple meshes you need to process, allocate
    // and array which can hold the max number of triangles you need to process.
    unsigned char * triangleAreaTypes = ( unsigned char * )GHunkMemory.HunkMemoryCleared( trianglesCount, 1 );

    // Find triangles which are walkable based on their slope and rasterize them.
    // If your input data is multiple meshes, you can transform them here, calculate
    // the are type for each of the meshes and rasterize them.
    MarkWalkableTriangles( config.walkableSlopeAngle, vertices.ToPtr(), indices.ToPtr(), trianglesCount, 0, walkableMask, triangleAreaTypes );

    bool rasterized = rcRasterizeTriangles( &RecastContext,
                                            (float const *)vertices.ToPtr(),
                                            vertices.Size(),
                                            (int const *)indices.ToPtr(),
                                            triangleAreaTypes,
                                            trianglesCount,
                                            *temporal.Heightfield,
                                            config.walkableClimb );

    GHunkMemory.ClearToMark( hunkMark );

    if ( !rasterized ) {
        GLogger.Printf( "Failed on rcRasterizeTriangles\n" );
        return false;
    }

    // Filter walkables surfaces.

    // Once all geoemtry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    rcFilterLowHangingWalkableObstacles( &RecastContext, config.walkableClimb, *temporal.Heightfield );
    rcFilterLedgeSpans( &RecastContext, config.walkableHeight, config.walkableClimb, *temporal.Heightfield );
    rcFilterWalkableLowHeightSpans( &RecastContext, config.walkableHeight, *temporal.Heightfield );

    // Partition walkable surface to simple regions.
    // Compact the heightfield so that it is faster to handle from now on.
    // This will result more cache coherent data as well as the neighbours
    // between walkable cells will be calculated.
    temporal.CompactHeightfield = rcAllocCompactHeightfield();
    if ( !temporal.CompactHeightfield ) {
        GLogger.Printf( "Failed on rcAllocCompactHeightfield\n" );
        return false;
    }

    if ( !rcBuildCompactHeightfield( &RecastContext, config.walkableHeight, config.walkableClimb, *temporal.Heightfield, *temporal.CompactHeightfield ) ) {
        GLogger.Printf( "Failed on rcBuildCompactHeightfield\n" );
        return false;
    }

    // Erode the walkable area by agent radius.
    if ( !rcErodeWalkableArea( &RecastContext, config.walkableRadius, *temporal.CompactHeightfield ) ) {
        GLogger.Printf( "AAINavigationMesh::Build: Failed on rcErodeWalkableArea\n" );
        return false;
    }
#if 1
    BvAxisAlignedBox areaBoundingBox;
    for ( int areaNum = 0; areaNum < OwnerLevel->NavigationAreas.Size() ; ++areaNum ) {
        SAINavigationArea const & area = OwnerLevel->NavigationAreas[areaNum];
        rcCompactHeightfield const & chf = *temporal.CompactHeightfield;

        area.CalcBoundingBox( areaBoundingBox );

        if ( areaBoundingBox.IsEmpty() ) {
            // Invalid bounding box
            continue;
        }

        if ( !BvBoxOverlapBox( tileWorldBoundsWithPadding, areaBoundingBox ) ) {
            // Area is outside of tile bounding box
            continue;
        }

        // The next code is based on rcMarkBoxArea and rcMarkConvexPolyArea
        int minx = (int)((areaBoundingBox.Mins[0]-chf.bmin[0])/chf.cs);
        int miny = (int)((areaBoundingBox.Mins[1]-chf.bmin[1])/chf.ch);
        int minz = (int)((areaBoundingBox.Mins[2]-chf.bmin[2])/chf.cs);
        int maxx = (int)((areaBoundingBox.Maxs[0]-chf.bmin[0])/chf.cs);
        int maxy = (int)((areaBoundingBox.Maxs[1]-chf.bmin[1])/chf.ch);
        int maxz = (int)((areaBoundingBox.Maxs[2]-chf.bmin[2])/chf.cs);

        if (maxx < 0) continue;
        if (minx >= chf.width) continue;
        if (maxz < 0) continue;
        if (minz >= chf.height) continue;

        if (minx < 0) minx = 0;
        if (maxx >= chf.width) maxx = chf.width-1;
        if (minz < 0) minz = 0;
        if (maxz >= chf.height) maxz = chf.height-1;

        if ( area.Shape == AI_NAV_MESH_AREA_SHAPE_CONVEX_VOLUME ) {            
            for (int z = minz; z <= maxz; ++z) {
                for (int x = minx; x <= maxx; ++x) {
                    const rcCompactCell& c = chf.cells[x+z*chf.width];
                    for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i) {
                        rcCompactSpan& s = chf.spans[i];
                        if (chf.areas[i] == RC_NULL_AREA)
                            continue;
                        if ((int)s.y >= miny && (int)s.y <= maxy) {
                            float p[2];
                            p[0] = chf.bmin[0] + (x+0.5f)*chf.cs;
                            p[1] = chf.bmin[2] + (z+0.5f)*chf.cs;

                            if ( PointInPoly2D( area.NumConvexVolumeVerts, (float *)area.ConvexVolume[0].ToPtr(), p ) ) {
                                chf.areas[i] = area.AreaId;
                            }
                        }
                    }
                }
            }
        } else {
            for (int z = minz; z <= maxz; ++z) {
                for (int x = minx; x <= maxx; ++x) {
                    const rcCompactCell& c = chf.cells[x+z*chf.width];
                    for (int i = (int)c.index, ni = (int)(c.index+c.count); i < ni; ++i) {
                        rcCompactSpan& s = chf.spans[i];
                        if ((int)s.y >= miny && (int)s.y <= maxy) {
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

    if ( Initial.RecastPartitionMethod == AI_NAV_MESH_PARTITION_WATERSHED ) {
        // Prepare for region partitioning, by calculating distance field along the walkable surface.
        if ( !rcBuildDistanceField( &RecastContext, *temporal.CompactHeightfield ) ) {
            GLogger.Printf( "Could not build distance field\n" );
            return false;
        }

        // Partition the walkable surface into simple regions without holes.
        if ( !rcBuildRegions( &RecastContext, *temporal.CompactHeightfield, config.borderSize/*0*/, config.minRegionArea, config.mergeRegionArea ) ) {
            GLogger.Printf( "Could not build watershed regions\n" );
            return false;
        }
    } else if ( Initial.RecastPartitionMethod == AI_NAV_MESH_PARTITION_MONOTONE ) {
        // Partition the walkable surface into simple regions without holes.
        // Monotone partitioning does not need distancefield.
        if ( !rcBuildRegionsMonotone( &RecastContext, *temporal.CompactHeightfield, config.borderSize/*0*/, config.minRegionArea, config.mergeRegionArea ) ) {
            GLogger.Printf( "Could not build monotone regions\n" );
            return false;
        }
    } else { // RECAST_PARTITION_LAYERS
        // Partition the walkable surface into simple regions without holes.
        if ( !rcBuildLayerRegions( &RecastContext, *temporal.CompactHeightfield, config.borderSize/*0*/, config.minRegionArea ) ) {
            GLogger.Printf( "Could not build layer regions\n" );
            return false;
        }
    }

    if ( Initial.bDynamicNavMesh ) {

        temporal.LayerSet = rcAllocHeightfieldLayerSet();
        if ( !temporal.LayerSet ) {
            GLogger.Printf( "Failed on rcAllocHeightfieldLayerSet\n" );
            return false;
        }

        if ( !rcBuildHeightfieldLayers( &RecastContext, *temporal.CompactHeightfield, config.borderSize, config.walkableHeight, *temporal.LayerSet ) ) {
            GLogger.Printf( "Failed on rcBuildHeightfieldLayers\n" );
            return false;
        }

        STileCacheData cacheData[MAX_LAYERS];

        int numLayers = Math::Min( temporal.LayerSet->nlayers, MAX_LAYERS );
        int numValidLayers = 0;
        for ( int i = 0 ; i < numLayers ; i++ ) {
            STileCacheData * tile = &cacheData[i];
            rcHeightfieldLayer const * layer = &temporal.LayerSet->layers[i];

            dtTileCacheLayerHeader header;
            header.magic = DT_TILECACHE_MAGIC;
            header.version = DT_TILECACHE_VERSION;
            header.tx = _X;
            header.ty = _Z;
            header.tlayer = i;
            dtVcopy( header.bmin, layer->bmin );
            dtVcopy( header.bmax, layer->bmax );
            header.width = (unsigned char)layer->width;
            header.height = (unsigned char)layer->height;
            header.minx = (unsigned char)layer->minx;
            header.maxx = (unsigned char)layer->maxx;
            header.miny = (unsigned char)layer->miny;
            header.maxy = (unsigned char)layer->maxy;
            header.hmin = (unsigned short)layer->hmin;
            header.hmax = (unsigned short)layer->hmax;

            dtStatus status = dtBuildTileCacheLayer( &TileCompressorCallback, &header, layer->heights, layer->areas, layer->cons, &tile->Data, &tile->Size );
            if ( dtStatusFailed( status ) ) {
                GLogger.Printf( "Failed on dtBuildTileCacheLayer\n" );
                break;
            }

            numValidLayers++;
        }

        int cacheLayerCount = 0;
        //int cacheCompressedSize = 0;
        //int cacheRawSize = 0;
        //const int layerBufferSize = dtAlign4( sizeof( dtTileCacheLayerHeader ) ) + Initial.NavTileSize * Initial.NavTileSize * numLayers;
        for ( int i = 0 ; i < numValidLayers ; i++ ) {
            dtCompressedTileRef ref;
            dtStatus status = TileCache->addTile( cacheData[i].Data, cacheData[i].Size, DT_COMPRESSEDTILE_FREE_DATA, &ref );
            if ( dtStatusFailed( status ) ) {
                 dtFree( cacheData[i].Data );
                 cacheData[i].Data = nullptr;
                 continue;
            }

            status = TileCache->buildNavMeshTile( ref, NavMesh );
            if ( dtStatusFailed( status ) ) {
                GLogger.Printf( "Failed to build navmesh tile\n" );
            }

            cacheLayerCount++;
//            cacheCompressedSize += cacheData[i].Size;
//            cacheRawSize += layerBufferSize;
        }

        if ( cacheLayerCount == 0 ) {
            return false;
        }


//        int cacheBuildMemUsage = LinearAllocator->High;
//        const float compressionRatio = (float)cacheCompressedSize / (float)(cacheRawSize+1);

//        int totalMemoryUsage = 0;
//        const dtNavMesh * mesh = NavMesh;
//        for ( int i = 0 ; i < mesh->getMaxTiles() ; ++i ) {
//            const dtMeshTile * tile = mesh->getTile( i );
//            if ( tile->header ) {
//                totalMemoryUsage += tile->dataSize;
//            }
//        }

//        GLogger.Printf( "Processed navigation data:\n"
//                        "Total memory usage for NavMesh: %.1f kB\n"
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

    } else {

        temporal.ContourSet = rcAllocContourSet();
        if ( !temporal.ContourSet ) {
            GLogger.Printf( "Failed on rcAllocContourSet\n" );
            return false;
        }

        // Trace and simplify region contours.

        // Create contours.
        if ( !rcBuildContours( &RecastContext, *temporal.CompactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *temporal.ContourSet ) ) {
            GLogger.Printf( "Could not create contours\n" );
            return false;
        }

        temporal.PolyMesh = rcAllocPolyMesh();
        if ( !temporal.PolyMesh ) {
            GLogger.Printf( "Failed on rcAllocPolyMesh\n" );
            return false;
        }

        // Build polygon navmesh from the contours.
        if ( !rcBuildPolyMesh( &RecastContext, *temporal.ContourSet, config.maxVertsPerPoly, *temporal.PolyMesh ) ) {
            GLogger.Printf( "Could not triangulate contours\n" );
            return false;
        }

        if ( !temporal.PolyMesh->nverts || !temporal.PolyMesh->npolys ) {
            // no data to build tile
            return true;
        }

        temporal.PolyMeshDetail = rcAllocPolyMeshDetail();
        if ( !temporal.PolyMeshDetail ) {
            GLogger.Printf( "Failed on rcAllocPolyMeshDetail\n" );
            return false;
        }

        // Create detail mesh which allows to access approximate height on each polygon.
        if ( !rcBuildPolyMeshDetail( &RecastContext,
                                     *temporal.PolyMesh,
                                     *temporal.CompactHeightfield,
                                     config.detailSampleDist,
                                     config.detailSampleMaxError,
                                     *temporal.PolyMeshDetail ) ) {
            GLogger.Printf( "Could not build detail mesh\n" );
            return false;
        }

        // At this point the navigation mesh data is ready
        // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

        // Update poly flags from areas.
        static_assert( AI_NAV_MESH_AREA_GROUND == RC_WALKABLE_AREA, "Navmesh area id static check" );
        for ( int i = 0 ; i < temporal.PolyMesh->npolys; ++i ) {

            if ( temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_GROUND ||
                 temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_GRASS ||
                 temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_ROAD ) {

                temporal.PolyMesh->flags[i] = AI_NAV_MESH_FLAGS_WALK;

            } else if ( temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_WATER ) {

                temporal.PolyMesh->flags[i] = AI_NAV_MESH_FLAGS_SWIM;

            } else if ( temporal.PolyMesh->areas[i] == AI_NAV_MESH_AREA_DOOR ) {

                temporal.PolyMesh->flags[i] = AI_NAV_MESH_FLAGS_WALK | AI_NAV_MESH_FLAGS_DOOR;
            }
        }

        BvAxisAlignedBox conBoundingBox;
        const float margin = 0.2f;
        TPodArray< Float3 > offMeshConVerts;
        TPodArray< float > offMeshConRads;
        TPodArray< unsigned char > offMeshConDirs;
        TPodArray< unsigned char > offMeshConAreas;
        TPodArray< unsigned short > offMeshConFlags;
        TPodArray< unsigned int > offMeshConId;
        int offMeshConCount = 0;
        for ( int i = 0 ; i < OwnerLevel->NavMeshConnections.Size() ; i++ ) {
            SAINavMeshConnection const & con = OwnerLevel->NavMeshConnections[i];

            con.CalcBoundingBox( conBoundingBox );
            conBoundingBox.Mins -= margin;
            conBoundingBox.Maxs += margin;

            if ( !BvBoxOverlapBox( tileWorldBoundsWithPadding, conBoundingBox ) ) {
                // Connection is outside of tile bounding box
                continue;
            }

            offMeshConVerts.Append( con.StartPosition );
            offMeshConVerts.Append( con.EndPosition );
            offMeshConRads.Append( con.Radius );
            offMeshConDirs.Append( con.bBidirectional ? DT_OFFMESH_CON_BIDIR : 0 );
            offMeshConAreas.Append( con.AreaId );
            offMeshConFlags.Append( con.Flags );
            offMeshConId.Append( i ); // FIXME?

            offMeshConCount++;
        }

        // Create Detour data from poly mesh.

        dtNavMeshCreateParams params;
        ZeroMem( &params, sizeof( params ) );
        params.verts = temporal.PolyMesh->verts;
        params.vertCount = temporal.PolyMesh->nverts;
        params.polys = temporal.PolyMesh->polys;
        params.polyAreas = temporal.PolyMesh->areas;
        params.polyFlags = temporal.PolyMesh->flags;
        params.polyCount = temporal.PolyMesh->npolys;
        params.nvp = temporal.PolyMesh->nvp;
        params.detailMeshes = temporal.PolyMeshDetail->meshes;
        params.detailVerts = temporal.PolyMeshDetail->verts;
        params.detailVertsCount = temporal.PolyMeshDetail->nverts;
        params.detailTris = temporal.PolyMeshDetail->tris;
        params.detailTriCount = temporal.PolyMeshDetail->ntris;
        params.offMeshConVerts = (float *)offMeshConVerts.ToPtr();
        params.offMeshConRad = offMeshConRads.ToPtr();
        params.offMeshConDir = offMeshConDirs.ToPtr();
        params.offMeshConAreas = offMeshConAreas.ToPtr();
        params.offMeshConFlags = offMeshConFlags.ToPtr();
        params.offMeshConUserID = offMeshConId.ToPtr();
        params.offMeshConCount = offMeshConCount;
        params.walkableHeight = Initial.NavWalkableHeight;
        params.walkableRadius = Initial.NavWalkableRadius;
        params.walkableClimb = Initial.NavWalkableClimb;
        params.tileX = _X;
        params.tileY = _Z;
        rcVcopy( params.bmin, temporal.PolyMesh->bmin );
        rcVcopy( params.bmax, temporal.PolyMesh->bmax );
        params.cs = config.cs;
        params.ch = config.ch;
        params.buildBvTree = true;

        unsigned char * navData = 0;
        int navDataSize = 0;

        if ( !dtCreateNavMeshData( &params, &navData, &navDataSize ) ) {

            if ( params.vertCount >= 0xffff ) {
                GLogger.Printf( "vertCount >= 0xffff\n" );
            }

            GLogger.Printf( "Could not build navmesh tile\n" );
            return false;
        }

        dtStatus status = NavMesh->addTile( navData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr );
        if ( dtStatusFailed( status ) ) {
            dtFree( navData );
            GLogger.Printf( "Could not add tile to navmesh\n" );
            return false;
        }
    }

    return true;
}

void AAINavigationMesh::RemoveTile( int _X, int _Z ) {
    if ( !NavMesh ) {
        return;
    }

    if ( Initial.bDynamicNavMesh ) {

        AN_Assert( TileCache );

        dtCompressedTileRef compressedTiles[MAX_LAYERS];
        int count = TileCache->getTilesAt( _X, _Z, compressedTiles, Initial.MaxLayers );
        for ( int i = 0 ; i < count ; i++ ) {
            byte * data = nullptr;
            dtStatus status = TileCache->removeTile( compressedTiles[i], &data, nullptr );
            if ( dtStatusFailed( status ) ) {
                continue;
            }
            dtFree( data );
        }
    } else {

        dtTileRef ref = NavMesh->getTileRefAt( _X, _Z, 0 );
        if ( ref ) {
            NavMesh->removeTile( ref, nullptr, nullptr );
        }

    }
}

void AAINavigationMesh::RemoveTiles() {
    if ( !NavMesh ) {
        return;
    }

    if ( Initial.bDynamicNavMesh ) {

        AN_Assert( TileCache );

        int numTiles = TileCache->getTileCount();
        for ( int i = 0 ; i < numTiles ; i++ ) {
            const dtCompressedTile * tile = TileCache->getTile( i );
            if ( tile && tile->header ) {
                TileCache->removeTile( TileCache->getTileRef( tile ), nullptr, nullptr );
            }
        }

    } else {
        int numTiles = NavMesh->getMaxTiles();
        const dtNavMesh * navMesh = NavMesh;
        for ( int i = 0 ; i < numTiles ; i++ ) {
            const dtMeshTile * tile = navMesh->getTile( i );
            if ( tile && tile->header ) {
                NavMesh->removeTile( NavMesh->getTileRef( tile ), nullptr, nullptr );
            }
        }
    }
}

void AAINavigationMesh::RemoveTiles( Int2 const & _Mins, Int2 const & _Maxs ) {
    if ( !NavMesh ) {
        return;
    }
    for ( int z = _Mins[1] ; z <= _Maxs[1] ; z++ ) {
        for ( int x = _Mins[0] ; x <= _Maxs[0] ; x++ ) {
            RemoveTile( x, z );
        }
    }
}

bool AAINavigationMesh::IsTileExsist( int _X, int _Z ) const {
    return NavMesh ? NavMesh->getTileAt( _X, _Z, 0 ) != nullptr : false;
}

void AAINavigationMesh::AddObstacle( AAINavMeshObstacle * _Obstacle ) {
    if ( !TileCache ) {
        return;
    }

    dtObstacleRef ref;
    dtStatus status;

    // TODO:
    //while ( TileCache->isObstacleQueueFull() ) {
    //    TileCache->update( 1, NavMesh );
    //}

    if ( _Obstacle->Shape == AI_NAV_MESH_OBSTACLE_BOX ) {
        Float3 Mins = _Obstacle->Position - _Obstacle->HalfExtents;
        Float3 Maxs = _Obstacle->Position + _Obstacle->HalfExtents;
        status = TileCache->addBoxObstacle( (const float *)Mins.ToPtr(), (const float *)Maxs.ToPtr(), &ref );
    } else {

        while ( 1 ) {
            status = TileCache->addObstacle( (const float *)_Obstacle->Position.ToPtr(), _Obstacle->Radius, _Obstacle->Height, &ref );

            if ( status & DT_BUFFER_TOO_SMALL ) {
                TileCache->update( 1, NavMesh );
                continue;
            }

            break;
        }
    }

    if ( dtStatusFailed( status ) ) {
        GLogger.Printf( "Failed to add navmesh obstacle\n" );
        if ( status & DT_OUT_OF_MEMORY ) {
            GLogger.Printf( "DT_OUT_OF_MEMORY\n" );
        }
        return;
    }
GLogger.Printf("AddObstacle: %d\n",ref);
    _Obstacle->ObstacleRef = ref;
}

void AAINavigationMesh::RemoveObstacle( AAINavMeshObstacle * _Obstacle ) {
    if ( !TileCache ) {
        return;
    }

    if ( !_Obstacle->ObstacleRef ) {
        return;
    }

    // TODO:
    //while ( TileCache->isObstacleQueueFull() ) {
    //    TileCache->update( 1, NavMesh );
    //}

    dtStatus status;

    while ( 1 ) {
        status = TileCache->removeObstacle( _Obstacle->ObstacleRef );

        if ( status & DT_BUFFER_TOO_SMALL ) {
            TileCache->update( 1, NavMesh );
            continue;
        }

        break;
    }

    if ( dtStatusFailed( status ) ) {
        GLogger.Printf( "Failed to remove navmesh obstacle\n" );
        return;
    }

    _Obstacle->ObstacleRef = 0;
}

void AAINavigationMesh::UpdateObstacle( AAINavMeshObstacle * _Obstacle ) {
    if ( !_Obstacle->ObstacleRef ) {
        GLogger.Printf( "AAINavigationMesh::UpdateObstacle: obstacle is not in navmesh\n" );
        return;
    }

    RemoveObstacle( _Obstacle );
    AddObstacle( _Obstacle );
}

void AAINavigationMesh::Purge() {
    dtFreeNavMeshQuery( NavQuery );
    NavQuery = nullptr;

    dtFreeNavMesh( NavMesh );
    NavMesh = nullptr;

//    dtFreeCrowd( Crowd );
//    Crowd = nullptr;

    dtFreeTileCache( TileCache );
    TileCache = nullptr;

    if ( LinearAllocator ) {
        LinearAllocator->~ADetourLinearAllocator();
        GZoneMemory.Dealloc( LinearAllocator );
        LinearAllocator = nullptr;
    }

    if ( MeshProcess ) {
        MeshProcess->~ADetourMeshProcess();
        GZoneMemory.Dealloc( MeshProcess );
        MeshProcess = nullptr;
    }

    NumTilesX = 0;
    NumTilesZ = 0;
}

struct SDebugDrawCallback : public duDebugDraw {

    ADebugDraw * DD;
    Float3 AccumVertices[3];
    int AccumIndex;
    duDebugDrawPrimitives Primitive;

    SDebugDrawCallback() {
        AccumIndex = 0;
    }

    void depthMask( bool state ) override {
        DD->SetDepthTest( state );
    }

    void texture( bool state ) override {
    }

    void begin( duDebugDrawPrimitives prim, float size = 1.0f ) override {
        Primitive = prim;
        AccumIndex = 0;
    }

    void vertex( const float * pos, unsigned int color ) override {
        vertex( pos[0], pos[1], pos[2], color );
    }

    void vertex( const float x, const float y, const float z, unsigned int color ) override {
        DD->SetColor( color );

        switch ( Primitive ) {
        case DU_DRAW_POINTS:
            DD->DrawPoint( Float3( x,y,z ) );
            break;
        case DU_DRAW_LINES:
            if ( AccumIndex > 0 ) {
                DD->DrawLine( AccumVertices[0], Float3( x,y,z ) );
                AccumIndex = 0;
            } else {
                AccumVertices[ AccumIndex++ ] = Float3( x,y,z );
            }
            break;
        case DU_DRAW_TRIS:
            if ( AccumIndex > 1 ) {
                DD->DrawTriangle( AccumVertices[0], AccumVertices[1], Float3( x,y,z ) );
                AccumIndex = 0;
            } else {
                AccumVertices[ AccumIndex++ ] = Float3( x,y,z );
            }
            break;
        case DU_DRAW_QUADS:
            if ( AccumIndex > 2 ) {
                DD->DrawTriangle( AccumVertices[0], AccumVertices[1], AccumVertices[2] );
                DD->DrawTriangle( AccumVertices[2], Float3( x,y,z ), AccumVertices[0] );
                AccumIndex = 0;
            } else {
                AccumVertices[ AccumIndex++ ] = Float3( x,y,z );
            }
            break;
        }
    }

    void vertex( const float * pos, unsigned int color, const float * uv ) override {
        vertex( pos, color );
    }

    void vertex( const float x, const float y, const float z, unsigned int color, const float u, const float v ) override {
        vertex( x, y, z, color );
    }

    void end() override {
    }
};

void AAINavigationMesh::DrawDebug( ADebugDraw * _DebugDraw ) {

    if ( !NavMesh ) {
        return;
    }

    SDebugDrawCallback callback;
    callback.DD = _DebugDraw;

    if ( RVDrawNavMeshBVTree ) {
        duDebugDrawNavMeshBVTree( &callback, *NavMesh );
    }

    if ( RVDrawNavMeshNodes ) {
        duDebugDrawNavMeshNodes( &callback, *NavQuery );
    }

    if ( RVDrawNavMeshWithClosedList ) {
        duDebugDrawNavMeshWithClosedList( &callback, *NavMesh, *NavQuery, DU_DRAWNAVMESH_OFFMESHCONS|DU_DRAWNAVMESH_CLOSEDLIST|DU_DRAWNAVMESH_COLOR_TILES );
    }
    //duDebugDrawNavMeshPolysWithFlags( &callback, *NavMesh, AI_NAV_MESH_FLAGS_DISABLED/*AI_NAV_MESH_FLAGS_DISABLED*/, duRGBA(0,0,0,128));
    //duDebugDrawNavMeshPolysWithFlags( &callback, *NavMesh, 0/*AI_NAV_MESH_FLAGS_DISABLED*/, duRGBA(0,0,0,128));


#if 0
    if ( RVDrawNavMeshTileBounds && TileCache ) {
        float bmin[3], bmax[3];
        for ( int i = 0 ; i < TileCache->getTileCount() ; ++i ) {
            const dtCompressedTile* tile = TileCache->getTile(i);

            if ( !tile->header ) {
                continue;
            }

            TileCache->calcTightTileBounds( tile->header, bmin, bmax );

            const unsigned int col = duIntToCol( i, 255 );
            const float pad = TileCache->getParams()->cs * 0.1f;

            duDebugDrawBoxWire( &callback, bmin[0]-pad, bmin[1]-pad,bmin[2]-pad,
                bmax[0]+pad, bmax[1]+pad,bmax[2]+pad, col, 2.0f);

        }
    }
#endif

    if ( RVDrawNavMeshTileBounds ) {
        BvAxisAlignedBox boundingBox;
        _DebugDraw->SetDepthTest(false);
        _DebugDraw->SetColor( AColor4( 1,1,1,1 ) );
        for ( int z = 0 ; z < NumTilesZ ; z++ ) {
            for ( int x = 0 ; x < NumTilesX ; x++ ) {
                if ( IsTileExsist( x, z ) ) {
                    GetTileWorldBounds( x, z, boundingBox );

                    _DebugDraw->DrawBox( boundingBox.Center(), boundingBox.HalfSize() );
                }
            }
        }
    }
}

ANavQueryFilter::ANavQueryFilter() {
    Filter = new ( GZoneMemory.Alloc( sizeof( dtQueryFilter ), 1 ) ) dtQueryFilter();
}

ANavQueryFilter::~ANavQueryFilter() {
    Filter->~dtQueryFilter();
    GZoneMemory.Dealloc( Filter );
}

void ANavQueryFilter::SetAreaCost( int _AreaId, float _Cost ) {
    Filter->setAreaCost( _AreaId, _Cost );
}

float ANavQueryFilter::GetAreaCost( int _AreaId ) const {
    return Filter->getAreaCost( _AreaId );
}

void ANavQueryFilter::SetIncludeFlags( unsigned short _Flags ) {
    Filter->setIncludeFlags( _Flags );
}

unsigned short ANavQueryFilter::GetIncludeFlags() const {
    return Filter->getIncludeFlags();
}

void ANavQueryFilter::SetExcludeFlags( unsigned short _Flags ) {
    Filter->setExcludeFlags( _Flags );
}

unsigned short ANavQueryFilter::GetExcludeFlags() const {
    return Filter->getExcludeFlags();
}

bool AAINavigationMesh::Trace( SAINavigationTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, Float3 const & _Extents, ANavQueryFilter const & _Filter ) const {
    SNavPolyRef StartRef;

    if ( !QueryNearestPoly( _RayStart, _Extents, &StartRef ) ) {
        _Result.Clear();
        return false;
    }

    int NumPolys;

    _Result.HitFraction = FLT_MAX;

    NavQuery->raycast( StartRef, (const float *)_RayStart.ToPtr(), (const float *)_RayEnd.ToPtr(), _Filter.Filter, &_Result.HitFraction, (float *)_Result.Normal.ToPtr(), TmpPolys, &NumPolys, MAX_POLYS );

    bool bHasHit = _Result.HitFraction != FLT_MAX;
    if ( !bHasHit ) {
        _Result.Clear();
        return false;
    }

    _Result.Position = _RayStart + ( _RayEnd - _RayStart ) * _Result.HitFraction;
    _Result.Distance = ( _Result.Position - _RayStart ).Length();

    return true;
}

bool AAINavigationMesh::Trace( SAINavigationTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, Float3 const & _Extents ) const {
    return Trace( _Result, _RayStart, _RayEnd, _Extents, QueryFilter );
}

bool AAINavigationMesh::QueryTileLocaction( Float3 const & _Position, int * _TileX, int * _TileY ) const {
    if ( !NavMesh ) {
        *_TileX = *_TileY = 0;
        return false;
    }

    NavMesh->calcTileLoc( (float *)_Position.ToPtr(), _TileX, _TileY );
    return true;
}

bool AAINavigationMesh::QueryNearestPoly( Float3 const & _Position, Float3 const & _Extents, ANavQueryFilter const & _Filter, SNavPolyRef * _NearestPolyRef ) const {
    *_NearestPolyRef = 0;

    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->findNearestPoly( (const float *)_Position.ToPtr(), (const float *)_Extents.ToPtr(), _Filter.Filter, _NearestPolyRef, NULL );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::QueryNearestPoly( Float3 const & _Position, Float3 const & _Extents, SNavPolyRef * _NearestPolyRef ) const {
    return QueryNearestPoly( _Position, _Extents, QueryFilter, _NearestPolyRef );
}

bool AAINavigationMesh::QueryNearestPoint( Float3 const & _Position, Float3 const & _Extents, ANavQueryFilter const & _Filter, SNavPointRef * _NearestPointRef ) const {
    _NearestPointRef->PolyRef = 0;
    _NearestPointRef->Position.Clear();

    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->findNearestPoly( (const float *)_Position.ToPtr(), (const float *)_Extents.ToPtr(), _Filter.Filter, &_NearestPointRef->PolyRef, ( float * )_NearestPointRef->Position.ToPtr() );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::QueryNearestPoint( Float3 const & _Position, Float3 const & _Extents, SNavPointRef * _NearestPointRef ) const {
    return QueryNearestPoint( _Position, _Extents, QueryFilter, _NearestPointRef );
}

bool AAINavigationMesh::QueryRandomPoint( ANavQueryFilter const & _Filter, SNavPointRef * _RandomPointRef ) const {
    _RandomPointRef->PolyRef = 0;
    _RandomPointRef->Position.Clear();

    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->findRandomPoint( _Filter.Filter, Math::Rand1, &_RandomPointRef->PolyRef, ( float * )_RandomPointRef->Position.ToPtr() );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::QueryRandomPoint( SNavPointRef * _RandomPointRef ) const {
    return QueryRandomPoint( QueryFilter, _RandomPointRef );
}

bool AAINavigationMesh::QueryRandomPointAroundCircle( Float3 const & _Position, float _Radius, Float3 const & _Extents, ANavQueryFilter const & _Filter, SNavPointRef * _RandomPointRef ) const {
    SNavPointRef StartRef;

    if ( !QueryNearestPoly( _Position, _Extents, _Filter, &StartRef.PolyRef ) ) {
        return false;
    }

    StartRef.Position = _Position;

    return QueryRandomPointAroundCircle( StartRef, _Radius, _Filter, _RandomPointRef );
}

bool AAINavigationMesh::QueryRandomPointAroundCircle( Float3 const & _Position, float _Radius, Float3 const & _Extents, SNavPointRef * _RandomPointRef ) const {
    return QueryRandomPointAroundCircle( _Position, _Radius, _Extents, QueryFilter, _RandomPointRef );
}

bool AAINavigationMesh::QueryRandomPointAroundCircle( SNavPointRef const & _StartRef, float _Radius, ANavQueryFilter const & _Filter, SNavPointRef * _RandomPointRef ) const {
    _RandomPointRef->PolyRef = 0;
    _RandomPointRef->Position.Clear();

    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->findRandomPointAroundCircle( _StartRef.PolyRef, (const float *)_StartRef.Position.ToPtr(), _Radius, _Filter.Filter, Math::Rand1, &_RandomPointRef->PolyRef, (float *)_RandomPointRef->Position.ToPtr() );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::QueryRandomPointAroundCircle( SNavPointRef const & _StartRef, float _Radius, SNavPointRef * _RandomPointRef ) const {
    return QueryRandomPointAroundCircle( _StartRef, _Radius, QueryFilter, _RandomPointRef );
}

bool AAINavigationMesh::QueryClosestPointOnPoly( SNavPointRef const & _PointRef, Float3 * _Point, bool * _OverPolygon ) const {
    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->closestPointOnPoly( _PointRef.PolyRef, (const float *)_PointRef.Position.ToPtr(), (float *)_Point->ToPtr(), _OverPolygon );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::QueryClosestPointOnPolyBoundary( SNavPointRef const & _PointRef, Float3 * _Point ) const {
    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->closestPointOnPolyBoundary( _PointRef.PolyRef, (const float *)_PointRef.Position.ToPtr(), (float *)_Point->ToPtr() );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::MoveAlongSurface( SNavPointRef const & _StartRef, Float3 const & _Destination, ANavQueryFilter const & _Filter, SNavPolyRef * _Visited, int * _VisitedCount, int _MaxVisitedSize, Float3 & _ResultPos ) const {
    if ( !NavQuery ) {
        return false;
    }

    _MaxVisitedSize = Math::Max( _MaxVisitedSize, 0 );

    dtStatus Status = NavQuery->moveAlongSurface( _StartRef.PolyRef, (const float *)_StartRef.Position.ToPtr(), (const float *)_Destination.ToPtr(), _Filter.Filter, (float *)_ResultPos.ToPtr(), _Visited, _VisitedCount, _MaxVisitedSize );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::MoveAlongSurface( SNavPointRef const & _StartRef, Float3 const & _Destination, SNavPolyRef * _Visited, int * _VisitedCount, int _MaxVisitedSize, Float3 & _ResultPos ) const {
    return MoveAlongSurface( _StartRef, _Destination, QueryFilter, _Visited, _VisitedCount, _MaxVisitedSize, _ResultPos );
}

bool AAINavigationMesh::MoveAlongSurface( Float3 const & _Position, Float3 const & _Destination, Float3 const & _Extents, ANavQueryFilter const & _Filter, int _MaxVisitedSize, Float3 & _ResultPos ) const {
    SNavPointRef StartRef;

    LastVisitedPolys.Clear();

    if ( !QueryNearestPoly( _Position, _Extents, _Filter, &StartRef.PolyRef ) ) {
        return false;
    }

    StartRef.Position = _Position;

    LastVisitedPolys.ResizeInvalidate( Math::Max( _MaxVisitedSize, 0 ) );

    int VisitedCount = 0;

    if ( !MoveAlongSurface( StartRef, _Destination, _Filter, LastVisitedPolys.ToPtr(), &VisitedCount, LastVisitedPolys.Size(), _ResultPos ) ) {
        LastVisitedPolys.Clear();
        return false;
    }

    LastVisitedPolys.Resize( VisitedCount );

    return true;
}

bool AAINavigationMesh::MoveAlongSurface( Float3 const & _Position, Float3 const & _Destination, Float3 const & _Extents, int _MaxVisitedSize, Float3 & _ResultPos ) const {
    return MoveAlongSurface( _Position, _Destination, _Extents, QueryFilter, _MaxVisitedSize, _ResultPos );
}

bool AAINavigationMesh::FindPath( SNavPointRef const & _StartRef, SNavPointRef const & _EndRef, ANavQueryFilter const & _Filter, SNavPolyRef * _Path, int * _PathCount, const int _MaxPath ) const {
    *_PathCount = 0;

    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->findPath( _StartRef.PolyRef, _EndRef.PolyRef, ( const float * )_StartRef.Position.ToPtr(), ( const float * )_EndRef.Position.ToPtr(), _Filter.Filter, _Path, _PathCount, _MaxPath );
    if ( dtStatusFailed( Status ) ) {
        *_PathCount = 0;
        return false;
    }

    return true;
}

bool AAINavigationMesh::FindPath( SNavPointRef const & _StartRef, SNavPointRef const & _EndRef, SNavPolyRef * _Path, int * _PathCount, const int _MaxPath ) const {
    return FindPath( _StartRef, _EndRef, QueryFilter, _Path, _PathCount, _MaxPath );
}

bool AAINavigationMesh::FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, ANavQueryFilter const & _Filter, TPodArray< SAINavigationPathPoint > & _PathPoints ) const {
    SNavPointRef StartRef;
    SNavPointRef EndRef;

    if ( !QueryNearestPoly( _StartPos, _Extents, _Filter, &StartRef.PolyRef ) ) {
        return false;
    }

    if ( !QueryNearestPoly( _EndPos, _Extents, _Filter, &EndRef.PolyRef ) ) {
        return false;
    }

    StartRef.Position = _StartPos;
    EndRef.Position = _EndPos;

    int NumPolys;

    if ( !FindPath( StartRef, EndRef, _Filter, TmpPolys, &NumPolys, MAX_POLYS ) ) {
        return false;
    }

    Float3 ClosestLocalEnd = _EndPos;

    // Если не удалось проложить полный путь, установить конечную точку ближайшую к указанной конечной точке
    if ( TmpPolys[NumPolys - 1] != EndRef.PolyRef ) {
        NavQuery->closestPointOnPoly( TmpPolys[NumPolys - 1], (const float *)_EndPos.ToPtr(), (float *)ClosestLocalEnd.ToPtr(), 0 );
    }

    int PathLength;

    NavQuery->findStraightPath( (const float *)_StartPos.ToPtr(), (const float *)ClosestLocalEnd.ToPtr(), TmpPolys, NumPolys, (float *)TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &PathLength, MAX_POLYS );

    _PathPoints.Resize( PathLength );

    for ( int i = 0 ; i < PathLength ; ++i ) {
        _PathPoints[ i ].Position = TmpPathPoints[i];
        _PathPoints[ i ].Flags = TmpPathFlags[i];
    }

    return true;
}

bool AAINavigationMesh::FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, TPodArray< SAINavigationPathPoint > & _PathPoints ) const {
    return FindPath( _StartPos, _EndPos, _Extents, QueryFilter, _PathPoints );
}

bool AAINavigationMesh::FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, ANavQueryFilter const & _Filter, TPodArray< Float3 > & _PathPoints ) const {
    SNavPointRef StartRef;
    SNavPointRef EndRef;

    if ( !QueryNearestPoly( _StartPos, _Extents, _Filter, &StartRef.PolyRef ) ) {
        return false;
    }

    if ( !QueryNearestPoly( _EndPos, _Extents, _Filter, &EndRef.PolyRef ) ) {
        return false;
    }

    StartRef.Position = _StartPos;
    EndRef.Position = _EndPos;

    int NumPolys;

    if ( !FindPath( StartRef, EndRef, _Filter, TmpPolys, &NumPolys, MAX_POLYS ) ) {
        return false;
    }

    Float3 ClosestLocalEnd = _EndPos;

    // Если не удалось проложить полный путь, установить конечную точку ближайшую к указанной конечной точке
    if ( TmpPolys[NumPolys - 1] != EndRef.PolyRef ) {
        NavQuery->closestPointOnPoly( TmpPolys[NumPolys - 1], (const float *)_EndPos.ToPtr(), (float *)ClosestLocalEnd.ToPtr(), 0 );
    }

    int PathLength;

    NavQuery->findStraightPath( (const float *)_StartPos.ToPtr(), (const float *)ClosestLocalEnd.ToPtr(), TmpPolys, NumPolys, (float *)TmpPathPoints[0].ToPtr(), TmpPathFlags, TmpPathPolys, &PathLength, MAX_POLYS );

    _PathPoints.Resize( PathLength );

    memcpy( _PathPoints.ToPtr(), TmpPathPoints, sizeof( Float3 ) * PathLength );

    return true;
}

bool AAINavigationMesh::FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, TPodArray< Float3 > & _PathPoints ) const {
    return FindPath( _StartPos, _EndPos, _Extents, QueryFilter, _PathPoints );
}

bool AAINavigationMesh::FindStraightPath( Float3 const & _StartPos, Float3 const & _EndPos, SNavPolyRef const * _Path, int _PathSize, Float3 * _StraightPath, unsigned char * _StraightPathFlags, SNavPolyRef * _StraightPathRefs, int * _StraightPathCount, int _MaxStraightPath, EAINavMeshStraightPathCrossing _StraightPathCrossing ) const {
    if ( !NavQuery ) {
        return false;
    }

    dtStatus Status = NavQuery->findStraightPath( (const float *)_StartPos.ToPtr(), (const float *)_EndPos.ToPtr(), _Path, _PathSize, ( float * )_StraightPath, _StraightPathFlags, _StraightPathRefs, _StraightPathCount, _MaxStraightPath, ( int )_StraightPathCrossing );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::CalcDistanceToWall( SNavPointRef const & _StartRef, float _Radius, ANavQueryFilter const & _Filter, SAINavigationHitResult & _HitResult ) const {
    dtStatus Status = NavQuery->findDistanceToWall( _StartRef.PolyRef, (const float *)_StartRef.Position.ToPtr(), _Radius, _Filter.Filter, &_HitResult.Distance, (float *)_HitResult.Position.ToPtr(), (float *)_HitResult.Normal.ToPtr() );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

bool AAINavigationMesh::CalcDistanceToWall( SNavPointRef const & _StartRef, float _Radius, SAINavigationHitResult & _HitResult ) const {
    return CalcDistanceToWall( _StartRef, _Radius, QueryFilter, _HitResult );
}

bool AAINavigationMesh::CalcDistanceToWall( Float3 const & _Position, float _Radius, Float3 const & _Extents, ANavQueryFilter const & _Filter, SAINavigationHitResult & _HitResult ) const {
    SNavPointRef StartRef;

    if ( !QueryNearestPoly( _Position, _Extents, _Filter, &StartRef.PolyRef ) ) {
        return false;
    }

    StartRef.Position = _Position;

    return CalcDistanceToWall( StartRef, _Radius, _Filter, _HitResult );
}

bool AAINavigationMesh::CalcDistanceToWall( Float3 const & _Position, float _Radius, Float3 const & _Extents, SAINavigationHitResult & _HitResult ) const {
    return CalcDistanceToWall( _Position, _Radius, _Extents, QueryFilter, _HitResult );
}

bool AAINavigationMesh::GetHeight( SNavPointRef const & _PointRef, float * _Height ) const {
    if ( !NavQuery ) {
        *_Height = 0;
        return false;
    }

    dtStatus Status = NavQuery->getPolyHeight( _PointRef.PolyRef, (const float *)_PointRef.Position.ToPtr(), _Height );
    if ( dtStatusFailed( Status ) ) {
        *_Height = 0;
        return false;
    }

    return true;
}

bool AAINavigationMesh::GetOffMeshConnectionPolyEndPoints( SNavPolyRef _PrevRef, SNavPolyRef _PolyRef, Float3 * _StartPos, Float3 * _EndPos ) const {
    if ( !NavMesh ) {
        return false;
    }

    dtStatus Status = NavMesh->getOffMeshConnectionPolyEndPoints( _PrevRef, _PolyRef, (float *)_StartPos->ToPtr(), (float *)_EndPos->ToPtr() );
    if ( dtStatusFailed( Status ) ) {
        return false;
    }

    return true;
}

void AAINavigationMesh::Tick( float _TimeStep ) {
    if ( TileCache ) {
        TileCache->update( _TimeStep, NavMesh );
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
int ANavigationMeshComponent::FixupShortcuts( SNavPolyRef * _Path, int _NPath ) const {
    if ( _NPath < 3 ) {
        return _NPath;
    }

    // Get connected polygons
    static const int maxNeis = 16;
    SNavPolyRef neis[maxNeis];
    int nneis = 0;

    const dtMeshTile * tile = 0;
    const dtPoly * poly = 0;
    if ( dtStatusFailed( NavQuery->getAttachedNavMesh()->getTileAndPolyByRef( _Path[0], &tile, &poly ) ) ) {
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

int ANavigationMeshComponent::FixupCorridor( SNavPolyRef * _Path, const int _NPath, const int _MaxPath, const SNavPolyRef * _Visited, const int _NVisited ) const {
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
        memmove(_Path+req, _Path+orig, size*sizeof(SNavPolyRef));

    // Store visited
    for (int i = 0; i < req; ++i)
        _Path[i] = _Visited[(_NVisited-1)-i];

    return req+size;
}
#endif

//    Crowd = dtAllocCrowd();
//    if ( !Crowd ) {
//        Purge();
//        GLogger.Printf( "Failed on dtAllocCrowd\n" );
//        return false;
//    }
