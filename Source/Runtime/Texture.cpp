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

#include "Texture.h"
#include "Asset.h"
#include "Engine.h"

#include <Platform/Logger.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <Core/ScopedTimer.h>
#include <Core/Image.h>

static const char* TextureTypeName[] =
    {
        "TEXTURE_1D",
        "TEXTURE_1D_ARRAY",
        "TEXTURE_2D",
        "TEXTURE_2D_ARRAY",
        "TEXTURE_3D",
        "TEXTURE_CUBEMAP",
        "TEXTURE_CUBEMAP_ARRAY",
};

struct STextureFormatMapper
{
    TArray<RenderCore::DATA_FORMAT, TEXTURE_PF_MAX>    PixelFormatTable;
    TArray<RenderCore::TEXTURE_FORMAT, TEXTURE_PF_MAX> InternalPixelFormatTable;

    STextureFormatMapper()
    {
        using namespace RenderCore;

        PixelFormatTable[TEXTURE_PF_R8_SNORM]  = FORMAT_BYTE1;
        PixelFormatTable[TEXTURE_PF_RG8_SNORM] = FORMAT_BYTE2;
        //PixelFormatTable[TEXTURE_PF_BGR8_SNORM] = FORMAT_BYTE3;
        PixelFormatTable[TEXTURE_PF_BGRA8_SNORM] = FORMAT_BYTE4;

        PixelFormatTable[TEXTURE_PF_R8_UNORM]  = FORMAT_UBYTE1;
        PixelFormatTable[TEXTURE_PF_RG8_UNORM] = FORMAT_UBYTE2;
        //PixelFormatTable[TEXTURE_PF_BGR8_UNORM] = FORMAT_UBYTE3;
        PixelFormatTable[TEXTURE_PF_BGRA8_UNORM] = FORMAT_UBYTE4;

        //PixelFormatTable[TEXTURE_PF_BGR8_SRGB] = FORMAT_UBYTE3;
        PixelFormatTable[TEXTURE_PF_BGRA8_SRGB] = FORMAT_UBYTE4;

        PixelFormatTable[TEXTURE_PF_R16I]  = FORMAT_SHORT1;
        PixelFormatTable[TEXTURE_PF_RG16I] = FORMAT_SHORT2;
        //PixelFormatTable[TEXTURE_PF_BGR16I] = FORMAT_SHORT3;
        PixelFormatTable[TEXTURE_PF_BGRA16I] = FORMAT_SHORT4;

        PixelFormatTable[TEXTURE_PF_R16UI]  = FORMAT_USHORT1;
        PixelFormatTable[TEXTURE_PF_RG16UI] = FORMAT_USHORT2;
        //PixelFormatTable[TEXTURE_PF_BGR16UI] = FORMAT_USHORT3;
        PixelFormatTable[TEXTURE_PF_BGRA16UI] = FORMAT_USHORT4;

        PixelFormatTable[TEXTURE_PF_R32I]    = FORMAT_INT1;
        PixelFormatTable[TEXTURE_PF_RG32I]   = FORMAT_INT2;
        PixelFormatTable[TEXTURE_PF_BGR32I]  = FORMAT_INT3;
        PixelFormatTable[TEXTURE_PF_BGRA32I] = FORMAT_INT4;

        PixelFormatTable[TEXTURE_PF_R32I]     = FORMAT_UINT1;
        PixelFormatTable[TEXTURE_PF_RG32UI]   = FORMAT_UINT2;
        PixelFormatTable[TEXTURE_PF_BGR32UI]  = FORMAT_UINT3;
        PixelFormatTable[TEXTURE_PF_BGRA32UI] = FORMAT_UINT4;

        PixelFormatTable[TEXTURE_PF_R16F]  = FORMAT_HALF1;
        PixelFormatTable[TEXTURE_PF_RG16F] = FORMAT_HALF2;
        //PixelFormatTable[TEXTURE_PF_BGR16F] = FORMAT_HALF3;
        PixelFormatTable[TEXTURE_PF_BGRA16F] = FORMAT_HALF4;

        PixelFormatTable[TEXTURE_PF_R32F]    = FORMAT_FLOAT1;
        PixelFormatTable[TEXTURE_PF_RG32F]   = FORMAT_FLOAT2;
        PixelFormatTable[TEXTURE_PF_BGR32F]  = FORMAT_FLOAT3;
        PixelFormatTable[TEXTURE_PF_BGRA32F] = FORMAT_FLOAT4;

        PixelFormatTable[TEXTURE_PF_R11F_G11F_B10F] = FORMAT_FLOAT3;

        InternalPixelFormatTable[TEXTURE_PF_R8_SNORM]  = TEXTURE_FORMAT_R8_SNORM;
        InternalPixelFormatTable[TEXTURE_PF_RG8_SNORM] = TEXTURE_FORMAT_RG8_SNORM;
        //InternalPixelFormatTable[TEXTURE_PF_BGR8_SNORM] = TEXTURE_FORMAT_RGB8_SNORM;
        InternalPixelFormatTable[TEXTURE_PF_BGRA8_SNORM] = TEXTURE_FORMAT_RGBA8_SNORM;

        InternalPixelFormatTable[TEXTURE_PF_R8_UNORM]  = TEXTURE_FORMAT_R8;
        InternalPixelFormatTable[TEXTURE_PF_RG8_UNORM] = TEXTURE_FORMAT_RG8;
        //InternalPixelFormatTable[TEXTURE_PF_BGR8_UNORM] = TEXTURE_FORMAT_RGB8;
        InternalPixelFormatTable[TEXTURE_PF_BGRA8_UNORM] = TEXTURE_FORMAT_RGBA8;

        //InternalPixelFormatTable[TEXTURE_PF_BGR8_SRGB] = TEXTURE_FORMAT_SRGB8;
        InternalPixelFormatTable[TEXTURE_PF_BGRA8_SRGB] = TEXTURE_FORMAT_SRGB8_ALPHA8;

        InternalPixelFormatTable[TEXTURE_PF_R16I]  = TEXTURE_FORMAT_R16I;
        InternalPixelFormatTable[TEXTURE_PF_RG16I] = TEXTURE_FORMAT_RG16I;
        //InternalPixelFormatTable[TEXTURE_PF_BGR16I] = TEXTURE_FORMAT_RGB16I;
        InternalPixelFormatTable[TEXTURE_PF_BGRA16I] = TEXTURE_FORMAT_RGBA16I;

        InternalPixelFormatTable[TEXTURE_PF_R16UI]  = TEXTURE_FORMAT_R16UI;
        InternalPixelFormatTable[TEXTURE_PF_RG16UI] = TEXTURE_FORMAT_RG16UI;
        //InternalPixelFormatTable[TEXTURE_PF_BGR16UI] = TEXTURE_FORMAT_RGB16UI;
        InternalPixelFormatTable[TEXTURE_PF_BGRA16UI] = TEXTURE_FORMAT_RGBA16UI;

        InternalPixelFormatTable[TEXTURE_PF_R32I]    = TEXTURE_FORMAT_R32I;
        InternalPixelFormatTable[TEXTURE_PF_RG32I]   = TEXTURE_FORMAT_RG32I;
        InternalPixelFormatTable[TEXTURE_PF_BGR32I]  = TEXTURE_FORMAT_RGB32I;
        InternalPixelFormatTable[TEXTURE_PF_BGRA32I] = TEXTURE_FORMAT_RGBA32I;

        InternalPixelFormatTable[TEXTURE_PF_R32I]     = TEXTURE_FORMAT_R32UI;
        InternalPixelFormatTable[TEXTURE_PF_RG32UI]   = TEXTURE_FORMAT_RG32UI;
        InternalPixelFormatTable[TEXTURE_PF_BGR32UI]  = TEXTURE_FORMAT_RGB32UI;
        InternalPixelFormatTable[TEXTURE_PF_BGRA32UI] = TEXTURE_FORMAT_RGBA32UI;

        InternalPixelFormatTable[TEXTURE_PF_R16F]  = TEXTURE_FORMAT_R16F;
        InternalPixelFormatTable[TEXTURE_PF_RG16F] = TEXTURE_FORMAT_RG16F;
        //InternalPixelFormatTable[TEXTURE_PF_BGR16F] = TEXTURE_FORMAT_RGB16F;
        InternalPixelFormatTable[TEXTURE_PF_BGRA16F] = TEXTURE_FORMAT_RGBA16F;

        InternalPixelFormatTable[TEXTURE_PF_R32F]    = TEXTURE_FORMAT_R32F;
        InternalPixelFormatTable[TEXTURE_PF_RG32F]   = TEXTURE_FORMAT_RG32F;
        InternalPixelFormatTable[TEXTURE_PF_BGR32F]  = TEXTURE_FORMAT_RGB32F;
        InternalPixelFormatTable[TEXTURE_PF_BGRA32F] = TEXTURE_FORMAT_RGBA32F;

        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC1_RGB]        = TEXTURE_FORMAT_COMPRESSED_BC1_RGB;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC1_SRGB]       = TEXTURE_FORMAT_COMPRESSED_BC1_SRGB;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC2_RGBA]       = TEXTURE_FORMAT_COMPRESSED_BC2_RGBA;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC2_SRGB_ALPHA] = TEXTURE_FORMAT_COMPRESSED_BC2_SRGB_ALPHA;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC3_RGBA]       = TEXTURE_FORMAT_COMPRESSED_BC3_RGBA;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC3_SRGB_ALPHA] = TEXTURE_FORMAT_COMPRESSED_BC3_SRGB_ALPHA;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC4_R]          = TEXTURE_FORMAT_COMPRESSED_BC4_R;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC4_R_SIGNED]   = TEXTURE_FORMAT_COMPRESSED_BC4_R_SIGNED;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC5_RG]         = TEXTURE_FORMAT_COMPRESSED_BC5_RG;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC5_RG_SIGNED]  = TEXTURE_FORMAT_COMPRESSED_BC5_RG_SIGNED;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC6H]           = TEXTURE_FORMAT_COMPRESSED_BC6H;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC6H_SIGNED]    = TEXTURE_FORMAT_COMPRESSED_BC6H_SIGNED;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC7_RGBA]       = TEXTURE_FORMAT_COMPRESSED_BC7_RGBA;
        InternalPixelFormatTable[TEXTURE_PF_COMPRESSED_BC7_SRGB_ALPHA] = TEXTURE_FORMAT_COMPRESSED_BC7_SRGB_ALPHA;

        InternalPixelFormatTable[TEXTURE_PF_R11F_G11F_B10F] = TEXTURE_FORMAT_R11F_G11F_B10F;
    }
};

