/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "Common/Components/ThirdPersonComponent.h"
#include "Common/Components/DoorComponent.h"
#include "Common/Components/DoorActivatorComponent.h"
#include "Common/CollisionLayer.h"

#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/UI/UIImage.h>

#include <Hork/Runtime/Renderer/WorldRenderer.h>

#include <Hork/Runtime/World/Modules/Input/InputInterface.h>

#include <Hork/Runtime/World/Modules/Physics/CollisionFilter.h>
#include <Hork/Runtime/World/Modules/Physics/Components/StaticBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/TriggerComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>

#include <Hork/Runtime/World/Modules/Gameplay/Components/SpringArmComponent.h>

#include <Hork/Runtime/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>

#include <Hork/Runtime/World/Modules/Animation/Components/NodeMotionComponent.h>
#include <Hork/Runtime/World/Modules/Animation/NodeMotion.h>

#include <Hork/Runtime/World/Modules/Audio/AudioInterface.h>

using namespace Hk;

Handle32<CameraComponent> MainCamera;
GameObjectHandle Mirror;

void MirrorTransform(PlaneF const& mirrorPlane, Float3 const& inPosition, Quat const& inRotation, Float3& outPosition, Quat& outRotation)
{
    // Mirror position
    outPosition = inPosition + mirrorPlane.Normal * (-mirrorPlane.DistanceToPoint(inPosition) * 2.0f);

    // Mirror orientation
    const float sqrt2_div_2 = 0.707106769f;   // square root 2 divided by 2 (sin 45)
    Quat mirrorRotation = mirrorPlane.GetRotation() * Quat(sqrt2_div_2, 0,sqrt2_div_2,0);
    //Quat mirrorRotation = (mirrorPlane.Normal.X < -0.9999) ? Quat(0, 0, -1, 0) : Quat(mirrorPlane.Normal.X + 1.0f, 0, -mirrorPlane.Normal.Z, mirrorPlane.Normal.Y).Normalized();
    Quat localRotation = mirrorRotation.Conjugated() * inRotation;
    localRotation.X = -localRotation.X;
    localRotation.W = -localRotation.W;
    outRotation = mirrorRotation * localRotation;
}

class CameraMirrorComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    void LateUpdate()
    {
        if (auto mainCamera = GetWorld()->GetComponent(MainCamera))
        {
            auto& tick = GetWorld()->GetTick();

            // Compute the final camera position by interpolating the two frames.
            Float3 worldPos = Math::Lerp (mainCamera->GetPosition(tick.PrevStateIndex), mainCamera->GetPosition(tick.StateIndex), tick.Interpolate);
            Quat   worldRot = Math::Slerp(mainCamera->GetRotation(tick.PrevStateIndex), mainCamera->GetRotation(tick.StateIndex), tick.Interpolate);

            auto mirror = GetWorld()->GetObject(Mirror);

            PlaneF plane(mirror->GetBackVector(), mirror->GetWorldPosition());

            // Mirror camera relative to plane
            MirrorTransform(plane, worldPos, worldRot, worldPos, worldRot);

            // Set mirrored camera transform
            GetOwner()->SetWorldPosition(worldPos);
            GetOwner()->SetWorldRotation(worldRot);

            if (auto camera = GetOwner()->GetComponent<CameraComponent>())
            {
                // Update viewport size
                camera->SetViewportSize(mainCamera->GetViewportSize());

                // No interpolation needed (world pos already stores the interpolated value)
                camera->SkipInterpolation();
            }
        }
    }
};

SampleApplication::SampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Render To Texture")
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
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::W, 100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::S, -100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Up, 100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Down, -100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveRight",   VirtualKey::A, -100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveRight",   VirtualKey::D, 100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveUp",      VirtualKey::Space, 1.0f);
    inputMappings->MapAxis(PlayerController::_1, "TurnRight",   VirtualKey::Left, -200.0f);
    inputMappings->MapAxis(PlayerController::_1, "TurnRight",   VirtualKey::Right, 200.0f);

    inputMappings->MapAxis(PlayerController::_1, "FreelookHorizontal", VirtualAxis::MouseHorizontal, 1.0f);
    inputMappings->MapAxis(PlayerController::_1, "FreelookVertical",   VirtualAxis::MouseVertical, 1.0f);
    
    inputMappings->MapAxis(PlayerController::_1, "Run",         VirtualKey::LeftShift, 1.0f);
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

    // Create main render view
    m_WorldRenderView = MakeRef<WorldRenderView>();
    m_WorldRenderView->SetWorld(m_World);
    m_WorldRenderView->bClearBackground = true;
    m_WorldRenderView->BackgroundColor = Color3::sBlack();
    m_WorldRenderView->bDrawDebug = true;
    mainViewport->SetWorldRenderView(m_WorldRenderView);

    // Create offscreen render view. Use resolution of window frame buffer
    auto window = sGetUIManager().GetGenericWindow();
    uint32_t width = window->GetFramebufferWidth();
    uint32_t height = window->GetFramebufferHeight();
    m_OffscreenRenderView = MakeRef<WorldRenderView>();
    m_OffscreenRenderView->SetViewport(width, height);
    m_OffscreenRenderView->SetWorld(m_World);
    m_OffscreenRenderView->BackgroundColor = m_WorldRenderView->BackgroundColor;
    m_OffscreenRenderView->bClearBackground = true;
    m_OffscreenRenderView->bAllowMotionBlur = false;
    m_OffscreenRenderView->TextureFormat = TEXTURE_FORMAT_RGBA16_FLOAT;
    m_OffscreenRenderView->Brightness = 1;
    m_OffscreenRenderView->AcquireRenderTarget();

    sGetStateMachine().Bind("State_Play", this, &SampleApplication::OnStartPlay, {}, &SampleApplication::OnUpdate);
    sGetStateMachine().MakeCurrent("State_Play");
}

