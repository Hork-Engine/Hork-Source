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

#include "AssetImporter.h"
#include "Asset.h"

#include <Engine/Core/Platform/Logger.h>
#include <Engine/Core/Platform/Memory/LinearAllocator.h>
#include <Engine/Core/Containers/Hash.h>
#include <Engine/Geometry/Transform.h>
#include <Engine/Geometry/Skinning.h>
#include <Engine/Geometry/VertexFormat.h>
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>
#include <Engine/Geometry/TangentSpace.h>
#include <Engine/Geometry/BV/BvhTree.h>
#include <Engine/Image/ImageEncoders.h>
#include <Engine/Core/HashFunc.h>

#include <cgltf/cgltf.h>
#include <fast_obj/fast_obj.h>

HK_NAMESPACE_BEGIN

class AssetImporter
{
public:
    bool ImportGLTF(AssetImportSettings const& Settings);
    bool ImportOBJ(AssetImportSettings const& Settings);
    bool ImportSkybox(AssetImportSettings const& Settings);

private:
    struct MeshInfo
    {
        int              BaseVertex{};
        int              VertexCount{};
        int              FirstIndex{};
        int              IndexCount{};
        String           UniqueName;
        cgltf_node*      NodeGltf{};
        int              MaterialNum{};
        BvAxisAlignedBox BoundingBox;
        bool             bSkinned{};
    };

    struct TextureInfo
    {
        String Name;
        String Path;
        String PathToWrite;
        bool   bSRGB{};
    };

    struct MaterialInfo
    {
        MaterialInfo()
        {
            Platform::ZeroMem(Uniforms, sizeof(Uniforms));
        }

        String                          PathToWrite;
        const char*                     DefaultMaterial{""};
        TVector<TextureInfo*>           Textures;
        float                           Uniforms[16];
        THashMap<uint32_t, const char*> DefaultTexture;
    };

    struct AnimationInfo
    {
        String                    Name;
        float                     FrameDelta; // fixed time delta between frames
        uint32_t                  FrameCount; // frames count, animation duration is FrameDelta * ( FrameCount - 1 )
        TVector<AnimationChannel> Channels;
        TVector<Transform>        Transforms;
        TVector<BvAxisAlignedBox> Bounds;
    };

    bool         ReadGLTF(cgltf_data* Data);
    void         ReadMaterial(cgltf_material* Material);
    void         ReadNode_r(cgltf_node* Node);
    void         ReadMesh(cgltf_node* Node);
    void         ReadMesh(cgltf_node* Node, cgltf_mesh* Mesh, Float3x4 const& GlobalTransform, Float3x3 const& NormalMatrix);
    void         ReadAnimations(cgltf_data* Data);
    void         ReadAnimation(cgltf_animation* Anim, AnimationInfo& _Animation);
    void         ReadSkeleton(cgltf_node* node, int parentIndex = -1);
    bool         ReadOBJ(fastObjMesh* pMesh);
    void         WriteAssets();
    void         WriteTextures();
    void         WriteTexture(TextureInfo& tex);
    void         WriteMaterials();
    void         WriteMaterial(MaterialInfo& m);
    void         WriteSkeleton();
    void         WriteAnimations();
    void         WriteAnimation(AnimationInfo const& Animation);
    void         WriteSingleModel();
    void         WriteMeshes();
    void         WriteMesh(MeshInfo const& Mesh);
    void         WriteSkyboxMaterial(StringView SkyboxTexture);
    String       GeneratePhysicalPath(StringView DesiredName, StringView Extension);
    int          MapGltfMaterial(cgltf_material* Material);
    TextureInfo* FindTextureImageGltf(cgltf_texture const* Texture);
    void         SetTextureProps(TextureInfo* Info, const char* Name, bool SRGB);

    AssetImportSettings     m_Settings;
    String                  m_Path;
    cgltf_data*             m_Data;
    bool                    m_bSkeletal;
    TVector<MeshVertex>     m_Vertices;
    TVector<MeshVertexSkin> m_Weights;
    TVector<unsigned int>   m_Indices;
    TVector<MeshInfo>       m_Meshes;
    TVector<TextureInfo>    m_Textures;
    TVector<MaterialInfo>   m_Materials;
    TVector<AnimationInfo>  m_Animations;
    TVector<SkeletonJoint>  m_Joints;
    MeshSkin                m_Skin;
    BvAxisAlignedBox        m_BindposeBounds;
    String                  m_SkeletonPath;
};

bool ImportGLTF(AssetImportSettings const& Settings)
{
    AssetImporter importer;
    return importer.ImportGLTF(Settings);
}

bool ImportOBJ(AssetImportSettings const& Settings)
{
    AssetImporter importer;
    return importer.ImportOBJ(Settings);
}

bool ImportSkybox(AssetImportSettings const& Settings)
{
    AssetImporter importer;
    return importer.ImportSkybox(Settings);
}

static void unpack_vec2_or_vec3(cgltf_accessor* acc, Float3* output, size_t stride)
{
    int   num_elements;
    float position[3];

    if (!acc)
    {
        return;
    }

    if (acc->type == cgltf_type_vec2)
    {
        num_elements = 2;
    }
    else if (acc->type == cgltf_type_vec3)
    {
        num_elements = 3;
    }
    else
    {
        return;
    }

    position[2] = 0;

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, position, num_elements);

        Platform::Memcpy(ptr, position, sizeof(float) * 3);

        ptr += stride;
    }
}

static void unpack_vec2_or_vec3_to_half3(cgltf_accessor* acc, Half* output, size_t stride, bool normalize)
{
    int    num_elements;
    Float3 tmp;

    if (!acc)
    {
        return;
    }

    if (acc->type == cgltf_type_vec2)
    {
        num_elements = 2;
    }
    else if (acc->type == cgltf_type_vec3)
    {
        num_elements = 3;
    }
    else
    {
        return;
    }

    tmp[2] = 0;

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, tmp.ToPtr(), num_elements);

        if (normalize)
        {
            tmp.NormalizeSelf();
        }

        ((Half*)ptr)[0] = tmp[0];
        ((Half*)ptr)[1] = tmp[1];
        ((Half*)ptr)[2] = tmp[2];

        ptr += stride;
    }
}

static void unpack_vec2(cgltf_accessor* acc, Float2* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_vec2)
    {
        return;
    }

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, (float*)ptr, 2);

        ptr += stride;
    }
}

static void unpack_vec2_to_half2(cgltf_accessor* acc, Half* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_vec2)
    {
        return;
    }

    byte* ptr = (byte*)output;
    float tmp[2];

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, tmp, 2);

        ((Half*)ptr)[0] = tmp[0];
        ((Half*)ptr)[1] = tmp[1];

        ptr += stride;
    }
}

static void unpack_vec3(cgltf_accessor* acc, Float3* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_vec3)
    {
        return;
    }

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, (float*)ptr, 3);

        ptr += stride;
    }
}

static void unpack_vec4(cgltf_accessor* acc, Float4* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_vec4)
    {
        return;
    }

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, (float*)ptr, 4);

        ptr += stride;
    }
}

static void unpack_tangents(cgltf_accessor* acc, MeshVertex* output)
{
    if (!acc || acc->type != cgltf_type_vec4)
    {
        return;
    }

    Float4 tmp;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, tmp.ToPtr(), 4);

        output->SetTangent(tmp.X, tmp.Y, tmp.Z);
        output->Handedness = tmp.W > 0.0f ? 1 : -1;

        output++;
    }
}

static void unpack_quat(cgltf_accessor* acc, Quat* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_vec4)
    {
        return;
    }

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, (float*)ptr, 4);

        ptr += stride;
    }
}

static void unpack_mat4(cgltf_accessor* acc, Float4x4* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_mat4)
    {
        return;
    }

    byte* ptr = (byte*)output;

    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, (float*)ptr, 16);

        ptr += stride;
    }
}

static void unpack_mat4_to_mat3x4(cgltf_accessor* acc, Float3x4* output, size_t stride)
{
    if (!acc || acc->type != cgltf_type_mat4)
    {
        return;
    }

    byte* ptr = (byte*)output;

    Float4x4 temp;
    for (int i = 0; i < acc->count; i++)
    {
        cgltf_accessor_read_float(acc, i, (float*)temp.ToPtr(), 16);

        Platform::Memcpy(ptr, temp.Transposed().ToPtr(), sizeof(Float3x4));

        ptr += stride;
    }
}

static void unpack_weights(cgltf_accessor* acc, MeshVertexSkin* weights)
{
    float weight[4];

    if (!acc || acc->type != cgltf_type_vec4)
    {
        return;
    }

    for (int i = 0; i < acc->count; i++, weights++)
    {
        cgltf_accessor_read_float(acc, i, weight, 4);

        const float invSum = 255.0f / (weight[0] + weight[1] + weight[2] + weight[3]);

        weights->JointWeights[0] = Math::Clamp<int>(weight[0] * invSum, 0, 255);
        weights->JointWeights[1] = Math::Clamp<int>(weight[1] * invSum, 0, 255);
        weights->JointWeights[2] = Math::Clamp<int>(weight[2] * invSum, 0, 255);
        weights->JointWeights[3] = Math::Clamp<int>(weight[3] * invSum, 0, 255);
    }
}

static void unpack_joints(cgltf_accessor* acc, MeshVertexSkin* weights)
{
    float indices[4];

    if (!acc || acc->type != cgltf_type_vec4)
    {
        return;
    }

    for (int i = 0; i < acc->count; i++, weights++)
    {
        cgltf_accessor_read_float(acc, i, indices, 4);

        weights->JointIndices[0] = Math::Clamp<int>(indices[0], 0, MAX_SKELETON_JOINTS);
        weights->JointIndices[1] = Math::Clamp<int>(indices[1], 0, MAX_SKELETON_JOINTS);
        weights->JointIndices[2] = Math::Clamp<int>(indices[2], 0, MAX_SKELETON_JOINTS);
        weights->JointIndices[3] = Math::Clamp<int>(indices[3], 0, MAX_SKELETON_JOINTS);
    }
}

