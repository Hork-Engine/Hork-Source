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

#include "RawMesh.h"
#include "Utilites.h"
#include "TangentSpace.h"

#include <Engine/Core/Logger.h>
#include <Engine/Core/Containers/Hash.h>

#define FAST_OBJ_REALLOC        Hk::Core::GetHeapAllocator<Hk::HEAP_TEMP>().Realloc
#define FAST_OBJ_FREE           Hk::Core::GetHeapAllocator<Hk::HEAP_TEMP>().Free
#define FAST_OBJ_IMPLEMENTATION
#include <fast_obj/fast_obj.h>

#include <cgltf/cgltf.h>

#undef free

HK_NAMESPACE_BEGIN

RawMesh::Surface* RawMesh::AllocSurface()
{
    Surfaces.EmplaceBack(new Surface);
    return Surfaces.Last().RawPtr();
}

RawMesh::Skin* RawMesh::AllocSkin()
{
    Skins.EmplaceBack(new Skin);
    return Skins.Last().RawPtr();
}

RawAnimation* RawMesh::AllocAnimation()
{
    Animations.EmplaceBack(new RawAnimation);
    return Animations.Last().RawPtr();
}

void RawMesh::Purge()
{
    Surfaces.Clear();
    Skins.Clear();
    Skeleton.Joints.Clear();
    Animations.Clear();
}

void RawMesh::CreateBox(Float3 const& extents, float texCoordScale)
{
    Surface* surface = AllocSurface();
    Geometry::CreateBoxMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, extents, texCoordScale);
}

void RawMesh::CreateSphere(float radius, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs)
{
    Surface* surface = AllocSurface();
    Geometry::CreateSphereMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, radius, texCoordScale, numVerticalSubdivs, numHorizontalSubdivs);
}

void RawMesh::CreatePlaneXZ(float width, float height, Float2 const& texCoordScale)
{
    Surface* surface = AllocSurface();
    Geometry::CreatePlaneMeshXZ(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, width, height, texCoordScale);
}

void RawMesh::CreatePlaneXY(float width, float height, Float2 const& texCoordScale)
{
    Surface* surface = AllocSurface();
    Geometry::CreatePlaneMeshXY(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, width, height, texCoordScale);
}

void RawMesh::CreatePatch(Float3 const& corner00, Float3 const& corner10, Float3 const& corner01, Float3 const& corner11, float texCoordScale, bool isTwoSided, int numVerticalSubdivs, int numHorizontalSubdivs)
{
    Surface* surface = AllocSurface();
    Geometry::CreatePatchMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, corner00, corner10, corner01, corner11, texCoordScale, isTwoSided, numVerticalSubdivs, numHorizontalSubdivs);
}

void RawMesh::CreateCylinder(float radius, float height, float texCoordScale, int numSubdivs)
{
    Surface* surface = AllocSurface();
    Geometry::CreateCylinderMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, radius, height, texCoordScale, numSubdivs);
}

void RawMesh::CreateCone(float radius, float height, float texCoordScale, int numSubdivs)
{
    Surface* surface = AllocSurface();
    Geometry::CreateConeMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, radius, height, texCoordScale, numSubdivs);
}

void RawMesh::CreateCapsule(float radius, float height, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs)
{
    Surface* surface = AllocSurface();
    Geometry::CreateCapsuleMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, radius, height, texCoordScale, numVerticalSubdivs, numHorizontalSubdivs);
}

void RawMesh::CreateSkybox(Float3 const& extents, float texCoordScale)
{
    Surface* surface = AllocSurface();
    Geometry::CreateSkyboxMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, extents, texCoordScale);
}

void RawMesh::CreateSkydome(float radius, float texCoordScale, int numVerticalSubdivs, int numHorizontalSubdivs, bool isHemisphere)
{
    Surface* surface = AllocSurface();
    Geometry::CreateSkydomeMesh(surface->Positions, surface->TexCoords, surface->Normals, surface->Tangents, surface->Indices, surface->BoundingBox, radius, texCoordScale, numVerticalSubdivs, numHorizontalSubdivs, isHemisphere);
}

BvAxisAlignedBox RawMesh::CalcBoundingBox() const
{
    BvAxisAlignedBox bounds;
    bounds.Clear();
    for (auto& surface : Surfaces)
        bounds.AddAABB(surface->BoundingBox);
    return bounds;
}

