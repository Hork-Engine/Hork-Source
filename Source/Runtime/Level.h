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

#include "Material.h"
#include "SoundResource.h"
#include "HitTest.h"
#include "VSD.h"
#include <Renderer/RenderDefs.h>

class AActor;
class AWorld;
class ALevel;
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

enum VISIBILITY_GROUP
{
    VISIBILITY_GROUP_DEFAULT = 1,
    VISIBILITY_GROUP_SKYBOX  = 2,
    VISIBILITY_GROUP_TERRAIN = 4
};

constexpr int MAX_AMBIENT_SOUNDS_IN_AREA = 4;

struct SAudioArea
{
    /** Baked leaf audio clip */
    uint16_t AmbientSound[MAX_AMBIENT_SOUNDS_IN_AREA];

    /** Baked leaf audio volume */
    uint8_t AmbientVolume[MAX_AMBIENT_SOUNDS_IN_AREA];
};

struct SLightPortalDef
{
    /** First mesh vertex in array of vertices */
    int FirstVert;

    /** Mesh vertex count */
    int NumVerts;

    int FirstIndex;

    int NumIndices;
};

class ABrushModel : public ABaseObject
{
    HK_CLASS(ABrushModel, ABaseObject)

public:
    /** Baked surface definitions */
    TPodVectorHeap<SSurfaceDef> Surfaces;

    /** Baked surface vertex data */
    TPodVectorHeap<SMeshVertex> Vertices;

    /** Baked surface vertex data */
    TPodVectorHeap<SMeshVertexUV> LightmapVerts;

    /** Baked surface vertex data */
    TPodVectorHeap<SMeshVertexLight> VertexLight;

    /** Baked surface triangle index data */
    TPodVectorHeap<unsigned int> Indices;

    /** Surface materials */
    TStdVector<TRef<AMaterialInstance>> SurfaceMaterials;

    /** Baked collision data */
    //TRef< ACollisionModel > CollisionModel;

    /** Lighting data will be used from that level. */
    TWeakRef<ALevel> ParentLevel;

    void Purge();

    ABrushModel() {}
    ~ABrushModel() {}
};


enum ELightmapFormat
{
    LIGHTMAP_GRAYSCALED_HALF,
    LIGHTMAP_BGR_HALF
};

/**

ALevel

Subpart of a world. Contains actors, level visibility, baked data like lightmaps, surfaces, collision, audio, etc.

*/
class ALevel : public ABaseObject
{
    HK_CLASS(ALevel, ABaseObject)

    friend class AWorld;
    friend class AActor;

public:
    using AArrayOfNodes = TPodVector<SBinarySpaceNode>;
    using AArrayOfLeafs = TPodVector<SBinarySpaceLeaf>;

    template <typename T, typename... TArgs>
    static TRef<ALevel> CreateLevel(TArgs&&... args)
    {
        TUniqueRef<T> ctor = MakeUnique<T>(std::forward<TArgs>(args)...);

        return MakeRef<ALevel>(std::move(ctor));
    }

    /** BSP nodes */
    AArrayOfNodes Nodes;

    /** BSP leafs */
    AArrayOfLeafs Leafs;

    /** Node split planes */
    TPodVector<SBinarySpacePlane> SplitPlanes;

    /** Level indoor areas */
    TPodVector<SVisArea> Areas;

    /** Level outdoor area */
    SVisArea OutdoorArea;

    /** Visibility method */
    ELevelVisibilityMethod VisibilityMethod = LEVEL_VISIBILITY_PORTAL;

    /** Lightmap pixel format */
    ELightmapFormat LightmapFormat = LIGHTMAP_GRAYSCALED_HALF;

    /** Lightmap atlas resolution */
    int LightmapBlockWidth = 0;

    /** Lightmap atlas resolution */
    int LightmapBlockHeight = 0;

    /** Lightmap raw data */
    void* LightData = nullptr;

    /** PVS data */
    byte* Visdata = nullptr;

    /** Is PVS data compressed or not (ZRLE) */
    bool bCompressedVisData = false;

