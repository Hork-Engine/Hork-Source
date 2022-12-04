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

#include "Resource.h"
#include "ResourceManager.h"
#include "Engine.h"
#include "EmbeddedResources.h"
#include <Platform/Logger.h>

HK_CLASS_META(AResource)

void AResource::InitializeDefaultObject()
{
    InitializeFromFile(GetDefaultResourcePath());
}

void AResource::InitializeFromFile(AStringView path)
{
    if (!LoadFromPath(path))
    {
        InitializeDefaultObject();
    }
}

bool AResource::LoadFromPath(AStringView path)
{
    if (!path.IcmpN("/Default/", 9))
    {
        LoadInternalResource(path);
        return true;
    }

    AFile f = GEngine->GetResourceManager()->OpenResource(path);
    if (!f)
        return false;

    return LoadResource(f);
}

HK_CLASS_META(ABinaryResource)

ABinaryResource::~ABinaryResource()
{
    Purge();
}

void ABinaryResource::Purge()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_pBinaryData);
    m_pBinaryData = nullptr;

    m_SizeInBytes = 0;
}

bool ABinaryResource::LoadResource(IBinaryStreamReadInterface& stream)
{
    Purge();

    m_SizeInBytes = stream.SizeInBytes();
    if (!m_SizeInBytes)
    {
        LOG("ABinaryResource::LoadResource: empty file\n");
        return false;
    }

    m_pBinaryData = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(m_SizeInBytes + 1);
    stream.Read(m_pBinaryData, m_SizeInBytes);
    ((uint8_t*)m_pBinaryData)[m_SizeInBytes] = 0;

    return true;
}

void ABinaryResource::LoadInternalResource(AStringView path)
{
    Purge();
}
