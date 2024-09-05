/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "TextureView.h"

HK_NAMESPACE_BEGIN

namespace RenderCore
{

enum TEXTURE_SWIZZLE : uint8_t
{
    TEXTURE_SWIZZLE_IDENTITY = 0,
    TEXTURE_SWIZZLE_ZERO     = 1,
    TEXTURE_SWIZZLE_ONE      = 2,
    TEXTURE_SWIZZLE_R        = 3,
    TEXTURE_SWIZZLE_G        = 4,
    TEXTURE_SWIZZLE_B        = 5,
    TEXTURE_SWIZZLE_A        = 6
};

struct TextureResolution
{
    uint32_t Width{};
    uint32_t Height{};
    uint32_t SliceCount{};

    bool operator==(TextureResolution const& Rhs) const
    {
        // clang-format off
        return Width      == Rhs.Width &&
               Height     == Rhs.Height &&
               SliceCount == Rhs.SliceCount;
        // clang-format on
    }

    bool operator!=(TextureResolution const& Rhs) const
    {
        return !(operator==(Rhs));
    }
};

struct TextureResolution1D : TextureResolution
{
    TextureResolution1D() = default;
    TextureResolution1D(uint32_t InWidth)
    {
        Width      = InWidth;
        Height     = 1;
        SliceCount = 1;
    }
};

struct TextureResolution1DArray : TextureResolution
{
    TextureResolution1DArray() = default;
    TextureResolution1DArray(uint32_t InWidth, uint32_t InNumLayers)
    {
        Width      = InWidth;
        Height     = 1;
        SliceCount = InNumLayers;
    }
};

struct TextureResolution2D : TextureResolution
{
    TextureResolution2D() = default;
    TextureResolution2D(uint32_t InWidth, uint32_t InHeight)
    {
        Width      = InWidth;
        Height     = InHeight;
        SliceCount = 1;
    }
};

struct TextureResolution2DArray : TextureResolution
{
    TextureResolution2DArray() = default;
    TextureResolution2DArray(uint32_t InWidth, uint32_t InHeight, uint32_t InNumLayers)
    {
        Width      = InWidth;
        Height     = InHeight;
        SliceCount = InNumLayers;
    }
};

struct TextureResolution3D : TextureResolution
{
    TextureResolution3D() = default;
    TextureResolution3D(uint32_t InWidth, uint32_t InHeight, uint32_t InDepth)
    {
        Width      = InWidth;
        Height     = InHeight;
        SliceCount = InDepth;
    }
};

struct TextureResolutionCubemap : TextureResolution
{
    TextureResolutionCubemap() = default;
    TextureResolutionCubemap(uint32_t InWidth)
    {
        Width      = InWidth;
        Height     = InWidth;
        SliceCount = 6;
    }
};

struct TextureResolutionCubemapArray : TextureResolution
{
    TextureResolutionCubemapArray() = default;
    TextureResolutionCubemapArray(uint32_t InWidth, uint32_t InNumLayers)
    {
        Width      = InWidth;
        Height     = InWidth;
        SliceCount = InNumLayers * 6;
    }
};

struct TextureOffset
{
    uint16_t MipLevel = 0;
    uint16_t X        = 0;
    uint16_t Y        = 0;
    uint16_t Z        = 0;
};

struct TextureDimension
{
    uint16_t X = 0;
    uint16_t Y = 0;
    uint16_t Z = 0;
};

struct TextureRect
{
    TextureOffset    Offset;
    TextureDimension Dimension;
};

struct TextureCopy
{
    TextureRect   SrcRect;
    TextureOffset DstOffset;
};

struct TextureMultisampleInfo
{
    /// The number of samples in the multisample texture's image.
    uint8_t NumSamples = 1;

    /// Specifies whether the image will use identical sample locations
    /// and the same number of samples for all texels in the image,
    /// and the sample locations will not depend on the internal format or size of the image.
    bool bFixedSampleLocations = false;

