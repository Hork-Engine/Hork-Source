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

#include "Application.h"

#include "Common/MapParser/Utils.h"

#include "Common/Components/DoorComponent.h"
#include "Common/Components/DoorActivatorComponent.h"
#include "Common/CollisionLayer.h"

#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/UI/UIGrid.h>
#include <Hork/Runtime/UI/UILabel.h>

#include <Hork/Runtime/World/Modules/Input/InputInterface.h>

#include <Hork/Runtime/World/Modules/Physics/CollisionFilter.h>
#include <Hork/Runtime/World/Modules/Physics/Components/StaticBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/TriggerComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>

#include <Hork/Runtime/World/Modules/NavMesh/NavMeshInterface.h>
#include <Hork/Runtime/World/Modules/NavMesh/Components/NavMeshObstacleComponent.h>
#include <Hork/Runtime/World/Modules/NavMesh/Components/NavMeshAreaComponent.h>
#include <Hork/Runtime/World/Modules/NavMesh/Components/OffMeshLinkComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Runtime/World/Modules/Audio/AudioInterface.h>

#include <Hork/Runtime/World/DebugRenderer.h>

using namespace Hk;

class ThirdPersonInputComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    GameObjectHandle ViewPoint;

private:
    PhysBodyID m_DragObject;
    Vector<Float3> m_Path;
    Vector<Float3> m_DebugPath;
    Float3 m_DesiredVelocity;
    Float3 m_MoveDir;

public:
    void BindInput(InputBindings& input)
    {
        input.BindAction("Pick", this, &ThirdPersonInputComponent::OnPick, InputEvent::OnPress);
        input.BindAction("Drag", this, &ThirdPersonInputComponent::OnDragBegin, InputEvent::OnPress);
        input.BindAction("Drag", this, &ThirdPersonInputComponent::OnDragEnd, InputEvent::OnRelease);
    }

    void FixedUpdate()
    {
        Float3 rayStart, rayDir;
        if (m_DragObject.IsValid() && GetRay(rayStart, rayDir))
        {
            ShapeCastResult result;
            ShapeCastFilter filter;

            filter.BroadphaseLayers.AddLayer(BroadphaseLayer::Static);

            auto& physics = GetWorld()->GetInterface<PhysicsInterface>();

            if (physics.CastBoxClosest(rayStart, rayDir, Float3(0.5f * 1.5f), Quat::sIdentity(), result, filter))
            {
                if (auto body = physics.TryGetComponent<DynamicBodyComponent>(m_DragObject))
                {
                    //body->MoveKinematic(rayStart + rayDir * result.Fraction);
                    body->SetWorldPosition(rayStart + rayDir * result.Fraction);
                }
            }
        }

        if (auto controller = GetOwner()->GetComponent<CharacterControllerComponent>())
        {
            float MoveSpeed = 8;

            Float3 p = FetchPathPoint(GetOwner()->GetWorldPosition());
            if (p.DistSqr(GetOwner()->GetWorldPosition()) > 0.01f)
            {
                if (controller->IsOnGround())
                {
                    m_MoveDir = p - GetOwner()->GetWorldPosition();
                    m_MoveDir.Y = 0;

                    m_MoveDir.NormalizeSelf();

                    MoveSpeed = 8;
                }
                else
                    MoveSpeed = 2;
            }
            else
                m_MoveDir.Clear();

            // Smooth the player input
            m_DesiredVelocity = 0.25f * m_MoveDir * MoveSpeed + 0.75f * m_DesiredVelocity;

            Float3 gravity = GetWorld()->GetInterface<PhysicsInterface>().GetGravity();

            // Determine new basic velocity
            Float3 newVelocity;
            if (controller->IsOnGround())
            {
                newVelocity = controller->GetGroundVelocity();
            }
            else
            {
                newVelocity.Y = controller->GetLinearVelocity().Y;

                if (controller->IsOnGround() && newVelocity.Y < 0)
                    newVelocity.Y = 0;
            }

            if (!controller->IsOnGround())
            {
                // Gravity
                newVelocity += gravity * GetWorld()->GetTick().FixedTimeStep;
            }

            // Player input
            newVelocity += m_DesiredVelocity;

            // Update character velocity
            controller->SetLinearVelocity(newVelocity);
        }
    }

    void DrawDebug(DebugRenderer& renderer)
    {
        renderer.SetDepthTest(false);
        renderer.SetColor(Color4::sBlue());
        renderer.DrawLine(m_DebugPath);
    }

