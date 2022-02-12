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

void AResource::InitializeFromFile(const char* _Path)
{
    if (!LoadFromPath(_Path))
    {
        InitializeDefaultObject();
    }
}

bool AResource::LoadFromPath(const char* _Path)
{
    if (!Platform::StricmpN(_Path, "/Default/", 9))
    {
        LoadInternalResource(_Path);
        return true;
    }

    if (!Platform::StricmpN(_Path, "/Root/", 6))
    {
        _Path += 6;

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

        GLogger.Printf("File not found /Root/%s\n", _Path);
        return false;
    }

    if (!Platform::StricmpN(_Path, "/Common/", 8))
    {
        _Path += 1;

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
        if (!f.OpenRead(_Path + 7, *GEngine->GetResourceManager()->GetCommonResources()))
        {
            return false;
        }
        return LoadResource(f);
    }

    if (!Platform::StricmpN(_Path, "/FS/", 4))
    {
        _Path += 4;

        AFileStream f;
        if (!f.OpenRead(_Path))
        {
            return false;
        }

        return LoadResource(f);
    }

    if (!Platform::StricmpN(_Path, "/Embedded/", 10))
    {
        _Path += 10;

        AMemoryStream f;
        if (!f.OpenRead(_Path, Runtime::GetEmbeddedResources()))
        {
            //GLogger.Printf( "Failed to open /Embedded/%s\n", _Path );
            return false;
        }

        return LoadResource(f);
    }

    // Invalid path
    GLogger.Printf("Invalid path \"%s\"\n", _Path);
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
    GHeapMemory.Free(pBinaryData);
    pBinaryData = nullptr;

    SizeInBytes = 0;
}

bool ABinaryResource::LoadResource(IBinaryStream& _Stream)
{
    Purge();

    SizeInBytes = _Stream.SizeInBytes();
    if (!SizeInBytes)
    {
        GLogger.Printf("ABinaryResource::LoadResource: empty file\n");
        return false;
    }

    pBinaryData = GHeapMemory.Alloc(SizeInBytes + 1);
    _Stream.Read(pBinaryData, SizeInBytes);
    ((uint8_t*)pBinaryData)[SizeInBytes] = 0;

    return true;
}

void ABinaryResource::LoadInternalResource(const char* _Path)
{
    Purge();
}
