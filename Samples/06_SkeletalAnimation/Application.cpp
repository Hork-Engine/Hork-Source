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

#include <Hork/Runtime/World/Modules/Skeleton/Components/AnimatorComponent.h>

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
    "/Root/thirdparty/mixamo/paladin/kick.anim",

    "/Root/thirdparty/mixamo/paladin/block.anim",
    "/Root/thirdparty/mixamo/paladin/block-idle.anim",
    "/Root/thirdparty/mixamo/paladin/casting.anim",
    "/Root/thirdparty/mixamo/paladin/casting-1.anim",

    "/Root/thirdparty/mixamo/paladin/slash.anim",
    "/Root/thirdparty/mixamo/paladin/slash-1.anim",
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
    shortcuts->AddShortcut(VirtualKey::F1, {}, {this, &SampleApplication::SetAnimationBlock});
    shortcuts->AddShortcut(VirtualKey::F2, {}, {this, &SampleApplication::SetAnimationCast});
    shortcuts->AddShortcut(VirtualKey::F3, {}, {this, &SampleApplication::SetAnimationSlash});
    shortcuts->AddShortcut(VirtualKey::F4, {}, {this, &SampleApplication::SetAnimationIdle});
    shortcuts->AddShortcut(VirtualKey::E, {}, {this, &SampleApplication::DropBarrel});
    shortcuts->AddShortcut(VirtualKey::R, {}, {this, &SampleApplication::SpawnPaladin});
    shortcuts->AddShortcut(VirtualKey::F6, {}, {this, &SampleApplication::ShowSkeleton});

    desktop->SetShortcuts(shortcuts);

    // Create viewport
    desktop->AddWidget(UINewAssign(m_Viewport, UIViewport)
                           .WithPadding({0, 0, 0, 0})
                           .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_BOTTOM))
                               [UINew(UILabel)
                                    .WithText(UINew(UIText, "F1 Block, F2 Cast, F3 Slash, F4 Idle, F6 Show Skeleton")
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

enum class state { idle, block, slash, cast };

void SampleApplication::SetAnimationBlock()
{
    if (auto animator = m_World->GetComponent(m_Animator))
    {
        static StringID ParamID_State{"State"};

        animator->SetParam(ParamID_State, state::block);
    }
}

void SampleApplication::SetAnimationCast()
{
    if (auto animator = m_World->GetComponent(m_Animator))
    {
        static StringID ParamID_State{"State"};

        animator->SetParam(ParamID_State, state::cast);
    }
}

void SampleApplication::SetAnimationSlash()
{
    if (auto animator = m_World->GetComponent(m_Animator))
    {
        static StringID ParamID_State{"State"};

        animator->SetParam(ParamID_State, state::slash);
    }
}

void SampleApplication::SetAnimationIdle()
{
    if (auto animator = m_World->GetComponent(m_Animator))
    {
        static StringID ParamID_State{"State"};

        animator->SetParam(ParamID_State, state::idle);
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

Ref<AnimationGraph_Cooked> CreateTestAnimationGraph()
{
    const StringID ParamID_State{"State"};
    const StringID ParamID_PlaybackSpeed{"PlaybackSpeed"};

    AnimationGraph graph;

    // Clips

    auto& idle = graph.AddNode<AnimGraph_Clip>();
    idle.SetClipID(/*"idle"*/"/Root/thirdparty/mixamo/paladin/idle-3.anim");

    auto& blockStart = graph.AddNode<AnimGraph_Clip>();
    blockStart.SetClipID(/*"block_start"*/ "/Root/thirdparty/mixamo/paladin/block.anim");

    auto& blockIdle = graph.AddNode<AnimGraph_Clip>();
    blockIdle.SetClipID(/*"block_idle"*/ "/Root/thirdparty/mixamo/paladin/block-idle.anim");

    auto& slash0 = graph.AddNode<AnimGraph_Clip>();
    slash0.SetClipID(/*"slash_0"*/ "/Root/thirdparty/mixamo/paladin/slash.anim");

    auto& slash1 = graph.AddNode<AnimGraph_Clip>();
    slash1.SetClipID(/*"slash_1"*/ "/Root/thirdparty/mixamo/paladin/slash-1.anim");

    auto& slash = graph.AddNode<AnimGraph_Random>();
    slash.SetChildrenNodes({slash0.GetID(), slash1.GetID()});

    auto& cast0 = graph.AddNode<AnimGraph_Clip>();
    cast0.SetClipID(/*"cast_0"*/ "/Root/thirdparty/mixamo/paladin/casting.anim");

    auto& cast1 = graph.AddNode<AnimGraph_Clip>();
    cast1.SetClipID(/*"cast_1"*/ "/Root/thirdparty/mixamo/paladin/casting-1.anim");

    auto& cast = graph.AddNode<AnimGraph_Random>();
    cast.SetChildrenNodes({cast0.GetID(), cast1.GetID()});

    // Nested state machine for block (start + idle states)

    auto& stateBlockStart = graph.AddNode<AnimGraph_State>();
    stateBlockStart.SetPoseNode(blockStart.GetID());
    stateBlockStart.SetName("block start");

    auto& stateBlockIdle = graph.AddNode<AnimGraph_State>();
    stateBlockIdle.SetPoseNode(blockIdle.GetID());
    stateBlockIdle.SetName("block idle");

    auto& conditionBlockStartEnded = graph.AddNode<AnimGraph_StateCondition>();
    conditionBlockStartEnded.SetPhase(1.0f);

    auto& transitionBlockStartToBlockIdle = graph.AddNode<AnimGraph_StateTransition>();
    transitionBlockStartToBlockIdle.SetConditionNode(conditionBlockStartEnded.GetID());
    transitionBlockStartToBlockIdle.SetDestinationStateNode(stateBlockIdle.GetID());
    transitionBlockStartToBlockIdle.SetDuration(0);//(0.1f);
    stateBlockStart.AddOutputTransitionNode(transitionBlockStartToBlockIdle.GetID());

    auto& blockStateMachine = graph.AddNode<AnimGraph_StateMachine>();
    blockStateMachine.SetStateNodes({stateBlockStart.GetID(), stateBlockIdle.GetID()});

    // Main state machine

    auto& stateIdle = graph.AddNode<AnimGraph_State>();
    stateIdle.SetPoseNode(idle.GetID());
    stateIdle.SetName("idle");

    auto& stateBlock = graph.AddNode<AnimGraph_State>();
    stateBlock.SetPoseNode(blockStateMachine.GetID());
    stateBlock.SetName("block");

    auto& stateSlash = graph.AddNode<AnimGraph_State>();
    stateSlash.SetPoseNode(slash.GetID());
    stateSlash.SetName("slash");

    auto& stateCast = graph.AddNode<AnimGraph_State>();
    stateCast.SetPoseNode(cast.GetID());
    stateCast.SetName("cast");

    auto& conditionBlockEnded = graph.AddNode<AnimGraph_ParamComparison>();
    conditionBlockEnded.SetParamID(ParamID_State);
    conditionBlockEnded.SetValue(static_cast<int>(state::block));
    conditionBlockEnded.SetOp(AnimGraph_ParamComparison::Op::NotEqual);

    auto& conditionStateIsBlock = graph.AddNode<AnimGraph_ParamComparison>();
    conditionStateIsBlock.SetParamID(ParamID_State);
    conditionStateIsBlock.SetValue(static_cast<int>(state::block));

    auto& conditionStateIsSlash = graph.AddNode<AnimGraph_ParamComparison>();
    conditionStateIsSlash.SetParamID(ParamID_State);
    conditionStateIsSlash.SetValue(static_cast<int>(state::slash));

    auto& conditionSlashAnimationEnded = graph.AddNode<AnimGraph_StateCondition>();
    conditionSlashAnimationEnded.SetPhase(1.0f);

    auto& conditionSlashStateEnded = graph.AddNode<AnimGraph_ParamComparison>();
    conditionSlashStateEnded.SetParamID(ParamID_State);
    conditionSlashStateEnded.SetValue(static_cast<int>(state::slash));
    conditionSlashStateEnded.SetOp(AnimGraph_ParamComparison::Op::NotEqual);

    auto& conditionSlashEnded = graph.AddNode<AnimGraph_And>();
    conditionSlashEnded.SetChildrenNodes({conditionSlashAnimationEnded.GetID(), conditionSlashStateEnded.GetID()});

    auto& conditionStateIsCast = graph.AddNode<AnimGraph_ParamComparison>();
    conditionStateIsCast.SetParamID(ParamID_State);
    conditionStateIsCast.SetValue(static_cast<int>(state::cast));

    auto& conditionCastAnimationEnded = graph.AddNode<AnimGraph_StateCondition>();
    conditionCastAnimationEnded.SetPhase(1.0f);

    auto& conditionCastStateEnded = graph.AddNode<AnimGraph_ParamComparison>();
    conditionCastStateEnded.SetParamID(ParamID_State);
    conditionCastStateEnded.SetValue(static_cast<int>(state::cast));
    conditionCastStateEnded.SetOp(AnimGraph_ParamComparison::Op::NotEqual);

    auto& conditionCastEnded = graph.AddNode<AnimGraph_And>();
    conditionCastEnded.SetChildrenNodes({conditionCastAnimationEnded.GetID(), conditionCastStateEnded.GetID()});

    auto& transitionIdleToBlock = graph.AddNode<AnimGraph_StateTransition>();
    transitionIdleToBlock.SetConditionNode(conditionStateIsBlock.GetID());
    transitionIdleToBlock.SetDestinationStateNode(stateBlock.GetID());
    transitionIdleToBlock.SetDuration(0.1F);
    stateIdle.AddOutputTransitionNode(transitionIdleToBlock.GetID());

    auto& transitionBlockToIdle = graph.AddNode<AnimGraph_StateTransition>();
    transitionBlockToIdle.SetConditionNode(conditionBlockEnded.GetID());
    transitionBlockToIdle.SetDestinationStateNode(stateIdle.GetID());
    transitionBlockToIdle.SetDuration(0.2F);
    stateBlock.AddOutputTransitionNode(transitionBlockToIdle.GetID());

    auto& transitionIdleToSlash = graph.AddNode<AnimGraph_StateTransition>();
    transitionIdleToSlash.SetConditionNode(conditionStateIsSlash.GetID());
    transitionIdleToSlash.SetDestinationStateNode(stateSlash.GetID());
    transitionIdleToSlash.SetDuration(0.1F);
    stateIdle.AddOutputTransitionNode(transitionIdleToSlash.GetID());

    auto& transitionSlashToIdle = graph.AddNode<AnimGraph_StateTransition>();
    transitionSlashToIdle.SetConditionNode(conditionSlashEnded.GetID());
    transitionSlashToIdle.SetDestinationStateNode(stateIdle.GetID());
    transitionSlashToIdle.SetDuration(0.1F);
    stateSlash.AddOutputTransitionNode(transitionSlashToIdle.GetID());

    auto& transitionIdleToCast = graph.AddNode<AnimGraph_StateTransition>();
    transitionIdleToCast.SetConditionNode(conditionStateIsCast.GetID());
    transitionIdleToCast.SetDestinationStateNode(stateCast.GetID());
    transitionIdleToCast.SetDuration(0.1F);
    stateIdle.AddOutputTransitionNode(transitionIdleToCast.GetID());

    auto& transitionCastToIdle = graph.AddNode<AnimGraph_StateTransition>();
    transitionCastToIdle.SetConditionNode(conditionCastEnded.GetID());
    transitionCastToIdle.SetDestinationStateNode(stateIdle.GetID());
    transitionCastToIdle.SetDuration(0.1F);
    stateCast.AddOutputTransitionNode(transitionCastToIdle.GetID());

    auto& stateMachine = graph.AddNode<AnimGraph_StateMachine>();
    stateMachine.SetStateNodes({stateIdle.GetID(), stateBlock.GetID(), stateCast.GetID(), stateSlash.GetID()});


    // Playback speed node and input param

    auto& playbackSpeedParam = graph.AddNode<AnimGraph_Param>();
    playbackSpeedParam.SetParamID(ParamID_PlaybackSpeed);

    auto& playback = graph.AddNode<AnimGraph_Playback>();
    playback.SetSpeedProviderNode(playbackSpeedParam.GetID());
    playback.SetChildNode(stateMachine.GetID());

    graph.SetRootNode(playback.GetID());

    graph.Validate();

    return graph.Cook();
}

Ref<AnimationGraph_Cooked> CreateBlendTest()
{
    // Blending animation graph:
    //  - three standing movement animations, blended by speed
    //  - two crouching movement animations, blended by speed
    //  - standing and movement animations are blended by crouching parameter
    // Additional node controls playback speed of this whole tree.

    AnimationGraph graph;

    StringID ParamID_Speed("Speed");
    StringID ParamID_Crouch("Crouch");
    StringID ParamID_PlaybackSpeed("PlaybackSpeed");

    constexpr float param_speed_walk{1.0F};
    constexpr float param_speed_jog{2.0F};
    constexpr float param_speed_run{3.0F};

    //
    // Animation clips
    //

    auto& walk = graph.AddNode<AnimGraph_Clip>();
    walk.SetClipID("Walk");

    auto& jog = graph.AddNode<AnimGraph_Clip>();
    jog.SetClipID("jog");

    auto& run = graph.AddNode<AnimGraph_Clip>();
    run.SetClipID("run");

    auto& crouchWalk = graph.AddNode<AnimGraph_Clip>();
    crouchWalk.SetClipID("walk_crouch");

    auto& crouchRun = graph.AddNode<AnimGraph_Clip>();
    crouchRun.SetClipID("run_crouch");

    //
    // Parameters
    //

    auto& walkSpeed = graph.AddNode<AnimGraph_Param>();
    walkSpeed.SetParamID(ParamID_Speed);

    auto& crouchSpeed = graph.AddNode<AnimGraph_Param>();
    crouchSpeed.SetParamID(ParamID_Speed);

    auto& crouchParam = graph.AddNode<AnimGraph_Param>();
    crouchParam.SetParamID(ParamID_Crouch);

    auto& playbackSpeedParam = graph.AddNode<AnimGraph_Param>();
    playbackSpeedParam.SetParamID(ParamID_PlaybackSpeed);

    //
    // Blending
    //

    auto& blendWalkJogRun = graph.AddNode<AnimGraph_Blend>();
    blendWalkJogRun.AddPoseNode(walk.GetID(), param_speed_walk);
    blendWalkJogRun.AddPoseNode(jog.GetID(), param_speed_jog);
    blendWalkJogRun.AddPoseNode(run.GetID(), param_speed_run);
    blendWalkJogRun.SetFactorNodeID(walkSpeed.GetID());

    auto& blendCrouchWalkRun = graph.AddNode<AnimGraph_Blend>();
    blendCrouchWalkRun.AddPoseNode(crouchWalk.GetID(), param_speed_walk);
    blendCrouchWalkRun.AddPoseNode(crouchRun.GetID(), param_speed_run);
    blendCrouchWalkRun.SetFactorNodeID(crouchSpeed.GetID());

    auto& blendStandCrouch = graph.AddNode<AnimGraph_Blend>();
    blendStandCrouch.AddPoseNode(blendWalkJogRun.GetID(), 0.0f);
    blendStandCrouch.AddPoseNode(blendCrouchWalkRun.GetID(), 1.0f);
    blendStandCrouch.SetFactorNodeID(crouchParam.GetID());

    //
    // Playback node
    //

    auto& playback = graph.AddNode<AnimGraph_Playback>();
    playback.SetSpeedProviderNode(playbackSpeedParam.GetID());
    playback.SetChildNode(blendStandCrouch.GetID());

    graph.SetRootNode(playback.GetID());
#if 0
    AnimationPlayerContext context;
    context.SetParam<float>(ParamID_Speed, param_speed_walk);
    context.SetParam<float>(ParamID_Crouch, 0.0f);
    context.SetParam<float>(ParamID_PlaybackSpeed, 1.0f);

    int numSeconds = 10;
    int n = 60 * numSeconds;
    for (int i = 0 ; i < n ; i++)
    {
        if (i / 60 > 3)
            context.SetParam<float>(ParamID_Speed, 1.5f);
        graph.UpdatePlayer(context, 1.0f/60);
    }
#endif

    return graph.Cook();
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

    static Ref<AnimationGraph_Cooked> animGraph = CreateTestAnimationGraph();

    SkeletonPoseComponent* pose;
    object->CreateComponent(pose);
    pose->SetMesh(meshHandle);

    AnimatorComponent* animator;
    m_Animator = object->CreateComponent(animator);
    animator->SetAnimationGraph(animGraph.RawPtr());
    animator->SetMesh(meshHandle);

    animator->SetParam(StringID{"PlaybackSpeed"}, 1.0f);

    MeshResource* meshResource = resourceMngr.TryGet(meshHandle);

    DynamicMeshComponent* mesh;
    object->CreateComponent(mesh);
    mesh->SetMesh(meshHandle);
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
