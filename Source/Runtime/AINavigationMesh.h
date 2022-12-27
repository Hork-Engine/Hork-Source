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

#pragma once

// TODO: dynamic obstacles, areas, area connections, crowd

#include "BaseObject.h"
#include "DebugRenderer.h"

#include <Core/IntrusiveLinkedListMacro.h>
#include <Containers/BitMask.h>

class dtNavMesh;
class dtNavMeshQuery;
class dtTileCache;

HK_NAMESPACE_BEGIN

#ifdef DT_POLYREF64
typedef uint64_t NavPolyRef;
#else
typedef unsigned int NavPolyRef;
#endif

struct NavPointRef
{
    NavPolyRef PolyRef;
    Float3      Position;
};

struct AINavigationPathPoint
{
    Float3 Position;
    int    Flags;
};

struct AINavigationTraceResult
{
    Float3 Position;
    Float3 Normal;
    float  Distance;
    float  HitFraction;

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }
};

struct AINavigationHitResult
{
    Float3 Position;
    Float3 Normal;
    float  Distance;

    void Clear()
    {
        Platform::ZeroMem(this, sizeof(*this));
    }
};

struct NavigationGeometry
{
    TVector<Float3>       Vertices;
    TVector<unsigned int> Indices;
    BvAxisAlignedBox             BoundingBox;
    TBitMask<>                   WalkableMask;
    BvAxisAlignedBox const*      pClipBoundingBox;
};

enum AI_NAV_MESH_PARTITION
{
    /** Best choice if you precompute the navmesh, use this if you have large open areas (default) */
    AI_NAV_MESH_PARTITION_WATERSHED,
    /** Use this if you want fast navmesh generation */
    AI_NAV_MESH_PARTITION_MONOTONE,
    /** Good choice to use for tiled navmesh with medium and small sized tiles */
    AI_NAV_MESH_PARTITION_LAYERS
};

enum AI_NAV_MESH_AREA
{
    AI_NAV_MESH_AREA_WATER = 0,
    AI_NAV_MESH_AREA_ROAD  = 1,
    AI_NAV_MESH_AREA_DOOR  = 2,
    AI_NAV_MESH_AREA_GRASS = 3,
    AI_NAV_MESH_AREA_JUMP  = 4,

    // Define own areas AI_NAV_MESH_AREA_<AreaName>

    AI_NAV_MESH_AREA_GROUND = 63,

    /** Max areas. Must match DT_MAX_AREAS */
    AI_NAV_MESH_AREA_MAX = 64
};

enum AI_NAV_MESH_FLAG
{
    /** Ability to walk (ground, grass, road) */
    AI_NAV_MESH_FLAGS_WALK = 0x01,
    /** Ability to swim (water) */
    AI_NAV_MESH_FLAGS_SWIM = 0x02,
    /** Ability to move through doors */
    AI_NAV_MESH_FLAGS_DOOR = 0x04,
    /** Ability to jump */
    AI_NAV_MESH_FLAGS_JUMP = 0x08,
    /** Disabled polygon */
    AI_NAV_MESH_FLAGS_DISABLED = 0x10,
    /** All abilities */
    AI_NAV_MESH_FLAGS_ALL = 0xffff
};

enum AI_NAV_MESH_STRAIGHTPATH
{
    /** The vertex is the start position in the path. */
    AI_NAV_MESH_STRAIGHTPATH_START = 0x01,
    /** The vertex is the end position in the path. */
    AI_NAV_MESH_STRAIGHTPATH_END = 0x02,
    /** The vertex is the start of an off-mesh connection. */
    AI_NAV_MESH_STRAIGHTPATH_OFFMESH_CONNECTION = 0x04
};

enum AI_NAV_MESH_STRAIGHTPATH_CROSSING
{
    AI_NAV_MESH_STRAIGHTPATH_DEFAULT = 0,
    /** Add a vertex at every polygon edge crossing where area changes */
    AI_NAV_MESH_STRAIGHTPATH_AREA_CROSSINGS = 0x01,
    /** Add a vertex at every polygon edge crossing */
    AI_NAV_MESH_STRAIGHTPATH_ALL_CROSSINGS = 0x02,
};

