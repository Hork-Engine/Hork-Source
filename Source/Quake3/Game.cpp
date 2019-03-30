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
#include "MyWorld.h"
#include "MyPlayerController.h"
#include <Engine/Runtime/Public/EntryDecl.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/MaterialAssembly.h>

AN_CLASS_META_NO_ATTRIBS( FGameModule )

FGameModule * GGameModule = nullptr;

AN_ENTRY_DECL( FGameModule )

void FGameModule::OnGameStart() {
    GGameModule = this;

    // Setup game master public attributes
    GGameMaster.bQuitOnEscape = true;
    GGameMaster.bToggleFullscreenAltEnter = true;
    GGameMaster.MouseSensitivity = 0.15f;

    GGameMaster.SetWindowDefs( 1.0f, true, false, false, "AngieEngine: Quake map sample" );
    GGameMaster.SetVideoMode( 640, 480, 0, 60, false, "OpenGL 4.5" );
    //GGameMaster.SetVideoMode( 1920, 1080, 0, 60, false, "OpenGL 4.5" );
    //GGameMaster.SetVideoMode( 1024, 768, 0, 60, false, "OpenGL 4.5" );
    GGameMaster.SetCursorEnabled( false );

    InitializeQuakeGame();

    CreateWallMaterial();
    CreateWallVertexLightMaterial();
    CreateWaterMaterial();
    CreateSkyMaterial();
    CreateSkyboxMaterial();

    SetInputMappings();

    SpawnWorld();

    //LoadQuakeMap( "maps/q3ctf3.bsp" );
    //LoadQuakeMap( "maps/q3dm5.bsp" );
    //LoadQuakeMap( "maps/q3ctf3.bsp" );
    //LoadQuakeMap( "maps/q3dm14.bsp" );
    LoadQuakeMap( "maps/q3tourney3.bsp" );
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
}

