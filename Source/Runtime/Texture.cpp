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
#include "AssetImporter.h"
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
        "TEXTURE_CUBE",
        "TEXTURE_CUBE_ARRAY",
};

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

bool ATexture::InitializeFromImage(ImageStorage const& Image)
{
    if (!Image)
    {
        LOG("ATexture::InitializeFromImage: empty image data\n");
        return false;
    }

    TEXTURE_FORMAT format = Image.GetDesc().Format;

    switch (Image.GetDesc().Type)
    {
        case TEXTURE_1D:
            Initialize1D(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width);
            break;
        case TEXTURE_1D_ARRAY:
            Initialize1DArray(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width, Image.GetDesc().SliceCount);
            break;
        case TEXTURE_2D:
            Initialize2D(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width, Image.GetDesc().Height);
            break;
        case TEXTURE_2D_ARRAY:
            Initialize2DArray(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width, Image.GetDesc().Height, Image.GetDesc().SliceCount);
            break;
        case TEXTURE_3D:
            Initialize2DArray(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width, Image.GetDesc().Height, Image.GetDesc().Depth);
            break;
        case TEXTURE_CUBE:
            InitializeCubemap(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width);
            break;
        case TEXTURE_CUBE_ARRAY:
            InitializeCubemapArray(format, Image.GetDesc().NumMipmaps, Image.GetDesc().Width, Image.GetDesc().SliceCount / 6);
            break;
        default:
            HK_ASSERT(0);
    };

    for (uint32_t slice = 0; slice < Image.GetDesc().SliceCount; ++slice)
    {
        for (uint32_t mip = 0; mip < Image.GetDesc().NumMipmaps; ++mip)
        {
            ImageSubresourceDesc desc;
            desc.SliceIndex  = slice;
            desc.MipmapIndex = mip;

            ImageSubresource subresource = Image.GetSubresource(desc);

            WriteArbitraryData(0, 0, slice, subresource.GetWidth(), subresource.GetHeight(), 1, mip, subresource.GetData());
        }
    }

    return true;
}

