#pragma once

#include <Engine/World/Modules/Physics/PhysicsInterface.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

class CollisionModel;

class TriggerComponent final : public BodyComponent
{
    friend class PhysicsInterface;
    friend class ContactListener;

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

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;
    class BodyUserData*     m_UserData = nullptr; // TODO: Use allocator for this
};

HK_NAMESPACE_END
