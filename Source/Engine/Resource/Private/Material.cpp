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

AN_CLASS_META( FMaterial )
AN_CLASS_META( FMaterialInstance )

FMaterial::FMaterial() {
    RenderProxy = FRenderProxy::NewProxy< FRenderProxy_Material >();
}

FMaterial::~FMaterial() {
    RenderProxy->KillProxy();
}

void FMaterial::Initialize( FMaterialBuildData const * _Data ) {
    FRenderFrame * frameData = GRuntime.GetFrameData();

    NumUniformVectors = _Data->NumUniformVectors;

    FMaterialBuildData * data = ( FMaterialBuildData * )frameData->AllocFrameData( _Data->Size );

    if ( data ) {
        memcpy( data, _Data, _Data->Size );

        Type = data->Type;

        RenderProxy->Data = data;
        RenderProxy->MarkUpdated();
    }
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
        FMaterialBuildData * buildData = builder->BuildData();
        Initialize( buildData );
        GMainMemoryZone.Dealloc( buildData );
        //SetName( _InternalResourceName );
        return;
    }
    GLogger.Printf( "Unknown internal material %s\n", _InternalResourceName );
}

FMaterialInstance::FMaterialInstance() {
    static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.Default" ) );
    //static TStaticInternalResourceFinder< FTexture > TextureResource( _CTS( "FTexture.Default" ) );
    //static TStaticResourceFinder< FTexture > TextureResource( _CTS( "gridyblack.png" ) );
    static TStaticResourceFinder< FTexture > TextureResource( _CTS( "uv_checker.png" ) );

    Material = MaterialResource.GetObject();

    SetTexture( 0, TextureResource.GetObject() );
}

void FMaterialInstance::InitializeInternalResource( const char * _InternalResourceName ) {
    if ( !FString::Icmp( _InternalResourceName, "FMaterialInstance.Default" ) )
    {
        static TStaticInternalResourceFinder< FMaterial > MaterialResource( _CTS( "FMaterial.Default" ) );
        //static TStaticInternalResourceFinder< FTexture > TextureResource( _CTS( "FTexture.Default" ) );
        //static TStaticResourceFinder< FTexture > TextureResource( _CTS( "gridyblack.png" ) );
        static TStaticResourceFinder< FTexture > TextureResource( _CTS( "uv_checker.png" ) );

        Material = MaterialResource.GetObject();

        SetTexture( 0, TextureResource.GetObject() );
        //SetName( _InternalResourceName );
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

FMaterialInstanceFrameData * FMaterialInstance::RenderFrontend_Update( int _VisMarker ) {
    if ( VisMarker == _VisMarker ) {
        return FrameData;
    }

    VisMarker = _VisMarker;

    FrameData = ( FMaterialInstanceFrameData * )GRuntime.GetFrameData()->AllocFrameData( sizeof( FMaterialInstanceFrameData ) );
    if ( !FrameData ) {
        return nullptr;
    }

    FrameData->Material = Material->GetRenderProxy();

    FRenderProxy_Texture ** textures = FrameData->Textures;
    FrameData->NumTextures = 0;

    for ( int i = 0; i < MAX_MATERIAL_TEXTURES; i++ ) {
        if ( Textures[ i ] ) {

            FRenderProxy_Texture * textureProxy = Textures[ i ]->GetRenderProxy();

            if ( textureProxy->IsSubmittedToRenderThread() ) {
                textures[ i ] = textureProxy;
                FrameData->NumTextures = i + 1;
            } else {
                textures[ i ] = 0;
            }
        } else {
            textures[ i ] = 0;
        }
    }

    FrameData->NumUniformVectors = Material->GetNumUniformVectors();
    memcpy( FrameData->UniformVectors, UniformVectors, sizeof( Float4 )*FrameData->NumUniformVectors );

    return FrameData;
}
