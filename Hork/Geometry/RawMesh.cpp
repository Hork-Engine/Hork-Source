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

#include <Hork/Core/Logger.h>
#include <Hork/Core/Containers/Hash.h>

#define FAST_OBJ_REALLOC        Hk::Core::GetHeapAllocator<Hk::HEAP_TEMP>().Realloc
#define FAST_OBJ_FREE           Hk::Core::GetHeapAllocator<Hk::HEAP_TEMP>().Free
#define FAST_OBJ_IMPLEMENTATION
#include <fast_obj/fast_obj.h>

#include <cgltf/cgltf.h>

#include <ufbx/ufbx.h>

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
    File file = File::sOpenRead(filename);
    if (!file)
    {
        LOG("Couldn't open {}\n", filename);
        return false;
    }

    StringView extension = PathUtils::sGetExt(filename);
    if (!extension.Icmp(".gltf") || !extension.Icmp(".glb"))
        return LoadGLTF(file, flags);
    if (!extension.Icmp(".fbx"))
        return LoadFBX(file, flags);
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
    using RawSurface =  RawMesh::Surface;
    using RawSkin =     RawMesh::Skin;
    using RawChannel =  RawAnimation::Channel;

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
        File file = File::sOpenRead(path);
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

    String path(PathUtils::sGetFilePath(stream.GetName()));
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

    bool calcTangents = false;
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

            if (texcoord)
                calcTangents = true;
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

    if (calcTangents)
    {
        surface->Tangents.Resize(surface->Positions.Size());
        Geometry::CalcTangentSpace(surface->Positions.ToPtr(), surface->TexCoords.ToPtr(), surface->Normals.ToPtr(), surface->Tangents.ToPtr(), surface->Indices.ToPtr(), surface->Indices.Size());
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

namespace
{
    Float3x4 UfbxToFloat34(ufbx_matrix const& m)
    {
        return Float3x4(m.m00, m.m01, m.m02, m.m03,
                        m.m10, m.m11, m.m12, m.m13,
                        m.m20, m.m21, m.m22, m.m23);
    }

    Float2 UfbxToFloat2(ufbx_vec2 const& v)
    {
        return Float2(v.x, v.y);
    }

    Float3 UfbxToFloat3(ufbx_vec3 const& v)
    {
        return Float3(v.x, v.y, v.z);
    }

    Quat UfbxToQuat(ufbx_quat const& v)
    {
        return Quat(v.w, v.x, v.y, v.z);
    }

    float QDot(Quat const& a, Quat const& b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
    }
}

class FbxReader
{
    using RawSkin =     RawMesh::Skin;
    using RawSurface =  RawMesh::Surface;

public:
    RawMeshLoadFlags    Flags;

    void                Read(RawMesh* rawMesh, ufbx_scene* scene, ufbx_allocator_opts const* allocator);

private:
    RawSkin*            ReadSkin(ufbx_skin_deformer* deformer);
    void                ReadMesh(ufbx_mesh* mesh, Float3x4 const& transform, uint16_t jointIndex);
    void                ReadMeshNode(ufbx_node *node);
    bool                ReadSkeletonNode(ufbx_node *node, int16_t parentIndex);
    void                ReadAnimation(ufbx_scene *scene, ufbx_anim_stack* stack);

    RawMesh*            m_RawMesh;
    Vector<RawSkin*>    m_Skins;
    HashMap<size_t, uint16_t> m_NodeToJoint;
    ufbx_allocator_opts const* m_Allocator;
};

void FbxReader::Read(RawMesh* rawMesh, ufbx_scene* scene, ufbx_allocator_opts const* allocator)
{
    ufbx_node* root = scene->root_node;
    if (!root)
        return;

    m_RawMesh = rawMesh;
    m_Skins.Clear();
    m_NodeToJoint.Clear();
    m_Allocator = allocator;

    if (!!(Flags & (RawMeshLoadFlags::Skeleton | RawMeshLoadFlags::Animation | RawMeshLoadFlags::Skins)))
    {
        if (root->bone)
        {
            ReadSkeletonNode(root, -1);
        }
        else
        {
            for (ufbx_node* child : root->children)
                ReadSkeletonNode(child, -1);
        }
    }

    if (!!(Flags & RawMeshLoadFlags::Surfaces))
    {
        ReadMeshNode(root);
    }

    if (!!(Flags & RawMeshLoadFlags::SingleAnimation))
        Flags |= RawMeshLoadFlags::Animation;

    if (!!(Flags & RawMeshLoadFlags::Animation))
    {
        if (!!(Flags & RawMeshLoadFlags::SingleAnimation))
        {
            if (scene->anim_stacks.count > 0)
                ReadAnimation(scene, scene->anim_stacks.data[scene->anim_stacks.count - 1]);
        }
        else
        {
            for (size_t n = 0; n < scene->anim_stacks.count; ++n)
                ReadAnimation(scene, scene->anim_stacks.data[n]);
        }
    }
}

RawMesh::Skin* FbxReader::ReadSkin(ufbx_skin_deformer* deformer)
{
    Vector<uint16_t> jointRemaps;
    Vector<Float3x4> inverseBindPoses;

    jointRemaps.Reserve(deformer->clusters.count);
    inverseBindPoses.Reserve(deformer->clusters.count);

    for (ufbx_skin_cluster* cluster : deformer->clusters)
    {
        jointRemaps.Add(m_NodeToJoint[(size_t)cluster->bone_node]);
        inverseBindPoses.Add(UfbxToFloat34(cluster->geometry_to_bone));
    }

    // Skip duplicates
    for (size_t skinIndex = 0; skinIndex < m_Skins.Size(); ++skinIndex)
    {
        auto* skin = m_Skins[skinIndex];

        if (deformer->clusters.count != skin->JointRemaps.Size())
            continue;

        if (memcmp(skin->JointRemaps.ToPtr(), jointRemaps.ToPtr(), deformer->clusters.count * sizeof(jointRemaps[0])) == 0)
        {
            bool equal = true;
            for (size_t j = 0; j < deformer->clusters.count && equal; ++j)
                equal = skin->InverseBindPoses[j].CompareEps(inverseBindPoses[j], std::numeric_limits<float>::epsilon());

            if (equal)
                return m_Skins[skinIndex];
        }
    }

    RawSkin* newSkin = m_RawMesh->AllocSkin();

    newSkin->JointRemaps = std::move(jointRemaps);
    newSkin->InverseBindPoses = std::move(inverseBindPoses);

    m_Skins.Add(newSkin);

    return newSkin;
}

void FbxReader::ReadMesh(ufbx_mesh* mesh, Float3x4 const& transform, uint16_t jointIndex)
{
    Vector<SkinVertex> skinVerticesTmp;
    RawSkin* skin = nullptr;

    if (!!(Flags & RawMeshLoadFlags::Skins) && mesh->skin_deformers.count > 0)
    {
        ufbx_skin_deformer* deformer = mesh->skin_deformers.data[0];
        skin = ReadSkin(deformer);

        for (size_t vi = 0; vi < mesh->num_vertices; ++vi)
        {
            size_t numWeights = 0;
            float totalWeight = 0.0f;
            float weights[4] = { 0.0f };
            uint16_t jointIndices[4] = { 0 };

            ufbx_skin_vertex vertexWeights = deformer->vertices.data[vi];
            for (size_t wi = 0; wi < vertexWeights.num_weights; ++wi)
            {
                if (numWeights >= 4)
                    break;

                ufbx_skin_weight weight = deformer->weights.data[vertexWeights.weight_begin + wi];

                if (weight.cluster_index >= MAX_SKELETON_JOINTS)
                    continue;

                float fweight = static_cast<float>(weight.weight);
                totalWeight += fweight;
                jointIndices[numWeights] = static_cast<uint16_t>(weight.cluster_index);
                weights[numWeights] = fweight;
                numWeights++;
            }

            SkinVertex& skinVert = skinVerticesTmp.EmplaceBack();

            if (totalWeight > 0.0f)
            {
                const float invSum = 255.0f / totalWeight;

                uint32_t quantizedSum = 0;
                for (size_t i = 0; i < 4; i++)
                {
                    skinVert.JointIndices[i] = jointIndices[i];

                    skinVert.JointWeights[i] = weights[i] * invSum;
                    quantizedSum += skinVert.JointWeights[i];
                }

                skinVert.JointWeights[0] += 255 - quantizedSum;
            }
            else
            {
                for (size_t i = 0; i < 4; i++)
                {
                    skinVert.JointIndices[i] = 0;
                    skinVert.JointWeights[i] = 0;
                }
            }
        }
    }

    size_t numTriIndices = mesh->max_face_triangles * 3;
    SmallVector<uint32_t, 32> triIndices(numTriIndices);

    Float3x4 inverseTransform = transform.Inversed();

    Float3x3 normalTransform;
    transform.DecomposeNormalMatrix(normalTransform);

    bool hasTexCoords = mesh->vertex_uv.exists;
    bool hasTangents = mesh->vertex_tangent.exists && mesh->vertex_bitangent.exists;

    for (ufbx_mesh_part& meshPart : mesh->material_parts)
    {
        if (meshPart.num_triangles == 0)
            continue;

        RawSurface* surface = m_RawMesh->AllocSurface();

        surface->Skin = skin;

        size_t numVertices = 0;
        for (size_t fi = 0; fi < meshPart.num_faces; fi++)
        {
            ufbx_face face = mesh->faces.data[meshPart.face_indices.data[fi]];
            size_t numTriangles = ufbx_triangulate_face(triIndices.ToPtr(), numTriIndices, mesh, face);

            for (size_t vi = 0; vi < numTriangles * 3; vi++)
            {
                uint32_t ix = triIndices[vi];

                surface->Positions.Add(UfbxToFloat3(ufbx_get_vertex_vec3(&mesh->vertex_position, ix)));
                surface->Normals.Add(UfbxToFloat3(ufbx_get_vertex_vec3(&mesh->vertex_normal, ix)).Normalized());

                if (hasTexCoords)
                {
                    Float2 texCoord = UfbxToFloat2(ufbx_get_vertex_vec2(&mesh->vertex_uv, ix));
                    texCoord.Y = 1.0f - texCoord.Y;
                    surface->TexCoords.Add(texCoord);
                }

                if (hasTangents)
                {
                    Float3 tangent = UfbxToFloat3(ufbx_get_vertex_vec3(&mesh->vertex_tangent, ix));
                    Float3 bitangent = UfbxToFloat3(ufbx_get_vertex_vec3(&mesh->vertex_bitangent, ix));
                    float handedness = Geometry::CalcHandedness(tangent, bitangent, surface->Normals.Last());
                    surface->Tangents.Add(Float4(tangent.X, tangent.Y, tangent.Z, handedness));
                }

                if (skin)
                    surface->SkinVerts.Add(skinVerticesTmp[mesh->vertex_indices.data[ix]]);

                numVertices++;
            }
        }

        ufbx_vertex_stream streams[5];
        size_t numStreams = 0;

        streams[numStreams].data = surface->Positions.ToPtr();
        streams[numStreams].vertex_count = numVertices;
        streams[numStreams].vertex_size = sizeof(surface->Positions[0]);
        numStreams++;

        streams[numStreams].data = surface->Normals.ToPtr();
        streams[numStreams].vertex_count = numVertices;
        streams[numStreams].vertex_size = sizeof(surface->Normals[0]);
        numStreams++;

        if (hasTexCoords)
        {
            streams[numStreams].data = surface->TexCoords.ToPtr();
            streams[numStreams].vertex_count = numVertices;
            streams[numStreams].vertex_size = sizeof(surface->TexCoords[0]);
            numStreams++;
        }

        if (hasTangents)
        {
            streams[numStreams].data = surface->Tangents.ToPtr();
            streams[numStreams].vertex_count = numVertices;
            streams[numStreams].vertex_size = sizeof(surface->Tangents[0]);
            numStreams++;
        }

        if (skin)
        {
            streams[numStreams].data = surface->SkinVerts.ToPtr();
            streams[numStreams].vertex_count = numVertices;
            streams[numStreams].vertex_size = sizeof(surface->SkinVerts[0]);
            numStreams++;
        }

        surface->Indices.Resize(numVertices);

        ufbx_error error;
        numVertices = ufbx_generate_indices(streams, numStreams, surface->Indices.ToPtr(), surface->Indices.Size(), m_Allocator, &error);
        HK_ASSERT(error.type == UFBX_ERROR_NONE);

        surface->Positions.Resize(numVertices);
        surface->Positions.ShrinkToFit();

        surface->Normals.Resize(numVertices);
        surface->Normals.ShrinkToFit();

        if (hasTexCoords)
        {
            surface->TexCoords.Resize(numVertices);
            surface->TexCoords.ShrinkToFit();
        }

        if (skin)
        {
            surface->SkinVerts.Resize(numVertices);
            surface->SkinVerts.ShrinkToFit();
        }

        if (!hasTangents && hasTexCoords)
        {
            surface->Tangents.Resize(numVertices);
            Geometry::CalcTangentSpace(surface->Positions.ToPtr(), surface->TexCoords.ToPtr(), surface->Normals.ToPtr(), surface->Tangents.ToPtr(), surface->Indices.ToPtr(), surface->Indices.Size());
        }

        if (!skin)
        {
            surface->InverseTransform = inverseTransform;

            for (size_t vi = 0; vi < numVertices; ++vi)
            {
                surface->Positions[vi] = transform * surface->Positions[vi];
                surface->Normals[vi] = normalTransform * surface->Normals[vi];
            }

            if (!surface->Tangents.IsEmpty())
            {
                auto& tangents = surface->Tangents;
                for (int v = 0; v < numVertices; ++v)
                {
                    Float3 t(tangents[v].X, tangents[v].Y, tangents[v].Z);
                    t = normalTransform * t;
                    tangents[v].X = t.X;
                    tangents[v].Y = t.Y;
                    tangents[v].Z = t.Z;
                }
            }
        }

        surface->BoundingBox.Clear();
        for (size_t vi = 0; vi < numVertices; ++vi)
            surface->BoundingBox.AddPoint(surface->Positions[vi]);

        surface->JointIndex = jointIndex;
    }
}

void FbxReader::ReadMeshNode(ufbx_node *node)
{
    if (!node)
        return;

    if (node->mesh)
    {
        uint16_t jointIndex = 0;
        if (!!(Flags & RawMeshLoadFlags::Skins))
        {
            jointIndex = m_NodeToJoint[(size_t)node];
            if (jointIndex >= m_RawMesh->Skeleton.Joints.Size())
                jointIndex = m_RawMesh->Skeleton.Joints.Size() - 1;
        }

        ReadMesh(node->mesh, UfbxToFloat34(node->geometry_to_world), jointIndex);
    }

    for (ufbx_node* child : node->children)
        ReadMeshNode(child);
}

bool FbxReader::ReadSkeletonNode(ufbx_node *node, int16_t parentIndex)
{
    auto& skeleton = m_RawMesh->Skeleton;
    if (skeleton.Joints.Size() >= MAX_SKELETON_JOINTS)
    {
        LOG("Too many skeleton joints\n");
        return false;
    }

    int16_t jointIndex = skeleton.Joints.Size();

    auto& joint = skeleton.Joints.EmplaceBack();
    joint.Parent = parentIndex;
    if (node->name.length)
        joint.Name = SmallString(StringView(node->name.data, node->name.length));
    else
        joint.Name = SmallString(HK_FORMAT("j_{}", jointIndex));
    joint.Position = UfbxToFloat3(node->local_transform.translation);
    joint.Rotation = UfbxToQuat(node->local_transform.rotation);
    joint.Scale = UfbxToFloat3(node->local_transform.scale);    

    m_NodeToJoint[(size_t)node] = jointIndex;

    for (ufbx_node* child : node->children)
        if (!ReadSkeletonNode(child, jointIndex))
            return false;
    return true;
}

void FbxReader::ReadAnimation(ufbx_scene *scene, ufbx_anim_stack* stack)
{
    const size_t MaxFrames = 4096;
    const int DesiredFramerate = 30;
    constexpr float CompareEpsilon = std::numeric_limits<float>::epsilon();

    double duration = (stack->time_end > stack->time_begin) ? stack->time_end - stack->time_begin : 1.0;
    size_t frameCount = Math::Clamp<size_t>(duration * DesiredFramerate, 2, MaxFrames);

    double framerate = static_cast<double>(frameCount - 1) / duration;

    Vector<Quat> rot(frameCount);
    Vector<Float3> pos(frameCount);
    Vector<Float3> scale(frameCount);
    Vector<float> timestamps(frameCount);

    RawAnimation* rawAnimation = m_RawMesh->AllocAnimation();
    rawAnimation->Name = StringView(stack->name.data, stack->name.length);

    for (ufbx_node *node : scene->nodes)
    {
        auto it = m_NodeToJoint.Find((size_t)node);
        if (it == m_NodeToJoint.End())
            continue;

        uint16_t joint = it->second;

        bool isConstRotation = true;
        bool isConstPosition = true;
        bool isConstScale = true;

        for (size_t i = 0; i < frameCount; ++i)
        {
            double time = stack->time_begin + i / framerate;

            ufbx_transform transform = ufbx_evaluate_transform(stack->anim, node, time);
            rot[i] = UfbxToQuat(transform.rotation);
            pos[i] = UfbxToFloat3(transform.translation);
            scale[i] = UfbxToFloat3(transform.scale);
            timestamps[i] = time;

            if (i > 0)
            {
                if (QDot(rot[i], rot[i - 1]) < 0.0f)
                    rot[i] = -rot[i];

                if (rot[i - 1] != rot[i])
                    isConstRotation = false;
                if (pos[i - 1] != pos[i])
                    isConstPosition = false;
                if (scale[i - 1] != scale[i])
                    isConstScale = false;
            }
        }
        if (!isConstRotation)
        {
            auto& channel = rawAnimation->Channels.EmplaceBack();
            channel.Type = RawAnimation::Channel::ChannelType::Rotation;
            channel.Interpolation = RawAnimation::Channel::InterpolationType::Linear;
            channel.JointIndex = joint;
            channel.Data.Resize(frameCount * 4);
            channel.Timestamps.Reserve(frameCount);

            Quat* data = reinterpret_cast<Quat*>(channel.Data.ToPtr());
            data[0] = rot[0];
            channel.Timestamps.Add(timestamps[0]);
            size_t keyframeCount = 1;
            for (size_t i = 1; i < frameCount; ++i)
            {
                size_t keyframe = keyframeCount - 1;

                if (rot[i].CompareEps(data[keyframe], CompareEpsilon))
                    continue;

                data[keyframeCount++] = rot[i];
                channel.Timestamps.Add(timestamps[i]);
            }
            channel.Data.Resize(keyframeCount * 4);
            channel.Data.ShrinkToFit();
            channel.Timestamps.ShrinkToFit();
        }
        else if (!UfbxToQuat(node->local_transform.rotation).CompareEps(rot[0], CompareEpsilon))
        {
            auto& channel = rawAnimation->Channels.EmplaceBack();
            channel.Type = RawAnimation::Channel::ChannelType::Rotation;
            channel.Interpolation = RawAnimation::Channel::InterpolationType::Linear;
            channel.JointIndex = joint;
            channel.Data.Resize(4);
            Core::Memcpy(channel.Data.ToPtr(), rot.ToPtr(), 4 * sizeof(float));
            channel.Timestamps.Add(timestamps[0]);
        }

        if (!isConstPosition)
        {
            auto& channel = rawAnimation->Channels.EmplaceBack();
            channel.Type = RawAnimation::Channel::ChannelType::Translation;
            channel.Interpolation = RawAnimation::Channel::InterpolationType::Linear;
            channel.JointIndex = joint;
            channel.Data.Resize(frameCount * 3);
            channel.Timestamps.Reserve(frameCount);

            Float3* data = reinterpret_cast<Float3*>(channel.Data.ToPtr());
            data[0] = pos[0];
            channel.Timestamps.Add(timestamps[0]);
            size_t keyframeCount = 1;
            for (size_t i = 1; i < frameCount; ++i)
            {
                size_t keyframe = keyframeCount - 1;

                if (pos[i].CompareEps(data[keyframe], CompareEpsilon))
                    continue;

                data[keyframeCount++] = pos[i];
                channel.Timestamps.Add(timestamps[i]);
            }
            channel.Data.Resize(keyframeCount * 3);
            channel.Data.ShrinkToFit();
            channel.Timestamps.ShrinkToFit();
        }
        else if (!UfbxToFloat3(node->local_transform.translation).CompareEps(pos[0], CompareEpsilon))
        {
            auto& channel = rawAnimation->Channels.EmplaceBack();
            channel.Type = RawAnimation::Channel::ChannelType::Translation;
            channel.Interpolation = RawAnimation::Channel::InterpolationType::Linear;
            channel.JointIndex = joint;
            channel.Data.Resize(3);
            Core::Memcpy(channel.Data.ToPtr(), pos.ToPtr(), 3 * sizeof(float));
            channel.Timestamps.Add(timestamps[0]);
        }

        if (!isConstScale)
        {
            auto& channel = rawAnimation->Channels.EmplaceBack();
            channel.Type = RawAnimation::Channel::ChannelType::Scale;
            channel.Interpolation = RawAnimation::Channel::InterpolationType::Linear;
            channel.JointIndex = joint;
            channel.Data.Resize(frameCount * 3);
            channel.Timestamps.Reserve(frameCount);

            Float3* data = reinterpret_cast<Float3*>(channel.Data.ToPtr());
            data[0] = scale[0];
            channel.Timestamps.Add(timestamps[0]);
            size_t keyframeCount = 1;
            for (size_t i = 1; i < frameCount; ++i)
            {
                size_t keyframe = keyframeCount - 1;

                if (scale[i].CompareEps(data[keyframe], CompareEpsilon))
                    continue;

                data[keyframeCount++] = scale[i];
                channel.Timestamps.Add(timestamps[i]);
            }
            channel.Data.Resize(keyframeCount * 3);
            channel.Data.ShrinkToFit();
            channel.Timestamps.ShrinkToFit();
        }
        else if (!UfbxToFloat3(node->local_transform.scale).CompareEps(scale[0], CompareEpsilon))
        {
            auto& channel = rawAnimation->Channels.EmplaceBack();
            channel.Type = RawAnimation::Channel::ChannelType::Scale;
            channel.Interpolation = RawAnimation::Channel::InterpolationType::Linear;
            channel.JointIndex = joint;
            channel.Data.Resize(3);
            Core::Memcpy(channel.Data.ToPtr(), scale.ToPtr(), 3 * sizeof(float));
            channel.Timestamps.Add(timestamps[0]);
        }
    }
}

bool RawMesh::LoadFBX(IBinaryStreamReadInterface& stream, RawMeshLoadFlags flags)
{
    Purge();

    ufbx_load_opts opts = {};
    opts.clean_skin_weights = true;
    opts.load_external_files = false;
    opts.ignore_missing_external_files = true;
    opts.generate_missing_normals = true;
    opts.evaluate_skinning = false;
    opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
    opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
    opts.target_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
    opts.target_unit_meters = 1.0f;
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;

    if (!(flags & RawMeshLoadFlags::Surfaces))
        opts.ignore_geometry = true;

    if (!(flags & RawMeshLoadFlags::Skins))
        opts.skip_skin_vertices = true;

    if (!(flags & (RawMeshLoadFlags::Animation|RawMeshLoadFlags::SingleAnimation)))
        opts.ignore_animation = true;

    ufbx_allocator_opts allocator = {};

    allocator.allocator.alloc_fn = [](void *user, size_t size) -> void*
    {
        return Core::GetHeapAllocator<HEAP_TEMP>().Alloc(size, 0);
    };
    allocator.allocator.realloc_fn = [](void *user, void *old_ptr, size_t old_size, size_t new_size) -> void*
    {
        return Core::GetHeapAllocator<HEAP_TEMP>().Realloc(old_ptr, new_size, 0);
    };
    allocator.allocator.free_fn = [](void *user, void *ptr, size_t size)
    {
        Core::GetHeapAllocator<HEAP_TEMP>().Free(ptr);
    };
    allocator.allocator.free_allocator_fn = [](void *user) {};

    opts.result_allocator = allocator;
    opts.temp_allocator = allocator;

    struct Deleter
    {
        void operator()(ufbx_scene* scene) const
        {
            ufbx_free_scene(scene);
        };
    };

    ufbx_stream s{};

    s.read_fn = [](void *user, void *data, size_t size) -> size_t
    {
        IBinaryStreamReadInterface* stream = (IBinaryStreamReadInterface*)user;
        return stream->Read(data, size);
    };

    s.skip_fn = [](void *user, size_t size) -> bool
    {
        IBinaryStreamReadInterface* stream = (IBinaryStreamReadInterface*)user;
        return stream->SeekCur(size);
    };

    s.user = &stream;

    ufbx_error error;
    std::unique_ptr<ufbx_scene, Deleter> scene(ufbx_load_stream(&s, &opts, &error));
    if (!scene)
        return false;

    FbxReader reader;
    reader.Flags = flags;
    reader.Read(this, scene.get(), &allocator);

    return true;
}

HK_NAMESPACE_END
