#pragma once

#include "EngineSystem.h"

#include "../Components/CameraComponent.h"

HK_NAMESPACE_BEGIN

class CameraSystem : public EngineSystemECS
{
public:
    CameraSystem(ECS::World* world);

    void HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<CameraComponent_ECS> const& event);
    void HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<CameraComponent_ECS> const& event);

    void Update();

private:
    ECS::World* m_World;
    //TVector<ECS::EntityHandle> m_Cameras;
};

HK_NAMESPACE_END
