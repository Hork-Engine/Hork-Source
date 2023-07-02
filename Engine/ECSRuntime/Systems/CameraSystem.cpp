#include "CameraSystem.h"
#include "../Components/FinalTransformComponent.h"

HK_NAMESPACE_BEGIN

CameraSystem::CameraSystem(ECS::World* world) :
    m_World(world)
{
    m_World->AddEventHandler<ECS::Event::OnComponentAdded<CameraComponent_ECS>>(this);
    m_World->AddEventHandler<ECS::Event::OnComponentRemoved<CameraComponent_ECS>>(this);
}

void CameraSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentAdded<CameraComponent_ECS> const& event)
{
    //m_Cameras.Add(event.m_EntityHandle);
}

void CameraSystem::HandleEvent(ECS::World* world, ECS::Event::OnComponentRemoved<CameraComponent_ECS> const& event)
{
    //auto index = m_Cameras.IndexOf(event.m_EntityHandle);
    //if (index != Core::NPOS)
    //{
    //    m_Cameras.Remove(index);
    //}
}

void CameraSystem::Update()
{
    //using Query = ECS::Query<>
    //    ::ReadOnly<FinalTransformComponent>
    //    ::Required<CameraComponent_ECS>;

    //for (Query::Iterator it(*m_World); it; it++)
    //{
    //    auto* t = it.Get<FinalTransformComponent>();
    //    auto* c = it.Get<CameraComponent_ECS>();
    //    int count = it.Count();

    //    for (int i = 0; i < count; ++i)
    //    {
    //        // The camera has its own rotation, which is updated with a variable time step. It can be used to reduce input lag.
    //        //c[i].Camera->SetTransform(t[i].Position, t[i].Rotation * c[i].Rotation, Float3(1));
    //    }
    //}
}

HK_NAMESPACE_END
