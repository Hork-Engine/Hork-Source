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

#include <Hork/Core/Containers/ArrayView.h>
#include <Hork/RenderCore/Texture.h>

HK_NAMESPACE_BEGIN

struct PhotometricPoolDesc
{
    uint16_t InitialSize = 128;
    uint16_t MaxSize = 2048;
};

class PhotometricPool final : public Noncopyable
{
public:
    explicit            PhotometricPool(PhotometricPoolDesc const& desc={});

    uint16_t            Add(ArrayView<uint8_t> samples);
    void                Remove(uint16_t id);

    uint32_t            Size() const { return m_PoolSize - m_FreeList.Size(); }
    uint32_t            Capacity() const { return m_MaxCapacity; }

    Ref<RenderCore::ITexture> GetTexture() { return m_Texture; }

private:
    void                GrowCapacity();

    Ref<RenderCore::ITexture>   m_Texture;
    Vector<uint8_t>             m_Memory;
    Vector<uint16_t>            m_FreeList;
    uint32_t                    m_PoolSize{};
    uint32_t                    m_MaxCapacity{};
};

HK_NAMESPACE_END
