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
#include "Common/Components/FirstPersonComponent.h"
#include "Common/CollisionLayer.h"

#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/UI/UIImage.h>

#include <Hork/Runtime/World/Modules/Audio/AudioInterface.h>
#include <Hork/Runtime/World/Modules/Audio/Components/SoundSource.h>

#include <Hork/Runtime/World/Modules/Input/InputInterface.h>

#include <Hork/Runtime/World/Modules/Physics/CollisionFilter.h>
#include <Hork/Runtime/World/Modules/Physics/PhysicsInterface.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>

#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Image/PhotometricData.h>

#include <Hork/Resources/Resource_Mesh.h>

using namespace Hk;

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Ies Profiles")
{}

SampleApplication::~SampleApplication()
{}

void SampleApplication::Initialize()
{
    // Create UI
    UIDesktop* desktop = UINew(UIDesktop);
    sGetUIManager().AddDesktop(desktop);

    m_Desktop = desktop;

    // Add shortcuts
    UIShortcutContainer* shortcuts = UINew(UIShortcutContainer);
    shortcuts->AddShortcut(VirtualKey::Pause, {}, {this, &SampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::P, {}, {this, &SampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::Escape, {}, {this, &SampleApplication::Quit});
    shortcuts->AddShortcut(VirtualKey::Y, {}, {this, &SampleApplication::ToggleWireframe});
    shortcuts->AddShortcut(VirtualKey::F10, {}, {this, &SampleApplication::Screenshot});
    desktop->SetShortcuts(shortcuts);

    // Create viewport
    desktop->AddWidget(UINewAssign(m_Viewport, UIViewport)
        .WithPadding({0,0,0,0}));

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
    m_WorldRenderView->bClearBackground = false;
    m_WorldRenderView->bDrawDebug = true;
    m_Viewport->SetWorldRenderView(m_WorldRenderView);

    auto& stateMachine = sGetStateMachine();

    stateMachine.Bind("State_Loading", this, &SampleApplication::OnStartLoading, {}, &SampleApplication::OnUpdateLoading);
    stateMachine.Bind("State_Play", this, &SampleApplication::OnStartPlay, {}, {});

    stateMachine.MakeCurrent("State_Loading");
}

void SampleApplication::Deinitialize()
{
    DestroyWorld(m_World);
}

void SampleApplication::OnStartLoading()
{
    ShowLoadingScreen(true);
}

void SampleApplication::OnUpdateLoading(float timeStep)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (resourceMngr.IsAreaReady(m_Resources))
    {
        sGetStateMachine().MakeCurrent("State_Play");
    }
}

void SampleApplication::OnStartPlay()
{
    ShowLoadingScreen(false);

    // Create scene
    CreateScene();

    // Create player
    GameObject* player = CreatePlayer(Float3(0,0,6), Quat::sIdentity());

    if (GameObject* camera = player->FindChildren(StringID("Camera")))
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
    input.BindInput(player->GetComponentHandle<FirstPersonComponent>(), PlayerController::_1);   
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

void SampleApplication::Screenshot()
{
    TakeScreenshot("screenshot.png");
}

void SampleApplication::ShowLoadingScreen(bool show)
{
    auto& resourceMngr = sGetResourceManager();

    if (show)
    {
        if (!m_LoadingScreen)
        {
            m_LoadingScreen = UINew(UIWidget);
            m_LoadingScreen->WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_CENTER));
            m_LoadingScreen->WithBackground(UINew(UISolidBrush, Color4::sBlack()));

            m_Desktop->AddWidget(m_LoadingScreen);

            auto textureHandle = resourceMngr.CreateResourceFromFile<TextureResource>("/Root/loading.png");
            auto texture = resourceMngr.TryGet(textureHandle);
            if (texture)
            {
                texture->Upload(sGetRenderDevice());

                m_LoadingScreen->AddWidget(UINew(UIImage)
                    .WithTexture(textureHandle)
                    .WithTextureSize(texture->GetWidth(), texture->GetHeight())
                    .WithSize(Float2(texture->GetWidth(), texture->GetHeight())));
            }
        }

        m_Desktop->SetFullscreenWidget(m_LoadingScreen);
        m_Desktop->SetFocusWidget(m_LoadingScreen);
    }
    else
    {
        if (m_LoadingScreen)
        {
            m_Desktop->RemoveWidget(m_LoadingScreen);
            m_LoadingScreen = nullptr;

            resourceMngr.PurgeResourceData(m_LoadingTexture);
            m_LoadingTexture = {};
        }
        m_Desktop->SetFullscreenWidget(m_Viewport);
        m_Desktop->SetFocusWidget(m_Viewport);
    }
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
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/default.mg"),
        resourceMngr.GetResource<TextureResource>("/Root/grid8.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/gray.png")
        
    };

    // Load resources asynchronously
    m_Resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(m_Resources);
}

