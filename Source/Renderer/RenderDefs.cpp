/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "RenderDefs.h"

#include <Platform/Logger.h>

struct STextureFormatDetails
{
    size_t  SinzeInBytesUncompressed;
    size_t  BlockSizeCompressed;
    uint8_t NumComponents;
    bool    bCompressed : 1;
    bool    bSRGB : 1;
    bool    bFloat : 1;
};

static const TArray< STextureFormatDetails, TEXTURE_PF_MAX > FormatTable = {
    // TEXTURE_PF_R8_SNORM
    STextureFormatDetails{ 1, 0, 1, false, false, false },

    // TEXTURE_PF_RG8_SNORM
    STextureFormatDetails{ 2, 0, 2, false, false, false },

    // TEXTURE_PF_BGR8_SNORM
    //STextureFormatDetails{ 3, 0, 3, false, false, false },

    // TEXTURE_PF_BGRA8_SNORM
    STextureFormatDetails{ 4, 0, 4, false, false, false },

    // TEXTURE_PF_R8_UNORM
    STextureFormatDetails{ 1, 0, 1, false, false, false },

    // TEXTURE_PF_RG8_UNORM
    STextureFormatDetails{ 2, 0, 2, false, false, false },

    // TEXTURE_PF_BGR8_UNORM
    //STextureFormatDetails{ 3, 0, 3, false, false, false },

    // TEXTURE_PF_BGRA8_UNORM
    STextureFormatDetails{ 4, 0, 4, false, false, false },

    // TEXTURE_PF_BGR8_SRGB
    //STextureFormatDetails{ 3, 0, 3, false, true, false },

    // TEXTURE_PF_BGRA8_SRGB
    STextureFormatDetails{ 4, 0, 4, false, true, false },

    // TEXTURE_PF_R16I
    STextureFormatDetails{ 2, 0, 1, false, false, false },

    // TEXTURE_PF_RG16I
    STextureFormatDetails{ 4, 0, 2, false, false, false },

    // TEXTURE_PF_BGR16I
    //STextureFormatDetails{ 6, 0, 3, false, false, false },

    // TEXTURE_PF_BGRA16I
    STextureFormatDetails{ 8, 0, 4, false, false, false },

    // TEXTURE_PF_R16UI
    STextureFormatDetails{ 2, 0, 1, false, false, false },

    // TEXTURE_PF_RG16UI
    STextureFormatDetails{ 4, 0, 2, false, false, false },

    // TEXTURE_PF_BGR16UI
    //STextureFormatDetails{ 6, 0, 3, false, false, false },

    // TEXTURE_PF_BGRA16UI
    STextureFormatDetails{ 8, 0, 4, false, false, false },

    // TEXTURE_PF_R32I
    STextureFormatDetails{ 4, 0, 1, false, false, false },

    // TEXTURE_PF_RG32I
    STextureFormatDetails{ 8, 0, 2, false, false, false },

    // TEXTURE_PF_BGR32I
    STextureFormatDetails{ 12, 0, 3, false, false, false },

    // TEXTURE_PF_BGRA32I
    STextureFormatDetails{ 16, 0, 4, false, false, false },

    // TEXTURE_PF_R32UI
    STextureFormatDetails{ 4, 0, 1, false, false, false },

    // TEXTURE_PF_RG32UI
    STextureFormatDetails{ 8, 0, 2, false, false, false },

    // TEXTURE_PF_BGR32UI
    STextureFormatDetails{ 12, 0, 3, false, false, false },

    // TEXTURE_PF_BGRA32UI
    STextureFormatDetails{ 16, 0, 4, false, false, false },

    // TEXTURE_PF_R16F
    STextureFormatDetails{ 2, 0, 1, false, false, true },

    // TEXTURE_PF_RG16F
    STextureFormatDetails{ 4, 0, 2, false, false, true },

    // TEXTURE_PF_BGR16F
    //STextureFormatDetails{ 6, 0, 3, false, false, true },

    // TEXTURE_PF_BGRA16F
    STextureFormatDetails{ 8, 0, 4, false, false, true },

    // TEXTURE_PF_R32F
    STextureFormatDetails{ 4, 0, 1, false, false, true },

    // TEXTURE_PF_RG32F
    STextureFormatDetails{ 8, 0, 2, false, false, true },

    // TEXTURE_PF_BGR32F
    STextureFormatDetails{ 12, 0, 3, false, false, true },

    // TEXTURE_PF_BGRA32F
    STextureFormatDetails{ 16, 0, 4, false, false, true },

    // TEXTURE_PF_R11F_G11F_B10F
    STextureFormatDetails{ 4, 0, 3, false, false, true },

    // Compressed formats

    // RGB
    // TEXTURE_PF_COMPRESSED_BC1_RGB
    STextureFormatDetails{ 3, /* TODO BlockSizeCompressed */ 0, 3, true, false, false },

    // TEXTURE_PF_COMPRESSED_BC1_SRGB
    STextureFormatDetails{ 3, /* TODO BlockSizeCompressed */ 0, 3, true, true, false },

    // RGB A-4bit / RGB (not the best quality, it is better to use BC3)
    // TEXTURE_PF_COMPRESSED_BC2_RGBA
    STextureFormatDetails{ 4, /* TODO BlockSizeCompressed */ 0, 4, true, false, false },

    // TEXTURE_PF_COMPRESSED_BC2_SRGB_ALPHA
    STextureFormatDetails{ 4, /* TODO BlockSizeCompressed */ 0, 4, true, true, false },

    // RGB A-8bit
    // TEXTURE_PF_COMPRESSED_BC3_RGBA
    STextureFormatDetails{ 4, /* TODO BlockSizeCompressed */ 0, 4, true, false, false },

    // TEXTURE_PF_COMPRESSED_BC3_SRGB_ALPHA
    STextureFormatDetails{ 4, /* TODO BlockSizeCompressed */ 0, 4, true, true, false },

    // R single channel texture (use for metalmap, glossmap, etc)
    // TEXTURE_PF_COMPRESSED_BC4_R
    STextureFormatDetails{ 1, /* TODO BlockSizeCompressed */ 0, 1, true, false, false },

    // TEXTURE_PF_COMPRESSED_BC4_R_SIGNED
    STextureFormatDetails{ 1, /* TODO BlockSizeCompressed */ 0, 1, true, false, false },

    // RG two channel texture (use for normal map or two grayscale maps)
    // TEXTURE_PF_COMPRESSED_BC5_RG
    STextureFormatDetails{ 2, /* TODO BlockSizeCompressed */ 0, 2, true, false, false },

    // TEXTURE_PF_COMPRESSED_BC5_RG_SIGNED
    STextureFormatDetails{ 2, /* TODO BlockSizeCompressed */ 0, 2, true, false, false },

    // RGB half float HDR
    // TEXTURE_PF_COMPRESSED_BC6H
    STextureFormatDetails{ 6, /* TODO BlockSizeCompressed */ 0, 3, true, false, true },

    // TEXTURE_PF_COMPRESSED_BC6H_SIGNED
    STextureFormatDetails{ 6, /* TODO BlockSizeCompressed */ 0, 3, true, false, true },

    // RGB[A], best quality, every block is compressed different
    // TEXTURE_PF_COMPRESSED_BC7_RGBA
    STextureFormatDetails{ 4, /* TODO BlockSizeCompressed */ 0, 4, true, false, false },

    // TEXTURE_PF_COMPRESSED_BC7_SRGB_ALPHA
    STextureFormatDetails{ 4, /* TODO BlockSizeCompressed */ 0, 4, true, true, false } };


