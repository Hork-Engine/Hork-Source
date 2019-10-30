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

#include "OpenGL45FrameResources.h"
#include "OpenGL45JointsAllocator.h"
#include "OpenGL45EnvProbeGenerator.h"

#include <Engine/Core/Public/Image.h>
#include <Engine/Runtime/Public/RuntimeVariable.h>

FRuntimeVariable RVDebugRenderMode( _CTS( "DebugRenderMode" ), _CTS( "0" ), VAR_CHEAT );

using namespace GHI;

namespace OpenGL45 {

FFrameResources GFrameResources;

void FFrameResources::Initialize() {
    GHI::BufferCreateInfo uniformBufferCI = {};
    uniformBufferCI.bImmutableStorage = true;
    uniformBufferCI.ImmutableStorageFlags = IMMUTABLE_DYNAMIC_STORAGE;

    uniformBufferCI.SizeInBytes = 2 * MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES * sizeof( Float4x4 );
    CascadeViewProjectionBuffer.Initialize( uniformBufferCI );

    uniformBufferCI.SizeInBytes = sizeof( FViewUniformBuffer );
    ViewUniformBuffer.Initialize( uniformBufferCI );

    InstanceUniformBufferSize = 1024;
    ShadowInstanceUniformBufferSize = 1024;
    GHI::BufferCreateInfo streamBufferCI = {};
    streamBufferCI.bImmutableStorage = false;
    streamBufferCI.MutableClientAccess = MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    streamBufferCI.MutableUsage = MUTABLE_STORAGE_STREAM;
    streamBufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
    streamBufferCI.SizeInBytes = InstanceUniformBufferSize * InstanceUniformBufferSizeof;
    InstanceUniformBuffer.Initialize( streamBufferCI );
    streamBufferCI.SizeInBytes = ShadowInstanceUniformBufferSize * ShadowInstanceUniformBufferSizeof;
    ShadowInstanceUniformBuffer.Initialize( streamBufferCI );

    memset( BufferBinding, 0, sizeof( BufferBinding ) );
    memset( TextureBindings, 0, sizeof( TextureBindings ) );
    memset( SamplerBindings, 0, sizeof( SamplerBindings ) );

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
    SkeletonBufferBinding->pBuffer = &GJointsAllocator.Buffer;

    CascadeBufferBinding = &BufferBinding[3];
    CascadeBufferBinding->BufferType = UNIFORM_BUFFER;
    CascadeBufferBinding->SlotIndex = 3;
    CascadeBufferBinding->pBuffer = &CascadeViewProjectionBuffer;

    for ( int i = 0 ; i < 16 ; i++ ) {
        TextureBindings[i].SlotIndex = i;
        SamplerBindings[i].SlotIndex = i;
    }

    memset( &Resources, 0, sizeof( Resources ) );
    Resources.Buffers = BufferBinding;
    Resources.NumBuffers = AN_ARRAY_SIZE( BufferBinding );

    Resources.Textures = TextureBindings;
    Resources.NumTextures = AN_ARRAY_SIZE( TextureBindings );

    Resources.Samplers = SamplerBindings;
    Resources.NumSamplers = AN_ARRAY_SIZE( SamplerBindings );

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////
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
    FImage rt, lt, up, dn, bk, ft;
    FImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
    rt.LoadHDRI( Cubemap[0], false, false, 3 );
    lt.LoadHDRI( Cubemap[1], false, false, 3 );
    up.LoadHDRI( Cubemap[2], false, false, 3 );
    dn.LoadHDRI( Cubemap[3], false, false, 3 );
    bk.LoadHDRI( Cubemap[4], false, false, 3 );
    ft.LoadHDRI( Cubemap[5], false, false, 3 );
    const float HDRI_Scale = 4.0f;
    const float HDRI_Pow = 1.1f;
    for ( int i = 0 ; i < 6 ; i++ ) {
        float * HDRI = (float*)cubeFaces[i]->pRawData;
        int count = cubeFaces[i]->Width*cubeFaces[i]->Height*3;
        for ( int j = 0; j < count ; j += 3 ) {
            HDRI[j] = pow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
            HDRI[j + 1] = pow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
            HDRI[j + 2] = pow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
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
    rt.LoadHDRI( Cubemap2[0], false, false, 3 );
    lt.LoadHDRI( Cubemap2[1], false, false, 3 );
    up.LoadHDRI( Cubemap2[2], false, false, 3 );
    dn.LoadHDRI( Cubemap2[3], false, false, 3 );
    bk.LoadHDRI( Cubemap2[4], false, false, 3 );
    ft.LoadHDRI( Cubemap2[5], false, false, 3 );
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
    FEnvProbeGenerator envProbeGenerator;
    envProbeGenerator.Initialize();
    envProbeGenerator.GenerateArray( EnvProbe, 7, 2, cubemaps );
    SamplerCreateInfo samplerCI;
    samplerCI.SetDefaults();
    samplerCI.Filter = FILTER_MIPMAP_BILINEAR;
    samplerCI.bCubemapSeamless = true;
    EnvProbeSampler = GDevice.GetOrCreateSampler( samplerCI );
    EnvProbeBindless.Initialize( &EnvProbe, EnvProbeSampler );
    EnvProbeBindless.MakeResident();
    /////////////////////////////////////////////////////////////////////
}

void FFrameResources::Deinitialize() {
    EnvProbeBindless.MakeNonResident();
    EnvProbe.Deinitialize();
    CascadeViewProjectionBuffer.Deinitialize();
    ViewUniformBuffer.Deinitialize();
    InstanceUniformBuffer.Deinitialize();
    ShadowInstanceUniformBuffer.Deinitialize();
    TempData.Free();
}

void FFrameResources::SetViewUniforms() {
    FViewUniformBuffer * uniformData = &ViewUniformBufferUniformData;

//    ViewUniformBlock->Viewport.X = 0;
//    ViewUniformBlock->Viewport.Y = 0;
//    ViewUniformBlock->Viewport.Z = RenderFrame.TargetTex->CurrentWidth;
//    ViewUniformBlock->Viewport.W = RenderFrame.TargetTex->CurrentHeight;
    

    uniformData->ModelviewProjection = GRenderView->ModelviewProjection;
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
    uniformData->ViewPostion = GRenderView->ViewPosition;

    uniformData->EnvProbeSampler = GFrameResources.EnvProbeBindless.GetHandle();

    uniformData->DebugMode = RVDebugRenderMode.GetInteger();

    uniformData->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //GLogger.Printf( "GRenderView->FirstDirectionalLight: %d\n", GRenderView->FirstDirectionalLight );

    for ( int i = 0 ; i < GRenderView->NumDirectionalLights ; i++ ) {
        FDirectionalLightDef * light = GFrameData->DirectionalLights[ GRenderView->FirstDirectionalLight + i ];

        uniformData->LightDirs[i] = Float4( GRenderView->NormalToViewMatrix * (light->Matrix[2]), 0.0f );
        uniformData->LightColors[i] = light->ColorAndAmbientIntensity;
        uniformData->LightParameters[i].X = light->RenderMask;
        uniformData->LightParameters[i].Y = light->FirstCascade;
        uniformData->LightParameters[i].Z = light->NumCascades;
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

void FFrameResources::UploadUniforms() {
    SetViewUniforms();

    // Set Instances Uniforms
    if ( InstanceUniformBufferSize < GRenderView->InstanceCount ) {
        InstanceUniformBufferSize = GRenderView->InstanceCount;
        InstanceUniformBuffer.Realloc( InstanceUniformBufferSize * InstanceUniformBufferSizeof/*, data*/ );
    }/* else {
        InstanceUniformBuffer.WriteRange( 0, GRenderView->InstanceCount * UniformBufferSizeof, data );
    }*/


    //InstanceUniformBuffer.MapRange( 0, GRenderView->InstanceCount * UniformBufferSizeof, MAP_TRANSFER_WRITE, MAP_INVALIDATE_ENTIRE_BUFFER, MAP_NON_PERSISTENT, false, false );
    TempData.ResizeInvalidate( GRenderView->InstanceCount * InstanceUniformBufferSizeof );

    // FIXME: Map?
    for ( int i = 0 ; i < GRenderView->InstanceCount ; i++ ) {
        FRenderInstance const * instance = GFrameData->Instances[GRenderView->FirstInstance + i];

        FInstanceUniformBuffer * pUniformBuf = reinterpret_cast< FInstanceUniformBuffer * >( TempData.ToPtr() + i*InstanceUniformBufferSizeof );

        memcpy( &pUniformBuf->TransformMatrix, &instance->Matrix, sizeof( pUniformBuf->TransformMatrix ) );
        StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, pUniformBuf->ModelNormalToViewSpace );
        memcpy( &pUniformBuf->LightmapOffset, &instance->LightmapOffset, sizeof( pUniformBuf->LightmapOffset ) );
        memcpy( &pUniformBuf->uaddr_0, instance->MaterialInstance->UniformVectors, sizeof( Float4 )*instance->MaterialInstance->NumUniformVectors );

        //InstanceUniformBuffer.WriteRange( i * InstanceUniformBufferSizeof, UniformBufferSizeof, pUniformBuf );
    }
    InstanceUniformBuffer.WriteRange( 0, GRenderView->InstanceCount * InstanceUniformBufferSizeof, TempData.ToPtr() );
    //Cmd.Barrier( UNIFORM_BARRIER_BIT );








    // Set Instances Uniforms
    if ( ShadowInstanceUniformBufferSize < GRenderView->ShadowInstanceCount ) {
        ShadowInstanceUniformBufferSize = GRenderView->ShadowInstanceCount;
        ShadowInstanceUniformBuffer.Realloc( ShadowInstanceUniformBufferSize * ShadowInstanceUniformBufferSizeof );
    }

    TempData.ResizeInvalidate( GRenderView->ShadowInstanceCount * ShadowInstanceUniformBufferSizeof );

    for ( int i = 0 ; i < GRenderView->ShadowInstanceCount ; i++ ) {
        FShadowRenderInstance const * instance = GFrameData->ShadowInstances[GRenderView->FirstShadowInstance + i];
        FShadowInstanceUniformBuffer * pUniformBuf = reinterpret_cast< FShadowInstanceUniformBuffer * >(TempData.ToPtr() + i*ShadowInstanceUniformBufferSizeof);

        StoreFloat3x4AsFloat4x4Transposed( instance->WorldTransformMatrix, pUniformBuf->TransformMatrix );
        memcpy( &pUniformBuf->uaddr_0, instance->MaterialInstance->UniformVectors, sizeof( Float4 )*instance->MaterialInstance->NumUniformVectors );
    }

    ShadowInstanceUniformBuffer.WriteRange( 0, GRenderView->ShadowInstanceCount * ShadowInstanceUniformBufferSizeof, TempData.ToPtr() );

    // Cascade matrices
    CascadeViewProjectionBuffer.WriteRange( 0, sizeof( Float4x4 ) * GRenderView->NumShadowMapCascades, GRenderView->LightViewProjectionMatrices );
    CascadeViewProjectionBuffer.WriteRange( MAX_DIRECTIONAL_LIGHTS * MAX_SHADOW_CASCADES * sizeof( Float4x4 ),
        sizeof( Float4x4 ) * GRenderView->NumShadowMapCascades, GRenderView->ShadowMapMatrices );
    

    //GLogger.Printf( "NumShadowMapCascades: %d\n", GRenderView->NumShadowMapCascades );
    //for ( int i = 0 ; i < GRenderView->NumShadowMapCascades ; i++ ) {
    //    GLogger.Printf( "---\n%s\n", GRenderView->LightViewProjectionMatrices[i].ToString().ToConstChar() );
    //}
}

}
