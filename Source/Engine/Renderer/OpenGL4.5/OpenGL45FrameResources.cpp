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

using namespace GHI;

namespace OpenGL45 {

FFrameResources GFrameResources;

void FFrameResources::Initialize() {
    GHI::BufferCreateInfo uniformBufferCI = {};
    uniformBufferCI.bImmutableStorage = true;
    uniformBufferCI.ImmutableStorageFlags =
        //    GHI_ImmutableMapWrite
        //    | GHI_ImmutableMapPersistent
        //    | GHI_ImmutableMapCoherent
        //|IMMUTABLE_DYNAMIC_STORAGE| GHI_ImmutableClientStorage;
    IMMUTABLE_DYNAMIC_STORAGE;

    uniformBufferCI.SizeInBytes = sizeof( FInstanceUniformBuffer );
    UniformBuffer.Initialize( uniformBufferCI );

    uniformBufferCI.SizeInBytes = sizeof( FViewUniformBuffer );
    ViewUniformBuffer.Initialize( uniformBufferCI );

    InstanceUniformBufferSize = 1024;
    GHI::BufferCreateInfo streamBufferCI = {};
    streamBufferCI.bImmutableStorage = false;
    streamBufferCI.MutableClientAccess = MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    streamBufferCI.MutableUsage = MUTABLE_STORAGE_STREAM;
    streamBufferCI.ImmutableStorageFlags = (IMMUTABLE_STORAGE_FLAGS)0;
    streamBufferCI.SizeInBytes = InstanceUniformBufferSize * InstanceUniformBufferSizeof;
    InstanceUniformBuffer.Initialize( streamBufferCI );

    memset( BufferBinding, 0, sizeof( BufferBinding ) );
    memset( TextureBindings, 0, sizeof( TextureBindings ) );
    memset( SamplerBindings, 0, sizeof( SamplerBindings ) );

    ViewUniformBufferBinding = &BufferBinding[0];
    ViewUniformBufferBinding->BufferType = UNIFORM_BUFFER;
    ViewUniformBufferBinding->SlotIndex = 0;
    ViewUniformBufferBinding->pBuffer = &ViewUniformBuffer;

    UniformBufferBinding = &BufferBinding[1];
    UniformBufferBinding->BufferType = UNIFORM_BUFFER;
    UniformBufferBinding->SlotIndex = 1;
    UniformBufferBinding->pBuffer = &UniformBuffer;

    SkeletonBufferBinding = &BufferBinding[2];
    SkeletonBufferBinding->BufferType = UNIFORM_BUFFER;
    SkeletonBufferBinding->SlotIndex = 2;
    SkeletonBufferBinding->pBuffer = &GJointsAllocator.Buffer;

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
}

void FFrameResources::Deinitialize() {
    UniformBuffer.Deinitialize();
    ViewUniformBuffer.Deinitialize();
    InstanceUniformBuffer.Deinitialize();
    TempData.Free();
}

void FFrameResources::SetViewUniforms() {
    static FViewUniformBuffer ViewUniformBufferUniformData;

    FViewUniformBuffer * uniformData = &ViewUniformBufferUniformData;
//    FViewUniformBuffer * uniformData = (FViewUniformBuffer * )GHI_MapBuffer( &st,
//                                                                 &ViewUniformBuffer,
//                                                                 GHI_MapTransferWrite,
//                                                                 GHI_MapInvalidateEntireBuffer,
//                                                                 GHI_MapPersistentCoherent,
//                                                                 false,
//                                                                 false );

    AN_Assert(uniformData);


//    ViewUniformBlock->Viewport.X = 0;
//    ViewUniformBlock->Viewport.Y = 0;
//    ViewUniformBlock->Viewport.Z = RenderFrame.TargetTex->CurrentWidth;
//    ViewUniformBlock->Viewport.W = RenderFrame.TargetTex->CurrentHeight;
//    ViewUniformBlock->InvViewportSize.X = 1.0f / RenderFrame.TargetTex->CurrentWidth;
//    ViewUniformBlock->InvViewportSize.Y = 1.0f / RenderFrame.TargetTex->CurrentHeight;
//    ViewUniformBlock->InvViewportSize.Z = RenderFrame.Camera->GetZNear();
//    ViewUniformBlock->InvViewportSize.W = RenderFrame.Camera->GetZFar();

//    ViewUniformBlock->InverseProjectionMatrix = GRenderView->InverseProjectionMatrix;
//    ViewUniformBlock->ProjectTranslateViewMatrix = RenderFrame.ProjectTranslateViewMatrix;

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

//    ViewUniformBlock->WorldNormalToViewSpace[0].X = RenderFrame.NormalToView[0][0];
//    ViewUniformBlock->WorldNormalToViewSpace[0].Y = RenderFrame.NormalToView[1][0];
//    ViewUniformBlock->WorldNormalToViewSpace[0].Z = RenderFrame.NormalToView[2][0];
//    ViewUniformBlock->WorldNormalToViewSpace[0].W = 0;

//    ViewUniformBlock->WorldNormalToViewSpace[1].X = RenderFrame.NormalToView[0][1];
//    ViewUniformBlock->WorldNormalToViewSpace[1].Y = RenderFrame.NormalToView[1][1];
//    ViewUniformBlock->WorldNormalToViewSpace[1].Z = RenderFrame.NormalToView[2][1];
//    ViewUniformBlock->WorldNormalToViewSpace[1].W = 0;

//    ViewUniformBlock->WorldNormalToViewSpace[2].X = RenderFrame.NormalToView[0][2];
//    ViewUniformBlock->WorldNormalToViewSpace[2].Y = RenderFrame.NormalToView[1][2];
//    ViewUniformBlock->WorldNormalToViewSpace[2].Z = RenderFrame.NormalToView[2][2];
//    ViewUniformBlock->WorldNormalToViewSpace[2].W = 0;

//    ViewUniformBlock->ModelNormalToViewSpace[0].W = 0;
//    ViewUniformBlock->ModelNormalToViewSpace[1].W = 0;
//    ViewUniformBlock->ModelNormalToViewSpace[2].W = 0;

    uniformData->Timer.X = GRenderView->GameRunningTimeSeconds;
    uniformData->Timer.Y = GRenderView->GameplayTimeSeconds;
    uniformData->ViewPostion.X = GRenderView->ViewPostion.X;
    uniformData->ViewPostion.Y = GRenderView->ViewPostion.Y;
    uniformData->ViewPostion.Z = GRenderView->ViewPostion.Z;

//    GHI_UnmapBuffer( &st, &ViewUniformBuffer );

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

        memcpy( &pUniformBuf->ProjectTranslateViewMatrix, &instance->Matrix, sizeof( pUniformBuf->ProjectTranslateViewMatrix ) );
        StoreFloat3x3AsFloat3x4Transposed( instance->ModelNormalToViewSpace, pUniformBuf->ModelNormalToViewSpace );
        memcpy( &pUniformBuf->LightmapOffset, &instance->LightmapOffset, sizeof( pUniformBuf->LightmapOffset ) );
        memcpy( &pUniformBuf->uaddr_0, instance->MaterialInstance->UniformVectors, sizeof( Float4 )*instance->MaterialInstance->NumUniformVectors );

        //InstanceUniformBuffer.WriteRange( i * InstanceUniformBufferSizeof, UniformBufferSizeof, pUniformBuf );
    }
    InstanceUniformBuffer.WriteRange( 0, GRenderView->InstanceCount * InstanceUniformBufferSizeof, TempData.ToPtr() );
    //Cmd.Barrier( UNIFORM_BARRIER_BIT );
}

}
