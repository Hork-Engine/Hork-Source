/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <atomic>

#include "ResourceRTTR.h"

HK_NAMESPACE_BEGIN

class IBinaryStreamReadInterface;

class ResourceCacheBase;

class Resource : public Noncopyable
{
    friend class        ResourceCacheBase;

    friend void         IntrusiveRef_AddRef(Resource* p);
    friend void         IntrusiveRef_RemoveRef(Resource *p);

public:
    virtual             ~Resource() = default;

    const char*         GetName() const { return m_Name; }    

    bool                IsPurged() const { return m_IsPurged; }

    virtual void        Purge() = 0;

    virtual void        Load(IBinaryStreamReadInterface& stream) = 0;

    int32_t             UseCount() const { return m_RefCount.load(std::memory_order_acquire); }

private:
    std::atomic<int32_t>m_RefCount = 0;

protected:
    bool                m_IsPurged = true;

private:
    bool                m_InGarbageList = false;
    Resource*           m_NextGarbage = nullptr;
    ResourceCacheBase*  m_Cache = nullptr;
    const char*         m_Name = "";
};

HK_FORCEINLINE uint32_t MakeResourceMagic(uint8_t type, uint8_t version)
{
    return (uint32_t('H')) | (uint32_t('k') << 8) | (uint32_t(type) << 16) | (uint32_t(version) << 24);
}

HK_NAMESPACE_END
