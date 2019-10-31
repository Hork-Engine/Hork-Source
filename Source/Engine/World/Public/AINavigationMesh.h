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

#pragma once

// TODO: dynamic obstacles, areas, area connections, crowd

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Base/Public/DebugDraw.h>

#include <Engine/Core/Public/BitMask.h>

#ifdef DT_POLYREF64
typedef uint64_t FNavPolyRef;
#else
typedef unsigned int FNavPolyRef;
#endif

class FLevel;

struct FNavPointRef {
    FNavPolyRef PolyRef;
    Float3 Position;
};

struct FAINavigationPathPoint {
    Float3 Position;
    int Flags;
};

struct FAINavigationTraceResult {
    Float3 Position;
    Float3 Normal;
    float Distance;
    float HitFraction;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

struct FAINavigationHitResult {
    Float3 Position;
    Float3 Normal;
    float Distance;

    void Clear() {
        memset( this, 0, sizeof( *this ) );
    }
};

enum EAINavMeshPartition {
    AI_NAV_MESH_PARTITION_WATERSHED,    // Best choice if you precompute the navmesh, use this if you have large open areas (default)
    AI_NAV_MESH_PARTITION_MONOTONE,     // Use this if you want fast navmesh generation
    AI_NAV_MESH_PARTITION_LAYERS        // Good choice to use for tiled navmesh with medium and small sized tiles
};

enum EAINavMeshArea {
    AI_NAV_MESH_AREA_WATER  = 0,
    AI_NAV_MESH_AREA_ROAD   = 1,
    AI_NAV_MESH_AREA_DOOR   = 2,
    AI_NAV_MESH_AREA_GRASS  = 3,
    AI_NAV_MESH_AREA_JUMP   = 4,

    // Define own areas AI_NAV_MESH_AREA_<AreaName>

    AI_NAV_MESH_AREA_GROUND = 63,
    AI_NAV_MESH_AREA_MAX = 64               // Max areas. Must match DT_MAX_AREAS
};

enum EAINavMeshAreaFlags {
    AI_NAV_MESH_FLAGS_WALK      = 0x01,     // Ability to walk (ground, grass, road)
    AI_NAV_MESH_FLAGS_SWIM      = 0x02,     // Ability to swim (water)
    AI_NAV_MESH_FLAGS_DOOR      = 0x04,     // Ability to move through doors
    AI_NAV_MESH_FLAGS_JUMP      = 0x08,     // Ability to jump
    AI_NAV_MESH_FLAGS_DISABLED  = 0x10,     // Disabled polygon
    AI_NAV_MESH_FLAGS_ALL       = 0xffff    // All abilities
};

enum EAINavMeshStraightFlags {
    AI_NAV_MESH_STRAIGHTPATH_START = 0x01,              // The vertex is the start position in the path.
    AI_NAV_MESH_STRAIGHTPATH_END   = 0x02,              // The vertex is the end position in the path.
    AI_NAV_MESH_STRAIGHTPATH_OFFMESH_CONNECTION = 0x04  // The vertex is the start of an off-mesh connection.
};

enum EAINavMeshStraightPathCrossing {
    AI_NAV_MESH_STRAIGHTPATH_DEFAULT         = 0,
    AI_NAV_MESH_STRAIGHTPATH_AREA_CROSSINGS  = 0x01, // Add a vertex at every polygon edge crossing where area changes
    AI_NAV_MESH_STRAIGHTPATH_ALL_CROSSINGS   = 0x02, // Add a vertex at every polygon edge crossing
};

struct FAINavMeshConnection {
    /** Connection start position */
    Float3          StartPosition;

    /** Connection end position */
    Float3          EndPosition;

    /** Connection radius */
    float           Radius;

    /** A flag that indicates that an off-mesh connection can be traversed in both directions */
    bool            bBidirectional;

    /** Area id assigned to the connection (see EAINavMeshArea) */
    unsigned char   AreaId;

    /** Flags assigned to the connection */
    unsigned short  Flags;

    void CalcBoundingBox( BvAxisAlignedBox & _BoundingBox ) const {
        _BoundingBox.Mins.X = FMath::Min( StartPosition.X, EndPosition.X );
        _BoundingBox.Mins.Y = FMath::Min( StartPosition.Y, EndPosition.Y );
        _BoundingBox.Mins.Z = FMath::Min( StartPosition.Z, EndPosition.Z );
        _BoundingBox.Maxs.X = FMath::Max( StartPosition.X, EndPosition.X );
        _BoundingBox.Maxs.Y = FMath::Max( StartPosition.Y, EndPosition.Y );
        _BoundingBox.Maxs.Z = FMath::Max( StartPosition.Z, EndPosition.Z );
    }
};

enum EAINavigationAreaShape {
    AI_NAV_MESH_AREA_SHAPE_BOX,
    AI_NAV_MESH_AREA_SHAPE_CONVEX_VOLUME
};

struct FAINavigationArea {
    enum { MAX_VERTS = 32 };

