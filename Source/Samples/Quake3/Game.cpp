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

#include "Game.h"
#include "MyPlayerController.h"
#include <Engine/Runtime/Public/EntryDecl.h>
#include <Engine/GameThread/Public/GameEngine.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/Resource/Public/MaterialAssembly.h>

AN_CLASS_META( FGameModule )

FGameModule * GGameModule = nullptr;

AN_ENTRY_DECL( FGameModule )

void FGameModule::OnGameStart() {
    GGameModule = this;

    // Setup game master public attributes
    GGameEngine.bQuitOnEscape = true;
    GGameEngine.bToggleFullscreenAltEnter = true;
    GGameEngine.MouseSensitivity = 0.15f;

    GGameEngine.SetWindowDefs( 1.0f, true, false, false, "AngieEngine: Quake map sample" );
    GGameEngine.SetVideoMode( 640, 480, 0, 60, false, "OpenGL 4.5" );
    //GGameEngine.SetVideoMode( 1920, 1080, 0, 60, false, "OpenGL 4.5" );
    //GGameEngine.SetVideoMode( 1024, 768, 0, 60, false, "OpenGL 4.5" );
    GGameEngine.SetCursorEnabled( false );

    InitializeQuakeGame();

    CreateWallMaterial();
    CreateWaterMaterial();
    CreateSkyMaterial();
    CreateSkyboxMaterial();

    SetInputMappings();

    SpawnWorld();

    //LoadQuakeMap( "pak0.pk3", "maps/q3ctf3.bsp" );
    //LoadQuakeMap( "pak0.pk3", "maps/q3dm5.bsp" );
    //LoadQuakeMap( "pak0.pk3", "maps/q3ctf2.bsp" );
    //LoadQuakeMap( "pak0.pk3", "maps/q3dm14.bsp" );
    //LoadQuakeMap( "pak0.pk3", "maps/q3dm12.bsp" );
    //LoadQuakeMap( "pak0.pk3", "maps/q3tourney3.bsp" );
    LoadQuakeMap( "pak0.pk3", "maps/q3dm1.bsp" );
    //LoadQuakeMap( "pak0.pk3", "maps/q3dm6.bsp" );
    //LoadQuakeMap( "E:/Games/Quake III Arena/missionpack/pak0.pk3", "maps/mpterra2.bsp" );
    //LoadQuakeMap( "E:/Games/Quake III Arena/missionpack/pak0.pk3", "maps/mpterra1.bsp" );
    //LoadQuakeMap( "E:/Games/Quake III Arena/baseq3/pak6.pk3", "maps/pro-q3tourney2.bsp" );
    //LoadQuakeMap( "E:/Program Files (x86)/Steam/steamapps/common/Quake Live/baseq3/pak00.pk3", "maps/overkill.bsp" );
    //LoadQuakeMap( "E:/Program Files (x86)/Steam/steamapps/common/Quake Live/baseq3/pak00.pk3", "maps/elder.bsp" );
    //LoadQuakeMap( "E:/Program Files (x86)/Steam/steamapps/common/Quake Live/baseq3/pak00.pk3", "maps/asylum.bsp" );
    //LoadQuakeMap( "E:/Program Files (x86)/Steam/steamapps/common/Quake Live/baseq3/pak00.pk3", "maps/gospelcrossings.bsp" );
    //LoadQuakeMap( "E:/Program Files (x86)/Steam/steamapps/common/Quake Live/baseq3/pak00.pk3", "maps/aerowalk.bsp" );
    //LoadQuakeMap( "E:/Program Files (x86)/Steam/steamapps/common/Quake Live/baseq3/pak00.pk3", "maps/warehouse.bsp" );
}

void FGameModule::OnGameEnd() {

}

void FGameModule::InitializeQuakeGame() {

    Level = NewObject< FLevel >();

    // Create rendering parameters
    RenderingParams = NewObject< FRenderingParameters >();
    RenderingParams->BackgroundColor = Float3(1,0,0);
}

void FGameModule::OnPreGameTick( float _TimeStep ) {
}

void FGameModule::OnPostGameTick( float _TimeStep ) {
}

