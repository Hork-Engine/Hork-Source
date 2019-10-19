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

#include <Engine/Resource/Public/Material.h>
#include <Engine/Resource/Public/MaterialAssembly.h>
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>
#include <Engine/Runtime/Public/Runtime.h>

AN_CLASS_META( FMaterial )
AN_CLASS_META( FMaterialInstance )

FMaterial::FMaterial() {
    MaterialGPU = GRenderBackend->CreateMaterial( this );
}

FMaterial::~FMaterial() {
    GRenderBackend->DestroyMaterial( MaterialGPU );
}

void FMaterial::Initialize( FMaterialBuildData const * _Data ) {
    NumUniformVectors = _Data->NumUniformVectors;
    Type = _Data->Type;

    GRenderBackend->InitializeMaterial( MaterialGPU, _Data );
}

void FMaterial::InitializeInternalResource( const char * _InternalResourceName ) {
    if ( !FString::Icmp( _InternalResourceName, "FMaterial.Default" ) ) {
        FMaterialProject * proj = NewObject< FMaterialProject >();

        FMaterialInTexCoordBlock * inTexCoordBlock = proj->AddBlock< FMaterialInTexCoordBlock >();

        FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();

        FAssemblyNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );

        FMaterialTextureSlotBlock * diffuseTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
        diffuseTexture->Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
        diffuseTexture->AddressU = diffuseTexture->AddressV = diffuseTexture->AddressW = TEXTURE_ADDRESS_WRAP;

        FMaterialSamplerBlock * textureSampler = proj->AddBlock< FMaterialSamplerBlock >();
        textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
        materialFragmentStage->Color->Connect( textureSampler, "RGBA" );

        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_UNLIT;
        builder->RegisterTextureSlot( diffuseTexture );

        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GZoneMemory.Dealloc( buildData );
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FMaterial.Skybox" ) ) {
#if 1
        FMaterialProject * proj = NewObject< FMaterialProject >();

        FMaterialInPositionBlock * inPositionBlock = proj->AddBlock< FMaterialInPositionBlock >();

        FMaterialVertexStage * materialVertexStage = proj->AddBlock< FMaterialVertexStage >();
        materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );

        FAssemblyNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
        NSV_Dir->Connect( inPositionBlock, "Value" );

        FMaterialTextureSlotBlock * cubemapTexture = proj->AddBlock< FMaterialTextureSlotBlock >();
        cubemapTexture->TextureType = TEXTURE_CUBEMAP;
        cubemapTexture->Filter = TEXTURE_FILTER_LINEAR;
        cubemapTexture->AddressU = cubemapTexture->AddressV = cubemapTexture->AddressW = TEXTURE_ADDRESS_CLAMP;

        FMaterialSamplerBlock * cubemapSampler = proj->AddBlock< FMaterialSamplerBlock >();
        cubemapSampler->TexCoord->Connect( materialVertexStage, "Dir" );
        cubemapSampler->TextureSlot->Connect( cubemapTexture, "Value" );

        FMaterialFragmentStage * materialFragmentStage = proj->AddBlock< FMaterialFragmentStage >();
        materialFragmentStage->Color->Connect( cubemapSampler, "RGBA" );

        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_UNLIT;
        builder->MaterialFacing = MATERIAL_FACE_BACK;
        builder->DepthHack = MATERIAL_DEPTH_HACK_SKYBOX;
        builder->RegisterTextureSlot( cubemapTexture );

        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GZoneMemory.Dealloc( buildData );
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

        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GZoneMemory.Dealloc( buildData );
#endif
        return;
    }

    GLogger.Printf( "Unknown internal material %s\n", _InternalResourceName );
}

FMaterialInstance::FMaterialInstance() {
    static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.Default" ) );
    //static TStaticInternalResourceFinder< FTexture2D > TextureResource( _CTS( "FTexture2D.Default" ) );
    //static TStaticResourceFinder< FTexture2D > TextureResource( _CTS( "gridyblack.png" ) );
    static TStaticResourceFinder< FTexture2D > TextureResource( _CTS( "uv_checker.png" ) );

    Material = MaterialResource.GetObject();

    SetTexture( 0, TextureResource.GetObject() );
}

void FMaterialInstance::InitializeInternalResource( const char * _InternalResourceName ) {
    if ( !FString::Icmp( _InternalResourceName, "FMaterialInstance.Default" ) )
    {
        static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.Default" ) );
        //static TStaticInternalResourceFinder< FTexture2D > TextureResource( _CTS( "FTexture2D.Default" ) );
        //static TStaticResourceFinder< FTexture2D > TextureResource( _CTS( "gridyblack.png" ) );
        static TStaticResourceFinder< FTexture2D > TextureResource( _CTS( "uv_checker.png" ) );

        Material = MaterialResource.GetObject();

        SetTexture( 0, TextureResource.GetObject() );
        return;
    }
    GLogger.Printf( "Unknown internal material instance %s\n", _InternalResourceName );
}

void FMaterialInstance::SetMaterial( FMaterial * _Material ) {
    if ( !_Material ) {
        static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.Default" ) );

        Material = MaterialResource.GetObject();
    } else {
        Material = _Material;
    }
}

FMaterial * FMaterialInstance::GetMaterial() const {
    return Material;
}

void FMaterialInstance::SetTexture( int _TextureSlot, FTexture * _Texture ) {
    if ( _TextureSlot >= MAX_MATERIAL_TEXTURES ) {
        return;
    }
    Textures[_TextureSlot] = _Texture;
}

FMaterialFrameData * FMaterialInstance::RenderFrontend_Update( int _VisMarker ) {
    if ( VisMarker == _VisMarker ) {
        return FrameData;
    }

    VisMarker = _VisMarker;

    FrameData = ( FMaterialFrameData * )GRuntime.AllocFrameMem( sizeof( FMaterialFrameData ) );
    if ( !FrameData ) {
        return nullptr;
    }

    FrameData->Material = Material->GetGPUResource();

    FTextureGPU ** textures = FrameData->Textures;
    FrameData->NumTextures = 0;

    for ( int i = 0; i < MAX_MATERIAL_TEXTURES; i++ ) {
        if ( Textures[ i ] ) {

            FTextureGPU * textureProxy = Textures[ i ]->GetGPUResource();

            //if ( textureProxy->IsSubmittedToRenderThread() ) {
                textures[ i ] = textureProxy;
                FrameData->NumTextures = i + 1;
            //} else {
            //    textures[ i ] = 0;
            //}
        } else {
            textures[ i ] = 0;
        }
    }

    FrameData->NumUniformVectors = Material->GetNumUniformVectors();
    memcpy( FrameData->UniformVectors, UniformVectors, sizeof( Float4 )*FrameData->NumUniformVectors );

    return FrameData;
}
