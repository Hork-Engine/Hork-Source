#include "LightingSystem.h"
#include <Engine/Core/ConsoleVar.h>
#include <Engine/World/Common/GameFrame.h>
#include <Engine/World/Resources/ResourceManager.h>
#include <Engine/World/Common/DebugRenderer.h>
#include <Engine/World/Modules/Transform/Components/RenderTransformComponent.h>
#include <Engine/World/Modules/Transform/Components/MovableTag.h>
#include "../Components/ExperimentalComponents.h"
#include "../Components/DynamicLightTag.h"

HK_NAMESPACE_BEGIN

ConsoleVar com_LightEnergyScale("com_LightEnergyScale"s, "16"s);
ConsoleVar com_DrawDirectionalLights("com_DrawDirectionalLights"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawPunctualLights("com_DrawPunctualLights"s, "0"s, CVAR_CHEAT);
ConsoleVar com_DrawEnvironmentProbes("com_DrawEnvironmentProbes"s, "0"s, CVAR_CHEAT);

#if 0
class EnvironmentMapCache
{
public:
    uint32_t AddMap(EnvironmentMapData const& data)
    {

    }

    void RemoveMap(uint32_t id)
    {

    }

    void Touch(uint32_t id)
    {
        if (m_NumPending < MAX_GPU_RESIDENT)
            m_Pending[m_NumPending++] = id;
        else
            LOG("MAX_GPU_RESIDENT hit\n");
    }

    void Update()
    {
        if (m_NumPending > 0)
        {
            int j = 0;
            for (int i = 0 ; i < m_NumPending ; i++)
            {
                auto& envMap = m_EnvMaps[m_Pending[i]];

                if (envMap.Index != -1)
                    m_Resident[envMap.Index].TouchTime = curtime;
                else
                    m_Pending[j] = m_Pending[i];
            }

            // Sort m_Resident by TouchTime

            for (int i = 0 ; i < MAX_GPU_RESIDENT && j > 0 ; i++)
            {
                if (m_Resident[i].TouchTime < curtime)
                {
                    m_Resident[i].TouchTime = curtime;
                    m_Resident[i].EnvMapID = m_Pending[--j];

                    auto& envMap = m_EnvMaps[m_Resident[i].EnvMapID];
                    envMap.Index = i;

                    UploadEnvironmentMap(i);
                }
            }

            HK_ASSERT(j == 0);

            m_NumPending = 0;
        }
    }

    void UploadEnvironmentMap(int index)
    {
        uint32_t id = m_Resident[index].EnvMapID;

        // TODO: Here upload from m_EnvMaps[id] to m_ReflectionMapArray[index], m_IrradianceMapArray[index]
    }

    // CPU cache
    TVector<TUniquePtr<EnvironmentMapData>> m_EnvMaps;
    TVector<uint32_t> m_FreeList;

    static constexpr uint32_t MAX_GPU_RESIDENT = 256;

    // GPU cache
    TRef<RenderCore::ITexture> m_IrradianceMapArray;
    TRef<RenderCore::ITexture> m_ReflectionMapArray;

    struct Resident
    {
        uint32_t EnvMapID;
        uint32_t TouchTime;
    };
    uint32_t m_Resident[MAX_GPU_RESIDENT];

    uint32_t m_Pending[MAX_GPU_RESIDENT];
    uint32_t m_NumPending{};

    
};
#endif

LightingSystem::LightingSystem(ECS::World* world) :
    m_World(world)
{
    world->AddEventHandler<ECS::Event::OnComponentAdded<DirectionalLightComponent>>(this);

    world->AddEventHandler<ECS::Event::OnComponentAdded<PunctualLightComponent>>(this);
    world->AddEventHandler<ECS::Event::OnComponentRemoved<PunctualLightComponent>>(this);
}

void LightingSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<DirectionalLightComponent> const& event)
{
    auto& light = event.Component();

    const float EnergyUnitScale = 1.0f / 100.0f / 100.0f;
    float energy = light.m_IlluminanceInLux * EnergyUnitScale;

    Color4 temperatureColor;
    temperatureColor.SetTemperature(light.m_Temperature);

    light.m_EffectiveColor[0] = light.m_Color[0] * temperatureColor[0] * energy;
    light.m_EffectiveColor[1] = light.m_Color[1] * temperatureColor[1] * energy;
    light.m_EffectiveColor[2] = light.m_Color[2] * temperatureColor[2] * energy;
}

void LightingSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<PunctualLightComponent> const& event)
{
    auto& light = event.Component();

    float candela;

    if (light.m_PhotometricProfileId && !light.m_bPhotometricAsMask)
    {
        candela = light.m_LuminousIntensityScale;// * light.PhotometricProfile->GetIntensity(); // TODO
    }
    else
    {
        float fCosHalfConeAngle;
        if (light.m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
            fCosHalfConeAngle = Math::Min(light.m_CosHalfOuterConeAngle, 0.9999f);
        else
            fCosHalfConeAngle = -1.0f;

        const float fLumensToCandela = 1.0f / Math::_2PI / (1.0f - fCosHalfConeAngle);

        candela = light.m_Lumens * fLumensToCandela;
    }

    Color4 temperatureColor;
    temperatureColor.SetTemperature(light.m_Temperature);

    const float EnergyUnitScale = 1.0f / com_LightEnergyScale.GetFloat();
    float fScale = candela * EnergyUnitScale;

    light.m_EffectiveColor[0] = light.m_Color[0] * temperatureColor[0] * fScale;
    light.m_EffectiveColor[1] = light.m_Color[1] * temperatureColor[1] * fScale;
    light.m_EffectiveColor[2] = light.m_Color[2] * temperatureColor[2] * fScale;

    //m_PendingAddLights.Add(event.GetEntity());

    // TODO: calc sphere world bounds

    //light.m_PrimID = m_SpatialTree.AddPrimitive(light.m_SphereWorldBounds);
    //m_SpatialTree.AssignEntity(light.m_PrimID, event.GetEntity());
}

void LightingSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<PunctualLightComponent> const& event)
{
    //auto& light = event.Component();

    //m_SpatialTree.RemovePrimitive(light.m_PrimID);

    //PendingDestroyLight(event.GetEntity(), light.m_PrimID);
}

void LightingSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<EnvironmentProbeComponent> const& event)
{
    //auto& probe = event.Component();

    //probe.m_PrimID = m_SpatialTree.AddPrimitive(probe.BoundingBox);
    //m_SpatialTree.AssignEntity(probe.m_PrimID, event.GetEntity());
}

void LightingSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<EnvironmentProbeComponent> const& event)
{
    //auto& probe = event.Component();

    //m_SpatialTree.RemovePrimitive(probe.m_PrimID);
}

LightingSystem::~LightingSystem()
{
    m_World->RemoveHandler(this);
}