static void sample_vec3(cgltf_animation_sampler* sampler, float frameTime, Float3& vec)
{
    cgltf_accessor* animtimes = sampler->input;
    cgltf_accessor* animdata  = sampler->output;

    float ft0, ftN, ct, nt;

    HK_ASSERT(animtimes->count > 0);

    cgltf_accessor_read_float(animtimes, 0, &ft0, 1);

    if (animtimes->count == 1 || frameTime <= ft0)
    {

        if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
        {
            cgltf_accessor_read_float(animdata, 0 * 3 + 1, (float*)vec.ToPtr(), 3);
        }
        else
        {
            cgltf_accessor_read_float(animdata, 0, (float*)vec.ToPtr(), 3);
        }

        return;
    }

    cgltf_accessor_read_float(animtimes, animtimes->count - 1, &ftN, 1);

    if (frameTime >= ftN)
    {

        if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
        {
            cgltf_accessor_read_float(animdata, (animtimes->count - 1) * 3 + 1, (float*)vec.ToPtr(), 3);
        }
        else
        {
            cgltf_accessor_read_float(animdata, animtimes->count - 1, (float*)vec.ToPtr(), 3);
        }

        return;
    }

    ct = ft0;

    for (int t = 0; t < (int)animtimes->count - 1; t++, ct = nt)
    {

        cgltf_accessor_read_float(animtimes, t + 1, &nt, 1);

        if (ct <= frameTime && nt > frameTime)
        {
            if (sampler->interpolation == cgltf_interpolation_type_linear)
            {
                if (frameTime == ct)
                {
                    cgltf_accessor_read_float(animdata, t, (float*)vec.ToPtr(), 3);
                }
                else
                {
                    Float3 p0, p1;
                    cgltf_accessor_read_float(animdata, t, (float*)p0.ToPtr(), 3);
                    cgltf_accessor_read_float(animdata, t + 1, (float*)p1.ToPtr(), 3);
                    float dur   = nt - ct;
                    float fract = (frameTime - ct) / dur;
                    HK_ASSERT(fract >= 0.0f && fract <= 1.0f);
                    vec = Math::Lerp(p0, p1, fract);
                }
            }
            else if (sampler->interpolation == cgltf_interpolation_type_step)
            {
                cgltf_accessor_read_float(animdata, t, (float*)vec.ToPtr(), 3);
            }
            else if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
            {
                float dur   = nt - ct;
                float fract = (dur == 0.0f) ? 0.0f : (frameTime - ct) / dur;
                HK_ASSERT(fract >= 0.0f && fract <= 1.0f);

                Float3 p0, m0, m1, p1;

                cgltf_accessor_read_float(animdata, t * 3 + 1, (float*)p0.ToPtr(), 3);
                cgltf_accessor_read_float(animdata, t * 3 + 2, (float*)m0.ToPtr(), 3);
                cgltf_accessor_read_float(animdata, (t + 1) * 3, (float*)m1.ToPtr(), 3);
                cgltf_accessor_read_float(animdata, (t + 1) * 3 + 1, (float*)p1.ToPtr(), 3);

                m0 *= dur;
                m1 *= dur;

                vec = Math::HermiteCubicSpline(p0, m0, p1, m1, fract);
            }
            break;
        }
    }
}

static void sample_quat(cgltf_animation_sampler* sampler, float frameTime, Quat& q)
{
    cgltf_accessor* animtimes = sampler->input;
    cgltf_accessor* animdata  = sampler->output;

    float ft0, ftN, ct, nt;

    HK_ASSERT(animtimes->count > 0);

    cgltf_accessor_read_float(animtimes, 0, &ft0, 1);

    if (animtimes->count == 1 || frameTime <= ft0)
    {

        if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
        {
            cgltf_accessor_read_float(animdata, 0 * 3 + 1, (float*)q.ToPtr(), 4);
        }
        else
        {
            cgltf_accessor_read_float(animdata, 0, (float*)q.ToPtr(), 4);
        }

        return;
    }

    cgltf_accessor_read_float(animtimes, animtimes->count - 1, &ftN, 1);

    if (frameTime >= ftN)
    {

        if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
        {
            cgltf_accessor_read_float(animdata, (animtimes->count - 1) * 3 + 1, (float*)q.ToPtr(), 4);
        }
        else
        {
            cgltf_accessor_read_float(animdata, animtimes->count - 1, (float*)q.ToPtr(), 4);
        }

        return;
    }

    ct = ft0;

    for (int t = 0; t < (int)animtimes->count - 1; t++, ct = nt)
    {

        cgltf_accessor_read_float(animtimes, t + 1, &nt, 1);

        if (ct <= frameTime && nt > frameTime)
        {
            if (sampler->interpolation == cgltf_interpolation_type_linear)
            {
                if (frameTime == ct)
                {
                    cgltf_accessor_read_float(animdata, t, (float*)q.ToPtr(), 4);
                }
                else
                {
                    Quat p0, p1;
                    cgltf_accessor_read_float(animdata, t, (float*)p0.ToPtr(), 4);
                    cgltf_accessor_read_float(animdata, t + 1, (float*)p1.ToPtr(), 4);
                    float dur   = nt - ct;
                    float fract = (frameTime - ct) / dur;
                    HK_ASSERT(fract >= 0.0f && fract <= 1.0f);
                    q = Math::Slerp(p0, p1, fract).Normalized();
                }
            }
            else if (sampler->interpolation == cgltf_interpolation_type_step)
            {
                cgltf_accessor_read_float(animdata, t, (float*)q.ToPtr(), 4);
            }
            else if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
            {
                float dur   = nt - ct;
                float fract = (dur == 0.0f) ? 0.0f : (frameTime - ct) / dur;
                HK_ASSERT(fract >= 0.0f && fract <= 1.0f);

                Quat p0, m0, m1, p1;

                cgltf_accessor_read_float(animdata, t * 3 + 1, (float*)p0.ToPtr(), 4);
                cgltf_accessor_read_float(animdata, t * 3 + 2, (float*)m0.ToPtr(), 4);
                cgltf_accessor_read_float(animdata, (t + 1) * 3, (float*)m1.ToPtr(), 4);
                cgltf_accessor_read_float(animdata, (t + 1) * 3 + 1, (float*)p1.ToPtr(), 4);

                m0 *= dur;
                m1 *= dur;

                p0.NormalizeSelf();
                m0.NormalizeSelf();
                m1.NormalizeSelf();
                p1.NormalizeSelf();

                q = Math::HermiteCubicSpline(p0, m0, p1, m1, fract);
                q.NormalizeSelf();
            }
            break;
        }
    }
}

static const char* GetErrorString(cgltf_result code)
{
    switch (code)
    {
        case cgltf_result_success:
            return "No error";
        case cgltf_result_data_too_short:
            return "Data too short";
        case cgltf_result_unknown_format:
            return "Unknown format";
        case cgltf_result_invalid_json:
            return "Invalid json";
        case cgltf_result_invalid_gltf:
            return "Invalid gltf";
        case cgltf_result_invalid_options:
            return "Invalid options";
        case cgltf_result_file_not_found:
            return "File not found";
        case cgltf_result_io_error:
            return "IO error";
        case cgltf_result_out_of_memory:
            return "Out of memory";
        default:;
    }

    return "Unknown error";
}

static bool IsChannelValid(cgltf_animation_channel* channel)
{
    cgltf_animation_sampler* sampler = channel->sampler;

    switch (channel->target_path)
    {
        case cgltf_animation_path_type_translation:
        case cgltf_animation_path_type_rotation:
        case cgltf_animation_path_type_scale:
            break;
        case cgltf_animation_path_type_weights:
            LOG("Warning: animation path weights is not supported yet\n");
            return false;
        default:
            LOG("Warning: unknown animation target path\n");
            return false;
    }

    switch (sampler->interpolation)
    {
        case cgltf_interpolation_type_linear:
        case cgltf_interpolation_type_step:
        case cgltf_interpolation_type_cubic_spline:
            break;
        default:
            LOG("Warning: unknown interpolation type\n");
            return false;
    }

    cgltf_accessor* animtimes = sampler->input;
    cgltf_accessor* animdata  = sampler->output;

    if (animtimes->count == 0)
    {
        LOG("Warning: empty channel data\n");
        return false;
    }

    if (sampler->interpolation == cgltf_interpolation_type_cubic_spline && animtimes->count != animdata->count * 3)
    {
        LOG("Warning: invalid channel data\n");
        return false;
    }
    else if (animtimes->count != animdata->count)
    {
        LOG("Warning: invalid channel data\n");
        return false;
    }

    return true;
}

bool AssetImporter::ImportGLTF(AssetImportSettings const& Settings)
{
    bool           ret    = false;
    String const& Source = Settings.ImportFile;

    m_Settings = Settings;

    m_Path = PathUtils::GetFilePath(Settings.ImportFile);
    m_Path += "/";

    File f = File::OpenRead(Source);
    if (!f)
    {
        LOG("Couldn't open {}\n", Source);
        return false;
    }

    HeapBlob blob = f.AsBlob();

    constexpr int MAX_MEMORY_GLTF = 16 << 20;
    using ALinearAllocatorGLTF    = TLinearAllocator<MAX_MEMORY_GLTF>;

    ALinearAllocatorGLTF allocator;

    cgltf_options options;

    Platform::ZeroMem(&options, sizeof(options));

    options.memory.alloc = [](void* user, cgltf_size size)
    {
        ALinearAllocatorGLTF& Allocator = *static_cast<ALinearAllocatorGLTF*>(user);
        return Allocator.Allocate(size);
    };
    options.memory.free = [](void* user, void* ptr) {
    };
    options.memory.user_data = &allocator;

    cgltf_data* data = NULL;

    cgltf_result result = cgltf_parse(&options, blob.GetData(), blob.Size(), &data);
    if (result != cgltf_result_success)
    {
        LOG("Couldn't load {} : {}\n", Source, GetErrorString(result));
        return false;
    }

    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        LOG("Couldn't load {} : {}\n", Source, GetErrorString(result));
        return false;
    }

    result = cgltf_load_buffers(&options, data, m_Path.CStr());
    if (result != cgltf_result_success)
    {
        LOG("Couldn't load {} buffers : {}\n", Source, GetErrorString(result));
        return false;
    }

    ret = ReadGLTF(data);

    WriteAssets();

    return true;
}

void AssetImporter::ReadSkeleton(cgltf_node* node, int parentIndex)
{
    SkeletonJoint&  joint = m_Joints.Add();
    Float4x4 localTransform;

    cgltf_node_transform_local(node, (float*)localTransform.ToPtr());
    joint.LocalTransform = Float3x4(localTransform.Transposed());

    if (node->name)
    {
        Platform::Strcpy(joint.Name, sizeof(joint.Name), node->name);
    }
    else
    {
        String name = HK_FORMAT("unnamed_{}", m_Joints.Size() - 1);
        Platform::Strcpy(joint.Name, sizeof(joint.Name), name.CStr());
    }

    LOG("ReadSkeleton: {}\n", node->name ? node->name : "unnamed");

    joint.Parent = parentIndex;

    // HACK: store joint index at camera pointer
    node->camera = (cgltf_camera*)(size_t)m_Joints.Size();

    parentIndex = m_Joints.Size() - 1;

    for (int i = 0; i < node->children_count; i++)
    {
        ReadSkeleton(node->children[i], parentIndex);
    }
}

