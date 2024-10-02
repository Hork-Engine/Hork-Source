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

#include <Hork/Runtime/GameApplication/GameApplication.h>
#include <Hork/Runtime/UI/UIViewport.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Hork/Runtime/World/Modules/Render/RenderInterface.h>
#include <Hork/Runtime/World/Modules/Input/InputInterface.h>
#include <Hork/Runtime/World/DebugRenderer.h>

using namespace Hk;

class PlayerComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    void BindInput(InputBindings& input)
    {
        input.BindAxis("MoveForward", this, &PlayerComponent::MoveForward);
        input.BindAxis("MoveRight", this, &PlayerComponent::MoveRight);
        input.BindAxis("MoveUp", this, &PlayerComponent::MoveUp);
        input.BindAxis("MoveDown", this, &PlayerComponent::MoveDown);
        input.BindAxis("TurnRight", this, &PlayerComponent::TurnRight);
        input.BindAxis("FreelookHorizontal", this, &PlayerComponent::FreelookHorizontal);
    }

    void MoveForward(float amount)
    {
        GetOwner()->Move(GetOwner()->GetForwardVector() * amount * GetWorld()->GetTick().FrameTimeStep);
    }

    void MoveRight(float amount)
    {
        GetOwner()->Move(GetOwner()->GetRightVector() * amount * GetWorld()->GetTick().FrameTimeStep);
    }

    void MoveUp(float amount)
    {
        GetOwner()->Move(Float3::sAxisY() * amount * GetWorld()->GetTick().FrameTimeStep);
    }

    void MoveDown(float amount)
    {
        GameObject* owner = GetOwner();
        Hk::Float3 pos = owner->GetWorldPosition();
        pos.Y -= amount * GetWorld()->GetTick().FrameTimeStep;
        if (pos.Y < 0)
            pos.Y = 0;
        owner->SetWorldPosition(pos);
    }

    void TurnRight(float amount)
    {
        const float RotationSpeed = 1;
        GetOwner()->Rotate(-amount * GetWorld()->GetTick().FrameTimeStep * RotationSpeed, Float3::sAxisY());
    }

    void FreelookHorizontal(float amount)
    {
        const float RotationSpeed = 1;
        GetOwner()->Rotate(-amount * RotationSpeed, Float3::sAxisY());
    }

    void DrawDebug(DebugRenderer& renderer)
    {
        GameObject* owner = GetOwner();
        Float3 pos = owner->GetWorldPosition();
        Float3 dir = owner->GetWorldForwardVector();
        Float3 p1 = pos + dir * 0.5f;
        Float3 p2 = pos + dir * 2.0f;
        renderer.SetColor(Color4::sBlue());
        renderer.DrawLine(p1, p2);
        renderer.DrawCone(p2, owner->GetWorldRotation().ToMatrix3x3() * Float3x3::sRotationAroundNormal(Math::_PI, Float3(1, 0, 0)), 0.4f, 30);
    }
};

class SampleApplication final : public GameApplication
{
    World*                      m_World{};
    Ref<WorldRenderView>        m_WorldRenderView;
    Handle32<CameraComponent>   m_MainCamera;

public:
    SampleApplication(ArgumentPack const& args) :
        GameApplication(args, "Hork Engine: Hello World")
    {}

    void Initialize()
    {
        // Set input mappings
        Ref<InputMappings> inputMappings = MakeRef<InputMappings>();
        inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::W, 1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::S, -1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Up, 1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Down, -1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveRight", VirtualKey::A, -1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveRight", VirtualKey::D, 1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveUp", VirtualKey::Space, 1.0f);
        inputMappings->MapAxis(PlayerController::_1, "MoveDown", VirtualKey::C, 1.0f);
        inputMappings->MapAxis(PlayerController::_1, "FreelookHorizontal", VirtualAxis::MouseHorizontal, 1.0f);
        inputMappings->MapAxis(PlayerController::_1, "TurnRight", VirtualKey::Left, -90.0f);
        inputMappings->MapAxis(PlayerController::_1, "TurnRight", VirtualKey::Right, 90.0f);

        sGetInputSystem().SetInputMappings(inputMappings);

        // Set rendering parameters
        m_WorldRenderView = MakeRef<WorldRenderView>();
        m_WorldRenderView->bDrawDebug = true;

        // Create UI desktop
        UIDesktop* desktop = UINew(UIDesktop);

        // Add viewport to desktop
        UIViewport* viewport;
        desktop->AddWidget(UINewAssign(viewport, UIViewport)
            .SetWorldRenderView(m_WorldRenderView));

        desktop->SetFullscreenWidget(viewport);
        desktop->SetFocusWidget(viewport);

        // Hide mouse cursor
        sGetUIManager().bCursorVisible = false;

        // Add desktop and set current
        sGetUIManager().AddDesktop(desktop);

        // Add shortcuts
        UIShortcutContainer* shortcuts = UINew(UIShortcutContainer);
        shortcuts->AddShortcut(VirtualKey::Escape, {}, {this, &SampleApplication::Quit});
        shortcuts->AddShortcut(VirtualKey::Pause, {}, {this, &SampleApplication::Pause});
        shortcuts->AddShortcut(VirtualKey::P, {}, {this, &SampleApplication::Pause});
        desktop->SetShortcuts(shortcuts);

        // Create game resources
        CreateResources();

        // Create game world
        m_World = CreateWorld();

        // Create camera
        m_MainCamera = CreateCamera();

        // Set camera for render view
        m_WorldRenderView->SetCamera(m_MainCamera);
        m_WorldRenderView->SetWorld(m_World);

        // Spawn player
        GameObject* player = CreatePlayer(Float3(0, 0, 0), Quat::sIdentity());

        // Bind input to the player
        InputInterface& input = m_World->GetInterface<InputInterface>();
        input.BindInput(player->GetComponentHandle<PlayerComponent>(), PlayerController::_1);
        input.SetActive(true);

        // Attach main camera to player bind point
        CameraComponent* cameraComponent = m_World->GetComponent(m_MainCamera);
        cameraComponent->GetOwner()->SetParent(player->FindChildren(StringID("CameraBindPoint")));

        RenderInterface& render = m_World->GetInterface<RenderInterface>();
        render.SetAmbient(0.1f);

        CreateScene();
    }

