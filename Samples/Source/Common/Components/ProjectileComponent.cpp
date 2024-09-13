/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "ProjectileComponent.h"
#include "FirstPersonComponent.h"
#include "LifeSpanComponent.h"
#include "../CollisionLayer.h"

#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void ProjectileComponent::OnBeginContact(Collision& collision)
{
    if (auto gameObject = collision.Body->GetOwner())
    {
        if (auto pawn = gameObject->GetComponent<FirstPersonComponent>())
        {
            if (pawn->Team != Team)
            {
                //LOG("BEGIN  Projectile contact with {} normal {} num points {} depth {}\n", collision.Body->GetOwner()->GetName(), collision.Normal, collision.Contacts.Size(), collision.Depth);

                m_Contact = collision.Contacts[0].PositionSelf;
                m_Normal = collision.Normal;

                pawn->ApplyDamage(collision.Contacts[0].VelocitySelf);

                GetWorld()->DestroyObject(GetOwner());
            }
        }
    }
}

void ProjectileComponent::OnUpdateContact(Collision& collision)
{
    //if (Component::Upcast<CharacterControllerComponent>(collision.Body))
    //{
    //    LOG("UPDATE Projectile contact with {} normal {} num points {} depth {}\n", collision.Body->GetOwner()->GetName(), collision.Normal, collision.Contacts.Size(), collision.Depth);

    //    m_Contact = collision.Contacts[0];
    //    m_Normal = collision.Normal;
    //}
}

void ProjectileComponent::OnEndContact(BodyComponent* body)
{
    //if (Component::Upcast<CharacterControllerComponent>(body))
    //{
    //    LOG("END    Projectile contact with {}\n", body->GetOwner()->GetName());
    //}
}

void ProjectileComponent::DrawDebug(DebugRenderer& renderer)
{
    renderer.DrawLine(m_Contact, m_Contact + m_Normal);
}

void SpawnProjectile(World* world, Float3 const& position, Float3 const& impulse, PlayerTeam team)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    auto& materialMngr = GameApplication::sGetMaterialManager();

    static auto meshResource = resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh");

    GameObjectDesc desc;
    desc.Name.FromString("Projectile");
    desc.Position = position;
    desc.Scale = Float3(0.2f);
    desc.IsDynamic = true;
    GameObject* object;
    world->CreateObject(desc, object);
    DynamicBodyComponent* phys;
    object->CreateComponent(phys);
    phys->CollisionLayer = CollisionLayer::Bullets;
    phys->UseCCD = true;
    phys->DispatchContactEvents = true;
    phys->CanPushCharacter = false;
    phys->Material.Restitution = 0.3f;
    phys->AddImpulse(impulse);
    SphereCollider* collider;
    object->CreateComponent(collider);
    collider->Radius = 0.5f;
    DynamicMeshComponent* mesh;
    object->CreateComponent(mesh);
    mesh->SetMesh(meshResource);
    mesh->SetMaterial(materialMngr.TryGet(team == PlayerTeam::Blue ? "blank512" : "red512"));
    mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});
    LifeSpanComponent* lifespan;
    object->CreateComponent(lifespan);
    lifespan->Time = 2;
    ProjectileComponent* projectile;
    object->CreateComponent(projectile);
    projectile->Team = team;
}

HK_NAMESPACE_END
