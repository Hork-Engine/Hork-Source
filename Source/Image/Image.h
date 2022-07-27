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

#pragma once

#include "RawImage.h"
#include <Core/HeapBlob.h>

#include <Containers/Array.h>

enum TEXTURE_TYPE
{
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_CUBE,
    TEXTURE_CUBE_ARRAY,

    TEXTURE_TYPE_MAX
};

enum TEXTURE_FORMAT : uint8_t
{
    TEXTURE_FORMAT_UNDEFINED,

    TEXTURE_FORMAT_R8_UINT,
    TEXTURE_FORMAT_R8_SINT,
    TEXTURE_FORMAT_R8_UNORM,
    TEXTURE_FORMAT_R8_SNORM,

    TEXTURE_FORMAT_RG8_UINT,
    TEXTURE_FORMAT_RG8_SINT,
    TEXTURE_FORMAT_RG8_UNORM,
    TEXTURE_FORMAT_RG8_SNORM,

    TEXTURE_FORMAT_R16_UINT,
    TEXTURE_FORMAT_R16_SINT,
    TEXTURE_FORMAT_R16_UNORM,
    TEXTURE_FORMAT_R16_SNORM,
    TEXTURE_FORMAT_R16_FLOAT,

    TEXTURE_FORMAT_BGRA4_UNORM,
    TEXTURE_FORMAT_B5G6R5_UNORM,
    TEXTURE_FORMAT_B5G5R5A1_UNORM,

    TEXTURE_FORMAT_RGBA8_UINT,
    TEXTURE_FORMAT_RGBA8_SINT,
    TEXTURE_FORMAT_RGBA8_UNORM,
    TEXTURE_FORMAT_RGBA8_SNORM,
    TEXTURE_FORMAT_BGRA8_UNORM,
    TEXTURE_FORMAT_SRGBA8_UNORM,
    TEXTURE_FORMAT_SBGRA8_UNORM,

    TEXTURE_FORMAT_R10G10B10A2_UNORM,

    TEXTURE_FORMAT_R11G11B10_FLOAT,

    TEXTURE_FORMAT_RG16_UINT,
    TEXTURE_FORMAT_RG16_SINT,
    TEXTURE_FORMAT_RG16_UNORM,
    TEXTURE_FORMAT_RG16_SNORM,
    TEXTURE_FORMAT_RG16_FLOAT,

    TEXTURE_FORMAT_R32_UINT,
    TEXTURE_FORMAT_R32_SINT,
    TEXTURE_FORMAT_R32_FLOAT,

    TEXTURE_FORMAT_RGBA16_UINT,
    TEXTURE_FORMAT_RGBA16_SINT,
    TEXTURE_FORMAT_RGBA16_FLOAT,
    TEXTURE_FORMAT_RGBA16_UNORM,
    TEXTURE_FORMAT_RGBA16_SNORM,

    TEXTURE_FORMAT_RG32_UINT,
    TEXTURE_FORMAT_RG32_SINT,
    TEXTURE_FORMAT_RG32_FLOAT,
    TEXTURE_FORMAT_RGB32_UINT,
    TEXTURE_FORMAT_RGB32_SINT,
    TEXTURE_FORMAT_RGB32_FLOAT,
    TEXTURE_FORMAT_RGBA32_UINT,
    TEXTURE_FORMAT_RGBA32_SINT,
    TEXTURE_FORMAT_RGBA32_FLOAT,

    TEXTURE_FORMAT_D16,
    TEXTURE_FORMAT_D24S8,
    TEXTURE_FORMAT_X24G8_UINT,
    TEXTURE_FORMAT_D32,
    TEXTURE_FORMAT_D32S8,
    TEXTURE_FORMAT_X32G8_UINT,

    // RGB - 8 bytes per block
    TEXTURE_FORMAT_BC1_UNORM,
    TEXTURE_FORMAT_BC1_UNORM_SRGB,

    // RGB A-4bit / RGB (not the best quality, it is better to use BC3)  - 16 bytes per block
    TEXTURE_FORMAT_BC2_UNORM,
    TEXTURE_FORMAT_BC2_UNORM_SRGB,

    // RGB A-8bit - 16 bytes per block
    TEXTURE_FORMAT_BC3_UNORM,
    TEXTURE_FORMAT_BC3_UNORM_SRGB,

    // R single channel texture (use for metalmap, glossmap, etc)  - 8 bytes per block
    TEXTURE_FORMAT_BC4_UNORM,
    TEXTURE_FORMAT_BC4_SNORM,