bool AssetImporter::ReadGLTF(cgltf_data* Data)
{
    m_Data      = Data;
    m_bSkeletal = Data->skins_count > 0 && m_Settings.bImportSkinning;

    m_BindposeBounds.Clear();

    LOG("{} scenes\n", Data->scenes_count);
    LOG("{} skins\n", Data->skins_count);
    LOG("{} meshes\n", Data->meshes_count);
    LOG("{} nodes\n", Data->nodes_count);
    LOG("{} cameras\n", Data->cameras_count);
    LOG("{} lights\n", Data->lights_count);
    LOG("{} materials\n", Data->materials_count);

    if (Data->extensions_used_count > 0)
    {
        LOG("Used extensions:\n");
        for (int i = 0; i < Data->extensions_used_count; i++)
        {
            LOG("    {}\n", Data->extensions_used[i]);
        }
    }

    if (Data->extensions_required_count > 0)
    {
        LOG("Required extensions:\n");
        for (int i = 0; i < Data->extensions_required_count; i++)
        {
            LOG("    {}\n", Data->extensions_required[i]);
        }
    }

    if (m_Settings.bImportTextures)
    {
        m_Textures.Resize(Data->images_count);
        for (int i = 0; i < Data->images_count; i++)
        {
            if (Data->images[i].name)
                m_Textures[i].Name = Data->images[i].name;

            m_Textures[i].Path = Data->images[i].uri;
        }
    }

    if (m_Settings.bImportMaterials)
    {
        m_Materials.Reserve(Data->materials_count);
        for (int i = 0; i < Data->materials_count; i++)
        {
            ReadMaterial(&Data->materials[i]);
        }
    }

    for (int i = 0; i < Data->scenes_count; i++)
    {
        cgltf_scene* scene = &Data->scene[i];

        LOG("Scene \"{}\" nodes {}\n", scene->name ? scene->name : "unnamed", scene->nodes_count);

        for (int n = 0; n < scene->nodes_count; n++)
        {
            cgltf_node* node = scene->nodes[n];

            ReadNode_r(node);
        }
    }

    if (m_bSkeletal)
    {
        if (Data->skins)
        {
            // FIXME: Only one skin per file supported now
            // TODO: for ( int i = 0; i < Data->skins_count; i++ ) {
            cgltf_skin* skin = &Data->skins[0];

            m_Joints.Clear();
#if 0
            ReadSkeleton( skin->skeleton );
#else

            int rootsCount = 0;
            for (int n = 0; n < Data->nodes_count; n++)
            {
                if (!Data->nodes[n].parent)
                {
                    rootsCount++;
                }
            }

            int parentIndex = -1;

            if (rootsCount > 1)
            {
                // Add root node

                SkeletonJoint& joint = m_Joints.Add();

                joint.LocalTransform.SetIdentity();
                Platform::Strcpy(joint.Name, sizeof(joint.Name), "generated_root");

                joint.Parent = -1;

                parentIndex = 0;
            }

            for (int n = 0; n < Data->nodes_count; n++)
            {
                if (!Data->nodes[n].parent)
                {
                    ReadSkeleton(&Data->nodes[n], parentIndex);
                }
            }
#endif

            // Apply scaling by changing local joint position
            if (m_Settings.Scale != 1.0f)
            {
                Float3   transl, scale;
                Float3x3 rot;
                //Float3x4 scaleMatrix = Float3x4::Scale( Float3( m_Settings.Scale ) );
                for (int i = 0; i < m_Joints.Size(); i++)
                {
                    SkeletonJoint* joint = &m_Joints[i];

                    // Scale skeleton joints
                    joint->LocalTransform.DecomposeAll(transl, rot, scale);
                    joint->LocalTransform.Compose(transl * m_Settings.Scale, rot, scale);
                }
            }

            // Apply rotation to root node
            if (!m_Joints.IsEmpty())
            {
                Float3x4 rotation     = Float3x4(m_Settings.Rotation.ToMatrix3x3().Transposed());
                SkeletonJoint*  joint        = &m_Joints[0];
                joint->LocalTransform = rotation * joint->LocalTransform;
            }

            // Read skin
            m_Skin.JointIndices.Resize(m_Joints.Size());
            m_Skin.OffsetMatrices.Resize(m_Joints.Size());

            unpack_mat4_to_mat3x4(skin->inverse_bind_matrices, m_Skin.OffsetMatrices.ToPtr(), sizeof(m_Skin.OffsetMatrices[0]));

            Float3x4 scaleMatrix = Float3x4::Scale(Float3(m_Settings.Scale));

            Float3x4 rotationInverse = Float3x4(m_Settings.Rotation.ToMatrix3x3().Inversed().Transposed());

            for (int i = 0; i < skin->joints_count; i++)
            {
                cgltf_node* jointNode = skin->joints[i];

                // Scale offset matrix
                m_Skin.OffsetMatrices[i] = scaleMatrix * m_Skin.OffsetMatrices[i] * scaleMatrix.Inversed() * rotationInverse;

                // Map skin onto joints
                m_Skin.JointIndices[i] = -1;

                // HACK: get joint index from camera pointer
                int nodeIndex = jointNode->camera ? (size_t)jointNode->camera - 1 : m_Joints.Size();
                if (nodeIndex >= m_Joints.Size())
                {
                    LOG("Invalid skin\n");
                    m_Skin.JointIndices[i] = 0;
                }
                else
                {
                    m_Skin.JointIndices[i] = nodeIndex;
                }
            }

            for (int i = skin->joints_count; i < m_Joints.Size(); i++)
            {
                m_Skin.OffsetMatrices[i].SetIdentity();

                // Scale offset matrix
                m_Skin.OffsetMatrices[i] = scaleMatrix * m_Skin.OffsetMatrices[i] * scaleMatrix.Inversed() * rotationInverse;

                // Map skin onto joints
                m_Skin.JointIndices[i] = i;
            }

            for (auto& mesh : m_Meshes)
            {
                if (!mesh.bSkinned)
                {
                    int nodeIndex = mesh.NodeGltf->camera ? (size_t)mesh.NodeGltf->camera - 1 : 0;

                    for (int n = 0; n < mesh.VertexCount; n++)
                    {
                        m_Weights[mesh.BaseVertex + n].JointIndices[0] = nodeIndex;
                        m_Weights[mesh.BaseVertex + n].JointIndices[1] = 0;
                        m_Weights[mesh.BaseVertex + n].JointIndices[2] = 0;
                        m_Weights[mesh.BaseVertex + n].JointIndices[3] = 0;
                        m_Weights[mesh.BaseVertex + n].JointWeights[0] = 255;
                        m_Weights[mesh.BaseVertex + n].JointWeights[1] = 0;
                        m_Weights[mesh.BaseVertex + n].JointWeights[2] = 0;
                        m_Weights[mesh.BaseVertex + n].JointWeights[3] = 0;
                    }
                }
            }

            m_BindposeBounds = Geometry::CalcBindposeBounds(m_Vertices.ToPtr(), m_Weights.ToPtr(), m_Vertices.Size(), &m_Skin, m_Joints.ToPtr(), m_Joints.Size());

            LOG("Total skeleton nodes {}\n", m_Joints.Size());
            LOG("Total skinned nodes {}\n", m_Skin.JointIndices.Size());
        }

        if (!m_Joints.IsEmpty() && m_Settings.bImportAnimations)
        {
            ReadAnimations(Data);
        }
    }

    return true;
}

#if 0
#    include <World/Public/MaterialGraph/MaterialGraph.h>

static TEXTURE_FILTER ChooseSamplerFilter( int MinFilter, int MagFilter ) {
    // TODO:...

    return TEXTURE_FILTER_MIPMAP_TRILINEAR;
}

static TEXTURE_ADDRESS ChooseSamplerWrap( int Wrap ) {
    // TODO:...

    return TEXTURE_ADDRESS_WRAP;
}
#endif

AssetImporter::TextureInfo* AssetImporter::FindTextureImageGltf(cgltf_texture const* Texture)
{
    if (!Texture)
        return nullptr;
    for (int i = 0; i < m_Data->images_count; i++)
        if (&m_Data->images[i] == Texture->image)
            return &m_Textures[i];
    return nullptr;
}

void AssetImporter::SetTextureProps(TextureInfo* Info, const char* Name, bool SRGB)
{
    if (Info)
    {
        Info->bSRGB = SRGB;

        if (Info->Name.IsEmpty())
            Info->Name = Name;
    }
}