void FGameModule::SpawnWorld() {

    // Spawn world
    TWorldSpawnParameters< FMyWorld > WorldSpawnParameters;
    World = GGameMaster.SpawnWorld< FMyWorld >( WorldSpawnParameters );

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

bool FGameModule::LoadQuakeMap( const char * _MapName ) {

    FArchive pack;

    byte * buffer;
    int size;

    if ( !pack.Open( "pak0.pk3" ) ) {
        return false;
    }

    if ( !pack.ReadFile( _MapName, &buffer, &size ) ) {
        return false;
    }

    FQuakeBSP * model = NewObject< FQuakeBSP >();
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
    FQuakeBSPActor * BSPActor = World->SpawnActor< FQuakeBSPActor >( Level );
    BSPActor->SetModel( model );

    // Setup player controller
    PlayerController->SetPawn( player );
    PlayerController->SetViewCamera( player->Camera );
    PlayerController->AddViewActor( BSPActor );


//    World->SpawnActor< FGroundActor >( Float3( 0,0,0 ), Quat::Identity(), Level );
//    World->SpawnActor< FSkyboxActor >( Level );

    return true;
}
#if 0
void FGameModule::CreateWaterMaterial() {
    const char * vertexSourceCode = AN_STRINGIFY(

            out gl_PerVertex
            {
                vec4 gl_Position;
//                float gl_PointSize;
//                float gl_ClipDistance[];
            };

            layout( location = 0 ) out vec2 VS_TexCoord;

    layout( binding = 0, std140 ) uniform UniformBuffer0
    {
        vec4 Timers;
        vec4 ViewPostion;
    };
    layout( binding = 1, std140 ) uniform UniformBuffer1
    {
        mat4 ProjectTranslateViewMatrix;
        vec4 LightmapOffset;
    };

            void main() {
                gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
                VS_TexCoord = InTexCoord;
            }

            );

    const char * fragmentSourceCode = AN_STRINGIFY(

            layout( location = 0 ) in vec2 VS_TexCoord;
            layout( location = 0 ) out vec4 FS_FragColor;

            layout( binding = 0 ) uniform sampler2D colorTex;

            layout( binding = 0, std140 ) uniform UniformBuffer0
            {
                vec4 Timers;
                vec4 ViewPostion;
            };
            layout( binding = 1, std140 ) uniform UniformBuffer1
            {
                mat4 ProjectTranslateViewMatrix;
                vec4 LightmapOffset;
            };

            void main() {
                vec2 tc;

                float t = Timers.y*2;

                //tc = VS_TexCoord + sin( VS_TexCoord.yx * 0.125 + t );

                tc = VS_TexCoord + sin( VS_TexCoord.yx*8 + t ) / 64.0;

                vec4 color = texture( colorTex, tc );

                FS_FragColor = pow( color, vec4(1.0/2.2) );
            }

            );


    int vertexSourceLength = strlen( vertexSourceCode )+1;
    int fragmentSourceLength = strlen( fragmentSourceCode )+1;

    int size = sizeof( FMaterialBuildData )
            - sizeof( FMaterialBuildData::ShaderData )
            + vertexSourceLength
            + fragmentSourceLength;

    FMaterialBuildData * buildData = ( FMaterialBuildData * )GMainMemoryZone.AllocCleared( size, 1 );

    buildData->Size = size;
    buildData->Type = MATERIAL_TYPE_UNLIT;

    buildData->NumSamplers = 1;

    FSamplerDesc & desc = buildData->Samplers[0];
    desc.TextureType = TEXTURE_2D;
    desc.Filter = TEXTURE_FILTER_MIPMAP_NEAREST;
    desc.AddressU = TEXTURE_SAMPLER_WRAP;
    desc.AddressV = TEXTURE_SAMPLER_WRAP;
    desc.AddressW = TEXTURE_SAMPLER_WRAP;
    desc.MipLODBias = 0;
    desc.Anisotropy = 16;
    desc.MinLod = -1000;
    desc.MaxLod = 1000;

    buildData->VertexSourceOffset = 0;
    buildData->VertexSourceLength = vertexSourceLength;

    buildData->FragmentSourceOffset = vertexSourceLength;
    buildData->FragmentSourceLength = fragmentSourceLength;

    memcpy( &buildData->ShaderData[buildData->VertexSourceOffset], vertexSourceCode, vertexSourceLength );
    memcpy( &buildData->ShaderData[buildData->FragmentSourceOffset], fragmentSourceCode, fragmentSourceLength );

    WaterMaterial = NewObject< FMaterial >();
    WaterMaterial->Initialize( buildData );

    GMainMemoryZone.Dealloc( buildData );
}
#endif
void FGameModule::CreateWaterMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialInTexCoordBlock * inTexCoordBlock = proj->NewBlock< FMaterialInTexCoordBlock >();

    FMaterialProjectionBlock * projectionBlock = proj->NewBlock< FMaterialProjectionBlock >();
    projectionBlock->Vector->Connect( inPositionBlock, "Value" );

    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( projectionBlock, "Result" );

    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );

    FAssemblyNextStageVariable * texCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    texCoord->Connect( inTexCoordBlock, "Value" );

    FMaterialTextureSlotBlock * diffuseTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

    FMaterialFloatBlock * floatConstant2 = proj->NewBlock< FMaterialFloatBlock >();
    floatConstant2->Value = 2.0f;

    FMaterialFloatBlock * floatConstant8 = proj->NewBlock< FMaterialFloatBlock >();
    floatConstant8->Value = 8.0f;

    FMaterialFloatBlock * floatConstant64 = proj->NewBlock< FMaterialFloatBlock >();
    floatConstant64->Value = 1.0f/64.0f;

    // Get timer
    FMaterialInTimerBlock * timer = proj->NewBlock< FMaterialInTimerBlock >();

    FMaterialMulBlock * scaledTime = proj->NewBlock< FMaterialMulBlock >();
    scaledTime->ValueA->Connect( timer, "GameplayTimeSeconds" );
    scaledTime->ValueB->Connect( floatConstant2, "Value" );

    FMaterialDecomposeVectorBlock * texCoordXYDecomposed = proj->NewBlock< FMaterialDecomposeVectorBlock >();
    texCoordXYDecomposed->Vector->Connect( materialVertexStage, "TexCoord" );

    FMaterialMakeVectorBlock * texCoordYX = proj->NewBlock< FMaterialMakeVectorBlock >();
    texCoordYX->X->Connect( texCoordXYDecomposed, "Y" );
    texCoordYX->Y->Connect( texCoordXYDecomposed, "X" );

    FMaterialMADBlock * sinArg = proj->NewBlock< FMaterialMADBlock >();
    sinArg->ValueA->Connect( texCoordYX, "Result" );
    sinArg->ValueB->Connect( floatConstant8, "Value" );
    sinArg->ValueC->Connect( scaledTime, "Result" );

    FMaterialSinusBlock * sinus = proj->NewBlock< FMaterialSinusBlock >();
    sinus->Value->Connect( sinArg, "Result" );

    FMaterialMADBlock * mad = proj->NewObject< FMaterialMADBlock >();
    mad->ValueA->Connect( sinus, "Result" );
    mad->ValueB->Connect( floatConstant64, "Value" );
    mad->ValueC->Connect( materialVertexStage, "TexCoord" );

    FMaterialSamplerBlock * diffuseSampler = proj->NewBlock< FMaterialSamplerBlock >();
    diffuseSampler->TexCoord->Connect( mad, "Result" );
    diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( diffuseSampler, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( diffuseTexture );
    WaterMaterial = builder->Build();
}

