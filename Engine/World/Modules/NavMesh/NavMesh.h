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

#pragma once

// TODO: dynamic obstacles, areas, area connections, crowd

#include <Engine/Core/Containers/BitMask.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/World/DebugRenderer.h>

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
    Float3     Position;
};

struct NavMeshPathPoint
{
    Float3 Position;
    int    Flags;
};

struct NavMeshRayCastResult
{
    float  Fraction{};
    Float3 Normal;
};

struct NavMeshHitResult
{
    Float3 Position;
    Float3 Normal;
    float  Distance;

    void Clear()
    {
        Core::ZeroMem(this, sizeof(*this));
    }
};

struct NavigationGeometry
{
    TVector<Float3> Vertices;
    TVector<unsigned int> Indices;
    BvAxisAlignedBox BoundingBox;
    TBitMask<> WalkableMask;
    BvAxisAlignedBox const* pClipBoundingBox;
};

enum NAV_MESH_PARTITION
{
    /// Best choice if you precompute the navmesh, use this if you have large open areas (default)
    NAV_MESH_PARTITION_WATERSHED,
    /// Use this if you want fast navmesh generation
    NAV_MESH_PARTITION_MONOTONE,
    /// Good choice to use for tiled navmesh with medium and small sized tiles
    NAV_MESH_PARTITION_LAYERS
};

enum NAV_MESH_AREA
{
    NAV_MESH_AREA_WATER = 0,
    NAV_MESH_AREA_ROAD  = 1,
    NAV_MESH_AREA_DOOR  = 2,
    NAV_MESH_AREA_GRASS = 3,
    NAV_MESH_AREA_JUMP  = 4,

    // Define own areas NAV_MESH_AREA_<AreaName>

    NAV_MESH_AREA_GROUND = 63,

    /// Max areas. Must match DT_MAX_AREAS
    NAV_MESH_AREA_MAX = 64
};

enum NAV_MESH_FLAG
{
    /// Ability to walk (ground, grass, road)
    NAV_MESH_FLAGS_WALK = 0x01,
    /// Ability to swim (water)
    NAV_MESH_FLAGS_SWIM = 0x02,
    /// Ability to move through doors
    NAV_MESH_FLAGS_DOOR = 0x04,
    /// Ability to jump
    NAV_MESH_FLAGS_JUMP = 0x08,
    /// Disabled polygon
    NAV_MESH_FLAGS_DISABLED = 0x10,
    /// All abilities
    NAV_MESH_FLAGS_ALL = 0xffff
};

enum NAV_MESH_STRAIGHTPATH
{
    /// The vertex is the start position in the path.
    NAV_MESH_STRAIGHTPATH_START = 0x01,
    /// The vertex is the end position in the path.
    NAV_MESH_STRAIGHTPATH_END = 0x02,
    /// The vertex is the start of an off-mesh connection.
    NAV_MESH_STRAIGHTPATH_OFFMESH_CONNECTION = 0x04
};

enum NAV_MESH_STRAIGHTPATH_CROSSING
{
    NAV_MESH_STRAIGHTPATH_DEFAULT = 0,
    /// Add a vertex at every polygon edge crossing where area changes
    NAV_MESH_STRAIGHTPATH_AREA_CROSSINGS = 0x01,
    /// Add a vertex at every polygon edge crossing
    NAV_MESH_STRAIGHTPATH_ALL_CROSSINGS = 0x02,
};

struct NavMeshConnection
{
    /// Connection start position
    Float3 StartPosition;

    /// Connection end position
    Float3 EndPosition;

    /// Connection radius
    float Radius;

    /// A flag that indicates that an off-mesh connection can be traversed in both directions
    bool bBidirectional;

    /// Area id assigned to the connection (see NAV_MESH_AREA)
    unsigned char AreaID;

    /// Flags assigned to the connection
    unsigned short Flags;

    HK_INLINE BvAxisAlignedBox CalcBoundingBox() const
    {
        return {
            {Math::Min(StartPosition.X, EndPosition.X), Math::Min(StartPosition.Y, EndPosition.Y), Math::Min(StartPosition.Z, EndPosition.Z)},
            {Math::Max(StartPosition.X, EndPosition.X), Math::Max(StartPosition.Y, EndPosition.Y), Math::Max(StartPosition.Z, EndPosition.Z)}};
    }
};

