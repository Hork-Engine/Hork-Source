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

// TODO: Add to this example: Skeletal Animation, Sounds

#include "Application.h"

#include "Common/MapParser/Utils.h"

#include "Common/Components/ThirdPersonComponent.h"
#include "Common/Components/DoorComponent.h"
#include "Common/Components/DoorActivatorComponent.h"
#include "Common/Components/LightAnimator.h"
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

#include <Hork/Runtime/World/Modules/Gameplay/Components/SpringArmComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Runtime/World/Modules/Animation/Components/NodeMotionComponent.h>
#include <Hork/Runtime/World/Modules/Animation/NodeMotion.h>

#include <Hork/Runtime/World/Modules/Audio/AudioInterface.h>

using namespace Hk;

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Third Person")
{}

SampleApplication::~SampleApplication()
{}

void SampleApplication::Initialize()
{
    // Create UI
    UIDesktop* desktop = UINew(UIDesktop);
    sGetUIManager().AddDesktop(desktop);

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

    // Hide mouse cursor
    sGetUIManager().bCursorVisible = false;

    // Set input mappings
    Ref<InputMappings> inputMappings = MakeRef<InputMappings>();
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::W, 1);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::S, -1);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Up, 1);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Down, -1);
    inputMappings->MapAxis(PlayerController::_1, "MoveRight",   VirtualKey::A, -1);
    inputMappings->MapAxis(PlayerController::_1, "MoveRight",   VirtualKey::D, 1);
    inputMappings->MapAxis(PlayerController::_1, "MoveUp",      VirtualKey::Space, 1.0f);
    inputMappings->MapAxis(PlayerController::_1, "TurnRight",   VirtualKey::Left, -200.0f);
    inputMappings->MapAxis(PlayerController::_1, "TurnRight",   VirtualKey::Right, 200.0f);

    inputMappings->MapAxis(PlayerController::_1, "FreelookHorizontal", VirtualAxis::MouseHorizontal, 1.0f);
    inputMappings->MapAxis(PlayerController::_1, "FreelookVertical",   VirtualAxis::MouseVertical, 1.0f);
    
    inputMappings->MapAction(PlayerController::_1, "Attack",    VirtualKey::MouseLeftBtn, {});
    inputMappings->MapAction(PlayerController::_1, "Attack",    VirtualKey::LeftControl, {});

    inputMappings->MapGamepadAction(PlayerController::_1,   "Attack",       GamepadKey::X);
    inputMappings->MapGamepadAction(PlayerController::_1,   "Attack",       GamepadAxis::TriggerRight);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveForward",  GamepadAxis::LeftY, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveRight",    GamepadAxis::LeftX, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveUp",       GamepadKey::A, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "TurnRight",    GamepadAxis::RightX, 200.0f);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "TurnUp",       GamepadAxis::RightY, 200.0f);

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
    m_WorldRenderView->BackgroundColor = Color4::sBlack();
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
    input.BindInput(player->GetComponentHandle<ThirdPersonComponent>(), PlayerController::_1);
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
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"),
        resourceMngr.GetResource<TextureResource>("/Root/grid8.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank256.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp")
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

    CreateSceneFromMap(m_World, "/Root/maps/sample3.map");

    Float3 playerSpawnPosition = Float3(12,0,0);
    Quat playerSpawnRotation = Quat::sRotationY(Math::_HALF_PI);

    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(16,2,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(300);

        LightAnimator* animator;
        object->CreateComponent(animator);
        animator->Type = LightAnimator::AnimationType::SlowPulse;
    }
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(-48,2,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(300);

        LightAnimator* animator;
        object->CreateComponent(animator);
        animator->Type = LightAnimator::AnimationType::SlowPulse;
    }

    // Boxes
    {
        Float3 positions[] = {
            Float3( -21, 0, 27 ),
            Float3( -18, 0, 28 ),
            Float3( -23.5, 0, 26.5 ),
            Float3( -21, 3, 27 ) };

        float yaws[] = { 0, 15, 10, 10 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            GameObjectDesc desc;
            desc.Position = positions[i] + Float3(22-33, 0, -28-6);
            desc.Rotation.FromAngles(0, Math::Radians(yaws[i]), 0);
            desc.Scale = Float3(1.5f);
            desc.IsDynamic = true;
            GameObject* object;
            m_World->CreateObject(desc, object);
            DynamicBodyComponent* phys;
            object->CreateComponent(phys);
            phys->Mass = 30;
            object->CreateComponent<BoxCollider>();
            DynamicMeshComponent* mesh;
            object->CreateComponent(mesh);
            mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
            mesh->SetMaterial(materialMngr.TryGet("blank256"));
            mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});
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
        doorTrigger->CreateComponent<BoxCollider>();
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
        object->CreateComponent<BoxCollider>();
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
        object->CreateComponent<BoxCollider>();
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
        resource->Upload(sGetRenderDevice());

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
        desc.Position = Float3(0,1.7f,0);
        desc.Rotation = rotation;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, viewPoint);

        desc.Name.FromString("Torch");
        desc.Parent = viewPoint->GetHandle();
        desc.Position = Float3(1,0,0);
        desc.IsDynamic = true;
        GameObject* torch;
        m_World->CreateObject(desc, torch);

        PunctualLightComponent* light;
        torch->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(100);
        light->SetTemperature(3500);
        LightAnimator* animator;
        torch->CreateComponent(animator);
    }

    // Create view camera
    GameObject* camera;
    {
        GameObjectDesc desc;
        desc.Name.FromString("Camera");
        desc.Parent = viewPoint->GetHandle();
        desc.IsDynamic = true;
        m_World->CreateObject(desc, camera);

        CameraComponent* cameraComponent;
        camera->CreateComponent(cameraComponent);
        cameraComponent->SetFovY(75);

        SpringArmComponent* sprintArm;
        camera->CreateComponent(sprintArm);
        sprintArm->DesiredDistance = 5;
        
        camera->CreateComponent<AudioListenerComponent>();
    }

    // Create input
    ThirdPersonComponent* pawn;
    player->CreateComponent(pawn);
    pawn->ViewPoint = viewPoint->GetHandle();

    return player;
}

using ApplicationClass = SampleApplication;
#include "Common/EntryPoint.h"
