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
#if 0
#pragma once

#include "SoundResource.h"
#include "HitTest.h"
#include <Engine/Renderer/RenderDefs.h>
#include "VisibilitySystem.h"

HK_NAMESPACE_BEGIN

class Actor;
class World;
class Level;
class Texture;
class SceneComponent;
class ConvexHull;
class IndexedMesh;
class VertexLight;
class DebugRenderer;
struct VisArea;
struct PrimitiveDef;
struct PortalLink;
struct PrimitiveLink;

struct LightingSystemCreateInfo
{
    LIGHTMAP_FORMAT       LightmapFormat;
    void*                 LightData;
    size_t                LightDataSize;
    int                   LightmapBlockWidth;
    int                   LightmapBlockHeight;
    int                   LightmapBlockCount;

    // FUTURE: split shadow casters to chunks for culling
    Float3 const*         ShadowCasterVertices;
    int                   ShadowCasterVertexCount;
    unsigned int const*   ShadowCasterIndices;
    int                   ShadowCasterIndexCount;
    LightPortalDef const* LightPortals;
    int                   LightPortalsCount;
    Float3 const*         LightPortalVertices;
    int                   LightPortalVertexCount;
    unsigned int const*   LightPortalIndices;
    int                   LightPortalIndexCount;
};

class LevelLighting : public RefCounted
{
public:
    LevelLighting(LightingSystemCreateInfo const& CreateInfo);
    ~LevelLighting();

    Float3 SampleLight(int InLightmapBlock, Float2 const& InLighmapTexcoord) const;

    /** Get shadow caster GPU buffers */
    RenderCore::IBuffer* GetShadowCasterVB() { return ShadowCasterVB; }
    RenderCore::IBuffer* GetShadowCasterIB() { return ShadowCasterIB; }

    int GetShadowCasterIndexCount() const { return ShadowCasterIndexCount; }

    /** Get light portals GPU buffers */
    RenderCore::IBuffer* GetLightPortalsVB() { return LightPortalsVB; }
    RenderCore::IBuffer* GetLightPortalsIB() { return LightPortalsIB; }

    Vector<LightPortalDef> const& GetLightPortals() const { return LightPortals; }

    /** Lightmap pixel format */
    LIGHTMAP_FORMAT LightmapFormat = LIGHTMAP_GRAYSCALED16_FLOAT;

    /** Lightmap atlas resolution */
    int LightmapBlockWidth = 0;

    /** Lightmap atlas resolution */
    int LightmapBlockHeight = 0;

    /** Lightmap raw data */
    void* LightData = nullptr;

    /** Static lightmaps (experimental). Indexed by lightmap block. */
    Vector<Ref<RenderCore::ITexture>> Lightmaps;

    Ref<RenderCore::IBuffer> ShadowCasterVB;
    Ref<RenderCore::IBuffer> ShadowCasterIB;

    Ref<RenderCore::IBuffer> LightPortalsVB;
    Ref<RenderCore::IBuffer> LightPortalsIB;

    int ShadowCasterIndexCount{};

    /** Light portals */
    Vector<LightPortalDef>  LightPortals;
    Vector<Float3>       LightPortalVertexBuffer;
    Vector<unsigned int> LightPortalIndexBuffer;
};

constexpr int MAX_AMBIENT_SOUNDS_IN_AREA = 4;

struct AudioArea
{
    /** Baked leaf audio clip */
    uint16_t AmbientSound[MAX_AMBIENT_SOUNDS_IN_AREA];

    /** Baked leaf audio volume */
    uint8_t AmbientVolume[MAX_AMBIENT_SOUNDS_IN_AREA];
};

struct LevelAudioCreateInfo
{
    AudioArea* AudioAreas;
    int NumAudioAreas;

    Vector<Ref<SoundResource>> AmbientSounds;
};

class LevelAudio : public RefCounted
{
public:
    LevelAudio(LevelAudioCreateInfo const& CreateInfo);

    /** Baked audio */
    Vector<AudioArea> AudioAreas;

    /** Ambient sounds */
    Vector<Ref<SoundResource>> AmbientSounds;
};

class LevelGeometry
{
public:
    /* TODO */
};


/**

Level

Subpart of a world. Contains actors, level visibility, baked data like lightmaps, surfaces, collision, audio, etc.

*/
class Level
{
    friend class World;
    friend class Actor;

public:
    Ref<VisibilityLevel> Visibility;
    Ref<LevelLighting>   Lighting;
    Ref<LevelAudio>      Audio;

    Level() = default;
    
    /** Level is persistent if created by owner world */
    bool IsPersistentLevel() const { return m_bIsPersistent; }

    /** Get level world */
    World* GetOwnerWorld() const { return m_OwnerWorld; }

    /** Get actors in level */
    Vector<Actor*> const& GetActors() const { return m_Actors; }

    /** Destroy all actors in the level */
    void DestroyActors();
    #if 0
    /** Create vertex light channel for a mesh to store light colors */
    uint32_t AddVertexLightChannel(IndexedMesh* InSourceMesh);

    void RemoveVertexLightChannel(uint32_t VertexLightChannel);

    /** Remove all vertex light channels inside the level */
    void RemoveVertexLightChannels();

    VertexLight* GetVertexLight(uint32_t VertexLightChannel);

    /** Get all vertex light channels inside the level */
    Vector<VertexLight*> const& GetVertexLightChannels() const { return m_VertexLightChannels; }
    #endif

    /** Sample lightmap by texture coordinate */
    Float3 SampleLight(int InLightmapBlock, Float2 const& InLighmapTexcoord) const;

protected:
    /** Draw debug. Called by owner world. */
    void DrawDebug(DebugRenderer* InRenderer);

private:
    /** Callback on add level to world. Called by owner world. */
    void OnAddLevelToWorld();

    /** Callback on remove level from world. Called by owner world. */
    void OnRemoveLevelFromWorld();

    /** Parent world */
    World* m_OwnerWorld = nullptr;

    bool m_bIsPersistent = false;

    /** Array of actors */
    Vector<Actor*> m_Actors;

    Vector<VertexLight*> m_VertexLightChannels;
    Vector<uint32_t> m_FreeVertexLightChannels;
};

HK_NAMESPACE_END
#endif