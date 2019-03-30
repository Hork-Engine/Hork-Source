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

#include <Engine/Runtime/Public/RenderBackend.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

FRenderBackend const * GRenderBackend = nullptr;
FRenderProxy * GRenderProxyHead = nullptr;
FRenderProxy * GRenderProxyTail = nullptr;

static FRenderBackend * RenderBackends = nullptr;

FRenderBackend const * GetRenderBackends() {
    return RenderBackends;
}

FRenderBackend const * FindRenderBackend( const char * _Name ) {
    for ( FRenderBackend * backend = RenderBackends ; backend ; backend = backend->Next ) {
        if ( !FString::Icmp( backend->Name, _Name ) ) {
            return backend;
        }
    }
    return nullptr;
}

void RegisterRenderBackend( FRenderBackend * _BackendInterface ) {
    _BackendInterface->Next = RenderBackends;
    RenderBackends = _BackendInterface;
}

void * AllocateBufferData( size_t _Size ) {
    return GMainMemoryZone.Alloc( _Size, 1 );
}

void * ExtendBufferData( void * _Data, size_t _OldSize, size_t _NewSize, bool _KeepOld ) {
    return GMainMemoryZone.Extend( _Data, _OldSize, _NewSize, 1, _KeepOld );
}

void DeallocateBufferData( void * _Data ) {
    GMainMemoryZone.Dealloc( _Data );
}

bool IsTextureCompressed( ETexturePixelFormat _PixelFormat ) {
    return ( _PixelFormat >> 6 ) & 1;
}

bool IsTextureSRGB( ETexturePixelFormat _PixelFormat ) {
    return ( _PixelFormat >> 4 ) & 1;
}

int UncompressedPixelByteLength( ETexturePixelFormat _PixelFormat ) {

    if ( IsTextureCompressed( _PixelFormat ) ) {
        GLogger.Printf( "UncompressedPixelByteLength: called for compressed pixel format\n" );
        return 0;
    }

    int bytesPerChannel = 1 << ( _PixelFormat & 3 );
    int channelsCount = ( ( _PixelFormat >> 2 ) & 3 ) + 1;

    return bytesPerChannel * channelsCount;
}

int CompressedTextureBlockLength( ETexturePixelFormat _PixelFormat ) {

    if ( !IsTextureCompressed( _PixelFormat ) ) {
        GLogger.Printf( "CompressedTextureBlockLength: called for uncompressed pixel format\n" );
        return 0;
    }

    // TODO
    AN_Assert( 0 );
    return 0;
}

int NumPixelComponents( ETexturePixelFormat _PixelFormat ) {
    return ( ( _PixelFormat >> 2 ) & 3 ) + 1;
}







//FRenderProxy * FRenderProxy::PendingKillProxies = nullptr;

void FRenderProxy::KillProxy() {
    AN_Assert( !bPendingKill );

    FRenderFrame * frameData = GRuntime.GetFrameData();

    if ( !bSubmittedToRenderThread ) {
        this->~FRenderProxy();
        GMainMemoryZone.Dealloc( this );
        return;
    } else {
        NextFreeProxy = frameData->RenderProxyFree;
        frameData->RenderProxyFree = this;
    }

    bPendingKill = true;
}

// TODO: call this after SwapBuffers()
void FRenderProxy::FreeDeadProxies(/* FRenderFrame * _Frame */) {
#if 0
    FRenderProxy * next, *prev = nullptr;
    for ( FRenderProxy * proxy = PendingKillProxies ; proxy ; proxy = next ) {

        next = proxy->NextPendingKillProxy;

        if ( proxy->bCanRemove/* || !proxy->bSubmittedToRenderThread*/ ) {

            if ( prev ) {
                prev->NextPendingKillProxy = next;
            } else {
                PendingKillProxies = next;
            }

            proxy->~FRenderProxy();
            GMainMemoryZone.Dealloc( proxy );

        } /*else if ( !proxy->bFreed ) {

            proxy->NextFreeProxy = _Frame->FreeProxies;
            _Frame->FreeProxies = proxy;

            proxy->bFreed = true;
            prev = proxy;
        }*/ else {
            prev = proxy;
        }
    }
#endif
}

void FRenderProxy::MarkUpdated() {
    AN_Assert( !bPendingKill );

    FRenderFrame * frameData = GRuntime.GetFrameData();

    //bLost[frameData->SmpIndex] = false;

    if ( IntrusiveIsInList( this,
                            NextUpload[ frameData->SmpIndex ],
                            PrevUpload[ frameData->SmpIndex ],
                            frameData->RenderProxyUploadHead,
                            frameData->RenderProxyUploadTail ) ) {
        // Already marked
        return;
    }

    IntrusiveAddToList( this,
                        NextUpload[ frameData->SmpIndex ],
                        PrevUpload[ frameData->SmpIndex ],
                        frameData->RenderProxyUploadHead,
                        frameData->RenderProxyUploadTail );

    bSubmittedToRenderThread = true;
}

//void FRenderProxy::SubmitToRenderThread( int _FrameIndex ) {

//    if ( SubmitFrameIndex == _FrameIndex ) {
//        // Already submitted at this frame
//        return;
//    }

//    SubmitFrameIndex = _FrameIndex;

//    if ( bLost[frameData->SmpIndex] ) {
//        if ( DataHolder ) {
//            DataHolder->OnLost();
//        }
//        bLost[frameData->SmpIndex] = false;
//    }

//    bSubmittedToRenderThread = true;

//    AN_Assert( !bPendingKill );
//}

FRenderProxy_IndexedMesh::FRenderProxy_IndexedMesh() {
    Type = RENDER_PROXY_INDEXED_MESH;
}

FRenderProxy_IndexedMesh::~FRenderProxy_IndexedMesh() {
}

FRenderProxy_LightmapUVChannel::FRenderProxy_LightmapUVChannel() {
    Type = RENDER_PROXY_LIGHTMAP_UV_CHANNEL;
}

FRenderProxy_LightmapUVChannel::~FRenderProxy_LightmapUVChannel() {

}

FRenderProxy_VertexLightChannel::FRenderProxy_VertexLightChannel() {
    Type = RENDER_PROXY_VERTEX_LIGHT_CHANNEL;
}

FRenderProxy_VertexLightChannel::~FRenderProxy_VertexLightChannel() {

}

FRenderProxy_Texture::FRenderProxy_Texture() {
    Type = RENDER_PROXY_TEXTURE;
}

FRenderProxy_Texture::~FRenderProxy_Texture() {

}

FRenderProxy_Material::FRenderProxy_Material() {
    Type = RENDER_PROXY_MATERIAL;
}

FRenderProxy_Material::~FRenderProxy_Material() {

}

void * FRenderFrame::AllocFrameData( size_t _BytesCount ) {
    if ( FrameMemoryUsed + _BytesCount > FrameMemorySize ) {
        GLogger.Printf( "AllocFrameData: failed on allocation of %u bytes (available %u, total %u)\n", _BytesCount, FrameMemoryUsed, FrameMemorySize );
        return nullptr;
    }

    void * pMemory;

    if ( SmpIndex & 1 ) {
        pMemory = (byte *)pFrameMemory - FrameMemoryUsed - _BytesCount;
    } else {
        pMemory = (byte *)pFrameMemory + FrameMemoryUsed;
    }

    FrameMemoryUsed += _BytesCount;

    //GLogger.Printf( "Allocated %u, Used %u\n", _BytesCount, FrameMemoryUsed );

    return pMemory;
}