    /** Count of a clusters in PVS data */
    int PVSClustersCount = 0;

    /** Surface to area attachments */
    TPodVector<int> AreaSurfaces;

    /** Baked audio */
    TPodVector<SAudioArea> AudioAreas;

    /** Ambient sounds */
    TStdVector<TRef<ASoundResource>> AmbientSounds;

    /** Baked surface data */
    TRef<ABrushModel> Model;

    /** Static lightmaps (experimental). Indexed by lightmap block. */
    TStdVector<TRef<ATexture>> Lightmaps;

    /** Vertex buffer for baked static shadow casters
    FUTURE: split to chunks for culling
    */
    TPodVectorHeap<Float3> ShadowCasterVerts;

    /** Index buffer for baked static shadow casters */
    TPodVectorHeap<unsigned int> ShadowCasterIndices;

    /** Not movable primitives */
    //TPodVector< SPrimitiveDef * > BakedPrimitives;

    // TODO: Keep here static navigation geometry
    // TODO: Octree/AABBtree for outdoor area

    /** Create and link portals */
    void CreatePortals(SPortalDef const* InPortals, int InPortalsCount, Float3 const* InHullVertices);

    /** Create light portals */
    void CreateLightPortals(SLightPortalDef const* InPortals, int InPortalsCount, Float3 const* InMeshVertices, int InVertexCount, unsigned int const* InMeshIndices, int InIndexCount);

    /** Build level visibility */
    void Initialize();

    /** Purge level data */
    void Purge();

    /** Level is persistent if created by owner world */
    bool IsPersistentLevel() const { return bIsPersistent; }

    /** Get level world */
    AWorld* GetOwnerWorld() const { return OwnerWorld; }

    /** Get actors in level */
    TPodVector<AActor*> const& GetActors() const { return Actors; }

    /** Get level indoor bounding box */
    BvAxisAlignedBox const& GetIndoorBounds() const { return IndoorBounds; }

    /** Get level areas */
    TPodVector<SVisArea> const& GetAreas() const { return Areas; }

    /** Get level outdoor area */
    SVisArea const* GetOutdoorArea() const { return &OutdoorArea; }

    /** Find level leaf */
    int FindLeaf(Float3 const& _Position);

    /** Find level area */
    SVisArea* FindArea(Float3 const& _Position);

    /** Mark potentially visible leafs. Uses PVS */
    int MarkLeafs(int _ViewLeaf);

    /** Destroy all actors in the level */
    void DestroyActors();

    /** Create lightmap channel for a mesh to store lighmap UVs */
    ALightmapUV* CreateLightmapUVChannel(AIndexedMesh* InSourceMesh);

    /** Create vertex light channel for a mesh to store light colors */
    AVertexLight* CreateVertexLightChannel(AIndexedMesh* InSourceMesh);

    /** Remove all lightmap channels inside the level */
    void RemoveLightmapUVChannels();

    /** Remove all vertex light channels inside the level */
    void RemoveVertexLightChannels();

    /** Get all lightmap channels inside the level */
    TPodVector<ALightmapUV*> const& GetLightmapUVChannels() const { return LightmapUVs; }

    /** Get all vertex light channels inside the level */
    TPodVector<AVertexLight*> const& GetVertexLightChannels() const { return VertexLightChannels; }

    /** Sample lightmap by texture coordinate */
    Float3 SampleLight(int InLightmapBlock, Float2 const& InLighmapTexcoord) const;

    /** Query vis areas by bounding box */
    void QueryOverplapAreas(BvAxisAlignedBox const& InBounds, TPodVector<SVisArea*>& Areas);

    /** Query vis areas by bounding sphere */
    void QueryOverplapAreas(BvSphere const& InBounds, TPodVector<SVisArea*>& Areas);

    /** Add primitive to the level */
    void AddPrimitive(SPrimitiveDef* InPrimitive);

    /** Remove primitive from the level */
    void RemovePrimitive(SPrimitiveDef* InPrimitive);

    /** Mark primitive dirty */
    void MarkPrimitive(SPrimitiveDef* InPrimitive);

