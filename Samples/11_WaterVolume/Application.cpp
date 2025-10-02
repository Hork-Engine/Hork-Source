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

#include "Common/CollisionLayer.h"
#include "Common/MapParser/Utils.h"
#include "Common/Components/FirstPersonComponent.h"

#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/UI/UIImage.h>
#include <Hork/Runtime/UI/UILabel.h>
#include <Hork/Runtime/UI/UIText.h>

#include <Hork/Runtime/World/Modules/Input/InputInterface.h>

#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/WaterVolumeComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Resources/Resource_Animation.h>

HK_NAMESPACE_BEGIN

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Water Volume")
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
        shortcuts->AddShortcut(VirtualKey::E, {}, {this, &SampleApplication::DropBarrel});

    desktop->SetShortcuts(shortcuts);

    // Create viewport
    desktop->AddWidget(UINewAssign(m_Viewport, UIViewport)
                           .WithPadding({0, 0, 0, 0})
                           .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_BOTTOM))
                               [UINew(UILabel)
                                    .WithText(UINew(UIText, "E Drop Barrel")
                                                  .WithFontSize(20)
                                                  .WithWordWrap(false)
                                                  .WithAlignment(TEXT_ALIGNMENT_HCENTER))
                                    .WithAutoWidth(true)
                                    .WithAutoHeight(true)]);

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
    render.SetAmbient(0.015f);

    // Set rendering parameters
    m_WorldRenderView = MakeRef<WorldRenderView>();
    m_WorldRenderView->SetWorld(m_World);
    m_WorldRenderView->bDrawDebug = true;
    m_Viewport->SetWorldRenderView(m_WorldRenderView);

    auto& stateMachine = sGetStateMachine();

    stateMachine.Bind("State_Loading", this, &SampleApplication::OnStartLoading, {}, &SampleApplication::OnUpdateLoading);
    stateMachine.Bind("State_Play", this, &SampleApplication::OnStartPlay, {}, {});

    stateMachine.MakeCurrent("State_Loading");

    sGetCommandProcessor().Add("com_MaxFPS 0\n");
    //sGetCommandProcessor().Add("com_DrawWaterVolume 1\n");
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
    GameObject* player = CreatePlayer(Float3(0,0,20), Quat::sIdentity());
    m_Player = player->GetHandle();

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
    materialMngr.LoadLibrary("/Root/thirdparty/freepbr.com/freepbr.mlib");
    materialMngr.LoadLibrary("/Root/thirdparty/sketchfab.com/sketchfab.mlib");

    // List of resources used in scene
    SmallVector<ResourceID, 32> sceneResources;

    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"));

    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"));
    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default_orm.mat"));
    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/water_orm.mat"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/black.png"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/dirt.png"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/albedo.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/orm.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/normal.tex"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/alien-slime1/albedo.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/alien-slime1/orm.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/alien-slime1/normal.tex"));

    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/thirdparty/sketchfab.com/barrel/barrel.mesh"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/sketchfab.com/barrel/albedo.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/sketchfab.com/barrel/orm.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/sketchfab.com/barrel/normal.tex"));

    // Load resources asynchronously
    m_Resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(m_Resources);
}