static STextureFormatMapper TextureFormatMapper;

HK_CLASS_META(ATexture)

ATexture::ATexture()
{
}

ATexture::~ATexture()
{
}

void ATexture::Purge()
{
}

bool ATexture::InitializeFromImage(AImage const& _Image)
{
    if (!_Image.GetData())
    {
        GLogger.Printf("ATexture::InitializeFromImage: empty image data\n");
        return false;
    }

    STexturePixelFormat pixelFormat;

    if (!STexturePixelFormat::GetAppropriatePixelFormat(_Image.GetPixelFormat(), pixelFormat))
    {
        return false;
    }

    Initialize2D(pixelFormat, _Image.GetNumMipLevels(), _Image.GetWidth(), _Image.GetHeight());

    byte* pSrc = (byte*)_Image.GetData();
    int   w, h, stride;
    int   pixelSizeInBytes = pixelFormat.SizeInBytesUncompressed();

    for (int lod = 0; lod < _Image.GetNumMipLevels(); lod++)
    {
        w = Math::Max(1, _Image.GetWidth() >> lod);
        h = Math::Max(1, _Image.GetHeight() >> lod);

        stride = w * h * pixelSizeInBytes;

        WriteTextureData2D(0, 0, w, h, lod, pSrc);

        pSrc += stride;
    }

    return true;
}