bool RawMesh::Load(StringView filename, RawMeshLoadFlags flags)
{
    File file = File::OpenRead(filename);
    if (!file)
    {
        LOG("Couldn't open {}\n", filename);
        return false;
    }

    StringView extension = PathUtils::GetExt(filename);
    if (!extension.Icmp(".gltf") || !extension.Icmp(".glb"))
        return LoadGLTF(file, flags);
    if (!extension.Icmp(".obj"))
        return LoadOBJ(file, flags);

    LOG("Unexpected mesh format {}\n", filename);
    return {};
}

bool RawMesh::LoadOBJ(IBinaryStreamReadInterface& stream, RawMeshLoadFlags flags)
{
    Purge();

    if (!!(flags & RawMeshLoadFlags::Surfaces))
    {
        // Nothing to load, not an error.
        return true;
    }

    struct UserData
    {
        bool isObj;
        IBinaryStreamReadInterface* stream;
    };

    UserData userData;

    userData.isObj = true;
    userData.stream = &stream;

    fastObjCallbacks callbacks;
    callbacks.file_open = [](const char* path, void* user_data) -> void*
    {
        // Reset isObj flag on first use to ignore loading materials
        if (((UserData*)user_data)->isObj)
        {
            ((UserData*)user_data)->isObj = false;
            return (void*)1;
        }
        return nullptr;
    };
    callbacks.file_close = [](void*, void*) {};
    callbacks.file_read = [](void* file, void* dst, size_t bytes, void* user_data) -> size_t
    {
        return ((UserData*)user_data)->stream->Read(dst, bytes);
    };
    callbacks.file_size = [](void*, void* ) -> unsigned long
    {
        // Only used for materials, so we just return 0 here
        return 0;
    };

    fastObjMesh* mesh = fast_obj_read_with_callbacks("", &callbacks, &userData);
    if (!mesh)
    {
        LOG("Failed to load {}\n", stream.GetName());
        return false;
    }

    struct Vertex
    {
        Float3 Position;
        Float2 TexCoord;
        Float3 Normal;

        Vertex() = default;
        Vertex(Float3 const& position, Float2 const& texCoord, Float3 const& normal) :
            Position(position), TexCoord(texCoord), Normal(normal)
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

    HashMap<unsigned int, Vector<Vertex>> vertexList;
    HashMap<Vertex, unsigned int>         vertexHash;
    bool                                  hasUnsupportedVertexCount = false;
    bool                                  hasTexCoords = false;
    bool                                  hasNormals = false;

    for (unsigned int groupIndex = 0; groupIndex < mesh->group_count; ++groupIndex)
    {
        fastObjGroup const* group        = &mesh->groups[groupIndex];
        fastObjIndex const* groupIndices = &mesh->indices[group->index_offset];

        unsigned int indexNum = 0;
        for (unsigned int faceIndex = 0; faceIndex < group->face_count; ++faceIndex)
        {
            unsigned int vertexCount = mesh->face_vertices[group->face_offset + faceIndex];
            unsigned int material    = mesh->face_materials[group->face_offset + faceIndex];

            Vector<Vertex>& vertices = vertexList[material];

            if (vertexCount == 3)
            {
                for (unsigned int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    fastObjIndex index = groupIndices[indexNum++];

                    Vertex& v = vertices.Add();

                    v.Position.X = mesh->positions[index.p * 3 + 0];
                    v.Position.Y = mesh->positions[index.p * 3 + 1];
                    v.Position.Z = mesh->positions[index.p * 3 + 2];

                    v.TexCoord.X = mesh->texcoords[index.t * 2 + 0];
                    v.TexCoord.Y = mesh->texcoords[index.t * 2 + 1];

                    v.Normal.X = mesh->normals[index.n * 3 + 0];
                    v.Normal.Y = mesh->normals[index.n * 3 + 1];
                    v.Normal.Z = mesh->normals[index.n * 3 + 2];

                    hasTexCoords |= index.t != 0;
                    hasNormals |= index.n != 0;
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

                        v.Position.X = mesh->positions[index.p * 3 + 0];
                        v.Position.Y = mesh->positions[index.p * 3 + 1];
                        v.Position.Z = mesh->positions[index.p * 3 + 2];

                        v.TexCoord.X = mesh->texcoords[index.t * 2 + 0];
                        v.TexCoord.Y = mesh->texcoords[index.t * 2 + 1];

                        v.Normal.X = mesh->normals[index.n * 3 + 0];
                        v.Normal.Y = mesh->normals[index.n * 3 + 1];
                        v.Normal.Z = mesh->normals[index.n * 3 + 2];

                        hasTexCoords |= index.t != 0;
                        hasNormals |= index.n != 0;
                    }
                }
            }
            else
            {
                hasUnsupportedVertexCount = true;

                // TODO: Triangulate
            }
        }
    }

    if (hasUnsupportedVertexCount)
    {
        LOG("LoadOBJ: The mesh contains polygons with an unsupported number of vertices. Polygons are expected to have 3 or 4 vertices.\n");
    }

    unsigned int firstIndex = 0;

    for (auto& it : vertexList)
    {
        Vector<Vertex>& vertices = it.second;

        if (vertices.IsEmpty())
            continue;

        Surface* surface = AllocSurface();

        surface->BoundingBox.Clear();

        vertexHash.Clear();
        for (Vertex const& v : vertices)
        {
            auto vertexIt = vertexHash.Find(v);
            if (vertexIt == vertexHash.End())
            {
                vertexHash[v] = surface->Positions.Size();

                surface->Positions.Add(v.Position);
                if (hasTexCoords)
                    surface->TexCoords.EmplaceBack(v.TexCoord.X, 1.0f - v.TexCoord.Y);
                if (hasNormals)
                    surface->Normals.Add(v.Normal);

                surface->BoundingBox.AddPoint(v.Position);
            }
        }

        unsigned int indexCount = vertices.Size();

        surface->Indices.Resize(indexCount);
        for (unsigned int i = 0; i < indexCount; ++i)
            surface->Indices[firstIndex + i] = vertexHash[vertices[i]];

        //if (hasNormals && hasTexcoords)
        //{
        //    surface->Tangents.Resize(surface->Positions.Size());
        //    Geometry::CalcTangentSpace(surface->Positions.ToPtr(), surface->TexCoords.ToPtr(), surface->Normals.ToPtr(), surface->Positions.Size(), surface->Indices.ToPtr(), surface->Indices.Size());
        //}
    }

    fast_obj_destroy(mesh);

    return true;
}