void AssetImporter::ReadMaterial(cgltf_material* Material)
{
    MaterialInfo& matInfo = m_Materials.Add();

    matInfo.DefaultMaterial = "/Default/Materials/Unlit";

    if (Material->unlit && m_Settings.bAllowUnlitMaterials)
    {
        switch (Material->alpha_mode)
        {
            case cgltf_alpha_mode_opaque:
                matInfo.DefaultMaterial = "/Default/Materials/Unlit";
                break;
            case cgltf_alpha_mode_mask:
                matInfo.DefaultMaterial = "/Default/Materials/UnlitMask";
                break;
            case cgltf_alpha_mode_blend:
                matInfo.DefaultMaterial = "/Default/Materials/UnlitOpacity";
                break;
        }

        matInfo.DefaultTexture[0] = "/Default/Textures/BaseColorWhite";

        if (Material->has_pbr_metallic_roughness)
        {
            matInfo.Textures.Add(FindTextureImageGltf(Material->pbr_metallic_roughness.base_color_texture.texture));
        }
        else if (Material->has_pbr_specular_glossiness)
        {
            matInfo.Textures.Add(FindTextureImageGltf(Material->pbr_specular_glossiness.diffuse_texture.texture));
        }
        else
        {
            matInfo.Textures.Add(nullptr);
        }

        SetTextureProps(matInfo.Textures[0], "Texture_BaseColor", true);

        // TODO: create material graph

#if 0
        cgltf_texture_view * texView;

        if ( Material->has_pbr_metallic_roughness ) {
            texView = &Material->pbr_metallic_roughness.base_color_texture;
        } else if ( Material->has_pbr_specular_glossiness ) {
            texView = &Material->pbr_specular_glossiness.diffuse_texture;
        } else {
            texView = nullptr;
        }

        MGMaterialGraph * graph = NewObj< MGMaterialGraph >();

        if ( texView ) {
            MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();

            MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();

            MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
            texCoord->Connect( inTexCoordBlock, "Value" );

            MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();

            cgltf_sampler * sampler = texView->texture->sampler;
            diffuseTexture->SamplerDesc.Filter = ChooseSamplerFilter( sampler->min_filter, sampler->mag_filter );
            diffuseTexture->SamplerDesc.AddressU = ChooseSamplerWrap( sampler->wrap_s );
            diffuseTexture->SamplerDesc.AddressV = ChooseSamplerWrap( sampler->wrap_t );

            MGTextureLoad * textureSampler = graph->AddNode< MGTextureLoad >();
            textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
            textureSampler->Texture->Connect( diffuseTexture, "Value" );

            MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
            materialFragmentStage->Color->Connect( textureSampler, "RGBA" );

            graph->VertexStage = materialVertexStage;
            graph->FragmentStage = materialFragmentStage;
            graph->MaterialType = MATERIAL_TYPE_UNLIT;
            graph->RegisterTextureSlot( diffuseTexture );

            matInfo.Graph = graph;
        } else {
            matInfo.Graph = nullptr; // Will be used default material
        }
#endif
    }
    else if (Material->has_pbr_metallic_roughness)
    {

        matInfo.Textures.ResizeInvalidate(5);
        matInfo.DefaultTexture[0] = "/Default/Textures/BaseColorWhite"; // base color
        matInfo.DefaultTexture[1] = "/Default/Textures/White";          // metallic&roughness
        matInfo.DefaultTexture[2] = "/Default/Textures/Normal";         // normal
        matInfo.DefaultTexture[3] = "/Default/Textures/White";          // occlusion
        matInfo.DefaultTexture[4] = "/Default/Textures/Black";          // emissive

        bool emissiveFactor = Material->emissive_factor[0] > 0.0f || Material->emissive_factor[1] > 0.0f || Material->emissive_factor[2] > 0.0f;

        bool factor = Material->pbr_metallic_roughness.base_color_factor[0] < 1.0f || Material->pbr_metallic_roughness.base_color_factor[1] < 1.0f || Material->pbr_metallic_roughness.base_color_factor[2] < 1.0f || Material->pbr_metallic_roughness.base_color_factor[3] < 1.0f || Material->pbr_metallic_roughness.metallic_factor < 1.0f || Material->pbr_metallic_roughness.roughness_factor < 1.0f || emissiveFactor;

        if (emissiveFactor)
        {
            matInfo.DefaultTexture[4] = "/Default/Textures/White"; // emissive
        }

        if (factor)
        {
            switch (Material->alpha_mode)
            {
                case cgltf_alpha_mode_opaque:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactor";
                    break;
                case cgltf_alpha_mode_mask:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactorMask";
                    break;
                case cgltf_alpha_mode_blend:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactorOpacity";
                    break;
            }

            matInfo.Uniforms[0] = Material->pbr_metallic_roughness.base_color_factor[0];
            matInfo.Uniforms[1] = Material->pbr_metallic_roughness.base_color_factor[1];
            matInfo.Uniforms[2] = Material->pbr_metallic_roughness.base_color_factor[2];
            matInfo.Uniforms[3] = Material->pbr_metallic_roughness.base_color_factor[3];

            matInfo.Uniforms[4] = Material->pbr_metallic_roughness.metallic_factor;
            matInfo.Uniforms[5] = Material->pbr_metallic_roughness.roughness_factor;
            matInfo.Uniforms[6] = 0;
            matInfo.Uniforms[7] = 0;

            matInfo.Uniforms[8] = Material->emissive_factor[0];
            matInfo.Uniforms[9] = Material->emissive_factor[1];
            matInfo.Uniforms[10] = Material->emissive_factor[2];
        }
        else
        {
            switch (Material->alpha_mode)
            {
                case cgltf_alpha_mode_opaque:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughness";
                    break;
                case cgltf_alpha_mode_mask:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessMask";
                    break;
                case cgltf_alpha_mode_blend:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessOpacity";
                    break;
            }
        }

        matInfo.Textures[0] = FindTextureImageGltf(Material->pbr_metallic_roughness.base_color_texture.texture);
        matInfo.Textures[1] = FindTextureImageGltf(Material->pbr_metallic_roughness.metallic_roughness_texture.texture);
        matInfo.Textures[2] = FindTextureImageGltf(Material->normal_texture.texture);
        matInfo.Textures[3] = FindTextureImageGltf(Material->occlusion_texture.texture);
        matInfo.Textures[4] = FindTextureImageGltf(Material->emissive_texture.texture);

        SetTextureProps(matInfo.Textures[0], "Texture_BaseColor", true);
        SetTextureProps(matInfo.Textures[1], "Texture_MetallicRoughness", false);
        SetTextureProps(matInfo.Textures[2], "Texture_Normal", false);
        if (matInfo.Textures[3] != matInfo.Textures[1])
            SetTextureProps(matInfo.Textures[3], "Texture_Occlusion", true);
        SetTextureProps(matInfo.Textures[4], "Texture_Emissive", true);

        // TODO: create material graph

        //cgltf_pbr_metallic_roughness & tex = Material->pbr_metallic_roughness;

        // TODO: create pbr material
    }
    else if (Material->has_pbr_specular_glossiness)
    {
        LOG("Warning: pbr specular glossiness workflow is not supported yet\n");

        matInfo.Textures.ResizeInvalidate(5);
        matInfo.DefaultTexture[0] = "/Default/Textures/BaseColorWhite"; // diffuse
        matInfo.DefaultTexture[1] = "/Default/Textures/White";          // specular&glossiness
        matInfo.DefaultTexture[2] = "/Default/Textures/Normal";         // normal
        matInfo.DefaultTexture[3] = "/Default/Textures/White";          // occlusion
        matInfo.DefaultTexture[4] = "/Default/Textures/Black";          // emissive

        bool emissiveFactor = Material->emissive_factor[0] > 0.0f || Material->emissive_factor[1] > 0.0f || Material->emissive_factor[2] > 0.0f;

        bool factor = Material->pbr_specular_glossiness.diffuse_factor[0] < 1.0f || Material->pbr_specular_glossiness.diffuse_factor[1] < 1.0f || Material->pbr_specular_glossiness.diffuse_factor[2] < 1.0f || Material->pbr_specular_glossiness.diffuse_factor[3] < 1.0f || Material->pbr_specular_glossiness.specular_factor[0] < 1.0f || Material->pbr_specular_glossiness.glossiness_factor < 1.0f || emissiveFactor;

        if (emissiveFactor)
        {
            matInfo.DefaultTexture[4] = "/Default/Textures/White"; // emissive
        }

        if (factor)
        {
            switch (Material->alpha_mode)
            {
                case cgltf_alpha_mode_opaque:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactor";
                    break;
                case cgltf_alpha_mode_mask:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactorMask";
                    break;
                case cgltf_alpha_mode_blend:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessFactorOpacity";
                    break;
            }

            //matInfo.DefaultMaterial = "/Default/Materials/PBRSpecularGlossinessFactor";

            matInfo.Uniforms[0] = Material->pbr_specular_glossiness.diffuse_factor[0];
            matInfo.Uniforms[1] = Material->pbr_specular_glossiness.diffuse_factor[1];
            matInfo.Uniforms[2] = Material->pbr_specular_glossiness.diffuse_factor[2];
            matInfo.Uniforms[3] = Material->pbr_specular_glossiness.diffuse_factor[3];

            matInfo.Uniforms[4] = Material->pbr_specular_glossiness.specular_factor[0];
            matInfo.Uniforms[5] = Material->pbr_specular_glossiness.glossiness_factor;
            matInfo.Uniforms[6] = 0;
            matInfo.Uniforms[7] = 0;

            matInfo.Uniforms[8] = Material->emissive_factor[0];
            matInfo.Uniforms[9] = Material->emissive_factor[1];
            matInfo.Uniforms[10] = Material->emissive_factor[2];
        }
        else
        {
            switch (Material->alpha_mode)
            {
                case cgltf_alpha_mode_opaque:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughness";
                    break;
                case cgltf_alpha_mode_mask:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessMask";
                    break;
                case cgltf_alpha_mode_blend:
                    matInfo.DefaultMaterial = "/Default/Materials/PBRMetallicRoughnessOpacity";
                    break;
            }

            //matInfo.DefaultMaterial = "/Default/Materials/PBRSpecularGlossiness";
        }

        matInfo.Textures[0] = FindTextureImageGltf(Material->pbr_specular_glossiness.diffuse_texture.texture);
        matInfo.Textures[1] = FindTextureImageGltf(Material->pbr_specular_glossiness.specular_glossiness_texture.texture);
        matInfo.Textures[2] = FindTextureImageGltf(Material->normal_texture.texture);
        matInfo.Textures[3] = FindTextureImageGltf(Material->occlusion_texture.texture);
        matInfo.Textures[4] = FindTextureImageGltf(Material->emissive_texture.texture);

        SetTextureProps(matInfo.Textures[0], "Texture_Diffuse", true);
        SetTextureProps(matInfo.Textures[1], "Texture_SpecularGlossiness", false);
        SetTextureProps(matInfo.Textures[2], "Texture_Normal", false);
        SetTextureProps(matInfo.Textures[3], "Texture_Occlusion", true);
        SetTextureProps(matInfo.Textures[4], "Texture_Emissive", true);

        //if (Material->alpha_mode == cgltf_alpha_mode_blend)
        //    SetTextureProps(matInfo.Textures[5], "Texture_Opacity", true);
    }
}

void AssetImporter::ReadNode_r(cgltf_node* Node)
{
    //LOG( "node \"{}\"\n", Node->name );

    //if ( Node->camera ) {
    //    LOG( "has camera\n" );
    //}

    //if ( Node->light ) {
    //    LOG( "has light\n" );
    //}

    //if ( Node->weights ) {
    //    LOG( "has weights {}\n", Node->weights_count );
    //}

    if (m_Settings.bImportMeshes || m_Settings.bImportSkinning || m_Settings.bImportAnimations)
    {
        ReadMesh(Node);
    }

    for (int n = 0; n < Node->children_count; n++)
    {
        cgltf_node* child = Node->children[n];

        ReadNode_r(child);
    }
}