enum NAV_MESH_AREA_SHAPE
{
    NAV_MESH_AREA_SHAPE_BOX,
    NAV_MESH_AREA_SHAPE_CONVEX_VOLUME
};

struct NavMeshArea
{
    enum
    {
        MAX_VERTS = 32
    };

    /// Area ID (see NAV_MESH_AREA)
    unsigned char AreaID;

    /// Area shape
    NAV_MESH_AREA_SHAPE Shape;

    /// Convex volume definition
    int    NumConvexVolumeVerts;
    Float2 ConvexVolume[MAX_VERTS];
    float  ConvexVolumeMinY;
    float  ConvexVolumeMaxY;

    /// Box definition
    Float3 BoxMins;
    Float3 BoxMaxs;

    BvAxisAlignedBox CalcBoundingBoxFromVerts() const
    {
        if (!NumConvexVolumeVerts)
        {
            return {
                {0, 0, 0},
                {0, 0, 0}};
        }

        BvAxisAlignedBox bounding_box;
        bounding_box.Mins[0] = ConvexVolume[0][0];
        bounding_box.Mins[2] = ConvexVolume[0][1];
        bounding_box.Maxs[0] = ConvexVolume[0][0];
        bounding_box.Maxs[2] = ConvexVolume[0][1];
        for (Float2 const* pVert = &ConvexVolume[1]; pVert < &ConvexVolume[NumConvexVolumeVerts]; pVert++)
        {
            bounding_box.Mins[0] = Math::Min(bounding_box.Mins[0], pVert->X);
            bounding_box.Mins[2] = Math::Min(bounding_box.Mins[2], pVert->Y);
            bounding_box.Maxs[0] = Math::Max(bounding_box.Maxs[0], pVert->X);
            bounding_box.Maxs[2] = Math::Max(bounding_box.Maxs[2], pVert->Y);
        }
        bounding_box.Mins[1] = ConvexVolumeMinY;
        bounding_box.Maxs[1] = ConvexVolumeMaxY;
        return bounding_box;
    }

    BvAxisAlignedBox CalcBoundingBox() const
    {
        switch (Shape)
        {
        case NAV_MESH_AREA_SHAPE_BOX:
            return {BoxMins, BoxMaxs};
        case NAV_MESH_AREA_SHAPE_CONVEX_VOLUME:
            return CalcBoundingBoxFromVerts();
        }
        return {};
    }
};

enum NAV_MESH_OBSTACLE
{
    NAV_MESH_OBSTACLE_BOX,
    NAV_MESH_OBSTACLE_CYLINDER
};

class NavMeshObstacle
{
public:
    NAV_MESH_OBSTACLE Shape;

    Float3 Position;

    /// For box
    Float3 HalfExtents;

    /// For cylinder
    float Radius;
    float Height;

    unsigned int ObstacleRef;
};

struct NavQueryFilter
{
    HK_FORBID_COPY(NavQueryFilter)

    friend class NavMesh;

    NavQueryFilter();
    virtual ~NavQueryFilter();

    /// Sets the traversal cost of the area.
    void SetAreaCost(int inAreaID, float inCost);

    /// Returns the traversal cost of the area.
    float GetAreaCost(int inAreaID) const;

    /// Sets the include flags for the filter.
    void SetIncludeFlags(unsigned short inFlags);

    /// Returns the include flags for the filter. Any polygons that include one or more of these flags will be
    /// included in the operation.
    unsigned short GetIncludeFlags() const;

    /// Sets the exclude flags for the filter.
    void SetExcludeFlags(unsigned short inFlags);

    /// Returns the exclude flags for the filter.
    unsigned short GetExcludeFlags() const;

private:
    TUniqueRef<class NavQueryFilterPrivate> m_Filter;
};

struct NavMeshDesc
{
    //unsigned int NavTrianglesPerChunk = 256;

    /// The walkable height
    float WalkableHeight = 2.0f;

    /// The walkable radius
    float WalkableRadius = 0.6f;

