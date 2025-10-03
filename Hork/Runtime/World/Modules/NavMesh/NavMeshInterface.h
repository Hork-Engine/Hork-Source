/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Core/Containers/Array.h>
#include <Hork/Core/Color.h>
#include <Hork/Geometry/BV/BvAxisAlignedBox.h>
#include <Hork/Runtime/World/WorldInterface.h>

class dtNavMesh;
class dtNavMeshQuery;
class dtTileCache;

HK_NAMESPACE_BEGIN

#ifdef DT_POLYREF64
typedef uint64_t NavPolyRef;
#else
typedef uint32_t NavPolyRef;
#endif

enum class NavMeshPathFlags : uint8_t
{
    /// The vertex is the start position in the path.
    Start = 0x01,
    /// The vertex is the end position in the path.
    End = 0x02,
    /// The vertex is the start of an off-mesh connection.
    OffMeshLink = 0x04
};

HK_FLAG_ENUM_OPERATORS(NavMeshPathFlags)

struct NavPointRef
{
    NavPolyRef              PolyRef{};
    Float3                  Position;
};

struct NavMeshPathPoint
{
    Float3                  Position;
    NavMeshPathFlags        Flags{};
};

struct NavMeshRayCastResult
{
    float                   Fraction{};
    Float3                  Normal;
};

struct NavMeshHitResult
{
    Float3                  Position;
    Float3                  Normal;
    float                   Distance{};

    void                    Clear();
};

HK_INLINE void NavMeshHitResult::Clear()
{
    Position.Clear();
    Normal.Clear();
    Distance = 0;
}

enum class NavMeshPartition
{
    /// Best choice if you precompute the navmesh, use this if you have large open areas (default)
    Watershed,
    /// Use this if you want fast navmesh generation
    Monotone,
    /// Good choice to use for tiled navmesh with medium and small sized tiles
    Layers
};

/// Naviagation area type.
/// You can define your own area types. Example:
/// #define NAV_MESH_AREA_ROAD  NAV_MESH_AREA(2)
/// #define NAV_MESH_AREA_DOOR  NAV_MESH_AREA(3)
/// #define NAV_MESH_AREA_GRASS NAV_MESH_AREA(4)
/// #define NAV_MESH_AREA_JUMP  NAV_MESH_AREA(5)
enum NAV_MESH_AREA : uint8_t
{
    NAV_MESH_AREA_GROUND    = 0,
    NAV_MESH_AREA_WATER     = 1,

    /// Max area types
    NAV_MESH_AREA_MAX = 32
};

enum class NavMeshCrossings
{
    Default = 0,
    /// Add a vertex at every polygon edge crossing where area changes
    AreaCrossings = 1,
    /// Add a vertex at every polygon edge crossing
    AllCrossings = 2,
};

struct NavQueryFilter
{
    using AreaCostArray     = Array<float, NAV_MESH_AREA_MAX>;

                            NavQueryFilter();

    /// Sets the traversal cost of the area.
    void                    SetAreaCost(NAV_MESH_AREA areaType, float cost) { m_AreaCost[areaType] = cost; }

    /// Returns the traversal cost of the area.
    float                   GetAreaCost(NAV_MESH_AREA areaType) const { return m_AreaCost[areaType]; }

    /// Include all area types
    void                    IncludeAll() { m_AreaMask = ~0u; }

    /// Exclude all area types
    void                    ExcludeAll() { m_AreaMask = 0; }

    /// Include area type
    void                    IncludeArea(NAV_MESH_AREA areaType) { m_AreaMask |= HK_BIT(areaType); }

    /// Exclude area type
    void                    ExcludeArea(NAV_MESH_AREA areaType) { m_AreaMask &= ~HK_BIT(areaType); }

    /// Set area mask bits
    void                    SetAreaMask(uint32_t mask) { m_AreaMask = mask; }

    /// Get area mask bits
    uint32_t                GetAreaMask() const { return m_AreaMask; }

