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

// TODO: Add to this example: HUD, Health/Damage, Frags, Sounds

#include "Application.h"

#include "Common/MapParser/Utils.h"

#include "Common/Components/FirstPersonComponent.h"
#include "Common/Components/JumpadComponent.h"
#include "Common/Components/TeleporterComponent.h"
#include "Common/Components/ElevatorComponent.h"
#include "Common/CollisionLayer.h"

#include <Hork/RenderUtils/Utilites.h>

#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/UI/UIGrid.h>
#include <Hork/Runtime/UI/UILabel.h>
#include <Hork/Runtime/UI/UIImage.h>

#include <Hork/Runtime/World/Modules/Input/InputInterface.h>

#include <Hork/Runtime/World/Modules/Physics/CollisionFilter.h>
#include <Hork/Runtime/World/Modules/Physics/Components/StaticBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/TriggerComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Runtime/World/Modules/Animation/Components/NodeMotionComponent.h>
#include <Hork/Runtime/World/Modules/Animation/NodeMotion.h>

#include <Hork/Runtime/World/Modules/Audio/AudioInterface.h>
#include <Hork/Runtime/World/Modules/Audio/Components/SoundSource.h>

#define SPLIT_SCREEN

using namespace Hk;

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: First Person Shooter")
{}

SampleApplication::~SampleApplication()
{}

void SampleApplication::Initialize()
{
    // Create UI
    UIDesktop* desktop = UINew(UIDesktop);
    GUIManager->AddDesktop(desktop);

    m_Desktop = desktop;

    // Add shortcuts
    UIShortcutContainer* shortcuts = UINew(UIShortcutContainer);
    shortcuts->AddShortcut(VirtualKey::Pause, {}, {this, &SampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::P, {}, {this, &SampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::Escape, {}, {this, &SampleApplication::Quit});
    shortcuts->AddShortcut(VirtualKey::Y, {}, {this, &SampleApplication::ToggleWireframe});
    desktop->SetShortcuts(shortcuts);

    // Create viewport
#ifdef SPLIT_SCREEN
    desktop->AddWidget(UINewAssign(m_SplitView, UIGrid, 0, 0)
        .AddRow(1)
        .AddColumn(0.5f)
        .AddColumn(0.5f)
        .WithNormalizedColumnWidth(true)
        .WithNormalizedRowWidth(true)
        .WithHSpacing(0)
        .WithVSpacing(0)
        .WithPadding(0)
        .AddWidget(UINewAssign(m_Viewports[0], UIViewport)
            .WithGridOffset(UIGridOffset()
                .WithColumnIndex(0)
                .WithRowIndex(0))
            .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_TOP))
            [UINew(UILabel)
            .WithText(UINew(UIText, "PLAYER1")
                .WithFontSize(48)
                .WithWordWrap(false)
                .WithAlignment(TEXT_ALIGNMENT_HCENTER))
            .WithAutoWidth(true)
            .WithAutoHeight(true)])
        .AddWidget(UINewAssign(m_Viewports[1], UIViewport)
            .WithGridOffset(UIGridOffset()
                .WithColumnIndex(1)
                .WithRowIndex(0))
            .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_TOP))
            [UINew(UILabel)
            .WithText(UINew(UIText, "PLAYER2")
                .WithFontSize(48)
                .WithWordWrap(false)
                .WithAlignment(TEXT_ALIGNMENT_HCENTER))
            .WithAutoWidth(true)
            .WithAutoHeight(true)]));

    desktop->SetFullscreenWidget(m_SplitView);
    desktop->SetFocusWidget(m_Viewports[0]);
#else
    UIViewport* mainViewport;
    desktop->AddWidget(UINewAssign(mainViewport, UIViewport)
        .WithPadding({0,0,0,0}));
    desktop->SetFullscreenWidget(mainViewport);
    desktop->SetFocusWidget(mainViewport);
