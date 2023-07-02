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

#include "PhotometricPool.h"

#include <Engine/Runtime/GameApplication.h>

HK_NAMESPACE_BEGIN

PhotometricPool::PhotometricPool(PhotometricPool::CreateInfo const& createInfo)
{
    m_MaxSize = Math::Min(Math::ToGreaterPowerOfTwo(uint32_t(createInfo.MaxSize)), 2048u);

    uint32_t size = Math::Clamp(Math::ToGreaterPowerOfTwo(uint32_t(createInfo.InitialSize)), 128u, m_MaxSize);

    m_Memory.Resize(size * 256);

    GameApplication::GetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(256, size))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_Texture);

    m_Texture->SetDebugName("PhotometricPool");

    uint8_t invalidData[256] = {};
    Add(invalidData);
}

void PhotometricPool::GrowCapacity()
{
    uint32_t size = m_Memory.Size() / 256;

    if (size > m_PoolSize)
        return;

    while (m_PoolSize >= size)
    {
        size += 128;
    }

    if (size > m_MaxSize)
    {
        size = m_MaxSize;
    }

    m_Memory.Resize(size * 256);

    m_Texture.Reset();

    GameApplication::GetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(256, size))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_Texture);
    m_Texture->SetDebugName("PhotometricPool");

    RenderCore::TextureRect rect;
    rect.Dimension.X = 256;
    rect.Dimension.Y = 1;
    rect.Dimension.Z = m_PoolSize;
    m_Texture->WriteRect(rect, 256, 4, m_Memory.ToPtr());
}

uint16_t PhotometricPool::Add(uint8_t const* data)
{
    uint16_t id;

    if (m_FreeList.IsEmpty())
    {
        if (m_PoolSize >= m_MaxSize)
        {
            LOG("PhotometricPool::Add: Exceeds the maximum pool size\n");
            return 0;
        }

        GrowCapacity();

        id = m_PoolSize++;
    }
    else
    {
        id = m_FreeList.Last();
        m_FreeList.RemoveLast();
    }

    RenderCore::TextureRect rect;
    rect.Offset.Z = id;
    rect.Dimension.X = 256;
    rect.Dimension.Y = 1;
    rect.Dimension.Z = 1;
    m_Texture->WriteRect(rect, 256, 4, data);

    Core::Memcpy(&m_Memory[id * 256], data, 256);

    return id;
}

void PhotometricPool::Remove(uint16_t id)
{
    if (!id)
        return;

    //HK_ASSERT(m_FreeList.IndexOf(id) == Core::NPOS);
    //m_FreeList.Add(id);

    auto iter = std::lower_bound(m_FreeList.begin(), m_FreeList.end(), id);
    if (iter != m_FreeList.end() && *iter == id)
    {
        HK_ASSERT(0);
        return;
    }
    m_FreeList.Insert(iter, id);
}

HK_NAMESPACE_END