void FGameModule::SetInputMappings() {
    InputMappings = NewObject< FInputMappings >();

    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_W, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveForward", ID_KEYBOARD, KEY_S, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_A, -1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveRight", ID_KEYBOARD, KEY_D, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveUp", ID_KEYBOARD, KEY_SPACE, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "MoveDown", ID_KEYBOARD, KEY_C, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnRight", ID_MOUSE, MOUSE_AXIS_X, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAxis( "TurnUp", ID_MOUSE, MOUSE_AXIS_Y, 1.0f, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Speed", ID_KEYBOARD, KEY_LEFT_SHIFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F12, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
}

void FGameModule::SpawnWorld() {

    // Spawn world
    TWorldSpawnParameters< FWorld > WorldSpawnParameters;
    World = GGameEngine.SpawnWorld< FWorld >( WorldSpawnParameters );

    // Spawn player controller
    PlayerController = World->SpawnActor< FMyPlayerController >();
    PlayerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
    PlayerController->SetInputMappings( InputMappings );
    PlayerController->SetRenderingParameters( RenderingParams );
}

//    float step = 2.0f;
//    float d = -NumModels * step * 0.5f;
//    for ( int i = 0 ; i < NumModels ; i++ ) {
//        SpawnParameters.SpawnTransform.Position = Float3( d + step * i, 0.75f, FMath::Rand2()*5.0f );
//        float angle = FMath::Rand2() * FMath::_HALF_PI * 0.8f;
//        SpawnParameters.SpawnTransform.Rotation.FromAngles( 0, angle, 0 );
//        FQuakeModel * model = NewObject< FQuakeModel >();
//        model->LoadFromPack( &pack0, QuakePalette, ModelNames[i] );
//        FQuakeMonster * monster = World->SpawnActor< FQuakeMonster >( SpawnParameters );
//        monster->SetModel( model );
//    }

bool FGameModule::LoadQuakeMap( const char * _PackName, const char * _MapName ) {

    FArchive pack;

    byte * buffer;
    int size;

    if ( !pack.Open( _PackName ) ) {
        return false;
    }

    if ( !pack.ReadFileToZoneMemory( _MapName, &buffer, &size ) ) {
        return false;
    }

    FQuakeBSP * model = NewObject< FQuakeBSP >();
    model->PackName = _PackName; // for texture loading
    model->FromData( Level, buffer );

    GMainMemoryZone.Dealloc( buffer );

    Level->DestroyActors();

//    // Spawn map monsters
//    for ( QEntity & ent : model->Entities ) {

//        for ( int i = 0 ; i < NumClasses ; i++ ) {
//            if ( !FString::Icmp( ent.ClassName, MonsterClassName[i][0] ) ) {

//                FClassMeta const * meta = FActor::Factory().LookupClass( MonsterClassName[i][1] );
//                FActorSpawnParameters spawnParameters( meta );

//                spawnParameters.SpawnTransform.Position = ent.Origin;
//                spawnParameters.SpawnTransform.Rotation.FromAngles( 0, FMath::Radians( ent.Angle ), 0 );
//                spawnParameters.Level = Level;

//                World->SpawnActor( spawnParameters );
//                break;
//            }
//        }

//        if ( !FString::Icmp( ent.ClassName, "info_player_start" ) ) {
//            PlayerSpawnParameters.SpawnTransform.Position = ent.Origin + Float3(0,27.0f/32.0f,0);
//            PlayerSpawnParameters.SpawnTransform.Rotation.FromAngles( 0, FMath::Radians( ent.Angle + 180 ), 0 );
//        }
//    }

    PlayerSpawnParameters.SpawnTransform.Clear();

    // Spawn player
    PlayerSpawnParameters.Level = Level;
    FPlayer * player = World->SpawnActor< FPlayer >( PlayerSpawnParameters );

    // Spawn bsp actor
    FQuakeBSPView * BSPActor = World->SpawnActor< FQuakeBSPView >( Level );
    BSPActor->SetModel( model );

    // Setup player controller
    PlayerController->SetPawn( player );
    PlayerController->SetViewCamera( player->Camera );
    PlayerController->AddViewActor( BSPActor );

    return true;
}

void FGameModule::CreateWaterMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();
    FMaterialInTexCoordBlock * inTexCoordBlock = proj->AddBlock< FMaterialInTexCoordBlock >();

    FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( inPositionBlock, "Value" );

    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );

    FAssemblyNextStageVariable * texCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    texCoord->Connect( inTexCoordBlock, "Value" );

    FMaterialTextureSlotBlock * diffuseTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
    diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

    FMaterialFloatBlock * floatConstant2 = proj->AddBlock< FMaterialFloatBlock >();
    floatConstant2->Value = 2.0f;

    FMaterialFloatBlock * floatConstant8 = proj->AddBlock< FMaterialFloatBlock >();
    floatConstant8->Value = 8.0f;

    FMaterialFloatBlock * floatConstant64 = proj->AddBlock< FMaterialFloatBlock >();
    floatConstant64->Value = 1.0f/64.0f;

    // Get timer
    FMaterialInTimerBlock * timer = proj->AddBlock< FMaterialInTimerBlock >();

    FMaterialMulBlock * scaledTime = proj->AddBlock< FMaterialMulBlock >();
    scaledTime->ValueA->Connect( timer, "GameplayTimeSeconds" );
    scaledTime->ValueB->Connect( floatConstant2, "Value" );

    FMaterialDecomposeVectorBlock * texCoordXYDecomposed = proj->AddBlock< FMaterialDecomposeVectorBlock >();
    texCoordXYDecomposed->Vector->Connect( materialVertexStage, "TexCoord" );

    FMaterialMakeVectorBlock * texCoordYX = proj->AddBlock< FMaterialMakeVectorBlock >();
    texCoordYX->X->Connect( texCoordXYDecomposed, "Y" );
    texCoordYX->Y->Connect( texCoordXYDecomposed, "X" );

    FMaterialMADBlock * sinArg = proj->AddBlock< FMaterialMADBlock >();
    sinArg->ValueA->Connect( texCoordYX, "Result" );
    sinArg->ValueB->Connect( floatConstant8, "Value" );
    sinArg->ValueC->Connect( scaledTime, "Result" );

    FMaterialSinusBlock * sinus = proj->AddBlock< FMaterialSinusBlock >();
    sinus->Value->Connect( sinArg, "Result" );

    FMaterialMADBlock * mad = proj->NewObject< FMaterialMADBlock >();
    mad->ValueA->Connect( sinus, "Result" );
    mad->ValueB->Connect( floatConstant64, "Value" );
    mad->ValueC->Connect( materialVertexStage, "TexCoord" );

    FMaterialSamplerBlock * diffuseSampler = proj->AddBlock< FMaterialSamplerBlock >();
    diffuseSampler->TexCoord->Connect( mad, "Result" );
    diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

    FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( diffuseSampler, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( diffuseTexture );
    WaterMaterial = builder->Build();
}