void AssetImporter::ReadMesh(cgltf_node* Node)
{
    cgltf_mesh* mesh = Node->mesh;

    if (!mesh)
    {
        return;
    }

    Float3x4 globalTransform;
    Float3x3 normalMatrix;
    Float4x4 temp;
    cgltf_node_transform_world(Node, (float*)temp.ToPtr());
    Float3x4 rotation = Float3x4(m_Settings.Rotation.ToMatrix3x3().Transposed());
    globalTransform   = rotation * Float3x4(temp.Transposed());
    globalTransform.DecomposeNormalMatrix(normalMatrix);

    ReadMesh(Node, mesh, Float3x4::Scale(Float3(m_Settings.Scale)) * globalTransform, normalMatrix);
}

void AssetImporter::ReadMesh(cgltf_node* Node, cgltf_mesh* Mesh, Float3x4 const& GlobalTransform, Float3x3 const& NormalMatrix)
{
    struct ASortFunction
    {
        bool operator()(cgltf_primitive const& _A, cgltf_primitive const& _B)
        {
            return (_A.material < _B.material);
        }
    } SortFunction;

    std::sort(&Mesh->primitives[0], &Mesh->primitives[Mesh->primitives_count], SortFunction);

    cgltf_material* material = nullptr;

    MeshInfo* meshInfo = nullptr;

    const Half pos  = 1.0f;
    const Half zero = 0.0f;

    for (int i = 0; i < Mesh->primitives_count; i++)
    {
        cgltf_primitive* prim = &Mesh->primitives[i];

        if (prim->type != cgltf_primitive_type_triangles)
        {
            LOG("Only triangle primitives supported\n");
            continue;
        }

        cgltf_accessor* position = nullptr;
        cgltf_accessor* normal   = nullptr;
        cgltf_accessor* tangent  = nullptr;
        cgltf_accessor* texcoord = nullptr;
        cgltf_accessor* color    = nullptr;
        cgltf_accessor* joints   = nullptr;
        cgltf_accessor* weights  = nullptr;

        for (int a = 0; a < prim->attributes_count; a++)
        {
            cgltf_attribute* attrib = &prim->attributes[a];

            if (attrib->data->is_sparse)
            {
                LOG("Warning: sparsed accessors are not supported\n");
                continue;
            }

            switch (attrib->type)
            {
                case cgltf_attribute_type_invalid:
                    LOG("Warning: invalid attribute type\n");
                    continue;

                case cgltf_attribute_type_position:
                    position = attrib->data;
                    break;

                case cgltf_attribute_type_normal:
                    normal = attrib->data;
                    break;

                case cgltf_attribute_type_tangent:
                    tangent = attrib->data;
                    break;

                case cgltf_attribute_type_texcoord:
                    // get first texcoord channel
                    if (!texcoord)
                    {
                        texcoord = attrib->data;
                    }
                    break;

                case cgltf_attribute_type_color:
                    color = attrib->data;
                    break;

                case cgltf_attribute_type_joints:
                    joints = attrib->data;
                    break;

                case cgltf_attribute_type_weights:
                    weights = attrib->data;
                    break;
            }
        }

        if (!position)
        {
            LOG("Warning: no positions\n");
            continue;
        }

        if (position->type != cgltf_type_vec2 && position->type != cgltf_type_vec3)
        {
            LOG("Warning: invalid vertex positions\n");
            continue;
        }

        if (!texcoord)
        {
            LOG("Warning: no texcoords\n");
        }

        if (texcoord && texcoord->type != cgltf_type_vec2)
        {
            LOG("Warning: invalid texcoords\n");
            texcoord = nullptr;
        }

        int vertexCount = position->count;
        if (texcoord && texcoord->count != vertexCount)
        {
            LOG("Warning: texcoord count != position count\n");
            texcoord = nullptr;
        }

        if (!material || material != prim->material || !m_Settings.bMergePrimitives)
        {
            meshInfo              = &m_Meshes.Add();
            meshInfo->BaseVertex  = m_Vertices.Size();
            meshInfo->FirstIndex  = m_Indices.Size();
            meshInfo->VertexCount = 0;
            meshInfo->IndexCount  = 0;
            meshInfo->UniqueName  = Mesh->name;
            meshInfo->MaterialNum = MapGltfMaterial(prim->material);
            meshInfo->BoundingBox.Clear();

            meshInfo->NodeGltf = Node;
            meshInfo->bSkinned = weights != nullptr;

            material = prim->material;
        }

        int firstVert = m_Vertices.Size();
        m_Vertices.Resize(firstVert + vertexCount);

        int vertexOffset = firstVert - meshInfo->BaseVertex;

        int firstIndex = m_Indices.Size();
        int indexCount;
        if (prim->indices)
        {
            indexCount = prim->indices->count;
            m_Indices.Resize(firstIndex + indexCount);
            unsigned int* pInd = m_Indices.ToPtr() + firstIndex;
            for (int index = 0; index < indexCount; index++, pInd++)
            {
                *pInd = vertexOffset + cgltf_accessor_read_index(prim->indices, index);
            }
        }
        else
        {
            indexCount = vertexCount;
            m_Indices.Resize(firstIndex + indexCount);
            unsigned int* pInd = m_Indices.ToPtr() + firstIndex;
            for (int index = 0; index < indexCount; index++, pInd++)
            {
                *pInd = vertexOffset + index;
            }
        }

        unpack_vec2_or_vec3(position, &m_Vertices[firstVert].Position, sizeof(MeshVertex));

        if (texcoord)
        {
            unpack_vec2_to_half2(texcoord, &m_Vertices[firstVert].TexCoord[0], sizeof(MeshVertex));
        }
        else
        {
            for (int v = 0; v < vertexCount; v++)
            {
                m_Vertices[firstVert + v].SetTexCoord(zero, zero);
            }
        }

        if (normal && (normal->type == cgltf_type_vec2 || normal->type == cgltf_type_vec3) && normal->count == vertexCount)
        {
            unpack_vec2_or_vec3_to_half3(normal, &m_Vertices[firstVert].Normal[0], sizeof(MeshVertex), true);
        }
        else
        {
            // TODO: compute normals

            LOG("Warning: no normals\n");

            for (int v = 0; v < vertexCount; v++)
            {
                m_Vertices[firstVert + v].SetNormal(zero, pos, zero);
            }
        }

        if (tangent && (tangent->type == cgltf_type_vec4) && tangent->count == vertexCount)
        {
            unpack_tangents(tangent, &m_Vertices[firstVert]);

            //CalcTangentSpace( m_Vertices.ToPtr() + meshInfo->BaseVertex, m_Vertices.Size() - meshInfo->BaseVertex, m_Indices.ToPtr() + firstIndex, indexCount );
        }
        else
        {

            if (texcoord)
            {
                Geometry::CalcTangentSpace(m_Vertices.ToPtr() + meshInfo->BaseVertex, m_Vertices.Size() - meshInfo->BaseVertex, m_Indices.ToPtr() + firstIndex, indexCount);
            }
            else
            {
                MeshVertex* pVert = m_Vertices.ToPtr() + firstVert;
                for (int v = 0; v < vertexCount; v++, pVert++)
                {
                    pVert->SetTangent(pos, zero, zero);
                    pVert->Handedness = 1;
                }
            }
        }

        if (weights && (weights->type == cgltf_type_vec4) && weights->count == vertexCount && joints && (joints->type == cgltf_type_vec4) && joints->count == vertexCount)
        {
            m_Weights.Resize(m_Vertices.Size());
            unpack_weights(weights, &m_Weights[firstVert]);
            unpack_joints(joints, &m_Weights[firstVert]);
        }

        HK_UNUSED(color);

        MeshVertex* pVert = m_Vertices.ToPtr() + firstVert;

        if (/*m_Settings.bSingleModel && */ !m_bSkeletal)
        {
            for (int v = 0; v < vertexCount; v++, pVert++)
            {
                // Pretransform vertices
                pVert->Position = Float3(GlobalTransform * pVert->Position);
                pVert->SetNormal(NormalMatrix * pVert->GetNormal());
                pVert->SetTangent(NormalMatrix * pVert->GetTangent());

                // Calc bounding box
                meshInfo->BoundingBox.AddPoint(pVert->Position);
            }
        }
        else
        {
            Float3x3 rotation = m_Settings.Rotation.ToMatrix3x3();
            for (int v = 0; v < vertexCount; v++, pVert++)
            {
                pVert->Position = m_Settings.Scale * Float3(rotation * pVert->Position);
                pVert->SetNormal(rotation * pVert->GetNormal());
                pVert->SetTangent(rotation * pVert->GetTangent());

                // Calc bounding box
                meshInfo->BoundingBox.AddPoint(pVert->Position);
            }
        }

        meshInfo->VertexCount += vertexCount;
        meshInfo->IndexCount += indexCount;

        //cgltf_morph_target* targets;
        //cgltf_size targets_count;
    }

    //for ( int i = 0; i < mesh->weights_count; i++ ) {
    //    cgltf_float * w = &mesh->weights[ i ];
    //}

    LOG("Subparts {}, Primitives {}\n", m_Meshes.Size(), Mesh->primitives_count);

    if (m_bSkeletal)
    {
        int numWeights  = m_Weights.Size();
        int numVertices = m_Vertices.Size();
        if (numWeights != numVertices)
        {
            LOG("Warning: invalid mesh (num weights != num vertices)\n");

            m_Weights.Resize(numVertices);

            // Clear
            int count = numVertices - numWeights;
            for (int i = 0; i < count; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    m_Weights[numWeights + i].JointIndices[j] = 0;
                    m_Weights[numWeights + i].JointWeights[j] = 0;
                }
                m_Weights[numWeights + i].JointWeights[0] = 255;
            }
        }
    }
}

void AssetImporter::ReadAnimations(cgltf_data* Data)
{
    m_Animations.Resize(Data->animations_count);
    for (int animIndex = 0; animIndex < Data->animations_count; animIndex++)
    {
        AnimationInfo& animation = m_Animations[animIndex];

        ReadAnimation(&Data->animations[animIndex], animation);

        Geometry::CalcBoundingBoxes(m_Vertices.ToPtr(),
                                    m_Weights.ToPtr(),
                                    m_Vertices.Size(),
                                    &m_Skin,
                                    m_Joints.ToPtr(),
                                    m_Joints.Size(),
                                    animation.FrameCount,
                                    animation.Channels.ToPtr(),
                                    animation.Channels.Size(),
                                    animation.Transforms.ToPtr(),
                                    animation.Bounds);
    }
}

