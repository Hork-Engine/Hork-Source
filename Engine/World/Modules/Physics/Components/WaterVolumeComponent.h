#pragma once

#include <Engine/World/Component.h>
#include <Engine/World/Modules/Physics/PhysicsInterface.h>
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

class WaterVolumeComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    Float3                  HalfExtents{0.5f};
    uint8_t                 m_CollisionLayer = CollisionLayer::Default;
};

HK_NAMESPACE_END
