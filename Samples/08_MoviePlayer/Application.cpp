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
#include "Common/Components/JumpadComponent.h"
#include "Common/Components/TeleporterComponent.h"
#include "Common/Components/ElevatorComponent.h"

#include "Common/CollisionLayer.h"

#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/UI/UIImage.h>

#include <Hork/Runtime/World/Modules/Input/InputInterface.h>

#include <Hork/Runtime/World/Modules/Physics/CollisionFilter.h>

#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

using namespace Hk;

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Movie Player"),
    m_Cinematic("cinematic")
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

    RenderInterface& render = m_World->GetInterface<RenderInterface>();
    render.SetAmbient(0.001f);

    // Set rendering parameters
    m_WorldRenderView = MakeRef<WorldRenderView>();
    m_WorldRenderView->SetWorld(m_World);
    m_WorldRenderView->bClearBackground = false;
    m_WorldRenderView->bDrawDebug = true;
    m_Viewport->SetWorldRenderView(m_WorldRenderView);

    auto& stateMachine = sGetStateMachine();

    stateMachine.Bind("State_Intro", this, &SampleApplication::OnStartIntro, {}, &SampleApplication::OnUpdateIntro);
    stateMachine.Bind("State_Play", this, &SampleApplication::OnStartPlay, {}, &SampleApplication::OnUpdate);

    stateMachine.MakeCurrent("State_Intro");

    sGetCommandProcessor().Add("com_ShowStat 1\n");
    sGetCommandProcessor().Add("com_ShowFPS 1\n");
    sGetCommandProcessor().Add("com_MaxFPS 0\n");
    sGetCommandProcessor().Add("rt_SwapInterval 1\n");
}

void SampleApplication::Deinitialize()
{
    DestroyWorld(m_World);
}

void SampleApplication::OnStartIntro()
{
    ShowIntro(true);
}

void SampleApplication::OnUpdateIntro(float timeStep)
{
    m_Cinematic.Tick(timeStep);

    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (resourceMngr.IsAreaReady(m_Resources) && m_Cinematic.IsEnded())
    {
        sGetStateMachine().MakeCurrent("State_Play");
    }
}

void SampleApplication::OnUpdate(float timeStep)
{
    if (!m_World->GetTick().IsPaused)
        m_Cinematic.Tick(timeStep);
}

void SampleApplication::OnVideoFrameUpdated(uint8_t const* data, uint32_t width, uint32_t height)
{
    uint32_t pixcount = width * height;
    float r_avg = 0;
    float g_avg = 0;
    float b_avg = 0;
    uint32_t total = 0;
    for (uint32_t i = 0; i < pixcount; i += 10)
    {
        uint8_t const* color = data + i * 4;

        b_avg += color[0] / 255.0f;
        g_avg += color[1] / 255.0f;
        r_avg += color[2] / 255.0f;
        total++;
    }

    r_avg = LinearFromSRGB(r_avg / total);
    g_avg = LinearFromSRGB(g_avg / total);
    b_avg = LinearFromSRGB(b_avg / total);
    PunctualLightComponent* light = m_World->GetComponent(m_Light);
    light->SetColor({r_avg,g_avg,b_avg});
}

void SampleApplication::OnStartPlay()
{
    ShowIntro(false);

    // Create scene
    CreateScene();

    // Create player
    GameObject* player = CreatePlayer(Float3(0,0,7), Quat::sIdentity());

    if (GameObject* camera = player->FindChildren(StringID("Camera")))
    {
        // Set camera for rendering
        m_WorldRenderView->SetCamera(camera->GetComponentHandle<CameraComponent>());
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

void SampleApplication::ShowIntro(bool show)
{
    if (show)
    {
        m_Cinematic.Open("/Root/ai_generated.mpg");
        m_Cinematic.SetLoop(false);

        if (!m_IntroWidget)
        {
            m_IntroWidget = UINew(UIImage)
                .WithTexture(m_Cinematic.GetTextureHandle())
                .WithTextureSize(m_Cinematic.GetWidth(), m_Cinematic.GetHeight())
                .WithStretchedX(true)
                .WithStretchedY(true)
                ;
            m_Desktop->AddWidget(m_IntroWidget);
        }

        m_Desktop->SetFullscreenWidget(m_IntroWidget);
        m_Desktop->SetFocusWidget(m_IntroWidget);
    }
    else
    {
        if (m_IntroWidget)
        {
            m_Desktop->RemoveWidget(m_IntroWidget);
            m_IntroWidget = nullptr;

            m_Cinematic.Close();
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
        resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default_sslr.mat"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/unlit_clamped.mat"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/dirt.png")
    };

    // Load resources asynchronously
    m_Resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(m_Resources);
}

void SampleApplication::CreateScene()
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    auto& materialMngr = GameApplication::sGetMaterialManager();

    {
        GameObjectDesc desc;
        desc.Position = Float3(0,2,0);
        GameObject* monitor;
        m_World->CreateObject(desc, monitor);
        StaticMeshComponent* face;
        monitor->CreateComponent(face);

        RawMesh rawMesh;
        rawMesh.CreatePlaneXY(16.0f/4, 9.0f/4, Float2(1,1));

        MeshResourceBuilder builder;
        UniqueRef<MeshResource> quadMesh = builder.Build(rawMesh);
        if (quadMesh)
            quadMesh->Upload(sGetRenderDevice());

        auto surfaceHandle = resourceMngr.CreateResourceWithData<MeshResource>("monitor_surface", std::move(quadMesh));

        face->SetMesh(surfaceHandle);
        face->SetLocalBoundingBox(rawMesh.CalcBoundingBox());

        m_Cinematic.Open("/Root/ai_generated.mpg");
        m_Cinematic.SetLoop(true);
        m_Cinematic.E_OnImageUpdate.Bind(this, &SampleApplication::OnVideoFrameUpdated);

        Ref<MaterialLibrary> matlib = materialMngr.CreateLibrary();
        Material* material = matlib->CreateMaterial("cinematic_surface");
        material->SetResource(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/unlit_clamped.mat"));
        material->SetTexture(0, m_Cinematic.GetTextureHandle());
        face->SetMaterial(material);
    }

    // Light
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(0,2,0.2f);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        m_Light = object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(1500);
        light->SetRadius(10);
    }

    // Room
    CreateSceneFromMap(m_World, "/Root/sample8_9.map", "dirt_sslr");
}

GameObject* SampleApplication::CreatePlayer(Float3 const& position, Quat const& rotation)
{
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