struct AINavMeshConnection
{
    /** Connection start position */
    Float3 StartPosition;

    /** Connection end position */
    Float3 EndPosition;

    /** Connection radius */
    float Radius;

    /** A flag that indicates that an off-mesh connection can be traversed in both directions */
    bool bBidirectional;

    /** Area id assigned to the connection (see AI_NAV_MESH_AREA) */
    unsigned char AreaId;

    /** Flags assigned to the connection */
    unsigned short Flags;

    void CalcBoundingBox(BvAxisAlignedBox& _BoundingBox) const
    {
        _BoundingBox.Mins.X = Math::Min(StartPosition.X, EndPosition.X);
        _BoundingBox.Mins.Y = Math::Min(StartPosition.Y, EndPosition.Y);
        _BoundingBox.Mins.Z = Math::Min(StartPosition.Z, EndPosition.Z);
        _BoundingBox.Maxs.X = Math::Max(StartPosition.X, EndPosition.X);
        _BoundingBox.Maxs.Y = Math::Max(StartPosition.Y, EndPosition.Y);
        _BoundingBox.Maxs.Z = Math::Max(StartPosition.Z, EndPosition.Z);
    }
};

enum AI_NAV_MESH_AREA_SHAPE
{
    AI_NAV_MESH_AREA_SHAPE_BOX,
    AI_NAV_MESH_AREA_SHAPE_CONVEX_VOLUME
};

struct AINavigationArea
{
    enum
    {
        MAX_VERTS = 32
    };

    /** Area ID (see AI_NAV_MESH_AREA) */
    unsigned char AreaId;

    /** Area shape */
    AI_NAV_MESH_AREA_SHAPE Shape;

    /** Convex volume definition */
    int    NumConvexVolumeVerts;
    Float2 ConvexVolume[MAX_VERTS];
    float  ConvexVolumeMinY;
    float  ConvexVolumeMaxY;

    /** Box definition */
    Float3 BoxMins;
    Float3 BoxMaxs;

    void CalcBoundingBoxFromVerts(BvAxisAlignedBox& _BoundingBox) const
    {
        if (!NumConvexVolumeVerts)
        {
            _BoundingBox.Mins = _BoundingBox.Maxs = Float3(0.0f);
            return;
        }

        _BoundingBox.Mins[0] = ConvexVolume[0][0];
        _BoundingBox.Mins[2] = ConvexVolume[0][1];
        _BoundingBox.Maxs[0] = ConvexVolume[0][0];
        _BoundingBox.Maxs[2] = ConvexVolume[0][1];
        for (Float2 const* pVert = &ConvexVolume[1]; pVert < &ConvexVolume[NumConvexVolumeVerts]; pVert++)
        {
            _BoundingBox.Mins[0] = Math::Min(_BoundingBox.Mins[0], pVert->X);
            _BoundingBox.Mins[2] = Math::Min(_BoundingBox.Mins[2], pVert->Y);
            _BoundingBox.Maxs[0] = Math::Max(_BoundingBox.Maxs[0], pVert->X);
            _BoundingBox.Maxs[2] = Math::Max(_BoundingBox.Maxs[2], pVert->Y);
        }
        _BoundingBox.Mins[1] = ConvexVolumeMinY;
        _BoundingBox.Maxs[1] = ConvexVolumeMaxY;
    }

    void CalcBoundingBox(BvAxisAlignedBox& _BoundingBox) const
    {
        if (Shape == AI_NAV_MESH_AREA_SHAPE_BOX)
        {
            _BoundingBox.Mins = BoxMins;
            _BoundingBox.Maxs = BoxMaxs;
        }
        else
        {
            CalcBoundingBoxFromVerts(_BoundingBox);
        }
    }
};

enum AI_NAV_MESH_OBSTACLE
{
    AI_NAV_MESH_OBSTACLE_BOX,
    AI_NAV_MESH_OBSTACLE_CYLINDER
};

class AINavMeshObstacle
{
public:
    AI_NAV_MESH_OBSTACLE Shape;

    Float3 Position;

    /** For box */
    Float3 HalfExtents;