void AssetImporter::ReadAnimation(cgltf_animation* Anim, AnimationInfo& Animation)
{
    const int framesPerSecond = 30;
    //    float gcd = 0;
    float maxDuration = 0;

    for (int ch = 0; ch < Anim->channels_count; ch++)
    {
        cgltf_animation_channel* channel   = &Anim->channels[ch];
        cgltf_animation_sampler* sampler   = channel->sampler;
        cgltf_accessor*          animtimes = sampler->input;

        if (animtimes->count == 0)
        {
            continue;
        }

#if 0
        float time = 0;
        for ( int t = 0; t < animtimes->count; t++ ) {
            cgltf_accessor_read_float( animtimes, t, &time, 1 );
            gcd = Math::GreaterCommonDivisor( gcd, time );
        }
#endif
        float time = 0;
        cgltf_accessor_read_float(animtimes, animtimes->count - 1, &time, 1);
        maxDuration = Math::Max(maxDuration, time);
    }

    int numFrames = maxDuration * framesPerSecond;
    //numFrames--;

    float frameDelta = maxDuration / numFrames;

    Animation.Name       = Anim->name ? Anim->name : "Animation";
    Animation.FrameDelta = frameDelta;
    Animation.FrameCount = numFrames; // frames count, animation duration is FrameDelta * ( FrameCount - 1 )

    for (int ch = 0; ch < Anim->channels_count; ch++)
    {

        cgltf_animation_channel* channel = &Anim->channels[ch];
        cgltf_animation_sampler* sampler = channel->sampler;

        if (!IsChannelValid(channel))
        {
            continue;
        }

        // HACK: get joint index from camera pointer
        int nodeIndex = channel->target_node->camera ? (size_t)channel->target_node->camera - 1 : m_Joints.Size();
        if (nodeIndex >= m_Joints.Size())
        {
            LOG("Warning: joint {} is not found\n", channel->target_node->name);
            continue;
        }

        // just for debug
        //if ( channel->target_node->name && Core::Icmp( channel->target_node->name, "L_arm_029" ) ) {
        //    continue;
        //}

        int mergedChannel;
        for (mergedChannel = 0; mergedChannel < Animation.Channels.Size(); mergedChannel++)
        {
            if (nodeIndex == Animation.Channels[mergedChannel].JointIndex)
            {
                break;
            }
        }

        AnimationChannel* jointAnim;

        if (mergedChannel < Animation.Channels.Size())
        {
            jointAnim = &Animation.Channels[mergedChannel];
        }
        else
        {
            jointAnim                  = &Animation.Channels.Add();
            jointAnim->JointIndex      = nodeIndex;
            jointAnim->TransformOffset = Animation.Transforms.Size();
            jointAnim->bHasPosition    = false;
            jointAnim->bHasRotation    = false;
            jointAnim->bHasScale       = false;
            Animation.Transforms.Resize(Animation.Transforms.Size() + numFrames);

            Float3   position;
            Float3x3 rotation;
            Quat     q;
            Float3   scale;
            m_Joints[nodeIndex].LocalTransform.DecomposeAll(position, rotation, scale);
            q.FromMatrix(rotation);
            for (int f = 0; f < numFrames; f++)
            {
                Transform& transform = Animation.Transforms[jointAnim->TransformOffset + f];
                transform.Position    = position;
                transform.Scale       = scale;
                transform.Rotation    = q;
            }
        }

        switch (channel->target_path)
        {
            case cgltf_animation_path_type_translation:
                jointAnim->bHasPosition = true;

                for (int f = 0; f < numFrames; f++)
                {
                    Transform& transform = Animation.Transforms[jointAnim->TransformOffset + f];

                    sample_vec3(sampler, f * frameDelta, transform.Position);

                    transform.Position *= m_Settings.Scale;
                }

                break;
            case cgltf_animation_path_type_rotation:
                jointAnim->bHasRotation = true;

                for (int f = 0; f < numFrames; f++)
                {
                    Transform& transform = Animation.Transforms[jointAnim->TransformOffset + f];

                    sample_quat(sampler, f * frameDelta, transform.Rotation);
                }

                break;
            case cgltf_animation_path_type_scale:
                jointAnim->bHasScale = true;

                for (int f = 0; f < numFrames; f++)
                {
                    Transform& transform = Animation.Transforms[jointAnim->TransformOffset + f];

                    sample_vec3(sampler, f * frameDelta, transform.Scale);
                }

                break;
            default:
                LOG("Warning: Unsupported target path\n");
                break;
        }

        for (int f = 0; f < numFrames; f++)
        {
            Transform& transform = Animation.Transforms[jointAnim->TransformOffset + f];

            float frameTime = f * frameDelta;

            switch (channel->target_path)
            {
                case cgltf_animation_path_type_translation:
                    sample_vec3(sampler, frameTime, transform.Position);
                    transform.Position *= m_Settings.Scale;
                    break;
                case cgltf_animation_path_type_rotation:
                    sample_quat(sampler, frameTime, transform.Rotation);
                    break;
                case cgltf_animation_path_type_scale:
                    sample_vec3(sampler, frameTime, transform.Scale);
                    break;
                default:
                    LOG("Warning: Unsupported target path\n");
                    f = numFrames;
                    break;
            }
        }
    }

    for (int channel = 0; channel < Animation.Channels.Size(); channel++)
    {
        AnimationChannel* jointAnim = &Animation.Channels[channel];

        if (jointAnim->JointIndex == 0 && jointAnim->bHasRotation)
        {
            for (int frameIndex = 0; frameIndex < numFrames; frameIndex++)
            {

                Transform& transform = Animation.Transforms[jointAnim->TransformOffset + frameIndex];

                transform.Rotation = m_Settings.Rotation * transform.Rotation;
            }
        }
    }
}

#if 0
Float3x4 CalcJointWorldTransform( SkeletonJoint const & InJoint, SkeletonJoint const * InJoints ) {
    if ( InJoint.Parent == -1 ) {
        return InJoint.LocalTransform;
    }
    return CalcJointWorldTransform( InJoints[InJoint.Parent], InJoints ) * InJoint.LocalTransform;
}
#endif

void AssetImporter::WriteAssets()
{
    if (m_Settings.bImportTextures)
    {
        WriteTextures();
    }

    if (m_Settings.bImportMaterials)
    {
        WriteMaterials();
    }

    if (m_Settings.bImportSkinning)
    {
        if (m_Settings.bImportSkeleton)
        {
            WriteSkeleton();
        }

        if (m_Settings.bImportAnimations)
        {
            WriteAnimations();
        }
    }

    if (m_Settings.bImportMeshes)
    {
        if (m_Settings.bSingleModel || m_bSkeletal)
        {
            WriteSingleModel();
        }
        else
        {
            WriteMeshes();
        }
    }
}

void AssetImporter::WriteTextures()
{
    for (TextureInfo& tex : m_Textures)
    {
        WriteTexture(tex);
    }
}

void AssetImporter::WriteTexture(TextureInfo& tex)
{
    String fileName       = GeneratePhysicalPath(!tex.Name.IsEmpty() ? tex.Name : "texture", ".texture");
    String sourceFileName = m_Path + tex.Path;
    String fileSystemPath = m_Settings.RootPath + fileName;

    ImageMipmapConfig mipmapConfig;
    mipmapConfig.EdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
    mipmapConfig.Filter   = IMAGE_RESAMPLE_FILTER_MITCHELL;

    ImageStorage image = CreateImage(sourceFileName, &mipmapConfig, IMAGE_STORAGE_FLAGS_DEFAULT, tex.bSRGB ? TEXTURE_FORMAT_SRGBA8_UNORM : TEXTURE_FORMAT_RGBA8_UNORM);
    //ImageStorage image = CreateImage(sourceFileName, &mipmapConfig, IMAGE_STORAGE_FLAGS_DEFAULT, tex.bSRGB ? TEXTURE_FORMAT_BC6H_UFLOAT : TEXTURE_FORMAT_RGBA8_UNORM);
    if (!image)
        return;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    tex.PathToWrite = "/Root/" + fileName;

    f.WriteUInt32(ASSET_TEXTURE);
    f.WriteUInt32(ASSET_VERSION_TEXTURE);
    f.WriteObject(image);

    f.WriteUInt32(1); // num source files
    f.WriteString(sourceFileName);
}

void AssetImporter::WriteMaterials()
{
    for (MaterialInfo& m : m_Materials)
    {
        WriteMaterial(m);
    }
}

void AssetImporter::WriteMaterial(MaterialInfo& m)
{
    String fileName       = GeneratePhysicalPath("matinst", ".minst");
    String fileSystemPath = m_Settings.RootPath + fileName;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    m.PathToWrite = "/Root/" + fileName;

    f.FormattedPrint("Material \"{}\"\n", m.DefaultMaterial);
    f.FormattedPrint("Textures [\n");
    for (uint32_t i = 0; i < m.Textures.Size(); i++)
    {
        if (m.Textures[i])
        {
            f.FormattedPrint("\"{}\"\n", m.Textures[i]->PathToWrite);
        }
        else
        {
            f.FormattedPrint("\"{}\"\n", m.DefaultTexture.At(i));
        }
    }
    f.FormattedPrint("]\n");
    f.FormattedPrint("Uniforms [\n");
    for (int i = 0; i < HK_ARRAY_SIZE(m.Uniforms); i++)
    {
        f.FormattedPrint("\"{}\"\n", Core::ToString(m.Uniforms[i]));
    }
    f.FormattedPrint("]\n");
}

static String ValidateFileName(StringView FileName)
{
    String ValidatedName = FileName;

    for (StringSizeType i = 0; i < ValidatedName.Size(); i++)
    {
        char& Ch = ValidatedName[i];

        switch (Ch)
        {
            case ':':
            case '\\':
            case '/':
            case '?':
            case '@':
            case '$':
            case '*':
            case '|':
                Ch = '_';
                break;
        }
    }

    return ValidatedName;
}

String AssetImporter::GeneratePhysicalPath(StringView DesiredName, StringView Extension)
{
    String sourceName    = PathUtils::GetFilenameNoExt(PathUtils::GetFilenameNoPath(m_Settings.ImportFile));
    String validatedName = ValidateFileName(DesiredName);

    sourceName.ToLower();
    validatedName.ToLower();

    String path   = m_Settings.OutputPath / sourceName + "_" + validatedName;
    String result = path + Extension;

    int uniqueNumber = 0;

    while (Core::IsFileExists(m_Settings.RootPath + result))
    {
        result = path + "_" + Core::ToString(++uniqueNumber) + Extension;
    }

    return result;
}