    /** Area ID (see EAINavMeshArea) */
    unsigned char       AreaId;

    /** Area shape */
    EAINavigationAreaShape Shape;

    /** Convex volume definition */
    int                 NumConvexVolumeVerts;
    Float2              ConvexVolume[MAX_VERTS];
    float               ConvexVolumeMinY;
    float               ConvexVolumeMaxY;

    /** Box definition */
    Float3              BoxMins;
    Float3              BoxMaxs;

    void CalcBoundingBoxFromVerts( BvAxisAlignedBox & _BoundingBox ) const {
        if ( !NumConvexVolumeVerts ) {
            _BoundingBox.Mins = _BoundingBox.Maxs = Float3(0.0f);
            return;
        }

        _BoundingBox.Mins[0] = ConvexVolume[0][0];
        _BoundingBox.Mins[2] = ConvexVolume[0][1];
        _BoundingBox.Maxs[0] = ConvexVolume[0][0];
        _BoundingBox.Maxs[2] = ConvexVolume[0][1];
        for ( Float2 const * pVert = &ConvexVolume[1] ; pVert < &ConvexVolume[NumConvexVolumeVerts] ; pVert++ ) {
            _BoundingBox.Mins[0] = FMath::Min(_BoundingBox.Mins[0], pVert->X );
            _BoundingBox.Mins[2] = FMath::Min(_BoundingBox.Mins[2], pVert->Y );
            _BoundingBox.Maxs[0] = FMath::Max(_BoundingBox.Maxs[0], pVert->X );
            _BoundingBox.Maxs[2] = FMath::Max(_BoundingBox.Maxs[2], pVert->Y );
        }
        _BoundingBox.Mins[1] = ConvexVolumeMinY;
        _BoundingBox.Maxs[1] = ConvexVolumeMaxY;
    }

    void CalcBoundingBox( BvAxisAlignedBox & _BoundingBox ) const {
        if ( Shape == AI_NAV_MESH_AREA_SHAPE_BOX ) {
            _BoundingBox.Mins = BoxMins;
            _BoundingBox.Maxs = BoxMaxs;
        } else {
            CalcBoundingBoxFromVerts( _BoundingBox );
        }
    }
};

enum ENavMeshObstacleShape {
    AI_NAV_MESH_OBSTACLE_BOX,
    AI_NAV_MESH_OBSTACLE_CYLINDER
};

class FAINavMeshObstacle {
public:
    ENavMeshObstacleShape Shape;

    Float3 Position;

    /** For box */
    Float3 HalfExtents;

    /** For cylinder */
    float Radius;
    float Height;

    unsigned int ObstacleRef;
};

struct FNavQueryFilter {
    AN_FORBID_COPY( FNavQueryFilter  )

    friend class FAINavigationMesh;

    FNavQueryFilter();
    virtual ~FNavQueryFilter();

    /** Sets the traversal cost of the area */
    void SetAreaCost( int _AreaId, float _Cost );

    /** Returns the traversal cost of the area */
    float GetAreaCost( int _AreaId ) const;

    /** Sets the include flags for the filter. */
    void SetIncludeFlags( unsigned short _Flags );

    /** Returns the include flags for the filter.
    Any polygons that include one or more of these flags will be
    included in the operation. */
    unsigned short GetIncludeFlags() const;

    /** Sets the exclude flags for the filter. */
    void SetExcludeFlags( unsigned short _Flags );

    /** Returns the exclude flags for the filter. */
    unsigned short GetExcludeFlags() const;

private:
    class dtQueryFilter * Filter;
};

struct FAINavMeshInitial {
    //unsigned int NavTrianglesPerChunk = 256;

    /** The walkable height */
    float NavWalkableHeight = 2.0f;

    /** The walkable radius */
    float NavWalkableRadius = 0.6f;

    /** The maximum traversable ledge (Up/Down) */
    float NavWalkableClimb = 0.2f;//0.9f

    /** The maximum slope that is considered walkable. In degrees, ( 0 <= value < 90 ) */
    float NavWalkableSlopeAngle = 45.0f;

    /** The xz-plane cell size to use for fields. (value > 0) */
    float NavCellSize = 0.3f;

    /** The y-axis cell size to use for fields. (value > 0) */
    float NavCellHeight = 0.01f;// 0.2f

    float NavEdgeMaxLength = 12.0f;

