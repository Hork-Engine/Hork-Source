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

#include "Resource.h"
#include "HitTest.h"
#include <Geometry/BV/BvAxisAlignedBox.h>

struct STerrainTriangle
{
    Float3 Vertices[3];
    Float3 Normal;
    Float2 Texcoord;
};

class ATerrainComponent;

class ATerrain : public AResource
{
    HK_CLASS(ATerrain, AResource)

public:
    ATerrain();
    ATerrain(int Resolution, const float* pData);

    ~ATerrain();

    /** Navigation areas are used to gather navigation geometry.
    
    NOTE: In the future, we can create a bit mask for each terrain quad to decide which triangles should be used for navigation.
    e.g. TBitMask<> WalkableMask
    */
    TPodVector<BvAxisAlignedBox> NavigationAreas;

    float GetMinHeight() const { return MinHeight; }

    float GetMaxHeight() const { return MaxHeight; }

    float ReadHeight(int X, int Z, int Lod) const;

    float SampleHeight(float X, float Z) const;

    Int2 const& GetClipMin() const { return ClipMin; }
    Int2 const& GetClipMax() const { return ClipMax; }

    BvAxisAlignedBox const& GetBoundingBox() const { return BoundingBox; }

    /** Find ray intersection. Result is unordered by distance to save performance */
    bool Raycast(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, TPodVector<STriangleHitResult>& HitResult) const;
    /** Find ray intersection */
    bool RaycastClosest(Float3 const& RayStart, Float3 const& RayDir, float Distance, bool bCullBackFace, STriangleHitResult& HitResult) const;

    bool GetTriangleVertices(float X, float Z, Float3& V0, Float3& V1, Float3& V2) const;

    bool GetNormal(float X, float Z, Float3& Normal) const;

    bool GetTexcoord(float X, float Z, Float2& Texcoord) const;

    bool GetTriangle(float X, float Z, STerrainTriangle& Triangle) const;

    void GatherGeometry(BvAxisAlignedBox const& LocalBounds, TPodVectorHeap<Float3>& Vertices, TPodVectorHeap<unsigned int>& Indices) const;

    class btHeightfieldTerrainShape* GetHeightfieldShape() const { return HeightfieldShape.GetObject(); }

    void AddListener(ATerrainComponent* Listener);
    void RemoveListener(ATerrainComponent* Listener);

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(const char* Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Terrain/Default"; }

private:
    void Purge();
    void GenerateLods();
    void UpdateTerrainBounds();
    void UpdateTerrainShape();
    void NotifyTerrainModified();

    int                                   HeightmapResolution{};
    int                                   HeightmapLods{};
    TPodVector<float*>                    Heightmap;
    float                                 MinHeight{};
    float                                 MaxHeight{};
    TUniqueRef<btHeightfieldTerrainShape> HeightfieldShape;
    Int2                                  ClipMin;
    Int2                                  ClipMax;
    BvAxisAlignedBox                      BoundingBox;

    // Terrain components that uses this resource
    ATerrainComponent* Listeners{};
    ATerrainComponent* ListenersTail{};
};