void ATexture::LoadInternalResource(AStringView Path)
{
    if (!Path.Icmp("/Default/Textures/White"))
    {
        const byte data[4] = {255, 255, 255, 255};

        Initialize2D(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Path.Icmp("/Default/Textures/Black"))
    {
        const byte data[4] = {0, 0, 0, 255};

        Initialize2D(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Path.Icmp("/Default/Textures/Gray"))
    {
        const byte data[4] = {127, 127, 127, 255};

        Initialize2D(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Path.Icmp("/Default/Textures/BaseColorWhite") || !Path.Icmp("/Default/Textures/Default2D"))
    {
        const byte data[4] = {240, 240, 240, 255};

        Initialize2D(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Path.Icmp("/Default/Textures/BaseColorBlack"))
    {
        const byte data[4] = {30, 30, 30, 255};

        Initialize2D(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Path.Icmp("/Default/Textures/Normal"))
    {
        const byte data[4] = {255, 127, 127, 255}; // Z Y X Alpha

        Initialize2D(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1, 1);
        WriteTextureData2D(0, 0, 1, 1, 0, data);

        return;
    }

    if (!Path.Icmp("/Default/Textures/DefaultCubemap"))
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

        InitializeCubemap(TEXTURE_FORMAT_BGRA8_UNORM, 1, 1);

        for (int face = 0; face < 6; face++)
        {
            WriteTextureDataCubemap(0, 0, 1, 1, face, 0, data[face]);
        }
        return;
    }

    if (!Path.Icmp("/Default/Textures/LUT1") || !Path.Icmp("/Default/Textures/Default3D"))
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

    if (!Path.Icmp("/Default/Textures/LUT2"))
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

    if (!Path.Icmp("/Default/Textures/LUT3"))
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

    if (!Path.Icmp("/Default/Textures/LUT_Luminance"))
    {
        HeapBlob blob(16 * 16 * 16 * 4);
        byte*    data = (byte*)blob.GetData();
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
        Initialize3D(TEXTURE_FORMAT_SBGRA8_UNORM, 1, 16, 16, 16);
        WriteArbitraryData(0, 0, 0, 16, 16, 16, 0, data);
        return;
    }

    LOG("Unknown internal texture {}\n", Path);

    LoadInternalResource("/Default/Textures/Default2D");
}

static bool IsImageExtension(AStringView Extension)
{
    for (auto ext : {".jpg",
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
                     ".pgm"})
    {
        if (!Extension.Icmp(ext))
            return true;
    }
    return false;
}

bool ATexture::LoadResource(IBinaryStreamReadInterface& Stream)
{
    AString const& fn        = Stream.GetFileName();
    AStringView    extension = PathUtils::GetExt(fn);

    AScopedTimer ScopedTime(fn.CStr());

    ImageStorage image;

    if (IsImageExtension(extension))
    {
        ImageMipmapConfig mipmapGen;
        mipmapGen.EdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
        mipmapGen.Filter   = IMAGE_RESAMPLE_FILTER_MITCHELL;

        image = CreateImage(Stream, &mipmapGen, IMAGE_STORAGE_FLAGS_DEFAULT, TEXTURE_FORMAT_UNDEFINED);
        if (!image)
            return false;

        return InitializeFromImage(image);
    }

    uint32_t fileFormat;
    uint32_t fileVersion;

    fileFormat = Stream.ReadUInt32();

    if (fileFormat != FMT_FILE_TYPE_TEXTURE)
    {
        LOG("Expected file format {}\n", FMT_FILE_TYPE_TEXTURE);
        return false;
    }

    fileVersion = Stream.ReadUInt32();

    if (fileVersion != FMT_VERSION_TEXTURE)
    {
        LOG("Expected file version {}\n", FMT_VERSION_TEXTURE);
        return false;
    }

    Stream.ReadObject(image);

    return InitializeFromImage(image);
}

bool ATexture::IsCubemap() const
{
    return m_Type == TEXTURE_CUBE || m_Type == TEXTURE_CUBE_ARRAY;
}

static void SetTextureSwizzle(TEXTURE_FORMAT const& Format, RenderCore::STextureSwizzle& _Swizzle)
{
    TextureFormatInfo const& info = GetTextureFormatInfo(Format);

    int numChannels = 0;
    if (info.bHasRed)
        ++numChannels;
    if (info.bHasGreen)
        ++numChannels;
    if (info.bHasBlue)
        ++numChannels;
    if (info.bHasAlpha)
        ++numChannels;

    switch (numChannels)
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

void ATexture::Initialize1D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width)
{
    Purge();

    m_Type       = TEXTURE_1D;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = 1;
    m_Depth      = 1;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution1D(Width));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

void ATexture::Initialize1DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize)
{
    Purge();

    m_Type       = TEXTURE_1D_ARRAY;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = 1;
    m_Depth      = ArraySize;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution1DArray(Width, ArraySize));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

void ATexture::Initialize2D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height)
{
    Purge();

    m_Type       = TEXTURE_2D;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = Height;
    m_Depth      = 1;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution2D(Width, Height));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

void ATexture::Initialize2DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t ArraySize)
{
    Purge();

    m_Type       = TEXTURE_2D_ARRAY;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = Height;
    m_Depth      = ArraySize;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution2DArray(Width, Height, ArraySize));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

void ATexture::Initialize3D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t Depth)
{
    Purge();

    m_Type       = TEXTURE_3D;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = Height;
    m_Depth      = Depth;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolution3D(Width, Height, Depth));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

void ATexture::InitializeColorGradingLUT(IBinaryStreamReadInterface& Stream)
{
    ImageStorage image = CreateImage(Stream, nullptr, IMAGE_STORAGE_NO_ALPHA, TEXTURE_FORMAT_SBGRA8_UNORM);

    if (image && image.GetDesc().Width == 16 * 16 && image.GetDesc().Height == 16)
    {
        const byte* p = static_cast<const byte*>(image.GetData());

        HeapBlob blob(16 * 16 * 16 * 4);
        byte*    data = (byte*)blob.GetData();

        for (int y = 0; y < 16; y++)
        {
            for (int z = 0; z < 16; z++)
            {
                byte* row = data + (size_t)z * (16 * 16 * 4) + y * (16 * 4);

                Platform::Memcpy(row, p, 16 * 4);
                p += 16 * 4;
            }
        }

        Initialize3D(image.GetDesc().Format, 1, 16, 16, 16);
        WriteArbitraryData(0, 0, 0, 16, 16, 16, 0, data);

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

void ATexture::InitializeColorGradingLUT(SColorGradingPreset const& Preset)
{
    Color4 color;
    Float3  result;

    Initialize3D(TEXTURE_FORMAT_SBGRA8_UNORM, 1, 16, 16, 16);

    const float scale = 1.0f / 15.0f;

    HeapBlob blob(16 * 16 * 16 * 4);
    byte*    data = (byte*)blob.GetData();

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

                result = ApplyColorGrading(Preset, color) * 255.0f;

                byte* pixel = row + (size_t)x * 4;
                pixel[0]    = Math::Clamp(result.Z, 0.0f, 255.0f);
                pixel[1]    = Math::Clamp(result.Y, 0.0f, 255.0f);
                pixel[2]    = Math::Clamp(result.X, 0.0f, 255.0f);
                pixel[3]    = 255;
            }
        }
    }

    WriteArbitraryData(0, 0, 0, 16, 16, 16, 0, data);
}

