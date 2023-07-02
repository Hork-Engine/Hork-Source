#include "SpawnSystem.h"
#include "../GameFrame.h"
#include "../CollisionModel_ECS.h"
#include <Engine/Runtime/GameApplication.h>

#include "../Components/WorldTransformComponent.h"
#include "../Components/ExperimentalComponents.h"

HK_NAMESPACE_BEGIN

SpawnSystem::SpawnSystem(ECS::World* world) :
    m_World(world)
{
    CollisionModelCreateInfo modelCreateInfo;
    CollisionBoxDef boxDef;
    modelCreateInfo.pBoxes = &boxDef;
    modelCreateInfo.BoxCount = 1;
    m_Models.EmplaceBack(CollisionModel::Create(modelCreateInfo));
    m_Meshes.Add("/Root/default/box.mesh");

    modelCreateInfo = CollisionModelCreateInfo();
    CollisionSphereDef sphereDef;
    modelCreateInfo.pSpheres = &sphereDef;
    modelCreateInfo.SphereCount = 1;
    m_Models.EmplaceBack(CollisionModel::Create(modelCreateInfo));
    m_Meshes.Add("/Root/default/sphere.mesh");

    modelCreateInfo = CollisionModelCreateInfo();
    CollisionCylinderDef cylinderDef;
    modelCreateInfo.pCylinders = &cylinderDef;
    modelCreateInfo.CylinderCount = 1;
    m_Models.EmplaceBack(CollisionModel::Create(modelCreateInfo));
    m_Meshes.Add("/Root/default/cylinder.mesh");

    modelCreateInfo = CollisionModelCreateInfo();
    CollisionCapsuleDef capsuleDef;
    modelCreateInfo.pCapsules = &capsuleDef;
    modelCreateInfo.CapsuleCount = 1;
    m_Models.EmplaceBack(CollisionModel::Create(modelCreateInfo));
    m_Meshes.Add("/Root/default/capsule.mesh");

    // TODO: Check convex hull and triangle soup
}

void SpawnSystem::Update(GameFrame const& frame)
{
    auto& commandBuffer = m_World->GetCommandBuffer(0);

    {
        using Query = ECS::Query<>
            ::Required<SpawnerComponent>
            ::ReadOnly<ActiveComponent>
            ::ReadOnly<WorldTransformComponent>;

        for (Query::Iterator it(*m_World); it; it++)
        {
            SpawnerComponent* spawner = it.Get<SpawnerComponent>();
            WorldTransformComponent const* worldTransform = it.Get<WorldTransformComponent>();
            ActiveComponent const* active = it.Get<ActiveComponent>();
            for (int i = 0; i < it.Count(); i++)
            {
                if (active[i].bIsActive)
                {
                    spawner[i].m_NextThink -= frame.FixedTimeStep;
                    if (spawner[i].m_NextThink <= 0)
                    {
                        spawner[i].m_NextThink = spawner[i].SpawnInterval;

                        SpawnFunction(commandBuffer, worldTransform[i].Position[frame.StateIndex]);
                    }
                }
                else
                {
                    spawner[i].m_NextThink = spawner[i].SpawnInterval;
                }
            }
        }
    }
}

void SpawnSystem::SpawnFunction(ECS::CommandBuffer& commandBuffer, Float3 const& worldPosition)
{
    // TODO: All this code should be replaced by something like: spawner.Prefab->Instantiate(worldPosition)

    int index = GameApplication::GetRandom().Get() % m_Models.Size();

    RigidBodyDesc rbdesc;
    rbdesc.Position = worldPosition;
    rbdesc.Rotation = Angl(45, 45, 45).ToQuat();
    rbdesc.Scale = Float3(0.5f);
    rbdesc.NodeFlags = SCENE_NODE_FLAGS_DEFAULT;
    rbdesc.MotionBehavior = MB_SIMULATED;
    rbdesc.Model = m_Models[index];

    ECS::EntityHandle phys = CreateRigidBody(commandBuffer, rbdesc);

    MeshComponent_ECS& mesh = commandBuffer.AddComponent<MeshComponent_ECS>(phys);
    mesh.Mesh = GameApplication::GetResourceManager().GetResource<MeshResource>(m_Meshes[index]);
    mesh.SubmeshIndex = 0;
    mesh.BoundingBox = BvAxisAlignedBox(Float3(-0.5f), Float3(0.5f));
    mesh.Materials[0] = GameApplication::GetMaterialManager().Get("grid8");

    commandBuffer.AddComponent<ShadowCastComponent>(phys);

    m_Entities.Add(phys);

    //meshComp->SetMass(1.0f);
    //meshComp->SetRestitution(0.4f);
}

HK_NAMESPACE_END