#endif

    // Hide mouse cursor
    GUIManager->bCursorVisible = false;

    // Set input mappings
    Ref<InputMappings> inputMappings = MakeRef<InputMappings>();
    inputMappings->MapAxis(PlayerController::_2, "MoveForward", VirtualKey::W, 1);
    inputMappings->MapAxis(PlayerController::_2, "MoveForward", VirtualKey::S, -1);
    inputMappings->MapAxis(PlayerController::_2, "MoveForward", VirtualKey::Up, 1);
    inputMappings->MapAxis(PlayerController::_2, "MoveForward", VirtualKey::Down, -1);
    inputMappings->MapAxis(PlayerController::_2, "MoveRight",   VirtualKey::A, -1);
    inputMappings->MapAxis(PlayerController::_2, "MoveRight",   VirtualKey::D, 1);

    inputMappings->MapAxis(PlayerController::_2, "MoveUp",      VirtualKey::Space, 1.0f);
    inputMappings->MapAxis(PlayerController::_2, "TurnRight",   VirtualKey::Left, -200.0f);
    inputMappings->MapAxis(PlayerController::_2, "TurnRight",   VirtualKey::Right, 200.0f);

    inputMappings->MapAxis(PlayerController::_2, "FreelookHorizontal", VirtualAxis::MouseHorizontal, 1.0f);
    inputMappings->MapAxis(PlayerController::_2, "FreelookVertical",   VirtualAxis::MouseVertical, 1.0f);
    
    inputMappings->MapAction(PlayerController::_2, "Attack",    VirtualKey::MouseLeftBtn, {});
    inputMappings->MapAction(PlayerController::_2, "Attack",    VirtualKey::LeftControl, {});

    inputMappings->MapGamepadAction(PlayerController::_1,   "Attack",       GamepadKey::X);
    inputMappings->MapGamepadAction(PlayerController::_1,   "Attack",       GamepadAxis::TriggerRight);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveForward",  GamepadAxis::LeftY, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveRight",    GamepadAxis::LeftX, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveUp",       GamepadKey::A, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "TurnRight",    GamepadAxis::RightX, 200.0f);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "TurnUp",       GamepadAxis::RightY, 200.0f);

    inputMappings->MapGamepadAction(PlayerController::_2,   "Attack",       GamepadKey::X);
    inputMappings->MapGamepadAction(PlayerController::_2,   "Attack",       GamepadAxis::TriggerRight);
    inputMappings->MapGamepadAxis(PlayerController::_2,     "MoveForward",  GamepadAxis::LeftY, 1);
    inputMappings->MapGamepadAxis(PlayerController::_2,     "MoveRight",    GamepadAxis::LeftX, 1);
    inputMappings->MapGamepadAxis(PlayerController::_2,     "MoveUp",       GamepadKey::A, 1);
    inputMappings->MapGamepadAxis(PlayerController::_2,     "TurnRight",    GamepadAxis::RightX, 200.0f);
    inputMappings->MapGamepadAxis(PlayerController::_2,     "TurnUp",       GamepadAxis::RightY, 200.0f);

    sGetInputSystem().SetInputMappings(inputMappings);

    // Create game resources
    CreateResources();

    // Create game world
    m_World = CreateWorld();

    // Setup world collision
    m_World->GetInterface<PhysicsInterface>().SetCollisionFilter(CollisionLayer::CreateFilter());

    // Set rendering parameters
#ifdef SPLIT_SCREEN
    for (int i = 0; i < 2; ++i)
    {
        m_WorldRenderView[i] = MakeRef<WorldRenderView>();
        m_WorldRenderView[i]->SetWorld(m_World);
        m_WorldRenderView[i]->bClearBackground = false;
        m_WorldRenderView[i]->bDrawDebug = true;
    }
    m_Viewports[0]->SetWorldRenderView(m_WorldRenderView[0]);
    m_Viewports[1]->SetWorldRenderView(m_WorldRenderView[1]);
#else
    m_WorldRenderView[0] = MakeRef<WorldRenderView>();
    m_WorldRenderView[0]->SetWorld(m_World);
    m_WorldRenderView[0]->bClearBackground = true;
    m_WorldRenderView[0]->BackgroundColor = Color4(0.2f, 0.2f, 0.3f, 1);
    m_WorldRenderView[0]->bDrawDebug = true;
    mainViewport->SetWorldRenderView(m_WorldRenderView[0]);