void LightingSystem::Update(struct GameFrame const& frame)
{
    {
        using Query = ECS::Query<>
            ::Required<DirectionalLightComponent>
            ::ReadOnly<DynamicLightTag>;

        const float EnergyUnitScale = 1.0f / 100.0f / 100.0f;

        for (Query::Iterator it(*m_World); it; it++)
        {
            DirectionalLightComponent* lights = it.Get<DirectionalLightComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                auto& light = lights[i];

                float energy = light.m_IlluminanceInLux * EnergyUnitScale; // * GetAnimationBrightness();

                Color4 temperatureColor;
                temperatureColor.SetTemperature(light.m_Temperature);

                light.m_EffectiveColor[0] = light.m_Color[0] * temperatureColor[0] * energy;
                light.m_EffectiveColor[1] = light.m_Color[1] * temperatureColor[1] * energy;
                light.m_EffectiveColor[2] = light.m_Color[2] * temperatureColor[2] * energy;
            }
        }
    }

    {
        using Query = ECS::Query<>
            ::Required<PunctualLightComponent>
            ::ReadOnly<DynamicLightTag>;

        const float EnergyUnitScale = 1.0f / com_LightEnergyScale.GetFloat();

        for (Query::Iterator it(*m_World); it; it++)
        {
            PunctualLightComponent* lights = it.Get<PunctualLightComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                auto& light = lights[i];

                float candela;

                if (light.m_PhotometricProfileId && !light.m_bPhotometricAsMask)
                {
                    candela = light.m_LuminousIntensityScale;// * light.PhotometricProfile->GetIntensity(); // TODO
                }
                else
                {
                    float fCosHalfConeAngle;
                    if (light.m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
                        fCosHalfConeAngle = Math::Min(light.m_CosHalfOuterConeAngle, 0.9999f);
                    else
                        fCosHalfConeAngle = -1.0f;

                    const float fLumensToCandela = 1.0f / Math::_2PI / (1.0f - fCosHalfConeAngle);

                    candela = light.m_Lumens * fLumensToCandela;
                }

                // Animate light intensity
                //candela *= GetAnimationBrightness();

                Color4 temperatureColor;
                temperatureColor.SetTemperature(light.m_Temperature);

                float fScale = candela * EnergyUnitScale;

                light.m_EffectiveColor[0] = light.m_Color[0] * temperatureColor[0] * fScale;
                light.m_EffectiveColor[1] = light.m_Color[1] * temperatureColor[1] * fScale;
                light.m_EffectiveColor[2] = light.m_Color[2] * temperatureColor[2] * fScale;
            }
        }
    }
}