    TextureMultisampleInfo() = default;

    bool operator==(TextureMultisampleInfo const& Rhs) const
    {
        return NumSamples == Rhs.NumSamples && bFixedSampleLocations == Rhs.bFixedSampleLocations;
    }

    bool operator!=(TextureMultisampleInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    TextureMultisampleInfo& SetSamples(int InNumSamples)
    {
        NumSamples = InNumSamples;
        return *this;
    }

    TextureMultisampleInfo& SetFixedSampleLocations(bool InbFixedSampleLocations)
    {
        bFixedSampleLocations = InbFixedSampleLocations;
        return *this;
    }
};

struct TextureSwizzle
{
    uint8_t R = TEXTURE_SWIZZLE_IDENTITY;
    uint8_t G = TEXTURE_SWIZZLE_IDENTITY;
    uint8_t B = TEXTURE_SWIZZLE_IDENTITY;
    uint8_t A = TEXTURE_SWIZZLE_IDENTITY;

    TextureSwizzle() = default;

    TextureSwizzle(uint8_t InR,
                    uint8_t InG,
                    uint8_t InB,
                    uint8_t InA) :
        R(InR), G(InG), B(InB), A(InA)
    {
    }

    bool operator==(TextureSwizzle const& Rhs) const
    {
        return std::memcmp(this, &Rhs, sizeof(*this)) == 0;
    }

    bool operator!=(TextureSwizzle const& Rhs) const
    {
        return !(operator==(Rhs));
    }
};

enum BIND_FLAG : uint16_t
{
    BIND_NONE             = 0,
    BIND_VERTEX_BUFFER    = 1 << 0, // TODO
    BIND_INDEX_BUFFER     = 1 << 1, // TODO
    BIND_CONSTANT_BUFFER  = 1 << 2, // TODO
    BIND_SHADER_RESOURCE  = 1 << 3,
    BIND_STREAM_OUTPUT    = 1 << 4, // TODO
    BIND_RENDER_TARGET    = 1 << 5,
    BIND_DEPTH_STENCIL    = 1 << 6,
    BIND_UNORDERED_ACCESS = 1 << 7
};

HK_FLAG_ENUM_OPERATORS(BIND_FLAG)

struct TextureDesc
{
    TEXTURE_TYPE           Type       = TEXTURE_2D;
    TEXTURE_FORMAT         Format     = TEXTURE_FORMAT_RGBA8_UNORM;
    BIND_FLAG              BindFlags  = BIND_NONE;
    TextureResolution      Resolution = {};
    TextureMultisampleInfo Multisample;
    TextureSwizzle         Swizzle;
    uint16_t               NumMipLevels = 1;

    TextureDesc() = default;

    bool operator==(TextureDesc const& Rhs) const
    {
        // clang-format off
        return Type        == Rhs.Type &&
               Format      == Rhs.Format &&
               BindFlags   == Rhs.BindFlags &&
               Resolution  == Rhs.Resolution &&
               Multisample == Rhs.Multisample &&
               Swizzle     == Rhs.Swizzle &&
               NumMipLevels== Rhs.NumMipLevels;
        // clang-format on
    }