void SampleApplication::Deinitialize()
{
    DestroyWorld(m_World);
}

void SampleApplication::OnStartPlay()
{
    // Create scene
    CreateScene();

    // Create player
    GameObject* player = CreatePlayer(Float3(10,0,0), Quat::sRotationY(Math::_HALF_PI));

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

    RenderInterface& render = m_World->GetInterface<RenderInterface>();
    render.SetAmbient(0.001f);
}

void SampleApplication::OnUpdate(float timeStep)
{
    // We must add the render view to renderer for offscreen rendering
    GameApplication::sGetRenderer().AddRenderView(m_OffscreenRenderView);
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
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/default.mat"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/mirror.mat"),
        resourceMngr.GetResource<TextureResource>("/Root/dirt.png"),
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

    // Create level geometry
    CreateSceneFromMap(m_World, "/Root/maps/sample4.map", "dirt");

    // Create mirror
    {
        GameObjectDesc desc;
        desc.Position = Float3(0,4,0);
        desc.Rotation.FromAngles(0, Math::Radians(90), 0);
        desc.IsDynamic=true;
        GameObject* mirror;
        m_World->CreateObject(desc, mirror);
        DynamicMeshComponent* face;
        mirror->CreateComponent(face);

        BoxCollider* boxCollider;
        mirror->CreateComponent(boxCollider);
        boxCollider->HalfExtents = Float3(2,4,0.05f);
        StaticBodyComponent* body;
        mirror->CreateComponent(body);

        Mirror= mirror->GetHandle();

        RawMesh rawMesh;
        rawMesh.CreatePlaneXY(4.0f, 8.0f);

        MeshResourceBuilder builder;
        UniqueRef<MeshResource> quadMesh = builder.Build(rawMesh);
        if (quadMesh)
            quadMesh->Upload(sGetRenderDevice());

        auto surfaceHandle = resourceMngr.CreateResourceWithData<MeshResource>("mirror_surface", std::move(quadMesh));

        face->SetMesh(surfaceHandle);
        face->SetLocalBoundingBox(rawMesh.CalcBoundingBox());
        
        Ref<MaterialLibrary> matlib = materialMngr.CreateLibrary();
        Material* material = matlib->CreateMaterial("render_to_tex_material");
        material->SetResource(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/mirror.mat"));
        material->SetTexture(0, m_OffscreenRenderView->GetTextureHandle());
        face->SetMaterial(material);
    }
    {
        GameObjectDesc desc;
        desc.Position = Float3(12,2,0);
        GameObject* renderCamera;
        m_World->CreateObject(desc, renderCamera);
        CameraComponent* cameraComponent;
        auto cameraHandle = renderCamera->CreateComponent(cameraComponent);
        cameraComponent->SetFovY(75);
        cameraComponent->SetExposure(0);
        m_OffscreenRenderView->SetCamera(cameraHandle);
        renderCamera->CreateComponent<CameraMirrorComponent>();
        m_OffscreenRenderView->SetCamera(cameraHandle);
    }

    // Create first point light
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(12,2.3f,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(300);
    }

    // Create second point light
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(-12,2.3f,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(300);
    }

    // Create boxes
    {
        Float3 positions[] = {
            Float3( 6, 0, -4 ),
            Float3( 9, 0, -3 ),
            Float3( 3.5, 0, -4.5 ),
            Float3( 6, 3, -4 )};

        float yaws[] = { 0, 15, 10, 10 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            GameObjectDesc desc;
            desc.Position = positions[i];
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
        MainCamera = camera->CreateComponent(cameraComponent);
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