namespace
{
    const char* GetErrorString(cgltf_result code)
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
}

class GltfReader
{
    using RawSurface = RawMesh::Surface;
    using RawSkin = RawMesh::Skin;
    using RawChannel = RawAnimation::Channel;

public:
    RawMeshLoadFlags    Flags;

    void                Read(RawMesh* rawMesh, cgltf_data* data);

private:
    bool                ReadSkeletonNode(cgltf_node* node, int parentIndex);
    RawSkin*            ReadSkin(cgltf_skin* skin);
    void                ReadNode(cgltf_node* node);
    void                ReadMesh(cgltf_node* node);
    void                ReadPrimitive(cgltf_primitive* prim, cgltf_skin* skin, uint16_t jointIndex, Float3x4 const& transform, Float3x3 const& normalTransform);
    void                ReadAnimations(cgltf_data* data);
    void                ReadAnimation(cgltf_animation* animation, int animIndex);

    RawMesh*            m_RawMesh;
    RawSkeleton*        m_Skeleton;
    int                 m_SceneIndex;
    Vector<cgltf_skin*> m_Skins;
};

bool RawMesh::LoadGLTF(IBinaryStreamReadInterface& stream, RawMeshLoadFlags flags)
{
    Purge();

    HeapBlob blob = stream.AsBlob();

    cgltf_options options = {};

    options.memory.alloc = [](void* user, cgltf_size size)
    {
        return Core::GetHeapAllocator<HEAP_TEMP>().Alloc(Math::Max(cgltf_size(1), size));
    };
    options.memory.free = [](void* user, void* ptr)
    {
        return Core::GetHeapAllocator<HEAP_TEMP>().Free(ptr);
    };

    options.file.read = [](const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data) -> cgltf_result
    {
        File file = File::OpenRead(path);
        if (!file)
        {
            LOG("Couldn't open {}\n", path);
            return cgltf_result_file_not_found;
        }

        *size = file.SizeInBytes();
        *data = memory_options->alloc(nullptr, *size);
        if (!*data)
        {
            return cgltf_result_out_of_memory;
        }

        file.Read(*data, *size);

        return cgltf_result_success;
    };

    options.file.release = [](const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, void* data)
    {
        memory_options->free(nullptr, data);
    };

    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse(&options, blob.GetData(), blob.Size(), &data);
    if (result != cgltf_result_success)
    {
        LOG("Couldn't load {} : {}\n", stream.GetName(), GetErrorString(result));
        return false;
    }

    struct Deleter
    {
        void operator()(cgltf_data* data) const
        {
            if (data)
                cgltf_free(data);
        };
    };

    std::unique_ptr<cgltf_data, Deleter> tempData(data);

    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        LOG("Couldn't load {} : {}\n", stream.GetName(), GetErrorString(result));
        return false;
    }

    String path(PathUtils::GetFilePath(stream.GetName()));
    path += "/";

    result = cgltf_load_buffers(&options, data, path.CStr());
    if (result != cgltf_result_success)
    {
        LOG("Couldn't load {} buffers : {}\n", stream.GetName(), GetErrorString(result));
        return false;
    }

    GltfReader reader;
    reader.Flags = flags;
    reader.Read(this, data);

    return true;
}

