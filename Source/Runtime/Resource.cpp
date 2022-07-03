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

void AResource::InitializeFromFile(AStringView Path)
{
    if (!LoadFromPath(Path))
    {
        InitializeDefaultObject();
    }
}

bool AResource::IsResourceExists(AStringView Path)
{
    if (!Path.IcmpN("/Default/", 9))
    {
        return false;
    }

    if (!Path.IcmpN("/Root/", 6))
    {
        Path = Path.TruncateHead(6);

        // find in file system
        AString fileSystemPath = GEngine->GetRootPath() + Path;
        if (Core::IsFileExists(fileSystemPath))
            return true;

        // find in resource pack
        AArchive* resourcePack;
        int       fileIndex;
        if (GEngine->GetResourceManager()->FindFile(Path, &resourcePack, &fileIndex))
            return true;

        return false;
    }

    if (!Path.IcmpN("/Common/", 8))
    {
        Path = Path.TruncateHead(1);

        // find in file system
        if (Core::IsFileExists(Path))
            return true;

        Path = Path.TruncateHead(7);

        // find in resource pack
        AArchive const& archive = *GEngine->GetResourceManager()->GetCommonResources();
        if (archive.LocateFile(Path) < 0)
            return false;

        return true;
    }

    if (!Path.IcmpN("/FS/", 4))
    {
        Path = Path.TruncateHead(4);

        if (Core::IsFileExists(Path))
            return true;

        return false;
    }

    if (!Path.IcmpN("/Embedded/", 10))
    {
        Path = Path.TruncateHead(10);

        AArchive const& archive = Runtime::GetEmbeddedResources();
        if (archive.LocateFile(Path) < 0)
            return false;

        return true;
    }

    // Invalid path
    LOG("Invalid path \"{}\"\n", Path);
    return false;
}

bool AResource::LoadFromPath(AStringView Path)
{
    if (!Path.IcmpN("/Default/", 9))
    {
        LoadInternalResource(Path);
        return true;
    }

    if (!Path.IcmpN("/Root/", 6))
    {
        Path = Path.TruncateHead(6);

        // try to load from file system
        AString fileSystemPath = GEngine->GetRootPath() + Path;
        if (Core::IsFileExists(fileSystemPath))
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
        if (GEngine->GetResourceManager()->FindFile(Path, &resourcePack, &fileIndex))
        {
            AMemoryStream f;
            if (!f.OpenRead(fileIndex, *resourcePack))
            {
                return false;
            }
            return LoadResource(f);
        }

        LOG("File not found /Root/{}\n", Path);
        return false;
    }

    if (!Path.IcmpN("/Common/", 8))
    {
        Path = Path.TruncateHead(1);

        // try to load from file system
        if (Core::IsFileExists(Path))
        {
            AFileStream f;
            if (!f.OpenRead(Path))
            {
                return false;
            }
            return LoadResource(f);
        }

        // try to load from resource pack
        AMemoryStream f;
        if (!f.OpenRead(Path.TruncateHead(7), *GEngine->GetResourceManager()->GetCommonResources()))
        {
            return false;
        }
        return LoadResource(f);
    }

    if (!Path.IcmpN("/FS/", 4))
    {
        Path = Path.TruncateHead(4);

        AFileStream f;
        if (!f.OpenRead(Path))
        {
            return false;
        }

        return LoadResource(f);
    }

    if (!Path.IcmpN("/Embedded/", 10))
    {
        Path = Path.TruncateHead(10);

        AMemoryStream f;
        if (!f.OpenRead(Path, Runtime::GetEmbeddedResources()))
        {
            //LOG( "Failed to open /Embedded/{}\n", Path );
            return false;
        }

        return LoadResource(f);
    }

    // Invalid path
    LOG("Invalid path \"{}\"\n", Path);
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

void ABinaryResource::LoadInternalResource(AStringView Path)
{
    Purge();
}