    /** Get shadow caster GPU buffers */
    RenderCore::IBuffer* GetShadowCasterVB() { return ShadowCasterVB; }
    RenderCore::IBuffer* GetShadowCasterIB() { return ShadowCasterIB; }

    /** Get light portals GPU buffers */
    RenderCore::IBuffer* GetLightPortalsVB() { return LightPortalsVB; }
    RenderCore::IBuffer* GetLightPortalsIB() { return LightPortalsIB; }

    TPodVector<SLightPortalDef> const& GetLightPortals() const { return LightPortals; }

    using APrimitiveLinkPool = TPoolAllocator<SPrimitiveLink>;

    static APrimitiveLinkPool PrimitiveLinkPool;

    ALevel();
    ~ALevel();

protected:
    /** Level ticking. Called by owner world. */
    void Tick(float _TimeStep);

    /** Draw debug. Called by owner world. */
    virtual void DrawDebug(ADebugRenderer* InRenderer);

private:
    template <typename T>
    ALevel(TUniqueRef<T>&& ctor)
    {
        ctor->pNodes = &Nodes;
        ctor->Build();
    }

    /** Callback on add level to world. Called by owner world. */
    void OnAddLevelToWorld();

    /** Callback on remove level from world. Called by owner world. */
    void OnRemoveLevelFromWorld();

    void QueryOverplapAreas_r(int InNodeIndex, BvAxisAlignedBox const& InBounds, TPodVector<SVisArea*>& Areas);
    void QueryOverplapAreas_r(int InNodeIndex, BvSphere const& InBounds, TPodVector<SVisArea*>& Areas);

    void AddBoxRecursive(int InNodeIndex, SPrimitiveDef* InPrimitive);
    void AddSphereRecursive(int InNodeIndex, SPrimitiveDef* InPrimitive);

    /** Link primitive to the level areas */
    void LinkPrimitive(SPrimitiveDef* InPrimitive);

    /** Unlink primitive from the level areas */
    void UnlinkPrimitive(SPrimitiveDef* InPrimitive);

    /** Mark all primitives in the level */
    void MarkPrimitives();

    /** Unmark all primitives in the level */
    void UnmarkPrimitives();

    void RemovePrimitives();

    byte const* LeafPVS(SBinarySpaceLeaf const* _Leaf);

    byte const* DecompressVisdata(byte const* _Data);

    void PurgePortals();

    void UpdatePrimitiveLinks();

    void AddPrimitiveToArea(SVisArea* Area, SPrimitiveDef* Primitive);

    /** Parent world */
    AWorld* OwnerWorld = nullptr;

    bool bIsPersistent = false;

    /** Level portals */
    TPodVector<SVisPortal> Portals;

    /** Links between the portals and areas */
    TPodVector<SPortalLink> AreaLinks;

    /** Light portals */
    TPodVector<SLightPortalDef>  LightPortals;
    TPodVectorHeap<Float3>       LightPortalVertexBuffer;
    TPodVectorHeap<unsigned int> LightPortalIndexBuffer;

    /** Array of actors */
    TPodVector<AActor*> Actors;

    BvAxisAlignedBox IndoorBounds;

    TPodVector<ALightmapUV*>  LightmapUVs;
    TPodVector<AVertexLight*> VertexLightChannels;

    byte* DecompressedVisData = nullptr;

    /** Node visitor mark */
    int ViewMark = 0;

    /** Cluster index for view origin */
    int ViewCluster = -1;

    TRef<RenderCore::IBuffer> ShadowCasterVB;
    TRef<RenderCore::IBuffer> ShadowCasterIB;

    TRef<RenderCore::IBuffer> LightPortalsVB;
    TRef<RenderCore::IBuffer> LightPortalsIB;

    SPrimitiveDef* PrimitiveList           = nullptr;
    SPrimitiveDef* PrimitiveListTail       = nullptr;
    SPrimitiveDef* PrimitiveUpdateList     = nullptr;
    SPrimitiveDef* PrimitiveUpdateListTail = nullptr;
};