#endif

    // Create scene
    CreateScene();

    // Create players
    GameObject* player = CreatePlayer(m_PlayerSpawnPoints[0].Position, m_PlayerSpawnPoints[0].Rotation, PlayerTeam::Blue);
    GameObject* player2 = CreatePlayer(m_PlayerSpawnPoints[1].Position, m_PlayerSpawnPoints[1].Rotation, PlayerTeam::Red);

    if (GameObject* camera = player->FindChildren(StringID("Camera")))
    {
        // Set camera for rendering
        m_WorldRenderView[0]->SetCamera(camera->GetComponentHandle<CameraComponent>());
    
        // Set audio listener
        auto& audio = m_World->GetInterface<AudioInterface>();
        audio.SetListener(camera->GetComponentHandle<AudioListenerComponent>());
    }

#ifdef SPLIT_SCREEN
    if (GameObject* camera = player2->FindChildren(StringID("Camera")))
    {
        // Set camera for rendering
        m_WorldRenderView[1]->SetCamera(camera->GetComponentHandle<CameraComponent>());
    }
#endif
    // Bind input to the player
    InputInterface& input = m_World->GetInterface<InputInterface>();
    input.SetActive(true);
    input.BindInput(player->GetComponentHandle<FirstPersonComponent>(), PlayerController::_2);    

    input.BindInput(player2->GetComponentHandle<FirstPersonComponent>(), PlayerController::_1);

    RenderInterface& render = m_World->GetInterface<RenderInterface>();
    render.SetAmbient(0.1f);

    auto& stateMachine = sGetStateMachine();

    stateMachine.Bind("State_Loading", this, &SampleApplication::OnStartLoading, {}, &SampleApplication::OnUpdateLoading);
    stateMachine.Bind("State_Play", this, &SampleApplication::OnStartPlay, {}, {});

    stateMachine.MakeCurrent("State_Loading");

    sGetCommandProcessor().Add("com_ShowStat 1\n");
    sGetCommandProcessor().Add("com_ShowFPS 1\n");
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

#if 0
    {
        auto& resourceMngr = GameApplication::GetResourceManager();

        GameObject* object;
        m_World->CreateObject({}, object);
        SoundSource* sound;
        object->CreateComponent(sound);
        sound->SetVolume(0.2f);
        sound->SetSourceType(SoundSourceType::Background);
        sound->PlaySound(resourceMngr.GetResource<SoundResource>("/Root/soundtrack.ogg"), 0, 0);
    }
#endif
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
    m_WorldRenderView[0]->bWireframe = !m_WorldRenderView[0]->bWireframe;
#ifdef SPLIT_SCREEN
    m_WorldRenderView[1]->bWireframe = !m_WorldRenderView[1]->bWireframe;
#endif
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
                texture->Upload();

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
        m_Desktop->SetFullscreenWidget(m_SplitView);
        m_Desktop->SetFocusWidget(m_Viewports[0]);
    }
}

void SampleApplication::CreateResources()
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    materialMngr.LoadLibrary("/Root/default/materials/default.mlib");

    // Procedurally generate a skybox image
    ImageStorage skyboxImage = RenderUtils::GenerateAtmosphereSkybox(sGetRenderDevice(), SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, Float3(1, -1, -1).Normalized());
    // Convert image to resource
    UniqueRef<TextureResource> skybox = MakeUnique<TextureResource>(std::move(skyboxImage));
    skybox->Upload();
    // Register the resource in the resource manager with the name "internal_skybox" so that it can be accessed by name from the materials.
    resourceMngr.CreateResourceWithData<TextureResource>("internal_skybox", std::move(skybox));

    // List of resources used in scene
    ResourceID sceneResources[] = {
        resourceMngr.GetResource<MeshResource>("/Root/default/skybox.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/capsule.mesh"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/default.mg"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/skybox.mg"),
        //resourceMngr.GetResource<TextureResource>("/Root/dirt.png"),
        resourceMngr.GetResource<TextureResource>("/Root/grid8.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank256.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/red512.png")
    };

    // Load resources asynchronously
    m_Resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(m_Resources);

    //resourceMngr.MainThread_WaitResourceArea(m_Resources);
}