namespace
{

    void UnpackMat4ToFloat3x4(cgltf_accessor* acc, Float3x4* output, size_t maxCount, size_t stride)
    {
        if (!acc || acc->type != cgltf_type_mat4)
        {
            return;
        }

        byte* ptr = (byte*)output;

        Float4x4 temp;
        for (cgltf_size i = 0, count = Math::Min(acc->count, maxCount); i < count; i++)
        {
            cgltf_accessor_read_float(acc, i, (float*)temp.ToPtr(), 16);

            Core::Memcpy(ptr, temp.Transposed().ToPtr(), sizeof(Float3x4));

            ptr += stride;
        }
    }

    Float3x4 UnpackTransformAsFloat3x4(cgltf_node* node)
    {
        Float4x4 temp;
        cgltf_node_transform_world(node, (float*)temp.ToPtr());

        return Float3x4(temp.Transposed());
    }

    void UnpackVec2(cgltf_accessor* acc, Float2* output, size_t stride)
    {
        if (!acc || acc->type != cgltf_type_vec2)
        {
            return;
        }

        uint8_t* ptr = (uint8_t*)output;

        for (cgltf_size i = 0; i < acc->count; i++)
        {
            cgltf_accessor_read_float(acc, i, (float*)ptr, 2);

            ptr += stride;
        }
    }

    void UnpackVec2orVec3(cgltf_accessor* acc, Float3* output, size_t stride, bool normalize = false)
    {
        int num_elements;
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

        uint8_t* ptr = (uint8_t*)output;

        for (cgltf_size i = 0; i < acc->count; i++)
        {
            cgltf_accessor_read_float(acc, i, position, num_elements);

            if (normalize)
            {
                if (num_elements == 2)
                    ((Float2*)&position[0])->NormalizeSelf();
                else
                    ((Float3*)&position[0])->NormalizeSelf();
            }

            Core::Memcpy(ptr, position, sizeof(float) * 3);

            ptr += stride;
        }
    }

    void UnpackTangents(cgltf_accessor* acc, Float4* output)
    {
        if (!acc || acc->type != cgltf_type_vec4)
        {
            return;
        }

        for (cgltf_size i = 0; i < acc->count; i++)
        {
            cgltf_accessor_read_float(acc, i, &output->X, 4);

            output->W = output->W > 0.0f ? 1 : -1;
            output++;
        }
    }

    void UnpackWeights(cgltf_accessor* acc, SkinVertex* skinVertices)
    {
        float weight[4];

        if (!acc || acc->type != cgltf_type_vec4)
        {
            return;
        }

        for (cgltf_size i = 0; i < acc->count; i++, skinVertices++)
        {
            cgltf_accessor_read_float(acc, i, weight, 4);

            const float invSum = 255.0f / (weight[0] + weight[1] + weight[2] + weight[3]);

            uint32_t quantizedSum = 0;

            skinVertices->JointWeights[0] = weight[0] * invSum;
            quantizedSum += skinVertices->JointWeights[0];

            skinVertices->JointWeights[1] = weight[1] * invSum;
            quantizedSum += skinVertices->JointWeights[1];

            skinVertices->JointWeights[2] = weight[2] * invSum;
            quantizedSum += skinVertices->JointWeights[2];

            skinVertices->JointWeights[3] = weight[3] * invSum;
            quantizedSum += skinVertices->JointWeights[3];

            skinVertices->JointWeights[0] += 255 - quantizedSum;
        }
    }

