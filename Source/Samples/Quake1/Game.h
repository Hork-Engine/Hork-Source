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

#include <Engine/GameThread/Public/GameEngine.h>
#include <Engine/World/Public/Components/InputComponent.h>
#include <Engine/World/Public/Actors/PlayerController.h>
#include <Engine/World/Public/World.h>

#include "Player.h"
#include "Spectator.h"
#include "QuakeBSPActor.h"

class FMyPlayerController;

class FGameModule final : public IGameModule {
    AN_CLASS( FGameModule, IGameModule )

public:
    TRef< FRenderingParameters > RenderingParams;
    TRef< FInputMappings > InputMappings;
    FWorld * World;
    FMyPlayerController * PlayerController;
    TActorSpawnParameters< FPlayer > PlayerSpawnParameters;
    TRef< FLevel > Level;
    FPlayer * Player;
    FSpectator * Spectator;

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

    template< typename ResourceType >
    ResourceType * LoadQuakeResource( const char * _FileName );

    void CleanResources();

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

    static unsigned QuakePalette[ 256 ];
};

template< typename ResourceType >
ResourceType * FGameModule::LoadQuakeResource( const char * _FileName ) {

    /*

      Also we can improve FQuakeModel/FQuakeAudio/etc by adding InitializeDefaultObject and InitializeFromFile to it.
      Then just call GetOrCreateResource< FQuakeModel >( _FileName );

    */

    bool bMetadataMismatch;
    int hash;

    ResourceType * resource = static_cast< ResourceType * >( FindResource( ResourceType::ClassMeta(), _FileName, bMetadataMismatch, hash ) );
    if ( resource ) {
        GLogger.Printf( "Caching %s\n", _FileName );
        return resource;
    }
    resource = NewObject< ResourceType >();
    bool bFound = false;
    for ( int i = 0 ; i < MAX_PACKS ; i++ ) {
        if ( resource->LoadFromPack( &Packs[i], QuakePalette, _FileName ) ) {
            bFound = true;
            break;
        }
    }
    if ( !bFound ) {
        return nullptr;
    }
    resource->SetName( _FileName );
    RegisterResource( resource );
    return resource;
}

extern FGameModule * GGameModule;