    AreaCostArray const&    GetAreaCosts() const { return m_AreaCost; }

private:
    AreaCostArray           m_AreaCost;
    uint32_t                m_AreaMask = ~0u;
};

class NavMeshInterface : public WorldInterfaceBase
{
public:
    static constexpr int    MaxVertsPerPoly = 6;
    static constexpr int    MaxAllowedLayers = 255;

    //
    // Initial properties
    //

    /// The walkable height
    float                   WalkableHeight = 2.0f;

    /// The walkable radius
    float                   WalkableRadius = 0.6f;

    /// The maximum traversable ledge (Up/Down)
    float                   WalkableClimb = 0.4f;

    /// The maximum slope that is considered walkable. In degrees, ( 0 <= value < 90 )
    float                   WalkableSlopeAngle = 45.0f;

    /// The xz-plane cell size to use for fields. (value > 0)
    float                   CellSize = 0.3f;

    /// The y-axis cell size to use for fields. (value > 0)
    float                   CellHeight = 0.2f;

    float                   EdgeMaxLength = 12.0f;

    /// The maximum distance a simplfied contour's border edges should deviate
    /// the original raw contour. (value >= 0)
    float                   EdgeMaxError = 1.3f;

    float                   MinRegionSize = 8.0f;

    float                   MergeRegionSize = 20.0f;

    float                   DetailSampleDist = 6.0f;

    float                   DetailSampleMaxError = 1.0f;

    /// The maximum number of vertices allowed for polygons generated during the
    /// contour to polygon conversion process. (value >= 3)
    int                     VertsPerPoly = 6;

    /// The width/height size of tile's on the xz-plane. (value >= 0)
    int                     TileSize = 48;

    bool                    IsDynamic = true;

    /// Max layers for dynamic navmesh (1..255)
    int                     MaxLayers = 16;

    /// Max obstacles for dynamic navmesh
    int                     MaxDynamicObstacles = 1024;

    /// Partition method
    NavMeshPartition        PartitionMethod = NavMeshPartition::Watershed;

    Vector<BvAxisAlignedBox>NavigationVolumes;

    //
    // Public
    //

                            NavMeshInterface();

    /// Create empty nav mesh
    bool                    Create();

    /// Free nav mesh
    void                    Purge();

    /// Clear navigation data
    void                    Clear();

    /// Clear navigation data for specified tile
    void                    ClearTile(int inX, int inZ);

    /// Clear navigation data for specified tiles
    void                    ClearTiles(Int2 const& inMins, Int2 const& inMaxs);

    /// Is navigation data exists for specified tile
    bool                    IsEmpty(int inX, int inZ) const;

    /// Build all tiles in nav mesh
    bool                    Build();

    /// Build on the next frame when all components are initialized
    void                    BuildOnNextFrame();

    /// Build tiles in specified range
    bool                    Build(Int2 const& inMins, Int2 const& inMaxs);

    /// Build tiles in specified bounding box
    bool                    Build(BvAxisAlignedBox const& inBoundingBox);

    /// Sets the traversal cost of the area.
    void                    SetAreaCost(NAV_MESH_AREA inAreaType, float inCost);

    /// Returns the traversal cost of the area.
    float                   GetAreaCost(NAV_MESH_AREA inAreaType) const;

