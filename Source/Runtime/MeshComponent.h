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

#include "Texture.h"
#include "Material.h"
#include "IndexedMesh.h"
#include "Drawable.h"

/**

MeshComponent

Mesh without skinning

*/
class MeshComponent : public Drawable,  private IndexedMeshListener
{
    HK_COMPONENT(MeshComponent, Drawable)

public:
    /** Lightmap atlas index */
    uint32_t LightmapBlock = 0;

    /** Lighmap channel UV offset and scale */
    Float4 LightmapOffset = Float4(0, 0, 1, 1);

    /** Baked vertex light channel */
    uint32_t VertexLightChannel = 0;

    bool bHasLightmap    = false;
    bool bHasVertexLight = false;

    /** Flipbook animation page offset */
    unsigned int SubpartBaseVertexOffset = 0;

    /** Allow raycasting */
    void SetAllowRaycast(bool bAllowRaycast) override;

    /** Set indexed mesh for the component */
    void SetMesh(IndexedMesh* mesh);

    /** Get indexed mesh. Never return null */
    IndexedMesh* GetMesh() const { return m_Mesh; }

    /** Set materials from mesh resource */
    void CopyMaterialsFromMeshResource();

    /** Unset materials */
    void ClearRenderViews();

    void SetRenderView(MeshRenderView* renderView);
    void AddRenderView(MeshRenderView* renderView);
    void RemoveRenderView(MeshRenderView* renderView);

    using MeshRenderViews = TSmallVector<MeshRenderView*, 1>;

    MeshRenderViews const& GetRenderViews() const { return m_Views; }

    BvAxisAlignedBox GetSubpartWorldBounds(int subpartIndex) const;

    Float3x4 const& GetRenderTransformMatrix(int frameNum) const
    {
        return m_RenderTransformMatrix[frameNum & 1];
    }

protected:
    MeshComponent();
    ~MeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    CollisionModel* GetMeshCollisionModel() const override;

    void DrawDebug(DebugRenderer* InRenderer) override;

    virtual void OnMeshChanged() {}

    void OnPreRenderUpdate(RenderFrontendDef const* _Def) override;

private:
    void NotifyMeshChanged();
    void OnMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag) override;

    TRef<IndexedMesh> m_Mesh;
    MeshRenderViews    m_Views;

    /** Transform matrix used during last rendering */
    Float3x4 m_RenderTransformMatrix[2];
    int      m_RenderTransformMatrixFrame{0};
};

class ProceduralMeshComponent : public Drawable
{
    HK_COMPONENT(ProceduralMeshComponent, Drawable)

public:
    /** Allow raycasting */
    void SetAllowRaycast(bool bAllowRaycast) override;

    void SetMesh(ProceduralMesh* mesh);
    ProceduralMesh* GetMesh() const;

    /** Unset materials */
    void ClearRenderViews();

    void SetRenderView(MeshRenderView* renderView);
    void AddRenderView(MeshRenderView* renderView);
    void RemoveRenderView(MeshRenderView* renderView);

    using MeshRenderViews = TSmallVector<MeshRenderView*, 1>;

    MeshRenderViews const& GetRenderViews() const { return m_Views; }

    Float3x4 const& GetRenderTransformMatrix(int frameNum) const
    {
        return m_RenderTransformMatrix[frameNum & 1];
    }

protected:
    ProceduralMeshComponent();
    ~ProceduralMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void OnPreRenderUpdate(RenderFrontendDef const* _Def) override;

    void DrawDebug(DebugRenderer* InRenderer) override;

    TRef<ProceduralMesh> m_Mesh;
    MeshRenderViews      m_Views;

    /** Transform matrix used during last rendering */
    Float3x4 m_RenderTransformMatrix[2];
    int      m_RenderTransformMatrixFrame{0};
};
