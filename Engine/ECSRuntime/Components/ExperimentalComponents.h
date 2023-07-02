#pragma once

#include <Engine/ECS/ECS.h>

#include "../Resources/Resource_Mesh.h"
#include "../Resources/Resource_MaterialInstance.h"
#include "../ProceduralMesh.h"
#include "../SkeletalAnimation.h"

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Quat.h>

HK_NAMESPACE_BEGIN

struct TeleportComponent
{
    Float3 DestPosition;
    Quat DestRotation;
};

struct TeleportationComponent
{
    Float3 DestPosition;
    Quat DestRotation;

    TeleportationComponent(Float3 destPosition,
                           Quat destRotation) :
        DestPosition(destPosition),
        DestRotation(destRotation)
    {}
};

struct ActivatorComponent
{
    enum ActivationMode : uint8_t
    {
        ACTIVATE,
        DEACTVATE,
        TOGGLE,
        KEEP
    };

    ECS::EntityHandle Target;

    struct
    {
        ActivationMode OnBeginOverlap = ACTIVATE;
        ActivationMode OnEndOverlap = KEEP;
    } TriggerEvent;
};

struct ActorComponent_ECS
{
    ECS::EntityHandle Root;
};

struct ActiveComponent
{
    bool bIsActive = true;
};

struct JumpadComponent
{
    Float3 ThrowVelocity;
};

struct SpawnerComponent
{
    float SpawnInterval = 0.5f;
    float m_NextThink = 0;
};

struct OneFrameEntityTag
{

};


struct DoorComponent
{
    Hk::Float3 Position;
    Hk::Float3 Direction;

    float m_MaxOpenDist{};
    float m_OpenSpeed{1};
    float m_CloseSpeed{1};

    enum DOOR_STATE
    {
        STATE_CLOSED,
        STATE_OPENED,
        STATE_OPENING,
        STATE_CLOSING
    };
    DOOR_STATE m_DoorState{STATE_CLOSED};
    float m_NextThinkTime{};
    float m_OpenDist{};
    bool m_bIsActive = false;
};

struct DoorActivatorComponent
{
    TStaticVector<ECS::EntityHandle, 2> Parts;
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

    Float3x4 TransformHistory;

    bool bOutline{};
};
struct SkeletonControllerComponent
{
    TRef<AnimationInstance> AnimInstance;
};
struct SkeletonPoseComponent
{
    TRef<SkeletonPose> Pose;
    MeshHandle Mesh;
};
struct ShadowCastComponent
{
    uint32_t m_CascadeMask{0};
};
struct ProceduralMeshComponent_ECS
{
    static constexpr size_t MAX_LAYERS = 4;

    TRef<ProceduralMesh_ECS> Mesh;

    BvAxisAlignedBox BoundingBox;
    BvAxisAlignedBox m_WorldBoundingBox;

    int NumLayers{1};

    MaterialInstance* Materials[MAX_LAYERS] = {};

    Float3x4 TransformHistory;

    bool bOutline{};
};


// If Dynamic tag attached, effective light color will be recaluclated on each frame
struct DynamicLightTag
{
};

// If ShadowCast tag attached, light will cast shadows
struct ShadowCastTag
{
};

struct DirectionalLightComponent_ECS
{
    // Public

    void SetTemperature(float temperature)
    {
        m_Temperature = temperature;
    }

    float GetTemperature() const
    {
        return m_Temperature;
    }

    void SetColor(Float3 const& color)
    {
        m_Color = color;
    }

    Float3 const& GetColor() const
    {
        return m_Color;
    }

    void SetIlluminance(float illuminanceInLux)
    {
        m_IlluminanceInLux = illuminanceInLux;
    }

    float GetIlluminance() const
    {
        return m_IlluminanceInLux;
    }

    void SetDirection(Float3 const& direction)
    {
        m_Direction = direction.Normalized();
    }

    Float3 const& GetDirection() const
    {
        return m_Direction;
    }

    void SetShadowMaxDistance(float maxDistance)
    {
        m_ShadowMaxDistance = maxDistance;
    }

    float GetShadowMaxDistance() const
    {
        return m_ShadowMaxDistance;
    }

    void SetShadowCascadeResolution(int resolution)
    {
        m_ShadowCascadeResolution = Math::ToClosestPowerOfTwo(resolution);
    }

    int GetShadowCascadeResolution() const
    {
        return m_ShadowCascadeResolution;
    }

    void SetShadowCascadeOffset(float offset)
    {
        m_ShadowCascadeOffset = offset;
    }

    float GetShadowCascadeOffset() const
    {
        return m_ShadowCascadeOffset;
    }

    void SetShadowCascadeSplitLambda(float splitLambda)
    {
        m_ShadowCascadeSplitLambda = splitLambda;
    }

    float GetShadowCascadeSplitLambda() const
    {
        return m_ShadowCascadeSplitLambda;
    }

    void SetMaxShadowCascades(int maxShadowCascades)
    {
        m_MaxShadowCascades = Math::Clamp(maxShadowCascades, 1, MAX_SHADOW_CASCADES);
    }