    void UnpackJoints(cgltf_accessor* acc, SkinVertex* skinVertices, cgltf_size skinJointsCount)
    {
        float indices[4];

        if (!acc || acc->type != cgltf_type_vec4)
        {
            return;
        }

        bool warn = false;
        for (cgltf_size i = 0; i < acc->count; i++, skinVertices++)
        {
            cgltf_accessor_read_float(acc, i, indices, 4);

            warn |= (indices[0] < 0 || indices[0] >= skinJointsCount ||
                     indices[1] < 0 || indices[1] >= skinJointsCount ||
                     indices[2] < 0 || indices[2] >= skinJointsCount ||
                     indices[3] < 0 || indices[3] >= skinJointsCount);

            skinVertices->JointIndices[0] = Math::Clamp(indices[0], 0, skinJointsCount-1);
            skinVertices->JointIndices[1] = Math::Clamp(indices[1], 0, skinJointsCount-1);
            skinVertices->JointIndices[2] = Math::Clamp(indices[2], 0, skinJointsCount-1);
            skinVertices->JointIndices[3] = Math::Clamp(indices[3], 0, skinJointsCount-1);
        }

        if (warn)
            LOG("UnpackJoins: invalid joint index\n");
    }

}

void GltfReader::Read(RawMesh* rawMesh, cgltf_data* data)
{
    if (data->scenes_count == 0)
        return;

    if (!!(Flags & RawMeshLoadFlags::SingleAnimation))
        Flags |= RawMeshLoadFlags::Animation;

    m_RawMesh = rawMesh;

    m_Skins.Clear();
    m_Skeleton = &m_RawMesh->Skeleton;

    for (cgltf_size n = 0; n < data->nodes_count; ++n)
        data->nodes[n].camera = (cgltf_camera*)(size_t(MAX_SKELETON_JOINTS) | 0xffff0000);

    // Load only the first scene
    m_SceneIndex = 0;
    cgltf_scene* scene = &data->scenes[m_SceneIndex];

    if (!!(Flags & (RawMeshLoadFlags::Skeleton | RawMeshLoadFlags::Animation | RawMeshLoadFlags::Skins)))
    {
        for (cgltf_size n = 0; n < scene->nodes_count; ++n)
        {
            if (!ReadSkeletonNode(scene->nodes[n], -1))
                break; // too many joints
        }
    }

    if (!!(Flags & RawMeshLoadFlags::Surfaces))
    {
        for (cgltf_size n = 0; n < scene->nodes_count; ++n)
            ReadNode(scene->nodes[n]);
    }

    if (!!(Flags & RawMeshLoadFlags::Animation))
        ReadAnimations(data);
}

bool GltfReader::ReadSkeletonNode(cgltf_node* node, int parentIndex)
{
    if (m_Skeleton->Joints.Size() >= MAX_SKELETON_JOINTS)
    {
        LOG("Too many joints in skeleton\n");
        return false;
    }

    auto& joint = m_Skeleton->Joints.Add();

    if (node->has_matrix)
    {
        Float3x3 rotationMatrix;
        Float3x4(((Float4x4*)&node->matrix[0])->Transposed()).DecomposeAll(joint.Position, rotationMatrix, joint.Scale);
        joint.Rotation.FromMatrix(rotationMatrix);
        joint.Rotation.NormalizeSelf();
    }
    else
    {
        if (node->has_translation)
        {
            joint.Position.X = node->translation[0];
            joint.Position.Y = node->translation[1];
            joint.Position.Z = node->translation[2];
        }

        if (node->has_rotation)
        {
            joint.Rotation.X = node->rotation[0];
            joint.Rotation.Y = node->rotation[1];
            joint.Rotation.Z = node->rotation[2];
            joint.Rotation.W = node->rotation[3];
            joint.Rotation.NormalizeSelf();
        }

        if (node->has_scale)
        {
            joint.Scale.X = node->scale[0];
            joint.Scale.Y = node->scale[1];
            joint.Scale.Z = node->scale[2];
        }
        else
        {
            joint.Scale.X = 1;
            joint.Scale.Y = 1;
            joint.Scale.Z = 1;
        }
    }

    joint.Name = node->name ? SmallString(node->name) : SmallString(HK_FORMAT("j_{}", m_Skeleton->Joints.Size() - 1));
    joint.Parent = parentIndex;

    node->camera = (cgltf_camera*)(((size_t)m_Skeleton->Joints.Size() - 1) | (m_SceneIndex << 16));

    parentIndex = m_Skeleton->Joints.Size() - 1;

    for (cgltf_size n = 0; n < node->children_count; ++n)
        if (!ReadSkeletonNode(node->children[n], parentIndex))
            return false;
    return true;
}