    /// The maximum traversable ledge (Up/Down)
    float WalkableClimb = 0.2f; //0.9f

    /// The maximum slope that is considered walkable. In degrees, ( 0 <= value < 90 )
    float WalkableSlopeAngle = 45.0f;

    /// The xz-plane cell size to use for fields. (value > 0)
    float CellSize = 0.3f;

    /// The y-axis cell size to use for fields. (value > 0)
    float CellHeight = 0.01f; // 0.2f

    float EdgeMaxLength = 12.0f;

    /// The maximum distance a simplfied contour's border edges should deviate
    /// the original raw contour. (value >= 0)
    float EdgeMaxError = 1.3f;

    float MinRegionSize = 8.0f;

    float MergeRegionSize = 20.0f;

    float DetailSampleDist = 6.0f;

    float DetailSampleMaxError = 1.0f;

    /// The maximum number of vertices allowed for polygons generated during the
    /// contour to polygon conversion process. (value >= 3)
    int VertsPerPoly = 6;

    /// The width/height size of tile's on the xz-plane. (value >= 0)
    int TileSize = 48;

    bool bDynamicNavMesh = true;

    /// Max layers for dynamic navmesh (1..255)
    int MaxLayers = 16;

    /// Max obstacles for dynamic navmesh
    int MaxDynamicObstacles = 1024;

    /// Partition for non-tiled nav mesh
    NAV_MESH_PARTITION RecastPartitionMethod = NAV_MESH_PARTITION_WATERSHED;

    BvAxisAlignedBox BoundingBox = BvAxisAlignedBox::Empty();
};

class NavigationPrimitive
{
public:
    TLink<NavigationPrimitive> Link;

    virtual void GatherNavigationGeometry(NavigationGeometry& Geometry) const = 0;
};

class NavMesh
{
    HK_FORBID_COPY(NavMesh)

public:
    /// Default query filter
    NavQueryFilter QueryFilter;

    /// Navigation mesh connections. You must rebuild navigation mesh if you change connections.
    TVector<NavMeshConnection> NavMeshConnections; // TODO: Components?

    /// Navigation areas. You must rebuild navigation mesh if you change areas.
    TVector<NavMeshArea> NavigationAreas; // TODO: Components?

    TList<NavigationPrimitive> NavigationPrimitives;

    //
    // Public methods
    //

    NavMesh();
    virtual ~NavMesh();

    /// Initialize empty nav mesh. You must rebuild nav mesh after that.
    bool Initialize(NavMeshDesc const& inNavigationConfig);

    /// Build all tiles in nav mesh
    bool Build();

    /// Build tiles in specified range
    bool Build(Int2 const& inMins, Int2 const& inMaxs);

    /// Build tiles in specified bounding box
    bool Build(BvAxisAlignedBox const& inBoundingBox);

    bool IsTileExsist(int inX, int inZ) const;

    void RemoveTile(int inX, int inZ);

    void RemoveTiles();

    void RemoveTiles(Int2 const& inMins, Int2 const& inMaxs);

    void AddObstacle(NavMeshObstacle* inObstacle);

    void RemoveObstacle(NavMeshObstacle* inObstacle);

    void UpdateObstacle(NavMeshObstacle* inObstacle);

    /// Purge navigation data
    void Purge();

    /// NavMesh ticking
    void Update(float inTimeStep);

    /// Draw debug info
    void DrawDebug(DebugRenderer* inRenderer);