#if 0
void FGameModule::CreateSkyMaterial() {
    const char * vertexSourceCode = AN_STRINGIFY(

            out gl_PerVertex
            {
                vec4 gl_Position;
//                float gl_PointSize;
//                float gl_ClipDistance[];
            };

            layout( location = 0 ) out vec2 VS_TexCoord;
            layout( location = 1 ) out vec3 VS_Dir;

            layout( binding = 0, std140 ) uniform UniformBuffer0
            {
                vec4 Timers;
                vec4 ViewPostion;
            };
            layout( binding = 1, std140 ) uniform UniformBuffer1
            {
                mat4 ProjectTranslateViewMatrix;
                vec4 LightmapOffset;
            };

            void main() {
                gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
                VS_TexCoord = InTexCoord;

                VS_Dir = InPosition - ViewPostion.xyz;
            }

            );

    const char * fragmentSourceCode = AN_STRINGIFY(

            layout( location = 0 ) in vec2 VS_TexCoord;
            layout( location = 1 ) in vec3 VS_Dir;

            layout( location = 0 ) out vec4 FS_FragColor;

            layout( binding = 0 ) uniform sampler2DArray colorTex;

            layout( binding = 0, std140 ) uniform UniformBuffer0
            {
                vec4 Timers;
                vec4 ViewPostion;
            };
            layout( binding = 1, std140 ) uniform UniformBuffer1
            {
                mat4 ProjectTranslateViewMatrix;
                vec4 LightmapOffset;
            };

            void main() {
                vec4 color;

                vec3 dir = VS_Dir;

                dir.y *= 3;
                dir.xz /= length(dir);
                dir.x = -dir.x;

                const float speed1 = 0.4;
                const float speed2 = 0.2;

                vec2 tc1 = dir.xz + vec2( Timers.y * speed1 );
                vec2 tc2 = dir.xz + vec2( Timers.y * speed2 );

                color  = texture( colorTex, vec3(tc1,0) );
                color += texture( colorTex, vec3(tc2,1) );

                FS_FragColor = pow( color, vec4(1.0/2.2) );
            }

            );


    int vertexSourceLength = strlen( vertexSourceCode )+1;
    int fragmentSourceLength = strlen( fragmentSourceCode )+1;

    int size = sizeof( FMaterialBuildData )
            - sizeof( FMaterialBuildData::ShaderData )
            + vertexSourceLength
            + fragmentSourceLength;

    FMaterialBuildData * buildData = ( FMaterialBuildData * )GMainMemoryZone.AllocCleared( size, 1 );

    buildData->Size = size;
    buildData->Type = MATERIAL_TYPE_UNLIT;

    buildData->NumSamplers = 1;

    buildData->Samplers[0].Filter = TEXTURE_FILTER_LINEAR;

    buildData->VertexSourceOffset = 0;
    buildData->VertexSourceLength = vertexSourceLength;

    buildData->FragmentSourceOffset = vertexSourceLength;
    buildData->FragmentSourceLength = fragmentSourceLength;

    memcpy( &buildData->ShaderData[buildData->VertexSourceOffset], vertexSourceCode, vertexSourceLength );
    memcpy( &buildData->ShaderData[buildData->FragmentSourceOffset], fragmentSourceCode, fragmentSourceLength );

    SkyMaterial = NewObject< FMaterial >();
    SkyMaterial->Initialize( buildData );

    GMainMemoryZone.Dealloc( buildData );
}
#endif
#if 0
void FGameModule::CreateSkinMaterial() {
    const char * vertexSourceCode = AN_STRINGIFY(

            out gl_PerVertex
            {
                vec4 gl_Position;
//                float gl_PointSize;
//                float gl_ClipDistance[];
            };

            layout( location = 0 ) out vec2 VS_TexCoord;

            layout( binding = 0, std140 ) uniform UniformBuffer0
            {
                mat4 ProjectTranslateViewMatrix;
                vec4 Timers;
             };

            void main() {
                gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
                VS_TexCoord = InTexCoord;
            }

            );

    const char * fragmentSourceCode = AN_STRINGIFY(

            layout( location = 0 ) in vec2 VS_TexCoord;
            layout( location = 0 ) out vec4 FS_FragColor;

            layout( binding = 0 ) uniform sampler2D colorTex;

            void main() {
                vec4 color = texture( colorTex, VS_TexCoord );

                FS_FragColor = pow( color, vec4(1.0/2.2) );
            }

            );


    int vertexSourceLength = strlen( vertexSourceCode )+1;
    int fragmentSourceLength = strlen( fragmentSourceCode )+1;

    int size = sizeof( FMaterialBuildData )
            - sizeof( FMaterialBuildData::ShaderData )
            + vertexSourceLength
            + fragmentSourceLength;

    FMaterialBuildData * buildData = ( FMaterialBuildData * )GMainMemoryZone.AllocCleared( size, 1 );

    buildData->Size = size;
    buildData->Type = MATERIAL_TYPE_UNLIT;

    buildData->NumSamplers = 1;

    FSamplerDesc & desc = buildData->Samplers[0];
    desc.TextureType = TEXTURE_2D;
    desc.Filter = TEXTURE_FILTER_MIPMAP_NEAREST;
    desc.AddressU = TEXTURE_SAMPLER_WRAP;
    desc.AddressV = TEXTURE_SAMPLER_WRAP;
    desc.AddressW = TEXTURE_SAMPLER_WRAP;
    desc.MipLODBias = 0;
    desc.Anisotropy = 16;
    desc.MinLod = -1000;
    desc.MaxLod = 1000;

    buildData->VertexSourceOffset = 0;
    buildData->VertexSourceLength = vertexSourceLength;

    buildData->FragmentSourceOffset = vertexSourceLength;
    buildData->FragmentSourceLength = fragmentSourceLength;

    memcpy( &buildData->ShaderData[buildData->VertexSourceOffset], vertexSourceCode, vertexSourceLength );
    memcpy( &buildData->ShaderData[buildData->FragmentSourceOffset], fragmentSourceCode, fragmentSourceLength );

    SkinMaterial = NewObject< FMaterial >();
    SkinMaterial->Initialize( buildData );

    GMainMemoryZone.Dealloc( buildData );
}

