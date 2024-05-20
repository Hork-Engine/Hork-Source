#pragma once

#include <Engine/World/Modules/Skeleton/SkeletalAnimation.h>
#include <Engine/World/Modules/Render/ProceduralMesh.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Resources/Resource_MaterialInstance.h>

#include <Engine/World/Modules/Transform/EntityGraph.h>

HK_NAMESPACE_BEGIN

struct MeshInstance
{
    static constexpr size_t MAX_LAYERS = 4;

    EntityNodeID        Node;
    MeshHandle          Resource;
    int                 SubmeshIndex{};
    int                 NumLayers{1};
    BvAxisAlignedBox    BoundingBox;
    BvAxisAlignedBox    m_WorldBoundingBox;
    MaterialInstance*   Materials[MAX_LAYERS] = {};
    TRef<SkeletonPose>  Pose;
    bool                bOutline{};
};

struct MeshRenderer
{
    TVector<TUniqueRef<MeshInstance>> Meshes;
};

struct MeshComponent_ECS
{
    static constexpr size_t MAX_LAYERS = 4;

    MeshHandle Mesh;

    int SubmeshIndex{};

    BvAxisAlignedBox BoundingBox;
    BvAxisAlignedBox m_WorldBoundingBox;

    int NumLayers{1};

    MaterialInstance* Materials[MAX_LAYERS] = {};

    TRef<SkeletonPose> Pose;

    bool bOutline{};
};

struct ProceduralMeshComponent_ECS
{
    static constexpr size_t MAX_LAYERS = 4;

    TRef<ProceduralMesh_ECS> Mesh;

    BvAxisAlignedBox BoundingBox;
    BvAxisAlignedBox m_WorldBoundingBox;

    int NumLayers{1};

    MaterialInstance* Materials[MAX_LAYERS] = {};

    bool bOutline{};
};

HK_NAMESPACE_END
