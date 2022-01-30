/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Runtime/DirectionalLightComponent.h>
#include <Runtime/PlayerController.h>
#include <Runtime/MaterialGraph.h>
#include <Runtime/WDesktop.h>
#include <Runtime/Engine.h>

#include "Spectator.h"

class AModule final : public AGameModule
{
    AN_CLASS(AModule, AGameModule)

public:
    ARenderingParameters* RenderingParams;

    AModule()
    {
        AInputMappings* inputMappings = CreateInstanceOf<AInputMappings>();
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_W}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_S}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_A}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_D}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveUp", {ID_KEYBOARD, KEY_SPACE}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveDown", {ID_KEYBOARD, KEY_C}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_MOUSE, MOUSE_AXIS_X}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnUp", {ID_MOUSE, MOUSE_AXIS_Y}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_LEFT}, -90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_RIGHT}, 90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Speed", {ID_KEYBOARD, KEY_LEFT_SHIFT}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Trace", {ID_KEYBOARD, KEY_LEFT_CONTROL}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_P}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_PAUSE}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("TakeScreenshot", {ID_KEYBOARD, KEY_F12}, 0, CONTROLLER_PLAYER_1);

        RenderingParams = CreateInstanceOf<ARenderingParameters>();
        RenderingParams->BackgroundColor      = Color4(0.0f);
        RenderingParams->bClearBackground     = true;
        RenderingParams->bWireframe           = false;
        RenderingParams->bDrawDebug           = true;

        AWorld* world = AWorld::CreateWorld();

        // Spawn specator
        ASpectator* spectator = world->SpawnActor2<ASpectator>({Float3(0, 2, 0), Quat::Identity()});

        CreateScene(world);

        // Spawn player controller
        APlayerController* playerController = world->SpawnActor2<APlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderingParameters(RenderingParams);
        playerController->SetPawn(spectator);

        WDesktop* desktop = CreateInstanceOf<WDesktop>();
        GEngine->SetDesktop(desktop);

        desktop->AddWidget(
            &WNew(WViewport)
                 .SetPlayerController(playerController)
                 .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetFocus());

        AShortcutContainer* shortcuts = CreateInstanceOf<AShortcutContainer>();
        shortcuts->AddShortcut(KEY_Y, 0, {this, &AModule::ToggleWireframe});
        shortcuts->AddShortcut(KEY_G, 0, {this, &AModule::ToggleDebugDraw});
        
        desktop->SetShortcuts(shortcuts);
    }

    void ToggleWireframe()
    {
        RenderingParams->bWireframe ^= 1;
    }

    void ToggleDebugDraw()
    {
        RenderingParams->bDrawDebug ^= 1;
    }

    void CreateScene(AWorld* world)
    {
        // Spawn directional light
        AActor*                     dirlight          = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/directionallight.def"));
        ADirectionalLightComponent* dirlightcomponent = dirlight->GetComponent<ADirectionalLightComponent>();
        if (dirlightcomponent)
        {
            dirlightcomponent->SetCastShadow(true);
            dirlightcomponent->SetDirection({-0.5f, -2, -2});
        }

        // Spawn terrain
        AActor*            terrain          = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/terrain.def"));
        ATerrainComponent* terrainComponent = terrain->GetComponent<ATerrainComponent>();
        if (terrainComponent)
            terrainComponent->SetTerrain(CreateInstanceOf<ATerrain>());

        // Spawn skybox
        STransform t;
        t.SetScale(4000);
        AActor*         skybox        = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/staticmesh.def"), t);
        AMeshComponent* meshComponent = skybox->GetComponent<AMeshComponent>();
        if (meshComponent)
        {
            static TStaticResourceFinder<AIndexedMesh>      SkyMesh(_CTS("/Default/Meshes/Skybox"));
            //static TStaticResourceFinder<AIndexedMesh>      SkyMesh(_CTS("/Default/Meshes/SkydomeHemisphere"));
            //static TStaticResourceFinder<AIndexedMesh>      SkyMesh(_CTS("/Default/Meshes/Skydome"));
            static TStaticResourceFinder<AMaterialInstance> SkyboxMaterialInst(_CTS("/Root/Skybox2/Skybox_MaterialInstance.asset"));

            meshComponent->SetMesh(SkyMesh.GetObject());
            meshComponent->SetMaterialInstance(0, SkyboxMaterialInst.GetObject());
        }        
    }
};

#include <Runtime/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "AngieEngine: Terrain",
    // Root path
    "Samples/04_Terrain",
    // Module class
    &AModule::ClassMeta()};

AN_ENTRY_DECL(ModuleDecl)

//
// Declare meta
//

AN_CLASS_META(AModule)