    /// Casts a 'walkability' ray along the surface of the navigation mesh from
    /// the start position toward the end position.
    bool CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavMeshRayCastResult& outResult, NavQueryFilter const& inFilter) const;

    /// Casts a 'walkability' ray along the surface of the navigation mesh from
    /// the start position toward the end position.
    bool CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavMeshRayCastResult& outResult) const;

    /// Query tile loaction
    bool QueryTileLocaction(Float3 const& inPosition, int* outTileX, int* outTileY) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPolyRef* outNearestPolyRef) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavPolyRef* outNearestPolyRef) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef* outNearestPointRef) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavPointRef* outNearestPointRef) const;

    /// Queries random location on navmesh.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    bool QueryRandomPoint(NavQueryFilter const& inFilter, NavPointRef* outRandomPointRef) const;

    /// Queries random location on navmesh.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    bool QueryRandomPoint(NavPointRef* outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef* outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavPointRef* outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavPointRef* outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavPointRef* outRandomPointRef) const;

    /// Queries the closest point on the specified polygon.
    bool QueryClosestPointOnPoly(NavPointRef const& inPointRef, Float3* outPoint, bool* outOverPolygon = nullptr) const;

    /// Query a point on the boundary closest to the source point if the source point is outside the
    /// polygon's xz-bounds.
    bool QueryClosestPointOnPolyBoundary(NavPointRef const& inPointRef, Float3* outPoint) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavQueryFilter const& inFilter, NavPolyRef* outVisited, int* outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavPolyRef* outVisited, int* outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, NavQueryFilter const& inFilter, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Last visited polys from MoveAlongSurface
    TVector<NavPolyRef> const& GetLastVisitedPolys() const { return m_LastVisitedPolys; }

    /// Finds a path from the start polygon to the end polygon.
    bool FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavQueryFilter const& inFilter, NavPolyRef* outPath, int* outPathCount, const int inMaxPath) const;

    /// Finds a path from the start polygon to the end polygon.
    bool FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavPolyRef* outPath, int* outPathCount, const int inMaxPath) const;

    /// Finds a path from the start position to the end position.
    bool FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, TVector<NavMeshPathPoint>& outPathPoints) const;

    /// Finds a path from the start position to the end position.
    bool FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, TVector<NavMeshPathPoint>& outPathPoints) const;

    /// Finds a path from the start position to the end position.
    bool FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, TVector<Float3>& outPathPoints) const;

    /// Finds a path from the start position to the end position.
    bool FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, TVector<Float3>& outPathPoints) const;

    /// Finds the straight path from the start to the end position within the polygon corridor.
    bool FindStraightPath(Float3 const& inStartPos, Float3 const& inEndPos, NavPolyRef const* inPath, int inPathSize, Float3* outStraightPath, unsigned char* outStraightPathFlags, NavPolyRef* outStraightPathRefs, int* outStraightPathCount, int inMaxStraightPath, NAV_MESH_STRAIGHTPATH_CROSSING inStraightPathCrossing = NAV_MESH_STRAIGHTPATH_DEFAULT) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavMeshHitResult& outHitResult) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavMeshHitResult& outHitResult) const;

    /// Gets the height of the polygon at the provided position using the height detail.
    bool GetHeight(NavPointRef const& inPointRef, float* outHeight) const;

    /// Gets the endpoints for an off-mesh connection, ordered by "direction of travel".
    bool GetOffMeshConnectionPolyEndPoints(NavPolyRef inPrevRef, NavPolyRef inPolyRef, Float3* outStartPos, Float3* outEndPos) const;

    /// Navmesh tile bounding box in world space
    BvAxisAlignedBox GetTileWorldBounds(int inX, int inZ) const;

    /// Navmesh bounding box
    BvAxisAlignedBox const& GetWorldBounds() const { return m_BoundingBox; }

    int GetTileCountX() const { return m_NumTilesX; }
    int GetTileCountZ() const { return m_NumTilesZ; }

    void GatherNavigationGeometry(NavigationGeometry& Geometry);

private:
    bool BuildTile(int inX, int inZ);

    NavMeshDesc      m_Desc;
    int              m_NumTilesX{};
    int              m_NumTilesZ{};
    float            m_TileWidth{1.0f};
    BvAxisAlignedBox m_BoundingBox;
    dtNavMesh*       m_NavMesh{};
    dtNavMeshQuery*  m_NavQuery{};
    //dtCrowd *      m_Crowd{};
    dtTileCache*     m_TileCache{};

    // For tile cache
    TUniqueRef<struct DetourLinearAllocator> m_LinearAllocator;
    TUniqueRef<struct DetourMeshProcess>     m_MeshProcess;

    // NavMesh areas
    //TVector<NavMeshArea> m_Areas;

    // Temp array to reduce memory allocations in MoveAlongSurface
    mutable TVector<NavPolyRef> m_LastVisitedPolys;
};

HK_NAMESPACE_END