private:
    bool GetRay(Float3& rayStart, Float3& rayDir)
    {
        if (auto cameraObject = GetOwner()->FindChildrenRecursive(StringID("Camera")))
            if (auto cameraComponent = cameraObject->GetComponent<CameraComponent>())
                return cameraComponent->ScreenPointToRay(GUIManager->CursorPosition, rayStart, rayDir);
        return false;
    }
    
    void OnPick()
    {
        //static bool build = true;
        //if (build)
        //{
        //    {
        //        auto& navmesh = GetWorld()->GetInterface<NavMeshInterface>();

        //        navmesh.NavigationVolumes.EmplaceBack(Float3(-128), Float3(128));

        //        navmesh.WalkableClimb = 0.4f;
        //        navmesh.CellHeight = 0.2f;

        //        navmesh.SetAreaCost(NAV_MESH_AREA_GROUND, 1);
        //        navmesh.SetAreaCost(NAV_MESH_AREA_WATER, 4);

        //        navmesh.Create();

        //        navmesh.BuildOnNextFrame();
        //    }
        //    build = false;
        //}
        
        Float3 rayStart, rayDir;
        if (GetRay(rayStart, rayDir))
        {
            RayCastResult result;
            RayCastFilter filter;

            filter.BroadphaseLayers.AddLayer(BroadphaseLayer::Static);

            if (GetWorld()->GetInterface<PhysicsInterface>().CastRayClosest(rayStart, rayDir, result, filter))
            {
                Float3 destination = rayStart + rayDir * result.Fraction;

                Float3 extents(1);
                m_Path.Clear();

                auto& navigation = GetWorld()->GetInterface<NavMeshInterface>();

                navigation.FindPath(GetOwner()->GetWorldPosition(), destination, extents, m_Path);

                m_DebugPath = m_Path;
            }
        }
    }

    void OnDragBegin()
    {
        Float3 rayStart, rayDir;
        if (GetRay(rayStart, rayDir))
        {
            RayCastResult result;
            RayCastFilter filter;

            filter.BroadphaseLayers.AddLayer(BroadphaseLayer::Dynamic);

            auto& physics = GetWorld()->GetInterface<PhysicsInterface>();

            if (physics.CastRayClosest(rayStart, rayDir, result, filter))
            {
                if (auto body = physics.TryGetComponent<DynamicBodyComponent>(result.BodyID))
                {
                    if (!body->IsKinematic())
                    {
                        m_DragObject = result.BodyID;
                        //body->SetKinematic(true);
                        body->SetGravityFactor(0);
                    }
                }
            }
        }
    }

    void OnDragEnd()
    {
        if (m_DragObject.IsValid())
        {
            auto& physics = GetWorld()->GetInterface<PhysicsInterface>();
            if (auto body = physics.TryGetComponent<DynamicBodyComponent>(m_DragObject))
            //    body->SetKinematic(false);
                    body->SetGravityFactor(1);
            m_DragObject = {};
        }
    }

    Float3 FetchPathPoint(Float3 const& position)
    {
        if (m_Path.IsEmpty())
            return position;

        if (m_Path[0].DistSqr(position) < 0.1f)
        {
            m_Path.Remove(0);
            if (m_Path.IsEmpty())
                return position;
        }
        return m_Path[0];
    }
};

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Nav Mesh")
{}

SampleApplication::~SampleApplication()
{}

