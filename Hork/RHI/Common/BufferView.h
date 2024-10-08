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

#include "DeviceObject.h"
#include "Texture.h"

HK_NAMESPACE_BEGIN

namespace RHI
{

enum BUFFER_VIEW_PIXEL_FORMAT : uint8_t
{
    BUFFER_VIEW_PIXEL_FORMAT_R8       = TEXTURE_FORMAT_R8_UNORM,
    BUFFER_VIEW_PIXEL_FORMAT_R16      = TEXTURE_FORMAT_R16_UNORM,
    BUFFER_VIEW_PIXEL_FORMAT_R16F     = TEXTURE_FORMAT_R16_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_R32F     = TEXTURE_FORMAT_R32_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_R8I      = TEXTURE_FORMAT_R8_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_R16I     = TEXTURE_FORMAT_R16_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_R32I     = TEXTURE_FORMAT_R32_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_R8UI     = TEXTURE_FORMAT_R8_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_R16UI    = TEXTURE_FORMAT_R16_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_R32UI    = TEXTURE_FORMAT_R32_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RG8      = TEXTURE_FORMAT_RG8_UNORM,
    BUFFER_VIEW_PIXEL_FORMAT_RG16     = TEXTURE_FORMAT_RG16_UNORM,
    BUFFER_VIEW_PIXEL_FORMAT_RG16F    = TEXTURE_FORMAT_RG16_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_RG32F    = TEXTURE_FORMAT_RG32_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_RG8I     = TEXTURE_FORMAT_RG8_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RG16I    = TEXTURE_FORMAT_RG16_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RG32I    = TEXTURE_FORMAT_RG32_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RG8UI    = TEXTURE_FORMAT_RG8_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RG16UI   = TEXTURE_FORMAT_RG16_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RG32UI   = TEXTURE_FORMAT_RG32_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGB32F   = TEXTURE_FORMAT_RGB32_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_RGB32I   = TEXTURE_FORMAT_RGB32_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGB32UI  = TEXTURE_FORMAT_RGB32_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA8    = TEXTURE_FORMAT_RGBA8_UNORM,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA16   = TEXTURE_FORMAT_RGBA16_UNORM,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA16F  = TEXTURE_FORMAT_RGBA16_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA32F  = TEXTURE_FORMAT_RGBA32_FLOAT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA8I   = TEXTURE_FORMAT_RGBA8_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA16I  = TEXTURE_FORMAT_RGBA16_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA32I  = TEXTURE_FORMAT_RGBA32_SINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA8UI  = TEXTURE_FORMAT_RGBA8_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA16UI = TEXTURE_FORMAT_RGBA16_UINT,
    BUFFER_VIEW_PIXEL_FORMAT_RGBA32UI = TEXTURE_FORMAT_RGBA32_UINT
};

struct BufferViewDesc
{
    BUFFER_VIEW_PIXEL_FORMAT Format      = BUFFER_VIEW_PIXEL_FORMAT_R8;
    size_t                   Offset      = 0;
    size_t                   SizeInBytes = 0;
};

class IBufferView : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_BUFFER_VIEW;

    IBufferView(IDevice* pDevice, BufferViewDesc const& Desc) :
        IDeviceObject(pDevice, PROXY_TYPE), Desc(Desc)
    {}

    BufferViewDesc const& GetDesc() const { return Desc; }

    virtual void SetRange(size_t Offset, size_t SizeInBytes) = 0;

    virtual size_t GetBufferOffset(uint16_t MipLevel) const  = 0;
    virtual size_t GetBufferSizeInBytes(uint16_t MipLevel) const = 0;

protected:
    BufferViewDesc Desc;
};

} // namespace RHI

HK_NAMESPACE_END