bool STexturePixelFormat::IsCompressed() const
{
    return FormatTable[Data].bCompressed;
}

bool STexturePixelFormat::IsSRGB() const
{
    return FormatTable[Data].bSRGB;
}

int STexturePixelFormat::SizeInBytesUncompressed() const
{
    if ( IsCompressed() ) {
        GLogger.Printf( "SizeInBytesUncompressed: called for compressed pixel format\n" );
        return 0;
    }

    return FormatTable[Data].SinzeInBytesUncompressed;
}

int STexturePixelFormat::BlockSizeCompressed() const
{
    if ( !IsCompressed() ) {
        GLogger.Printf( "BlockSizeCompressed: called for uncompressed pixel format\n" );
        return 0;
    }

    // TODO
    HK_ASSERT_( 0, "Not implemented" );

    return FormatTable[Data].BlockSizeCompressed;
}

int STexturePixelFormat::NumComponents() const
{
    return FormatTable[Data].NumComponents;
}

bool STexturePixelFormat::GetAppropriatePixelFormat( EImagePixelFormat const & _ImagePixelFormat, STexturePixelFormat & _PixelFormat )
{
    switch ( _ImagePixelFormat ) {
        case IMAGE_PF_R:
            _PixelFormat = TEXTURE_PF_R8_UNORM;
            break;
        case IMAGE_PF_R16F:
            _PixelFormat = TEXTURE_PF_R16F;
            break;
        case IMAGE_PF_R32F:
            _PixelFormat = TEXTURE_PF_R32F;
            break;
        case IMAGE_PF_RG:
            _PixelFormat = TEXTURE_PF_RG8_UNORM;
            break;
        case IMAGE_PF_RG16F:
            _PixelFormat = TEXTURE_PF_RG16F;
            break;
        case IMAGE_PF_RG32F:
            _PixelFormat = TEXTURE_PF_RG32F;
            break;
        //case IMAGE_PF_RGB:
        //    _PixelFormat = TEXTURE_PF_BGRA8_UNORM;
        //    bNeedToAddAlpha = true;
        //    GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
        //    break;
        //case IMAGE_PF_RGB_GAMMA2:
        //    _PixelFormat = TEXTURE_PF_BGRA8_SRGB;
        //    bNeedToAddAlpha = true;
        //    GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
        //    break;
        //case IMAGE_PF_RGB16F:
        //    _PixelFormat = TEXTURE_PF_BGRA16F;
        //    bNeedToAddAlpha = true;
        //    GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
        //    break;
        case IMAGE_PF_RGB32F:
            _PixelFormat = TEXTURE_PF_BGR32F;
            GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
            break;
        case IMAGE_PF_RGBA:
            _PixelFormat = TEXTURE_PF_BGRA8_UNORM;
            GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
            break;
        case IMAGE_PF_RGBA_GAMMA2:
            _PixelFormat = TEXTURE_PF_BGRA8_SRGB;
            GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
            break;
        case IMAGE_PF_RGBA16F:
            _PixelFormat = TEXTURE_PF_BGRA16F;
            GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
            break;
        case IMAGE_PF_RGBA32F:
            _PixelFormat = TEXTURE_PF_BGRA32F;
            GLogger.Printf( "GetAppropriatePixelFormat: Waring: expect channel order BGR\n" );
            break;
        //case IMAGE_PF_BGR:
        //    _PixelFormat = TEXTURE_PF_BGRA8_UNORM;
        //    bNeedToAddAlpha = true;
        //    break;
        //case IMAGE_PF_BGR_GAMMA2:
        //    _PixelFormat = TEXTURE_PF_BGRA8_SRGB;
        //    bNeedToAddAlpha = true;
        //    break;
        //case IMAGE_PF_BGR16F:
        //    _PixelFormat = TEXTURE_PF_BGRA16F;
        //    bNeedToAddAlpha = true;
        //    break;
        case IMAGE_PF_BGR32F:
            _PixelFormat = TEXTURE_PF_BGR32F;
            break;
        case IMAGE_PF_BGRA:
            _PixelFormat = TEXTURE_PF_BGRA8_UNORM;
            break;
        case IMAGE_PF_BGRA_GAMMA2:
            _PixelFormat = TEXTURE_PF_BGRA8_SRGB;
            break;
        case IMAGE_PF_BGRA16F:
            _PixelFormat = TEXTURE_PF_BGRA16F;
            break;
        case IMAGE_PF_BGRA32F:
            _PixelFormat = TEXTURE_PF_BGRA32F;
            break;

        case IMAGE_PF_AUTO:
        case IMAGE_PF_AUTO_GAMMA2:
        case IMAGE_PF_AUTO_16F:
        case IMAGE_PF_AUTO_32F:
        default:
            GLogger.Printf( "GetAppropriatePixelFormat: invalid image\n" );
            return false;
    }

    return true;
}