auto GltfReader::ReadSkin(cgltf_skin* skin) -> RawSkin*
{
    for (int n = 0; n < m_Skins.Size(); ++n)
    {
        if (m_Skins[n] == skin)
            return m_RawMesh->Skins[n].RawPtr();
    }

    RawSkin* rawSkin = m_RawMesh->AllocSkin();

    m_Skins.Add(skin);

    rawSkin->JointRemaps.Resize(skin->joints_count);
    rawSkin->InverseBindPoses.Resize(skin->joints_count);

    size_t jointsInSkeleton = m_Skeleton->Joints.Size();

    // From GLT2 spec: Each skin is defined by a REQUIRED joints property that lists the indices
    //                 of nodes used as joints to pose the skin and an OPTIONAL inverseBindMatrices property.
    //                 The number of elements of the accessor referenced by inverseBindMatrices MUST greater
    //                 than or equal to the number of joints elements.
    UnpackMat4ToFloat3x4(skin->inverse_bind_matrices, rawSkin->InverseBindPoses.ToPtr(), rawSkin->InverseBindPoses.Size(), sizeof(rawSkin->InverseBindPoses[0]));

    bool warn = false;
    for (cgltf_size i = 0; i < skin->joints_count; ++i)
    {
        cgltf_node* joint =  skin->joints[i];

        size_t jointIndex = (size_t)joint->camera & 0xffff;
        if (jointIndex >= jointsInSkeleton)
        {
            // Invalid joint index
            jointIndex = jointsInSkeleton - 1;
            warn = true;
        }

        rawSkin->JointRemaps[i] = jointIndex;
    }

    if (warn)
        LOG("Invalid skin - joint index is out of range\n");

    return rawSkin;
}

void GltfReader::ReadNode(cgltf_node* node)
{
    ReadMesh(node);
    for (cgltf_size n = 0; n < node->children_count; ++n)
        ReadNode(node->children[n]);
}

void GltfReader::ReadMesh(cgltf_node* node)
{
    if (cgltf_mesh* mesh = node->mesh)
    {
        Float3x4 transform = UnpackTransformAsFloat3x4(node);

        Float3x3 normalTransform;
        transform.DecomposeNormalMatrix(normalTransform);

        cgltf_skin* skin = nullptr;
        uint16_t jointIndex = 0;

        if (!!(Flags & RawMeshLoadFlags::Skins))
        {
            skin = node->skin;

            jointIndex = (size_t)node->camera & 0xffff;
            if (jointIndex >= m_Skeleton->Joints.Size())
                jointIndex = m_Skeleton->Joints.Size() - 1;
        }

        for (cgltf_size i = 0; i < mesh->primitives_count; ++i)
        {
            cgltf_primitive* prim = &mesh->primitives[i];
            if (prim->type != cgltf_primitive_type_triangles)
                continue;

            // TODO: Support for cgltf_primitive_type_triangle_strip and cgltf_primitive_type_triangle_fan

            ReadPrimitive(prim, skin, jointIndex, transform, normalTransform);
        }
    }
}

