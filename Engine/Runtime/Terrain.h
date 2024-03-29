/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "Resource.h"
#include "HitTest.h"
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>
#include <Engine/Core/IntrusiveLinkedListMacro.h>

class btHeightfieldTerrainShape;

HK_NAMESPACE_BEGIN

struct TerrainTriangle
{
    Float3 Vertices[3];
    Float3 Normal;
    Float2 Texcoord;
};

enum TERRAIN_UPDATE_FLAG : uint8_t
{
    TERRAIN_UPDATE_ALL = TERRAIN_UPDATE_FLAG(~0),
};

HK_FLAG_ENUM_OPERATORS(TERRAIN_UPDATE_FLAG)

class TerrainResourceListener
{
public:
    TLink<TerrainResourceListener> Link;

    virtual void OnTerrainResourceUpdate(TERRAIN_UPDATE_FLAG UpdateFlag) = 0;
};

class Terrain : public Resource
{
    HK_CLASS(Terrain, Resource)

public:
    TList<TerrainResourceListener> Listeners;

    Terrain() = default;
    Terrain(int Resolution, const float* pData);

    ~Terrain();

    /** Navigation areas are used to gather navigation geometry.
    
    NOTE: In the future, we can create a bit mask for each terrain quad to decide which triangles should be used for navigation.
    e.g. TBitMask<> WalkableMask
    */
    TVector<BvAxisAlignedBox> NavigationAreas;

    float GetMinHeight() const { return m_MinHeight; }

    float GetMaxHeight() const { return m_MaxHeight; }

    float ReadHeight(int X, int Z, int Lod) const;

    float SampleHeight(float X, float Z) const;

    Int2 const& GetClipMin() const { return m_ClipMin; }
    Int2 const& GetClipMax() const { return m_ClipMax; }

    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    /** Find ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TVector<TriangleHitResult>& HitResult) const;
    /** Find ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TriangleHitResult& HitResult) const;

    bool GetTriangleVertices(float X, float Z, Float3& V0, Float3& V1, Float3& V2) const;

    bool GetNormal(float X, float Z, Float3& Normal) const;

    bool GetTexcoord(float X, float Z, Float2& Texcoord) const;

    bool GetTriangle(float X, float Z, TerrainTriangle& Triangle) const;

    void GatherGeometry(BvAxisAlignedBox const& LocalBounds, TVector<Float3>& Vertices, TVector<unsigned int>& Indices) const;

    btHeightfieldTerrainShape* GetHeightfieldShape() const { return m_HeightfieldShape; }

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Terrain/Default"; }

private:
    void Purge();
    void GenerateLods();
    void UpdateTerrainBounds();
    void UpdateTerrainShape();
    void NotifyTerrainResourceUpdate(TERRAIN_UPDATE_FLAG UpdateFlag);

    int m_HeightmapResolution{};
    int m_HeightmapLods{};
    TVector<float*> m_Heightmap;
    float m_MinHeight{};
    float m_MaxHeight{};
    btHeightfieldTerrainShape* m_HeightfieldShape{};
    Int2 m_ClipMin{};
    Int2 m_ClipMax{};
    BvAxisAlignedBox m_BoundingBox;
};

HK_NAMESPACE_END
