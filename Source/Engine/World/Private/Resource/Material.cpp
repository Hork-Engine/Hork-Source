/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/Resource/Material.h>
#include <World/Public/Resource/Asset.h>
#include <World/Public/Base/ResourceManager.h>
#include <World/Public/MaterialGraph/MaterialGraph.h>
#include <World/Public/EngineInstance.h>
#include <Core/Public/Logger.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Runtime/Public/Runtime.h>

AN_CLASS_META( AMaterial )
AN_CLASS_META( AMaterialInstance )

static AMaterial * GMaterials = nullptr, * GMaterialsTail = nullptr;

AMaterial::AMaterial() {
    INTRUSIVE_ADD( this, pNext, pPrev, GMaterials, GMaterialsTail );
}

AMaterial::~AMaterial() {
    INTRUSIVE_REMOVE( this, pNext, pPrev, GMaterials, GMaterialsTail );
}

static void CreateMaterial( AMaterialGPU * Material, SMaterialDef const * Def )
{
    GEngine->GetRenderBackend()->InitializeMaterial( Material, Def );
}

void AMaterial::RebuildMaterials() {
    for ( AMaterial * material = GMaterials ; material ; material = material->pNext ) {
        CreateMaterial( &material->MaterialGPU, &material->Def );
    }
}

void AMaterial::Initialize( MGMaterialGraph * Graph ) {
    CompileMaterialGraph( Graph, &Def );

    CreateMaterial( &MaterialGPU, &Def );
}

void AMaterial::Purge() {
    Def.RemoveShaders();
}

bool AMaterial::LoadResource( IBinaryStream & Stream ) {
    uint32_t fileFormat = Stream.ReadUInt32();

    if ( fileFormat != FMT_VERSION_MATERIAL ) {
        GLogger.Printf( "Expected file format %d\n", FMT_VERSION_MATERIAL );
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if ( fileVersion != FMT_VERSION_MATERIAL ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_MATERIAL );
        return false;
    }

    Purge();

    AString guid;

    Stream.ReadObject( guid );

    Def.Type = (EMaterialType)Stream.ReadUInt8();
    Def.Blending = (EColorBlending)Stream.ReadUInt8();
    Def.TessellationMethod = (ETessellationMethod)Stream.ReadUInt8();
    Def.RenderingPriority = (ERenderingPriority)Stream.ReadUInt8();
    Def.LightmapSlot = (ETessellationMethod)Stream.ReadUInt16();
    Def.DepthPassTextureCount = Stream.ReadUInt8();
    Def.LightPassTextureCount = Stream.ReadUInt8();
    Def.WireframePassTextureCount = Stream.ReadUInt8();
    Def.NormalsPassTextureCount = Stream.ReadUInt8();
    Def.ShadowMapPassTextureCount = Stream.ReadUInt8();
    Def.bHasVertexDeform = Stream.ReadBool();
    Def.bDepthTest_EXPERIMENTAL = Stream.ReadBool();
    Def.bNoCastShadow = Stream.ReadBool();
    Def.bAlphaMasking = Stream.ReadBool();
    Def.bShadowMapMasking = Stream.ReadBool();
    Def.bDisplacementAffectShadow = Stream.ReadBool();
    //Def.bParallaxMappingSelfShadowing = Stream.ReadBool(); // TODO
    Def.bTranslucent = Stream.ReadBool();
    Def.bTwoSided = Stream.ReadBool();
    Def.NumUniformVectors = Stream.ReadUInt8();
    Def.NumSamplers = Stream.ReadUInt8();
    for ( int i = 0 ; i < Def.NumSamplers ; i++ ) {
        Def.Samplers[i].TextureType = (ETextureType)Stream.ReadUInt8();
        Def.Samplers[i].Filter = (ETextureFilter)Stream.ReadUInt8();
        Def.Samplers[i].AddressU = (ETextureAddress)Stream.ReadUInt8();
        Def.Samplers[i].AddressV = (ETextureAddress)Stream.ReadUInt8();
        Def.Samplers[i].AddressW = (ETextureAddress)Stream.ReadUInt8();
        Def.Samplers[i].MipLODBias = Stream.ReadFloat();
        Def.Samplers[i].Anisotropy = Stream.ReadFloat();
        Def.Samplers[i].MinLod = Stream.ReadFloat();
        Def.Samplers[i].MaxLod = Stream.ReadFloat();
    }

    int numShaders = Stream.ReadUInt16();
    AString sourceName, sourceCode;
    for ( int i = 0 ; i < numShaders ; i++ ) {
        Stream.ReadObject( sourceName );
        Stream.ReadObject( sourceCode );
        Def.AddShader( sourceName.CStr(), sourceCode );
    }

    CreateMaterial( &MaterialGPU, &Def );

    return true;
}