void SampleApplication::CreateScene()
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    auto& materialMngr = GameApplication::sGetMaterialManager();

    CreateSceneFromMap(m_World, "/Root/sample2.map"/*, "dirt"*/);

    Float3 playerSpawnPosition = Float3(0,8.25f,28);
    Quat playerSpawnRotation = Quat::sIdentity();
    Float3 playerSpawnPosition2 = Float3(0,8.25f,-28);
    Quat playerSpawnRotation2 = Quat::sRotationAroundNormal(Math::_PI, Float3(0,1,0));

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
        dirlight->SetShadowCascadeOffset(0.0f);
        dirlight->SetShadowCascadeSplitLambda(0.8f);
    }

    // Platform
    {
        GameObject* object;

        GameObjectDesc desc;
        desc.Position = Float3(-8.75f, 6.5f, 0);
        desc.Scale = Float3(5.5f, 1, 4);
        desc.IsDynamic = true;
        m_World->CreateObject(desc, object);

        DynamicBodyComponent* dynamicBody;
        object->CreateComponent(dynamicBody);
        dynamicBody->SetKinematic(true);

        object->CreateComponent<BoxCollider>();

        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("grid8"));
        mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});

        uint32_t nodeID = 0;

        Ref<NodeMotion> animation = MakeRef<NodeMotion>();
        {
            NodeMotion::AnimationChannel& channel = animation->m_Channels.Add();
            channel.TargetNode = nodeID;
            channel.TargetPath = NODE_ANIMATION_PATH_TRANSLATION;
            channel.Smp.Offset = 0;
            channel.Smp.Count = 5;
            channel.Smp.DataOffset = 0;
            channel.Smp.Interpolation = INTERPOLATION_TYPE_LINEAR;

            animation->m_AnimationTimes.Add(0);
            animation->m_AnimationTimes.Add(2);
            animation->m_AnimationTimes.Add(5);
            animation->m_AnimationTimes.Add(7);
            animation->m_AnimationTimes.Add(10);

            animation->m_VectorData.EmplaceBack(-8.75f, 6.5f, 0);
            animation->m_VectorData.EmplaceBack(-8.75f, 6.5f, 0);

            animation->m_VectorData.EmplaceBack(8.75f, 6.5f, 0);
            animation->m_VectorData.EmplaceBack(8.75f, 6.5f, 0);

            animation->m_VectorData.EmplaceBack(-8.75f, 6.5f, 0);
        }

        NodeMotionComponent* nodeMotion;
        object->CreateComponent(nodeMotion);
        nodeMotion->Animation = animation;
        nodeMotion->Timer.LoopTime = 10;
        nodeMotion->NodeID = nodeID;
    }

    // Teleporter
    {
        GameObjectDesc desc;
        desc.Position = Float3(0, -20, 0);
        desc.Scale = Float3(200, 20, 200);
        GameObject* object;
        m_World->CreateObject(desc, object);
        TriggerComponent* phys;
        object->CreateComponent(phys);
        phys->CollisionLayer = CollisionLayer::Teleporter;
        object->CreateComponent<BoxCollider>();
        TeleporterComponent* teleport;
        object->CreateComponent(teleport);
        teleport->TeleportPoints[0] = {playerSpawnPosition, playerSpawnRotation};
        teleport->TeleportPoints[1] = {playerSpawnPosition2, playerSpawnRotation2};
    }

    // Jumpad
    {
        GameObjectDesc desc;
        desc.Position = Float3(0, 0.5f, 0);
        desc.Scale = Float3(4, 1, 4);
        GameObject* object;
        m_World->CreateObject(desc, object);
        TriggerComponent* phys;
        object->CreateComponent(phys);
        phys->CollisionLayer = CollisionLayer::CharacterOnlyTrigger;
        object->CreateComponent<BoxCollider>();
        JumpadComponent* jumpad;
        object->CreateComponent(jumpad);
        jumpad->ThrowVelocity = Float3(0, 20, 0);//Float3(0, 30, 0);
    }

    // Boxes
    {
        Float3 positions[] = {
            Float3( -21, 4, 27 ),
            Float3( -18, 4, 28 ),
            Float3( -23.5, 4, 26.5 ),
            Float3( -21, 7, 27 ) };

        float yaws[] = { 0, 15, 10, 10 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            GameObjectDesc desc;
            desc.Position = positions[i];
            desc.Rotation.FromAngles(0, Math::Radians(yaws[i]), 0);
            desc.Scale = Float3(2);
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

    CreateElevator(Float3(7.5f,4.25f,-28));
    CreateElevator(Float3(7.5f,4.25f,28));
    CreateElevator(Float3(-7.5f,4.25f,-28));
    CreateElevator(Float3(-7.5f,4.25f,28));

    m_PlayerSpawnPoints.Add({playerSpawnPosition, playerSpawnRotation});
    m_PlayerSpawnPoints.Add({playerSpawnPosition2, playerSpawnRotation2});
}

void SampleApplication::CreateElevator(Float3 const& position)
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    GameObject* object;

    GameObjectDesc desc;
    desc.Position = position;
    desc.Scale = Float3(3,0.5f,3.5f);
    desc.IsDynamic = true;
    m_World->CreateObject(desc, object);

    DynamicBodyComponent* dynamicBody;
    object->CreateComponent(dynamicBody);
    dynamicBody->SetKinematic(true);
    object->CreateComponent<BoxCollider>();

    DynamicMeshComponent* mesh;
    object->CreateComponent(mesh);
    mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
    mesh->SetMaterial(materialMngr.TryGet("grid8"));
    mesh->SetLocalBoundingBox({Float3(-0.5f),Float3(0.5f)});

    ElevatorComponent* elevatorComp;
    auto elevatorHandle = object->CreateComponent(elevatorComp);
    elevatorComp->MaxHeight = 3.5f;

    GameObject* triggerObject;
    desc = {};
    desc.Position = position + Float3::sAxisY() * 0.5f;
    desc.Scale = Float3(2.5f,0.5f,3);
    desc.IsDynamic = false;
    m_World->CreateObject(desc, triggerObject);
    TriggerComponent* trigger;
    triggerObject->CreateComponent(trigger);
    trigger->CollisionLayer = CollisionLayer::CharacterOnlyTrigger;
    triggerObject->CreateComponent<BoxCollider>();

    ElevatorActivatorComponent* activator;
    triggerObject->CreateComponent(activator);
    activator->Elevator = elevatorHandle;
}