#endif

void FGameModule::CreateWallMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialInTexCoordBlock * inTexCoordBlock = proj->NewBlock< FMaterialInTexCoordBlock >();
    //FMaterialInLightmapTexCoordBlock * inLightmapTexCoordBlock = proj->NewBlock< FMaterialInLightmapTexCoordBlock >();

    FMaterialProjectionBlock * projectionBlock = proj->NewBlock< FMaterialProjectionBlock >();
    projectionBlock->Vector->Connect( inPositionBlock, "Value" );

    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( projectionBlock, "Result" );

    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    //materialVertexStage->AddNextStageVariable( "LightmapTexCoord", AT_Float2 );

    FAssemblyNextStageVariable * texCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    texCoord->Connect( inTexCoordBlock, "Value" );

    //FAssemblyNextStageVariable * lightmapTexCoord = materialVertexStage->FindNextStageVariable( "LightmapTexCoord" );
    //lightmapTexCoord->Connect( inLightmapTexCoordBlock, "Value" );

    FMaterialTextureSlotBlock * diffuseTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

    //FMaterialTextureSlotBlock * lightmapTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    //lightmapTexture->Filter = TEXTURE_FILTER_LINEAR;

    FMaterialSamplerBlock * diffuseSampler = proj->NewBlock< FMaterialSamplerBlock >();
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

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( diffuseSampler, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_LIGHTMAP;
    //builder->MaterialType = MATERIAL_TYPE_VERTEX_LIGHT;
    builder->RegisterTextureSlot( diffuseTexture );
    //builder->RegisterTextureSlot( lightmapTexture );
    WallMaterial = builder->Build();

//    FDocument document;
//    int object = proj->Serialize( document );
//    GLogger.Print( document.ObjectToString( object ).ToConstChar() );
//    GLogger.Printf( "breakpoint\n" );
}
#if 1
void FGameModule::CreateSkyMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    //
    // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
    //
    FMaterialInPositionBlock * InPosition = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialProjectionBlock * projectionBlock = proj->NewBlock< FMaterialProjectionBlock >();
    projectionBlock->Vector->Connect( InPosition, "Value" );
    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( projectionBlock, "Result" );

    //
    // VS_TexCoord = InTexCoord;
    //
    FMaterialInTexCoordBlock * InTexCoord = proj->NewBlock< FMaterialInTexCoordBlock >();
    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    FAssemblyNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    NSV_TexCoord->Connect( InTexCoord, "Value" );

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * InViewPosition = proj->NewBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->NewBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( InPosition, "Value" );
    positionMinusViewPosition->ValueB->Connect( InViewPosition, "Value" );
    materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
    FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
    NSV_Dir->Connect( positionMinusViewPosition, "Result" );

    //
    // vec3 dir = VS_Dir * vec3( 1, 3, 1 );
    //
    FMaterialFloat3Block * flattenMultiplier = proj->NewBlock< FMaterialFloat3Block >();
    flattenMultiplier->Value = Float3(1,3,1);
    FMaterialMulBlock * flattenDir = proj->NewBlock< FMaterialMulBlock >();
    flattenDir->ValueA->Connect( materialVertexStage, "Dir" );
    flattenDir->ValueB->Connect( flattenMultiplier, "Value" );

    // dir = normalize( dir )
    FMaterialNormalizeBlock * normDir = proj->NewBlock< FMaterialNormalizeBlock >();
    normDir->Value->Connect( flattenDir, "Result" );

    // dir.x = -dir.x
    FMaterialDecomposeVectorBlock * decomposeDir = proj->NewBlock< FMaterialDecomposeVectorBlock >();
    decomposeDir->Vector->Connect( normDir, "Result" );
    FMaterialNegateBlock * negateDirX = proj->NewBlock< FMaterialNegateBlock >();
    negateDirX->Value->Connect( decomposeDir, "X" );

    // vec2 tc = dir.xz
    FMaterialMakeVectorBlock * tc = proj->NewBlock< FMaterialMakeVectorBlock >();
    tc->X->Connect( negateDirX, "Result" );
    tc->Y->Connect( decomposeDir, "Z" );

    // Get timer
    FMaterialInTimerBlock * timer = proj->NewBlock< FMaterialInTimerBlock >();

    // const float speed1 = 0.4;
    FMaterialFloatBlock * speed1 = proj->NewBlock< FMaterialFloatBlock >();
    speed1->Value = 0.2f;

    // const float speed2 = 0.2;
    FMaterialFloatBlock * speed2 = proj->NewBlock< FMaterialFloatBlock >();
    speed2->Value = 0.4f;

    // t1 = Timer.y * speed1
    FMaterialMulBlock * t1 = proj->NewBlock< FMaterialMulBlock >();
    t1->ValueA->Connect( timer, "GameplayTimeSeconds" );
    t1->ValueB->Connect( speed1, "Value" );

    // t2 = Timer.y * speed2
    FMaterialMulBlock * t2 = proj->NewBlock< FMaterialMulBlock >();
    t2->ValueA->Connect( timer, "GameplayTimeSeconds" );
    t2->ValueB->Connect( speed2, "Value" );

    // vec2 tc1 = tc + t1
    FMaterialAddBlock * tc1 = proj->NewBlock< FMaterialAddBlock >();
    tc1->ValueA->Connect( tc, "Result" );
    tc1->ValueB->Connect( t1, "Result" );

    // vec2 tc2 = tc + t2
    FMaterialAddBlock * tc2 = proj->NewBlock< FMaterialAddBlock >();
    tc2->ValueA->Connect( tc, "Result" );
    tc2->ValueB->Connect( t2, "Result" );