    // RG two channel texture (use for normal map or two grayscale maps)  - 16 bytes per block
    TEXTURE_FORMAT_BC5_UNORM,
    TEXTURE_FORMAT_BC5_SNORM,

    // RGB half float HDR   - 16 bytes per block
    TEXTURE_FORMAT_BC6H_UFLOAT,
    TEXTURE_FORMAT_BC6H_SFLOAT,

    // RGB[A], best quality, every block is compressed different  - 16 bytes per block
    TEXTURE_FORMAT_BC7_UNORM,
    TEXTURE_FORMAT_BC7_UNORM_SRGB,

    TEXTURE_FORMAT_MAX,
};

enum TEXTURE_FORMAT_KIND : uint8_t
{
    TEXTURE_FORMAT_KIND_INTEGER,
    TEXTURE_FORMAT_KIND_NORMALIZED,
    TEXTURE_FORMAT_KIND_FLOAT,
    TEXTURE_FORMAT_KIND_DEPTHSTENCIL
};

enum IMAGE_DATA_TYPE : uint8_t
{
    IMAGE_DATA_TYPE_UNKNOWN,
    IMAGE_DATA_TYPE_UINT8,
    IMAGE_DATA_TYPE_UINT16,
    IMAGE_DATA_TYPE_UINT32,
    IMAGE_DATA_TYPE_FLOAT,
    IMAGE_DATA_TYPE_HALF,
    IMAGE_DATA_TYPE_ENCODED_R4G4B4A4,
    IMAGE_DATA_TYPE_ENCODED_R5G6B5,
    IMAGE_DATA_TYPE_ENCODED_R5G5B5A1,
    IMAGE_DATA_TYPE_ENCODED_R10G10B10A2,
    IMAGE_DATA_TYPE_ENCODED_R11G11B10F,
    IMAGE_DATA_TYPE_ENCODED_DEPTH,
    IMAGE_DATA_TYPE_COMPRESSED
};

enum NORMAL_MAP_PACK
{
    NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE          = 0,
    NORMAL_MAP_PACK_RG_BC5_COMPATIBLE            = 1,
    NORMAL_MAP_PACK_SPHEREMAP_BC5_COMPATIBLE     = 2,
    NORMAL_MAP_PACK_STEREOGRAPHIC_BC5_COMPATIBLE = 3,
    NORMAL_MAP_PACK_PARABOLOID_BC5_COMPATIBLE    = 4,
    NORMAL_MAP_PACK_RGBA_BC3_COMPATIBLE          = 5
};

struct TextureFormatInfo
{
    TEXTURE_FORMAT      Format;
    const char*         Name;
    uint8_t             BytesPerBlock;
    uint8_t             BlockSize;
    TEXTURE_FORMAT_KIND Kind;
    IMAGE_DATA_TYPE     DataType;
    bool                bHasRed : 1;
    bool                bHasGreen : 1;
    bool                bHasBlue : 1;
    bool                bHasAlpha : 1;
    bool                bHasDepth : 1;
    bool                bHasStencil : 1;
    bool                bSigned : 1;
    bool                bSRGB : 1;
};

TextureFormatInfo const& GetTextureFormatInfo(TEXTURE_FORMAT Format);

HK_FORCEINLINE bool IsCompressedFormat(TEXTURE_FORMAT Format)
{
    return Format >= TEXTURE_FORMAT_BC1_UNORM && Format <= TEXTURE_FORMAT_BC7_UNORM_SRGB;
}

HK_FORCEINLINE bool IsDepthStencilFormat(TEXTURE_FORMAT Format)
{
    auto& info = GetTextureFormatInfo(Format);
    return info.bHasDepth || info.bHasStencil;
}

enum IMAGE_STORAGE_FLAGS
{
    IMAGE_STORAGE_FLAGS_DEFAULT       = 0,
    IMAGE_STORAGE_NO_ALPHA            = 1,
    IMAGE_STORAGE_ALPHA_PREMULTIPLIED = 2,
};

HK_FLAG_ENUM_OPERATORS(IMAGE_STORAGE_FLAGS)

struct ImageStorageDesc
{
    TEXTURE_TYPE Type{TEXTURE_2D};
    uint32_t     Width{1};
    uint32_t     Height{1};
    union
    {
        uint32_t Depth{1};
        uint32_t SliceCount;
    };
    uint32_t NumMipmaps{1};

    TEXTURE_FORMAT Format{TEXTURE_FORMAT_UNDEFINED};

    IMAGE_STORAGE_FLAGS Flags{IMAGE_STORAGE_FLAGS_DEFAULT};
};