bool WriteMaterial( AString const & _Path, SMaterialDef const * pDef ) {
    AFileStream f;

    if ( !f.OpenWrite( _Path ) ) {
        return false;
    }

    //AString guid;
    AGUID guid;
    guid.Generate();

    f.WriteUInt32( FMT_VERSION_MATERIAL );
    f.WriteUInt32( FMT_VERSION_MATERIAL );
    f.WriteObject( guid.ToString() );
    f.WriteUInt8( pDef->Type );
    f.WriteUInt8( pDef->Blending );
    f.WriteUInt8( pDef->RenderingPriority );
    f.WriteUInt8( pDef->TessellationMethod );
    f.WriteUInt16( pDef->LightmapSlot );
    f.WriteUInt8( pDef->DepthPassTextureCount );
    f.WriteUInt8( pDef->LightPassTextureCount );
    f.WriteUInt8( pDef->WireframePassTextureCount );
    f.WriteUInt8( pDef->NormalsPassTextureCount );
    f.WriteUInt8( pDef->ShadowMapPassTextureCount );
    f.WriteBool( pDef->bHasVertexDeform );
    f.WriteBool( pDef->bDepthTest_EXPERIMENTAL );
    f.WriteBool( pDef->bNoCastShadow );
    f.WriteBool( pDef->bAlphaMasking );
    f.WriteBool( pDef->bShadowMapMasking );
    f.WriteBool( pDef->bDisplacementAffectShadow );
    //f.WriteBool( pDef->bParallaxMappingSelfShadowing ); // TODO
    f.WriteBool( pDef->bTranslucent );
    f.WriteBool( pDef->bTwoSided );
    f.WriteUInt8( pDef->NumUniformVectors );
    f.WriteUInt8( pDef->NumSamplers );
    for ( int i = 0 ; i < pDef->NumSamplers ; i++ ) {
         f.WriteUInt8( pDef->Samplers[i].TextureType );
         f.WriteUInt8( pDef->Samplers[i].Filter );
         f.WriteUInt8( pDef->Samplers[i].AddressU );
         f.WriteUInt8( pDef->Samplers[i].AddressV );
         f.WriteUInt8( pDef->Samplers[i].AddressW );
         f.WriteFloat( pDef->Samplers[i].MipLODBias );
         f.WriteFloat( pDef->Samplers[i].Anisotropy );
         f.WriteFloat( pDef->Samplers[i].MinLod );
         f.WriteFloat( pDef->Samplers[i].MaxLod );
    }

    int numShaders = 0;
    for ( SMaterialShader * s = pDef->Shaders ; s ; s = s->Next ) {
        numShaders++;
    }

    f.WriteUInt16( numShaders );

    for ( SMaterialShader * s = pDef->Shaders ; s ; s = s->Next ) {
        f.WriteObject( AString( s->SourceName ) );
        f.WriteObject( AString( s->Code ) );
    }

    return true;
}

