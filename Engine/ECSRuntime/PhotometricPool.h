/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/RenderCore/Texture.h>

HK_NAMESPACE_BEGIN

class PhotometricPool
{
public:
    struct CreateInfo
    {
        uint16_t InitialSize;
        uint16_t MaxSize;
    };
    PhotometricPool(CreateInfo const& createInfo);

    uint16_t Add(uint8_t const* data);

    void Remove(uint16_t id);

private:
    void GrowCapacity();

    TRef<RenderCore::ITexture> m_Texture;
    TVector<uint8_t> m_Memory;
    TVector<uint16_t> m_FreeList;
    uint32_t m_PoolSize{1};
    uint32_t m_MaxSize{};
};

HK_NAMESPACE_END
