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
#include <Engine/Resource/Public/ResourceManager.h>

AN_CLASS_META( FGameModule )

FGameModule * GGameModule = nullptr;

unsigned FGameModule::QuakePalette[ 256 ];

//static ETextureFilter TextureFilter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
static ETextureFilter TextureFilter = TEXTURE_FILTER_MIPMAP_NEAREST;

AN_ENTRY_DECL( FGameModule )

void FGameModule::OnGameStart() {
    GGameModule = this;

    // Setup game master public attributes
    GGameEngine.bQuitOnEscape = true;
    GGameEngine.bToggleFullscreenAltEnter = true;
    GGameEngine.MouseSensitivity = 0.3f;//0.15f;

    GGameEngine.SetRenderFeatures( VSync_Disabled );
    //GGameEngine.SetRenderFeatures( VSync_Fixed );
    //GGameEngine.SetVideoMode( 640, 480, 0, 60, false, "OpenGL 4.5" );
    GGameEngine.SetVideoMode( 1920, 1080, 0, 60, true, "OpenGL 4.5" );
    GGameEngine.SetWindowDefs( 1.0f, true, false, false, "AngieEngine: Quake map sample" );
    GGameEngine.SetCursorEnabled( false );

    InitializeQuakeGame();

    CreateWallMaterial();
    CreateWaterMaterial();
    CreateSkyMaterial();
    CreateSkyboxMaterial();
    CreateSkinMaterial();

    // Unit sphere
    {
        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->InitializeInternalMesh( "*sphere*" );
        mesh->SetName( "UnitSphere" );
        RegisterResource( mesh );
    }

    // MipmapChecker
    {
        GetOrCreateResource< FTexture >( "mipmapchecker.png", "MipmapChecker" );
    }

    // ExplosionMaterial
    {
        FMaterialProject * proj = NewObject< FMaterialProject >();
        FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
        FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();

        FMaterialUniformAddress * uniformAddress = proj->AddBlock< FMaterialUniformAddress >();
        uniformAddress->Address = 0;
        uniformAddress->Type = AT_Float4;

        materialFragmentStage->Color->Connect( uniformAddress, "Value" );
        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_UNLIT;

        FMaterial * material = builder->Build();
        material->SetName( "ExplosionMaterial" );

        RegisterResource( material );
    }

    // Explosion material instance
    {
        FMaterialInstance * ExplosionMaterialInstance = NewObject< FMaterialInstance >();
        ExplosionMaterialInstance->Material = GetResource< FMaterial >( "ExplosionMaterial" );
        ExplosionMaterialInstance->SetName( "ExplosionMaterialInstance" );
        ExplosionMaterialInstance->UniformVectors[0] = Float4( 1,1,0.3f,1 );
        RegisterResource( ExplosionMaterialInstance );
    }

    SetInputMappings();

    SpawnWorld();

    //LoadQuakeMap( "maps/start.bsp" );
    //LoadQuakeMap( "maps/end.bsp" );

    //LoadQuakeMap( "maps/e1m1.bsp" );
    //LoadQuakeMap( "maps/e1m2.bsp" );
    //LoadQuakeMap( "maps/e1m3.bsp" );
    //LoadQuakeMap( "maps/e1m4.bsp" );
    //LoadQuakeMap( "maps/e1m5.bsp" );
    //LoadQuakeMap( "maps/e1m6.bsp" );
    //LoadQuakeMap( "maps/e1m7.bsp" );
    //LoadQuakeMap( "maps/e1m8.bsp" );

    //LoadQuakeMap( "maps/e2m1.bsp" );
    //LoadQuakeMap( "maps/e2m2.bsp" );
    //LoadQuakeMap( "maps/e2m3.bsp" );
LoadQuakeMap( "maps/e2m4.bsp" );
    //LoadQuakeMap( "maps/e2m5.bsp" );
    //LoadQuakeMap( "maps/e2m6.bsp" );
    //LoadQuakeMap( "maps/e2m7.bsp" );

    //LoadQuakeMap( "maps/e3m1.bsp" );
    //LoadQuakeMap( "maps/e3m2.bsp" );
    //LoadQuakeMap( "maps/e3m3.bsp" );
    //LoadQuakeMap( "maps/e3m4.bsp" );
    //LoadQuakeMap( "maps/e3m5.bsp" );
    //LoadQuakeMap( "maps/e3m6.bsp" );
    //LoadQuakeMap( "maps/e3m7.bsp" );

    //LoadQuakeMap( "maps/e4m1.bsp" );
    //LoadQuakeMap( "maps/e4m2.bsp" );
    //LoadQuakeMap( "maps/e4m3.bsp" );
    //LoadQuakeMap( "maps/e4m4.bsp" );
    //LoadQuakeMap( "maps/e4m5.bsp" );
    //LoadQuakeMap( "maps/e4m6.bsp" );
    //LoadQuakeMap( "maps/e4m7.bsp" );
    //LoadQuakeMap( "maps/e4m8.bsp" );

    //LoadQuakeMap( "maps/dm1.bsp" );
    //LoadQuakeMap( "maps/dm2.bsp" );
    //LoadQuakeMap( "maps/dm3.bsp" );
    //LoadQuakeMap( "maps/dm4.bsp" );
    //LoadQuakeMap( "maps/dm5.bsp" );
    //LoadQuakeMap( "maps/dm6.bsp" );
}

void FGameModule::OnGameEnd() {
    CleanResources();
}

