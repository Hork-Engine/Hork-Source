/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "Resource/CollisionBody.h"
#include "Resource/Material.h"
#include "Audio/AudioClip.h"
#include "HitTest.h"
#include <Runtime/Public/RenderCore.h>

class AActor;
class AWorld;
class ATexture;
class ASceneComponent;
class ABrushModel;
class AConvexHull;
class AIndexedMesh;
class ALightmapUV;
class AVertexLight;
class ATreeAABB;
class ADebugRenderer;
struct SVisArea;
struct SPrimitiveDef;
struct SPortalLink;
struct SPrimitiveLink;

enum VSD_PRIMITIVE
{
    VSD_PRIMITIVE_BOX,
    VSD_PRIMITIVE_SPHERE
};

enum VSD_QUERY_MASK
{
    VSD_QUERY_MASK_VISIBLE                  = 0x00000001,
    VSD_QUERY_MASK_INVISIBLE                = 0x00000002,

    VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS    = 0x00000004,
    VSD_QUERY_MASK_INVISIBLE_IN_LIGHT_PASS  = 0x00000008,

    VSD_QUERY_MASK_SHADOW_CAST              = 0x00000010,
    VSD_QUERY_MASK_NO_SHADOW_CAST           = 0x00000020,

    VSD_QUERY_LIGHTMAP_EXPEREMENTAL         = 0x00000040,

    // Reserved for future
    VSD_QUERY_MASK_RESERVED1                = 0x00000080,
    VSD_QUERY_MASK_RESERVED2                = 0x00000100,
    VSD_QUERY_MASK_RESERVED3                = 0x00000200,
    VSD_QUERY_MASK_RESERVED4                = 0x00000400,
    VSD_QUERY_MASK_RESERVED5                = 0x00000800,
    VSD_QUERY_MASK_RESERVED6                = 0x00001000,
    VSD_QUERY_MASK_RESERVED7                = 0x00002000,
    VSD_QUERY_MASK_RESERVED8                = 0x00004000,
    VSD_QUERY_MASK_RESERVED9                = 0x00008000,

    // User filter mask
    VSD_QUERY_MASK_USER0                    = 0x00010000,
    VSD_QUERY_MASK_USER1                    = 0x00020000,
    VSD_QUERY_MASK_USER2                    = 0x00040000,
    VSD_QUERY_MASK_USER3                    = 0x00080000,
    VSD_QUERY_MASK_USER4                    = 0x00100000,
    VSD_QUERY_MASK_USER5                    = 0x00200000,
    VSD_QUERY_MASK_USER6                    = 0x00400000,
    VSD_QUERY_MASK_USER7                    = 0x00800000,
    VSD_QUERY_MASK_USER8                    = 0x01000000,
    VSD_QUERY_MASK_USER9                    = 0x02000000,
    VSD_QUERY_MASK_USER10                   = 0x04000000,
    VSD_QUERY_MASK_USER11                   = 0x08000000,
    VSD_QUERY_MASK_USER12                   = 0x10000000,
    VSD_QUERY_MASK_USER13                   = 0x20000000,
    VSD_QUERY_MASK_USER14                   = 0x40000000,
    VSD_QUERY_MASK_USER15                   = 0x80000000,
};

enum VISIBILITY_GROUP
{
    VISIBILITY_GROUP_DEFAULT = 1
};

constexpr int MAX_AMBIENT_SOUNDS_IN_AREA = 4;

struct SAudioArea
{
    /** Baked leaf audio clip */
    uint16_t AmbientSound[MAX_AMBIENT_SOUNDS_IN_AREA];

    /** Baked leaf audio volume */
    uint8_t AmbientVolume[MAX_AMBIENT_SOUNDS_IN_AREA];
};

struct SBinarySpacePlane : PlaneF
{
    /** Plane axial type */
    uint8_t Type;

    AN_FORCEINLINE float DistFast( Float3 const & InPoint ) const {
        return ( Type < 3 ) ? ( InPoint[ Type ] + D ) : ( Math::Dot( InPoint, Normal ) + D );
    }
};

struct SNodeBase
{
    /** Parent node */
    struct SBinarySpaceNode * Parent;

    /** Visited mark */
    int ViewMark;

    /** Node bounding box (for culling) */
    BvAxisAlignedBox Bounds;
};

struct SBinarySpaceNode : SNodeBase
{
    /** Node split plane */
    SBinarySpacePlane * Plane;

    /** Child indices */
    int ChildrenIdx[2];
};

enum EBinarySpaceLeafContents
{
    BSP_CONTENTS_NORMAL,
    BSP_CONTENTS_INVISIBLE
};

struct SBinarySpaceLeaf : SNodeBase
{
    /** Leaf PVS cluster */
    int PVSCluster;

    /** Leaf PVS */
    byte const * Visdata;

