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
//#include "ResourceManager.h"
#include "Drawable.h"

class ACameraComponent;

/**

AMeshComponent

Mesh component without skinning

*/
class AMeshComponent : public ADrawable,  private AIndexedMeshListener
{
    HK_COMPONENT(AMeshComponent, ADrawable)

public:
    /** Lightmap atlas index */
    int LightmapBlock = 0;

    /** Lighmap channel UV offset and scale */
    Float4 LightmapOffset = Float4(0, 0, 1, 1);

    /** Lightmap UV channel */
    TRef<ALightmapUV> LightmapUVChannel;

    /** Baked vertex light channel */
    TRef<AVertexLight> VertexLightChannel;

    /** Flipbook animation page offset */
    unsigned int SubpartBaseVertexOffset = 0;

    bool bOverrideMeshMaterials = true;

    /** Transform matrix used during last rendering */
    Float3x4 RenderTransformMatrix;

    /** Allow raycasting */
    void SetAllowRaycast(bool _AllowRaycast) override;

    /** Set indexed mesh for the component */
    void SetMesh(AIndexedMesh* _Mesh);

    /** Get indexed mesh. Never return null */
    AIndexedMesh* GetMesh() const { return Mesh; }

    /** Unset materials */
    void ClearMaterials();

    /** Set materials from mesh resource */
    void CopyMaterialsFromMeshResource();

    /** Set material instance for subpart of the mesh */
    void SetMaterialInstance(int _SubpartIndex, AMaterialInstance* _Instance);

    /** Get material instance of subpart of the mesh. Never return null. */
    AMaterialInstance* GetMaterialInstance(int _SubpartIndex) const;

    /** Set material instance for subpart of the mesh */
    void SetMaterialInstance(AMaterialInstance* _Instance) { SetMaterialInstance(0, _Instance); }

    /** Get material instance of subpart of the mesh. Never return null. */
    AMaterialInstance* GetMaterialInstance() const { return GetMaterialInstance(0); }

    BvAxisAlignedBox GetSubpartWorldBounds(int _SubpartIndex) const;

protected:
    AMeshComponent();
    ~AMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    ACollisionModel* GetMeshCollisionModel() const override;

    void DrawDebug(ADebugRenderer* InRenderer) override;

    virtual void OnMeshChanged() {}

private:
    void NotifyMeshChanged();
    void OnMeshResourceUpdate(INDEXED_MESH_UPDATE_FLAG UpdateFlag) override;

    AMaterialInstance* GetMaterialInstanceUnsafe(int _SubpartIndex) const;

    TRef<AIndexedMesh>          Mesh;
    TVector<AMaterialInstance*> Materials;
};

class AProceduralMeshComponent : public ADrawable
{
    HK_COMPONENT(AProceduralMeshComponent, ADrawable)

public:
    /** Transform matrix used during last rendering */
    Float3x4 RenderTransformMatrix;

    /** Allow raycasting */
    void SetAllowRaycast(bool _AllowRaycast) override;

    void SetMesh(AProceduralMesh* _Mesh)
    {
        ProceduralMesh = _Mesh;
    }

    AProceduralMesh* GetMesh() const
    {
        return ProceduralMesh;
    }

    void SetMaterialInstance(AMaterialInstance* _MaterialInstance)
    {
        MaterialInstance = _MaterialInstance;
    }

    AMaterialInstance* GetMaterialInstance() const;

protected:
    AProceduralMeshComponent();

    void InitializeComponent() override;
    void DeinitializeComponent() override;

    void DrawDebug(ADebugRenderer* InRenderer) override;

    TRef<AProceduralMesh>   ProceduralMesh;
    TRef<AMaterialInstance> MaterialInstance;
};