void SampleApplication::CreateScene()
{
    // Light
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(0,4,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(5500);
        light->SetRadius(40);
    }

    auto lightColor = Color3(155,171,62)/255;
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(-10,-3,-10);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(2500);
        light->SetRadius(20);
        light->SetCastShadow(false);
        light->SetColor(lightColor);
    }
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(-10,-3,10);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(2500);
        light->SetRadius(20);
        light->SetCastShadow(false);
        light->SetColor(lightColor);
    }
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(10,-3,10);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(2500);
        light->SetRadius(20);
        light->SetCastShadow(false);
        light->SetColor(lightColor);
    }
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(10,-3,-10);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(2500);
        light->SetRadius(20);
        light->SetCastShadow(false);
        light->SetColor(lightColor);
    }

    // Water volume
    {
        GameObjectDesc desc;
        desc.Name.FromString("WaterVolume");
        desc.Position = Float3(0,-5.5f,0);
        desc.IsDynamic = false;
        //desc.Scale.X = 16;
        //desc.Scale.Y = 4;
        //desc.Scale.Z = 16;
        GameObject* object;
        m_World->CreateObject(desc, object);

        WaterVolumeComponent* waterVol;
        object->CreateComponent(waterVol);
        waterVol->HalfExtents.X = 16;
        waterVol->HalfExtents.Y = 4;
        waterVol->HalfExtents.Z = 16;
    }
    {
        GameObjectDesc desc;
        desc.Name.FromString("WaterVolume");
        desc.Position = Float3(0,-1.5f,0);
        desc.IsDynamic = false;

        GameObject* object;
        m_World->CreateObject(desc, object);

        StaticMeshComponent* mesh;
        object->CreateComponent(mesh);

        RawMesh rawMesh;
        rawMesh.CreatePlaneXZ(32, 32, Float2(8));

        MeshResourceBuilder meshBuilder;
        UniqueRef<MeshResource> meshResource = meshBuilder.Build(rawMesh);
        meshResource->Upload(sGetRenderDevice());

        auto meshHandle = sGetResourceManager().CreateResourceWithData("water_surface", std::move(meshResource));

        mesh->SetMesh(meshHandle);
        mesh->SetMaterial(sGetMaterialManager().TryGet("alien-slime1"));
        mesh->SetLocalBoundingBox(rawMesh.CalcBoundingBox());
    }

    // Barrels
    {
        for (int i = 0; i <= 32; i+=2)
        {
            for (int j = 0; j <= 32; j+=2)
            {
                Quat rotation = Quat::sRotationAroundVector(sGetRandom().GetFloat() * Math::_2PI, Float3(sGetRandom().GetFloat(),sGetRandom().GetFloat(),sGetRandom().GetFloat()));

                SpawnBarrel(Float3(i-16, 4, j-16), rotation);
            }
        }
    }

    // Room
    CreateSceneFromMap(m_World, "/Root/maps/sample11.map", "grime-alley-brick2");
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

GameObject* SampleApplication::SpawnBarrel(Float3 const& position, Quat const& rotation)
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    static MeshHandle meshHandle = resourceMngr.GetResource<MeshResource>("/Root/thirdparty/sketchfab.com/barrel/barrel.mesh");

    GameObjectDesc desc;
    desc.Position = position;
    desc.Rotation = rotation;
    desc.IsDynamic = true;
    GameObject* object;
    m_World->CreateObject(desc, object);
    DynamicBodyComponent* phys;
    object->CreateComponent(phys);
    phys->Mass = 10;
    CylinderCollider* collider;
    object->CreateComponent<CylinderCollider>(collider);
    collider->Height = 0.85f;
    collider->Radius = 0.35f;
    DynamicMeshComponent* mesh;
    object->CreateComponent(mesh);
    mesh->SetMesh(meshHandle);
    mesh->SetMaterial(0, materialMngr.TryGet("thirdparty/sketchfab/barrel"));
    mesh->SetMaterial(1, materialMngr.TryGet("thirdparty/sketchfab/barrel"));
    mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});

    return object;
}

void SampleApplication::DropBarrel()
{
    if (GameObject* player = m_World->GetObject(m_Player))
    {
        if (GameObject* camera = player->FindChildren(StringID("Camera")))
        {
            GameObject* barrel = SpawnBarrel(player->GetPosition() + camera->GetWorldForwardVector() * 0.8f + Float3::sAxisY() * 1.3f, camera->GetWorldRotation() * Quat::sRotationZ(Math::_HALF_PI));            

            if (auto body = barrel->GetComponent<DynamicBodyComponent>())
            {
                body->AddImpulse(camera->GetWorldForwardVector() * 100);
            }
        }
    }
}

HK_NAMESPACE_END

using ApplicationClass = Hk::SampleApplication;
#include "Common/EntryPoint.h"