    /** Leaf contents (e.g. Water, Slime) */
    int Contents;       // EBinarySpaceLeafContents

    /** Baked audio */
    int AudioArea;

    /** Visibility area */
    SVisArea * Area;
};

enum ESurfaceGeometryType
{
    /** Triangle soup mesh */
    SURF_TRISOUP,

    /** Planar surface */
    SURF_PLANAR,

    /** Bezier patch */
    //SURF_BEZIER_PATCH
};

struct SSurfaceDef
{
    /** Parent brush model */
    ABrushModel * Model;

    /** Bounding box of the surface */
    BvAxisAlignedBox Bounds;

    /** Vertex offset */
    int FirstVertex;

    /** Vertex count */
    int NumVertices;

    /** Index offset */
    int FirstIndex;

    /** Index count */
    int NumIndices;
#if 0
    /** Patch size for the bezier surfaces */
    int PatchWidth;
    int PatchHeight;
#endif
    /** Index in array of materials */
    uint32_t MaterialIndex;

    /** Sort key. Used for surface batching. */
    uint32_t SortKey;

    /** Surface geometry type */
    ESurfaceGeometryType GeometryType;

    /** Plane for planar surface */
    PlaneF Face;

    /** Lightmap atlas index */
    int LightmapBlock;

    /** Size of the lightmap */
    int LightmapWidth;

    /** Size of the lightmap */
    int LightmapHeight;

    /** Offset in the lightmap */
    int LightmapOffsetX;

    /** Offset in the lightmap */
    int LightmapOffsetY;

    /** Visibility query group. See VSD_QUERY_MASK enum. */
    int QueryGroup;

    /** Visibility group. See VISIBILITY_GROUP enum. */
    int VisGroup;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;

    /** Surface marked as visible. Used by VSD. */
    int VisPass;

    /** Drawable rendering order */
    uint8_t RenderingOrder;

    /** Generate sort key. Call this after RenderingOrder/Model/MaterialIndex/LightmapBlock have changed. */
    void RegenerateSortKey() {
        // NOTE: 8 bits are still unused. We can use it in future.
        SortKey = ((uint64_t)(RenderingOrder & 0xffu) << 56u)
                | ((uint64_t)(Core::PHHash64( (uint64_t)Model ) & 0xffffu) << 40u)
                | ((uint64_t)(Core::PHHash32( MaterialIndex ) & 0xffffu) << 24u)
                | ((uint64_t)(Core::PHHash32( LightmapBlock ) & 0xffffu) << 8u);
    }
};

struct SPrimitiveDef
{
    ASceneComponent * Owner;

    VSD_PRIMITIVE Type;

    union
    {
        BvAxisAlignedBox Box;
        BvSphere Sphere;
    };

    PlaneF Face;

    /** Visibility query group. See VSD_QUERY_MASK enum. */
    int QueryGroup;

    /** Visibility group. See VISIBILITY_GROUP enum. */
    int VisGroup;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;

    /** Primitve marked as visible. Used by VSD. */
    int VisPass;

    //int BakeIndex;

    bool bFaceCull : 1;

    bool bIsOutdoor : 1;

    bool bPendingRemove : 1;

    bool bMovable : 1;

    SPrimitiveLink * Links;

    SPrimitiveDef * Next;
    SPrimitiveDef * Prev;
    SPrimitiveDef * NextUpd;
    SPrimitiveDef * PrevUpd;

    bool (*RaycastCallback)( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 const & InRayEnd, TPodArray< STriangleHitResult > & Hits, int & ClosestHit );
    bool (*RaycastClosestCallback)( SPrimitiveDef const * Self, Float3 const & InRayStart, Float3 & HitLocation, Float2 & HitUV, float & HitDistance, SMeshVertex const ** pVertices, unsigned int Indices[3], TRef< AMaterialInstance > & Material );
};

struct SPrimitiveLink
{
    /** The area */
    SVisArea *        Area;

    /** The primitive */
    SPrimitiveDef *   Primitive;

    /** Next primitive in the area */
    SPrimitiveLink *  NextInArea;

    /** Next link for the primitive */
    SPrimitiveLink *  Next;
};

struct SVisPortal
{
#if 0
    /** Portal owner */
    ALevel * ParentLevel;
#endif

    /** Linked areas */
    SVisArea * Area[2];

    /** Portal to areas */
    SPortalLink * Portals[2];

    /** Portal hull */
    AConvexHull * Hull;

    /** Portal plane */
    PlaneF Plane;

#if 0
    /** Blocking bits for doors (TODO) */
    int BlockingBits;
#endif

    /** Visibility marker */
    int VisMark;
};

struct SPortalLink
{
    /** Area visible from the portal */
    SVisArea * ToArea;

    /** Portal hull */
    AConvexHull * Hull;

    /** Portal plane */
    PlaneF Plane;