    /** The maximum distance a simplfied contour's border edges should deviate
    the original raw contour. (value >= 0) */
    float NavEdgeMaxError = 1.3f;

    float NavMinRegionSize = 8.0f;

    float NavMergeRegionSize = 20.0f;

    float NavDetailSampleDist = 6.0f;

    float NavDetailSampleMaxError = 1.0f;

    /** The maximum number of vertices allowed for polygons generated during the
    contour to polygon conversion process. (value >= 3) */
    int NavVertsPerPoly = 6;

    /** The width/height size of tile's on the xz-plane. (value >= 0) */
    int NavTileSize = 48;

    bool bDynamicNavMesh = true;

    /** Max layers for dynamic navmesh (1..255) */
    int MaxLayers = 16;

    /** Max obstacles for dynamic navmesh */
    int MaxDynamicObstacles = 1024;

    /** Partition for non-tiled nav mesh */
    EAINavMeshPartition RecastPartitionMethod = AI_NAV_MESH_PARTITION_WATERSHED;

    BvAxisAlignedBox BoundingBox;
};

class FAINavigationMesh {
public:

    FAINavigationMesh();
    ~FAINavigationMesh();

    /** Default query filter */
    FNavQueryFilter QueryFilter;

    //
    // Public methods
    //

    /** Initialize empry nav mesh */
    bool Initialize( FLevel * _OwnerLevel, FAINavMeshInitial const & _Initial );

    /** Build all tiles in nav mesh */
    bool Build();

    /** Build tiles in specified range */
    bool Build( Int2 const & _Mins, Int2 const & _Maxs );

    /** Build tiles in specified bounding box */
    bool Build( BvAxisAlignedBox const & _BoundingBox );

    bool IsTileExsist( int _X, int _Z ) const;

    void RemoveTile( int _X, int _Z );

    void RemoveTiles();

    void RemoveTiles( Int2 const & _Mins, Int2 const & _Maxs );

    void AddObstacle( FAINavMeshObstacle * _Obstacle );

    void RemoveObstacle( FAINavMeshObstacle * _Obstacle );

    void UpdateObstacle( FAINavMeshObstacle * _Obstacle );

    /** Purge navigation data */
    void Purge();

    /** NavMesh ticking */
    void Tick( float _TimeStep );

    /** Draw debug info */
    void DrawDebug( FDebugDraw * _DebugDraw );

    /** Casts a 'walkability' ray along the surface of the navigation mesh from
    the start position toward the end position. */
    bool Trace( FAINavigationTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, Float3 const & _Extents, FNavQueryFilter const & _Filter ) const;

    /** Casts a 'walkability' ray along the surface of the navigation mesh from
    the start position toward the end position. */
    bool Trace( FAINavigationTraceResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, Float3 const & _Extents ) const;

    /** Query tile loaction */
    bool QueryTileLocaction( Float3 const & _Position, int * _TileX, int * _TileY ) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoly( Float3 const & _Position, Float3 const & _Extents, FNavQueryFilter const & _Filter, FNavPolyRef * _NearestPolyRef ) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoly( Float3 const & _Position, Float3 const & _Extents, FNavPolyRef * _NearestPolyRef ) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoint( Float3 const & _Position, Float3 const & _Extents, FNavQueryFilter const & _Filter, FNavPointRef * _NearestPointRef ) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoint( Float3 const & _Position, Float3 const & _Extents, FNavPointRef * _NearestPointRef ) const;

    /** Queries random location on navmesh.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon. */
    bool QueryRandomPoint( FNavQueryFilter const & _Filter, FNavPointRef * _RandomPointRef ) const;

    /** Queries random location on navmesh.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon. */
    bool QueryRandomPoint( FNavPointRef * _RandomPointRef ) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle( Float3 const & _Position, float _Radius, Float3 const & _Extents, FNavQueryFilter const & _Filter, FNavPointRef * _RandomPointRef ) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle( Float3 const & _Position, float _Radius, Float3 const & _Extents, FNavPointRef * _RandomPointRef ) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle( FNavPointRef const & _StartRef, float _Radius, FNavQueryFilter const & _Filter, FNavPointRef * _RandomPointRef ) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle( FNavPointRef const & _StartRef, float _Radius, FNavPointRef * _RandomPointRef ) const;

    /** Queries the closest point on the specified polygon. */
    bool QueryClosestPointOnPoly( FNavPointRef const & _PointRef, Float3 * _Point, bool * _OverPolygon = nullptr ) const;

