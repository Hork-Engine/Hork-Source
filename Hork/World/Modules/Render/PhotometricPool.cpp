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

#include "PhotometricPool.h"

#include <Hork/GameApplication/GameApplication.h>
#include <Hork/Image/PhotometricData.h>

HK_NAMESPACE_BEGIN

PhotometricPool::PhotometricPool(PhotometricPoolDesc const& desc)
{
    m_MaxCapacity = Math::Min(Math::ToGreaterPowerOfTwo(uint32_t(desc.MaxSize)), 2048u);

    uint32_t capacity = Math::Clamp(Math::ToGreaterPowerOfTwo(uint32_t(desc.InitialSize)), 128u, m_MaxCapacity);

    m_Memory.Resize(capacity * PHOTOMETRIC_DATA_SIZE);

    GameApplication::sGetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(PHOTOMETRIC_DATA_SIZE, capacity))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_Texture);

    m_Texture->SetDebugName("PhotometricPool");
}

void PhotometricPool::GrowCapacity()
{
    uint32_t capacity = m_Memory.Size() / PHOTOMETRIC_DATA_SIZE;

    if (capacity > m_PoolSize)
        return;

    while (m_PoolSize >= capacity)
        capacity += 128;

    if (capacity > m_MaxCapacity)
        capacity = m_MaxCapacity;

    m_Memory.Resize(capacity * PHOTOMETRIC_DATA_SIZE);

    m_Texture.Reset();
    GameApplication::sGetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                  .SetResolution(RenderCore::TextureResolution1DArray(PHOTOMETRIC_DATA_SIZE, capacity))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE),
                                              &m_Texture);
    m_Texture->SetDebugName("PhotometricPool");

    RenderCore::TextureRect rect;
    rect.Dimension.X = PHOTOMETRIC_DATA_SIZE;
    rect.Dimension.Y = 1;
    rect.Dimension.Z = m_PoolSize;
    m_Texture->WriteRect(rect, PHOTOMETRIC_DATA_SIZE, 4, m_Memory.ToPtr());
}

uint16_t PhotometricPool::Add(ArrayView<uint8_t> samples)
{
    uint16_t id;

    if (samples.Size() != PHOTOMETRIC_DATA_SIZE)
    {
        LOG("PhotometricPool::Add: wrong array size - should be {}\n", PHOTOMETRIC_DATA_SIZE);
        return 0xffff;
    }

    if (m_FreeList.IsEmpty())
    {
        if (m_PoolSize >= m_MaxCapacity)
        {
            LOG("PhotometricPool::Add: Exceeds the maximum pool size\n");
            return 0xffff;
        }

        ++m_PoolSize;

        GrowCapacity();

        id = m_PoolSize - 1;
    }
    else
    {
        id = m_FreeList.Last();
        m_FreeList.RemoveLast();
    }

    RenderCore::TextureRect rect;
    rect.Offset.Z = id;
    rect.Dimension.X = PHOTOMETRIC_DATA_SIZE;
    rect.Dimension.Y = 1;
    rect.Dimension.Z = 1;
    m_Texture->WriteRect(rect, PHOTOMETRIC_DATA_SIZE, 4, samples.ToPtr());

    Core::Memcpy(&m_Memory[id * PHOTOMETRIC_DATA_SIZE], samples.ToPtr(), PHOTOMETRIC_DATA_SIZE);

    return id;
}

void PhotometricPool::Remove(uint16_t id)
{
    if (id == 0xffff)
        return;

    HK_IF_NOT_ASSERT(id < m_PoolSize)
    {
        return;
    }

    auto iter = std::lower_bound(m_FreeList.begin(), m_FreeList.end(), id);
    if (iter != m_FreeList.end() && *iter == id)
    {
        HK_ASSERT_(0, "Invalid photometric ID");
        return;
    }
    m_FreeList.Insert(iter, id);
}

HK_NAMESPACE_END