struct ImageSubresourceDesc
{
    uint32_t SliceIndex{};
    uint32_t MipmapIndex{};
};

class ImageSubresource
{
public:
    ImageSubresource() = default;

    operator bool() const
    {
        return m_SizeInBytes != 0;
    }

    ImageSubresourceDesc const& GetDesc() const { return m_Desc; }

    void* GetData()
    {
        return m_pData;
    }

    void const* GetData() const
    {
        return m_pData;
    }

    size_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }

    uint32_t GetWidth() const
    {
        return m_Width;
    }

    uint32_t GetHeight() const
    {
        return m_Height;
    }

    bool Write(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height, void const* Bytes);

    bool Read(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height, void* Bytes, size_t _SizeInBytes) const;

    bool IsCompressed() const
    {
        return IsCompressedFormat(m_Format);
    }

    int NumChannels() const;

    size_t GetBytesPerPixel() const;

    size_t GetBlockSizeInBytes() const;

    IMAGE_DATA_TYPE GetDataType() const;

    ImageSubresource NextSlice() const;

private:
    ImageSubresourceDesc m_Desc;
    uint8_t*       m_pData{};
    size_t         m_SizeInBytes{};
    uint32_t       m_Width{};
    uint32_t       m_Height{};
    uint32_t       m_SliceCount{};
    TEXTURE_FORMAT m_Format{TEXTURE_FORMAT_UNDEFINED};

    friend class ImageStorage;
};

struct TextureOffset
{
    uint32_t X{};
    uint32_t Y{};
    uint32_t Z{};
    uint32_t MipLevel{};
};

/** Resampling edge mode for 2D/Array textures. */
enum IMAGE_RESAMPLE_EDGE_MODE
{
    IMAGE_RESAMPLE_EDGE_CLAMP     = 1,
    IMAGE_RESAMPLE_EDGE_REFLECT = 2,
    IMAGE_RESAMPLE_EDGE_WRAP    = 3,
    IMAGE_RESAMPLE_EDGE_ZERO    = 4,
};

/** Resampling filter for 2D/Array textures. */
enum IMAGE_RESAMPLE_FILTER
{
    /** A trapezoid w/1-pixel wide ramps, same result as box for integer scale ratios */
    IMAGE_RESAMPLE_FILTER_BOX = 1,
    /** On upsampling, produces same results as bilinear texture filtering */
    IMAGE_RESAMPLE_FILTER_TRIANGLE = 2,
    /** The cubic b-spline (aka Mitchell-Netrevalli with B=1,C=0), gaussian-esque */
    IMAGE_RESAMPLE_FILTER_CUBICBSPLINE = 3,
    /** An interpolating cubic spline */
    IMAGE_RESAMPLE_FILTER_CATMULLROM = 4,
    /** Mitchell-Netrevalli filter with B=1/3, C=1/3 */
    IMAGE_RESAMPLE_FILTER_MITCHELL = 5
};

/** Resampling filter for 3D textures (Not yet implemented. Reserved for future.) */
enum IMAGE_RESAMPLE_FILTER_3D
{
    IMAGE_RESAMPLE_FILTER_3D_AVERAGE = 1,
    IMAGE_RESAMPLE_FILTER_3D_MIN     = 2,
    IMAGE_RESAMPLE_FILTER_3D_MAX     = 3,
};

struct ImageMipmapConfig
{
    /** Resampling edge mode for 2D/Array textures. */
    IMAGE_RESAMPLE_EDGE_MODE EdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;

    /** Resampling filter for 2D/Array textures. */
    IMAGE_RESAMPLE_FILTER    Filter   = IMAGE_RESAMPLE_FILTER_MITCHELL;

    /** Resampling filter for 3D textures (Not yet implemented. Reserved for future.) */
    IMAGE_RESAMPLE_FILTER_3D Filter3D = IMAGE_RESAMPLE_FILTER_3D_AVERAGE;
};

class ImageStorage
{
public:
    ImageStorage() = default;

    ImageStorage(ImageStorageDesc const& _Desc)
    {
        Reset(_Desc);
    }

    virtual ~ImageStorage() = default;

    ImageStorage(ImageStorage const& Rhs) = delete;
    ImageStorage& operator=(ImageStorage const& Rhs) = delete;

    ImageStorage(ImageStorage&& Rhs) = default;

    ImageStorage& operator=(ImageStorage&& Rhs) = default;

    ImageSubresource operator[](uint32_t SliceNum)
    {
        return GetSubresource({SliceNum, 0});
    }

    const ImageSubresource operator[](uint32_t SliceNum) const
    {
        return GetSubresource({SliceNum, 0});
    }

