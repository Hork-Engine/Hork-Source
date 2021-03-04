/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "Animation.h"
#include "Skeleton.h"
#include "IndexedMesh.h"

#include <unordered_map>

struct SAssetImportSettings
{
    SAssetImportSettings()
    {
        bImportMeshes = true;
        bImportMaterials = true;
        bImportSkinning = true;
        bImportSkeleton = true;
        bImportAnimations = true;
        bImportTextures = true;
        bSingleModel = true;
        bMergePrimitives = true;
        //bGenerateStaticCollisions = true;
        bGenerateRaycastBVH = true;
        RaycastPrimitivesPerLeaf = 16;
        bImportSkybox = false;
        bImportSkyboxExplicit = false;
        bSkyboxHDRI = false;
        Scale = 1.0f;
        Rotation = Quat::Identity();
        SkyboxHDRIScale = 1; // 4
        SkyboxHDRIPow = 1; // 1.1
        Core::ZeroMem( ExplicitSkyboxFaces, sizeof( ExplicitSkyboxFaces ) );
        bCreateSkyboxMaterialInstance = true;
        bAllowUnlitMaterials = true;
    }

    /** Source file name */
    AString ImportFile;

    /** Source files for skybox */
    const char * ExplicitSkyboxFaces[6];

    /** Asset output directory */
    AString OutputPath;

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

    /** Import skybox as HDRI image */
    bool bSkyboxHDRI;

    /** Import skybox material instance */
    bool bCreateSkyboxMaterialInstance;

    /** Allow to create unlit materials */
    bool bAllowUnlitMaterials;

    /** Scale units */
    float Scale;

    /** Rotate models */
    Quat Rotation;

    float SkyboxHDRIScale;
    float SkyboxHDRIPow;
};

class AAssetImporter {
public:
    bool ImportGLTF( SAssetImportSettings const & InSettings );
    bool ImportSkybox( SAssetImportSettings const & _Settings );

private:
    struct MeshInfo {
        AGUID GUID;
        int BaseVertex;
        int VertexCount;
        int FirstIndex;
        int IndexCount;
        struct cgltf_mesh * Mesh;
        struct cgltf_material * Material;
        BvAxisAlignedBox BoundingBox;
    };

    struct TextureInfo {
        AGUID GUID;
        bool bSRGB;
        struct cgltf_image * Image;
    };

    struct MaterialInfo {
        AGUID GUID;
        struct cgltf_material * Material;
        //class MGMaterialGraph * Graph;
        const char * DefaultMaterial;

        TextureInfo * Textures[MAX_MATERIAL_TEXTURES];
        int NumTextures;

        float Uniforms[16];

        const char * DefaultTexture[MAX_MATERIAL_TEXTURES];
    };

    struct AnimationInfo {
        AGUID GUID;
        AString Name;
        float FrameDelta;       // fixed time delta between frames
        uint32_t FrameCount;         // frames count, animation duration is FrameDelta * ( FrameCount - 1 )
        TPodArray< SAnimationChannel > Channels;
        TPodArray< STransform > Transforms;
        TPodArray< BvAxisAlignedBox > Bounds;
    };

    bool ReadGLTF( struct cgltf_data * Data );
    void ReadMaterial( struct cgltf_material * Material, MaterialInfo & Info );
    void ReadNode_r( struct cgltf_node * Node );
    void ReadMesh( struct cgltf_node * Node );
    void ReadMesh( struct cgltf_mesh * Mesh, Float3x4 const & GlobalTransform, Float3x3 const & NormalMatrix );
    void ReadAnimations( struct cgltf_data * Data );
    void ReadAnimation( struct cgltf_animation * Anim, AnimationInfo & _Animation );
    void ReadSkeleton( struct cgltf_node * node, int parentIndex = -1 );
    void WriteAssets();
    void WriteTextures();
    void WriteTexture( TextureInfo & tex );
    void WriteMaterials();
    void WriteMaterial( MaterialInfo const & m );
    void WriteSkeleton();
    void WriteAnimations();
    void WriteAnimation( AnimationInfo const & Animation );
    void WriteSingleModel();
    void WriteMeshes();
    void WriteMesh( MeshInfo const & Mesh );
    void WriteSkyboxMaterial( AGUID const & SkyboxTextureGUID );
    AString GeneratePhysicalPath( const char * DesiredName, const char * Extension );
    AString GetMaterialGUID( cgltf_material * Material );
    TextureInfo * FindTextureImage( struct cgltf_texture const * Texture );
    void SetTextureProps( TextureInfo * Info, const char * Name, bool SRGB );

    std::unordered_map< std::string, AString > GuidMap; // Guid -> file name

    SAssetImportSettings m_Settings;
    AString m_Path;
    struct cgltf_data * m_Data;
    bool m_bSkeletal;
    TPodArray< SMeshVertex > m_Vertices;
    TPodArray< SMeshVertexSkin > m_Weights;
    TPodArray< unsigned int > m_Indices;
    TPodArray< MeshInfo > m_Meshes;
    TPodArray< TextureInfo > m_Textures;
    TStdVector< MaterialInfo > m_Materials;
    TStdVector< AnimationInfo > m_Animations;
    TPodArray< SJoint > m_Joints;
    ASkin m_Skin;
    BvAxisAlignedBox m_BindposeBounds;
    AGUID m_SkeletonGUID;
};


bool LoadLWO( IBinaryStream & InStream, float InScale, AMaterialInstance * (*GetMaterial)( const char * _Name ), AIndexedMesh ** IndexedMesh );
