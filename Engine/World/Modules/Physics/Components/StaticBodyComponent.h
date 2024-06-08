#pragma once

#include <Engine/World/Modules/Physics/PhysicsInterface.h>
#include <Engine/World/Modules/Physics/PhysicsMaterial.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

class CollisionModel;

class StaticBodyComponent final : public BodyComponent
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;


    /// Collision model of the body
    TRef<CollisionModel>    m_CollisionModel;

    /// The collision layer this body belongs to (determines if two objects can collide)
    uint8_t                 m_CollisionLayer;

    //uint32_t              m_ObjectFilterID = ~0u;

    PhysicsMaterial         Material;

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;

    class BodyUserData*     m_UserData = nullptr;
};

HK_NAMESPACE_END