    bool operator!=(TextureDesc const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    TextureDesc& SetFormat(TEXTURE_FORMAT InFormat)
    {
        Format = InFormat;
        return *this;
    }

    TextureDesc& SetBindFlags(BIND_FLAG InBindFlags)
    {
        BindFlags = InBindFlags;
        return *this;
    }

    TextureDesc& SetMultisample(TextureMultisampleInfo const& InMultisample)
    {
        Multisample = InMultisample;
        return *this;
    }

    TextureDesc& SetSwizzle(TextureSwizzle const& InSwizzle)
    {
        Swizzle = InSwizzle;
        return *this;
    }

    TextureDesc& SetMipLevels(int InNumMipLevels)
    {
        NumMipLevels = InNumMipLevels;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolution1D const& InResolution)
    {
        Type       = TEXTURE_1D;
        Resolution = InResolution;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolution1DArray const& InResolution)
    {
        Type       = TEXTURE_1D_ARRAY;
        Resolution = InResolution;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolution2D const& InResolution)
    {
        Type       = TEXTURE_2D;
        Resolution = InResolution;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolution2DArray const& InResolution)
    {
        Type       = TEXTURE_2D_ARRAY;
        Resolution = InResolution;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolution3D const& InResolution)
    {
        Type       = TEXTURE_3D;
        Resolution = InResolution;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolutionCubemap const& InResolution)
    {
        Type       = TEXTURE_CUBE;
        Resolution = InResolution;
        return *this;
    }

    TextureDesc& SetResolution(TextureResolutionCubemapArray const& InResolution)
    {
        Type       = TEXTURE_CUBE_ARRAY;
        Resolution = InResolution;
        return *this;
    }
};

struct TextureMipLevelInfo
{
    TextureResolution Resoultion                = {};
    bool              bCompressed               = false;
    size_t            CompressedDataSizeInBytes = 0;
};

enum COMPARISON_FUNCTION : uint8_t
{
    CMPFUNC_NEVER     = 0,
    CMPFUNC_LESS      = 1,
    CMPFUNC_EQUAL     = 2,
    CMPFUNC_LEQUAL    = 3,
    CMPFUNC_GREATER   = 4,
    CMPFUNC_NOT_EQUAL = 5,
    CMPFUNC_GEQUAL    = 6,
    CMPFUNC_ALWAYS    = 7
};

//
// Sampler state info
//

enum SAMPLER_FILTER : uint8_t
{
    FILTER_MIN_NEAREST_MAG_NEAREST = 0,
    FILTER_MIN_LINEAR_MAG_NEAREST,
    FILTER_MIN_NEAREST_MIPMAP_NEAREST_MAG_NEAREST,
    FILTER_MIN_LINEAR_MIPMAP_NEAREST_MAG_NEAREST,
    FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_NEAREST,
    FILTER_MIN_LINEAR_MIPMAP_LINEAR_MAG_NEAREST,

    FILTER_MIN_NEAREST_MAG_LINEAR,
    FILTER_MIN_LINEAR_MAG_LINEAR,
    FILTER_MIN_NEAREST_MIPMAP_NEAREST_MAG_LINEAR,
    FILTER_MIN_LINEAR_MIPMAP_NEAREST_MAG_LINEAR,
    FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_LINEAR,
    FILTER_MIN_LINEAR_MIPMAP_LINEAR_MAG_LINEAR,

    FILTER_NEAREST          = FILTER_MIN_NEAREST_MAG_NEAREST,
    FILTER_LINEAR           = FILTER_MIN_LINEAR_MAG_LINEAR,
    FILTER_MIPMAP_NEAREST   = FILTER_MIN_NEAREST_MIPMAP_NEAREST_MAG_NEAREST,
    FILTER_MIPMAP_BILINEAR  = FILTER_MIN_LINEAR_MIPMAP_NEAREST_MAG_LINEAR,
    FILTER_MIPMAP_NLINEAR   = FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_NEAREST,
    FILTER_MIPMAP_TRILINEAR = FILTER_MIN_LINEAR_MIPMAP_LINEAR_MAG_LINEAR
};

enum SAMPLER_ADDRESS_MODE : uint8_t
{
    SAMPLER_ADDRESS_WRAP        = 0,
    SAMPLER_ADDRESS_MIRROR      = 1,
    SAMPLER_ADDRESS_CLAMP       = 2,
    SAMPLER_ADDRESS_BORDER      = 3,
    SAMPLER_ADDRESS_MIRROR_ONCE = 4
};

struct SamplerDesc
{
    /// Filtering method to use when sampling a texture
    SAMPLER_FILTER Filter = FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_LINEAR;

