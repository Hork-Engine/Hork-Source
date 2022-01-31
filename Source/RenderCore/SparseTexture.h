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

#include "Texture.h"

namespace RenderCore
{

/// Sparse texture types
enum SPARSE_TEXTURE_TYPE : uint8_t
{
    SPARSE_TEXTURE_2D,
    SPARSE_TEXTURE_2D_ARRAY,
    SPARSE_TEXTURE_3D,
    SPARSE_TEXTURE_CUBE_MAP,
    SPARSE_TEXTURE_CUBE_MAP_ARRAY,

    SPARSE_TEXTURE_RECT_GL // Can be used only with OpenGL backend
};

struct SSparseTextureDesc
{
    SPARSE_TEXTURE_TYPE Type       = SPARSE_TEXTURE_2D;
    TEXTURE_FORMAT      Format     = TEXTURE_FORMAT_RGBA8;
    STextureResolution  Resolution = {};
    STextureSwizzle     Swizzle;
    uint16_t            NumMipLevels = 1;

    SSparseTextureDesc() = default;

    bool operator==(SSparseTextureDesc const& Rhs) const
    {
        // clang-format off
        return Type        == Rhs.Type &&
               Format      == Rhs.Format &&
               Resolution  == Rhs.Resolution &&
               Swizzle     == Rhs.Swizzle &&
               NumMipLevels== Rhs.NumMipLevels;
        // clang-format on
    }

    bool operator!=(SSparseTextureDesc const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    SSparseTextureDesc& SetFormat(TEXTURE_FORMAT InFormat)
    {
        Format = InFormat;
        return *this;
    }

    SSparseTextureDesc& SetSwizzle(STextureSwizzle const& InSwizzle)
    {
        Swizzle = InSwizzle;
        return *this;
    }

    SSparseTextureDesc& SetMipLevels(int InNumMipLevels)
    {
        NumMipLevels = InNumMipLevels;
        return *this;
    }

    SSparseTextureDesc& SetResolution(STextureResolution2D const& InResolution)
    {
        Type       = SPARSE_TEXTURE_2D;
        Resolution = InResolution;
        return *this;
    }

    SSparseTextureDesc& SetResolution(STextureResolution2DArray const& InResolution)
    {
        Type       = SPARSE_TEXTURE_2D_ARRAY;
        Resolution = InResolution;
        return *this;
    }

    SSparseTextureDesc& SetResolution(STextureResolution3D const& InResolution)
    {
        Type       = SPARSE_TEXTURE_3D;
        Resolution = InResolution;
        return *this;
    }

    SSparseTextureDesc& SetResolution(STextureResolutionCubemap const& InResolution)
    {
        Type       = SPARSE_TEXTURE_CUBE_MAP;
        Resolution = InResolution;
        return *this;
    }

    SSparseTextureDesc& SetResolution(STextureResolutionCubemapArray const& InResolution)
    {
        Type       = SPARSE_TEXTURE_CUBE_MAP_ARRAY;
        Resolution = InResolution;
        return *this;
    }

    SSparseTextureDesc& SetResolution(STextureResolutionRectGL const& InResolution)
    {
        Type       = SPARSE_TEXTURE_RECT_GL;
        Resolution = InResolution;
        return *this;
    }
};

class ISparseTexture : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_SPARSE_TEXTURE;

    ISparseTexture(IDevice* pDevice, SSparseTextureDesc const& Desc) :
        IDeviceObject(pDevice, PROXY_TYPE), Desc(Desc)
    {}

    SSparseTextureDesc const& GetDesc() const { return Desc; }

    uint32_t GetWidth() const { return Desc.Resolution.Width; }

    uint32_t GetHeight() const { return Desc.Resolution.Height; }

    bool IsCompressed() const { return bCompressed; }

    int GetPageSizeX() const { return PageSizeX; }
    int GetPageSizeY() const { return PageSizeY; }
    int GetPageSizeZ() const { return PageSizeZ; }

protected:
    SSparseTextureDesc Desc;
    int                PageSizeX;
    int                PageSizeY;
    int                PageSizeZ;
    bool               bCompressed;
};

} // namespace RenderCore