    /** For cylinder */
    float Radius;
    float Height;

    unsigned int ObstacleRef;
};

struct NavQueryFilter
{
    HK_FORBID_COPY(NavQueryFilter)

    friend class AINavigationMesh;

    NavQueryFilter();
    virtual ~NavQueryFilter();

    /** Sets the traversal cost of the area */
    void SetAreaCost(int _AreaId, float _Cost);

    /** Returns the traversal cost of the area */
    float GetAreaCost(int _AreaId) const;

    /** Sets the include flags for the filter. */
    void SetIncludeFlags(unsigned short _Flags);

    /** Returns the include flags for the filter.
    Any polygons that include one or more of these flags will be
    included in the operation. */
    unsigned short GetIncludeFlags() const;

    /** Sets the exclude flags for the filter. */
    void SetExcludeFlags(unsigned short _Flags);

    /** Returns the exclude flags for the filter. */
    unsigned short GetExcludeFlags() const;

private:
    TUniqueRef<class NavQueryFilterPrivate> Filter;
};

struct AINavigationConfig
{
    //unsigned int NavTrianglesPerChunk = 256;

    /** The walkable height */
    float WalkableHeight = 2.0f;

    /** The walkable radius */
    float WalkableRadius = 0.6f;

    /** The maximum traversable ledge (Up/Down) */
    float WalkableClimb = 0.2f; //0.9f

    /** The maximum slope that is considered walkable. In degrees, ( 0 <= value < 90 ) */
    float WalkableSlopeAngle = 45.0f;

    /** The xz-plane cell size to use for fields. (value > 0) */
    float CellSize = 0.3f;

    /** The y-axis cell size to use for fields. (value > 0) */
    float CellHeight = 0.01f; // 0.2f

    float EdgeMaxLength = 12.0f;

    /** The maximum distance a simplfied contour's border edges should deviate
    the original raw contour. (value >= 0) */
    float EdgeMaxError = 1.3f;

    float MinRegionSize = 8.0f;

    float MergeRegionSize = 20.0f;

    float DetailSampleDist = 6.0f;

    float DetailSampleMaxError = 1.0f;

    /** The maximum number of vertices allowed for polygons generated during the
    contour to polygon conversion process. (value >= 3) */
    int VertsPerPoly = 6;

    /** The width/height size of tile's on the xz-plane. (value >= 0) */
    int TileSize = 48;

    bool bDynamicNavMesh = true;

    /** Max layers for dynamic navmesh (1..255) */
    int MaxLayers = 16;

    /** Max obstacles for dynamic navmesh */
    int MaxDynamicObstacles = 1024;

    /** Partition for non-tiled nav mesh */
    AI_NAV_MESH_PARTITION RecastPartitionMethod = AI_NAV_MESH_PARTITION_WATERSHED;

    BvAxisAlignedBox BoundingBox = BvAxisAlignedBox::Empty();
};

class NavigationPrimitive
{
public:
    TLink<NavigationPrimitive> Link;

    virtual void GatherNavigationGeometry(NavigationGeometry& Geometry) const = 0;
};

class AINavigationMesh
{
    HK_FORBID_COPY(AINavigationMesh)

public:
    /** Default query filter */
    NavQueryFilter QueryFilter;

    /** Navigation mesh connections. You must rebuild navigation mesh if you change connections. */
    TVector<AINavMeshConnection> NavMeshConnections; // TODO: Components?

    /** Navigation areas. You must rebuild navigation mesh if you change areas. */
    TVector<AINavigationArea> NavigationAreas; // TODO: Components?

    TList<NavigationPrimitive> NavigationPrimitives;

    //
    // Public methods
    //

    AINavigationMesh();
    virtual ~AINavigationMesh();

    /** Initialize empty nav mesh. You must rebuild nav mesh after that. */
    bool Initialize(AINavigationConfig const& _NavigationConfig);

    /** Build all tiles in nav mesh */
    bool Build();

    /** Build tiles in specified range */
    bool Build(Int2 const& _Mins, Int2 const& _Maxs);

    /** Build tiles in specified bounding box */
    bool Build(BvAxisAlignedBox const& _BoundingBox);