    SAMPLER_ADDRESS_MODE AddressU = SAMPLER_ADDRESS_WRAP;
    SAMPLER_ADDRESS_MODE AddressV = SAMPLER_ADDRESS_WRAP;
    SAMPLER_ADDRESS_MODE AddressW = SAMPLER_ADDRESS_WRAP;

    uint8_t MaxAnisotropy = 0;

    /// a function that compares sampled data against existing sampled data
    COMPARISON_FUNCTION ComparisonFunc = CMPFUNC_LEQUAL;

    bool bCompareRefToTexture = false;
    bool bCubemapSeamless     = false;

    float MipLODBias     = 0;
    float MinLOD         = 0;
    float MaxLOD         = 1000.0f;
    float BorderColor[4] = {0, 0, 0, 0};

    SamplerDesc& SetFilter(SAMPLER_FILTER InFilter)
    {
        Filter = InFilter;
        return *this;
    }

    SamplerDesc& SetAddress(SAMPLER_ADDRESS_MODE InAddress)
    {
        AddressU = InAddress;
        AddressV = InAddress;
        AddressW = InAddress;
        return *this;
    }

    SamplerDesc& SetAddressU(SAMPLER_ADDRESS_MODE InAddressU)
    {
        AddressU = InAddressU;
        return *this;
    }

    SamplerDesc& SetAddressV(SAMPLER_ADDRESS_MODE InAddressV)
    {
        AddressV = InAddressV;
        return *this;
    }

    SamplerDesc& SetAddressW(SAMPLER_ADDRESS_MODE InAddressW)
    {
        AddressW = InAddressW;
        return *this;
    }

    SamplerDesc& SetMipLODBias(float InMipLODBias)
    {
        MipLODBias = InMipLODBias;
        return *this;
    }

    SamplerDesc& SetMaxAnisotropy(uint8_t InMaxAnisotropy)
    {
        MaxAnisotropy = InMaxAnisotropy;
        return *this;
    }

    SamplerDesc& SetComparisonFunc(COMPARISON_FUNCTION InComparisonFunc)
    {
        ComparisonFunc = InComparisonFunc;
        return *this;
    }

    SamplerDesc& SetCompareRefToTexture(bool InbCompareRefToTexture)
    {
        bCompareRefToTexture = InbCompareRefToTexture;
        return *this;
    }

    SamplerDesc& SetBorderColor(float InR, float InG, float InB, float InA)
    {
        BorderColor[0] = InR;
        BorderColor[1] = InG;
        BorderColor[2] = InB;
        BorderColor[3] = InA;
        return *this;
    }

    SamplerDesc& SetMinLOD(float InMinLOD)
    {
        MinLOD = InMinLOD;
        return *this;
    }

    SamplerDesc& SetMaxLOD(float InMaxLOD)
    {
        MaxLOD = InMaxLOD;
        return *this;
    }

    SamplerDesc& SetCubemapSeamless(bool InbCubemapSeamless)
    {
        bCubemapSeamless = InbCubemapSeamless;
        return *this;
    }

    bool operator==(SamplerDesc const& Rhs) const
    {
        return std::memcmp(this, &Rhs, sizeof(*this)) == 0;
    }

    bool operator!=(SamplerDesc const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        // clang-format off
        static_assert(sizeof(*this) == sizeof(Filter) +
            sizeof(AddressU) +
            sizeof(AddressV) +
            sizeof(AddressW) +
            sizeof(MaxAnisotropy) + 
            sizeof(ComparisonFunc) + 
            sizeof(bCompareRefToTexture) + 
            sizeof(bCubemapSeamless) + 
            sizeof(MipLODBias) + 
            sizeof(MinLOD) + 
            sizeof(MaxLOD) + 
            sizeof(BorderColor), "Unexpected alignment");
        // clang-format on

        return HashTraits::SDBMHash(reinterpret_cast<const char*>(this), sizeof(*this));
    }
};

enum DATA_FORMAT : uint8_t
{
    FORMAT_BYTE1,
    FORMAT_BYTE2,
    FORMAT_BYTE3,
    FORMAT_BYTE4,

