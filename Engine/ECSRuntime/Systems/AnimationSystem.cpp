#include "AnimationSystem.h"
#include "../GameFrame.h"
#include "../Components/TransformComponent.h"
#include "../Components/VelocityComponent.h"
#include "../Components/ExperimentalComponents.h"
#include "../Components/MeshComponent.h"

HK_NAMESPACE_BEGIN

AnimationSystem::AnimationSystem(ECS::World* world) :
    m_World(world)
{}

void AnimationSystem::Update(GameFrame const& frame)
{
    using MoveQuery = ECS::Query<>
        ::Required<TransformComponent>
        ::ReadOnly<Velocity>;

    using RotateQuery = ECS::Query<>
        ::Required<TransformComponent>
        ::ReadOnly<AngularVelocity>;

    float timeStep = frame.FixedTimeStep;

    //float delta = timeStep * 0.1f;
    //for (MoveQuery::Iterator it(*m_World); it; it++)
    //{
    //    auto* t = it.Get<TransformComponent>();
    //    auto* v = it.Get<Velocity>();
    //    int count = it.Count();

    //    for (int i = 0; i < count; ++i)
    //    {
    //        t[i].Position.X += v[i].x * delta;
    //        t[i].Position.Y += v[i].y * delta;
    //        t[i].Position.Z += v[i].z * delta;
    //    }
    //}

    {
        using Query = ECS::Query<>
            ::Required<ProceduralMeshComponent_ECS>;

        float t = Math::Fract(frame.FixedTime) * Math::_2PI;

        for (Query::Iterator q(*m_World); q; q++)
        {
            ProceduralMeshComponent_ECS* mesh = q.Get<ProceduralMeshComponent_ECS>();

            for (int i = 0; i < q.Count(); i++)
            {
                ProceduralMesh_ECS* proceduralMesh = mesh[i].Mesh;

                if (proceduralMesh)
                {
                    CreateSphereMesh(proceduralMesh->VertexCache, proceduralMesh->IndexCache, proceduralMesh->BoundingBox, 0.5f, 2.0f, 8, 8);

                    for (MeshVertex& v : proceduralMesh->VertexCache)
                    {
                        Float3 offset;
                        offset.X = Math::Sin(v.Position.Y + t);
                        offset.Y = Math::Sin(v.Position.Z + t);
                        offset.Z = Math::Sin(v.Position.X + t);

                        v.Position += offset * 0.2f;
                    }
                }
            }
        }
    }

    for (RotateQuery::Iterator it(*m_World); it; it++)
    {
        auto* t = it.Get<TransformComponent>();
        auto* v = it.Get<AngularVelocity>();
        int count = it.Count();

        for (int i = 0; i < count; ++i)
        {
            Quat& r = t[i].Rotation;
            r = r.RotateAroundNormal(v[i].vel * timeStep * 10, Float3(0, 1, 0));
            r.NormalizeSelf();
        }
    }
}

HK_NAMESPACE_END