    int GetMaxShadowCascades() const
    {
        return m_MaxShadowCascades;
    }

    // Private

    Float3         m_Color = Float3(1.0f);
    float          m_Temperature = 6590.0f;
    float          m_IlluminanceInLux = 110000.0f;
    Float4         m_EffectiveColor;
    Float3         m_Direction = Float3(0,-1,0);

    // TODO: move to CascadeShadowComponent?
    float          m_ShadowMaxDistance = 128;
    float          m_ShadowCascadeOffset = 3;
    int            m_MaxShadowCascades = 4;
    int            m_ShadowCascadeResolution = 1024;
    float          m_ShadowCascadeSplitLambda = 0.5f;    
};

struct PunctualLightComponent_ECS
{
    static constexpr float MinRadius = 0.01f;
    static constexpr float MinConeAngle = 1.0f;
    static constexpr float MaxConeAngle = 180.0f;

    // Public
    
    void SetLumens(float lumens)
    {
        m_Lumens = Math::Max(0.0f, lumens);
    }

    float GetLumens() const
    {
        return m_Lumens;
    }

    void SetTemperature(float temperature)
    {
        m_Temperature = temperature;
    }

    float GetTemperature() const
    {
        return m_Temperature;
    }

    void SetColor(Float3 const& color)
    {
        m_Color = color;
    }

    Float3 const& GetColor() const
    {
        return m_Color;
    }

    void SetRadius(float radius)
    {
        m_Radius = Math::Max(MinRadius, radius);
        m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    }

    float GetRadius() const
    {
        return m_Radius;
    }

    void SetInnerConeAngle(float angle)
    {
        m_InnerConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
        m_CosHalfInnerConeAngle = Math::Cos(Math::Radians(m_InnerConeAngle * 0.5f));
    }

    float GetInnerConeAngle() const
    {
        return m_InnerConeAngle;
    }

    void SetOuterConeAngle(float angle)
    {
        m_OuterConeAngle = Math::Clamp(angle, MinConeAngle, MaxConeAngle);
        m_CosHalfOuterConeAngle = Math::Cos(Math::Radians(m_OuterConeAngle * 0.5f));
    }

    float GetOuterConeAngle() const
    {
        return m_OuterConeAngle;
    }

    void SetSpotExponent(float exponent)
    {
        m_SpotExponent = exponent;
    }

    float GetSpotExponent() const
    {
        return m_SpotExponent;
    }

    void SetPhotometric(uint16_t id)
    {
        m_PhotometricProfileId = id;
    }

    uint16_t GetPhotometric() const
    {
        return m_PhotometricProfileId;
    }

    void SetPhotometricAsMask(bool bPhotometricAsMask)
    {
        m_bPhotometricAsMask = bPhotometricAsMask;
    }

    bool IsPhotometricAsMask() const
    {
        return m_bPhotometricAsMask;
    }

    /** Luminous intensity scale for photometric profile */
    void SetLuminousIntensityScale(float intensityScale)
    {
        m_LuminousIntensityScale = intensityScale;
    }

    float GetLuminousIntensityScale() const
    {
        return m_LuminousIntensityScale;
    }

    // Private

    BvSphere         m_SphereWorldBounds;
    BvOrientedBox    m_OBBWorldBounds;

    BvAxisAlignedBox m_AABBWorldBounds;
    Float4x4         m_OBBTransformInverse;
    uint32_t         m_PrimID;

    Float3           m_Color = Float3(1.0f);
    float            m_Temperature = 6590.0f;
    float            m_Lumens = 3000.0f;
    float            m_LuminousIntensityScale = 1;
    Float3           m_EffectiveColor; // Composed from Temperature, Lumens, Color
    uint16_t         m_PhotometricProfileId = 0;
    bool             m_bPhotometricAsMask = false;
    float            m_Radius = 15;
    float            m_InverseSquareRadius = 1.0f / (m_Radius * m_Radius);
    float            m_InnerConeAngle = 180;
    float            m_OuterConeAngle = 180;
    float            m_CosHalfInnerConeAngle = 0;
    float            m_CosHalfOuterConeAngle = 0;
    float            m_SpotExponent = 1.0f;
};

struct LightAnimationComponent_ECS
{
    //TRef<LightAnimation> Anim; // todo: Keyframes to perform light transitions
    float Time{};
};

struct EnvironmentProbeComponent
{
    BvAxisAlignedBox BoundingBox; // FIXME: Or OBB?
    uint32_t m_PrimID;
    uint32_t m_ProbeIndex; // Probe index inside level
};

struct SpringArmComponent
{
    static constexpr float SPRING_ARM_SPHERE_CAST_RADIUS = 0.3f;

    float DesiredDistance{};
    float ActualDistance{};
    float MinDistance{0.2f};
    float Speed{2};
    //float MaxVelocity{2};
    //float MinVelocity{0.01f};
};

HK_NAMESPACE_END
