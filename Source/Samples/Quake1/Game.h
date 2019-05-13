/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#pragma once

#include <Engine/World/Public/InputComponent.h>
#include <Engine/World/Public/PlayerController.h>
#include <Engine/World/Public/World.h>

#include "Player.h"
#include "QuakeBSPActor.h"

class FMyPlayerController;

class FGameModule final : public IGameModule {
    AN_CLASS( FGameModule, IGameModule )

public:
    TRefHolder< FRenderingParameters > RenderingParams;
    TRefHolder< FInputMappings > InputMappings;
    FWorld * World;
    FMyPlayerController * PlayerController;
    TActorSpawnParameters< FPlayer > PlayerSpawnParameters;
    TRefHolder< FLevel > Level;

    FGameModule() {}

    //
    // Game module interface
    //

    void OnGameStart() override;
    void OnGameEnd() override;
    void OnPreGameTick( float _TimeStep ) override;
    void OnPostGameTick( float _TimeStep ) override;
    void DrawCanvas( FCanvas * _Canvas ) override;

    bool LoadQuakeMap( const char * _MapName );

    FQuakeModel * LoadQuakeModel( const char * _ModelName );
    void CleanModels();

private:
    void InitializeQuakeGame();

    void CreateWallMaterial();
    void CreateWaterMaterial();
    void CreateSkyMaterial();
    void CreateSkyboxMaterial();
    void CreateSkinMaterial();
    void SetInputMappings();

    void SpawnWorld();

    enum { MAX_PACKS = 2 };
    FQuakePack Packs[MAX_PACKS];
};

extern FGameModule * GGameModule;
