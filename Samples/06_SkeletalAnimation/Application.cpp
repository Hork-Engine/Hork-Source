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

#include <Hork/Runtime/World/Modules/Skeleton/Components/AnimationPlayerSimple.h>

#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Resources/Resource_Animation.h>

const char* PaladinModel = "/Root/thirdparty/mixamo/paladin/paladin.mesh";
const char* PaladinMaterial = "/Root/thirdparty/mixamo/paladin/paladin.mg";

const char* PaladinAnimations[] =
{
    "/Root/thirdparty/mixamo/paladin/idle-1.anim",
    "/Root/thirdparty/mixamo/paladin/idle-3.anim",
    "/Root/thirdparty/mixamo/paladin/casting-1.anim",
    "/Root/thirdparty/mixamo/paladin/impact-2.anim",
    "/Root/thirdparty/mixamo/paladin/kick.anim"
};

const char* PaladinTextures[] =
{
    "/Root/thirdparty/mixamo/paladin/albedo.tex",
    "/Root/thirdparty/mixamo/paladin/normal.tex"
};

const char* PaladinMaterialLib = "/Root/thirdparty/mixamo/paladin/paladin.mlib";

HK_NAMESPACE_BEGIN

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Skeletal Animation")
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
    shortcuts->AddShortcut(VirtualKey::F1, {}, {this, &SampleApplication::SetAnimationIdle1});
    shortcuts->AddShortcut(VirtualKey::F2, {}, {this, &SampleApplication::SetAnimationIdle3});
    shortcuts->AddShortcut(VirtualKey::F3, {}, {this, &SampleApplication::SetAnimationCasting1});
    shortcuts->AddShortcut(VirtualKey::F4, {}, {this, &SampleApplication::SetAnimationImpact});
    shortcuts->AddShortcut(VirtualKey::F5, {}, {this, &SampleApplication::SetAnimationKick});
    shortcuts->AddShortcut(VirtualKey::E, {}, {this, &SampleApplication::DropBarrel});
    shortcuts->AddShortcut(VirtualKey::R, {}, {this, &SampleApplication::SpawnPaladin});
    shortcuts->AddShortcut(VirtualKey::F6, {}, {this, &SampleApplication::ShowSkeleton});

    desktop->SetShortcuts(shortcuts);

    // Create viewport
    desktop->AddWidget(UINewAssign(m_Viewport, UIViewport)
                           .WithPadding({0, 0, 0, 0})
                           .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_BOTTOM))
                               [UINew(UILabel)
                                    .WithText(UINew(UIText, "F1 Idle, F2 Idle, F3 Cast, F4 Impact, F5 Kick, F6 Show Skeleton")
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
    m_Player = CreatePlayer(Float3(0,0,4), Quat::sIdentity());

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
    materialMngr.LoadLibrary(PaladinMaterialLib);
    materialMngr.LoadLibrary("/Root/thirdparty/freepbr.com/freepbr.mlib");
    materialMngr.LoadLibrary("/Root/thirdparty/sketchfab.com/sketchfab.mlib");

    // List of resources used in scene
    SmallVector<ResourceID, 32> sceneResources;

    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"));

    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"));
    sceneResources.Add(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default_orm.mat"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/black.png"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/dirt.png"));

    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/albedo.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/orm.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/freepbr.com/grime-alley-brick2/normal.tex"));

    sceneResources.Add(resourceMngr.GetResource<MeshResource>("/Root/thirdparty/sketchfab.com/barrel/barrel.mesh"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/sketchfab.com/barrel/albedo.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/sketchfab.com/barrel/orm.tex"));
    sceneResources.Add(resourceMngr.GetResource<TextureResource>("/Root/thirdparty/sketchfab.com/barrel/normal.tex"));

    // Paladin resources
    sceneResources.Add(resourceMngr.GetResource<MeshResource>(PaladinModel));
    sceneResources.Add(resourceMngr.GetResource<MaterialResource>(PaladinMaterial));
    for (auto animation : PaladinAnimations)
        sceneResources.Add(resourceMngr.GetResource<AnimationResource>(animation));
    for (auto texture : PaladinTextures)
        sceneResources.Add(resourceMngr.GetResource<TextureResource>(texture));

    // Load resources asynchronously
    m_Resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(m_Resources);
}

void SampleApplication::CreateScene()
{
    SpawnPaladin(Float3(0,0,-2), Quat::sIdentity(), 1);

    // Barrels
    {
        Float3 positions[] = {
            Float3( -2.5, 0.5, -1 ),
            Float3( 2, 0.5, 1 ),
            Float3( -1.5, 0.5, -1.5 ),
            Float3( -2, 1.5, -1 ) };

        float yaws[] = { 0, 15, 10, 10 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            Quat rotation;
            rotation.FromAngles(0, Math::Radians(yaws[i]), 0);
            SpawnBarrel(positions[i], rotation);
        }
    }

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
        light->SetLumens(2500);
        light->SetRadius(10);
    }

    // Room
    CreateSceneFromMap(m_World, "/Root/maps/sample6.map", "grime-alley-brick2");
}

void SampleApplication::SetAnimationIdle1()
{
    if (auto animPlayer = m_World->GetComponent(m_AnimPlayer))
    {
        auto& resourceMngr = GameApplication::sGetResourceManager();
        animPlayer->PlayAnimation(resourceMngr.GetResource<AnimationResource>(PaladinAnimations[0]), 0.1f, 0.0f);
    }
}
void SampleApplication::SetAnimationIdle3()
{
    if (auto animPlayer = m_World->GetComponent(m_AnimPlayer))
    {
        auto& resourceMngr = GameApplication::sGetResourceManager();
        animPlayer->PlayAnimation(resourceMngr.GetResource<AnimationResource>(PaladinAnimations[1]), 0.1f, 0.0f);
    }
}
void SampleApplication::SetAnimationCasting1()
{
    if (auto animPlayer = m_World->GetComponent(m_AnimPlayer))
    {
        auto& resourceMngr = GameApplication::sGetResourceManager();
        animPlayer->PlayAnimation(resourceMngr.GetResource<AnimationResource>(PaladinAnimations[2]), 0.1f, 0.0f);
    }
}
void SampleApplication::SetAnimationImpact()
{
    if (auto animPlayer = m_World->GetComponent(m_AnimPlayer))
    {
        auto& resourceMngr = GameApplication::sGetResourceManager();
        animPlayer->PlayAnimation(resourceMngr.GetResource<AnimationResource>(PaladinAnimations[3]), 0.1f, 0.0f);
    }
}
void SampleApplication::SetAnimationKick()
{
    if (auto animPlayer = m_World->GetComponent(m_AnimPlayer))
    {
        auto& resourceMngr = GameApplication::sGetResourceManager();
        animPlayer->PlayAnimation(resourceMngr.GetResource<AnimationResource>(PaladinAnimations[4]), 0.1f, 0.0f);
    }
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

void SampleApplication::SpawnPaladin()
{
    if (m_Player)
    {
        if (GameObject* camera = m_Player->FindChildren(StringID("Camera")))
        {
            Float3 rayPos = m_Player->GetPosition() + camera->GetWorldForwardVector() + Float3::sAxisY();

            RayCastResult rayResult;
            RayCastFilter rayFilter;
            rayFilter.BroadphaseLayers.AddLayer(BroadphaseLayer::Static);

            if (m_World->GetInterface<PhysicsInterface>().CastRayClosest(rayPos, -Float3::sAxisY(), rayResult, rayFilter))
            {
                Float3 pos = rayPos - Float3::sAxisY() * rayResult.Fraction;
                SpawnPaladin(pos, Quat::sIdentity(), sGetRandom().Get());
            }            
        }
    }
}

void SampleApplication::SpawnPaladin(Float3 const& position, Quat const& rotation, int anim)
{
    auto& resourceMngr = sGetResourceManager();
    auto& materialMngr = sGetMaterialManager();

    static MeshHandle meshHandle = resourceMngr.GetResource<MeshResource>(PaladinModel);

    GameObjectDesc desc;
    desc.IsDynamic = true;
    desc.Position = position;
    desc.Rotation = rotation;

    GameObject* object;
    m_World->CreateObject(desc, object);

    AnimationPlayerSimple* animPlayer;
    m_AnimPlayer = object->CreateComponent(animPlayer);
    animPlayer->SetMesh(meshHandle);
    animPlayer->PlayAnimation(resourceMngr.GetResource<AnimationResource>(PaladinAnimations[anim % HK_ARRAY_SIZE(PaladinAnimations)]), 0.1f, 0.0f);

    MeshResource* meshResource = resourceMngr.TryGet(meshHandle);

    DynamicMeshComponent* mesh;
    object->CreateComponent(mesh);
    mesh->SetMesh(meshHandle);
    mesh->SetPose(animPlayer->GetPose());
    mesh->SetMaterialCount(meshResource->GetSurfaceCount());
    for (int i = 0 ; i < meshResource->GetSurfaceCount(); ++i)
        mesh->SetMaterial(i, materialMngr.TryGet("thirdparty/mixamo/paladin"));
    mesh->SetLocalBoundingBox({{-0.4f,0,-0.4f},{0.4f,1.8f,0.4f}});

    CapsuleCollider* collider;
    object->CreateComponent(collider);
    collider->Radius = 0.3f;
    collider->Height = 1.2f;
    collider->OffsetPosition.Y = (collider->Radius*2 + collider->Height)/2;
    DynamicBodyComponent* body;
    object->CreateComponent(body);
    body->CanPushCharacter = false;
    body->SetKinematic(true);
}

void SampleApplication::SpawnBarrel(Float3 const& position, Quat const& rotation)
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
    phys->Mass = 50;
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
}

void SampleApplication::DropBarrel()
{
    if (m_Player)
    {
        if (GameObject* camera = m_Player->FindChildren(StringID("Camera")))
        {
            SpawnBarrel(m_Player->GetPosition() + camera->GetWorldForwardVector() * 0.8f + Float3::sAxisY(), Quat::sIdentity());            
        }
    }
}

extern ConsoleVar com_DrawSkeletons;

void SampleApplication::ShowSkeleton()
{
    com_DrawSkeletons = !com_DrawSkeletons;
}

HK_NAMESPACE_END

using ApplicationClass = Hk::SampleApplication;
#include "Common/EntryPoint.h"