    /** Query a point on the boundary closest to the source point if the source point is outside the
    polygon's xz-bounds. */
    bool QueryClosestPointOnPolyBoundary( FNavPointRef const & _PointRef, Float3 * _Point ) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface( FNavPointRef const & _StartRef, Float3 const & _Destination, FNavQueryFilter const & _Filter, FNavPolyRef * _Visited, int * _VisitedCount, int _MaxVisitedSize, Float3 & _ResultPos ) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface( FNavPointRef const & _StartRef, Float3 const & _Destination, FNavPolyRef * _Visited, int * _VisitedCount, int _MaxVisitedSize, Float3 & _ResultPos ) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface( Float3 const & _Position, Float3 const & _Destination, Float3 const & _Extents, FNavQueryFilter const & _Filter, int _MaxVisitedSize, Float3 & _ResultPos ) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface( Float3 const & _Position, Float3 const & _Destination, Float3 const & _Extents, int _MaxVisitedSize, Float3 & _ResultPos ) const;

    /** Last visited polys from MoveAlongSurface */
    TPodArray< FNavPolyRef > const & GetLastVisitedPolys() const { return LastVisitedPolys; }

    /** Finds a path from the start polygon to the end polygon. */
    bool FindPath( FNavPointRef const & _StartRef, FNavPointRef const & _EndRef, FNavQueryFilter const & _Filter, FNavPolyRef * _Path, int * _PathCount, const int _MaxPath ) const;

    /** Finds a path from the start polygon to the end polygon. */
    bool FindPath( FNavPointRef const & _StartRef, FNavPointRef const & _EndRef, FNavPolyRef * _Path, int * _PathCount, const int _MaxPath ) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, FNavQueryFilter const & _Filter, TPodArray< FAINavigationPathPoint > & _PathPoints ) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, TPodArray< FAINavigationPathPoint > & _PathPoints ) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, FNavQueryFilter const & _Filter, TPodArray< Float3 > & _PathPoints ) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath( Float3 const & _StartPos, Float3 const & _EndPos, Float3 const & _Extents, TPodArray< Float3 > & _PathPoints ) const;

    /** Finds the straight path from the start to the end position within the polygon corridor. */
    bool FindStraightPath( Float3 const & _StartPos, Float3 const & _EndPos, FNavPolyRef const * _Path, int _PathSize, Float3 * _StraightPath, unsigned char * _StraightPathFlags, FNavPolyRef * _StraightPathRefs, int * _StraightPathCount, int _MaxStraightPath, EAINavMeshStraightPathCrossing _StraightPathCrossing = AI_NAV_MESH_STRAIGHTPATH_DEFAULT ) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall( FNavPointRef const & _StartRef, float _Radius, FNavQueryFilter const & _Filter, FAINavigationHitResult & _HitResult ) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall( FNavPointRef const & _StartRef, float _Radius, FAINavigationHitResult & _HitResult ) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall( Float3 const & _Position, float _Radius, Float3 const & _Extents, FNavQueryFilter const & _Filter, FAINavigationHitResult & _HitResult ) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall( Float3 const & _Position, float _Radius, Float3 const & _Extents, FAINavigationHitResult & _HitResult ) const;

    /** Gets the height of the polygon at the provided position using the height detail. */
    bool GetHeight( FNavPointRef const & _PointRef, float * _Height ) const;

    /** Gets the endpoints for an off-mesh connection, ordered by "direction of travel". */
    bool GetOffMeshConnectionPolyEndPoints( FNavPolyRef _PrevRef, FNavPolyRef _PolyRef, Float3 * _StartPos, Float3 * _EndPos ) const;

    /** Navmesh tile bounding box in world space */
    void GetTileWorldBounds( int _X, int _Z, BvAxisAlignedBox & _BoundingBox ) const;

    /** Navmesh bounding box */
    BvAxisAlignedBox const & GetWorldBounds() const { return BoundingBox; }

    int GetTileCountX() const { return NumTilesX; }
    int GetTileCountZ() const { return NumTilesZ; }

private:

    bool BuildTiles( Int2 const & _Mins, Int2 const & _Maxs );
    bool BuildTile( int _X, int _Z );

    FAINavMeshInitial Initial;

    int NumTilesX;
    int NumTilesZ;
    float TileWidth;
    BvAxisAlignedBox BoundingBox;

    // Detour data
    class dtNavMesh * NavMesh;
    class dtNavMeshQuery * NavQuery;
    //class dtCrowd * Crowd;
    class dtTileCache * TileCache;

    // For tile cache
    struct FDetourLinearAllocator * LinearAllocator;
    struct FDetourMeshProcess * MeshProcess;

    FLevel * OwnerLevel;

    // NavMesh areas
    //TPodArray< FAINavigationArea > Areas;

    // Temp array to reduce memory allocations during MoveAlongSurface
    mutable TPodArray< FNavPolyRef > LastVisitedPolys;
};