void SampleApplication::CreateScene()
{
    CreateSceneFromMap(m_World, "/Root/sample7.map", "gray");

    auto& resourceMngr = GameApplication::sGetResourceManager();
    auto& materialMngr = GameApplication::sGetMaterialManager();

    auto& renderInterface = m_World->GetInterface<RenderInterface>();
    auto& photometricPool = renderInterface.GetPhotometricPool();

    // Light
    {
        GameObjectDesc desc;
        desc.Name.FromString("Ies Light");
        desc.Position = Float3(0, 6.5f, 0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetRadius(10);
        object->SetDirection(Float3(0, -1, 0));    

        if (auto file = resourceMngr.OpenFile("/Root/ies/test.ies"))
        {
            PhotometricData photometricData = ParsePhotometricData(file.AsString());
            if (photometricData)
            {
                uint8_t samples[256];
                float intensity;
                photometricData.ReadSamples(samples, intensity);

                uint16_t photometricID = photometricPool.Add(samples);

                light->SetPhotometric(photometricID);
                light->SetPhotometricIntensity(intensity);
            }
        }

        renderInterface.SetAmbient(0.001f);
    }

    // Boxes
    {
        Float3 positions[] = {
            Float3( -1.5, 0.5, -1 ),
            Float3( 2, 0.5, 1 ),
            Float3( -0.5, 0.5, -1.5 ),
            Float3( -1, 1.5, -1 ) };

        float yaws[] = { 0, 15, 10, 10 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            GameObjectDesc desc;
            desc.Position = positions[i];
            desc.Rotation.FromAngles(0, Math::Radians(yaws[i]), 0);
            desc.Scale = Float3(1);
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
            mesh->SetMaterial(materialMngr.TryGet("gray"));
            mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});
        }
    }

#if 0
    // Center of mass test
    {
            GameObjectDesc desc;
            desc.Position = Float3(0, 2, 3);
            desc.Scale = Float3(1,2,1);
            desc.IsDynamic = true;
            GameObject* object;
            m_World->CreateObject(desc, object);
            DynamicBodyComponent* phys;
            object->CreateComponent(phys);
            phys->Mass = 30;
            phys->CenterOfMassOverride = Float3(0.5f, -0.5f, 0);
            BoxCollider* boxCollider;
            object->CreateComponent<BoxCollider>(boxCollider);
            boxCollider->OffsetPosition.X = 0.5f;

            BoxCollider* boxCollider2;
            object->CreateComponent<BoxCollider>(boxCollider2);
            boxCollider2->OffsetPosition.Y = 0.5f;

            DynamicMeshComponent* mesh;
            object->CreateComponent(mesh);
            mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
            mesh->SetMaterial(materialMngr.TryGet("grid8"));
            mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});
    }
#endif
}

GameObject* SampleApplication::CreatePlayer(Float3 const& position, Quat const& rotation)
{
    const float HeightStanding = 1.35f;
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

    // Create view camera
    GameObject* camera;
    {
        GameObjectDesc desc;
        desc.Name.FromString("Camera");
        desc.Parent = player->GetHandle();
        desc.Position = Float3(0,1.7f,0);
        desc.Rotation = rotation;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, camera);

        CameraComponent* cameraComponent;
        camera->CreateComponent(cameraComponent);
        cameraComponent->SetFovY(75);

        camera->CreateComponent<AudioListenerComponent>();
    }

    // Create input
    FirstPersonComponent* pawn;
    player->CreateComponent(pawn);
    pawn->ViewPoint = camera->GetHandle();
    pawn->Team = PlayerTeam::Blue;

    return player;
}

using ApplicationClass = SampleApplication;
#include "Common/EntryPoint.h"