bool ATexture::InitializeCubemapFromImages(TArray<AImage const*, 6> const& _Faces)
{
    const void* faces[6];

    int width = _Faces[0]->GetWidth();

    for (int i = 0; i < 6; i++)
    {

        if (!_Faces[i]->GetData())
        {
            GLogger.Printf("ATexture::InitializeCubemapFromImages: empty image data\n");
            return false;
        }

        if (_Faces[i]->GetWidth() != width || _Faces[i]->GetHeight() != width)
        {
            GLogger.Printf("ATexture::InitializeCubemapFromImages: faces with different sizes\n");
            return false;
        }

        faces[i] = _Faces[i]->GetData();
    }

    STexturePixelFormat pixelFormat;

    if (!STexturePixelFormat::GetAppropriatePixelFormat(_Faces[0]->GetPixelFormat(), pixelFormat))
    {
        return false;
    }

    for (AImage const* faceImage : _Faces)
    {
        STexturePixelFormat facePF;

        if (!STexturePixelFormat::GetAppropriatePixelFormat(faceImage->GetPixelFormat(), facePF))
        {
            return false;
        }

        if (pixelFormat != facePF)
        {
            GLogger.Printf("ATexture::InitializeCubemapFromImages: faces with different pixel formats\n");
            return false;
        }
    }

    InitializeCubemap(pixelFormat, 1, width);

    for (int face = 0; face < 6; face++)
    {
        WriteTextureDataCubemap(0, 0, width, width, face, 0, (byte*)faces[face]);
    }

    return true;
}