    /// Casts a 'walkability' ray along the surface of the navigation mesh from
    /// the start position toward the end position.
    bool                    CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavQueryFilter const& inFilter, NavMeshRayCastResult& outResult) const;

    /// Casts a 'walkability' ray along the surface of the navigation mesh from
    /// the start position toward the end position.
    bool                    CastRay(Float3 const& inRayStart, Float3 const& inRayEnd, Float3 const& inExtents, NavMeshRayCastResult& outResult) const;

    /// Query tile location
    bool                    GetTileLocation(Float3 const& inPosition, int& outTileX, int& outTileY) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool                    QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPolyRef& outNearestPolyRef) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool                    QueryNearestPoly(Float3 const& inPosition, Float3 const& inExtents, NavPolyRef& outNearestPolyRef) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool                    QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef& outNearestPointRef) const;

    /// Queries the polygon nearest to the specified position.
    /// Extents is the search distance along each axis
    bool                    QueryNearestPoint(Float3 const& inPosition, Float3 const& inExtents, NavPointRef& outNearestPointRef) const;

    /// Queries random location on navmesh.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    bool                    QueryRandomPoint(NavQueryFilter const& inFilter, NavPointRef& outRandomPointRef) const;

    /// Queries random location on navmesh.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    bool                    QueryRandomPoint(NavPointRef& outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool                    QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavPointRef& outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool                    QueryRandomPointAroundCircle(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavPointRef& outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool                    QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavPointRef& outRandomPointRef) const;

    /// Queries random location on navmesh within the reach of specified location.
    /// Polygons are chosen weighted by area. The search runs in linear related to number of polygon.
    /// The location is not exactly constrained by the circle, but it limits the visited polygons.
    bool                    QueryRandomPointAroundCircle(NavPointRef const& inPointRef, float inRadius, NavPointRef& outRandomPointRef) const;

    /// Queries the closest point on the specified polygon.
    bool                    QueryClosestPointOnPoly(NavPointRef const& inPointRef, Float3& outPoint, bool* outOverPolygon = nullptr) const;

    /// Query a point on the boundary closest to the source point if the source point is outside the
    /// polygon's xz-bounds.
    bool                    QueryClosestPointOnPolyBoundary(NavPointRef const& inPointRef, Float3& outPoint) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool                    MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavQueryFilter const& inFilter, NavPolyRef* outVisited, int& outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool                    MoveAlongSurface(NavPointRef const& inPointRef, Float3 const& inDestination, NavPolyRef* outVisited, int& outVisitedCount, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool                    MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, NavQueryFilter const& inFilter, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Moves from the start to the end position constrained to the navigation mesh
    bool                    MoveAlongSurface(Float3 const& inPosition, Float3 const& inDestination, Float3 const& inExtents, int inMaxVisitedSize, Float3& outResultPos) const;

    /// Last visited polys from MoveAlongSurface
    Vector<NavPolyRef> const& GetLastVisitedPolys() const { return m_LastVisitedPolys; }

    /// Finds a path from the start polygon to the end polygon.
    bool                    FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavQueryFilter const& inFilter, NavPolyRef* outPath, int& outPathCount, const int inMaxPath) const;

    /// Finds a path from the start polygon to the end polygon.
    bool                    FindPath(NavPointRef const& inStartRef, NavPointRef const& inEndRef, NavPolyRef* outPath, int& outPathCount, const int inMaxPath) const;

    /// Finds a path from the start position to the end position.
    bool                    FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, Vector<NavMeshPathPoint>& outPathPoints) const;

    /// Finds a path from the start position to the end position.
    bool                    FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, Vector<NavMeshPathPoint>& outPathPoints) const;

    /// Finds a path from the start position to the end position.
    bool                    FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, NavQueryFilter const& inFilter, Vector<Float3>& outPathPoints) const;

    /// Finds a path from the start position to the end position.
    bool                    FindPath(Float3 const& inStartPos, Float3 const& inEndPos, Float3 const& inExtents, Vector<Float3>& outPathPoints) const;

    /// Finds the straight path from the start to the end position within the polygon corridor.
    bool                    FindStraightPath(Float3 const& inStartPos, Float3 const& inEndPos, NavPolyRef const* inPath, int inPathSize, Float3* outStraightPath, NavMeshPathFlags* outStraightPathFlags, NavPolyRef* outStraightPathRefs, int& outStraightPathCount, int inMaxStraightPath, NavMeshCrossings inStraightPathCrossing = NavMeshCrossings::Default) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool                    CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool                    CalcDistanceToWall(NavPointRef const& inPointRef, float inRadius, NavMeshHitResult& outHitResult) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool                    CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavQueryFilter const& inFilter, NavMeshHitResult& outHitResult) const;

    /// Calculates the distance from the specified position to the nearest polygon wall.
    bool                    CalcDistanceToWall(Float3 const& inPosition, float inRadius, Float3 const& inExtents, NavMeshHitResult& outHitResult) const;

    /// Gets the height of the polygon at the provided position using the height detail.
    bool                    GetHeight(NavPointRef const& inPointRef, float& outHeight) const;

    /// Gets the endpoints for an off-mesh connection, ordered by "direction of travel".
    bool                    GetOffMeshConnectionPolyEndPoints(NavPolyRef inPrevRef, NavPolyRef inPolyRef, Float3& outStartPos, Float3& outEndPos) const;

    /// Navmesh tile bounding box in world space
    BvAxisAlignedBox        GetTileWorldBounds(int inX, int inZ) const;

    /// Navmesh bounding box
    BvAxisAlignedBox const& GetWorldBounds() const { return m_BoundingBox; }

    int                     GetTileCountX() const { return m_NumTilesX; }
    int                     GetTileCountZ() const { return m_NumTilesZ; }

    void                    RegisterArea(NAV_MESH_AREA inAreaType, StringView inName, Color4 const& inVisualizeColor);

    NAV_MESH_AREA           GetAreaType(StringView inName) const;

    String                  GetAreaName(NAV_MESH_AREA inAreaType) const;

