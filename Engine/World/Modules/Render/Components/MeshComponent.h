#pragma once

#include <Engine/World/Modules/Skeleton/SkeletalAnimation.h>
#include <Engine/World/Modules/Render/ProceduralMesh.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Resources/Resource_MaterialInstance.h>

HK_NAMESPACE_BEGIN

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