void ATexture::InitializeCubemap(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width)
{
    Purge();

    m_Type       = TEXTURE_CUBE;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = Width;
    m_Depth      = 1;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolutionCubemap(Width));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

void ATexture::InitializeCubemapArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize)
{
    Purge();

    m_Type       = TEXTURE_CUBE_ARRAY;
    m_Format     = Format;
    m_Width      = Width;
    m_Height     = Width;
    m_Depth      = ArraySize;
    m_NumMipmaps = NumMipLevels;

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolutionCubemapArray(Width, ArraySize));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);
}

uint32_t ATexture::GetArraySize() const
{
    switch (m_Type)
    {
        case TEXTURE_1D_ARRAY:
        case TEXTURE_2D_ARRAY:
        case TEXTURE_CUBE_ARRAY:
            return m_Depth;
        default:
            break;
    }
    return 1;
}

bool ATexture::WriteTextureData1D(uint32_t LocationX, uint32_t Width, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_1D && m_Type != TEXTURE_1D_ARRAY)
    {
        LOG("ATexture::WriteTextureData1D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, 0, 0, Width, 1, 1, MipLevel, pData);
}

bool ATexture::WriteTextureData1DArray(uint32_t LocationX, uint32_t Width, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_1D_ARRAY)
    {
        LOG("ATexture::WriteTextureData1DArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, 0, ArrayLayer, Width, 1, 1, MipLevel, pData);
}

bool ATexture::WriteTextureData2D(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_2D && m_Type != TEXTURE_2D_ARRAY)
    {
        LOG("ATexture::WriteTextureData2D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, LocationY, 0, Width, Height, 1, MipLevel, pData);
}

bool ATexture::WriteTextureData2DArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_2D_ARRAY)
    {
        LOG("ATexture::WriteTextureData2DArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, LocationY, ArrayLayer, Width, Height, 1, MipLevel, pData);
}

bool ATexture::WriteTextureData3D(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_3D)
    {
        LOG("ATexture::WriteTextureData3D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, LocationY, LocationZ, Width, Height, Depth, MipLevel, pData);
}

bool ATexture::WriteTextureDataCubemap(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_CUBE && m_Type != TEXTURE_CUBE_ARRAY)
    {
        LOG("ATexture::WriteTextureDataCubemap: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, LocationY, FaceIndex, Width, Height, 1, MipLevel, pData);
}

bool ATexture::WriteTextureDataCubemapArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_CUBE_ARRAY)
    {
        LOG("ATexture::WriteTextureDataCubemapArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteArbitraryData(LocationX, LocationY, ArrayLayer * 6 + FaceIndex, Width, Height, 1, MipLevel, pData);
}

bool ATexture::WriteArbitraryData(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData)
{
    if (!m_Width)
    {
        LOG("ATexture::WriteArbitraryData: texture is not initialized\n");
        return false;
    }

    TextureFormatInfo const& info = GetTextureFormatInfo(m_Format);

    if (info.BlockSize > 1)
    {
        // Compressed 3D textures are not supported
        Depth = 1;
    }

    HK_ASSERT((LocationX % info.BlockSize) == 0);
    HK_ASSERT((LocationY % info.BlockSize) == 0);
    HK_ASSERT((Width % info.BlockSize) == 0);
    HK_ASSERT((Height % info.BlockSize) == 0);

    // TODO: bounds check?

    RenderCore::STextureRect rect;
    rect.Offset.X        = LocationX;
    rect.Offset.Y        = LocationY;
    rect.Offset.Z        = LocationZ;
    rect.Offset.MipLevel = MipLevel;
    rect.Dimension.X     = Width;
    rect.Dimension.Y     = Height;
    rect.Dimension.Z     = Depth;

    size_t rowWidth    = Width / info.BlockSize * info.BytesPerBlock;
    size_t sizeInBytes = rowWidth * Height / info.BlockSize * Depth;

    int rowAlignment;
    if (IsAligned(rowWidth, 8))
        rowAlignment = 8;
    else if (IsAligned(rowWidth, 4))
        rowAlignment = 4;
    else if (IsAligned(rowWidth, 2))
        rowAlignment = 2;
    else
        rowAlignment = 1;

    m_TextureGPU->WriteRect(rect, sizeInBytes, rowAlignment, pData);

    return true;
}

void ATexture::SetDebugName(AStringView DebugName)
{
    if (!m_TextureGPU)
    {
        LOG("ATexture::SetDebugName: texture must be initialized\n");
        return;
    }

    m_TextureGPU->SetDebugName(DebugName);
}