void ATexture::LoadInternalResource(const char* _Path)
{
    if (!Platform::Stricmp(_Path, "/Default/Textures/White"))
    {
        const byte data[4] = {255, 255, 255, 255};

        Initialize2D(TEXTURE_PF_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/Black"))
    {
        const byte data[4] = {0, 0, 0, 255};

        Initialize2D(TEXTURE_PF_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/Gray"))
    {
        const byte data[4] = {127, 127, 127, 255};

        Initialize2D(TEXTURE_PF_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/BaseColorWhite") || !Platform::Stricmp(_Path, "/Default/Textures/Default2D"))
    {
        const byte data[4] = {240, 240, 240, 255};

        Initialize2D(TEXTURE_PF_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/BaseColorBlack"))
    {
        const byte data[4] = {30, 30, 30, 255};

        Initialize2D(TEXTURE_PF_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/Normal"))
    {
        const byte data[4] = {255, 127, 127, 255}; // Z Y X Alpha

        Initialize2D(TEXTURE_PF_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/DefaultCubemap"))
    {
        constexpr Float3 dirs[6] = {
            Float3(1, 0, 0),
            Float3(-1, 0, 0),
            Float3(0, 1, 0),
            Float3(0, -1, 0),
            Float3(0, 0, 1),
            Float3(0, 0, -1)};

        byte data[6][4];
        for (int i = 0; i < 6; i++)
        {
            data[i][0] = (dirs[i].Z + 1.0f) * 127.5f;
            data[i][1] = (dirs[i].Y + 1.0f) * 127.5f;
            data[i][2] = (dirs[i].X + 1.0f) * 127.5f;
            data[i][3] = 255;
        }

        InitializeCubemap(TEXTURE_PF_BGRA8_UNORM, 1, 1);

        for (int face = 0; face < 6; face++)
        {
            WriteTextureDataCubemap(0, 0, 1, 1, face, 0, data[face]);
        }
        return;
    }

#if 0
    if ( !Platform::Stricmp( _Path, "/Default/Textures/BlackCubemap" ) ) {
        const byte data[1] = {};

        InitializeCubemap( TEXTURE_PF_R8, 1, 1 );

        for ( int face = 0 ; face < 6 ; face++ ) {
            WriteTextureDataCubemap( 0, 0, 1, 1, face, 0, data );
        }
        return;
    }
#endif

    if (!Platform::Stricmp(_Path, "/Default/Textures/LUT1") || !Platform::Stricmp(_Path, "/Default/Textures/Default3D"))
    {

        constexpr SColorGradingPreset ColorGradingPreset1 = {
            Float3(0.5f), // Gain
            Float3(0.5f), // Gamma
            Float3(0.5f), // Lift
            Float3(1.0f), // Presaturation
            Float3(0.0f), // Color temperature strength
            6500.0f,      // Color temperature (in K)
            0.0f          // Color temperature brightness normalization factor
        };

        InitializeColorGradingLUT(ColorGradingPreset1);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/LUT2"))
    {
        constexpr SColorGradingPreset ColorGradingPreset2 = {
            Float3(0.5f), // Gain
            Float3(0.5f), // Gamma
            Float3(0.5f), // Lift
            Float3(1.0f), // Presaturation
            Float3(1.0f), // Color temperature strength
            3500.0f,      // Color temperature (in K)
            1.0f          // Color temperature brightness normalization factor
        };

        InitializeColorGradingLUT(ColorGradingPreset2);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/LUT3"))
    {
        constexpr SColorGradingPreset ColorGradingPreset3 = {
            Float3(0.51f, 0.55f, 0.53f), // Gain
            Float3(0.45f, 0.57f, 0.55f), // Gamma
            Float3(0.5f, 0.4f, 0.6f),    // Lift
            Float3(1.0f, 0.9f, 0.8f),    // Presaturation
            Float3(1.0f, 1.0f, 1.0f),    // Color temperature strength
            6500.0,                      // Color temperature (in K)
            0.0                          // Color temperature brightness normalization factor
        };

        InitializeColorGradingLUT(ColorGradingPreset3);

        return;
    }

    if (!Platform::Stricmp(_Path, "/Default/Textures/LUT_Luminance"))
    {
        byte* data = (byte*)GHunkMemory.Alloc(16 * 16 * 16 * 4);
        for (int z = 0; z < 16; z++)
        {
            byte* depth = data + (size_t)z * (16 * 16 * 4);
            for (int y = 0; y < 16; y++)
            {
                byte* row = depth + (size_t)y * (16 * 4);
                for (int x = 0; x < 16; x++)
                {
                    byte* pixel = row + (size_t)x * 4;
                    pixel[0] = pixel[1] = pixel[2] = Math::Clamp(x * (0.2126f / 15.0f * 255.0f) + y * (0.7152f / 15.0f * 255.0f) + z * (0.0722f / 15.0f * 255.0f), 0.0f, 255.0f);
                    pixel[3]                       = 255;
                }
            }
        }
        Initialize3D(TEXTURE_PF_BGRA8_SRGB, 1, 16, 16, 16);
        WriteArbitraryData(0, 0, 0, 16, 16, 16, 0, data);

        GHunkMemory.ClearLastHunk();

        return;
    }

    GLogger.Printf("Unknown internal texture %s\n", _Path);

    LoadInternalResource("/Default/Textures/Default2D");
}

static bool IsImageExtension(const char* pExtension)
{
    const char* extensionList[] = {".jpg",
                                   ".jpeg",
                                   ".png",
                                   ".tga",
                                   ".psd",
                                   ".gif",
                                   ".hdr",
                                   ".exr",
                                   ".pic",
                                   ".pnm",
                                   ".ppm",
                                   ".pgm"};

    for (int i = 0 ; i < HK_ARRAY_SIZE(extensionList) ; i++)
    {
        if (!Platform::Stricmp(pExtension, extensionList[i]))
            return true;
    }
    return false;
}

static bool IsHDRImageExtension(const char* pExtension)
{
    return !Platform::Stricmp(pExtension, ".hdr") || !Platform::Stricmp(pExtension, ".exr");
}

bool ATexture::LoadResource(IBinaryStream& Stream)
{
    const char* fn = Stream.GetFileName();

    AScopedTimer ScopedTime(fn);

    AImage image;

    int i = Platform::FindExt(fn);
    if (IsImageExtension(&fn[i]))
    {
        SImageMipmapConfig mipmapGen;
        mipmapGen.EdgeMode            = MIPMAP_EDGE_WRAP;
        mipmapGen.Filter              = MIPMAP_FILTER_MITCHELL;
        mipmapGen.bPremultipliedAlpha = false;

        if (IsHDRImageExtension(&fn[i]))
        {
            if (!image.Load(Stream, &mipmapGen, IMAGE_PF_AUTO_16F))
            {
                return false;
            }
        }
        else
        {
            if (!image.Load(Stream, &mipmapGen, IMAGE_PF_AUTO_GAMMA2))
            {
                return false;
            }
        }

        if (!InitializeFromImage(image))
        {
            return false;
        }
    }
    else
    {

        uint32_t fileFormat;
        uint32_t fileVersion;

        fileFormat = Stream.ReadUInt32();

        if (fileFormat != FMT_FILE_TYPE_TEXTURE)
        {
            GLogger.Printf("Expected file format %d\n", FMT_FILE_TYPE_TEXTURE);
            return false;
        }

        fileVersion = Stream.ReadUInt32();

        if (fileVersion != FMT_VERSION_TEXTURE)
        {
            GLogger.Printf("Expected file version %d\n", FMT_VERSION_TEXTURE);
            return false;
        }

        AString             guid;
        uint32_t            textureType;
        STexturePixelFormat texturePixelFormat;
        uint32_t            w, h, d, mipLevels;

        Stream.ReadObject(guid);
        textureType = Stream.ReadUInt32();
        Stream.ReadObject(texturePixelFormat);
        w         = Stream.ReadUInt32();
        h         = Stream.ReadUInt32();
        d         = Stream.ReadUInt32();
        mipLevels = Stream.ReadUInt32();

        switch (textureType)
        {
            case TEXTURE_1D:
                Initialize1D(texturePixelFormat, mipLevels, w);
                break;
            case TEXTURE_1D_ARRAY:
                Initialize1DArray(texturePixelFormat, mipLevels, w, h);
                break;
            case TEXTURE_2D:
                Initialize2D(texturePixelFormat, mipLevels, w, h);
                break;
            case TEXTURE_2D_ARRAY:
                Initialize2DArray(texturePixelFormat, mipLevels, w, h, d);
                break;
            case TEXTURE_3D:
                Initialize3D(texturePixelFormat, mipLevels, w, h, d);
                break;
            case TEXTURE_CUBEMAP:
                InitializeCubemap(texturePixelFormat, mipLevels, w);
                break;
            case TEXTURE_CUBEMAP_ARRAY:
                InitializeCubemapArray(texturePixelFormat, mipLevels, w, d);
                break;
            default:
                GLogger.Printf("ATexture::LoadResource: Unknown texture type %d\n", textureType);
                return false;
        }

        uint32_t lodWidth, lodHeight, lodDepth;
        size_t   pixelSize = texturePixelFormat.SizeInBytesUncompressed();
        size_t   maxSize   = (size_t)w * h * d * pixelSize;
        //byte * lodData = (byte *)GHunkMemory.HunkMemory( maxSize, 1 );
        byte* lodData = (byte*)GHeapMemory.Alloc(maxSize, 1);

        //int numLayers = 1;

        //if ( textureType == TEXTURE_CUBEMAP ) {
        //    numLayers = 6;
        //} else if ( textureType == TEXTURE_CUBEMAP_ARRAY ) {
        //    numLayers = d * 6;
        //}
        //
        //for ( int layerNum = 0 ; layerNum < numLayers ; layerNum++ ) {
        for (int n = 0; n < mipLevels; n++)
        {
            lodWidth  = Stream.ReadUInt32();
            lodHeight = Stream.ReadUInt32();
            lodDepth  = Stream.ReadUInt32();

            size_t size = (size_t)lodWidth * lodHeight * lodDepth * pixelSize;

            if (size > maxSize)
            {
                GLogger.Printf("ATexture::LoadResource invalid image %s\n", fn);
                break;
            }

            Stream.ReadBuffer(lodData, size);

            WriteArbitraryData(0, 0, /*layerNum*/ 0, lodWidth, lodHeight, lodDepth, n, lodData);
        }
        //}

        //GHunkMemory.ClearLastHunk();
        GHeapMemory.Free(lodData);

#if 0
        byte * buf = (byte *)GHeapMemory.Alloc( size );

        Stream.Read( buf, size );

        AMemoryStream ms;

        if ( !ms.OpenRead( _Path, buf, size ) ) {
            GHeapMemory.HeapFree( buf );
            return false;
        }

        if ( !image.LoadLDRI( ms, bSRGB, true ) ) {
            GHeapMemory.HeapFree( buf );
            return false;
        }

        GHeapMemory.HeapFree( buf );

        if ( !InitializeFromImage( image ) ) {
            return false;
        }
#endif
    }

    return true;
}

bool ATexture::IsCubemap() const
{
    return TextureType == TEXTURE_CUBEMAP || TextureType == TEXTURE_CUBEMAP_ARRAY;
}

size_t ATexture::TextureSizeInBytes1D(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _ArraySize)
{
    if (_PixelFormat.IsCompressed())
    {
        // TODO
        HK_ASSERT(0);
        return 0;
    }
    else
    {
        size_t sum = 0;
        for (int i = 0; i < _NumMipLevels; i++)
        {
            sum += Math::Max(1, _Width);
            _Width >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * Math::Max(_ArraySize, 1);
    }
}

size_t ATexture::TextureSizeInBytes2D(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _Height, int _ArraySize)
{
    if (_PixelFormat.IsCompressed())
    {
        // TODO
        HK_ASSERT(0);
        return 0;
    }
    else
    {
        size_t sum = 0;
        for (int i = 0; i < _NumMipLevels; i++)
        {
            sum += (size_t)Math::Max(1, _Width) * Math::Max(1, _Height);
            _Width >>= 1;
            _Height >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * Math::Max(_ArraySize, 1);
    }
}

size_t ATexture::TextureSizeInBytes3D(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _Height, int _Depth)
{
    if (_PixelFormat.IsCompressed())
    {
        // TODO
        HK_ASSERT(0);
        return 0;
    }
    else
    {
        size_t sum = 0;
        for (int i = 0; i < _NumMipLevels; i++)
        {
            sum += (size_t)Math::Max(1, _Width) * Math::Max(1, _Height) * Math::Max(1, _Depth);
            _Width >>= 1;
            _Height >>= 1;
            _Depth >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum;
    }
}

size_t ATexture::TextureSizeInBytesCubemap(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _ArraySize)
{
    if (_PixelFormat.IsCompressed())
    {
        // TODO
        HK_ASSERT(0);
        return 0;
    }
    else
    {
        size_t sum = 0;
        for (int i = 0; i < _NumMipLevels; i++)
        {
            sum += (size_t)Math::Max(1, _Width) * Math::Max(1, _Width);
            _Width >>= 1;
        }
        return _PixelFormat.SizeInBytesUncompressed() * sum * 6 * Math::Max(_ArraySize, 1);
    }
}

static void SetTextureSwizzle(STexturePixelFormat const& _PixelFormat, RenderCore::STextureSwizzle& _Swizzle)
{
    switch (_PixelFormat.NumComponents())
    {
        case 1:
            // Apply texture swizzle for single channel textures
            _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
            _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_R;
            _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
            _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_R;
            break;
#if 0
    case 2:
        // Apply texture swizzle for two channel textures
        _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_G;
        _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_G;
        break;
    case 3:
        // Apply texture swizzle for three channel textures
        _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_G;
        _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_B;
        _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_ONE;
        break;
#endif
    }
}

void ATexture::Initialize1D(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width)
{
    Purge();

    TextureType  = TEXTURE_1D;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = 1;
    Depth        = 1;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution1D(_Width));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

void ATexture::Initialize1DArray(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _ArraySize)
{
    Purge();

    TextureType  = TEXTURE_1D_ARRAY;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = _ArraySize;
    Depth        = 1;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution1DArray(_Width, _ArraySize));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

void ATexture::Initialize2D(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _Height)
{
    Purge();

    TextureType  = TEXTURE_2D;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = _Height;
    Depth        = 1;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution2D(_Width, _Height));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

void ATexture::Initialize2DArray(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _Height, int _ArraySize)
{
    Purge();

    TextureType  = TEXTURE_2D_ARRAY;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = _Height;
    Depth        = _ArraySize;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution2DArray(_Width, _Height, _ArraySize));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

void ATexture::Initialize3D(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _Height, int _Depth)
{
    Purge();

    TextureType  = TEXTURE_3D;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = _Height;
    Depth        = _Depth;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution3D(_Width, _Height, _Depth));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

void ATexture::InitializeColorGradingLUT(const char* _Path)
{
    AImage image;

    if (image.Load(_Path, nullptr, IMAGE_PF_BGRA_GAMMA2))
    {
        const byte* p = static_cast<const byte*>(image.GetData());

        byte* data = (byte*)GHunkMemory.Alloc(16 * 16 * 16 * 4);

        for (int y = 0; y < 16; y++)
        {
            for (int z = 0; z < 16; z++)
            {
                byte* row = data + (size_t)z * (16 * 16 * 4) + y * (16 * 4);

                Platform::Memcpy(row, p, 16 * 4);
                p += 16 * 4;
            }
        }

        Initialize3D(TEXTURE_PF_BGRA8_SRGB, 1, 16, 16, 16);
        WriteArbitraryData(0, 0, 0, 16, 16, 16, 0, data);

        GHunkMemory.ClearLastHunk();

        return;
    }

    LoadInternalResource("/Default/Textures/LUT_Luminance");
}

static Float3 ApplyColorGrading(SColorGradingPreset const& p, Color4 const& _Color)
{
    float lum = _Color.GetLuminance();

    Color4 mult;

    mult.SetTemperature(Math::Clamp(p.ColorTemperature, 1000.0f, 40000.0f));

    Color4 c;
    c.R = Math::Lerp(_Color.R, _Color.R * mult.R, p.ColorTemperatureStrength.X);
    c.G = Math::Lerp(_Color.G, _Color.G * mult.G, p.ColorTemperatureStrength.Y);
    c.B = Math::Lerp(_Color.B, _Color.B * mult.B, p.ColorTemperatureStrength.Z);
    c.A = 1;

    float newLum = c.GetLuminance();

    c *= Math::Lerp(1.0f, (newLum > 1e-6) ? (lum / newLum) : 1.0f, p.ColorTemperatureBrightnessNormalization);

    lum = c.GetLuminance();

    Float3 rgb;
    rgb.X = Math::Lerp(lum, c.R, p.Presaturation.X);
    rgb.Y = Math::Lerp(lum, c.G, p.Presaturation.Y);
    rgb.Z = Math::Lerp(lum, c.B, p.Presaturation.Z);

    rgb = (p.Gain * 2.0f) * (rgb + ((p.Lift * 2.0f - 1.0) * (Float3(1.0) - rgb)));

    rgb.X = Math::Pow(rgb.X, 0.5f / p.Gamma.X);
    rgb.Y = Math::Pow(rgb.Y, 0.5f / p.Gamma.Y);
    rgb.Z = Math::Pow(rgb.Z, 0.5f / p.Gamma.Z);

    return rgb;
}

void ATexture::InitializeColorGradingLUT(SColorGradingPreset const& _Preset)
{
    Color4 color;
    Float3  result;

    Initialize3D(TEXTURE_PF_BGRA8_SRGB, 1, 16, 16, 16);

    const float scale = 1.0f / 15.0f;

    byte* data = (byte*)GHunkMemory.Alloc(16 * 16 * 16 * 4);

    for (int z = 0; z < 16; z++)
    {
        color.B     = scale * z;
        byte* depth = data + (size_t)z * (16 * 16 * 4);
        for (int y = 0; y < 16; y++)
        {
            color.G   = scale * y;
            byte* row = depth + (size_t)y * (16 * 4);
            for (int x = 0; x < 16; x++)
            {
                color.R = scale * x;

                result = ApplyColorGrading(_Preset, color) * 255.0f;

                byte* pixel = row + (size_t)x * 4;
                pixel[0]    = Math::Clamp(result.Z, 0.0f, 255.0f);
                pixel[1]    = Math::Clamp(result.Y, 0.0f, 255.0f);
                pixel[2]    = Math::Clamp(result.X, 0.0f, 255.0f);
                pixel[3]    = 255;
            }
        }
    }

    WriteArbitraryData(0, 0, 0, 16, 16, 16, 0, data);

    GHunkMemory.ClearLastHunk();
}

void ATexture::InitializeCubemap(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width)
{
    Purge();

    TextureType  = TEXTURE_CUBEMAP;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = _Width;
    Depth        = 1;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolutionCubemap(_Width));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

void ATexture::InitializeCubemapArray(STexturePixelFormat _PixelFormat, int _NumMipLevels, int _Width, int _ArraySize)
{
    Purge();

    TextureType  = TEXTURE_CUBEMAP_ARRAY;
    PixelFormat  = _PixelFormat;
    Width        = _Width;
    Height       = _Width;
    Depth        = _ArraySize;
    NumMipLevels = _NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolutionCubemapArray(_Width, _ArraySize));
    textureDesc.SetFormat(TextureFormatMapper.InternalPixelFormatTable[_PixelFormat.Data]);
    textureDesc.SetMipLevels(_NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(_PixelFormat, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &TextureGPU);
}

size_t ATexture::GetSizeInBytes() const
{
    switch (TextureType)
    {
        case TEXTURE_1D:
            return ATexture::TextureSizeInBytes1D(PixelFormat, NumMipLevels, Width, 1);
        case TEXTURE_1D_ARRAY:
            return ATexture::TextureSizeInBytes1D(PixelFormat, NumMipLevels, Width, GetArraySize());
        case TEXTURE_2D:
            return ATexture::TextureSizeInBytes2D(PixelFormat, NumMipLevels, Width, Height, 1);
        case TEXTURE_2D_ARRAY:
            return ATexture::TextureSizeInBytes2D(PixelFormat, NumMipLevels, Width, Height, GetArraySize());
        case TEXTURE_3D:
            return ATexture::TextureSizeInBytes3D(PixelFormat, NumMipLevels, Width, Height, Depth);
        case TEXTURE_CUBEMAP:
            return ATexture::TextureSizeInBytesCubemap(PixelFormat, NumMipLevels, Width, 1);
        case TEXTURE_CUBEMAP_ARRAY:
            return ATexture::TextureSizeInBytesCubemap(PixelFormat, NumMipLevels, Width, GetArraySize());
    }
    return 0;
}

int ATexture::GetArraySize() const
{
    switch (TextureType)
    {
        case TEXTURE_1D_ARRAY:
            return Height;
        case TEXTURE_2D_ARRAY:
        case TEXTURE_CUBEMAP_ARRAY:
            return Depth;
    }
    return 1;
}

bool ATexture::WriteTextureData1D(int _LocationX, int _Width, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_1D && TextureType != TEXTURE_1D_ARRAY)
    {
        GLogger.Printf("ATexture::WriteTextureData1D: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, 0, 0, _Width, 1, 1, _Lod, _SysMem);
}

bool ATexture::WriteTextureData1DArray(int _LocationX, int _Width, int _ArrayLayer, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_1D_ARRAY)
    {
        GLogger.Printf("ATexture::WriteTextureData1DArray: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, 0, _ArrayLayer, _Width, 1, 1, _Lod, _SysMem);
}

bool ATexture::WriteTextureData2D(int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_2D && TextureType != TEXTURE_2D_ARRAY)
    {
        GLogger.Printf("ATexture::WriteTextureData2D: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, _LocationY, 0, _Width, _Height, 1, _Lod, _SysMem);
}

bool ATexture::WriteTextureData2DArray(int _LocationX, int _LocationY, int _Width, int _Height, int _ArrayLayer, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_2D_ARRAY)
    {
        GLogger.Printf("ATexture::WriteTextureData2DArray: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, _LocationY, _ArrayLayer, _Width, _Height, 1, _Lod, _SysMem);
}

bool ATexture::WriteTextureData3D(int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Depth, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_3D)
    {
        GLogger.Printf("ATexture::WriteTextureData3D: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, _LocationY, _LocationZ, _Width, _Height, _Depth, _Lod, _SysMem);
}

bool ATexture::WriteTextureDataCubemap(int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_CUBEMAP && TextureType != TEXTURE_CUBEMAP_ARRAY)
    {
        GLogger.Printf("ATexture::WriteTextureDataCubemap: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, _LocationY, _FaceIndex, _Width, _Height, 1, _Lod, _SysMem);
}

bool ATexture::WriteTextureDataCubemapArray(int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _ArrayLayer, int _Lod, const void* _SysMem)
{
    if (TextureType != TEXTURE_CUBEMAP_ARRAY)
    {
        GLogger.Printf("ATexture::WriteTextureDataCubemapArray: called for %s\n", TextureTypeName[TextureType]);
        return false;
    }
    return WriteArbitraryData(_LocationX, _LocationY, _ArrayLayer * 6 + _FaceIndex, _Width, _Height, 1, _Lod, _SysMem);
}

bool ATexture::WriteArbitraryData(int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Depth, int _Lod, const void* _SysMem)
{
    if (!Width)
    {
        GLogger.Printf("ATexture::WriteArbitraryData: texture is not initialized\n");
        return false;
    }

    size_t sizeInBytes = (size_t)_Width * _Height * _Depth;

    if (IsCompressed())
    {
        // TODO
        HK_ASSERT(0);
        return false;
    }
    else
    {
        sizeInBytes *= SizeInBytesUncompressed();
    }

    // TODO: bounds check?

    RenderCore::STextureRect rect;
    rect.Offset.X        = _LocationX;
    rect.Offset.Y        = _LocationY;
    rect.Offset.Z        = _LocationZ;
    rect.Offset.MipLevel = _Lod;
    rect.Dimension.X     = _Width;
    rect.Dimension.Y     = _Height;
    rect.Dimension.Z     = _Depth;

    TextureGPU->WriteRect(rect, TextureFormatMapper.PixelFormatTable[PixelFormat.Data], sizeInBytes, 1, _SysMem);

    return true;
}
