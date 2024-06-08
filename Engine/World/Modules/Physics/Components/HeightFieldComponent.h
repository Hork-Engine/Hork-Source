#pragma once

#include <Engine/World/Modules/Physics/PhysicsInterface.h>
#include <Engine/World/Modules/Physics/PhysicsMaterial.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

class TerrainCollision;

class HeightFieldComponent final : public BodyComponent
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    //PhysicsMaterial         Material;

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t                 m_CollisionLayer = CollisionLayer::Default;

    //uint32_t                ObjectFilterID = ~0u;
    
    TRef<TerrainCollision>  m_CollisionModel;

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;

    class BodyUserData*     m_UserData = nullptr;
};

HK_NAMESPACE_END