    /** Next portal inside an area */
    SPortalLink * Next;

    /** Visibility portal */
    SVisPortal * Portal;
};

struct SVisArea
{
    /** Reference point is used to determine the portal facing */
    Float3 ReferencePoint;

#if 0
    /** Area owner */
    ALevel * ParentLevel;
#endif

    /** Area bounding box */
    BvAxisAlignedBox Bounds;  // FIXME: will be removed later?

    /** Linked portals */
    SPortalLink * PortalList;

    /** Movable primitives inside the area */
    SPrimitiveLink * Links;

    /** Non-movable primitives if AABB tree is not present */
    //SPrimitiveLink * NonMovableLinks;

    /** AABB tree for non-movable primitives */
    //ATreeAABB * AABBTree;

    /** Baked surfaces attached to the area */
    int FirstSurface;

    /** Count of the baked surfaces attached to the area */
    int NumSurfaces;

    /** Visibility/raycast processed marker. Used by VSD. */
    int VisMark;
};

class ALevel;

class ABrushModel : public ABaseObject {
    AN_CLASS( ABrushModel, ABaseObject )

public:

    /** Baked surface definitions */
    TPodArrayHeap< SSurfaceDef > Surfaces;

    /** Baked surface vertex data */
    TPodArrayHeap< SMeshVertex > Vertices;

    /** Baked surface vertex data */
    TPodArrayHeap< SMeshVertexUV > LightmapVerts;

    /** Baked surface vertex data */
    TPodArrayHeap< SMeshVertexLight > VertexLight;

    /** Baked surface triangle index data */
    TPodArrayHeap< unsigned int > Indices;

    /** Surface materials */
    TStdVector< TRef< AMaterialInstance > > SurfaceMaterials;

    /** Baked collision data */
    ACollisionBodyComposition BodyComposition;

    /** Lighting data will be used from that level. */
    TWeakRef< ALevel > ParentLevel;

    void Purge();

protected:
    ABrushModel() {}
    ~ABrushModel() {}
};


enum ELightmapFormat {
    LIGHTMAP_GRAYSCALED_HALF,
    LIGHTMAP_BGR_HALF
};


/**

ALevel

Subpart of a world. Contains actors, level visibility, baked data like lightmaps, surfaces, collision, audio, etc.

*/
class ALevel : public ABaseObject {
    AN_CLASS( ALevel, ABaseObject )

    friend class AWorld;

public:
    using AArrayOfNodes = TPodArray< SBinarySpaceNode >;
    using AArrayOfLeafs = TPodArray< SBinarySpaceLeaf >;

    /** BSP nodes */
    AArrayOfNodes Nodes;

    /** BSP leafs */
    AArrayOfLeafs Leafs;

    /** Node split planes */
    TPodArray< SBinarySpacePlane > SplitPlanes;

    /** Level indoor areas */
    TPodArray< SVisArea > Areas;

    /** Level outdoor area */
    SVisArea OutdoorArea;

    /** Level portals */
    TPodArray< SVisPortal > Portals;

    /** Links between the portals and areas */
    TPodArray< SPortalLink > AreaPortals;

    /** Lightmap pixel format */
    ELightmapFormat LightmapFormat;

    /** Lightmap atlas resolution */
    int LightmapBlockWidth;

    /** Lightmap atlas resolution */
    int LightmapBlockHeight;

    /** Lightmap raw data */
    void * LightData;

    /** PVS data */
    byte * Visdata;

    /** Is PVS data compressed or not (ZRLE) */
    bool bCompressedVisData;

    /** Count of a clusters in PVS data */
    int PVSClustersCount;

    /** Surface to area attachments */
    TPodArray< int > AreaSurfaces;

    /** Baked audio */
    TPodArray< SAudioArea > AudioAreas;

    /** Audio clips */
    TStdVector< TRef< AAudioClip > > AudioClips;

    /** Baked surface data */
    TRef< ABrushModel > Model;

    /** Static lightmaps (experemental). Indexed by lightmap block. */
    TStdVector< TRef< ATexture > > Lightmaps;

    /** Not movable primitives */
    //TPodArray< SPrimitiveDef * > BakedPrimitives;

    // TODO: Keep here static navigation geometry
    // TODO: Octree/AABBtree for outdoor area
    // TODO: combine AddArea/AddPortal/Initialize to Initialize() method

    /** Create vis area */
    int AddArea( Float3 const & _Position, Float3 const & _Extents, Float3 const & _ReferencePoint );

    /** Create vis portal. Don't create areas after portals! */
    int AddPortal( Float3 const * _HullPoints, int _NumHullPoints, int _Area1, int _Area2 );

    /** Build level visibility */
    void Initialize();

    /** Purge level data */
    void Purge();