    FORMAT_UBYTE1,
    FORMAT_UBYTE2,
    FORMAT_UBYTE3,
    FORMAT_UBYTE4,

    FORMAT_SHORT1,
    FORMAT_SHORT2,
    FORMAT_SHORT3,
    FORMAT_SHORT4,

    FORMAT_USHORT1,
    FORMAT_USHORT2,
    FORMAT_USHORT3,
    FORMAT_USHORT4,

    FORMAT_INT1,
    FORMAT_INT2,
    FORMAT_INT3,
    FORMAT_INT4,

    FORMAT_UINT1,
    FORMAT_UINT2,
    FORMAT_UINT3,
    FORMAT_UINT4,

    FORMAT_HALF1,
    FORMAT_HALF2,
    FORMAT_HALF3,
    FORMAT_HALF4,

    FORMAT_FLOAT1,
    FORMAT_FLOAT2,
    FORMAT_FLOAT3,
    FORMAT_FLOAT4
};

using BindlessHandle = uint64_t;

class ITexture : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_TEXTURE;

    ITexture(IDevice* pDevice, TextureDesc const& Desc) :
        IDeviceObject(pDevice, PROXY_TYPE), Desc(Desc)
    {
        const auto AllowedBindings = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_DEPTH_STENCIL | BIND_UNORDERED_ACCESS;
        HK_MAYBE_UNUSED(AllowedBindings);

        HK_ASSERT_(Desc.Resolution.Width > 0, "Invalid texture resolution");
        HK_ASSERT_(Desc.Resolution.Height > 0, "Invalid texture resolution");
        HK_ASSERT_(Desc.Resolution.SliceCount > 0, "Invalid texture resolution");

        HK_ASSERT_((Desc.BindFlags & ~AllowedBindings) == 0, "The following bind flags are allowed for texture: BIND_SHADER_RESOURCE, BIND_RENDER_TARGET, BIND_DEPTH_STENCIL, BIND_UNORDERED_ACCESS");
        HK_ASSERT_(!(Desc.Multisample.NumSamples > 1 && (Desc.BindFlags & BIND_UNORDERED_ACCESS)), "Multisampled textures cannot have BIND_UNORDERED_ACCESS flag");

        HK_ASSERT_(Desc.NumMipLevels > 0, "Invalid mipmap count");
        HK_ASSERT_(Desc.Multisample.NumSamples > 0, "Invalid sample count");
        HK_ASSERT_(Desc.Multisample.NumSamples == 1 ||
                       (Desc.Multisample.NumSamples > 1 && (Desc.Type == TEXTURE_2D || Desc.Type == TEXTURE_2D_ARRAY)),
                   "Multisample allowed only for 2D and 2DArray textures\n");

        HK_ASSERT_(Desc.NumMipLevels == 1 || (Desc.NumMipLevels > 1 && Desc.Multisample.NumSamples == 1), "Mipmapping is not allowed for multisample texture");
    }

    virtual BindlessHandle GetBindlessSampler(SamplerDesc const& SamplerDesc)                = 0;
    virtual void           MakeBindlessSamplerResident(BindlessHandle Handle, bool bResident) = 0;
    virtual bool           IsBindlessSamplerResident(BindlessHandle Handle)                   = 0;

    // The texture view is alive as long as the texture exists. Do not store a strong reference to the view.
    virtual ITextureView* GetTextureView(TextureViewDesc const& TextureViewDesc) = 0;

    ITextureView* GetRenderTargetView();
    ITextureView* GetDepthStencilView();
    ITextureView* GetShaderResourceView();
    ITextureView* GetUnorderedAccessView();

