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

#include <Engine/Image/Image.h>
#include <Engine/Math/Quat.h>

HK_NAMESPACE_BEGIN

struct AssetImportSettings
{
    AssetImportSettings()
    {
        bImportMeshes     = true;
        bImportMaterials  = true;
        bImportSkinning   = true;
        bImportSkeleton   = true;
        bImportAnimations = true;
        bImportTextures   = true;
        bSingleModel      = true;
        bMergePrimitives  = true;
        //bGenerateStaticCollisions = true;
        bGenerateRaycastBVH           = true;
        RaycastPrimitivesPerLeaf      = 16;
        bImportSkybox                 = false;
        bImportSkyboxExplicit         = false;
        Scale                         = 1.0f;
        Rotation                      = Quat::Identity();
        bCreateSkyboxMaterialInstance = true;
        bAllowUnlitMaterials          = true;
    }

    /** Source file name */
    String ImportFile;

    /** Asset output directory */
    String OutputPath;

    String RootPath;

    bool bImportMeshes;
    bool bImportMaterials;
    bool bImportSkinning;
    bool bImportSkeleton;
    bool bImportAnimations;
    bool bImportTextures;
    bool bImportSkybox;
    bool bImportSkyboxExplicit;

    /** Store result as single indexed mesh with subparts. Always true for skinned models. */
    bool bSingleModel;

    /** Merge primitives with same material */
    bool bMergePrimitives;

    /** Generate collision model for the meshes */
    //bool bGenerateStaticCollisions;

    /** Generate raycast AABB tree */
    bool bGenerateRaycastBVH;

    uint16_t RaycastPrimitivesPerLeaf;

    /** Import skybox material instance */
    bool bCreateSkyboxMaterialInstance;

    /** Allow to create unlit materials */
    bool bAllowUnlitMaterials;

    /** Scale units */
    float Scale;

    /** Rotate models */
    Quat Rotation;

    SkyboxImportSettings SkyboxImport;

    bool bHork2Format{};
};

bool ImportGLTF(AssetImportSettings const& Settings);
bool ImportOBJ(AssetImportSettings const& Settings);
bool ImportSkybox(AssetImportSettings const& Settings);

bool SaveSkyboxTexture(StringView FileName, ImageStorage const& Image);

HK_NAMESPACE_END
