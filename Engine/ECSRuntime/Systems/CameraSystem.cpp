#include "CameraSystem.h"
#include "../Components/CameraComponent.h"
#include "../Components/FinalTransformComponent.h"

HK_NAMESPACE_BEGIN

CameraSystem::CameraSystem(ECS::World* world) :
    m_World(world)
{}

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
