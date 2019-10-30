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
#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/MaterialGraph/Public/MaterialGraph.h>
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
    if ( !FString::Icmp( _InternalResourceName, "FMaterial.Default" )
        || !FString::Icmp( _InternalResourceName, "FMaterial.DefaultUnlit" ) ) {
        MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

        MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();

        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();

        MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
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

    if ( !FString::Icmp( _InternalResourceName, "FMaterial.DefaultBaseLight" ) ) {
        MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

        MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();

        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();

        MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
        materialFragmentStage->Color->Connect( textureSampler, "RGBA" );

        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_BASELIGHT;
        builder->RegisterTextureSlot( diffuseTexture );

        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GZoneMemory.Dealloc( buildData );
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FMaterial.DefaultPBR" ) ) {
        MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

        MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();

        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();

        MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * metallicTexture = graph->AddNode< MGTextureSlot >();
        metallicTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * normalTexture = graph->AddNode< MGTextureSlot >();
        normalTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * roughnessTexture = graph->AddNode< MGTextureSlot >();
        roughnessTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicSampler = graph->AddNode< MGSampler >();
        metallicSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        metallicSampler->TextureSlot->Connect( metallicTexture, "Value" );

        MGSampler * roughnessSampler = graph->AddNode< MGSampler >();
        roughnessSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        roughnessSampler->TextureSlot->Connect( roughnessTexture, "Value" );

        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
        materialFragmentStage->Color->Connect( textureSampler, "RGBA" );
        materialFragmentStage->Normal->Connect( normalSampler, "XYZ" );
        materialFragmentStage->Metallic->Connect( metallicSampler, "R" );
        materialFragmentStage->Roughness->Connect( roughnessSampler, "R" );

        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_PBR;
        builder->RegisterTextureSlot( diffuseTexture );
        builder->RegisterTextureSlot( metallicTexture );
        builder->RegisterTextureSlot( normalTexture );
        builder->RegisterTextureSlot( roughnessTexture );

        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GZoneMemory.Dealloc( buildData );
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FMaterial.PBRMetallicRoughness" ) ) {
        MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

        MGInTexCoord * inTexCoordBlock = graph->AddNode< MGInTexCoord >();

        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();

        MGNextStageVariable * texCoord = materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        texCoord->Connect( inTexCoordBlock, "Value" );

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * metallicRoughnessTexture = graph->AddNode< MGTextureSlot >();
        metallicRoughnessTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * normalTexture = graph->AddNode< MGTextureSlot >();
        normalTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * ambientTexture = graph->AddNode< MGTextureSlot >();
        ambientTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * emissiveTexture = graph->AddNode< MGTextureSlot >();
        emissiveTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( materialVertexStage, "TexCoord" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
        materialFragmentStage->Color->Connect( textureSampler, "RGBA" );
        materialFragmentStage->Normal->Connect( normalSampler, "XYZ" );
        materialFragmentStage->Metallic->Connect( metallicRoughnessSampler, "B" );
        materialFragmentStage->Roughness->Connect( metallicRoughnessSampler, "G" );
        materialFragmentStage->Ambient->Connect( ambientSampler, "R" );
        materialFragmentStage->Emissive->Connect( emissiveSampler, "RGBA" );

        FMaterialBuilder * builder = NewObject< FMaterialBuilder >();
        builder->VertexStage = materialVertexStage;
        builder->FragmentStage = materialFragmentStage;
        builder->MaterialType = MATERIAL_TYPE_PBR;
        builder->RegisterTextureSlot( diffuseTexture );
        builder->RegisterTextureSlot( metallicRoughnessTexture );
        builder->RegisterTextureSlot( normalTexture );
        builder->RegisterTextureSlot( ambientTexture );
        builder->RegisterTextureSlot( emissiveTexture );

        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GZoneMemory.Dealloc( buildData );
        return;
    }

    if ( !FString::Icmp( _InternalResourceName, "FMaterial.Skybox" ) ) {
#if 1
        MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

        MGInPosition * inPositionBlock = graph->AddNode< MGInPosition >();

        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();
        materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );

        MGNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
        NSV_Dir->Connect( inPositionBlock, "Value" );

        MGTextureSlot * cubemapTexture = graph->AddNode< MGTextureSlot >();
        cubemapTexture->SamplerDesc.TextureType = TEXTURE_CUBEMAP;
        cubemapTexture->SamplerDesc.Filter = TEXTURE_FILTER_LINEAR;
        cubemapTexture->SamplerDesc.AddressU =
        cubemapTexture->SamplerDesc.AddressV =
        cubemapTexture->SamplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;

        MGSampler * cubemapSampler = graph->AddNode< MGSampler >();
        cubemapSampler->TexCoord->Connect( materialVertexStage, "Dir" );
        cubemapSampler->TextureSlot->Connect( cubemapTexture, "Value" );

        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
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
        MGMaterialGraph * graph = NewObject< MGMaterialGraph >();

        //
        // gl_Position = TransformMatrix * vec4( InPosition, 1.0 );
        //
        MGInPosition * inPositionBlock = graph->AddNode< MGInPosition >();
        MGVertexStage * materialVertexStage = graph->AddNode< MGVertexStage >();
        materialVertexStage->Position->Connect( inPositionBlock, "Value" );

        //
        // VS_TexCoord = InTexCoord;
        //
        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();
        materialVertexStage->AddNextStageVariable( "TexCoord", AT_Float2 );
        MGNextStageVariable * NSV_TexCoord = materialVertexStage->FindNextStageVariable( "TexCoord" );
        NSV_TexCoord->Connect( inTexCoord, "Value" );

        //
        // VS_Dir = InPosition - ViewPostion.xyz;
        //
        MGInViewPosition * inViewPosition = graph->AddNode< MGInViewPosition >();
        MGSubNode * positionMinusViewPosition = graph->AddNode< MGSubNode >();
        positionMinusViewPosition->ValueA->Connect( inPositionBlock, "Value" );
        positionMinusViewPosition->ValueB->Connect( inViewPosition, "Value" );
        materialVertexStage->AddNextStageVariable( "Dir", AT_Float3 );
        MGNextStageVariable * NSV_Dir = materialVertexStage->FindNextStageVariable( "Dir" );
        NSV_Dir->Connect( positionMinusViewPosition, "Result" );

        // normDir = normalize( VS_Dir )
        MGNormalizeNode * normDir = graph->AddNode< MGNormalizeNode >();
        normDir->Value->Connect( materialVertexStage, "Dir" );

        MGTextureSlot * skyTexture = graph->AddNode< MGTextureSlot >();
        skyTexture->Filter = TEXTURE_FILTER_LINEAR;
        skyTexture->TextureType = TEXTURE_CUBEMAP;

        // color = texture( skyTexture, normDir );
        MGSampler * color = graph->AddNode< MGSampler >();
        color->TexCoord->Connect( normDir, "Result" );
        color->TextureSlot->Connect( skyTexture, "Value" );

        MGFragmentStage * materialFragmentStage = graph->AddNode< MGFragmentStage >();
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