void SampleApplication::Initialize()
{
    // Create UI
    UIDesktop* desktop = UINew(UIDesktop);
    GUIManager->AddDesktop(desktop);

    // Add shortcuts
    UIShortcutContainer* shortcuts = UINew(UIShortcutContainer);
    shortcuts->AddShortcut(VirtualKey::Pause, {}, {this, &SampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::P, {}, {this, &SampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::Escape, {}, {this, &SampleApplication::Quit});
    shortcuts->AddShortcut(VirtualKey::Y, {}, {this, &SampleApplication::ToggleWireframe});
    desktop->SetShortcuts(shortcuts);

    // Create viewport
    UIViewport* mainViewport;
    desktop->AddWidget(UINewAssign(mainViewport, UIViewport)
        .WithPadding({0,0,0,0}));
    desktop->SetFullscreenWidget(mainViewport);
    desktop->SetFocusWidget(mainViewport);

    // Show mouse cursor
    GUIManager->bCursorVisible = true;

    // Set input mappings
    Ref<InputMappings> inputMappings = MakeRef<InputMappings>();
    inputMappings->MapAction(PlayerController::_1, "Pick", VirtualKey::MouseLeftBtn, {});
    inputMappings->MapAction(PlayerController::_1, "Drag", VirtualKey::MouseRightBtn, {});

    sGetInputSystem().SetInputMappings(inputMappings);

    // Create game resources
    CreateResources();

    // Create game world
    m_World = CreateWorld();

    // Setup world collision
    m_World->GetInterface<PhysicsInterface>().SetCollisionFilter(CollisionLayer::CreateFilter());

    // Set rendering parameters
    m_WorldRenderView = MakeRef<WorldRenderView>();
    m_WorldRenderView->SetWorld(m_World);
    m_WorldRenderView->bClearBackground = true;
    m_WorldRenderView->BackgroundColor = Color4(0.2f, 0.2f, 0.3f, 1);
    m_WorldRenderView->bDrawDebug = true;
    mainViewport->SetWorldRenderView(m_WorldRenderView);

    // Create scene
    CreateScene();

    // Create players
    GameObject* player = CreatePlayer(m_PlayerSpawnPoints[0].Position, m_PlayerSpawnPoints[0].Rotation);

    if (GameObject* camera = player->FindChildrenRecursive(StringID("Camera")))
    {
        // Set camera for rendering
        m_WorldRenderView->SetCamera(camera->GetComponentHandle<CameraComponent>());
    
        // Set audio listener
        auto& audio = m_World->GetInterface<AudioInterface>();
        audio.SetListener(camera->GetComponentHandle<AudioListenerComponent>());
    }

    // Bind input to the player
    InputInterface& input = m_World->GetInterface<InputInterface>();
    input.SetActive(true);
    input.BindInput(player->GetComponentHandle<ThirdPersonInputComponent>(), PlayerController::_1);

    RenderInterface& render = m_World->GetInterface<RenderInterface>();
    render.SetAmbient(0.1f);

    sGetCommandProcessor().Add("com_DrawNavMesh 1\n");
    //sGetCommandProcessor().Add("com_DrawNavMeshAreas 1\n");    
    sGetCommandProcessor().Add("com_DrawOffMeshLinks 1\n");
}

void SampleApplication::Deinitialize()
{
    DestroyWorld(m_World);
}

void SampleApplication::Pause()
{
    m_World->SetPaused(!m_World->GetTick().IsPaused);
}

void SampleApplication::Quit()
{
    PostTerminateEvent();
}

void SampleApplication::ToggleWireframe()
{
    m_WorldRenderView->bWireframe = !m_WorldRenderView->bWireframe;
}

void SampleApplication::CreateResources()
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    materialMngr.LoadLibrary("/Root/default/materials/default.mlib");

    // List of resources used in scene
    ResourceID sceneResources[] = {
        resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/capsule.mesh"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/default.mg"),
        resourceMngr.GetResource<TextureResource>("/Root/grid8.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank256.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"),
        resourceMngr.GetResource<MeshResource>("/Root/default/quad_xy.mesh")
    };
    
    // Load resources asynchronously
    ResourceAreaID resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(resources);

    // Wait for the resources to load
    resourceMngr.MainThread_WaitResourceArea(resources);
}