void AMaterial::LoadInternalResource( const char * _Path ) {
    if ( !Core::Stricmp( _Path, "/Default/Materials/Unlit" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        graph->Color->Connect( textureSampler, "RGBA" );

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->RegisterTextureSlot( diffuseTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/UnlitMask" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        graph->Color->Connect( textureSampler->RGBA );
        graph->AlphaMask->Connect( textureSampler->A );
        graph->bTwoSided = true;

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->RegisterTextureSlot( diffuseTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/UnlitOpacity" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        graph->Color->Connect( textureSampler->RGBA );
        graph->Opacity->Connect( textureSampler->A );

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->RegisterTextureSlot( diffuseTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/BaseLight" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        graph->Color->Connect( textureSampler, "RGBA" );

        graph->MaterialType = MATERIAL_TYPE_BASELIGHT;
        graph->RegisterTextureSlot( diffuseTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/DefaultPBR" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

        MGTextureSlot * diffuseTexture = graph->AddNode< MGTextureSlot >();
        diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * metallicTexture = graph->AddNode< MGTextureSlot >();
        metallicTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * normalTexture = graph->AddNode< MGTextureSlot >();
        normalTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGTextureSlot * roughnessTexture = graph->AddNode< MGTextureSlot >();
        roughnessTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;

        MGSampler * textureSampler = graph->AddNode< MGSampler >();
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicSampler = graph->AddNode< MGSampler >();
        metallicSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicSampler->TextureSlot->Connect( metallicTexture, "Value" );

        MGSampler * roughnessSampler = graph->AddNode< MGSampler >();
        roughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        roughnessSampler->TextureSlot->Connect( roughnessTexture, "Value" );

        graph->Color->Connect( textureSampler, "RGBA" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicSampler, "R" );
        graph->Roughness->Connect( roughnessSampler, "R" );

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( roughnessTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughness" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

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
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( inTexCoord, "Value" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( inTexCoord, "Value" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        graph->Color->Connect( textureSampler, "RGBA" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicRoughnessSampler, "B" );
        graph->Roughness->Connect( metallicRoughnessSampler, "G" );
        graph->AmbientOcclusion->Connect( ambientSampler, "R" );
        graph->Emissive->Connect( emissiveSampler, "RGBA" );

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicRoughnessTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( ambientTexture );
        graph->RegisterTextureSlot( emissiveTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughnessMask" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

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
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( inTexCoord, "Value" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( inTexCoord, "Value" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        graph->Color->Connect( textureSampler, "RGBA" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicRoughnessSampler, "B" );
        graph->Roughness->Connect( metallicRoughnessSampler, "G" );
        graph->AmbientOcclusion->Connect( ambientSampler, "R" );
        graph->Emissive->Connect( emissiveSampler, "RGBA" );
        graph->AlphaMask->Connect( textureSampler->A );
        graph->bTwoSided = true;

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicRoughnessTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( ambientTexture );
        graph->RegisterTextureSlot( emissiveTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughnessOpacity" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

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
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( inTexCoord, "Value" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( inTexCoord, "Value" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        graph->Color->Connect( textureSampler, "RGBA" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicRoughnessSampler, "B" );
        graph->Roughness->Connect( metallicRoughnessSampler, "G" );
        graph->AmbientOcclusion->Connect( ambientSampler, "R" );
        graph->Emissive->Connect( emissiveSampler, "RGBA" );
        graph->Opacity->Connect( textureSampler->A );

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicRoughnessTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( ambientTexture );
        graph->RegisterTextureSlot( emissiveTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughnessFactor" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

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
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( inTexCoord, "Value" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( inTexCoord, "Value" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        MGUniformAddress * baseColorFactor = graph->AddNode< MGUniformAddress >();
        baseColorFactor->Type = AT_Float4;
        baseColorFactor->Address = 0;

        MGUniformAddress * metallicFactor = graph->AddNode< MGUniformAddress >();
        metallicFactor->Type = AT_Float1;
        metallicFactor->Address = 4;

        MGUniformAddress * roughnessFactor = graph->AddNode< MGUniformAddress >();
        roughnessFactor->Type = AT_Float1;
        roughnessFactor->Address = 5;

        MGUniformAddress * emissiveFactor = graph->AddNode< MGUniformAddress >();
        emissiveFactor->Type = AT_Float3;
        emissiveFactor->Address = 8;

        MGMulNode * colorMul = graph->AddNode< MGMulNode >();
        colorMul->ValueA->Connect( textureSampler, "RGBA" );
        colorMul->ValueB->Connect( baseColorFactor, "Value" );

        MGMulNode * metallicMul = graph->AddNode< MGMulNode >();
        metallicMul->ValueA->Connect( metallicRoughnessSampler, "B" );
        metallicMul->ValueB->Connect( metallicFactor, "Value" );

        MGMulNode * roughnessMul = graph->AddNode< MGMulNode >();
        roughnessMul->ValueA->Connect( metallicRoughnessSampler, "G" );
        roughnessMul->ValueB->Connect( roughnessFactor, "Value" );

        MGMulNode * emissiveMul = graph->AddNode< MGMulNode >();
        emissiveMul->ValueA->Connect( emissiveSampler, "RGB" );
        emissiveMul->ValueB->Connect( emissiveFactor, "Value" );

        graph->Color->Connect( colorMul, "Result" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicMul, "Result" );
        graph->Roughness->Connect( roughnessMul, "Result" );
        graph->AmbientOcclusion->Connect( ambientSampler, "R" );
        graph->Emissive->Connect( emissiveMul, "Result" );

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicRoughnessTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( ambientTexture );
        graph->RegisterTextureSlot( emissiveTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughnessFactorMask" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

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
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( inTexCoord, "Value" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( inTexCoord, "Value" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        MGUniformAddress * baseColorFactor = graph->AddNode< MGUniformAddress >();
        baseColorFactor->Type = AT_Float4;
        baseColorFactor->Address = 0;

        MGUniformAddress * metallicFactor = graph->AddNode< MGUniformAddress >();
        metallicFactor->Type = AT_Float1;
        metallicFactor->Address = 4;

        MGUniformAddress * roughnessFactor = graph->AddNode< MGUniformAddress >();
        roughnessFactor->Type = AT_Float1;
        roughnessFactor->Address = 5;

        MGUniformAddress * emissiveFactor = graph->AddNode< MGUniformAddress >();
        emissiveFactor->Type = AT_Float3;
        emissiveFactor->Address = 8;

        MGMulNode * colorMul = graph->AddNode< MGMulNode >();
        colorMul->ValueA->Connect( textureSampler, "RGBA" );
        colorMul->ValueB->Connect( baseColorFactor, "Value" );

        MGMulNode * metallicMul = graph->AddNode< MGMulNode >();
        metallicMul->ValueA->Connect( metallicRoughnessSampler, "B" );
        metallicMul->ValueB->Connect( metallicFactor, "Value" );

        MGMulNode * roughnessMul = graph->AddNode< MGMulNode >();
        roughnessMul->ValueA->Connect( metallicRoughnessSampler, "G" );
        roughnessMul->ValueB->Connect( roughnessFactor, "Value" );

        MGMulNode * emissiveMul = graph->AddNode< MGMulNode >();
        emissiveMul->ValueA->Connect( emissiveSampler, "RGB" );
        emissiveMul->ValueB->Connect( emissiveFactor, "Value" );

        graph->Color->Connect( colorMul, "Result" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicMul, "Result" );
        graph->Roughness->Connect( roughnessMul, "Result" );
        graph->AmbientOcclusion->Connect( ambientSampler, "R" );
        graph->Emissive->Connect( emissiveMul, "Result" );
        graph->AlphaMask->Connect( textureSampler->A );
        graph->bTwoSided = true;

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicRoughnessTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( ambientTexture );
        graph->RegisterTextureSlot( emissiveTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughnessFactorOpacity" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInTexCoord * inTexCoord = graph->AddNode< MGInTexCoord >();

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
        textureSampler->TexCoord->Connect( inTexCoord, "Value" );
        textureSampler->TextureSlot->Connect( diffuseTexture, "Value" );

        MGNormalSampler * normalSampler = graph->AddNode< MGNormalSampler >();
        normalSampler->TexCoord->Connect( inTexCoord, "Value" );
        normalSampler->TextureSlot->Connect( normalTexture, "Value" );
        normalSampler->Compression = NM_XYZ;

        MGSampler * metallicRoughnessSampler = graph->AddNode< MGSampler >();
        metallicRoughnessSampler->TexCoord->Connect( inTexCoord, "Value" );
        metallicRoughnessSampler->TextureSlot->Connect( metallicRoughnessTexture, "Value" );

        MGSampler * ambientSampler = graph->AddNode< MGSampler >();
        ambientSampler->TexCoord->Connect( inTexCoord, "Value" );
        ambientSampler->TextureSlot->Connect( ambientTexture, "Value" );

        MGSampler * emissiveSampler = graph->AddNode< MGSampler >();
        emissiveSampler->TexCoord->Connect( inTexCoord, "Value" );
        emissiveSampler->TextureSlot->Connect( emissiveTexture, "Value" );

        MGUniformAddress * baseColorFactor = graph->AddNode< MGUniformAddress >();
        baseColorFactor->Type = AT_Float4;
        baseColorFactor->Address = 0;

        MGUniformAddress * metallicFactor = graph->AddNode< MGUniformAddress >();
        metallicFactor->Type = AT_Float1;
        metallicFactor->Address = 4;

        MGUniformAddress * roughnessFactor = graph->AddNode< MGUniformAddress >();
        roughnessFactor->Type = AT_Float1;
        roughnessFactor->Address = 5;

        MGUniformAddress * emissiveFactor = graph->AddNode< MGUniformAddress >();
        emissiveFactor->Type = AT_Float3;
        emissiveFactor->Address = 8;

        MGMulNode * colorMul = graph->AddNode< MGMulNode >();
        colorMul->ValueA->Connect( textureSampler, "RGBA" );
        colorMul->ValueB->Connect( baseColorFactor, "Value" );

        MGMulNode * metallicMul = graph->AddNode< MGMulNode >();
        metallicMul->ValueA->Connect( metallicRoughnessSampler, "B" );
        metallicMul->ValueB->Connect( metallicFactor, "Value" );

        MGMulNode * roughnessMul = graph->AddNode< MGMulNode >();
        roughnessMul->ValueA->Connect( metallicRoughnessSampler, "G" );
        roughnessMul->ValueB->Connect( roughnessFactor, "Value" );

        MGMulNode * emissiveMul = graph->AddNode< MGMulNode >();
        emissiveMul->ValueA->Connect( emissiveSampler, "RGB" );
        emissiveMul->ValueB->Connect( emissiveFactor, "Value" );

        graph->Color->Connect( colorMul, "Result" );
        graph->Normal->Connect( normalSampler, "XYZ" );
        graph->Metallic->Connect( metallicMul, "Result" );
        graph->Roughness->Connect( roughnessMul, "Result" );
        graph->AmbientOcclusion->Connect( ambientSampler, "R" );
        graph->Emissive->Connect( emissiveMul, "Result" );
        graph->Opacity->Connect( textureSampler->A );

        graph->MaterialType = MATERIAL_TYPE_PBR;
        graph->RegisterTextureSlot( diffuseTexture );
        graph->RegisterTextureSlot( metallicRoughnessTexture );
        graph->RegisterTextureSlot( normalTexture );
        graph->RegisterTextureSlot( ambientTexture );
        graph->RegisterTextureSlot( emissiveTexture );

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/PBRMetallicRoughnessNoTex" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGUniformAddress * baseColorFactor = graph->AddNode< MGUniformAddress >();
        baseColorFactor->Type = AT_Float4;
        baseColorFactor->Address = 0;

        MGUniformAddress * metallicFactor = graph->AddNode< MGUniformAddress >();
        metallicFactor->Type = AT_Float1;
        metallicFactor->Address = 4;

        MGUniformAddress * roughnessFactor = graph->AddNode< MGUniformAddress >();
        roughnessFactor->Type = AT_Float1;
        roughnessFactor->Address = 5;

        MGUniformAddress * emissiveFactor = graph->AddNode< MGUniformAddress >();
        emissiveFactor->Type = AT_Float3;
        emissiveFactor->Address = 8;

        graph->Color->Connect( baseColorFactor, "Value" );
        graph->Metallic->Connect( metallicFactor, "Value" );
        graph->Roughness->Connect( roughnessFactor, "Value" );
        graph->Emissive->Connect( emissiveFactor, "Value" );

        graph->MaterialType = MATERIAL_TYPE_PBR;

        Initialize( graph );
        return;
    }

    if ( !Core::Stricmp( _Path, "/Default/Materials/Skybox" ) ) {
        MGMaterialGraph * graph = CreateInstanceOf< MGMaterialGraph >();

        MGInPosition * inPosition = graph->AddNode< MGInPosition >();

        MGTextureSlot * cubemapTexture = graph->AddNode< MGTextureSlot >();
        cubemapTexture->SamplerDesc.TextureType = TEXTURE_CUBEMAP;
        cubemapTexture->SamplerDesc.Filter = TEXTURE_FILTER_LINEAR;
        cubemapTexture->SamplerDesc.AddressU =
        cubemapTexture->SamplerDesc.AddressV =
        cubemapTexture->SamplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;

        MGSampler * cubemapSampler = graph->AddNode< MGSampler >();
        cubemapSampler->TexCoord->Connect( inPosition, "Value" );
        cubemapSampler->TextureSlot->Connect( cubemapTexture, "Value" );

        graph->Color->Connect( cubemapSampler, "RGBA" );

        graph->MaterialType = MATERIAL_TYPE_UNLIT;
        graph->DepthHack = MATERIAL_DEPTH_HACK_SKYBOX;
        graph->RegisterTextureSlot( cubemapTexture );

        Initialize( graph );
        return;
    }

    GLogger.Printf( "Unknown internal material %s\n", _Path );

    //LoadInternalResource( "/Default/Materials/Unlit" );
    LoadInternalResource( "/Default/Materials/BaseLight" );
}

AMaterialInstance::AMaterialInstance() {
    static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/Unlit" ) );
    //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Default/Textures/Default2D" ) );
    //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/gridyblack.png" ) );
    //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/uv_checker.png" ) );
    static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/grid8.png" ) );

    Material = MaterialResource.GetObject();

    SetTexture( 0, TextureResource.GetObject() );
}

void AMaterialInstance::LoadInternalResource( const char * _Path ) {
#if 0
    if ( !Core::Stricmp( _Path, "/Default/MaterialInstance/Default" ) )
    {
        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/Unlit" ) );
        //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Default/Textures/Default2D" ) );
        //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/gridyblack.png" ) );
        //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/uv_checker.png" ) );
        static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/grid8.png" ) );

        Material = MaterialResource.GetObject();

        SetTexture( 0, TextureResource.GetObject() );
        return;
    }
#endif
    if ( !Core::Stricmp( _Path, "/Default/MaterialInstance/BaseLight" ) || !Core::Stricmp( _Path, "/Default/MaterialInstance/Default" ) ) {
        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/BaseLight" ) );
        //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Default/Textures/Default2D" ) );
        //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/gridyblack.png" ) );
        //static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/uv_checker.png" ) );
        static TStaticResourceFinder< ATexture > TextureResource( _CTS( "/Common/grid8.png" ) );

        Material = MaterialResource.GetObject();

        SetTexture( 0, TextureResource.GetObject() );
        return;
    }
    if ( !Core::Stricmp( _Path, "/Default/MaterialInstance/Metal" ) )
    {
        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/PBRMetallicRoughnessNoTex" ) );

        Material = MaterialResource.GetObject();

        // Base color
        UniformVectors[0] = Float4(0.8f,0.8f,0.8f,1.0f);
        // Metallic
        UniformVectors[1].X = 1;
        // Roughness
        UniformVectors[1].Y = 0.5f;
        // Emissive
        UniformVectors[2] = Float4(0.0f);
        return;
    }
    if ( !Core::Stricmp( _Path, "/Default/MaterialInstance/Dielectric" ) )
    {
        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/PBRMetallicRoughnessNoTex" ) );

        Material = MaterialResource.GetObject();

        // Base color
        UniformVectors[0] = Float4(0.8f,0.8f,0.8f,1.0f);
        // Metallic
        UniformVectors[1].X = 0.0f;
        // Roughness
        UniformVectors[1].Y = 0.5f;
        // Emissive
        UniformVectors[2] = Float4(0.0f);
        return;
    }
    GLogger.Printf( "Unknown internal material instance %s\n", _Path );

    LoadInternalResource( "/Default/MaterialInstance/Default" );
}

bool AMaterialInstance::LoadResource( IBinaryStream & Stream ) {
    uint32_t fileFormat;
    uint32_t fileVersion;

    fileFormat = Stream.ReadUInt32();

    if ( fileFormat != FMT_FILE_TYPE_MATERIAL_INSTANCE ) {
        //GLogger.Printf( "Expected file format %d\n", FMT_FILE_TYPE_MATERIAL_INSTANCE );

        Stream.Rewind();
        return LoadTextVersion( Stream );
    }

    fileVersion = Stream.ReadUInt32();

    if ( fileVersion != FMT_VERSION_MATERIAL_INSTANCE ) {
        GLogger.Printf( "Expected file version %d\n", FMT_VERSION_MATERIAL_INSTANCE );
        return false;
    }

    AString guidStr;
    AString materialGUID;
    AString textureGUID;

    Stream.ReadObject( guidStr );
    Stream.ReadObject( materialGUID );

    int texCount = Stream.ReadUInt32();
    for ( int i = 0 ; i < texCount ; i++ ) {
        Stream.ReadObject( textureGUID );

        SetTexture( i, GetOrCreateResource< ATexture >( textureGUID.CStr() ) );
    }

    for ( int i = texCount ; i < MAX_MATERIAL_TEXTURES ; i++ ) {
        SetTexture( i, nullptr );
    }

    for ( int i = 0 ; i < MAX_MATERIAL_UNIFORMS ; i++ ) {
        Uniforms[i] = Stream.ReadFloat();
    }

    SetMaterial( GetOrCreateResource< AMaterial >( materialGUID.CStr() ) );

    return true;
}

bool AMaterialInstance::LoadTextVersion( IBinaryStream & Stream ) {
    AString text;
    text.FromFile( Stream );

    SDocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = text.CStr();
    deserializeInfo.bInsitu = true;

    ADocument doc;
    doc.DeserializeFromString( deserializeInfo );

    ADocMember * member;

    member = doc.FindMember( "Textures" );
    if ( member ) {
        ADocValue * values = member->GetArrayValues();
        int texSlot = 0;
        for ( ADocValue * v = values ; v && texSlot < MAX_MATERIAL_TEXTURES ; v = v->GetNext() ) {
            SetTexture( texSlot++, GetOrCreateResource< ATexture >( v->GetString().CStr() ) );
        }
    }

    member = doc.FindMember( "Uniforms" );
    if ( member ) {
        ADocValue * values = member->GetArrayValues();
        int uniformNum = 0;
        for ( ADocValue * v = values ; v && uniformNum < MAX_MATERIAL_UNIFORMS ; v = v->GetNext() ) {
            Uniforms[uniformNum++] = Math::ToFloat( v->GetString() );
        }
    }

    member = doc.FindMember( "Material" );
    SetMaterial( GetOrCreateResource< AMaterial >( member ? member->GetString().CStr() : "/Default/Materials/Unlit" ) );

    return true;
}

void AMaterialInstance::SetMaterial( AMaterial * _Material ) {
    if ( !_Material ) {
        static TStaticResourceFinder< AMaterial > MaterialResource( _CTS( "/Default/Materials/Unlit" ) );

        Material = MaterialResource.GetObject();
    } else {
        Material = _Material;
    }
}

AMaterial * AMaterialInstance::GetMaterial() const {
    return Material;
}

void AMaterialInstance::SetTexture( int _TextureSlot, ATexture * _Texture ) {
    if ( _TextureSlot >= MAX_MATERIAL_TEXTURES ) {
        return;
    }
    Textures[_TextureSlot] = _Texture;
}

ATexture * AMaterialInstance::GetTexture( int _TextureSlot ) {
    return Textures[_TextureSlot];
}

void AMaterialInstance::SetVirtualTexture( AVirtualTextureResource * VirtualTex ) {
    VirtualTexture = VirtualTex;
}

SMaterialFrameData * AMaterialInstance::PreRenderUpdate( int _FrameNumber ) {
    if ( VisFrame == _FrameNumber ) {
        return FrameData;
    }

    VisFrame = _FrameNumber;

    FrameData = ( SMaterialFrameData * )GRuntime->AllocFrameMem( sizeof( SMaterialFrameData ) );

    FrameData->Material = Material->GetGPUResource();

    RenderCore::ITexture ** textures = FrameData->Textures;
    FrameData->NumTextures = 0;

    for ( int i = 0; i < MAX_MATERIAL_TEXTURES; i++ ) {
        if ( Textures[ i ] ) {
            textures[ i ] = Textures[ i ]->GetGPUResource();
            FrameData->NumTextures = i + 1;
        } else {
            textures[ i ] = 0;
        }
    }

    FrameData->NumUniformVectors = Material->GetNumUniformVectors();
    Core::Memcpy( FrameData->UniformVectors, UniformVectors, sizeof( Float4 )*FrameData->NumUniformVectors );

    FrameData->VirtualTexture = VirtualTexture.GetObject();

    return FrameData;
}