void GltfReader::ReadPrimitive(cgltf_primitive* prim, cgltf_skin* skin, uint16_t jointIndex, Float3x4 const& transform, Float3x3 const& normalTransform)
{
    cgltf_accessor* position = nullptr;
    cgltf_accessor* normal   = nullptr;
    cgltf_accessor* tangent  = nullptr;
    cgltf_accessor* texcoord = nullptr;
    cgltf_accessor* texcoord2 = nullptr;
    cgltf_accessor* joints   = nullptr;
    cgltf_accessor* weights  = nullptr;

    // Find attributes
    for (cgltf_size a = 0; a < prim->attributes_count; a++)
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
            if (!texcoord)
                texcoord = attrib->data;
            else if (!texcoord2)
                texcoord2 = attrib->data;
            break;

        case cgltf_attribute_type_color:
            // We don't use colors
            break;

        case cgltf_attribute_type_joints:
            joints = attrib->data;
            break;

        case cgltf_attribute_type_weights:
            weights = attrib->data;
            break;
        }
    }

    if (!position || position->count == 0)
    {
        // Primitive has no positions
        return;
    }

    if (position->type != cgltf_type_vec2 && position->type != cgltf_type_vec3)
    {
        // Unexpected position type
        return;
    }

    RawSurface* surface = m_RawMesh->AllocSurface();

    auto vertexCount = position->count;

    surface->Positions.Resize(vertexCount);
    UnpackVec2orVec3(position, surface->Positions.ToPtr(), sizeof(Float3));

    if (texcoord && texcoord->type == cgltf_type_vec2 && texcoord->count == vertexCount)
    {
        surface->TexCoords.Resize(vertexCount);
        UnpackVec2(texcoord, surface->TexCoords.ToPtr(), sizeof(Float2));
    }

    if (texcoord2 && texcoord2->type == cgltf_type_vec2 && texcoord2->count == vertexCount)
    {
        surface->TexCoords2.Resize(vertexCount);
        UnpackVec2(texcoord2, surface->TexCoords2.ToPtr(), sizeof(Float2));
    }

    if (normal && (normal->type == cgltf_type_vec2 || normal->type == cgltf_type_vec3) && normal->count == vertexCount)
    {
        surface->Normals.Resize(vertexCount);
        UnpackVec2orVec3(normal, surface->Normals.ToPtr(), sizeof(Float3), true);

        if (tangent && (tangent->type == cgltf_type_vec4) && tangent->count == vertexCount)
        {
            surface->Tangents.Resize(vertexCount);
            UnpackTangents(tangent, surface->Tangents.ToPtr());
        }
        else
        {
            // Form GLTF2 spec: When tangents are not specified, client implementations SHOULD calculate tangents using default MikkTSpace
            //                  algorithms with the specified vertex positions, normals, and texture coordinates associated with the normal texture.
        }
    }
    else
    {
        // Form GLTF2 spec: When normals are not specified, client implementations MUST calculate flat normals
        //                  and the provided tangents (if present) MUST be ignored.
    }    

    if (skin && weights && (weights->type == cgltf_type_vec4) && weights->count == vertexCount && joints && (joints->type == cgltf_type_vec4) && joints->count == vertexCount)
    {
        surface->SkinVerts.Resize(vertexCount);
        UnpackWeights(weights, surface->SkinVerts.ToPtr());
        UnpackJoints(joints, surface->SkinVerts.ToPtr(), skin->joints_count);
    }

    if (prim->indices)
    {
        auto indexCount = prim->indices->count;
        surface->Indices.Resize(indexCount);
        unsigned int* ind = surface->Indices.ToPtr();
        for (int index = 0; index < indexCount; index++)
            ind[index] = cgltf_accessor_read_index(prim->indices, index);
    }
    else
    {
        auto indexCount = vertexCount;
        surface->Indices.Resize(indexCount);
        unsigned int* ind = surface->Indices.ToPtr();
        for (int index = 0; index < indexCount; index++)
            ind[index] = index;
    }

    surface->BoundingBox.Clear();

    if (!skin || surface->SkinVerts.IsEmpty())
    {
        // Apply node transform, calc bounding box
        auto& positions = surface->Positions;
        for (int v = 0; v < vertexCount; ++v)
        {
            positions[v] = transform * positions[v];
            surface->BoundingBox.AddPoint(positions[v]);
        }

        if (!surface->Normals.IsEmpty())
        {
            auto& normals = surface->Normals;
            for (int v = 0; v < vertexCount; ++v)
                normals[v] = normalTransform * normals[v];
        }

        if (!surface->Tangents.IsEmpty())
        {
            auto& tangents = surface->Tangents;
            for (int v = 0; v < vertexCount; ++v)
            {
                Float3 t(tangents[v].X, tangents[v].Y, tangents[v].Z);
                t = normalTransform * t;
                tangents[v].X = t.X;
                tangents[v].Y = t.Y;
                tangents[v].Z = t.Z;
            }
        }

        surface->InverseTransform = transform.Inversed();
    }
    else
    {
        // Calc bounding box for rest pose
        auto& positions = surface->Positions;
        for (int v = 0; v < vertexCount; ++v)
            surface->BoundingBox.AddPoint(transform * positions[v]);

        surface->Skin = ReadSkin(skin);
    }
    
    surface->JointIndex = jointIndex;
}