    bool IsTileExsist(int _X, int _Z) const;

    void RemoveTile(int _X, int _Z);

    void RemoveTiles();

    void RemoveTiles(Int2 const& _Mins, Int2 const& _Maxs);

    void AddObstacle(AINavMeshObstacle* _Obstacle);

    void RemoveObstacle(AINavMeshObstacle* _Obstacle);

    void UpdateObstacle(AINavMeshObstacle* _Obstacle);

    /** Purge navigation data */
    void Purge();

    /** NavMesh ticking */
    void Update(float _TimeStep);

    /** Draw debug info */
    void DrawDebug(DebugRenderer* InRenderer);

    /** Casts a 'walkability' ray along the surface of the navigation mesh from
    the start position toward the end position. */
    bool Trace(AINavigationTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, Float3 const& _Extents, NavQueryFilter const& _Filter) const;

    /** Casts a 'walkability' ray along the surface of the navigation mesh from
    the start position toward the end position. */
    bool Trace(AINavigationTraceResult& _Result, Float3 const& _RayStart, Float3 const& _RayEnd, Float3 const& _Extents) const;

    /** Query tile loaction */
    bool QueryTileLocaction(Float3 const& _Position, int* _TileX, int* _TileY) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoly(Float3 const& _Position, Float3 const& _Extents, NavQueryFilter const& _Filter, NavPolyRef* _NearestPolyRef) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoly(Float3 const& _Position, Float3 const& _Extents, NavPolyRef* _NearestPolyRef) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoint(Float3 const& _Position, Float3 const& _Extents, NavQueryFilter const& _Filter, NavPointRef* _NearestPointRef) const;

    /** Queries the polygon nearest to the specified position.
    Extents is the search distance along each axis */
    bool QueryNearestPoint(Float3 const& _Position, Float3 const& _Extents, NavPointRef* _NearestPointRef) const;

    /** Queries random location on navmesh.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon. */
    bool QueryRandomPoint(NavQueryFilter const& _Filter, NavPointRef* _RandomPointRef) const;

    /** Queries random location on navmesh.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon. */
    bool QueryRandomPoint(NavPointRef* _RandomPointRef) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle(Float3 const& _Position, float _Radius, Float3 const& _Extents, NavQueryFilter const& _Filter, NavPointRef* _RandomPointRef) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle(Float3 const& _Position, float _Radius, Float3 const& _Extents, NavPointRef* _RandomPointRef) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle(NavPointRef const& _StartRef, float _Radius, NavQueryFilter const& _Filter, NavPointRef* _RandomPointRef) const;

    /** Queries random location on navmesh within the reach of specified location.
    Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    The location is not exactly constrained by the circle, but it limits the visited polygons. */
    bool QueryRandomPointAroundCircle(NavPointRef const& _StartRef, float _Radius, NavPointRef* _RandomPointRef) const;

    /** Queries the closest point on the specified polygon. */
    bool QueryClosestPointOnPoly(NavPointRef const& _PointRef, Float3* _Point, bool* _OverPolygon = nullptr) const;

    /** Query a point on the boundary closest to the source point if the source point is outside the
    polygon's xz-bounds. */
    bool QueryClosestPointOnPolyBoundary(NavPointRef const& _PointRef, Float3* _Point) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface(NavPointRef const& _StartRef, Float3 const& _Destination, NavQueryFilter const& _Filter, NavPolyRef* _Visited, int* _VisitedCount, int _MaxVisitedSize, Float3& _ResultPos) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface(NavPointRef const& _StartRef, Float3 const& _Destination, NavPolyRef* _Visited, int* _VisitedCount, int _MaxVisitedSize, Float3& _ResultPos) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface(Float3 const& _Position, Float3 const& _Destination, Float3 const& _Extents, NavQueryFilter const& _Filter, int _MaxVisitedSize, Float3& _ResultPos) const;

    /** Moves from the start to the end position constrained to the navigation mesh */
    bool MoveAlongSurface(Float3 const& _Position, Float3 const& _Destination, Float3 const& _Extents, int _MaxVisitedSize, Float3& _ResultPos) const;