#if 0
    // float tc1_fract_x = fract( tc1.x )
    FMaterialDecomposeVectorBlock * tc1_x = proj->NewBlock< FMaterialDecomposeVectorBlock >();
    tc1_x->Vector->Connect( tc1, "Result" );
    FMaterialFractBlock * tc1_fract_x = proj->NewBlock< FMaterialFractBlock >();
    tc1_fract_x->Value->Connect( tc1_x, "X" );

    // float tc2_fract_x = fract( tc2.x )
    FMaterialDecomposeVectorBlock * tc2_x = proj->NewBlock< FMaterialDecomposeVectorBlock >();
    tc2_x->Vector->Connect( tc2, "Result" );
    FMaterialFractBlock * tc2_fract_x = proj->NewBlock< FMaterialFractBlock >();
    tc2_fract_x->Value->Connect( tc2_x, "X" );

    // const float half = 0.5;
    FMaterialFloatBlock * half = proj->NewBlock< FMaterialFloatBlock >();
    half->Value = 0.5;

    // const float zero = 0.0;
    FMaterialFloatBlock * zero = proj->NewBlock< FMaterialFloatBlock >();
    zero->Value = 0.0;

    // float tc1_step = step( fract( tc1.x ), 0.5 )
    FMaterialStepBlock * tc1_step = proj->NewBlock< FMaterialStepBlock >();
    tc1_step->ValueA->Connect( tc1_fract_x, "Result" );
    tc1_step->ValueB->Connect( half, "Value" );

    // float tc2_step = step( 0.5, fract( tc2.x ) )
    FMaterialStepBlock * tc2_step = proj->NewBlock< FMaterialStepBlock >();
    tc2_step->ValueA->Connect( half, "Value" );
    tc2_step->ValueB->Connect( tc2_fract_x, "Result" );

    // tc1_step_half = tc1_step * 0.5
    FMaterialMulBlock * tc1_step_half = proj->NewBlock< FMaterialMulBlock >();
    tc1_step_half->ValueA->Connect( tc1_step, "Result" );
    tc1_step_half->ValueB->Connect( half, "Value" );

    // tc2_step_half = tc2_step * 0.5
    FMaterialMulBlock * tc2_step_half = proj->NewBlock< FMaterialMulBlock >();
    tc2_step_half->ValueA->Connect( tc2_step, "Result" );
    tc2_step_half->ValueB->Connect( half, "Value" );

    // vec1 = vec2( tc1_step_half, 0 )
    FMaterialMakeVectorBlock * vec1 = proj->NewBlock< FMaterialMakeVectorBlock >();
    vec1->X->Connect( tc1_step_half, "Result" );
    vec1->Y->Connect( zero, "Value" );

    // vec2 = vec2( tc2_step_half, 0 )
    FMaterialMakeVectorBlock * vec2 = proj->NewBlock< FMaterialMakeVectorBlock >();
    vec2->X->Connect( tc2_step_half, "Result" );
    vec2->Y->Connect( zero, "Value" );

    // result_tc1 = tc1 + vec1
    FMaterialAddBlock * result_tc1 = proj->NewBlock< FMaterialAddBlock >();
    result_tc1->ValueA->Connect( tc1, "Result" );
    result_tc1->ValueB->Connect( vec1, "Result" );

    // result_tc2 = tc2 + vec2
    FMaterialAddBlock * result_tc2 = proj->NewBlock< FMaterialAddBlock >();
    result_tc2->ValueA->Connect( tc2, "Result" );
    result_tc2->ValueB->Connect( vec2, "Result" );

    FMaterialTextureSlotBlock * skyTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    skyTexture->Filter = TEXTURE_FILTER_LINEAR;

    // color1 = texture( colorTex, tc1 );
    FMaterialSamplerBlock * color1 = proj->NewBlock< FMaterialSamplerBlock >();
    color1->TexCoord->Connect( result_tc1, "Result" );
    color1->TextureSlot->Connect( skyTexture, "Value" );

    // color2 = texture( colorTex, tc2 );
    FMaterialSamplerBlock * color2 = proj->NewBlock< FMaterialSamplerBlock >();
    color2->TexCoord->Connect( result_tc2, "Result" );
    color2->TextureSlot->Connect( skyTexture, "Value" );

    // resultColor = color1 + color2
    FMaterialAddBlock * resultColor = proj->NewBlock< FMaterialAddBlock >();
    resultColor->ValueA->Connect( color1, "RGBA" );
    resultColor->ValueB->Connect( color2, "RGBA" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( resultColor, "Result" );
#endif

    FMaterialTextureSlotBlock * skyTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    skyTexture->Filter = TEXTURE_FILTER_LINEAR;
    skyTexture->TextureType = TEXTURE_2D_ARRAY;

    // const float zero = 0.0;
    // const float one = 1.0;
    FMaterialFloatBlock * zero = proj->NewBlock< FMaterialFloatBlock >();
    zero->Value = 0;
    FMaterialFloatBlock * one = proj->NewBlock< FMaterialFloatBlock >();
    one->Value = 1;

    FMaterialDecomposeVectorBlock * tc1_decompose = proj->NewBlock< FMaterialDecomposeVectorBlock >();
    tc1_decompose->Vector->Connect( tc1, "Result" );
    FMaterialDecomposeVectorBlock * tc2_decompose = proj->NewBlock< FMaterialDecomposeVectorBlock >();
    tc2_decompose->Vector->Connect( tc2, "Result" );

    FMaterialMakeVectorBlock * tc_0 = proj->NewBlock< FMaterialMakeVectorBlock >();
    tc_0->X->Connect( tc1_decompose, "X" );
    tc_0->Y->Connect( tc1_decompose, "Y" );
    tc_0->Z->Connect( zero, "Value" );

    FMaterialMakeVectorBlock * tc_1 = proj->NewBlock< FMaterialMakeVectorBlock >();
    tc_1->X->Connect( tc2_decompose, "X" );
    tc_1->Y->Connect( tc2_decompose, "Y" );
    tc_1->Z->Connect( one, "Value" );

    // color1 = texture( colorTex, tc_0 );
    FMaterialSamplerBlock * color1 = proj->NewBlock< FMaterialSamplerBlock >();
    color1->TexCoord->Connect( tc_0, "Result" );
    color1->TextureSlot->Connect( skyTexture, "Value" );

    // color2 = texture( colorTex, tc_1 );
    FMaterialSamplerBlock * color2 = proj->NewBlock< FMaterialSamplerBlock >();
    color2->TexCoord->Connect( tc_1, "Result" );
    color2->TextureSlot->Connect( skyTexture, "Value" );

    // resultColor = color1 + color2
    FMaterialAddBlock * resultColor = proj->NewBlock< FMaterialAddBlock >();
    resultColor->ValueA->Connect( color1, "RGBA" );
    resultColor->ValueB->Connect( color2, "RGBA" );

    // resultColor = lerp( color1, color2, color2.a )
    //FMaterialLerpBlock * resultColor = proj->NewBlock< FMaterialLerpBlock >();
    //resultColor->ValueA->Connect( color1, "RGBA" );
    //resultColor->ValueB->Connect( color2, "RGBA" );
    //resultColor->ValueC->Connect( color2, "A" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( resultColor, "Result" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( skyTexture );
    SkyMaterial = builder->Build();

//    FDocument document;
//    int object = proj->Serialize( document );
//    GLogger.Print( document.ObjectToString( object ).ToConstChar() );
    GLogger.Printf( "breakpoint\n" );
}
#endif
void FGameModule::CreateSkyboxMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    //
    // gl_Position = ProjectTranslateViewMatrix * vec4( InPosition, 1.0 );
    //
    FMaterialInPositionBlock * InPosition = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialProjectionBlock * projectionBlock = proj->NewBlock< FMaterialProjectionBlock >();
    projectionBlock->Vector->Connect( InPosition, "Value" );
    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( projectionBlock, "Result" );

    //
    // VS_TexCoord = InTexCoord;
    //
    FMaterialInTexCoordBlock * InTexCoord = proj->NewBlock< FMaterialInTexCoordBlock >();
    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    FAssemblyNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    NSV_TexCoord->Connect( InTexCoord, "Value" );

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * InViewPosition = proj->NewBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->NewBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( InPosition, "Value" );
    positionMinusViewPosition->ValueB->Connect( InViewPosition, "Value" );
    materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
    FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
    NSV_Dir->Connect( positionMinusViewPosition, "Result" );

    // normDir = normalize( VS_Dir )
    FMaterialNormalizeBlock * normDir = proj->NewBlock< FMaterialNormalizeBlock >();
    normDir->Value->Connect( materialVertexStage, "Dir" );

    FMaterialTextureSlotBlock * skyTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    skyTexture->Filter = TEXTURE_FILTER_LINEAR;
    skyTexture->TextureType = TEXTURE_CUBEMAP;

    // color = texture( skyTexture, normDir );
    FMaterialSamplerBlock * color = proj->NewBlock< FMaterialSamplerBlock >();
    color->TexCoord->Connect( normDir, "Result" );
    color->TextureSlot->Connect( skyTexture, "Value" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( color, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( skyTexture );
    SkyboxMaterial = builder->Build();

    GLogger.Printf( "breakpoint\n" );
}

void FGameModule::CreateWallVertexLightMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
    FMaterialInTexCoordBlock * inTexCoordBlock = proj->NewBlock< FMaterialInTexCoordBlock >();

    FMaterialProjectionBlock * projectionBlock = proj->NewBlock< FMaterialProjectionBlock >();
    projectionBlock->Vector->Connect( inPositionBlock, "Value" );

    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( projectionBlock, "Result" );

    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );

    FAssemblyNextStageVariable * texCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    texCoord->Connect( inTexCoordBlock, "Value" );

    FMaterialTextureSlotBlock * diffuseTexture = proj->NewBlock< FMaterialTextureSlotBlock >();
    diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

    FMaterialSamplerBlock * diffuseSampler = proj->NewBlock< FMaterialSamplerBlock >();
    diffuseSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
    diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

    FMaterialFragmentStage * materialFragmentStage = proj->NewBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( diffuseSampler, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_VERTEX_LIGHT;
    builder->RegisterTextureSlot( diffuseTexture );
    WallVertexLightMaterial = builder->Build();
}

//static FMaterial * CreateHudMaterial( FBaseObject * _Parent ) {
//    FMaterialProject * proj = _Parent->NewObject< FMaterialProject >();

//    FMaterialInPositionBlock * inPositionBlock = proj->NewBlock< FMaterialInPositionBlock >();
//    FMaterialInTexCoordBlock * inTexCoordBlock = proj->NewBlock< FMaterialInTexCoordBlock >();
//    FMaterialInColorBlock * inColorBlock = proj->NewBlock< FMaterialInColorBlock >();

//    FMaterialProjectionBlock * projectionBlock = proj->NewBlock< FMaterialProjectionBlock >();
//    projectionBlock->Vector->Connect( inPositionBlock, "Value" );

//    FMaterialVertexStage * materialVertexStage = proj->NewBlock< FMaterialVertexStage >();
//    materialVertexStage->Position->Connect( projectionBlock, "Result" );

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
