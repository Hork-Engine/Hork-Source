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

#include <Runtime/Public/RenderCore.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/RuntimeVariable.h>
#include <Core/Public/Logger.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Core/Public/Image.h>

#include <Renderer/OpenGL4.5/OpenGL45RenderBackend.h>

IRenderBackend * GRenderBackend = &OpenGL45::GOpenGL45RenderBackend;


IGPUResourceOwner * IGPUResourceOwner::Resources = nullptr;
IGPUResourceOwner * IGPUResourceOwner::ResourcesTail = nullptr;

IGPUResourceOwner::IGPUResourceOwner()
{
    INTRUSIVE_ADD( this, pNext, pPrev, Resources, ResourcesTail );
}

IGPUResourceOwner::~IGPUResourceOwner()
{
    INTRUSIVE_REMOVE( this, pNext, pPrev, Resources, ResourcesTail );
}

void IGPUResourceOwner::InvalidateResources()
{
    for ( IGPUResourceOwner * resource = Resources ; resource ; resource = resource->pNext )
    {
        resource->UploadResourcesGPU();
    }
}


AResourceGPU * AResourceGPU::GPUResources;
AResourceGPU * AResourceGPU::GPUResourcesTail;

AResourceGPU::AResourceGPU()
{
    INTRUSIVE_ADD( this, pNext, pPrev, GPUResources, GPUResourcesTail );
}

AResourceGPU::~AResourceGPU()
{
    INTRUSIVE_REMOVE( this, pNext, pPrev, GPUResources, GPUResourcesTail );
}


int STexturePixelFormat::SizeInBytesUncompressed() const {

    if ( IsCompressed() ) {
        GLogger.Printf( "SizeInBytesUncompressed: called for compressed pixel format\n" );
        return 0;
    }

    int bytesPerChannel = 1 << ( Data & 3 );
    int channelsCount = ( ( Data >> 2 ) & 3 ) + 1;

    return bytesPerChannel * channelsCount;
}

int STexturePixelFormat::BlockSizeCompressed() const {

    if ( !IsCompressed() ) {
        GLogger.Printf( "BlockSizeCompressed: called for uncompressed pixel format\n" );
        return 0;
    }

    // TODO
    AN_ASSERT( 0 );
    return 0;
}

bool STexturePixelFormat::GetAppropriatePixelFormat( AImage const & _Image, STexturePixelFormat & _PixelFormat ) {
    if ( _Image.bHDRI ) {

        if ( _Image.bHalf ) {

            switch ( _Image.NumChannels ) {
            case 1:
                _PixelFormat = TEXTURE_PF_R16F;
                break;
            case 2:
                _PixelFormat = TEXTURE_PF_RG16F;
                break;
            case 3:
                _PixelFormat = TEXTURE_PF_BGR16F;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA16F;
                break;
            default:
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
                return false;
            }

        } else {

            switch ( _Image.NumChannels ) {
            case 1:
                _PixelFormat = TEXTURE_PF_R32F;
                break;
            case 2:
                _PixelFormat = TEXTURE_PF_RG32F;
                break;
            case 3:
                _PixelFormat = TEXTURE_PF_BGR32F;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA32F;
                break;
            default:
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
                return false;
            }

        }
    } else {

        if ( _Image.bLinearSpace ) {

            switch ( _Image.NumChannels ) {
            case 1:
                _PixelFormat = TEXTURE_PF_R8;
                break;
            case 2:
                _PixelFormat = TEXTURE_PF_RG8;
                break;
            case 3:
                _PixelFormat = TEXTURE_PF_BGR8;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA8;
                break;
            default:
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
                return false;
            }

        } else {

            switch ( _Image.NumChannels ) {
            case 3:
                _PixelFormat = TEXTURE_PF_BGR8_SRGB;
                break;
            case 4:
                _PixelFormat = TEXTURE_PF_BGRA8_SRGB;
                break;
            case 1:
            case 2:
            default:
                GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
                return false;
            }
        }
    }

    return true;
}

// TODO: this can be computed at compile-time
float FRUSTUM_SLICE_SCALE = -(MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET) / std::log2( (double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR );
float FRUSTUM_SLICE_BIAS = std::log2( (double)FRUSTUM_CLUSTER_ZFAR ) * (MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET) / std::log2( (double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR ) - FRUSTUM_SLICE_OFFSET;
float FRUSTUM_SLICE_ZCLIP[MAX_FRUSTUM_CLUSTERS_Z + 1];

struct SFrustumSliceZClipInitializer {
    SFrustumSliceZClipInitializer() {
        FRUSTUM_SLICE_ZCLIP[0] = 1; // extended near cluster

        for ( int SliceIndex = 1 ; SliceIndex < MAX_FRUSTUM_CLUSTERS_Z + 1 ; SliceIndex++ ) {
            //float sliceZ = FRUSTUM_CLUSTER_ZNEAR * pow( ( FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR ), ( float )sliceIndex / MAX_FRUSTUM_CLUSTERS_Z ); // linear depth
            //FRUSTUM_SLICE_ZCLIP[ sliceIndex ] = ( FRUSTUM_CLUSTER_ZFAR * FRUSTUM_CLUSTER_ZNEAR / sliceZ - FRUSTUM_CLUSTER_ZNEAR ) / FRUSTUM_CLUSTER_ZRANGE; // to ndc

            FRUSTUM_SLICE_ZCLIP[SliceIndex] = (FRUSTUM_CLUSTER_ZFAR / pow( (double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR, (double)(SliceIndex + FRUSTUM_SLICE_OFFSET) / (MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET) ) - FRUSTUM_CLUSTER_ZNEAR) / FRUSTUM_CLUSTER_ZRANGE; // to ndc
        }
    }
};

static SFrustumSliceZClipInitializer FrustumSliceZClipInitializer;
