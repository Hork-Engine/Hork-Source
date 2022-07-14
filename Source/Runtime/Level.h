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
#include <Renderer/RenderDefs.h>
#include "VisibilitySystem.h"

class AActor;
class AWorld;
class ALevel;
class ATexture;
class ASceneComponent;
class AConvexHull;
class AIndexedMesh;
class ALightmapUV;
class AVertexLight;
class ADebugRenderer;
struct SVisArea;
struct SPrimitiveDef;
struct SPortalLink;
struct SPrimitiveLink;

struct SLightingSystemCreateInfo
{
    ELightmapFormat        LightmapFormat;
    void*                  LightData;
    size_t                 LightDataSize;
    int                    LightmapBlockWidth;
    int                    LightmapBlockHeight;
    int                    LightmapBlockCount;

    // FUTURE: split shadow casters to chunks for culling
    Float3 const*          ShadowCasterVertices;
    int                    ShadowCasterVertexCount;
    unsigned int const*    ShadowCasterIndices;
    int                    ShadowCasterIndexCount;
    SLightPortalDef const* LightPortals;
    int                    LightPortalsCount;
    Float3 const*          LightPortalVertices;
    int                    LightPortalVertexCount;
    unsigned int const*    LightPortalIndices;
    int                    LightPortalIndexCount;
};

class ALevelLighting : public ARefCounted
{
public:
    ALevelLighting(SLightingSystemCreateInfo const& CreateInfo);
    ~ALevelLighting();

    Float3 SampleLight(int InLightmapBlock, Float2 const& InLighmapTexcoord) const;

    /** Get shadow caster GPU buffers */
    RenderCore::IBuffer* GetShadowCasterVB() { return ShadowCasterVB; }
    RenderCore::IBuffer* GetShadowCasterIB() { return ShadowCasterIB; }

    int GetShadowCasterIndexCount() const { return ShadowCasterIndexCount; }

    /** Get light portals GPU buffers */
    RenderCore::IBuffer* GetLightPortalsVB() { return LightPortalsVB; }
    RenderCore::IBuffer* GetLightPortalsIB() { return LightPortalsIB; }

    TPodVector<SLightPortalDef> const& GetLightPortals() const { return LightPortals; }

    /** Lightmap pixel format */
    ELightmapFormat LightmapFormat = LIGHTMAP_GRAYSCALED_HALF;

    /** Lightmap atlas resolution */
    int LightmapBlockWidth = 0;

    /** Lightmap atlas resolution */
    int LightmapBlockHeight = 0;

    /** Lightmap raw data */
    void* LightData = nullptr;

    /** Static lightmaps (experimental). Indexed by lightmap block. */
    TVector<TRef<RenderCore::ITexture>> Lightmaps;

    TRef<RenderCore::IBuffer> ShadowCasterVB;
    TRef<RenderCore::IBuffer> ShadowCasterIB;

    TRef<RenderCore::IBuffer> LightPortalsVB;
    TRef<RenderCore::IBuffer> LightPortalsIB;

    int ShadowCasterIndexCount{};

    /** Light portals */
    TPodVector<SLightPortalDef>  LightPortals;
    TVector<Float3>       LightPortalVertexBuffer;
    TVector<unsigned int> LightPortalIndexBuffer;
};

constexpr int MAX_AMBIENT_SOUNDS_IN_AREA = 4;

struct SAudioArea
{
    /** Baked leaf audio clip */
    uint16_t AmbientSound[MAX_AMBIENT_SOUNDS_IN_AREA];

    /** Baked leaf audio volume */
    uint8_t AmbientVolume[MAX_AMBIENT_SOUNDS_IN_AREA];
};

struct SLevelAudioCreateInfo
{
    SAudioArea* AudioAreas;
    int NumAudioAreas;

    TVector<TRef<ASoundResource>> AmbientSounds;
};

class ALevelAudio : public ARefCounted
{
public:
    ALevelAudio(SLevelAudioCreateInfo const& CreateInfo);

    /** Baked audio */
    TPodVector<SAudioArea> AudioAreas;

    /** Ambient sounds */
    TVector<TRef<ASoundResource>> AmbientSounds;
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
    TRef<AVisibilityLevel> Visibility;
    TRef<ALevelLighting>   Lighting;
    TRef<ALevelAudio>      Audio;
    
    /** Level is persistent if created by owner world */
    bool IsPersistentLevel() const { return bIsPersistent; }

    /** Get level world */
    AWorld* GetOwnerWorld() const { return OwnerWorld; }

    /** Get actors in level */
    TPodVector<AActor*> const& GetActors() const { return Actors; }

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

    

    ALevel();
    ~ALevel();

protected:
    /** Draw debug. Called by owner world. */
    void DrawDebug(ADebugRenderer* InRenderer);

private:
    /** Callback on add level to world. Called by owner world. */
    void OnAddLevelToWorld();

    /** Callback on remove level from world. Called by owner world. */
    void OnRemoveLevelFromWorld();

    /** Parent world */
    AWorld* OwnerWorld = nullptr;

    bool bIsPersistent = false;

    /** Array of actors */
    TPodVector<AActor*> Actors;

    TPodVector<ALightmapUV*>  LightmapUVs;
    TPodVector<AVertexLight*> VertexLightChannels;
};
