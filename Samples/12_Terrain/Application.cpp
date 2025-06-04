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

#include <Hork/Runtime/World/Modules/Skeleton/Components/IkLookAtComponent.h>

#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/HeightFieldComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/TerrainComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Resources/Resource_Animation.h>

#include <Hork/RenderUtils/Utilites.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

HK_NAMESPACE_BEGIN

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Terrain (WIP)")
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
                           .WithPadding({0, 0, 0, 0})
                           .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_BOTTOM))
                               [UINew(UILabel)
                                    .WithText(UINew(UIText, "Y - Toggle Wireframe")
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
    render.SetAmbient(0.1f);

    // Set rendering parameters
    m_WorldRenderView = MakeRef<WorldRenderView>();
    m_WorldRenderView->SetWorld(m_World);
    m_WorldRenderView->bDrawDebug = true;
    m_WorldRenderView->bClearBackground = false;
    m_Viewport->SetWorldRenderView(m_WorldRenderView);

    auto& stateMachine = sGetStateMachine();

    stateMachine.Bind("State_Loading", this, &SampleApplication::OnStartLoading, {}, &SampleApplication::OnUpdateLoading);
    stateMachine.Bind("State_Play", this, &SampleApplication::OnStartPlay, {}, {});

    stateMachine.MakeCurrent("State_Loading");

    sGetCommandProcessor().Add("com_MaxFPS 0\n");
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

    // Create player
    m_Player = CreatePlayer(Float3(0,0,4), Quat::sIdentity());

    // Create scene
    CreateScene();    

    if (GameObject* camera = m_Player->FindChildren(StringID("Camera")))
    {
        // Set camera for rendering
        m_WorldRenderView->SetCamera(camera->GetComponentHandle<CameraComponent>());
    }

    // Bind input to the player
    InputInterface& input = m_World->GetInterface<InputInterface>();
    input.SetActive(true);
    input.BindInput(m_Player->GetComponentHandle<FirstPersonComponent>(), PlayerController::_1);   
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

    // Procedurally generate a skybox image
    ImageStorage skyboxImage = RenderUtils::GenerateAtmosphereSkybox(sGetRenderDevice(), SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, Float3(1, -1, -1).Normalized());
    // Convert image to resource
    UniqueRef<TextureResource> skybox = MakeUnique<TextureResource>(std::move(skyboxImage));
    skybox->Upload(sGetRenderDevice());
    // Register the resource in the resource manager with the name "internal_skybox" so that it can be accessed by name from the materials.
    resourceMngr.CreateResourceWithData<TextureResource>("internal_skybox", std::move(skybox));

    // List of resources used in scene
    SmallVector<ResourceID, 32> sceneResources;

    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"));

    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/default/skybox.mesh"));
    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/skybox.mat"));

    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"));
    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default_orm.mat"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/black.png"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/dirt.png"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/albedo.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/orm.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/normal.tex"));

    // Load resources asynchronously
    m_Resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(m_Resources);
}

void SampleApplication::CreateScene()
{
    // Light
    {
        Float3 lightDirection = Float3(2, -1, -2).Normalized();

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
        dirlight->SetShadowCascadeOffset(0.0f);
        dirlight->SetShadowCascadeSplitLambda(0.8f);
    }

    // Terrain
    {
        GameObjectDesc desc;

        GameObject* object;
        m_World->CreateObject(desc, object);

        uint32_t resolution = 8192;

        HeapBlob samples;
        samples.Reset(resolution*resolution*sizeof(float));
        samples.ZeroMem();

        float* heightmap = reinterpret_cast<float*>(samples.GetData());

        for (uint32_t y = 0; y < resolution; y++)
            for (uint32_t x = 0; x < resolution; x++)
            {
                heightmap[y * resolution + x] = stb_perlin_fbm_noise3((float)x / resolution * 3, (float)y / resolution * 3, 0, 2.3f, 0.5f, 4) * 400 + 4;

                // Hole in the terrain:
                //if (x > resolution / 2 + 15 && y > resolution / 2 + 15 && x < resolution / 2 + 45 && y < resolution / 2 + 35)
                //    h = FLT_MAX;
            }

        UniqueRef<TerrainResource> terrainResource = MakeUnique<TerrainResource>();
        terrainResource->Allocate(resolution, heightmap);

        auto terrainHandle = sGetResourceManager().CreateResourceWithData("terrain_surface", std::move(terrainResource));

        TerrainComponent* terrain;
        object->CreateComponent(terrain);
        terrain->SetResource(terrainHandle);

        HeightFieldComponent* heightfield;
        object->CreateComponent(heightfield);

        

        heightfield->Data = MakeRef<TerrainCollisionData>();
        heightfield->Data->Create(heightmap, resolution);
    }

    // Room
    CreateSceneFromMap(m_World, "/Root/maps/sample12.map", "grime-alley-brick2");
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

    // Create skybox attached to camera
    {
        GameObjectDesc desc;
        desc.Name.FromString("Skybox");
        desc.Parent = camera->GetHandle();
        desc.IsDynamic = true;
        desc.AbsoluteRotation = true;
        GameObject* skybox;
        m_World->CreateObject(desc, skybox);

        DynamicMeshComponent* mesh;
        skybox->CreateComponent(mesh);
        mesh->SetLocalBoundingBox({{-0.5f,-0.5f,-0.5f},{0.5f,0.5f,0.5f}});

        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/skybox.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("skybox"));
    }

    // Create input
    FirstPersonComponent* pawn;
    player->CreateComponent(pawn);
    pawn->ViewPoint = camera->GetHandle();
    pawn->Team = PlayerTeam::Blue;

    return player;
}

HK_NAMESPACE_END

using ApplicationClass = Hk::SampleApplication;
#include "Common/EntryPoint.h"