protected:
    virtual void            Initialize() override;
    virtual void            Deinitialize() override;

private:
    friend class NavMeshObstacleComponent;
    void                    AddObstacle(class NavMeshObstacleComponent* inObstacle);
    void                    RemoveObstacle(class NavMeshObstacleComponent* inObstacle);
    void                    UpdateObstacle(class NavMeshObstacleComponent* inObstacle);

private:
    void                    GatherNavigationGeometry(class NavigationGeometry& navGeometry);
    bool                    BuildTile(int inX, int inZ);
    void                    Update();
    void                    DrawDebug(DebugRenderer& renderer);

    bool                    m_BuildOnNextFrame = false;
    uint64_t                m_FrameNum = 0;
    int                     m_NumTilesX{};
    int                     m_NumTilesZ{};
    float                   m_TileWidth{1.0f};
    dtNavMesh*              m_NavMesh{};
    dtNavMeshQuery*         m_NavQuery{};
    //dtCrowd *             m_Crowd{};
    dtTileCache*            m_TileCache{};
    float                   m_WalkableHeight = 2.0f;
    float                   m_WalkableRadius = 0.6f;
    float                   m_WalkableClimb = 0.4f;
    float                   m_WalkableSlopeAngle = 45.0f;
    float                   m_CellSize = 0.3f;
    float                   m_CellHeight = 0.2f;
    float                   m_EdgeMaxLength = 12.0f;
    float                   m_EdgeMaxError = 1.3f;
    float                   m_MinRegionSize = 8.0f;
    float                   m_MergeRegionSize = 20.0f;
    float                   m_DetailSampleDist = 6.0f;
    float                   m_DetailSampleMaxError = 1.0f;
    int                     m_VertsPerPoly = 6;
    int                     m_TileSize = 48;
    bool                    m_IsDynamic = true;
    int                     m_MaxLayers = 16;
    NavMeshPartition        m_PartitionMethod = NavMeshPartition::Watershed;
    BvAxisAlignedBox        m_BoundingBox = BvAxisAlignedBox::sEmpty();
    mutable NavQueryFilter  m_QueryFilter;

    // For tile cache
    UniqueRef<struct DetourLinearAllocator> m_LinearAllocator;
    UniqueRef<struct DetourMeshProcess>     m_MeshProcess;

    // Temp array to reduce memory allocations in MoveAlongSurface
    mutable Vector<NavPolyRef> m_LastVisitedPolys;

    struct AreaDesc
    {
        String              Name;
        uint32_t            Color;
    };

    AreaDesc                m_AreaDesc[NAV_MESH_AREA_MAX];
};

HK_NAMESPACE_END