    void Swap(ImageStorage& Rhs)
    {
        ImageStorage temp = std::move(Rhs);
        Rhs               = std::move(*this);
        *this             = std::move(temp);
    }

    void Reset(ImageStorageDesc const& Desc);

    void Reset();

    bool WriteSubresource(TextureOffset const& Offset, uint32_t Width, uint32_t Height, void const* Bytes);

    bool ReadSubresource(TextureOffset const& Offset, uint32_t Width, uint32_t Height, void* Bytes, size_t _SizeInBytes) const;

    ImageSubresource GetSubresource(ImageSubresourceDesc const& Desc) const;

    void* GetData()
    {
        return m_Data.GetData();
    }

    void const* GetData() const
    {
        return m_Data.GetData();
    }

    size_t GetSizeInBytes() const
    {
        return m_Data.Size();
    }

    bool IsEmpty() const
    {
        return m_Data.IsEmpty();
    }

    operator bool() const
    {
        return !m_Data.IsEmpty();
    }

    ImageStorageDesc const& GetDesc() const
    {
        return m_Desc;
    }

    bool IsCompressed() const
    {
        return IsCompressedFormat(m_Desc.Format);
    }

    int NumChannels() const;

    size_t GetBytesPerPixel() const;

    size_t GetBlockSizeInBytes() const;

    IMAGE_DATA_TYPE GetDataType() const;

    bool GenerateMipmaps(uint32_t SliceIndex, ImageMipmapConfig const& MipmapConfig);
    bool GenerateMipmaps(ImageMipmapConfig const& MipmapConfig);

    void Write(IBinaryStreamWriteInterface& stream) const;
    void Read(IBinaryStreamReadInterface& stream);

private:
    bool GenerateMipmaps3D(ImageMipmapConfig const& MipmapConfig);

    HeapBlob         m_Data;
    ImageStorageDesc m_Desc;
};

/*

Utilites

*/

struct ImageResampleParams
{
    /** Source image */
    const void* pImage = nullptr;
    /** Data format */
    TEXTURE_FORMAT Format = TEXTURE_FORMAT_UNDEFINED;
    /** Source image width */
    uint32_t Width = 0;
    /** Source image height */
    uint32_t Height = 0;
    /** Is image has alpha. If enabled, the last texture channel is interpreted as alpha. */
    bool bHasAlpha = false;
    /** Set this flag if your image has premultiplied alpha. Otherwise, will be
    used alpha-weighted resampling (effectively premultiplying, resampling, then unpremultiplying). */
    bool bPremultipliedAlpha = false;
    /** Scaling edge mode for horizontal axis */
    IMAGE_RESAMPLE_EDGE_MODE HorizontalEdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
    /** Scaling edge mode for vertical axis */
    IMAGE_RESAMPLE_EDGE_MODE VerticalEdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
    /** Scaling filter for horizontal axis */
    IMAGE_RESAMPLE_FILTER HorizontalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;
    /** Scaling filter for vertical axis */
    IMAGE_RESAMPLE_FILTER VerticalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;
    /** Scaled image width */
    uint32_t ScaledWidth = 0;
    /** Scaled image height */
    uint32_t ScaledHeight = 0;
};

/** Scale image. */
bool ResampleImage(ImageResampleParams const& Desc, void* pDest);

enum RAW_IMAGE_RESAMPLE_FLAG
{
    RAW_IMAGE_RESAMPLE_FLAG_DEFAULT        = 0,
    RAW_IMAGE_RESAMPLE_HAS_ALPHA           = HK_BIT(0),
    RAW_IMAGE_RESAMPLE_ALPHA_PREMULTIPLIED = HK_BIT(1),
    RAW_IMAGE_RESAMPLE_COLORSPACE_SRGB     = HK_BIT(2),
};
HK_FLAG_ENUM_OPERATORS(RAW_IMAGE_RESAMPLE_FLAG);

struct RawImageResampleParams
{
    RAW_IMAGE_RESAMPLE_FLAG Flags = RAW_IMAGE_RESAMPLE_FLAG_DEFAULT;
    /** Scaling edge mode for horizontal axis */
    IMAGE_RESAMPLE_EDGE_MODE HorizontalEdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
    /** Scaling edge mode for vertical axis */
    IMAGE_RESAMPLE_EDGE_MODE VerticalEdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
    /** Scaling filter for horizontal axis */
    IMAGE_RESAMPLE_FILTER HorizontalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;
    /** Scaling filter for vertical axis */
    IMAGE_RESAMPLE_FILTER VerticalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;
    /** Scaled image width */
    uint32_t ScaledWidth = 0;
    /** Scaled image height */
    uint32_t ScaledHeight = 0;
};