    void Deinitialize()
    {
        DestroyWorld(m_World);
    }

    void CreateResources()
    {
        auto& resourceMngr = sGetResourceManager();
        auto& materialMngr = sGetMaterialManager();

        materialMngr.LoadLibrary("/Root/default/materials/default.mlib");

        // List of resources used in scene
        ResourceID sceneResources[] = {
            resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"),
            resourceMngr.GetResource<MeshResource>("/Root/default/plane_xz.mesh"),
            resourceMngr.GetResource<MaterialResource>("/Root/default/materials/compiled/default.mat"),
            resourceMngr.GetResource<TextureResource>("/Root/grid8.webp")
        };

        // Load resources asynchronously
        ResourceAreaID resources = resourceMngr.CreateResourceArea(sceneResources);
        resourceMngr.LoadArea(resources);

        // Wait for the resources to load
        resourceMngr.MainThread_WaitResourceArea(resources);
    }

    GameObject* CreatePlayer(Float3 const& position, Quat const& rotation)
    {
        static MeshHandle playerMesh = sGetResourceManager().GetResource<MeshResource>("/Root/default/box.mesh");

        GameObject* player;
        Handle32<PlayerComponent> playerComponent;
        {
            GameObjectDesc desc;
            desc.Position = position;
            desc.Rotation = rotation;
            desc.IsDynamic = true;
            m_World->CreateObject(desc, player);
            playerComponent = player->CreateComponent<PlayerComponent>();
        }

        GameObject* bindPoint;
        {
            GameObjectDesc desc;
            desc.Name.FromString("CameraBindPoint");
            desc.Parent = player->GetHandle();
            desc.AbsoluteRotation = true;
            desc.IsDynamic = true;
            m_World->CreateObject(desc, bindPoint);
        }

        GameObject* model;
        {
            GameObjectDesc desc;
            desc.Parent = player->GetHandle();
            desc.Position = Float3(0, 0.5f, 0);
            desc.IsDynamic = true;
            m_World->CreateObject(desc, model);

            DynamicMeshComponent* mesh;
            model->CreateComponent(mesh);
            mesh->SetMesh(playerMesh);
            mesh->SetMaterial(sGetMaterialManager().TryGet("grid8"));
            mesh->SetLocalBoundingBox({Float3(-0.5f), Float3(0.5f)});
        }

        return player;
    }

    Handle32<CameraComponent> CreateCamera()
    {
        GameObjectDesc cameraDesc;
        cameraDesc.Position = Float3(2, 4, 2);
        cameraDesc.Rotation = Angl(-60, 45, 0).ToQuat();
        cameraDesc.IsDynamic = true;

        GameObject* camera;
        m_World->CreateObject(cameraDesc, camera);
        return camera->CreateComponent<CameraComponent>();
    }

    void CreateScene()
    {
        // Spawn directional light
        {
            GameObjectDesc desc;
            desc.IsDynamic = true;

            GameObject* object;
            m_World->CreateObject(desc, object);
            object->SetDirection({1, -1, -1});

            DirectionalLightComponent* dirlight;
            object->CreateComponent(dirlight);

            dirlight->SetIlluminance(20000.0f);
            dirlight->SetShadowMaxDistance(40);
            dirlight->SetShadowCascadeResolution(2048);
            dirlight->SetShadowCascadeOffset(0.0f);
            dirlight->SetShadowCascadeSplitLambda(0.8f);
        }

        // Spawn ground
        {
            static MeshHandle groundMesh = sGetResourceManager().GetResource<MeshResource>("/Root/default/plane_xz.mesh");

            GameObjectDesc desc;
            desc.Scale = {2, 1, 2};

            GameObject* ground;
            m_World->CreateObject(desc, ground);

            StaticMeshComponent* groundModel;
            ground->CreateComponent(groundModel);

            groundModel->SetMesh(groundMesh);
            groundModel->SetMaterial(sGetMaterialManager().TryGet("grid8"));
            groundModel->SetCastShadow(false);
            groundModel->SetLocalBoundingBox({Float3(-128,-0.1f,-128), Float3(128,0.1f,128)});
        }
    }

    void Pause()
    {
        m_World->SetPaused(!m_World->GetTick().IsPaused);
    }

    void Quit()
    {
        PostTerminateEvent();
    }
};

using ApplicationClass = SampleApplication;
#include "Common/EntryPoint.h"