void SampleApplication::CreateScene()
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    auto& materialMngr = GameApplication::sGetMaterialManager();

    CreateSceneFromMap(m_World, "/Root/sample5.map");

    Float3 playerSpawnPosition = Float3(-1344/32.0f,0,0);
    Quat playerSpawnRotation = Quat::sRotationY(-Math::_HALF_PI);

    // Light
    {
        Float3 lightDirection = Float3(1, -1, -1).Normalized();

        GameObjectDesc desc;
        desc.IsDynamic = true;

        GameObject* object;
        m_World->CreateObject(desc, object);
        object->SetDirection(lightDirection);

        DirectionalLightComponent* dirlight;
        object->CreateComponent(dirlight);
        dirlight->SetIlluminance(20000.0f);
        dirlight->SetShadowMaxDistance(50);
        dirlight->SetShadowCascadeResolution(2048);
        dirlight->SetShadowCascadeOffset(-10.0f);
        dirlight->SetShadowCascadeSplitLambda(0.8f);
    }

    // Boxes
    {
        Float3 positions[] = {
            Float3(-32, 0.75, -7),
            Float3(-29, 0.75, -6),
            Float3(-34.5, 0.75, -7.5),
            Float3(-32, 1.25, -7),
            Float3(-37, 0.75, -19)};

        float yaws[] = { 0, 15, 10, 10, 45 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            const float boxScale = 1.5f;
            GameObjectDesc desc;
            desc.Position = positions[i];
            desc.Rotation.FromAngles(0, Math::Radians(yaws[i]), 0);
            desc.Scale = Float3(boxScale);
            desc.IsDynamic = true;
            GameObject* object;
            m_World->CreateObject(desc, object);
            DynamicBodyComponent* phys;
            object->CreateComponent(phys);
            phys->Mass = 30;
            phys->CanPushCharacter = false;
            BoxCollider* collider;
            object->CreateComponent(collider);
            DynamicMeshComponent* mesh;
            object->CreateComponent(mesh);
            mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
            mesh->SetMaterial(materialMngr.TryGet("blank256"));
            mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});

            NavMeshObstacleComponent* obstacle;
            object->CreateComponent(obstacle);
            obstacle->SetShape(NavMeshObstacleShape::Box);
            float boxDiagonal = Math::Sqrt(Math::Square(0.5f) + Math::Square(0.5f));
            obstacle->SetHalfExtents(Float3(boxDiagonal * boxScale));
        }
    }

    // Door

    GameObject* doorTrigger;
    DoorActivatorComponent* doorActivator;
    {
        GameObjectDesc desc;
        desc.Position = Float3(-512, 120, 0)/32;
        desc.Scale = Float3(32*6, 240, 112*2)/32.0f;
        m_World->CreateObject(desc, doorTrigger);
        TriggerComponent* trigger;
        doorTrigger->CreateComponent(trigger);
        trigger->CollisionLayer = CollisionLayer::CharacterOnlyTrigger;
        BoxCollider* collider;
        doorTrigger->CreateComponent(collider);
        doorTrigger->CreateComponent(doorActivator);
    }
    {
        GameObjectDesc desc;
        desc.Position = Float3(-512, 120, 56)/32;
        desc.Scale = Float3(32, 240, 112)/32.0f;
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);
        DynamicBodyComponent* phys;
        object->CreateComponent(phys);
        phys->SetKinematic(true);
        BoxCollider* collider;
        object->CreateComponent(collider);
        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("grid8"));
        mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});

        DoorComponent* doorComponent;
        object->CreateComponent(doorComponent);
        doorComponent->Direction = Float3(0, 0, 1);
        doorComponent->m_MaxOpenDist = 2.9f;
        doorComponent->m_OpenSpeed = 4;
        doorComponent->m_CloseSpeed = 2;

        doorActivator->Parts.Add(Handle32<DoorComponent>(doorComponent->GetHandle()));
    }
    {
        GameObjectDesc desc;
        desc.Position = Float3(-512, 120, -56)/32;
        desc.Scale = Float3(32, 240, 112)/32.0f;
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);
        DynamicBodyComponent* phys;
        object->CreateComponent(phys);
        phys->SetKinematic(true);
        BoxCollider* collider;
        object->CreateComponent(collider);
        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("grid8"));
        mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});

        DoorComponent* doorComponent;
        object->CreateComponent(doorComponent);
        doorComponent->Direction = Float3(0, 0, -1);
        doorComponent->m_MaxOpenDist = 2.9f;
        doorComponent->m_OpenSpeed = 4;
        doorComponent->m_CloseSpeed = 2;

        doorActivator->Parts.Add(Handle32<DoorComponent>(doorComponent->GetHandle()));
    }

    m_PlayerSpawnPoints.Add({playerSpawnPosition, playerSpawnRotation});

    {
        auto& navigation = m_World->GetInterface<NavMeshInterface>();

        navigation.NavigationVolumes.EmplaceBack(Float3(-128), Float3(128));

        navigation.WalkableClimb = 0.4f;
        navigation.CellHeight = 0.2f;

        navigation.SetAreaCost(NAV_MESH_AREA_GROUND, 1);
        navigation.SetAreaCost(NAV_MESH_AREA_WATER, 4);
        //navigation.SetAreaCost(NAV_MESH_AREA_WATER, 1);

        navigation.Create();

        navigation.BuildOnNextFrame();
    }

    {
        GameObjectDesc desc;
        desc.Position = Float3(-35, 3.75f, -31);
        GameObject* object;
        m_World->CreateObject(desc, object);

        GameObjectDesc desc2;
        desc2.Position = Float3(-35, 0, -27);
        GameObject* dest;
        m_World->CreateObject(desc2, dest);

        OffMeshLinkComponent* link;
        object->CreateComponent(link);
        link->SetDestination(dest->GetHandle());
        link->SetAreaType(NAV_MESH_AREA_GROUND);
        //link->SetFlags(1);//NAV_MESH_FLAGS_WALK);
    }

    {
        GameObjectDesc desc;
        GameObject* object;
        m_World->CreateObject(desc, object);
        NavMeshAreaComponent* area;
        object->CreateComponent(area);
        area->SetShape(NavMeshAreaShape::Box);

        Float3 pos(-32, 0.5f, 0);
        Float3 size(9, 4, 7);
        object->SetWorldPosition(pos);
        area->SetHalfExtents(size * 0.5f);
        area->SetAreaType(NAV_MESH_AREA_WATER);

        //Float2 vertices[3];
        //vertices[0] = Float2(-3,-3);
        //vertices[1] = Float2( 3,-3);
        //vertices[2] = Float2( 3, 3);
        //area->SetVolumeContour(vertices);
    }
}


