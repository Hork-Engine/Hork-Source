#pragma once

#include <Engine/World/Component.h>
#include <Engine/World/TickFunction.h>

HK_NAMESPACE_BEGIN

class SpringArmComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    float SphereCastRadius = 0.3f;
    float DesiredDistance{};
    float ActualDistance{};
    float MinDistance{0.2f};
    float Speed{2};

    void FixedUpdate();
};

class PhysicsInterface;
namespace TickGroup_FixedUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<SpringArmComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Spring Arm");
        desc.AddPrerequisiteInterface<PhysicsInterface>();
    }
}

HK_NAMESPACE_END