/** Scale image. */
ARawImage ResampleRawImage(ARawImage const& Source, RawImageResampleParams const& Desc);

enum IMAGE_IMPORT_FLAGS
{
    IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT        = HK_BIT(0),
    IMAGE_IMPORT_USE_COMPRESSION                 = HK_BIT(1),
    IMAGE_IMPORT_ALLOW_HDRI_COMPRESSION          = HK_BIT(2),
    IMAGE_IMPORT_ASSUME_8BIT_RGB_IMAGES_ARE_SRGB = HK_BIT(3),
    IMAGE_IMPORT_FLAGS_DEFAULT                   = IMAGE_IMPORT_STORE_HDRI_AS_HALF_FLOAT | IMAGE_IMPORT_ASSUME_8BIT_RGB_IMAGES_ARE_SRGB
};
HK_FLAG_ENUM_OPERATORS(IMAGE_IMPORT_FLAGS)

/** Create image storage from raw image */
ImageStorage CreateImage(ARawImage const& rawImage, ImageMipmapConfig const* pMipmapConfig, IMAGE_STORAGE_FLAGS Flags = IMAGE_STORAGE_FLAGS_DEFAULT, IMAGE_IMPORT_FLAGS ImportFlags = IMAGE_IMPORT_FLAGS_DEFAULT);

/** Create image storage from file */
ImageStorage CreateImage(IBinaryStreamReadInterface& Stream, ImageMipmapConfig const* pMipmapConfig = nullptr, IMAGE_STORAGE_FLAGS Flags = IMAGE_STORAGE_FLAGS_DEFAULT, TEXTURE_FORMAT Format = TEXTURE_FORMAT_UNDEFINED);

/** Create image storage from file */
ImageStorage CreateImage(AStringView FileName, ImageMipmapConfig const* pMipmapConfig = nullptr, IMAGE_STORAGE_FLAGS Flags = IMAGE_STORAGE_FLAGS_DEFAULT, TEXTURE_FORMAT Format = TEXTURE_FORMAT_UNDEFINED);

ImageStorage CreateNormalMap(IBinaryStreamReadInterface& Stream, NORMAL_MAP_PACK Pack, bool bUseCompression, bool bConvertFromDirectXNormalMap, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode);
ImageStorage CreateNormalMap(AStringView FileName, NORMAL_MAP_PACK Pack, bool bUseCompression, bool bConvertFromDirectXNormalMap, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode);

// Assume normals already normalized. The width and height of the normal map must be a multiple of blockSize if compression is enabled.
ImageStorage CreateNormalMap(Float3 const* pNormals, uint32_t Width, uint32_t Height, NORMAL_MAP_PACK Pack, bool bUseCompression, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode, IMAGE_RESAMPLE_FILTER ResampleFilter);

// The width and height of the roughness map must be a multiple of blockSize if compression is enabled.
ImageStorage CreateRoughnessMap(uint8_t const* pRoughnessMap, uint32_t Width, uint32_t Height, bool bUseCompression, IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode, IMAGE_RESAMPLE_FILTER ResampleFilter);

struct NormalRoughnessImportSettings
{
    AStringView NormalMap;
    AStringView RoughnessMap;

    // Result normal map packing
    NORMAL_MAP_PACK Pack = NORMAL_MAP_PACK_RGBA_BC1_COMPATIBLE;

    bool bConvertFromDirectXNormalMap = false;

    // Compress normal map: BC1, BC3 or BC5 - depends on packing type.
    bool bCompressNormals = false;

    // Compress roughness map (BC4)
    bool bCompressRoughness_BC4 = false;

    // Mipmapping
    IMAGE_RESAMPLE_EDGE_MODE ResampleEdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
};
// Output normal map in format RGB32_FLOAT or RG32_FLOAT (depends on pack), roughness map - R32_FLOAT
bool CreateNormalAndRoughness(NormalRoughnessImportSettings const& Settings, ImageStorage& NormalMapImage, ImageStorage& RoughnessMapImage);

struct SkyboxImportSettings
{
    /** Source files for skybox */
    TArray<AString, 6> Faces;

    /** Import skybox as HDRI image */
    bool bHDRI{false};

    float HDRIScale{1};
    float HDRIPow{1};
};

ImageStorage LoadSkyboxImages(SkyboxImportSettings const& Settings);

uint32_t CalcNumMips(TEXTURE_FORMAT Format, uint32_t Width, uint32_t Height, uint32_t Depth = 1);