    TextureDesc const& GetDesc() const { return Desc; }

    uint32_t GetWidth() const { return Desc.Resolution.Width; }

    uint32_t GetHeight() const { return Desc.Resolution.Height; }

    bool IsArray() const
    {
        return Desc.Type == TEXTURE_1D_ARRAY || Desc.Type == TEXTURE_2D_ARRAY || Desc.Type == TEXTURE_CUBE_ARRAY;
    }

    uint32_t GetSliceCount() const
    {
        return Desc.Resolution.SliceCount;
    }

    uint32_t GetSliceCount(uint16_t MipLevel) const
    {
        return (Desc.Type == TEXTURE_3D) ? Math::Max(1u, Desc.Resolution.SliceCount >> MipLevel) : Desc.Resolution.SliceCount;
    }

    bool IsCompressed() const { return bCompressed; }

    bool IsMultisample() const { return Desc.Multisample.NumSamples > 1; }

    virtual void GetMipLevelInfo(uint16_t MipLevel, TextureMipLevelInfo* pInfo) const = 0;


    // TODO: Move Invalidate to FrameGraph
    virtual void Invalidate(uint16_t MipLevel)                                          = 0;
    virtual void InvalidateRect(uint32_t NumRectangles, TextureRect const* Rectangles) = 0;


    virtual void Read(uint16_t     MipLevel,
                      size_t       SizeInBytes,
                      unsigned int Alignment,
                      void*        pSysMem) = 0;

    virtual void ReadRect(TextureRect const& Rectangle,
                          size_t              SizeInBytes,
                          unsigned int        Alignment,
                          void*               pSysMem) = 0;

    virtual bool Write(uint16_t     MipLevel,
                       size_t       SizeInBytes,
                       unsigned int Alignment,
                       const void*  pSysMem) = 0;

    virtual bool WriteRect(TextureRect const& Rectangle,
                           size_t              SizeInBytes,
                           unsigned int        Alignment,
                           const void*         pSysMem,
                           size_t              RowPitch   = 0,
                           size_t              DepthPitch = 0) = 0;

    //
    // Utilites
    //

    static int sCalcMaxMipLevels(TEXTURE_TYPE Type, TextureResolution const& Resolution);

protected:
    TextureDesc Desc;
    bool         bCompressed = false;
    ITextureView* pRenderTargetView{};
    ITextureView* pDepthStencilView{};
    ITextureView* pShaderResourceView{};
    ITextureView* pUnorderedAccesView{};
};

HK_FORCEINLINE ITextureView* ITexture::GetRenderTargetView()
{
    HK_ASSERT(GetDesc().BindFlags & BIND_RENDER_TARGET);
    return pRenderTargetView;
}

HK_FORCEINLINE ITextureView* ITexture::GetDepthStencilView()
{
    HK_ASSERT(GetDesc().BindFlags & BIND_DEPTH_STENCIL);
    return pDepthStencilView;
}

HK_FORCEINLINE ITextureView* ITexture::GetShaderResourceView()
{
    HK_ASSERT(GetDesc().BindFlags & BIND_SHADER_RESOURCE);
    return pShaderResourceView;
}

HK_FORCEINLINE ITextureView* ITexture::GetUnorderedAccessView()
{
    HK_ASSERT(GetDesc().BindFlags & BIND_UNORDERED_ACCESS);
    return pUnorderedAccesView;
}

HK_INLINE int ITexture::sCalcMaxMipLevels(TEXTURE_TYPE Type, TextureResolution const& Resolution)
{
    uint32_t maxDimension = (Type == TEXTURE_3D) ? Math::Max3(Resolution.Width, Resolution.Height, Resolution.SliceCount) :
                                                   Math::Max(Resolution.Width, Resolution.Height);
    return maxDimension > 0 ? Math::Log2(maxDimension) + 1 : 0;
}

} // namespace RenderCore

HK_NAMESPACE_END