int AssetImporter::MapGltfMaterial(cgltf_material* pMaterial)
{
    for (int i = 0; i < m_Data->materials_count; ++i)
    {
        if (pMaterial == &m_Data->materials[i])
            return i;
    }
    return -1;
}

void AssetImporter::WriteSkeleton()
{
    if (!m_Joints.IsEmpty())
    {
        String fileName       = GeneratePhysicalPath("skeleton", ".skeleton");
        String fileSystemPath = m_Settings.RootPath + fileName;

        File f = File::OpenWrite(fileSystemPath);
        if (!f)
        {
            LOG("Failed to write {}\n", fileName);
            return;
        }

        m_SkeletonPath = "/Root/" + fileName;

        f.WriteUInt32(ASSET_SKELETON);
        f.WriteUInt32(ASSET_VERSION_SKELETON);
        f.WriteString(""); // TODO: remove
        f.WriteArray(m_Joints);
        f.WriteObject(m_BindposeBounds);
    }
}

void AssetImporter::WriteAnimations()
{
    for (AnimationInfo& animation : m_Animations)
    {
        WriteAnimation(animation);
    }
}

void AssetImporter::WriteAnimation(AnimationInfo const& Animation)
{
    String fileName       = GeneratePhysicalPath(Animation.Name, ".animation");
    String fileSystemPath = m_Settings.RootPath + fileName;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    f.WriteUInt32(ASSET_ANIMATION);
    f.WriteUInt32(ASSET_VERSION_ANIMATION);
    f.WriteString(""); // TODO: remove
    f.WriteFloat(Animation.FrameDelta);
    f.WriteUInt32(Animation.FrameCount);
    f.WriteArray(Animation.Channels);
    f.WriteArray(Animation.Transforms);
    f.WriteArray(Animation.Bounds);
}

void AssetImporter::WriteSingleModel()
{
    if (m_Meshes.IsEmpty())
    {
        return;
    }

    String fileName       = GeneratePhysicalPath("mesh", ".mesh_data");
    String fileSystemPath = m_Settings.RootPath + fileName;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    String pathToWrite = "/Root/" + fileName;

    bool bSkinnedMesh = m_bSkeletal;

    BvAxisAlignedBox BoundingBox;
    BoundingBox.Clear();
    for (MeshInfo const& meshInfo : m_Meshes)
    {
        BoundingBox.AddAABB(meshInfo.BoundingBox);
    }

    bool bRaycastBVH = m_Settings.bGenerateRaycastBVH && !bSkinnedMesh;

    f.WriteUInt32(ASSET_MESH);
    f.WriteUInt32(ASSET_VERSION_MESH);
    f.WriteString(""); // TODO: remove
    f.WriteBool(bSkinnedMesh);
    f.WriteObject(BoundingBox);
    f.WriteArray(m_Indices);
    f.WriteArray(m_Vertices);
    if (bSkinnedMesh)
    {
        f.WriteArray(m_Weights);
    }
    else
    {
        f.WriteUInt32(0); // weights count
    }
    f.WriteBool(bRaycastBVH); // only for static meshes
    f.WriteUInt16(m_Settings.RaycastPrimitivesPerLeaf);

    // Write subparts
    f.WriteUInt32(m_Meshes.Size()); // subparts count
    uint32_t n = 0;
    for (MeshInfo const& meshInfo : m_Meshes)
    {
        if (!meshInfo.UniqueName.IsEmpty())
        {
            f.WriteString(meshInfo.UniqueName);
        }
        else
        {
            f.WriteString(HK_FORMAT("Subpart_{}", n));
        }
        f.WriteInt32(meshInfo.BaseVertex);
        f.WriteUInt32(meshInfo.FirstIndex);
        f.WriteUInt32(meshInfo.VertexCount);
        f.WriteUInt32(meshInfo.IndexCount);
        f.WriteObject(meshInfo.BoundingBox);

        n++;
    }

    if (bRaycastBVH)
    {
        for (MeshInfo const& meshInfo : m_Meshes)
        {
            // Generate subpart BVH
            BvhTree aabbTree(TArrayView<MeshVertex>(m_Vertices), {m_Indices.ToPtr() + meshInfo.FirstIndex, (size_t)meshInfo.IndexCount}, meshInfo.BaseVertex, m_Settings.RaycastPrimitivesPerLeaf);

            // Write subpart BVH
            f.WriteObject(aabbTree);
        }
    }

    f.WriteUInt32(0); // sockets count

    if (bSkinnedMesh)
    {
        f.WriteArray(m_Skin.JointIndices);
        f.WriteArray(m_Skin.OffsetMatrices);
    }

    //if ( m_Settings.bGenerateStaticCollisions ) {
    //TVector< ACollisionTriangleSoupData::SSubpart > subparts;

    //subparts.Resize( m_Meshes.Size() );
    //for ( int i = 0 ; i < subparts.Size() ; i++ ) {
    //    subparts[i].BaseVertex = m_Meshes[i].BaseVertex;
    //    subparts[i].VertexCount = m_Meshes[i].VertexCount;
    //    subparts[i].FirstIndex = m_Meshes[i].FirstIndex;
    //    subparts[i].IndexCount = m_Meshes[i].IndexCount;
    //}

    //ACollisionTriangleSoupData * tris = NewObj< ACollisionTriangleSoupData >();

    //tris->Initialize( (float *)&m_Vertices.ToPtr()->Position, sizeof( m_Vertices[0] ), m_Vertices.Size(),
    //    m_Indices.ToPtr(), m_Indices.Size(), subparts.ToPtr(), subparts.Size(), BoundingBox );

    //ACollisionTriangleSoupBVHData * bvh = NewObj< ACollisionTriangleSoupBVHData >();
    //bvh->TrisData = tris;
    //bvh->BuildBVH();
    //bvh->Write( f );
    //}


    fileName       = GeneratePhysicalPath("mesh", ".mesh");
    fileSystemPath = m_Settings.RootPath + fileName;

    f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    f.FormattedPrint("Mesh \"{}\"\n", pathToWrite);

    if (bSkinnedMesh)
    {
        f.FormattedPrint("Skeleton \"{}\"\n", m_SkeletonPath);
    }
    else
    {
        f.FormattedPrint("Skeleton \"{}\"\n", "/Default/Skeleton/Default");
    }
    f.FormattedPrint("Subparts [\n");
    String dummy;
    for (MeshInfo const& meshInfo : m_Meshes)
    {
        String materialPath = meshInfo.MaterialNum >= 0 ? m_Materials[meshInfo.MaterialNum].PathToWrite : dummy;
        f.FormattedPrint("\"{}\"\n", materialPath);
    }
    f.FormattedPrint("]\n");
}

void AssetImporter::WriteMeshes()
{
    for (MeshInfo& meshInfo : m_Meshes)
    {
        WriteMesh(meshInfo);
    }
}

void AssetImporter::WriteMesh(MeshInfo const& Mesh)
{
    String fileName       = GeneratePhysicalPath(!Mesh.UniqueName.IsEmpty() ? Mesh.UniqueName : "mesh", ".mesh_data");
    String fileSystemPath = m_Settings.RootPath + fileName;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    bool bSkinnedMesh = m_bSkeletal;
    HK_ASSERT(bSkinnedMesh == false);

    String pathToMesh = "/Root/" + fileName;

    bool bRaycastBVH = m_Settings.bGenerateRaycastBVH;

    f.WriteUInt32(ASSET_MESH);
    f.WriteUInt32(ASSET_VERSION_MESH);
    f.WriteString(""); // TODO: remove
    f.WriteBool(bSkinnedMesh);
    f.WriteObject(Mesh.BoundingBox);

    f.WriteUInt32(Mesh.IndexCount);
    unsigned int* indices = m_Indices.ToPtr() + Mesh.FirstIndex;
    for (int i = 0; i < Mesh.IndexCount; i++)
    {
        f.WriteUInt32(*indices++);
    }

    f.WriteUInt32(Mesh.VertexCount);
    MeshVertex* verts = m_Vertices.ToPtr() + Mesh.BaseVertex;
    for (int i = 0; i < Mesh.VertexCount; i++)
    {
        verts->Write(f);
        verts++;
    }

    if (bSkinnedMesh)
    {
        f.WriteUInt32(Mesh.VertexCount); // weights count

        MeshVertexSkin* weights = m_Weights.ToPtr() + Mesh.BaseVertex;
        for (int i = 0; i < Mesh.VertexCount; i++)
        {
            weights->Write(f);
            weights++;
        }
    }
    else
    {
        f.WriteUInt32(0); // weights count
    }
    f.WriteBool(bRaycastBVH); // only for static meshes
    f.WriteUInt16(m_Settings.RaycastPrimitivesPerLeaf);
    f.WriteUInt32(1); // subparts count
    if (!Mesh.UniqueName.IsEmpty())
    {
        f.WriteString(Mesh.UniqueName);
    }
    else
    {
        f.WriteString("Subpart_1");
    }
    f.WriteInt32(0);  // base vertex
    f.WriteUInt32(0); // first index
    f.WriteUInt32(Mesh.VertexCount);
    f.WriteUInt32(Mesh.IndexCount);
    f.WriteObject(Mesh.BoundingBox);

    if (bRaycastBVH)
    {
        // Generate subpart BVH
        BvhTree aabbTree(TArrayView<MeshVertex>(m_Vertices.ToPtr() + Mesh.BaseVertex, m_Vertices.Size() - Mesh.BaseVertex),
                         TArrayView<unsigned int>(m_Indices.ToPtr() + Mesh.FirstIndex, (size_t)Mesh.IndexCount),
                         0,
                         m_Settings.RaycastPrimitivesPerLeaf);

        // Write subpart BVH
        f.WriteObject(aabbTree);
    }

    f.WriteUInt32(0); // sockets count

    if (bSkinnedMesh)
    {
        f.WriteArray(m_Skin.JointIndices);
        f.WriteArray(m_Skin.OffsetMatrices);
    }

    fileName       = GeneratePhysicalPath("mesh", ".mesh");
    fileSystemPath = m_Settings.RootPath + fileName;

    f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    f.FormattedPrint("Mesh \"{}\"\n", pathToMesh);

    if (bSkinnedMesh)
    {
        f.FormattedPrint("Skeleton \"{}\"\n", m_SkeletonPath);
    }
    else
    {
        f.FormattedPrint("Skeleton \"{}\"\n", "/Default/Skeleton/Default");
    }
    f.FormattedPrint("Subparts [\n");
    String        dummy;
    String const& materialPath = Mesh.MaterialNum >= 0 ? m_Materials[Mesh.MaterialNum].PathToWrite : dummy;
    f.FormattedPrint("\"{}\"\n", materialPath);
    f.FormattedPrint("]\n");
}

