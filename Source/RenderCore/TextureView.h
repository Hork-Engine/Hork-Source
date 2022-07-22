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

#include "DeviceObject.h"
#include <Core/HashFunc.h>
#include <Image/Image.h>

namespace RenderCore
{

class ITexture;

/// Texture types
enum TEXTURE_TYPE : uint8_t
{
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
    TEXTURE_CUBE_MAP_ARRAY
};

enum TEXTURE_VIEW : uint8_t
{
    TEXTURE_VIEW_UNDEFINED = 0,
    TEXTURE_VIEW_SHADER_RESOURCE,
    TEXTURE_VIEW_RENDER_TARGET,
    TEXTURE_VIEW_DEPTH_STENCIL,
    TEXTURE_VIEW_UNORDERED_ACCESS
};

struct STextureViewDesc
{
    TEXTURE_VIEW   ViewType = TEXTURE_VIEW_UNDEFINED;
    TEXTURE_TYPE   Type     = TEXTURE_2D;
    TEXTURE_FORMAT Format   = TEXTURE_FORMAT_RGBA8_UNORM;

    uint16_t FirstMipLevel = 0;
    uint16_t NumMipLevels  = 0;

    /** Slice is an array layer or depth for a 3D texture. Cubemap has 6 slices, cubemap array has num_layers * 6 slices. */
    uint16_t FirstSlice = 0;
    uint16_t NumSlices  = 0;

    bool operator==(STextureViewDesc const& Rhs) const
    {
        // clang-format off
        return ViewType      == Rhs.ViewType &&
               Type          == Rhs.Type &&
               Format        == Rhs.Format &&
               FirstMipLevel == Rhs.FirstMipLevel &&
               NumMipLevels  == Rhs.NumMipLevels &&
               FirstSlice    == Rhs.FirstSlice &&
               NumSlices     == Rhs.NumSlices;
        // clang-format on
    }

    bool operator!=(STextureViewDesc const& Rhs) const
    {
        return !(operator==(Rhs));
    }
};

class ITextureView : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_TEXTURE_VIEW;

    ITextureView(STextureViewDesc const& TextureViewDesc, ITexture* pTexture);

    STextureViewDesc const& GetDesc() const { return Desc; }

    ITexture* GetTexture() { return pTexture; }

    uint32_t GetWidth() const;

    uint32_t GetHeight() const;

private:
    STextureViewDesc Desc;
    ITexture*        pTexture;
};

} // namespace RenderCore

namespace HashTraits
{

HK_FORCEINLINE std::size_t Hash(RenderCore::STextureViewDesc const& Desc)
{
    return Core::SDBMHash(reinterpret_cast<const char*>(&Desc), sizeof(Desc));
}

} // namespace HashTraits
