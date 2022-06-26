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

void AResource::InitializeFromFile(AStringView _Path)
{
    if (!LoadFromPath(_Path))
    {
        InitializeDefaultObject();
    }
}

bool AResource::LoadFromPath(AStringView _Path)
{
    if (!_Path.IcmpN("/Default/", 9))
    {
        LoadInternalResource(_Path);
        return true;
    }

    if (!_Path.IcmpN("/Root/", 6))
    {
        _Path = _Path.TruncateHead(6);

        // try to load from file system
        AString fileSystemPath = GEngine->GetRootPath() + _Path;
        if (Core::IsFileExists(fileSystemPath.CStr()))
        {
            AFileStream f;
            if (!f.OpenRead(fileSystemPath))
            {
                return false;
            }
            return LoadResource(f);
        }

        // try to load from resource pack
        AArchive* resourcePack;
        int       fileIndex;
        if (GEngine->GetResourceManager()->FindFile(_Path, &resourcePack, &fileIndex))
        {
            AMemoryStream f;
            if (!f.OpenRead(fileIndex, *resourcePack))
            {
                return false;
            }
            return LoadResource(f);
        }

        LOG("File not found /Root/{}\n", _Path);
        return false;
    }

    if (!_Path.IcmpN("/Common/", 8))
    {
        _Path = _Path.TruncateHead(1);

        // try to load from file system
        if (Core::IsFileExists(_Path))
        {
            AFileStream f;
            if (!f.OpenRead(_Path))
            {
                return false;
            }
            return LoadResource(f);
        }

        // try to load from resource pack
        AMemoryStream f;
        if (!f.OpenRead(_Path.TruncateHead(7), *GEngine->GetResourceManager()->GetCommonResources()))
        {
            return false;
        }
        return LoadResource(f);
    }

    if (!_Path.IcmpN("/FS/", 4))
    {
        _Path = _Path.TruncateHead(4);

        AFileStream f;
        if (!f.OpenRead(_Path))
        {
            return false;
        }

        return LoadResource(f);
    }

    if (!_Path.IcmpN("/Embedded/", 10))
    {
        _Path = _Path.TruncateHead(10);

        AMemoryStream f;
        if (!f.OpenRead(_Path, Runtime::GetEmbeddedResources()))
        {
            //LOG( "Failed to open /Embedded/{}\n", _Path );
            return false;
        }

        return LoadResource(f);
    }

    // Invalid path
    LOG("Invalid path \"{}\"\n", _Path);
    return false;
}

HK_CLASS_META(ABinaryResource)

ABinaryResource::ABinaryResource()
{
}

ABinaryResource::~ABinaryResource()
{
    Purge();
}

void ABinaryResource::Purge()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(pBinaryData);
    pBinaryData = nullptr;

    SizeInBytes = 0;
}

bool ABinaryResource::LoadResource(IBinaryStreamReadInterface& _Stream)
{
    Purge();

    SizeInBytes = _Stream.SizeInBytes();
    if (!SizeInBytes)
    {
        LOG("ABinaryResource::LoadResource: empty file\n");
        return false;
    }

    pBinaryData = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(SizeInBytes + 1);
    _Stream.Read(pBinaryData, SizeInBytes);
    ((uint8_t*)pBinaryData)[SizeInBytes] = 0;

    return true;
}

void ABinaryResource::LoadInternalResource(AStringView _Path)
{
    Purge();
}