void LightingSystem::UpdateLightBounding(PunctualLightComponent& light, Float3 const& worldPosition, Quat const& worldRotation)
{
    if (light.m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
    {
        const float ToHalfAngleRadians = 0.5f / 180.0f * Math::_PI;
        const float HalfConeAngle = light.m_OuterConeAngle * ToHalfAngleRadians;
        const Float3 WorldPos = worldPosition;
        const float SinHalfConeAngle = Math::Sin(HalfConeAngle);

        // Compute cone OBB for voxelization
        light.m_OBBWorldBounds.Orient = worldRotation.ToMatrix3x3();

        const Float3 SpotDir = -light.m_OBBWorldBounds.Orient[2];

        //light.m_OBBWorldBounds.HalfSize.X = light.m_OBBWorldBounds.HalfSize.Y = tan( HalfConeAngle ) * light.m_Radius;
        light.m_OBBWorldBounds.HalfSize.X = light.m_OBBWorldBounds.HalfSize.Y = SinHalfConeAngle * light.m_Radius;
        light.m_OBBWorldBounds.HalfSize.Z = light.m_Radius * 0.5f;
        light.m_OBBWorldBounds.Center = WorldPos + SpotDir * (light.m_OBBWorldBounds.HalfSize.Z);

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::Translation(light.m_OBBWorldBounds.Center) * Float4x4(light.m_OBBWorldBounds.Orient) * Float4x4::Scale(light.m_OBBWorldBounds.HalfSize);
        light.m_OBBTransformInverse = OBBTransform.Inversed();

        // Compute cone AABB for culling
        light.m_AABBWorldBounds.Clear();
        light.m_AABBWorldBounds.AddPoint(WorldPos);
        Float3 v = WorldPos + SpotDir * light.m_Radius;
        Float3 vx = light.m_OBBWorldBounds.Orient[0] * light.m_OBBWorldBounds.HalfSize.X;
        Float3 vy = light.m_OBBWorldBounds.Orient[1] * light.m_OBBWorldBounds.HalfSize.X;
        light.m_AABBWorldBounds.AddPoint(v + vx);
        light.m_AABBWorldBounds.AddPoint(v - vx);
        light.m_AABBWorldBounds.AddPoint(v + vy);
        light.m_AABBWorldBounds.AddPoint(v - vy);

        // Compute cone Sphere bounds
        if (HalfConeAngle > Math::_PI / 4)
        {
            light.m_SphereWorldBounds.Radius = SinHalfConeAngle * light.m_Radius;
            light.m_SphereWorldBounds.Center = WorldPos + SpotDir * (light.m_CosHalfOuterConeAngle * light.m_Radius);
        }
        else
        {
            light.m_SphereWorldBounds.Radius = light.m_Radius / (2.0 * light.m_CosHalfOuterConeAngle);
            light.m_SphereWorldBounds.Center = WorldPos + SpotDir * light.m_SphereWorldBounds.Radius;
        }
    }
    else
    {
        light.m_SphereWorldBounds.Radius = light.m_Radius;
        light.m_SphereWorldBounds.Center = worldPosition;
        light.m_AABBWorldBounds.Mins = light.m_SphereWorldBounds.Center - light.m_Radius;
        light.m_AABBWorldBounds.Maxs = light.m_SphereWorldBounds.Center + light.m_Radius;
        light.m_OBBWorldBounds.Center = light.m_SphereWorldBounds.Center;
        light.m_OBBWorldBounds.HalfSize = Float3(light.m_SphereWorldBounds.Radius);
        light.m_OBBWorldBounds.Orient.SetIdentity();

        // TODO: Optimize?
        Float4x4 OBBTransform = Float4x4::Translation(light.m_OBBWorldBounds.Center) * Float4x4::Scale(light.m_OBBWorldBounds.HalfSize);
        light.m_OBBTransformInverse = OBBTransform.Inversed();
    }
}

void LightingSystem::UpdateBoundingBoxes(GameFrame const& frame)
{
    using Query = ECS::Query<>
        ::Required<PunctualLightComponent>
        ::ReadOnly<WorldTransformComponent>
        ::ReadOnly<MovableTag>;

    for (Query::Iterator it(*m_World); it; it++)
    {
        PunctualLightComponent* lights = it.Get<PunctualLightComponent>();
        WorldTransformComponent const* transform = it.Get<WorldTransformComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            UpdateLightBounding(lights[i], transform[i].Position[frame.StateIndex], transform[i].Rotation[frame.StateIndex]);

            //m_SpatialTree.SetBounds(lights[i].m_PrimID, lights[i].m_SphereWorldBounds);
        }
    }
}

void LightingSystem::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawDirectionalLights)
    {
        using Query = ECS::Query<>
            ::ReadOnly<DirectionalLightComponent>
            ::ReadOnly<RenderTransformComponent>;

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 1, 1, 1));

        for (Query::Iterator it(*m_World); it; it++)
        {
            DirectionalLightComponent const* lights = it.Get<DirectionalLightComponent>();
            RenderTransformComponent const* transform = it.Get<RenderTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                renderer.SetColor(Color4(lights[i].m_EffectiveColor[0],
                                         lights[i].m_EffectiveColor[1],
                                         lights[i].m_EffectiveColor[2]));

                Float3 dir = -transform[i].Rotation.ZAxis();
                renderer.DrawLine(transform[i].Position, transform[i].Position + dir * 10.0f);
            }
        }
    }

    if (com_DrawPunctualLights)
    {
        using Query = ECS::Query<>
            ::ReadOnly<PunctualLightComponent>
            ::ReadOnly<RenderTransformComponent>;

        renderer.SetDepthTest(false);

        for (Query::Iterator it(*m_World); it; it++)
        {
            PunctualLightComponent const* lights = it.Get<PunctualLightComponent>();
            RenderTransformComponent const* transform = it.Get<RenderTransformComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                //if (lights[i].m_Primitive->VisPass != renderer.GetVisPass()) // TODO
                //    continue;

                Float3 const& pos = transform[i].Position;

                if (lights[i].m_InnerConeAngle < PunctualLightComponent::MaxConeAngle)
                {
                    Float3x3 orient = transform[i].Rotation.ToMatrix3x3();

                    renderer.SetColor(Color4(0.5f, 0.5f, 0.5f, 1));
                    renderer.DrawCone(pos, orient, lights[i].m_Radius, Math::Radians(lights[i].m_InnerConeAngle) * 0.5f);
                    renderer.SetColor(Color4(1, 1, 1, 1));
                    renderer.DrawCone(pos, orient, lights[i].m_Radius, Math::Radians(lights[i].m_OuterConeAngle) * 0.5f);
                }
                else
                {
                    renderer.SetColor(Color4(1, 1, 1, 1));
                    renderer.DrawSphere(pos, lights[i].m_Radius);
                }
            }
        }
    }

    if (com_DrawEnvironmentProbes)
    {
        using Query = ECS::Query<>
            ::ReadOnly<EnvironmentProbeComponent>;

        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 0, 1, 1));

        for (Query::Iterator it(*m_World); it; it++)
        {
            EnvironmentProbeComponent const* probes = it.Get<EnvironmentProbeComponent>();

            for (int i = 0; i < it.Count(); i++)
            {
                //if (probes[i].m_Primitive->VisPass != renderer.GetVisPass()) // TODO
                //    continue;

                renderer.DrawAABB(probes[i].BoundingBox);
            }
        }
    }
}