GameObject* SampleApplication::CreatePlayer(Float3 const& position, Quat const& rotation, PlayerTeam team)
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    const float HeightStanding = 1.20f;
    const float RadiusStanding = 0.3f;

    // Create character controller
    GameObject* player;
    {
        GameObjectDesc desc;
        desc.Name.FromString("Player");
        desc.Position = position;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, player);

        CharacterControllerComponent* characterController;
        player->CreateComponent(characterController);
        characterController->SetCollisionLayer(CollisionLayer::Character);
        characterController->HeightStanding = HeightStanding;
        characterController->RadiusStanding = RadiusStanding;
        characterController->ShapeType = CharacterShapeType::Cylinder;

        //if (team==PlayerTeam::Blue)
        //    characterController->CanPushCharacter = false;
        //else
        //    characterController->CanPushCharacter = true;
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
        mesh->SetMaterial(materialMngr.TryGet(team == PlayerTeam::Blue ? "blank512" : "red512"));

        mesh->SetVisibilityLayer(team == PlayerTeam::Blue ? 1 : 2);
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
        uint32_t visLayers = 0xffffffff;
        if (team == PlayerTeam::Blue)
            visLayers &= ~(1 << 1);
        else
            visLayers &= ~(1 << 2);
        cameraComponent->SetVisibilityMask(visLayers);

        camera->CreateComponent<AudioListenerComponent>();
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

        mesh->SetVisibilityLayer(team == PlayerTeam::Blue ? 2 : 1);
    }

    // Create input
    FirstPersonComponent* playerPawn;
    player->CreateComponent(playerPawn);
    playerPawn->ViewPoint = camera->GetHandle();
    playerPawn->Team = team;

    return player;
}

using ApplicationClass = SampleApplication;
#include "Common/EntryPoint.h"
