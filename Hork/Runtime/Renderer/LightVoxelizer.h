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

#include <Hork/Runtime/World/DebugRenderer.h>

HK_NAMESPACE_BEGIN

enum ITEM_TYPE
{
    ITEM_TYPE_LIGHT,
    ITEM_TYPE_PROBE
};

struct Float4x4SSE
{
    __m128 col0;
    __m128 col1;
    __m128 col2;
    __m128 col3;

    Float4x4SSE()
    {
    }

    Float4x4SSE(__m128 _col0, __m128 _col1, __m128 _col2, __m128 _col3) :
        col0(_col0), col1(_col1), col2(_col2), col3(_col3)
    {
    }

    Float4x4SSE(Float4x4 const& m)
    {
        col0 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[0]));
        col1 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[1]));
        col2 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[2]));
        col3 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[3]));
    }

    HK_FORCEINLINE void operator=(Float4x4 const& m)
    {
        col0 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[0]));
        col1 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[1]));
        col2 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[2]));
        col3 = _mm_loadu_ps(reinterpret_cast<const float*>(&m[3]));
    }
};

struct ItemInfo
{
    int MinSlice;
    int MinClusterX;
    int MinClusterY;
    int MaxSlice;
    int MaxClusterX;
    int MaxClusterY;

    // OBB before transform (OBB transformed by OBBTransformInv)
    //Float3 AabbMins;
    //Float3 AabbMaxs;
    alignas(16) Float3 Mins;
    alignas(16) Float3 Maxs;
    Float4x4 ClipToBoxMat;

    // Same date for SSE
    //alignas(16) __m128 AabbMinsSSE;
    //alignas(16) __m128 AabbMaxsSSE;
    alignas(16) Float4x4SSE ClipToBoxMatSSE;

    int ListIndex;

    uint8_t Type;
};

class StreamedMemoryGPU;

class LightVoxelizer
{
public:
    void                        Reset();

    bool                        IsSSE() const { return m_UseSSE; };

    ItemInfo*                   AllocItem() { HK_ASSERT(m_ItemsCount < MAX_ITEMS); return &m_ItemInfos[m_ItemsCount++]; }

    void                        Voxelize(StreamedMemoryGPU* streamMemory, RenderViewData* view);

    void                        DrawVoxels(DebugRenderer& renderer, Float4x4 const& cameraViewMatrix, Float4x4 const& clusterProjectionMatrix);

private:
    static void                 sVoxelizeWork(void* data);

    void                        VoxelizeWork(int sliceIndex);

    void                        TransformItemsSSE();
    void                        TransformItemsGeneric();

    void                        GatherVoxelGeometry(Vector<Float3>& lineVertices, Float4x4 const& viewProjectionInversed);

    ItemInfo                    m_ItemInfos[MAX_ITEMS];
    int                         m_ItemsCount;

    unsigned short              m_Items[MAX_FRUSTUM_CLUSTERS_Z][MAX_FRUSTUM_CLUSTERS_Y][MAX_FRUSTUM_CLUSTERS_X][MAX_CLUSTER_ITEMS * 3]; // TODO: optimize size!!! 4 MB
    AtomicInt                   m_ItemCounter;
    Float4x4                    m_ViewProj;
    Float4x4                    m_ViewProjInv;

    struct FrustumCluster
    {
        unsigned short LightsCount;
        unsigned short DecalsCount;
        unsigned short ProbesCount;
    };

    alignas(16) FrustumCluster  m_ClusterData[MAX_FRUSTUM_CLUSTERS_Z][MAX_FRUSTUM_CLUSTERS_Y][MAX_FRUSTUM_CLUSTERS_X];

    ClusterHeader*              m_pClusterHeaderData;
    ClusterPackedIndex*         m_pClusterPackedIndices;

    Vector<Float3>              m_DebugDrawVertices;

    bool                        m_UseSSE;
};

HK_NAMESPACE_END