void FGameModule::InitializeQuakeGame() {
    void FixQuakeNormals();

    FixQuakeNormals();

    for ( int i = 0 ; i < MAX_PACKS ; i++ ) {
        Packs[i].Load( FString::Fmt( "id1/PAK%d.PAK", i ) );
    }

    Packs[0].LoadPalette( QuakePalette );

    Level = NewObject< FLevel >();

    // Create rendering parameters
    RenderingParams = NewObject< FRenderingParameters >();
    //RenderingParams->BackgroundColor = Float3(1,0,0);
    RenderingParams->bDrawDebug = false;
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
    InputMappings->MapAction( "Attack", ID_MOUSE, MOUSE_BUTTON_LEFT, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_P, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "Pause", ID_KEYBOARD, KEY_PAUSE, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "TakeScreenshot", ID_KEYBOARD, KEY_F12, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleWireframe", ID_KEYBOARD, KEY_Y, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "ToggleDebugDraw", ID_KEYBOARD, KEY_G, 0, CONTROLLER_PLAYER_1 );
    InputMappings->MapAction( "SwitchToSpectator", ID_KEYBOARD, KEY_F1, 0, CONTROLLER_PLAYER_1 );
}

void FGameModule::SpawnWorld() {

    // Spawn world
    TWorldSpawnParameters< FWorld > WorldSpawnParameters;
    World = GGameEngine.SpawnWorld< FWorld >( WorldSpawnParameters );

    World->AddLevel( Level );

    // Spawn HUD
    FHUD * hud = World->SpawnActor< FMyHUD >();

    // Spawn player controller
    PlayerController = World->SpawnActor< FMyPlayerController >();
    PlayerController->SetPlayerIndex( CONTROLLER_PLAYER_1 );
    PlayerController->SetInputMappings( InputMappings );
    PlayerController->SetRenderingParameters( RenderingParams );
    PlayerController->SetHUD( hud );
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

class FStaticMesh : public FActor {
    AN_ACTOR( FStaticMesh, FActor )

public:
    void SetMesh( FIndexedMesh * _Mesh );

protected:
    FStaticMesh();

private:
    FMeshComponent * MeshComponent;
};

AN_CLASS_META( FStaticMesh )

FStaticMesh::FStaticMesh() {
    MeshComponent = AddComponent< FMeshComponent >( "StaticMesh" );
    RootComponent = MeshComponent;

    MeshComponent->PhysicsBehavior = PB_STATIC;
    MeshComponent->bUseDefaultBodyComposition = true;
}

void FStaticMesh::SetMesh( FIndexedMesh * _Mesh ) {
    MeshComponent->SetMesh( _Mesh );
    MeshComponent->UpdatePhysicsAttribs();
    MeshComponent->SetDefaultMaterials();
}

class FWorldCollisionActor : public FActor {
    AN_ACTOR( FWorldCollisionActor, FActor )

public:
    void SetModel( FQuakeBSP * _Model ) {
        FCollisionTriangleSoupData * tris = NewObject< FCollisionTriangleSoupData >();
        TPodArray< Float3 > & collisVerts = tris->Vertices;
        TPodArray< unsigned int > & collisInd = tris->Indices;

        FQuakeBSPModel * m = &_Model->Models[0];

        int numVertices = 0;
        int numIndices = 0;

        for ( int j = 0 ; j < m->NumSurfaces ; j++ ) {
            FSurfaceDef * surf = &_Model->BSP.Surfaces[ m->FirstSurf + j ];

            QLightmapGroup * lightmapGroup = &_Model->LightmapGroups[ surf->LightmapGroup ];
            FString const & name = _Model->Textures[ lightmapGroup->TextureIndex ].Object->GetName();
            if ( name[ 0 ] == '*' /*|| name == "trigger" */) {
                continue;
            }

            numVertices += surf->NumVertices;
            numIndices += surf->NumIndices;
        }

        collisVerts.ResizeInvalidate( numVertices );
        collisInd.ResizeInvalidate( numIndices );

        Float3 * pVert = collisVerts.ToPtr();
        unsigned int *pInd = collisInd.ToPtr();

        int firstVert = 0;
        for ( int j = 0 ; j < m->NumSurfaces ; j++ ) {
            FSurfaceDef * surf = &_Model->BSP.Surfaces[ m->FirstSurf + j ];

            QLightmapGroup * lightmapGroup = &_Model->LightmapGroups[ surf->LightmapGroup ];
            FString const & name = _Model->Textures[ lightmapGroup->TextureIndex ].Object->GetName();
            if ( name[ 0 ] == '*' /*|| name == "trigger" */) {
                continue;
            }

            for ( int v = 0 ; v < surf->NumVertices ; v++ ) {
                *pVert++ = _Model->BSP.Vertices[surf->FirstVertex + v].Position;
            }
#if 1
            for ( int v = 0 ; v < surf->NumIndices ; v++ ) {
                *pInd++ = firstVert + _Model->BSP.Indices[surf->FirstIndex + v];
            }
#else
            for ( int v = 0; v < surf->NumIndices; v+=3 ) {
                *pInd++ = firstVert + _Model->BSP.Indices[ surf->FirstIndex + v + 2 ];
                *pInd++ = firstVert + _Model->BSP.Indices[ surf->FirstIndex + v + 1 ];
                *pInd++ = firstVert + _Model->BSP.Indices[ surf->FirstIndex + v ];
            }
#endif
            firstVert += surf->NumVertices;
        }

        tris->Subparts.Resize( 1 );
        tris->Subparts[0].BaseVertex = 0;
        tris->Subparts[0].FirstIndex = 0;
        tris->Subparts[0].VertexCount = collisVerts.Size();
        tris->Subparts[0].IndexCount = collisInd.Size();
        tris->BoundingBox = m->BoundingBox;

        FCollisionTriangleSoupBVHData * bvh = NewObject< FCollisionTriangleSoupBVHData >();
        bvh->TrisData = tris;
        bvh->BuildBVH();

        // Create collision model
        FPhysicalBody * physBody = GetComponent< FPhysicalBody >();
        if ( !physBody ) {
            physBody = AddComponent< FPhysicalBody >( "physbody" );
        }
        physBody->BodyComposition.Clear();

        FCollisionTriangleSoupBVH * collisionBody = physBody->BodyComposition.AddCollisionBody< FCollisionTriangleSoupBVH >();
        collisionBody->BvhData = bvh;

        physBody->PhysicsBehavior = PB_STATIC;
        physBody->CollisionGroup = CM_WORLD_STATIC;
        physBody->CollisionMask = CM_ALL;
        physBody->bAINavigation = true;
        physBody->RegisterComponent();

        RootComponent = physBody;
    }

protected:
    FWorldCollisionActor() {

    }
};

AN_CLASS_META( FWorldCollisionActor )

bool FGameModule::LoadQuakeMap( const char * _MapName ) {
    static const char * MonsterClassName[][2] = {
        { "monster_ogre", "M_Ogre" },
        { "monster_knight", "M_Knight" },
        { "monster_demon1", "M_Demon" },
        { "monster_shambler", "M_Shambler" },
        { "monster_zombie", "M_Zombie" },
        { "monster_wizard", "M_Wizard" },
        { "monster_army", "M_Army" },
        { "monster_dog", "M_Dog" },
        { "monster_knight", "M_Knight" },
        { "monster_knight", "M_Knight" },
        { "monster_knight", "M_Knight" },
        { "monster_knight", "M_Knight" },
        { "monster_shalrath", "M_Shalrath" },
        { "monster_hell_knight", "M_HellKnight" },
        { "light_torch_small_walltorch", "M_Torch" },
        { "light_flame_large_yellow", "M_Flame" }
    };

    constexpr int NumClasses = AN_ARRAY_LENGTH( MonsterClassName );

    FQuakeBSP * model = NewObject< FQuakeBSP >();

    bool bFound = false;
    for ( int i = 0 ; i < MAX_PACKS ; i++ ) {
        if ( model->LoadFromPack( Level, &Packs[i], QuakePalette, _MapName ) ) {
            bFound = true;
            break;
        }
    }
    if ( !bFound ) {
        return false;
    }

    Level->DestroyActors();

    CleanResources();

    TActorSpawnParameters< FSpectator > SpectatorSpawnParameters;

    // Spawn map monsters
    for ( QEntity & ent : model->Entities ) {

        for ( int i = 0 ; i < NumClasses ; i++ ) {
            if ( !FString::Icmp( ent.ClassName, MonsterClassName[i][0] ) ) {

                FClassMeta const * meta = FActor::Factory().LookupClass( MonsterClassName[i][1] );
                FActorSpawnParameters spawnParameters( meta );

                spawnParameters.SpawnTransform.Position = ent.Origin;
                spawnParameters.SpawnTransform.Rotation.FromAngles( 0, FMath::Radians( ent.Angle ), 0 );
                spawnParameters.Level = Level;

                World->SpawnActor( spawnParameters );
                break;
            }
        }

        if ( !FString::Icmp( ent.ClassName, "info_player_start" ) ) {
            PlayerSpawnParameters.SpawnTransform.Position = ent.Origin + Float3(0,27.0f/32.0f,0);
            PlayerSpawnParameters.SpawnTransform.Rotation.FromAngles( 0, FMath::Radians( ent.Angle + 180 ), 0 );
        }
    }

    // Spawn player
    PlayerSpawnParameters.Level = Level;
    Player = World->SpawnActor< FPlayer >( PlayerSpawnParameters );

    //
    // Spawn specator
    //
    SpectatorSpawnParameters.SpawnTransform = PlayerSpawnParameters.SpawnTransform;
    SpectatorSpawnParameters.Level = Level;
    Spectator = World->SpawnActor< FSpectator >( SpectatorSpawnParameters );

    // Spawn bsp view actor
    FQuakeBSPView * BSPView = World->SpawnActor< FQuakeBSPView >( Level );
    BSPView->SetModel( model );

    // Spawn bsp collision actor
    FWorldCollisionActor * BSPCollision = World->SpawnActor< FWorldCollisionActor >( Level );
    BSPCollision->SetModel( model );

    // Create models
    FMaterialInstance * materialInstance = NewObject< FMaterialInstance >();
    materialInstance->Material = GetResource< FMaterial >( "WallMaterial" );
    materialInstance->SetTexture( 0, GetResource< FTexture >( "MipmapChecker" ) );

    FMaterialInstance * materialInstance2 = NewObject< FMaterialInstance >();
    materialInstance2->Material = GetResource< FMaterial >( "WallMaterial" );
    materialInstance2->SetTexture( 0, GetOrCreateResource< FTexture >( "blank512.png" ) );
#if 1
    for ( int i = 1 ; i < model->Models.Size() ; i++ ) {
        FQuakeBSPModel * m = &model->Models[i];

        int numVertices = 0;
        int numIndices = 0;

        bool bTrigger = false;

        for ( int j = 0 ; j < m->NumSurfaces ; j++ ) {
            FSurfaceDef * surf = &model->BSP.Surfaces[ m->FirstSurf + j ];

            if ( !bTrigger ) {
                QLightmapGroup * lightmapGroup = &model->LightmapGroups[ surf->LightmapGroup ];
                FString const & name = model->Textures[ lightmapGroup->TextureIndex ].Object->GetName();
                if ( name == "trigger" ) {
                    bTrigger = true;
                }
            }

            numVertices += surf->NumVertices;
            numIndices += surf->NumIndices;
        }

        FIndexedMesh * mesh = NewObject< FIndexedMesh >();
        mesh->Initialize( numVertices, numIndices, 1, false, false );

        FMeshVertex * pVert = mesh->GetVertices();
        unsigned int *pInd = mesh->GetIndices();

        int firstVert = 0;
        for ( int j = 0 ; j < m->NumSurfaces ; j++ ) {
            FSurfaceDef * surf = &model->BSP.Surfaces[ m->FirstSurf + j ];

            for ( int v = 0 ; v < surf->NumVertices ; v++ ) {
                *pVert = model->BSP.Vertices[surf->FirstVertex + v];
                pVert++;
            }

            for ( int v = 0 ; v < surf->NumIndices ; v++ ) {
                *pInd++ = firstVert + model->BSP.Indices[surf->FirstIndex + v];
            }

            firstVert += surf->NumVertices;
        }

        mesh->SendVertexDataToGPU( numVertices, 0 );
        mesh->SendIndexDataToGPU( numIndices, 0 );

        mesh->SetName( "*" + Int(i).ToString() );

        mesh->GetSubpart(0)->MaterialInstance = bTrigger ? materialInstance2 : materialInstance;
        mesh->GetSubpart(0)->SetBoundingBox( m->BoundingBox );

#if 0
        FCollisionTriangleSoupData * tris = CreateInstanceOf< FCollisionTriangleSoupData >();
        tris->Initialize( ( float * )mesh->GetVertices()->Position.ToPtr(), sizeof( FMeshVertex ), numVertices,
            mesh->GetIndices(), numIndices, m->BoundingBox );

        FCollisionTriangleSoupBVHData * bvh = CreateInstanceOf< FCollisionTriangleSoupBVHData >();
        bvh->TrisData = tris;
        bvh->BuildBVH();

        mesh->BodyComposition.Clear();
        FCollisionTriangleSoupBVH * CollisionBody = mesh->BodyComposition.AddCollisionBody< FCollisionTriangleSoupBVH >();
        CollisionBody->BvhData = bvh;
#endif

        RegisterResource( mesh );

        TActorSpawnParameters< FStaticMesh > spawnParameters;

        spawnParameters.Level = Level;

        FStaticMesh * actor = World->SpawnActor( spawnParameters );
        actor->SetMesh( mesh );
    }
#endif
    // Setup player controller
    PlayerController->SetPawn( Player );
    PlayerController->SetViewCamera( Player->Camera );
    PlayerController->AddViewActor( BSPView );

    //Level->NavMesh.NavWalkableClimb = 0.9f;
    //Level->NavMesh.NavWalkableSlopeAngle = 70;
    Level->BuildNavMesh();

    return true;
}

void FGameModule::CleanResources() {
    UnregisterResources< FQuakeModel >();
    UnregisterResources< FQuakeAudio >();
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
    diffuseTexture->Filter = TextureFilter;

    FMaterialFloatBlock * floatConstant2 = proj->AddBlock< FMaterialFloatBlock >();
    floatConstant2->Value = 3.0f;

    FMaterialFloatBlock * floatConstant8 = proj->AddBlock< FMaterialFloatBlock >();
    floatConstant8->Value = 8.0f;

    FMaterialFloatBlock * floatConstant64 = proj->AddBlock< FMaterialFloatBlock >();
    floatConstant64->Value = 1.0f/32;//64.0f;

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

    FMaterial * WaterMaterial = builder->Build();
    WaterMaterial->SetName( "WaterMaterial" );
    RegisterResource( WaterMaterial );
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
    diffuseTexture->Filter = TextureFilter;

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
    //builder->RegisterTextureSlot( lightmapTexture );
    FMaterial * WallMaterial = builder->Build();
    WallMaterial->SetName( "WallMaterial" );
    RegisterResource( WallMaterial );

//    FDocument document;
//    int object = proj->Serialize( document );
//    GLogger.Print( document.ObjectToString( object ).ToConstChar() );
}

void FGameModule::CreateSkyMaterial() {
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
    FMaterialInTexCoordBlock * inTexCoord = proj->AddBlock< FMaterialInTexCoordBlock >();
    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    FAssemblyNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    NSV_TexCoord->Connect( inTexCoord, "Value" );

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * inViewPosition = proj->AddBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->AddBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
    positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
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

    FMaterialFloatBlock * scale = proj->AddBlock< FMaterialFloatBlock >();
    scale->Value = 2;

    FMaterialMulBlock * scaleDir = proj->AddBlock< FMaterialMulBlock >();
    scaleDir->ValueA->Connect( normDir, "Result" );
    scaleDir->ValueB->Connect( scale, "Value" );

    // dir.x = -dir.x
    FMaterialDecomposeVectorBlock * decomposeDir = proj->AddBlock< FMaterialDecomposeVectorBlock >();
    decomposeDir->Vector->Connect( scaleDir, "Result" );
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

#if 0
    // resultColor = color1 + color2
    FMaterialAddBlock * resultColor = proj->NewBlock< FMaterialAddBlock >();
    resultColor->ValueA->Connect( color1, "RGBA" );
    resultColor->ValueB->Connect( color2, "RGBA" );
#else
    // resultColor = lerp( color1, color2, color2.a )
    FMaterialLerpBlock * resultColor = proj->AddBlock< FMaterialLerpBlock >();
    resultColor->ValueA->Connect( color1, "RGBA" );
    resultColor->ValueB->Connect( color2, "RGBA" );
    resultColor->ValueC->Connect( color2, "A" );
#endif

    FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( resultColor, "Result" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( skyTexture );

    FMaterial * SkyMaterial = builder->Build();
    SkyMaterial->SetName( "SkyMaterial" );
    RegisterResource( SkyMaterial );

//    FDocument document;
//    int object = proj->Serialize( document );
//    GLogger.Print( document.ObjectToString( object ).ToConstChar() );
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
    FMaterialInTexCoordBlock * inTexCoord = proj->AddBlock< FMaterialInTexCoordBlock >();
    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
    FAssemblyNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    NSV_TexCoord->Connect( inTexCoord, "Value" );

    //
    // VS_Dir = InPosition - ViewPostion.xyz;
    //
    FMaterialInViewPositionBlock * inViewPosition = proj->AddBlock< FMaterialInViewPositionBlock >();
    FMaterialSubBlock * positionMinusViewPosition = proj->AddBlock< FMaterialSubBlock >();
    positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
    positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
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

    FMaterial * SkyboxMaterial = builder->Build();
    SkyboxMaterial->SetName( "SkyboxMaterial" );
    RegisterResource( SkyboxMaterial );
}

void FGameModule::CreateSkinMaterial() {
    FMaterialProject * proj = NewObject< FMaterialProject >();

    FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();
    FMaterialInTexCoordBlock * inTexCoordBlock = proj->AddBlock< FMaterialInTexCoordBlock >();

    FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
    materialVertexStage->Position->Connect( inPositionBlock, "Value" );

    materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );

    FAssemblyNextStageVariable * texCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
    texCoord->Connect( inTexCoordBlock, "Value" );

    FMaterialTextureSlotBlock * diffuseTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
    diffuseTexture->Filter = TextureFilter;
    diffuseTexture->AddressU = diffuseTexture->AddressV = diffuseTexture->AddressW = TEXTURE_ADDRESS_CLAMP;

    FMaterialSamplerBlock * diffuseSampler = proj->AddBlock< FMaterialSamplerBlock >();
    diffuseSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
    diffuseSampler->TextureSlot->Connect( diffuseTexture, "Value" );

    FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
    materialFragmentStage->Color->Connect( diffuseSampler, "RGBA" );

    FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
    builder->VertexStage = materialVertexStage;
    builder->FragmentStage = materialFragmentStage;
    builder->MaterialType = MATERIAL_TYPE_UNLIT;
    builder->RegisterTextureSlot( diffuseTexture );

    FMaterial * SkinMaterial = builder->Build();
    SkinMaterial->SetName( "SkinMaterial" );
    RegisterResource( SkinMaterial );
}

void FGameModule::DrawCanvas( FCanvas * _Canvas ) {
    _Canvas->DrawViewport( PlayerController, 0, 0, _Canvas->Width, _Canvas->Height );
}


/*

Quake resources

sound/items/r_item1.wav
sound/items/r_item2.wav
sound/items/health1.wav
sound/misc/medkey.wav
sound/misc/runekey.wav
sound/items/protect.wav
sound/items/protect2.wav
sound/items/protect3.wav
sound/items/suit.wav
sound/items/suit2.wav
sound/items/inv1.wav
sound/items/inv2.wav
sound/items/inv3.wav
sound/items/damage.wav
sound/items/damage2.wav
sound/items/damage3.wav
sound/weapons/r_exp3.wav
sound/weapons/rocket1i.wav
sound/weapons/sgun1.wav
sound/weapons/guncock.wav
sound/weapons/ric1.wav
sound/weapons/ric2.wav
sound/weapons/ric3.wav
sound/weapons/spike2.wav
sound/weapons/tink1.wav
sound/weapons/grenade.wav
sound/weapons/bounce.wav
sound/weapons/shotgn2.wav
sound/misc/menu1.wav
sound/misc/menu2.wav
sound/misc/menu3.wav
sound/ambience/water1.wav
sound/ambience/wind2.wav
sound/demon/dland2.wav
sound/misc/h2ohit1.wav
sound/items/itembk2.wav
sound/player/plyrjmp8.wav
sound/player/land.wav
sound/player/land2.wav
sound/player/drown1.wav
sound/player/drown2.wav
sound/player/gasp1.wav
sound/player/gasp2.wav
sound/player/h2odeath.wav
sound/misc/talk.wav
sound/player/teledth1.wav
sound/misc/r_tele1.wav
sound/misc/r_tele2.wav
sound/misc/r_tele3.wav
sound/misc/r_tele4.wav
sound/misc/r_tele5.wav
sound/weapons/lock4.wav
sound/weapons/pkup.wav
sound/items/armor1.wav
sound/weapons/lhit.wav
sound/weapons/lstart.wav
sound/misc/power.wav
sound/player/gib.wav
sound/player/udeath.wav
sound/player/tornoff2.wav
sound/player/pain1.wav
sound/player/pain2.wav
sound/player/pain3.wav
sound/player/pain4.wav
sound/player/pain5.wav
sound/player/pain6.wav
sound/player/death1.wav
sound/player/death2.wav
sound/player/death3.wav
sound/player/death4.wav
sound/player/death5.wav
sound/weapons/ax1.wav
sound/player/axhit1.wav
sound/player/axhit2.wav
sound/player/h2ojump.wav
sound/player/slimbrn2.wav
sound/player/inh2o.wav
sound/player/inlava.wav
sound/misc/outwater.wav
sound/player/lburn1.wav
sound/player/lburn2.wav
sound/misc/water1.wav
sound/misc/water2.wav
sound/doors/medtry.wav
sound/doors/meduse.wav
sound/doors/runetry.wav
sound/doors/runeuse.wav
sound/doors/basetry.wav
sound/doors/baseuse.wav
sound/misc/null.wav
sound/doors/drclos4.wav
sound/doors/doormv1.wav
sound/doors/hydro1.wav
sound/doors/hydro2.wav
sound/doors/stndr1.wav
sound/doors/stndr2.wav
sound/doors/ddoor1.wav
sound/doors/ddoor2.wav
sound/doors/latch2.wav
sound/doors/winch2.wav
sound/doors/airdoor1.wav
sound/doors/airdoor2.wav
sound/doors/basesec1.wav
sound/doors/basesec2.wav
sound/buttons/airbut1.wav
sound/buttons/switch21.wav
sound/buttons/switch02.wav
sound/buttons/switch04.wav
sound/misc/secret.wav
sound/misc/trigger1.wav
sound/ambience/hum1.wav
sound/ambience/windfly.wav
sound/plats/plat1.wav
sound/plats/plat2.wav
sound/plats/medplat1.wav
sound/plats/medplat2.wav
sound/plats/train2.wav
sound/plats/train1.wav
sound/ambience/fl_hum1.wav
sound/ambience/buzz1.wav
sound/ambience/fire1.wav
sound/ambience/suck1.wav
sound/ambience/drone6.wav
sound/ambience/drip1.wav
sound/ambience/comp1.wav
sound/ambience/thunder1.wav
sound/ambience/swamp1.wav
sound/ambience/swamp2.wav
sound/ogre/ogdrag.wav
sound/ogre/ogdth.wav
sound/ogre/ogidle.wav
sound/ogre/ogidle2.wav
sound/ogre/ogpain1.wav
sound/ogre/ogsawatk.wav
sound/ogre/ogwake.wav
sound/demon/ddeath.wav
sound/demon/dhit2.wav
sound/demon/djump.wav
sound/demon/dpain1.wav
sound/demon/idle1.wav
sound/demon/sight2.wav
sound/shambler/sattck1.wav
sound/shambler/sboom.wav
sound/shambler/sdeath.wav
sound/shambler/shurt2.wav
sound/shambler/sidle.wav
sound/shambler/ssight.wav
sound/shambler/melee1.wav
sound/shambler/melee2.wav
sound/shambler/smack.wav
sound/knight/kdeath.wav
sound/knight/khurt.wav
sound/knight/ksight.wav
sound/knight/sword1.wav
sound/knight/sword2.wav
sound/knight/idle.wav
sound/soldier/death1.wav
sound/soldier/idle.wav
sound/soldier/pain1.wav
sound/soldier/pain2.wav
sound/soldier/sattck1.wav
sound/soldier/sight1.wav
sound/wizard/hit.wav
sound/wizard/wattack.wav
sound/wizard/wdeath.wav
sound/wizard/widle1.wav
sound/wizard/widle2.wav
sound/wizard/wpain.wav
sound/wizard/wsight.wav
sound/dog/dattack1.wav
sound/dog/ddeath.wav
sound/dog/dpain1.wav
sound/dog/dsight.wav
sound/dog/idle.wav
sound/zombie/z_idle.wav
sound/zombie/z_idle1.wav
sound/zombie/z_shot1.wav
sound/zombie/z_gib.wav
sound/zombie/z_pain.wav
sound/zombie/z_pain1.wav
sound/zombie/z_fall.wav
sound/zombie/z_miss.wav
sound/zombie/z_hit.wav
sound/zombie/idle_w2.wav
sound/boss1/out1.wav
sound/boss1/sight1.wav
sound/boss1/throw.wav
sound/boss1/pain.wav
sound/boss1/death.wav
sound/hknight/hit.wav
maps/b_bh10.bsp
maps/b_bh100.bsp
maps/b_bh25.bsp
progs/armor.mdl
progs/g_shot.mdl
progs/g_nail.mdl
progs/g_nail2.mdl
progs/g_rock.mdl
progs/g_rock2.mdl
progs/g_light.mdl
maps/b_shell1.bsp
maps/b_shell0.bsp
maps/b_nail1.bsp
maps/b_nail0.bsp
maps/b_rock1.bsp
maps/b_rock0.bsp
maps/b_batt1.bsp
maps/b_batt0.bsp
progs/w_s_key.mdl
progs/m_s_key.mdl
progs/w_g_key.mdl
progs/m_g_key.mdl
progs/end1.mdl
progs/invulner.mdl
progs/suit.mdl
progs/invisibl.mdl
progs/quaddama.mdl
progs/player.mdl
progs/eyes.mdl
progs/h_player.mdl
progs/gib1.mdl
progs/gib2.mdl
progs/gib3.mdl
progs/s_bubble.spr
progs/s_explod.spr
progs/v_axe.mdl
progs/v_shot.mdl
progs/v_nail.mdl
progs/v_rock.mdl
progs/v_shot2.mdl
progs/v_nail2.mdl
progs/v_rock2.mdl
progs/bolt.mdl
progs/bolt2.mdl
progs/bolt3.mdl
progs/lavaball.mdl
progs/missile.mdl
progs/grenade.mdl
progs/spike.mdl
progs/s_spike.mdl
progs/backpack.mdl
progs/zom_gib.mdl
progs/v_light.mdl
progs/s_light.spr
progs/flame.mdl
progs/flame2.mdl
maps/b_explob.bsp
progs/ogre.mdl
progs/h_ogre.mdl
progs/demon.mdl
progs/h_demon.mdl
progs/shambler.mdl
progs/s_light.mdl
progs/h_shams.mdl
progs/knight.mdl
progs/h_knight.mdl
progs/soldier.mdl
progs/h_guard.mdl
progs/wizard.mdl
progs/h_wizard.mdl
progs/w_spike.mdl
progs/h_dog.mdl
progs/dog.mdl
progs/zombie.mdl
progs/h_zombie.mdl
progs/boss.mdl
progs.dat
gfx.wad
quake.rc
default.cfg
end1.bin
demo1.dem
demo2.dem
demo3.dem
gfx/palette.lmp
gfx/colormap.lmp
gfx/complete.lmp
gfx/inter.lmp
gfx/ranking.lmp
gfx/vidmodes.lmp
gfx/finale.lmp
gfx/conback.lmp
gfx/qplaque.lmp
gfx/menudot1.lmp
gfx/menudot2.lmp
gfx/menudot3.lmp
gfx/menudot4.lmp
gfx/menudot5.lmp
gfx/menudot6.lmp
gfx/menuplyr.lmp
gfx/bigbox.lmp
gfx/dim_modm.lmp
gfx/dim_drct.lmp
gfx/dim_ipx.lmp
gfx/dim_tcp.lmp
gfx/dim_mult.lmp
gfx/mainmenu.lmp
gfx/box_tl.lmp
gfx/box_tm.lmp
gfx/box_tr.lmp
gfx/box_ml.lmp
gfx/box_mm.lmp
gfx/box_mm2.lmp
gfx/box_mr.lmp
gfx/box_bl.lmp
gfx/box_bm.lmp
gfx/box_br.lmp
gfx/sp_menu.lmp
gfx/ttl_sgl.lmp
gfx/ttl_main.lmp
gfx/ttl_cstm.lmp
gfx/mp_menu.lmp
gfx/netmen1.lmp
gfx/netmen2.lmp
gfx/netmen3.lmp
gfx/netmen4.lmp
gfx/netmen5.lmp
gfx/sell.lmp
gfx/help0.lmp
gfx/help1.lmp
gfx/help2.lmp
gfx/help3.lmp
gfx/help4.lmp
gfx/help5.lmp
gfx/pause.lmp
gfx/loading.lmp
gfx/p_option.lmp
gfx/p_load.lmp
gfx/p_save.lmp
gfx/p_multi.lmp
maps/start.bsp
maps/e1m1.bsp
maps/e1m2.bsp
maps/e1m3.bsp
maps/e1m4.bsp
maps/e1m5.bsp
maps/e1m6.bsp
maps/e1m7.bsp
maps/e1m8.bsp


sound/items/r_item1.wav
sound/items/r_item2.wav
sound/items/health1.wav
sound/misc/medkey.wav
sound/misc/runekey.wav
sound/items/protect.wav
sound/items/protect2.wav
sound/items/protect3.wav
sound/items/suit.wav
sound/items/suit2.wav
sound/items/inv1.wav
sound/items/inv2.wav
sound/items/inv3.wav
sound/items/damage.wav
sound/items/damage2.wav
sound/items/damage3.wav
sound/weapons/r_exp3.wav
sound/weapons/rocket1i.wav
sound/weapons/sgun1.wav
sound/weapons/guncock.wav
sound/weapons/ric1.wav
sound/weapons/ric2.wav
sound/weapons/ric3.wav
sound/weapons/spike2.wav
sound/weapons/tink1.wav
sound/weapons/grenade.wav
sound/weapons/bounce.wav
sound/weapons/shotgn2.wav
sound/misc/menu1.wav
sound/misc/menu2.wav
sound/misc/menu3.wav
sound/ambience/water1.wav
sound/ambience/wind2.wav
sound/demon/dland2.wav
sound/misc/h2ohit1.wav
sound/items/itembk2.wav
sound/player/plyrjmp8.wav
sound/player/land.wav
sound/player/land2.wav
sound/player/drown1.wav
sound/player/drown2.wav
sound/player/gasp1.wav
sound/player/gasp2.wav
sound/player/h2odeath.wav
sound/misc/talk.wav
sound/player/teledth1.wav
sound/misc/r_tele1.wav
sound/misc/r_tele2.wav
sound/misc/r_tele3.wav
sound/misc/r_tele4.wav
sound/misc/r_tele5.wav
sound/weapons/lock4.wav
sound/weapons/pkup.wav
sound/items/armor1.wav
sound/weapons/lhit.wav
sound/weapons/lstart.wav
sound/misc/power.wav
sound/player/gib.wav
sound/player/udeath.wav
sound/player/tornoff2.wav
sound/player/pain1.wav
sound/player/pain2.wav
sound/player/pain3.wav
sound/player/pain4.wav
sound/player/pain5.wav
sound/player/pain6.wav
sound/player/death1.wav
sound/player/death2.wav
sound/player/death3.wav
sound/player/death4.wav
sound/player/death5.wav
sound/weapons/ax1.wav
sound/player/axhit1.wav
sound/player/axhit2.wav
sound/player/h2ojump.wav
sound/player/slimbrn2.wav
sound/player/inh2o.wav
sound/player/inlava.wav
sound/misc/outwater.wav
sound/player/lburn1.wav
sound/player/lburn2.wav
sound/misc/water1.wav
sound/misc/water2.wav
sound/doors/medtry.wav
sound/doors/meduse.wav
sound/doors/runetry.wav
sound/doors/runeuse.wav
sound/doors/basetry.wav
sound/doors/baseuse.wav
sound/misc/null.wav
sound/doors/drclos4.wav
sound/doors/doormv1.wav
sound/doors/hydro1.wav
sound/doors/hydro2.wav
sound/doors/stndr1.wav
sound/doors/stndr2.wav
sound/doors/ddoor1.wav
sound/doors/ddoor2.wav
sound/doors/latch2.wav
sound/doors/winch2.wav
sound/doors/airdoor1.wav
sound/doors/airdoor2.wav
sound/doors/basesec1.wav
sound/doors/basesec2.wav
sound/buttons/airbut1.wav
sound/buttons/switch21.wav
sound/buttons/switch02.wav
sound/buttons/switch04.wav
sound/misc/secret.wav
sound/misc/trigger1.wav
sound/ambience/hum1.wav
sound/ambience/windfly.wav
sound/plats/plat1.wav
sound/plats/plat2.wav
sound/plats/medplat1.wav
sound/plats/medplat2.wav
sound/plats/train2.wav
sound/plats/train1.wav
sound/ambience/fl_hum1.wav
sound/ambience/buzz1.wav
sound/ambience/fire1.wav
sound/ambience/suck1.wav
sound/ambience/drone6.wav
sound/ambience/drip1.wav
sound/ambience/comp1.wav
sound/ambience/thunder1.wav
sound/ambience/swamp1.wav
sound/ambience/swamp2.wav
sound/ogre/ogdrag.wav
sound/ogre/ogdth.wav
sound/ogre/ogidle.wav
sound/ogre/ogidle2.wav
sound/ogre/ogpain1.wav
sound/ogre/ogsawatk.wav
sound/ogre/ogwake.wav
sound/demon/ddeath.wav
sound/demon/dhit2.wav
sound/demon/djump.wav
sound/demon/dpain1.wav
sound/demon/idle1.wav
sound/demon/sight2.wav
sound/shambler/sattck1.wav
sound/shambler/sboom.wav
sound/shambler/sdeath.wav
sound/shambler/shurt2.wav
sound/shambler/sidle.wav
sound/shambler/ssight.wav
sound/shambler/melee1.wav
sound/shambler/melee2.wav
sound/shambler/smack.wav
sound/knight/kdeath.wav
sound/knight/khurt.wav
sound/knight/ksight.wav
sound/knight/sword1.wav
sound/knight/sword2.wav
sound/knight/idle.wav
sound/soldier/death1.wav
sound/soldier/idle.wav
sound/soldier/pain1.wav
sound/soldier/pain2.wav
sound/soldier/sattck1.wav
sound/soldier/sight1.wav
sound/wizard/hit.wav
sound/wizard/wattack.wav
sound/wizard/wdeath.wav
sound/wizard/widle1.wav
sound/wizard/widle2.wav
sound/wizard/wpain.wav
sound/wizard/wsight.wav
sound/dog/dattack1.wav
sound/dog/ddeath.wav
sound/dog/dpain1.wav
sound/dog/dsight.wav
sound/dog/idle.wav
sound/zombie/z_idle.wav
sound/zombie/z_idle1.wav
sound/zombie/z_shot1.wav
sound/zombie/z_gib.wav
sound/zombie/z_pain.wav
sound/zombie/z_pain1.wav
sound/zombie/z_fall.wav
sound/zombie/z_miss.wav
sound/zombie/z_hit.wav
sound/zombie/idle_w2.wav
sound/boss1/out1.wav
sound/boss1/sight1.wav
sound/boss1/throw.wav
sound/boss1/pain.wav
sound/boss1/death.wav
sound/hknight/hit.wav
maps/b_bh10.bsp
maps/b_bh100.bsp
maps/b_bh25.bsp
progs/armor.mdl
progs/g_shot.mdl
progs/g_nail.mdl
progs/g_nail2.mdl
progs/g_rock.mdl
progs/g_rock2.mdl
progs/g_light.mdl
maps/b_shell1.bsp
maps/b_shell0.bsp
maps/b_nail1.bsp
maps/b_nail0.bsp
maps/b_rock1.bsp
maps/b_rock0.bsp
maps/b_batt1.bsp
maps/b_batt0.bsp
progs/w_s_key.mdl
progs/m_s_key.mdl
progs/w_g_key.mdl
progs/m_g_key.mdl
progs/end1.mdl
progs/invulner.mdl
progs/suit.mdl
progs/invisibl.mdl
progs/quaddama.mdl
progs/player.mdl
progs/eyes.mdl
progs/h_player.mdl
progs/gib1.mdl
progs/gib2.mdl
progs/gib3.mdl
progs/s_bubble.spr
progs/s_explod.spr
progs/v_axe.mdl
progs/v_shot.mdl
progs/v_nail.mdl
progs/v_rock.mdl
progs/v_shot2.mdl
progs/v_nail2.mdl
progs/v_rock2.mdl
progs/bolt.mdl
progs/bolt2.mdl
progs/bolt3.mdl
progs/lavaball.mdl
progs/missile.mdl
progs/grenade.mdl
progs/spike.mdl
progs/s_spike.mdl
progs/backpack.mdl
progs/zom_gib.mdl
progs/v_light.mdl
progs/s_light.spr
progs/flame.mdl
progs/flame2.mdl
maps/b_explob.bsp
progs/ogre.mdl
progs/h_ogre.mdl
progs/demon.mdl
progs/h_demon.mdl
progs/shambler.mdl
progs/s_light.mdl
progs/h_shams.mdl
progs/knight.mdl
progs/h_knight.mdl
progs/soldier.mdl
progs/h_guard.mdl
progs/wizard.mdl
progs/h_wizard.mdl
progs/w_spike.mdl
progs/h_dog.mdl
progs/dog.mdl
progs/zombie.mdl
progs/h_zombie.mdl
progs/boss.mdl
progs.dat
gfx.wad
quake.rc
default.cfg
end1.bin
demo1.dem
demo2.dem
demo3.dem
gfx/palette.lmp
gfx/colormap.lmp
gfx/complete.lmp
gfx/inter.lmp
gfx/ranking.lmp
gfx/vidmodes.lmp
gfx/finale.lmp
gfx/conback.lmp
gfx/qplaque.lmp
gfx/menudot1.lmp
gfx/menudot2.lmp
gfx/menudot3.lmp
gfx/menudot4.lmp
gfx/menudot5.lmp
gfx/menudot6.lmp
gfx/menuplyr.lmp
gfx/bigbox.lmp
gfx/dim_modm.lmp
gfx/dim_drct.lmp
gfx/dim_ipx.lmp
gfx/dim_tcp.lmp
gfx/dim_mult.lmp
gfx/mainmenu.lmp
gfx/box_tl.lmp
gfx/box_tm.lmp
gfx/box_tr.lmp
gfx/box_ml.lmp
gfx/box_mm.lmp
gfx/box_mm2.lmp
gfx/box_mr.lmp
gfx/box_bl.lmp
gfx/box_bm.lmp
gfx/box_br.lmp
gfx/sp_menu.lmp
gfx/ttl_sgl.lmp
gfx/ttl_main.lmp
gfx/ttl_cstm.lmp
gfx/mp_menu.lmp
gfx/netmen1.lmp
gfx/netmen2.lmp
gfx/netmen3.lmp
gfx/netmen4.lmp
gfx/netmen5.lmp
gfx/sell.lmp
gfx/help0.lmp
gfx/help1.lmp
gfx/help2.lmp
gfx/help3.lmp
gfx/help4.lmp
gfx/help5.lmp
gfx/pause.lmp
gfx/loading.lmp
gfx/p_option.lmp
gfx/p_load.lmp
gfx/p_save.lmp
gfx/p_multi.lmp
maps/start.bsp
maps/e1m1.bsp
maps/e1m2.bsp
maps/e1m3.bsp
maps/e1m4.bsp
maps/e1m5.bsp
maps/e1m6.bsp
maps/e1m7.bsp
maps/e1m8.bsp
sound/misc/basekey.wav
sound/enforcer/enfire.wav
sound/enforcer/enfstop.wav
sound/enforcer/sight1.wav
sound/enforcer/sight2.wav
sound/enforcer/sight3.wav
sound/enforcer/sight4.wav
sound/enforcer/pain1.wav
sound/enforcer/pain2.wav
sound/enforcer/death1.wav
sound/enforcer/idle1.wav
sound/blob/death1.wav
sound/blob/hit1.wav
sound/blob/land1.wav
sound/blob/sight1.wav
sound/hknight/attack1.wav
sound/hknight/death1.wav
sound/hknight/pain1.wav
sound/hknight/sight1.wav
sound/hknight/slash1.wav
sound/hknight/idle.wav
sound/hknight/grunt.wav
sound/fish/death.wav
sound/fish/bite.wav
sound/fish/idle.wav
sound/shalrath/attack.wav
sound/shalrath/attack2.wav
sound/shalrath/death.wav
sound/shalrath/idle.wav
sound/shalrath/pain.wav
sound/shalrath/sight.wav
sound/boss2/death.wav
sound/boss2/idle.wav
sound/boss2/sight.wav
sound/boss2/pop2.wav
progs/b_s_key.mdl
progs/b_g_key.mdl
progs/end2.mdl
progs/end3.mdl
progs/end4.mdl
progs/teleport.mdl
maps/b_exbox2.bsp
progs/laser.mdl
progs/tarbaby.mdl
progs/hknight.mdl
progs/k_spike.mdl
progs/h_hellkn.mdl
progs/fish.mdl
progs/shalrath.mdl
progs/h_shal.mdl
progs/v_spike.mdl
progs/enforcer.mdl
progs/h_mega.mdl
progs/oldone.mdl
end2.bin
gfx/pop.lmp
maps/e2m1.bsp
maps/e2m2.bsp
maps/e2m3.bsp
maps/e2m4.bsp
maps/e2m5.bsp
maps/e2m6.bsp
maps/e2m7.bsp
maps/e3m1.bsp
maps/e3m2.bsp
maps/e3m3.bsp
maps/e3m4.bsp
maps/e3m5.bsp
maps/e3m6.bsp
maps/e3m7.bsp
maps/e4m1.bsp
maps/e4m2.bsp
maps/e4m3.bsp
maps/e4m4.bsp
maps/e4m5.bsp
maps/e4m6.bsp
maps/e4m7.bsp
maps/e4m8.bsp
maps/end.bsp
maps/dm1.bsp
maps/dm2.bsp
maps/dm3.bsp
maps/dm4.bsp
maps/dm5.bsp
maps/dm6.bsp
*/
