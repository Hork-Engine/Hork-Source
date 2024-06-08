#pragma once

#include <Engine/World/Component.h>
#include <Engine/World/Modules/Render/ProceduralMesh.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Resources/Resource_MaterialInstance.h>

HK_NAMESPACE_BEGIN

class MeshComponent : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    struct Surface
    {
        TVector<MaterialInstance*>  Materials;
        //BvAxisAlignedBox            BoundingBox;
        //BvAxisAlignedBox            WorldBoundingBox;
    };

    MeshHandle              m_Resource;
    TVector<Surface>        m_Surfaces;
    TRef<SkeletonPose>      m_Pose;
    bool                    m_Outline = false;
    bool                    m_CastShadow = true;
    uint32_t                m_CascadeMask = 0;

    // Transform from current and previous fixed state
    Float3                  m_Position[2];
    Quat                    m_Rotation[2];
    Float3                  m_Scale[2];
    // Interpolated transform
    Float3                  m_LerpPosition;
    Quat                    m_LerpRotation;
    Float3                  m_LerpScale;
    // Transform from previous frame
    Float3                  m_PrevPosition;
    Quat                    m_PrevRotation;
    Float3                  m_PrevScale;

    void                    BeginPlay();
    void                    EndPlay();

    void                    UpdateBoundingBox();

    void                    DrawDebug(DebugRenderer& renderer);
};

class ProceduralMeshComponent : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    struct Surface
    {
        TVector<MaterialInstance*>  Materials;
        //BvAxisAlignedBox            BoundingBox;
        //BvAxisAlignedBox            WorldBoundingBox;
    };

    TRef<ProceduralMesh_ECS> m_Mesh;
    Surface                 m_Surface;
    bool                    m_Outline = false;
    bool                    m_CastShadow = true;

    // Transform from current and previous fixed state
    Float3                  m_Position[2];
    Quat                    m_Rotation[2];
    Float3                  m_Scale[2];
    // Interpolated transform
    Float3                  m_LerpPosition;
    Quat                    m_LerpRotation;
    Float3                  m_LerpScale;
    // Transform from previous frame
    Float3                  m_PrevPosition;
    Quat                    m_PrevRotation;
    Float3                  m_PrevScale;

    void                    UpdateBoundingBox();

    void                    DrawDebug(DebugRenderer& renderer);
};

HK_NAMESPACE_END