// TODO:
#if 0
void PackLight(Float4x4 const& InViewMatrix, LightParameters& Light)
{
    PhotometricProfile* profile = GetPhotometricProfile();

    Light.Position = Float3(InViewMatrix * GetWorldPosition());
    Light.Radius = GetRadius();
    Light.InverseSquareRadius = m_InverseSquareRadius;
    Light.Direction = InViewMatrix.TransformAsFloat3x3(-GetWorldDirection()); // Only for photometric light
    Light.RenderMask = ~0u;                                                   //RenderMask; // TODO
    Light.PhotometricProfile = profile ? profile->GetPhotometricProfileIndex() : 0xffffffff;

    if (m_InnerConeAngle < MaxConeAngle)
    {
        Light.CosHalfOuterConeAngle = m_CosHalfOuterConeAngle;
        Light.CosHalfInnerConeAngle = m_CosHalfInnerConeAngle;
        Light.SpotExponent = m_SpotExponent;
        Light.Color = GetEffectiveColor(Math::Min(m_CosHalfOuterConeAngle, 0.9999f));
        Light.LightType = CLUSTER_LIGHT_SPOT;
    }
    else
    {   
        Light.CosHalfOuterConeAngle = 0;
        Light.CosHalfInnerConeAngle = 0;
        Light.SpotExponent = 0;
        Light.Color = GetEffectiveColor(-1.0f);
        Light.LightType = CLUSTER_LIGHT_POINT;  
    }
}
#endif

// TODO: LightAnimationSystem to animate lights
#if 0
struct LightAnimationPatternComponent
{
    String Pattern;
    float Speed = 1.0f;
    float Quantizer = 0.0f;

    float GetSample(float inTime);
};

HK_FORCEINLINE float Quantize(float frac, float quantizer)
{
    return _Quantizer > 0.0f ? Math::Floor(frac * quantizer) / _Quantizer : frac;
}

float LightAnimationPatternComponent::GetSample(float inTime)
{
    int frame_count = Pattern.Length();

    if (frame_count > 0)
    {
        float t = inTime * Speed;

        int keyframe = Math::Floor(t);
        int nextframe = keyframe + 1;

        float lerp = t - keyframe;

        keyframe %= frame_count;
        nextframe %= frame_count;

        float a = (Math::Clamp(Pattern[keyframe], 'a', 'z') - 'a') / 26.0f;
        float b = (Math::Clamp(Pattern[nextframe], 'a', 'z') - 'a') / 26.0f;

        return Math::Lerp(a, b, Quantize(lerp, Quantizer));
    }

    return 1.0f;
}
#endif

HK_NAMESPACE_END