    /** Level is persistent if created by owner world */
    bool IsPersistentLevel() const { return bIsPersistent; }

    /** Get level world */
    AWorld * GetOwnerWorld() const { return OwnerWorld; }

    /** Get actors in level */
    TPodArray< AActor * > const & GetActors() const { return Actors; }

    /** Get level indoor bounding box */
    BvAxisAlignedBox const & GetIndoorBounds() const { return IndoorBounds; }

    /** Get level areas */
    TPodArray< SVisArea > const & GetAreas() const { return Areas; }

    /** Get level outdoor area */
    SVisArea const * GetOutdoorArea() const { return &OutdoorArea; }

    /** Find level leaf */
    int FindLeaf( Float3 const & _Position );

    /** Find level area */
    SVisArea * FindArea( Float3 const & _Position );

    /** Mark potentially visible leafs. Uses PVS */
    int MarkLeafs( int _ViewLeaf );

    /** Destroy all actors in the level */
    void DestroyActors();

    /** Create lightmap channel for a mesh to store lighmap UVs */
    ALightmapUV * CreateLightmapUVChannel( AIndexedMesh * InSourceMesh );

    /** Create vertex light channel for a mesh to store light colors */
    AVertexLight * CreateVertexLightChannel( AIndexedMesh * InSourceMesh );

    /** Remove all lightmap channels inside the level */
    void RemoveLightmapUVChannels();

    /** Remove all vertex light channels inside the level */
    void RemoveVertexLightChannels();

    /** Get all lightmap channels inside the level */
    TPodArray< ALightmapUV * > const & GetLightmapUVChannels() const { return LightmapUVs; }

    /** Get all vertex light channels inside the level */
    TPodArray< AVertexLight * > const & GetVertexLightChannels() const { return VertexLightChannels; }

    /** Sample lightmap by texture coordinate */
    Float3 SampleLight( int InLightmapBlock, Float2 const & InLighmapTexcoord ) const;

    /** Query vis areas by bounding box */
    void QueryOverplapAreas( BvAxisAlignedBox const & InBounds, TPodArray< SVisArea * > & Areas );

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas( BvSphere const & InBounds, TPodArray< SVisArea * > & Areas );

    /** Add primitive to the level */
    void AddPrimitive( SPrimitiveDef * InPrimitive );

    /** Add primitive from the level */
    void RemovePrimitive( SPrimitiveDef * InPrimitive );

    /** Mark primitive dirty */
    void MarkPrimitive( SPrimitiveDef * InPrimitive );

protected:
    ALevel();
    ~ALevel();

    /** Level ticking. Called by owner world. */
    void Tick( float _TimeStep );

    /** Draw debug. Called by owner world. */
    void DrawDebug( ADebugRenderer * InRenderer );

private:
    /** Callback on add level to world. Called by owner world. */
    void OnAddLevelToWorld();

    /** Callback on remove level from world. Called by owner world. */
    void OnRemoveLevelFromWorld();

    void QueryOverplapAreas_r( int InNodeIndex, BvAxisAlignedBox const & InBounds, TPodArray< SVisArea * > & Areas );
    void QueryOverplapAreas_r( int InNodeIndex, BvSphere const & InBounds, TPodArray< SVisArea * > & Areas );

    void AddBoxRecursive( int InNodeIndex, SPrimitiveDef * InPrimitive );
    void AddSphereRecursive( int InNodeIndex, SPrimitiveDef * InPrimitive );

    /** Link primitive to the level areas */
    void LinkPrimitive( SPrimitiveDef * InPrimitive );

    /** Unlink primitive from the level areas */
    void UnlinkPrimitive( SPrimitiveDef * InPrimitive );

    /** Mark all primitives in the level */
    void MarkPrimitives();

    /** Unmark all primitives in the level */
    void UnmarkPrimitives();

    void RemovePrimitives();

    byte const * LeafPVS( SBinarySpaceLeaf const * _Leaf );

    byte const * DecompressVisdata( byte const * _Data );

    void PurgePortals();

    void UpdatePrimitiveLinks();

    /** Parent world */
    AWorld * OwnerWorld;

    int IndexInArrayOfLevels = -1;

    bool bIsPersistent;

    /** Array of actors */
    TPodArray< AActor * > Actors;

    BvAxisAlignedBox IndoorBounds;

    class AWorldspawn * Worldspawn;

    TPodArray< ALightmapUV * > LightmapUVs;
    TPodArray< AVertexLight * > VertexLightChannels;

    byte * DecompressedVisData;

    /** Node visitor mark */
    int ViewMark;

    /** Cluster index for view origin */
    int ViewCluster;

    SPrimitiveDef * PrimitiveList;
    SPrimitiveDef * PrimitiveListTail;
    SPrimitiveDef * PrimitiveUpdateList;
    SPrimitiveDef * PrimitiveUpdateListTail;
};