// TODO: this can be computed at compile-time
float FRUSTUM_SLICE_SCALE = -( MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET ) / std::log2( (double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR );
float FRUSTUM_SLICE_BIAS  = std::log2( (double)FRUSTUM_CLUSTER_ZFAR ) * ( MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET ) / std::log2( (double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR ) - FRUSTUM_SLICE_OFFSET;
float FRUSTUM_SLICE_ZCLIP[MAX_FRUSTUM_CLUSTERS_Z + 1];

struct SFrustumSliceZClipInitializer
{
    SFrustumSliceZClipInitializer()
    {
        FRUSTUM_SLICE_ZCLIP[0] = 1; // extended near cluster

        for ( int SliceIndex = 1; SliceIndex < MAX_FRUSTUM_CLUSTERS_Z + 1; SliceIndex++ ) {
            //float sliceZ = FRUSTUM_CLUSTER_ZNEAR * Math::Pow( ( FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR ), ( float )sliceIndex / MAX_FRUSTUM_CLUSTERS_Z ); // linear depth
            //FRUSTUM_SLICE_ZCLIP[ sliceIndex ] = ( FRUSTUM_CLUSTER_ZFAR * FRUSTUM_CLUSTER_ZNEAR / sliceZ - FRUSTUM_CLUSTER_ZNEAR ) / FRUSTUM_CLUSTER_ZRANGE; // to ndc

            FRUSTUM_SLICE_ZCLIP[SliceIndex] = ( FRUSTUM_CLUSTER_ZFAR / Math::Pow( (double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR, (double)( SliceIndex + FRUSTUM_SLICE_OFFSET ) / ( MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET ) ) - FRUSTUM_CLUSTER_ZNEAR ) / FRUSTUM_CLUSTER_ZRANGE; // to ndc
        }
    }
};

static SFrustumSliceZClipInitializer FrustumSliceZClipInitializer;

void SMaterialDef::AddShader( const char * SourceName, AString const & SourceCode )
{
    int sourceNameLength = Platform::Strlen( SourceName );

    SMaterialShader * s = (SMaterialShader *)GZoneMemory.Alloc( sizeof( SMaterialShader ) + sourceNameLength + 1 + SourceCode.Length() + 1 );

    s->SourceName = (char *)s + sizeof( SMaterialShader );
    s->Code       = (char *)s + sizeof( SMaterialShader ) + sourceNameLength + 1;
    Platform::Memcpy( s->SourceName, SourceName, sourceNameLength + 1 );
    Platform::Memcpy( s->Code, SourceCode.CStr(), SourceCode.Length() + 1 );

    if ( !Shaders ) {
        Shaders = s;
        s->Next = NULL;
    }
    else {
        s->Next = Shaders;
        Shaders = s;
    }
}

void SMaterialDef::RemoveShaders()
{
    if ( !Shaders ) {
        return;
    }
    SMaterialShader * next;
    for ( SMaterialShader * s = Shaders; s; s = next ) {
        next = s->Next;
        GZoneMemory.Free( s );
    }
    Shaders = nullptr;
}