bool SaveSkyboxTexture(StringView FileName, ImageStorage const& Image)
{
    if (!Image || Image.GetDesc().Type != TEXTURE_CUBE)
    {
        LOG("SaveSkyboxTexture: invalid skybox\n");
        return false;
    }

    File f = File::OpenWrite(FileName);
    if (!f)
    {
        LOG("Failed to write {}\n", FileName);
        return false;
    }

    f.WriteUInt32(ASSET_TEXTURE);
    f.WriteUInt32(ASSET_VERSION_TEXTURE);

    f.WriteObject(Image);

    f.WriteUInt32(6); // num source files
    for (int i = 0; i < 6; i++)
    {
        f.WriteString("Generated"); // source file
    }
    return true;
}

bool AssetImporter::ImportSkybox(AssetImportSettings const& Settings)
{
    m_Settings            = Settings;
    m_Settings.ImportFile = "Skybox";

    if (!Settings.bImportSkyboxExplicit)
    {
        return false;
    }

    ImageStorage image = LoadSkyboxImages(Settings.SkyboxImport);
    if (!image)
        return false;

    String fileName       = GeneratePhysicalPath("texture", ".texture");
    String fileSystemPath = m_Settings.RootPath + fileName;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return false;
    }

    f.WriteUInt32(ASSET_TEXTURE);
    f.WriteUInt32(ASSET_VERSION_TEXTURE);

    f.WriteObject(image);

    f.WriteUInt32(6); // num source files
    for (int i = 0; i < 6; i++)
    {
        f.WriteString(Settings.SkyboxImport.Faces[i]); // source file
    }

    if (m_Settings.bCreateSkyboxMaterialInstance)
    {
        WriteSkyboxMaterial("/Root/" + fileName);
    }

    return true;
}

void AssetImporter::WriteSkyboxMaterial(StringView SkyboxTexture)
{
    String fileName       = GeneratePhysicalPath("matinst", ".minst");
    String fileSystemPath = m_Settings.RootPath + fileName;

    File f = File::OpenWrite(fileSystemPath);
    if (!f)
    {
        LOG("Failed to write {}\n", fileName);
        return;
    }

    f.FormattedPrint("Material \"/Default/Materials/Skybox\"\n");
    f.FormattedPrint("Textures [\n");
    f.FormattedPrint("\"{}\"\n", SkyboxTexture);
    f.FormattedPrint("]\n");
}

bool AssetImporter::ImportOBJ(AssetImportSettings const& Settings)
{
    String const& Source = Settings.ImportFile;

    m_Settings = Settings;

    m_Path = PathUtils::GetFilePath(Settings.ImportFile);
    m_Path += "/";

    fastObjMesh* mesh = fast_obj_read(Source.CStr());
    if (!mesh)
    {
        LOG("Failed to load {}\n", Source);
        return false;
    }

    ReadOBJ(mesh);

    fast_obj_destroy(mesh);

    WriteAssets();

    return true;
}

bool AssetImporter::ReadOBJ(fastObjMesh* pMesh)
{
    struct Vertex
    {
        Float3 Position;
        Float2 TexCoord;
        Float3 Normal;

        Vertex() = default;
        Vertex(Float3 const& Position, Float2 const& TexCoord, Float3 const& Normal) :
            Position(Position), TexCoord(TexCoord), Normal(Normal)
        {}

        bool operator==(Vertex const& Rhs) const
        {
            return Position == Rhs.Position && TexCoord == Rhs.TexCoord && Normal == Rhs.Normal;
        }

        std::size_t Hash() const
        {
            return (uint32_t(Position.X * 100) * 73856093) ^ (uint32_t(Position.Y * 100) * 19349663) ^ (uint32_t(Position.Z * 100) * 83492791);
        }
    };

    THashMap<unsigned int, TVector<Vertex>> vertexList;
    THashMap<Vertex, unsigned int>          vertexHash;
    bool                                    bUnsupportedVertexCount = false;

    for (unsigned int groupIndex = 0; groupIndex < pMesh->group_count; ++groupIndex)
    {
        fastObjGroup const* group        = &pMesh->groups[groupIndex];
        fastObjIndex const* groupIndices = &pMesh->indices[group->index_offset];

        unsigned int indexNum = 0;
        for (unsigned int faceIndex = 0; faceIndex < group->face_count; ++faceIndex)
        {
            unsigned int vertexCount = pMesh->face_vertices[group->face_offset + faceIndex];
            unsigned int material    = pMesh->face_materials[group->face_offset + faceIndex];

            TVector<Vertex>& vertices = vertexList[material];

            if (vertexCount == 3)
            {
                for (unsigned int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    fastObjIndex index = groupIndices[indexNum++];

                    Vertex& v = vertices.Add();

                    v.Position.X = pMesh->positions[index.p * 3 + 0];
                    v.Position.Y = pMesh->positions[index.p * 3 + 1];
                    v.Position.Z = pMesh->positions[index.p * 3 + 2];

                    v.TexCoord.X = pMesh->texcoords[index.t * 2 + 0];
                    v.TexCoord.Y = pMesh->texcoords[index.t * 2 + 1];

                    v.Normal.X = pMesh->normals[index.n * 3 + 0];
                    v.Normal.Y = pMesh->normals[index.n * 3 + 1];
                    v.Normal.Z = pMesh->normals[index.n * 3 + 2];
                }
            }
            else if (vertexCount == 4)
            {
                const int quadIndices[6] = {0, 1, 2, 2, 3, 0};

                for (unsigned int vertexIndex = 0; vertexIndex < vertexCount / 4; vertexIndex += 4, indexNum += 4)
                {
                    for (int vIndex = 0; vIndex < 6; ++vIndex)
                    {
                        fastObjIndex index = groupIndices[indexNum + quadIndices[vIndex]];
                        Vertex&      v     = vertices.Add();

                        v.Position.X = pMesh->positions[index.p * 3 + 0];
                        v.Position.Y = pMesh->positions[index.p * 3 + 1];
                        v.Position.Z = pMesh->positions[index.p * 3 + 2];

                        v.TexCoord.X = pMesh->texcoords[index.t * 2 + 0];
                        v.TexCoord.Y = pMesh->texcoords[index.t * 2 + 1];

                        v.Normal.X = pMesh->normals[index.n * 3 + 0];
                        v.Normal.Y = pMesh->normals[index.n * 3 + 1];
                        v.Normal.Z = pMesh->normals[index.n * 3 + 2];
                    }
                }
            }
            else
                bUnsupportedVertexCount = true;
        }
    }

    if (bUnsupportedVertexCount)
    {
        LOG("AssetImporter::ReadOBJ: The mesh contains polygons with an unsupported number of vertices. Polygons are expected to have 3 or 4 vertices.\n");
    }

    TStringHashMap<unsigned int> uniqueMaterials;
    TVector<fastObjMaterial const*> materialList;

    for (auto& it : vertexList)
    {
        unsigned int materialNum = it.first;

        HK_ASSERT(materialNum < pMesh->material_count);
        fastObjMaterial const* pMaterial = &pMesh->materials[materialNum];

        if (uniqueMaterials.Count(pMaterial->map_Kd.path) == 0)
        {
            uniqueMaterials[pMaterial->map_Kd.path] = materialList.Size();
            materialList.Add(pMaterial);
        }
    }

    m_Materials.Reserve(materialList.Size());
    m_Textures.Reserve(materialList.Size());

    for (unsigned int materialNum = 0; materialNum < materialList.Size(); ++materialNum)
    {
        TextureInfo& texInfo = m_Textures.Add();

        texInfo.bSRGB = true;
        texInfo.Path  = materialList[materialNum]->map_Kd.name;//.path;

        MaterialInfo& matInfo = m_Materials.Add();

        matInfo.DefaultMaterial = "/Default/Materials/Unlit";
        matInfo.Textures.Add(&texInfo);
    }

    unsigned int baseVertex = 0;
    unsigned int firstIndex = 0;

    for (auto& it : vertexList)
    {
        unsigned int     materialNum = it.first;
        TVector<Vertex>& vertices = it.second;

        if (vertices.IsEmpty())
            continue;

        fastObjMaterial const* pMaterial = &pMesh->materials[materialNum];

        BvAxisAlignedBox bounds;
        bounds.Clear();

        vertexHash.Clear();
        for (Vertex const& v : vertices)
        {
            auto vertexIt = vertexHash.Find(v);
            if (vertexIt == vertexHash.End())
            {
                vertexHash[v] = m_Vertices.Size() - baseVertex;

                MeshVertex& meshVert = m_Vertices.Add();

                meshVert.Position = v.Position * m_Settings.Scale;
                meshVert.SetTexCoord(Float2(v.TexCoord.X,1.0f-v.TexCoord.Y));
                meshVert.SetNormal(v.Normal);

                bounds.AddPoint(meshVert.Position);
            }
        }

        unsigned int vertexCount = m_Vertices.Size() - baseVertex;
        unsigned int indexCount  = vertices.Size();

        m_Indices.Resize(firstIndex + indexCount);
        for (unsigned int i = 0; i < indexCount; ++i)
            m_Indices[firstIndex + i] = vertexHash[vertices[i]];

        MeshInfo& meshInfo = m_Meshes.Add();
        meshInfo.BaseVertex  = baseVertex;
        meshInfo.VertexCount = vertexCount;
        meshInfo.FirstIndex  = firstIndex;
        meshInfo.IndexCount  = indexCount;
        meshInfo.MaterialNum = uniqueMaterials[pMaterial->map_Kd.path];
        meshInfo.BoundingBox = bounds;

        baseVertex += vertexCount;
        firstIndex += indexCount;

        Geometry::CalcTangentSpace(m_Vertices.ToPtr() + meshInfo.BaseVertex, meshInfo.VertexCount, m_Indices.ToPtr() + meshInfo.FirstIndex, meshInfo.IndexCount);
    }

    m_bSkeletal = false;

    return true;
}

HK_NAMESPACE_END