void GltfReader::ReadAnimations(cgltf_data* data)
{
    if (data->animations_count == 0)
        return;

    if (!!(Flags & RawMeshLoadFlags::SingleAnimation))
    {
        ReadAnimation(&data->animations[0], 0);
    }
    else
    {
        for (cgltf_size animIndex = 0; animIndex < data->animations_count; ++animIndex)
            ReadAnimation(&data->animations[animIndex], animIndex);
    }
}

namespace
{

    bool IsChannelValid(cgltf_animation_channel* channel)
    {
        cgltf_animation_sampler* sampler = channel->sampler;

        switch (channel->target_path)
        {
        case cgltf_animation_path_type_translation:
        case cgltf_animation_path_type_rotation:
        case cgltf_animation_path_type_scale:
        case cgltf_animation_path_type_weights:
            break;
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

        cgltf_accessor* timestamps = sampler->input;
        cgltf_accessor* data  = sampler->output;

        if (timestamps->count == 0)
        {
            LOG("Warning: empty channel data\n");
            return false;
        }

        if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
        {
            if (timestamps->count * 3 != data->count)
            {
                LOG("Warning: invalid channel data\n");
                return false;
            }
        }
        else
        {
            if (timestamps->count != data->count)
            {
                LOG("Warning: invalid channel data\n");
                return false;
            }
        }
        return true;
    }
}

void GltfReader::ReadAnimation(cgltf_animation* animation, int animIndex)
{
    RawAnimation* rawAnimation = m_RawMesh->AllocAnimation();

    rawAnimation->Name = animation->name ? animation->name : Core::ToString(animIndex);

    for (cgltf_size ch = 0; ch < animation->channels_count; ++ch)
    {
        cgltf_animation_channel* channel = &animation->channels[ch];
        cgltf_animation_sampler* sampler = channel->sampler;

        if (!IsChannelValid(channel))
            continue;

        auto sceneIndex = (size_t)channel->target_node->camera >> 16;
        if (sceneIndex != 0)
        {
            // The target node belongs to another scene
            break;
        }

        size_t jointIndex = (size_t)channel->target_node->camera & 0xffff;
        if (jointIndex >= m_Skeleton->Joints.Size())
        {
            LOG("Invalid joint index\n");
            continue;
        }

        RawChannel& rawChannel = rawAnimation->Channels.EmplaceBack();

        switch (channel->target_path)
        {
        case cgltf_animation_path_type_translation:
            rawChannel.Type = RawChannel::ChannelType::Translation;
            break;
        case cgltf_animation_path_type_rotation:
            rawChannel.Type = RawChannel::ChannelType::Rotation;
            break;
        case cgltf_animation_path_type_scale:
            rawChannel.Type = RawChannel::ChannelType::Scale;
            break;
        case cgltf_animation_path_type_weights:
            rawChannel.Type = RawChannel::ChannelType::Weights;
            break;
        default:
            HK_ASSERT(0);
            break;
        }

        switch (sampler->interpolation)
        {
        case cgltf_interpolation_type_linear:
            rawChannel.Interpolation = RawChannel::InterpolationType::Linear;
            break;
        case cgltf_interpolation_type_step:
            rawChannel.Interpolation = RawChannel::InterpolationType::Step;
            break;
        case cgltf_interpolation_type_cubic_spline:
            rawChannel.Interpolation = RawChannel::InterpolationType::CubicSpline;
            break;
        default:
            HK_ASSERT(0);
            break;
        }

        rawChannel.JointIndex = jointIndex;

        cgltf_accessor* timestamps = sampler->input;
        cgltf_accessor* data  = sampler->output;

        rawChannel.Timestamps.Resize(timestamps->count);
        cgltf_accessor_unpack_floats(timestamps, rawChannel.Timestamps.ToPtr(), rawChannel.Timestamps.Size());

        rawChannel.Data.Resize(data->count * cgltf_num_components(data->type));
        cgltf_accessor_unpack_floats(data, rawChannel.Data.ToPtr(), rawChannel.Data.Size());
    }
}

HK_NAMESPACE_END