void FGameModule::CreateWallMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();
    FMaterialInTexCoordBlock * inTexCoordBlock = proj->AddBlock< FMaterialInTexCoordBlock >();
    //FMaterialInLightmapTexCoordBlock * inLightmapTexCoordBlock = proj->NewBlock< FMaterialInLightmapTexCoordBlock >();

    FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( inPositionBlock, "Value" );

    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    //materialVertexStage->AddNextStageVariable( "LightmapTexCoord", AT_Float2 );

    FAssemblyNextStageVariable * texCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    texCoord->Connect( inTexCoordBlock, "Value" );

    //FAssemblyNextStageVariable * lightmapTexCoord = materialVertexStage->FindNextStageVariable( "LightmapTexCoord" );
    //lightmapTexCoord->Connect( inLightmapTexCoordBlock, "Value" );

    FMaterialTextureSlotBlock * diffuseTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
    diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

    //FMaterialTextureSlotBlock * lightmapTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    //lightmapTexture->Filter = TEXTURE_FILTER_LINEAR;

    FMaterialSamplerBlock * diffuseSampler = proj->AddBlock< FMaterialSamplerBlock >();
    diffuseSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
    diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

    //FMaterialSamplerBlock * lightmapSampler = proj->NewBlock< FMaterialSamplerBlock >();
    //lightmapSampler->TexCoord->Connect( materialVertexStage, "LightmapTexCoord" );
    //lightmapSampler->TextureSlot->Connect( lightmapTexture, "Value" );

    //FMaterialInFragmentCoordBlock * inFragmentCoord = proj->NewBlock< FMaterialInFragmentCoordBlock >();

    //FMaterialFloatBlock * constant = proj->NewBlock< FMaterialFloatBlock >();
    //constant->Value =9999;// 320;

    //FMaterialFloatBlock * powConstant = proj->NewBlock< FMaterialFloatBlock >();
    //powConstant->Value = 2.2f;

    //FMaterialPowBlock * powBlock = proj->NewBlock< FMaterialPowBlock >();
    //powBlock->ValueA->Connect( lightmapSampler, "R" );
    //powBlock->ValueB->Connect( powConstant, "Value" );

    //FMaterialCondLessBlock * condLess = proj->NewBlock< FMaterialCondLessBlock >();
    //condLess->ValueA->Connect( inFragmentCoord, "X" );
    //condLess->ValueB->Connect( constant, "Value" );
    //condLess->True->Connect( powBlock, "Result" );
    //condLess->False->Connect( lightmapSampler, "R" );

    //FMaterialMulBlock * lightmapMul = proj->NewBlock< FMaterialMulBlock >();
    //lightmapMul->ValueA->Connect( diffuseSampler, "RGBA" );
    //lightmapMul->ValueB->Connect( condLess, "Result" );

    FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( diffuseSampler, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_PBR;
    builder->RegisterTextureSlot( diffuseTexture );
    WallMaterial = builder->Build();
}