GameObject* SampleApplication::CreatePlayer(Float3 const& position, Quat const& rotation)
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    const float HeightStanding = 1.20f;
    const float RadiusStanding = 0.3f;

    // Create character controller
    GameObject* player;
    {
        GameObjectDesc desc;
        desc.Position = position;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, player);

        CharacterControllerComponent* characterController;
        player->CreateComponent(characterController);
        characterController->SetCollisionLayer(CollisionLayer::Character);
        characterController->HeightStanding = HeightStanding;
        characterController->RadiusStanding = RadiusStanding;
    }

    // Create model
    GameObject* model;
    {
        GameObjectDesc desc;
        desc.Parent = player->GetHandle();
        desc.Position = Float3(0,0.5f * HeightStanding + RadiusStanding,0);
        desc.IsDynamic = true;
        m_World->CreateObject(desc, model);

        DynamicMeshComponent* mesh;
        model->CreateComponent(mesh);

        RawMesh rawMesh;
        rawMesh.CreateCapsule(RadiusStanding, HeightStanding, 1.0f, 12, 10);
        MeshResourceBuilder builder;
        auto resource = builder.Build(rawMesh);
        resource->Upload();

        mesh->SetLocalBoundingBox(resource->GetBoundingBox());

        resourceMngr.CreateResourceWithData("character_controller_capsule", std::move(resource));

        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("character_controller_capsule"));
        mesh->SetMaterial(materialMngr.TryGet("blank512"));
    }

    GameObject* viewPoint;
    {
        GameObjectDesc desc;
        desc.Name.FromString("ViewPoint");
        desc.Parent = player->GetHandle();
        desc.Position = Float3(0,0,0);
        desc.Rotation.FromAngles(Math::Radians(-60), Math::Radians(-45), 0);
        desc.IsDynamic = true;
        m_World->CreateObject(desc, viewPoint);

        
    }

    // Create view camera
    GameObject* camera;
    {
        GameObjectDesc desc;
        desc.Name.FromString("Camera");
        desc.Parent = viewPoint->GetHandle();
        desc.Position.Z = 30;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, camera);

        CameraComponent* cameraComponent;
        camera->CreateComponent(cameraComponent);
        cameraComponent->SetFovY(45);
        
        camera->CreateComponent<AudioListenerComponent>();
    }

    // Create input
    ThirdPersonInputComponent* playerInput;
    player->CreateComponent(playerInput);
    playerInput->ViewPoint = viewPoint->GetHandle();

    return player;
}

using ApplicationClass = SampleApplication;
#include "Common/EntryPoint.h"
