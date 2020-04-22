/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "OpenGL45FrameResources.h"
#include "OpenGL45EnvProbeGenerator.h"

#include <Core/Public/Image.h>
#include <Runtime/Public/RuntimeVariable.h>

ARuntimeVariable RVDebugRenderMode( _CTS( "DebugRenderMode" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVPostprocessBloomScale( _CTS( "PostprocessBloomScale" ), _CTS( "1" ) );
ARuntimeVariable RVPostprocessBloom( _CTS( "PostprocessBloom" ), _CTS( "1" ) );
ARuntimeVariable RVPostprocessToneExposure( _CTS( "PostprocessToneExposure" ), _CTS( "0.05" ) );
ARuntimeVariable RVBrightness( _CTS( "Brightness" ), _CTS( "1" ) );
extern ARuntimeVariable RVFxaa;

using namespace GHI;

namespace OpenGL45 {

AFrameResources GFrameResources;

void AFrameResources::Initialize() {
    GHI::BufferCreateInfo uniformBufferCI = {};
    uniformBufferCI.bImmutableStorage = true;
    uniformBufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;

    uniformBufferCI.SizeInBytes = 2 * MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES * sizeof( Float4x4 );
    CascadeViewProjectionBuffer.Initialize( uniformBufferCI );

    uniformBufferCI.SizeInBytes = sizeof( SViewUniformBuffer );
    ViewUniformBuffer.Initialize( uniformBufferCI );

    InstanceUniformBufferSize = 1024;
    InstanceUniformBufferSizeof = Align( sizeof( SInstanceUniformBuffer ), GDevice.GetUniformBufferOffsetAlignment() );

    ShadowInstanceUniformBufferSize = 1024;
    ShadowInstanceUniformBufferSizeof = Align( sizeof( SShadowInstanceUniformBuffer ), GDevice.GetUniformBufferOffsetAlignment() );

    GHI::BufferCreateInfo streamBufferCI = {};
    streamBufferCI.bImmutableStorage = false;
    streamBufferCI.MutableClientAccess = MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    streamBufferCI.MutableUsage = MUTABLE_STORAGE_STREAM;
    streamBufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
    streamBufferCI.SizeInBytes = InstanceUniformBufferSize * InstanceUniformBufferSizeof;
    InstanceUniformBuffer.Initialize( streamBufferCI );
    streamBufferCI.SizeInBytes = ShadowInstanceUniformBufferSize * ShadowInstanceUniformBufferSizeof;
    ShadowInstanceUniformBuffer.Initialize( streamBufferCI );

    {
        GHI::TextureStorageCreateInfo createInfo = {};
        createInfo.Type = GHI::TEXTURE_3D;
        createInfo.InternalFormat = GHI::INTERNAL_PIXEL_FORMAT_RG32UI;
        createInfo.Resolution.Tex3D.Width = MAX_FRUSTUM_CLUSTERS_X;
        createInfo.Resolution.Tex3D.Height = MAX_FRUSTUM_CLUSTERS_Y;
        createInfo.Resolution.Tex3D.Depth = MAX_FRUSTUM_CLUSTERS_Z;
        createInfo.NumLods = 1;
        ClusterLookup.InitializeStorage( createInfo );

        GHI::SamplerCreateInfo samplerCI = {};
        samplerCI.SetDefaults();
        samplerCI.Filter = FILTER_NEAREST;
        samplerCI.AddressU = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressV = SAMPLER_ADDRESS_CLAMP;
        samplerCI.AddressW = SAMPLER_ADDRESS_CLAMP;
        ClusterLookupSampler = GDevice.GetOrCreateSampler( samplerCI );
    }

    {
        GHI::BufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = sizeof( SFrameLightData::ItemBuffer );
        ClusterItemBuffer.Initialize( bufferCI );
        ClusterItemTBO.InitializeTextureBuffer( GHI::BUFFER_DATA_UINT1, ClusterItemBuffer );
    }

    {
        GHI::BufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = sizeof( SFrameLightData::LightBuffer );
        LightBuffer.Initialize( bufferCI );
    }

    {
        constexpr Float2 saqVertices[4] = {
            { Float2( -1.0f,  1.0f ) },
            { Float2(  1.0f,  1.0f ) },
            { Float2( -1.0f, -1.0f ) },
            { Float2(  1.0f, -1.0f ) }
        };

        BufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.SizeInBytes = sizeof( saqVertices );
        Saq.Initialize( bufferCI, saqVertices );
    }

    Core::ZeroMem( BufferBinding, sizeof( BufferBinding ) );
    Core::ZeroMem( TextureBindings, sizeof( TextureBindings ) );
    Core::ZeroMem( SamplerBindings, sizeof( SamplerBindings ) );

    ViewUniformBufferBinding = &BufferBinding[0];
    ViewUniformBufferBinding->BufferType = UNIFORM_BUFFER;
    ViewUniformBufferBinding->SlotIndex = 0;
    ViewUniformBufferBinding->pBuffer = &ViewUniformBuffer;

    InstanceUniformBufferBinding = &BufferBinding[1];
    InstanceUniformBufferBinding->BufferType = UNIFORM_BUFFER;
    InstanceUniformBufferBinding->SlotIndex = 1;
    InstanceUniformBufferBinding->pBuffer = nullptr;

    SkeletonBufferBinding = &BufferBinding[2];
    SkeletonBufferBinding->BufferType = UNIFORM_BUFFER;
    SkeletonBufferBinding->SlotIndex = 2;
    SkeletonBufferBinding->pBuffer = nullptr;

    CascadeBufferBinding = &BufferBinding[3];
    CascadeBufferBinding->BufferType = UNIFORM_BUFFER;
    CascadeBufferBinding->SlotIndex = 3;
    CascadeBufferBinding->pBuffer = &CascadeViewProjectionBuffer;

    LightBufferBinding = &BufferBinding[4];
    LightBufferBinding->BufferType = UNIFORM_BUFFER;
    LightBufferBinding->SlotIndex = 4;
    LightBufferBinding->pBuffer = &LightBuffer;

    for ( int i = 0 ; i < 16 ; i++ ) {
        TextureBindings[i].SlotIndex = i;
        SamplerBindings[i].SlotIndex = i;
    }

    Core::ZeroMem( &Resources, sizeof( Resources ) );
    Resources.Buffers = BufferBinding;
    Resources.NumBuffers = AN_ARRAY_SIZE( BufferBinding );

    Resources.Textures = TextureBindings;
    Resources.NumTextures = AN_ARRAY_SIZE( TextureBindings );

    Resources.Samplers = SamplerBindings;
    Resources.NumSamplers = AN_ARRAY_SIZE( SamplerBindings );


    

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////
    {
    const char * Cubemap[6] = {
        "ClearSky/rt.bmp",
        "ClearSky/lt.bmp",
        "ClearSky/up.bmp",
        "ClearSky/dn.bmp",
        "ClearSky/bk.bmp",
        "ClearSky/ft.bmp"
    };
    const char * Cubemap2[6] = {
        "DarkSky/rt.tga",
        "DarkSky/lt.tga",
        "DarkSky/up.tga",
        "DarkSky/dn.tga",
        "DarkSky/bk.tga",
        "DarkSky/ft.tga"
    };
    AImage rt, lt, up, dn, bk, ft;
    AImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
    rt.Load( Cubemap[0], nullptr, IMAGE_PF_BGR32F );
    lt.Load( Cubemap[1], nullptr, IMAGE_PF_BGR32F );
    up.Load( Cubemap[2], nullptr, IMAGE_PF_BGR32F );
    dn.Load( Cubemap[3], nullptr, IMAGE_PF_BGR32F );
    bk.Load( Cubemap[4], nullptr, IMAGE_PF_BGR32F );
    ft.Load( Cubemap[5], nullptr, IMAGE_PF_BGR32F );
    const float HDRI_Scale = 4.0f;
    const float HDRI_Pow = 1.1f;
    for ( int i = 0 ; i < 6 ; i++ ) {
        float * HDRI = (float*)cubeFaces[i]->pRawData;
        int count = cubeFaces[i]->Width*cubeFaces[i]->Height*3;
        for ( int j = 0; j < count ; j += 3 ) {
            HDRI[j] = StdPow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
            HDRI[j + 1] = StdPow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
            HDRI[j + 2] = StdPow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
        }
    }
    int w = cubeFaces[0]->Width;
    TextureStorageCreateInfo cubemapCI = {};
    cubemapCI.Type = GHI::TEXTURE_CUBE_MAP;
    cubemapCI.InternalFormat = INTERNAL_PIXEL_FORMAT_RGB32F;
    cubemapCI.Resolution.TexCubemap.Width = w;
    cubemapCI.NumLods = 1;
    Texture cubemap;
    cubemap.InitializeStorage( cubemapCI );
    for ( int face = 0 ; face < 6 ; face++ ) {
        float * pSrc = (float *)cubeFaces[face]->pRawData;

        TextureRect rect = {};
        rect.Offset.Z = face;
        rect.Dimension.X = w;
        rect.Dimension.Y = w;
        rect.Dimension.Z = 1;

        cubemap.WriteRect( rect, PIXEL_FORMAT_FLOAT_BGR, w*w*3*sizeof( float ), 1, pSrc );
    }
    rt.Load( Cubemap2[0], nullptr, IMAGE_PF_BGR32F );
    lt.Load( Cubemap2[1], nullptr, IMAGE_PF_BGR32F );
    up.Load( Cubemap2[2], nullptr, IMAGE_PF_BGR32F );
    dn.Load( Cubemap2[3], nullptr, IMAGE_PF_BGR32F );
    bk.Load( Cubemap2[4], nullptr, IMAGE_PF_BGR32F );
    ft.Load( Cubemap2[5], nullptr, IMAGE_PF_BGR32F );
    w = cubeFaces[0]->Width;
    cubemapCI.Resolution.TexCubemap.Width = w;
    cubemapCI.NumLods = 1;
    Texture cubemap2;
    cubemap2.InitializeStorage( cubemapCI );
    for ( int face = 0 ; face < 6 ; face++ ) {
        float * pSrc = (float *)cubeFaces[face]->pRawData;

        TextureRect rect = {};
        rect.Offset.Z = face;
        rect.Dimension.X = w;
        rect.Dimension.Y = w;
        rect.Dimension.Z = 1;

        cubemap2.WriteRect( rect, PIXEL_FORMAT_FLOAT_BGR, w*w*3*sizeof( float ), 1, pSrc );
    }
    Texture * cubemaps[2] = { &cubemap, &cubemap2 };
    AEnvProbeGenerator envProbeGenerator;
    envProbeGenerator.Initialize();
    envProbeGenerator.GenerateArray( EnvProbe, 7, 2, cubemaps );
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_MIPMAP_BILINEAR;
    samplerCI.bCubemapSeamless = true;
    EnvProbeSampler = GDevice.GetOrCreateSampler( samplerCI );
    EnvProbeBindless.Initialize( &EnvProbe, EnvProbeSampler );
    EnvProbeBindless.MakeResident();
    }
    /////////////////////////////////////////////////////////////////////
}

void AFrameResources::Deinitialize() {
    Saq.Deinitialize();
    ClusterLookup.Deinitialize();
    ClusterItemTBO.Deinitialize();
    ClusterItemBuffer.Deinitialize();
    LightBuffer.Deinitialize();
    EnvProbeBindless.MakeNonResident();
    EnvProbe.Deinitialize();
    CascadeViewProjectionBuffer.Deinitialize();
    ViewUniformBuffer.Deinitialize();
    InstanceUniformBuffer.Deinitialize();
    ShadowInstanceUniformBuffer.Deinitialize();
    TempData.Free();
}

void AFrameResources::SetViewUniforms() {
    SViewUniformBuffer * uniformData = &ViewUniformBufferUniformData;

    uniformData->ViewProjection = GRenderView->ViewProjection;
    uniformData->InverseProjectionMatrix = GRenderView->InverseProjectionMatrix;

//    ViewUniformBlock->PositionToViewSpace[0].X = RenderFrame.TranslateViewMatrix[0].X;
//    ViewUniformBlock->PositionToViewSpace[0].Y = RenderFrame.TranslateViewMatrix[1].X;
//    ViewUniformBlock->PositionToViewSpace[0].Z = RenderFrame.TranslateViewMatrix[2].X;
//    ViewUniformBlock->PositionToViewSpace[0].W = RenderFrame.TranslateViewMatrix[3].X;

//    ViewUniformBlock->PositionToViewSpace[1].X = RenderFrame.TranslateViewMatrix[0].Y;
//    ViewUniformBlock->PositionToViewSpace[1].Y = RenderFrame.TranslateViewMatrix[1].Y;
//    ViewUniformBlock->PositionToViewSpace[1].Z = RenderFrame.TranslateViewMatrix[2].Y;
//    ViewUniformBlock->PositionToViewSpace[1].W = RenderFrame.TranslateViewMatrix[3].Y;

//    ViewUniformBlock->PositionToViewSpace[2].X = RenderFrame.TranslateViewMatrix[0].Z;
//    ViewUniformBlock->PositionToViewSpace[2].Y = RenderFrame.TranslateViewMatrix[1].Z;
//    ViewUniformBlock->PositionToViewSpace[2].Z = RenderFrame.TranslateViewMatrix[2].Z;
//    ViewUniformBlock->PositionToViewSpace[2].W = RenderFrame.TranslateViewMatrix[3].Z;

    uniformData->WorldNormalToViewSpace[0].X = GRenderView->NormalToViewMatrix[0][0];
    uniformData->WorldNormalToViewSpace[0].Y = GRenderView->NormalToViewMatrix[1][0];
    uniformData->WorldNormalToViewSpace[0].Z = GRenderView->NormalToViewMatrix[2][0];
    uniformData->WorldNormalToViewSpace[0].W = 0;

    uniformData->WorldNormalToViewSpace[1].X = GRenderView->NormalToViewMatrix[0][1];
    uniformData->WorldNormalToViewSpace[1].Y = GRenderView->NormalToViewMatrix[1][1];
    uniformData->WorldNormalToViewSpace[1].Z = GRenderView->NormalToViewMatrix[2][1];
    uniformData->WorldNormalToViewSpace[1].W = 0;

    uniformData->WorldNormalToViewSpace[2].X = GRenderView->NormalToViewMatrix[0][2];
    uniformData->WorldNormalToViewSpace[2].Y = GRenderView->NormalToViewMatrix[1][2];
    uniformData->WorldNormalToViewSpace[2].Z = GRenderView->NormalToViewMatrix[2][2];
    uniformData->WorldNormalToViewSpace[2].W = 0;

    uniformData->InvViewportSize.X = 1.0f / GRenderView->Width;
    uniformData->InvViewportSize.Y = 1.0f / GRenderView->Height;
    uniformData->ZNear = GRenderView->ViewZNear;
    uniformData->ZFar = GRenderView->ViewZFar;

    uniformData->GameRunningTimeSeconds = GRenderView->GameRunningTimeSeconds;
    uniformData->GameplayTimeSeconds = GRenderView->GameplayTimeSeconds;

    uniformData->DynamicResolutionRatioX = (float)GRenderView->Width / GFrameData->AllocSurfaceWidth;
    uniformData->DynamicResolutionRatioY = (float)GRenderView->Height / GFrameData->AllocSurfaceHeight;

    uniformData->ViewPosition = GRenderView->ViewPosition;
    uniformData->TimeDelta = GRenderView->GameplayTimeStep;

    uniformData->PostprocessBloomMix = Float4( 0.5f, 0.3f, 0.1f, 0.1f ) * RVPostprocessBloomScale.GetFloat(); // TODO: Get from GRenderView 
    uniformData->BloomEnabled = RVPostprocessBloom;  // TODO: Get from GRenderView
    uniformData->ToneMappingExposure = RVPostprocessToneExposure.GetFloat();  // TODO: Get from GRenderView
    uniformData->ColorGrading = GRenderView->CurrentColorGradingLUT ? 1.0f : 0.0f;
    uniformData->FXAA = RVFxaa;
    uniformData->VignetteColorIntensity = GRenderView->VignetteColorIntensity;
    uniformData->VignetteOuterRadiusSqr = GRenderView->VignetteOuterRadiusSqr;
    uniformData->VignetteInnerRadiusSqr = GRenderView->VignetteInnerRadiusSqr;
    uniformData->ColorGradingAdaptationSpeed = GRenderView->ColorGradingAdaptationSpeed;
    uniformData->ViewBrightness = Math::Saturate( RVBrightness.GetFloat() );

    uniformData->EnvProbeSampler = GFrameResources.EnvProbeBindless.GetHandle();

    uniformData->DebugMode = RVDebugRenderMode.GetInteger();

    uniformData->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //GLogger.Printf( "GRenderView->FirstDirectionalLight: %d\n", GRenderView->FirstDirectionalLight );

    for ( int i = 0 ; i < GRenderView->NumDirectionalLights ; i++ ) {
        SDirectionalLightDef * light = GFrameData->DirectionalLights[ GRenderView->FirstDirectionalLight + i ];

        uniformData->LightDirs[i] = Float4( GRenderView->NormalToViewMatrix * (light->Matrix[2]), 0.0f );
        uniformData->LightColors[i] = light->ColorAndAmbientIntensity;
        uniformData->LightParameters[i][0] = light->RenderMask;
        uniformData->LightParameters[i][1] = light->FirstCascade;
        uniformData->LightParameters[i][2] = light->NumCascades;
    }

    ViewUniformBuffer.Write( uniformData );
}

AN_FORCEINLINE void StoreFloat3x3AsFloat3x4Transposed( Float3x3 const & _In, Float3x4 & _Out ) {
    _Out[0][0] = _In[0][0];
    _Out[0][1] = _In[1][0];
    _Out[0][2] = _In[2][0];
    _Out[0][3] = 0;

    _Out[1][0] = _In[0][1];
    _Out[1][1] = _In[1][1];
    _Out[1][2] = _In[2][1];
    _Out[1][3] = 0;

    _Out[2][0] = _In[0][2];
    _Out[2][1] = _In[1][2];
    _Out[2][2] = _In[2][2];
    _Out[2][3] = 0;
}

AN_FORCEINLINE void StoreFloat3x4AsFloat4x4Transposed( Float3x4 const & _In, Float4x4 & _Out ) {
    _Out[0][0] = _In[0][0];
    _Out[0][1] = _In[1][0];
    _Out[0][2] = _In[2][0];
    _Out[0][3] = 0;

    _Out[1][0] = _In[0][1];
    _Out[1][1] = _In[1][1];
    _Out[1][2] = _In[2][1];
    _Out[1][3] = 0;

    _Out[2][0] = _In[0][2];
    _Out[2][1] = _In[1][2];
    _Out[2][2] = _In[2][2];
    _Out[2][3] = 0;

    _Out[3][0] = _In[0][3];
    _Out[3][1] = _In[1][3];
    _Out[3][2] = _In[2][3];
    _Out[3][3] = 1;
}

void AFrameResources::UploadUniforms() {
    SkeletonBufferBinding->pBuffer = GPUBufferHandle( GFrameData->StreamBuffer );

    SetViewUniforms();

    int totalInstanceCount = GRenderView->InstanceCount + GRenderView->TranslucentInstanceCount;

    // Set Instances Uniforms
    if ( InstanceUniformBufferSize < totalInstanceCount ) {
        InstanceUniformBufferSize = totalInstanceCount;
        InstanceUniformBuffer.Realloc( InstanceUniformBufferSize * InstanceUniformBufferSizeof/*, data*/ );
    }

    TempData.ResizeInvalidate( totalInstanceCount * InstanceUniformBufferSizeof );

    for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
        SRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

        SInstanceUniformBuffer * pUniformBuf = reinterpret_cast< SInstanceUniformBuffer * >( TempData.ToPtr() + i*InstanceUniformBufferSizeof );

        Core::Memcpy( &pUniformBuf->TransformMatrix, &instance->Matrix, sizeof( pUniformBuf->TransformMatrix ) );
        StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, pUniformBuf->ModelNormalToViewSpace );
        Core::Memcpy( &pUniformBuf->LightmapOffset, &instance->LightmapOffset, sizeof( pUniformBuf->LightmapOffset ) );
        Core::Memcpy( &pUniformBuf->uaddr_0, instance->MaterialInstance->UniformVectors, sizeof( Float4 )*instance->MaterialInstance->NumUniformVectors );

        //InstanceUniformBuffer.WriteRange( i * InstanceUniformBufferSizeof, UniformBufferSizeof, pUniformBuf );
    }
    int uniformTranslucentOffset = GRenderView->InstanceCount;
    for ( int i = 0 ; i < GRenderView->TranslucentInstanceCount ; i++ ) {
        SRenderInstance const * instance = GFrameData->TranslucentInstances[GRenderView->FirstTranslucentInstance + i];

        SInstanceUniformBuffer * pUniformBuf = reinterpret_cast< SInstanceUniformBuffer * >(TempData.ToPtr() + (uniformTranslucentOffset+i)*InstanceUniformBufferSizeof);

        Core::Memcpy( &pUniformBuf->TransformMatrix, &instance->Matrix, sizeof( pUniformBuf->TransformMatrix ) );
        StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, pUniformBuf->ModelNormalToViewSpace );
        Core::Memcpy( &pUniformBuf->LightmapOffset, &instance->LightmapOffset, sizeof( pUniformBuf->LightmapOffset ) );
        Core::Memcpy( &pUniformBuf->uaddr_0, instance->MaterialInstance->UniformVectors, sizeof( Float4 )*instance->MaterialInstance->NumUniformVectors );

        //InstanceUniformBuffer.WriteRange( i * InstanceUniformBufferSizeof, UniformBufferSizeof, pUniformBuf );
    }
    InstanceUniformBuffer.WriteRange( 0, totalInstanceCount * InstanceUniformBufferSizeof, TempData.ToPtr() );


    // Set Instances Uniforms
    if ( ShadowInstanceUniformBufferSize < GRenderView->ShadowInstanceCount ) {
        ShadowInstanceUniformBufferSize = GRenderView->ShadowInstanceCount;
        ShadowInstanceUniformBuffer.Realloc( ShadowInstanceUniformBufferSize * ShadowInstanceUniformBufferSizeof );
    }

    TempData.ResizeInvalidate( GRenderView->ShadowInstanceCount * ShadowInstanceUniformBufferSizeof );

    for ( int i = 0 ; i < GRenderView->ShadowInstanceCount ; i++ ) {
        SShadowRenderInstance const * instance = GFrameData->ShadowInstances[GRenderView->FirstShadowInstance + i];
        SShadowInstanceUniformBuffer * pUniformBuf = reinterpret_cast< SShadowInstanceUniformBuffer * >(TempData.ToPtr() + i*ShadowInstanceUniformBufferSizeof);

        StoreFloat3x4AsFloat4x4Transposed( instance->WorldTransformMatrix, pUniformBuf->TransformMatrix );

        if ( instance->MaterialInstance ) {
            Core::Memcpy( &pUniformBuf->uaddr_0, instance->MaterialInstance->UniformVectors, sizeof( Float4 )*instance->MaterialInstance->NumUniformVectors );
        }
    }

    ShadowInstanceUniformBuffer.WriteRange( 0, GRenderView->ShadowInstanceCount * ShadowInstanceUniformBufferSizeof, TempData.ToPtr() );

    // Cascade matrices
    CascadeViewProjectionBuffer.WriteRange( 0, sizeof( Float4x4 ) * GRenderView->NumShadowMapCascades, GRenderView->LightViewProjectionMatrices );
    CascadeViewProjectionBuffer.WriteRange( MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES * sizeof( Float4x4 ),
        sizeof( Float4x4 ) * GRenderView->NumShadowMapCascades, GRenderView->ShadowMapMatrices );
    

    // Write cluster data
    ClusterLookup.Write( 0,
                         GHI::PIXEL_FORMAT_UINT_RG,
                         sizeof( SClusterData )*MAX_FRUSTUM_CLUSTERS_X*MAX_FRUSTUM_CLUSTERS_Y*MAX_FRUSTUM_CLUSTERS_Z,
                         1,
                         GRenderView->LightData.ClusterLookup );

    ClusterItemBuffer.WriteRange( 0,
                                  sizeof( SClusterItemBuffer )*GRenderView->LightData.TotalItems,
                                  GRenderView->LightData.ItemBuffer );

    LightBuffer.WriteRange( 0,
                            sizeof( SClusterLight ) * GRenderView->LightData.TotalLights,
                            GRenderView->LightData.LightBuffer );
}

}