    /** Last visited polys from MoveAlongSurface */
    TPodVector<NavPolyRef> const& GetLastVisitedPolys() const { return m_LastVisitedPolys; }

    /** Finds a path from the start polygon to the end polygon. */
    bool FindPath(NavPointRef const& _StartRef, NavPointRef const& _EndRef, NavQueryFilter const& _Filter, NavPolyRef* _Path, int* _PathCount, const int _MaxPath) const;

    /** Finds a path from the start polygon to the end polygon. */
    bool FindPath(NavPointRef const& _StartRef, NavPointRef const& _EndRef, NavPolyRef* _Path, int* _PathCount, const int _MaxPath) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, NavQueryFilter const& _Filter, TPodVector<AINavigationPathPoint>& _PathPoints) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, TPodVector<AINavigationPathPoint>& _PathPoints) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, NavQueryFilter const& _Filter, TPodVector<Float3>& _PathPoints) const;

    /** Finds a path from the start position to the end position. */
    bool FindPath(Float3 const& _StartPos, Float3 const& _EndPos, Float3 const& _Extents, TPodVector<Float3>& _PathPoints) const;

    /** Finds the straight path from the start to the end position within the polygon corridor. */
    bool FindStraightPath(Float3 const& _StartPos, Float3 const& _EndPos, NavPolyRef const* _Path, int _PathSize, Float3* _StraightPath, unsigned char* _StraightPathFlags, NavPolyRef* _StraightPathRefs, int* _StraightPathCount, int _MaxStraightPath, AI_NAV_MESH_STRAIGHTPATH_CROSSING _StraightPathCrossing = AI_NAV_MESH_STRAIGHTPATH_DEFAULT) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall(NavPointRef const& _StartRef, float _Radius, NavQueryFilter const& _Filter, AINavigationHitResult& _HitResult) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall(NavPointRef const& _StartRef, float _Radius, AINavigationHitResult& _HitResult) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall(Float3 const& _Position, float _Radius, Float3 const& _Extents, NavQueryFilter const& _Filter, AINavigationHitResult& _HitResult) const;

    /** Calculates the distance from the specified position to the nearest polygon wall. */
    bool CalcDistanceToWall(Float3 const& _Position, float _Radius, Float3 const& _Extents, AINavigationHitResult& _HitResult) const;

    /** Gets the height of the polygon at the provided position using the height detail. */
    bool GetHeight(NavPointRef const& _PointRef, float* _Height) const;

    /** Gets the endpoints for an off-mesh connection, ordered by "direction of travel". */
    bool GetOffMeshConnectionPolyEndPoints(NavPolyRef _PrevRef, NavPolyRef _PolyRef, Float3* _StartPos, Float3* _EndPos) const;

    /** Navmesh tile bounding box in world space */
    void GetTileWorldBounds(int _X, int _Z, BvAxisAlignedBox& _BoundingBox) const;

    /** Navmesh bounding box */
    BvAxisAlignedBox const& GetWorldBounds() const { return m_BoundingBox; }

    int GetTileCountX() const { return m_NumTilesX; }
    int GetTileCountZ() const { return m_NumTilesZ; }

    void GatherNavigationGeometry(NavigationGeometry& Geometry);

private:
    bool BuildTiles(Int2 const& _Mins, Int2 const& _Maxs);
    bool BuildTile(int _X, int _Z);

    AINavigationConfig Initial;

    int              m_NumTilesX{};
    int              m_NumTilesZ{};
    float            m_TileWidth{1.0f};
    BvAxisAlignedBox m_BoundingBox;

    // Detour data
    dtNavMesh*      m_NavMesh{};
    dtNavMeshQuery* m_NavQuery{};
    //dtCrowd * m_Crowd{};
    dtTileCache* m_TileCache{};

    // For tile cache
    TUniqueRef<struct DetourLinearAllocator> m_LinearAllocator;
    TUniqueRef<struct DetourMeshProcess>     m_MeshProcess;

    // NavMesh areas
    //TVector<AINavigationArea> m_Areas;

    // Temp array to reduce memory allocations during MoveAlongSurface
    mutable TPodVector<NavPolyRef> m_LastVisitedPolys;
};

HK_NAMESPACE_END