void FGameModule::CreateSkyMaterial() {
#if 0
    FMaterialProject * proj = NewObject< FMaterialProject >();

    //
    // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
    //
    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * inViewPosition = proj->NewBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->NewBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
    positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
    materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
    FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
    NSV_Dir->Connect( positionMinusViewPosition, "Result" );

    FMaterialAtmosphereBlock * atmo = proj->NewBlock< FMaterialAtmosphereBlock >();
    atmo->Dir->Connect( materialVertexStage, "Dir" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( atmo, "Result" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    SkyMaterial = builder->Build();
#else
    FMaterialProject * proj = NewObject< FMaterialProject >();

    //
    // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
    //
    FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();
    FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( inPositionBlock, "Value" );

    //
    // VS_TexCoord = InTexCoord;
    //
    FMaterialInTexCoordBlock * InTexCoord = proj->AddBlock< FMaterialInTexCoordBlock >();
    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    FAssemblyNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    NSV_TexCoord->Connect( InTexCoord, "Value" );

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * InViewPosition = proj->AddBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->AddBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
    positionMinusViewPosition->ValueB->Connect( InViewPosition, "Value" );
    materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
    FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
    NSV_Dir->Connect( positionMinusViewPosition, "Result" );

    //
    // vec3 dir = VS_Dir * vec3( 1, 3, 1 );
    //
    FMaterialFloat3Block * flattenMultiplier = proj->AddBlock< FMaterialFloat3Block >();
    flattenMultiplier->Value = Float3(1,3,1);
    FMaterialMulBlock * flattenDir = proj->AddBlock< FMaterialMulBlock >();
    flattenDir->ValueA->Connect( materialVertexStage, "Dir" );
    flattenDir->ValueB->Connect( flattenMultiplier, "Value" );

    // dir = normalize( dir )
    FMaterialNormalizeBlock * normDir = proj->AddBlock< FMaterialNormalizeBlock >();
    normDir->Value->Connect( flattenDir, "Result" );

    // dir.x = -dir.x
    FMaterialDecomposeVectorBlock * decomposeDir = proj->AddBlock< FMaterialDecomposeVectorBlock >();
    decomposeDir->Vector->Connect( normDir, "Result" );
    FMaterialNegateBlock * negateDirX = proj->AddBlock< FMaterialNegateBlock >();
    negateDirX->Value->Connect( decomposeDir, "X" );

    // vec2 tc = dir.xz
    FMaterialMakeVectorBlock * tc = proj->AddBlock< FMaterialMakeVectorBlock >();
    tc->X->Connect( negateDirX, "Result" );
    tc->Y->Connect( decomposeDir, "Z" );

    // Get timer
    FMaterialInTimerBlock * timer = proj->AddBlock< FMaterialInTimerBlock >();

    // const float speed1 = 0.4;
    FMaterialFloatBlock * speed1 = proj->AddBlock< FMaterialFloatBlock >();
    speed1->Value = 0.2f;

    // const float speed2 = 0.2;
    FMaterialFloatBlock * speed2 = proj->AddBlock< FMaterialFloatBlock >();
    speed2->Value = 0.4f;

    // t1 = Timer.y * speed1
    FMaterialMulBlock * t1 = proj->AddBlock< FMaterialMulBlock >();
    t1->ValueA->Connect( timer, "GameplayTimeSeconds" );
    t1->ValueB->Connect( speed1, "Value" );

    // t2 = Timer.y * speed2
    FMaterialMulBlock * t2 = proj->AddBlock< FMaterialMulBlock >();
    t2->ValueA->Connect( timer, "GameplayTimeSeconds" );
    t2->ValueB->Connect( speed2, "Value" );

    // vec2 tc1 = tc + t1
    FMaterialAddBlock * tc1 = proj->AddBlock< FMaterialAddBlock >();
    tc1->ValueA->Connect( tc, "Result" );
    tc1->ValueB->Connect( t1, "Result" );

    // vec2 tc2 = tc + t2
    FMaterialAddBlock * tc2 = proj->AddBlock< FMaterialAddBlock >();
    tc2->ValueA->Connect( tc, "Result" );
    tc2->ValueB->Connect( t2, "Result" );

    FMaterialTextureSlotBlock * skyTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
    skyTexture->Filter = TEXTURE_FILTER_LINEAR;
    skyTexture->TextureType = TEXTURE_2D_ARRAY;

    // const float zero = 0.0;
    // const float one = 1.0;
    FMaterialFloatBlock * zero = proj->AddBlock< FMaterialFloatBlock >();
    zero->Value = 0;
    FMaterialFloatBlock * one = proj->AddBlock< FMaterialFloatBlock >();
    one->Value = 1;

    FMaterialDecomposeVectorBlock * tc1_decompose = proj->AddBlock< FMaterialDecomposeVectorBlock >();
    tc1_decompose->Vector->Connect( tc1, "Result" );
    FMaterialDecomposeVectorBlock * tc2_decompose = proj->AddBlock< FMaterialDecomposeVectorBlock >();
    tc2_decompose->Vector->Connect( tc2, "Result" );

    FMaterialMakeVectorBlock * tc_0 = proj->AddBlock< FMaterialMakeVectorBlock >();
    tc_0->X->Connect( tc1_decompose, "X" );
    tc_0->Y->Connect( tc1_decompose, "Y" );
    tc_0->Z->Connect( zero, "Value" );

    FMaterialMakeVectorBlock * tc_1 = proj->AddBlock< FMaterialMakeVectorBlock >();
    tc_1->X->Connect( tc2_decompose, "X" );
    tc_1->Y->Connect( tc2_decompose, "Y" );
    tc_1->Z->Connect( one, "Value" );

    // color1 = texture( colorTex, tc_0 );
    FMaterialSamplerBlock * color1 = proj->AddBlock< FMaterialSamplerBlock >();
    color1->TexCoord->Connect( tc_0, "Result" );
    color1->TextureSlot->Connect( skyTexture, "Value" );

    // color2 = texture( colorTex, tc_1 );
    FMaterialSamplerBlock * color2 = proj->AddBlock< FMaterialSamplerBlock >();
    color2->TexCoord->Connect( tc_1, "Result" );
    color2->TextureSlot->Connect( skyTexture, "Value" );

    // resultColor = color1 + color2
    FMaterialAddBlock * resultColor = proj->AddBlock< FMaterialAddBlock >();
    resultColor->ValueA->Connect( color1, "RGBA" );
    resultColor->ValueB->Connect( color2, "RGBA" );

    // resultColor = lerp( color1, color2, color2.a )
    //FMaterialLerpBlock * resultColor = proj->NewBlock< FMaterialLerpBlock >();
    //resultColor->ValueA->Connect( color1, "RGBA" );
    //resultColor->ValueB->Connect( color2, "RGBA" );
    //resultColor->ValueC->Connect( color2, "A" );

    FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( resultColor, "Result" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( skyTexture );
    SkyMaterial = builder->Build();

#endif
}

void FGameModule::CreateSkyboxMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    //
    // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
    //
    FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();
    FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( inPositionBlock, "Value" );

    //
    // VS_TexCoord = InTexCoord;
    //
    FMaterialInTexCoordBlock * InTexCoord = proj->AddBlock< FMaterialInTexCoordBlock >();
    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    FAssemblyNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    NSV_TexCoord->Connect( InTexCoord, "Value" );

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * InViewPosition = proj->AddBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->AddBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
    positionMinusViewPosition->ValueB->Connect( InViewPosition, "Value" );
    materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
    FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
    NSV_Dir->Connect( positionMinusViewPosition, "Result" );

    // normDir = normalize( VS_Dir )
    FMaterialNormalizeBlock * normDir = proj->AddBlock< FMaterialNormalizeBlock >();
    normDir->Value->Connect( materialVertexStage, "Dir" );

    FMaterialTextureSlotBlock * skyTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
    skyTexture->Filter = TEXTURE_FILTER_LINEAR;
    skyTexture->TextureType = TEXTURE_CUBEMAP;

    // color = texture( skyTexture, normDir );
    FMaterialSamplerBlock * color = proj->AddBlock< FMaterialSamplerBlock >();
    color->TexCoord->Connect( normDir, "Result" );
    color->TextureSlot->Connect( skyTexture, "Value" );

    FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( color, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( skyTexture );
    SkyboxMaterial = builder->Build();
}

//static FMaterial * CreateHudMaterial( FBaseObject * _Parent ) {
//    FMaterialProject * proj = _Parent->NewObject< FMaterialProject >();

//    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
//    FMaterialInTexCoordBlock * inTexCoordBlock = proj->NewBlock< FMaterialInTexCoordBlock >();
//    FMaterialInColorBlock * inColorBlock = proj->NewBlock< FMaterialInColorBlock >();

//    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
//    materialVertexStage->Position->Connect( inPositionBlock, "Value" );

//    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
//    materialVertexStage->AddNextStageVariable( "Color", AT_Float4 );

//    FAssemblyNextStageVariable * nsvTexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
//    nsvTexCoord->Connect( inTexCoordBlock, "Value" );

//    FAssemblyNextStageVariable * nsvColor = materialVertexStage->FindNextStageVariable( "Color" );
//    nsvColor->Connect( inColorBlock, "Value" );

//    FMaterialTextureSlotBlock * textureSlot = proj->NewBlock< FMaterialTextureSlotBlock >();
//    textureSlot->Filter = TEXTURE_FILTER_LINEAR;
//    textureSlot->AddressU = textureSlot->AddressV = textureSlot->AddressW = TEXTURE_SAMPLER_WRAP;

//    FMaterialSamplerBlock * textureSampler = proj->NewBlock< FMaterialSamplerBlock >();
//    textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
//    textureSampler->TextureSlot->Connect( textureSlot, "Value" );

//    FMaterialMulBlock * colorMul = proj->NewBlock< FMaterialMulBlock >();
//    colorMul->ValueA->Connect( textureSampler, "RGBA" );
//    colorMul->ValueB->Connect( materialVertexStage, "Color" );

//    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
//    materialFragmentStage->Color->Connect( colorMul, "Result" );

//    FMaterialBuilder * builder = _Parent->NewObject< FMaterialBuilder >();
//    builder->VertexStage = materialVertexStage;
//    builder->FragmentStage = materialFragmentStage;
//    builder->MaterialType = MATERIAL_TYPE_HUD;
//    builder->RegisterTextureSlot( textureSlot );
//    return builder->Build();
//}

void FGameModule::DrawCanvas( FCanvas * _Canvas ) {
    //_Canvas->DrawRectFilled( Float2(0,0), Float2(_Canvas->Width,_Canvas->Height), 0xffff00ff );

    //_Canvas->DrawViewport( PlayerController, 10, 10, _Canvas->Width/2-20, _Canvas->Height/2 - 20 );

    //_Canvas->DrawViewport( PlayerController, 10, 10 + _Canvas->Height/2, _Canvas->Width/2-20, _Canvas->Height/2 - 20 );

    //_Canvas->DrawRectFilled( Float2(30,30), Float2(60,60), 0xffffffff );

    _Canvas->DrawViewport( PlayerController, 0, 0, _Canvas->Width, _Canvas->Height );
}
