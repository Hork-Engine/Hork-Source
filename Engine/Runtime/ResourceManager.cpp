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

#include "ResourceManager.h"
#include "GameModule.h"
#include "EmbeddedResources.h"
#include <Engine/Core/Platform/Logger.h>

HK_NAMESPACE_BEGIN

ResourceManager::ResourceManager()
{
    Core::TraverseDirectory(GEngine->GetRootPath(), false,
                            [this](StringView fileName, bool bIsDirectory)
                            {
                                if (bIsDirectory)
                                {
                                    return;
                                }

                                if (PathUtils::CompareExt(fileName, ".resources"))
                                {
                                    AddResourcePack(fileName);
                                }
                            });

    m_CommonResources = Archive::Open("common.resources", true);
}

ResourceManager::~ResourceManager()
{
    for (auto it : m_ResourceCache)
    {
        Resource* resource = it.second;
        resource->RemoveRef();
    }
}

void ResourceManager::AddResourceFactory(ResourceFactory* Factory)
{
    m_ResourceFactories.Add(TRef<ResourceFactory>(Factory));
}

void ResourceManager::AddResourcePack(StringView fileName)
{
    m_ResourcePacks.EmplaceBack<Archive>(Archive::Open(fileName, true));
}

bool ResourceManager::FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const
{
    *pResourcePackIndex = -1;
    pFileHandle->Reset();

    for (int i = m_ResourcePacks.Size() - 1; i >= 0; i--)
    {
        Archive const& pack = m_ResourcePacks[i];

        FileHandle handle = pack.LocateFile(fileName);
        if (handle.IsValid())
        {
            *pResourcePackIndex = i;
            *pFileHandle        = handle;
            return true;
        }
    }
    return false;
}

Resource* ResourceManager::FindResource(ClassMeta const& classMeta, StringView path, bool& bMetadataMismatch)
{
    bMetadataMismatch = false;

    Resource* cachedResource = FindResource(path);
    if (!cachedResource)
        return nullptr;

    if (&cachedResource->FinalClassMeta() != &classMeta)
    {
        LOG("FindResource: {} class doesn't match meta data ({} vs {})\n", path, cachedResource->FinalClassName(), classMeta.GetName());
        bMetadataMismatch = true;
        return nullptr;
    }

    return cachedResource;
}

Resource* ResourceManager::FindResource(StringView path)
{
    auto it = m_ResourceCache.Find(path);
    if (it == m_ResourceCache.End())
        return nullptr;

    return it->second;
}

Resource* ResourceManager::GetResource(ClassMeta const& classMeta, StringView path, bool* bResourceFoundResult, bool* bMetadataMismatch)
{
    if (bResourceFoundResult)
    {
        *bResourceFoundResult = false;
    }

    if (bMetadataMismatch)
    {
        *bMetadataMismatch = false;
    }

    Resource* resource = FindResource(path);
    if (resource)
    {
        if (&resource->FinalClassMeta() != &classMeta)
        {
            LOG("GetResource: {} class doesn't match meta data ({} vs {})\n", path, resource->FinalClassName(), classMeta.GetName());

            if (bMetadataMismatch)
            {
                *bMetadataMismatch = true;
            }
        }

        if (bResourceFoundResult)
        {
            *bResourceFoundResult = true;
        }
        return resource;
    }

    // Never return nullptr, always create default object

    resource = static_cast<Resource*>(classMeta.CreateInstance());
    resource->InitializeDefaultObject();

    return resource;
}

ClassMeta const* ResourceManager::GetResourceInfo(StringView path)
{
    Resource* resource = FindResource(path);
    return resource ? &resource->FinalClassMeta() : nullptr;
}

Resource* ResourceManager::GetOrCreateResource(ClassMeta const& classMeta, StringView path, RESOURCE_FLAGS flags)
{
    bool bMetadataMismatch;

    Resource* resource = FindResource(classMeta, path, bMetadataMismatch);
    if (bMetadataMismatch)
    {
        // Never return null

        resource = static_cast<Resource*>(classMeta.CreateInstance());
        resource->InitializeDefaultObject();

        return resource;
    }

    if (resource)
    {
        //DEBUG( "CACHING {}: Name {}, Path: \"{}\"\n", resource->GetObjectName(), resource->FinalClassName(), resource->GetResourcePath() );

        return resource;
    }

    resource = static_cast<Resource*>(classMeta.CreateInstance());
    resource->AddRef();
    resource->SetResourcePath(path);
    resource->SetResourceFlags(flags);
    resource->InitializeFromFile(path);

    m_ResourceCache[path] = resource;

    return resource;
}

bool ResourceManager::RegisterResource(Resource* resource, StringView path)
{
    bool bMetadataMismatch;

    if (resource->IsManualResource() || !resource->GetResourcePath().IsEmpty())
    {
        LOG("RegisterResource: Resource already registered ({})\n", resource->GetResourcePath());
        return false;
    }

    Resource* cachedResource = FindResource(resource->FinalClassMeta(), path, bMetadataMismatch);
    if (cachedResource || bMetadataMismatch)
    {
        LOG("RegisterResource: Resource with same path already exists ({})\n", path);
        return false;
    }

    resource->AddRef();
    resource->SetResourcePath(path);
    resource->SetManualResource(true);

    m_ResourceCache[path] = resource;

    return true;
}

bool ResourceManager::UnregisterResource(Resource* resource)
{
    if (!resource->IsManualResource())
    {
        LOG("UnregisterResource: Resource {} is not manual\n", resource->GetResourcePath());
        return false;
    }

    auto it = m_ResourceCache.Find(resource->GetResourcePath());
    if (it == m_ResourceCache.End())
    {
        LOG("UnregisterResource: Resource {} is not found\n", resource->GetResourcePath());
        return false;
    }

    Resource* cachedResource = it->second;
    if (&cachedResource->FinalClassMeta() != &resource->FinalClassMeta())
    {
        LOG("UnregisterResource: {} class doesn't match meta data ({} vs {})\n", resource->GetResourcePath(), cachedResource->FinalClassName(), resource->FinalClassMeta().GetName());
        return false;
    }

    // FIXME: Match resource pointers/ids?

    resource->SetResourcePath("");
    resource->SetManualResource(false);
    resource->RemoveRef();

    m_ResourceCache.Erase(it);

    return true;
}

void ResourceManager::UnregisterResources(ClassMeta const& classMeta)
{
    for (auto it = m_ResourceCache.Begin(); it != m_ResourceCache.End();)
    {
        Resource* resource = it->second;
        if (resource->IsManualResource() && resource->FinalClassId() == classMeta.GetId())
        {
            resource->SetManualResource(false);
            resource->SetResourcePath("");

            it = m_ResourceCache.Erase(it);
            resource->RemoveRef();
        }
        else
        {
            ++it;
        }
    }
}

void ResourceManager::UnregisterResources()
{
    for (auto it = m_ResourceCache.Begin(); it != m_ResourceCache.End();)
    {
        Resource* resource = it->second;
        if (resource->IsManualResource())
        {
            resource->SetManualResource(false);
            resource->SetResourcePath("");

            it = m_ResourceCache.Erase(it);
            resource->RemoveRef();
        }
        else
        {
            ++it;
        }
    }
}

void ResourceManager::RemoveUnreferencedResources()
{
    for (auto it = m_ResourceCache.Begin(); it != m_ResourceCache.End();)
    {
        Resource* resource = it->second;
        if (resource->GetRefCount() == 1 && !resource->IsManualResource() && !resource->IsPersistent())
        {
            it = m_ResourceCache.Erase(it);
            resource->RemoveRef();
        }
        else
        {
            ++it;
        }
    }
}

bool ResourceManager::IsResourceExists(StringView path)
{
    if (!path.IcmpN("/Default/", 9))
    {
        return false;
    }

    if (!path.IcmpN("/Root/", 6))
    {
        path = path.TruncateHead(6);

        for (auto& factory : m_ResourceFactories)
        {
            if (factory->IsResourceExists(path))
                return true;
        }

        // find in file system
        String fileSystemPath = GEngine->GetRootPath() + path;
        if (Core::IsFileExists(fileSystemPath))
            return true;

        // find in resource pack
        int resourcePack;
        FileHandle fileHandle;
        if (FindFile(path, &resourcePack, &fileHandle))
            return true;

        return false;
    }

    if (!path.IcmpN("/Common/", 8))
    {
        path = path.TruncateHead(1);

        // find in file system
        if (Core::IsFileExists(path))
            return true;

        path = path.TruncateHead(7);

        // find in resource pack
        if (!m_CommonResources.LocateFile(path).IsValid())
            return false;

        return true;
    }

    if (!path.IcmpN("/FS/", 4))
    {
        path = path.TruncateHead(4);

        if (Core::IsFileExists(path))
            return true;

        return false;
    }

    if (!path.IcmpN("/Embedded/", 10))
    {
        path = path.TruncateHead(10);

        Archive const& archive = Runtime::GetEmbeddedResources();
        if (!archive.LocateFile(path).IsValid())
            return false;

        return true;
    }

    LOG("Invalid path \"{}\"\n", path);
    return false;
}

File ResourceManager::OpenResource(StringView path)
{
    if (!path.IcmpN("/Root/", 6))
    {
        path = path.TruncateHead(6);

        for (auto& factory : m_ResourceFactories)
        {
            File file = factory->OpenResource(path);
            if (file)
                return file;
        }

        // try to load from file system
        String fileSystemPath = GEngine->GetRootPath() + path;
        if (Core::IsFileExists(fileSystemPath))
        {
            return File::OpenRead(fileSystemPath);
        }

        // try to load from resource pack
        int resourcePack;
        FileHandle fileHandle;
        if (FindFile(path, &resourcePack, &fileHandle))
        {
            return File::OpenRead(fileHandle, m_ResourcePacks[resourcePack]);
        }

        LOG("File not found /Root/{}\n", path);
        return {};
    }

    if (!path.IcmpN("/Common/", 8))
    {
        path = path.TruncateHead(1);

        // try to load from file system
        if (Core::IsFileExists(path))
        {
            return File::OpenRead(path);
        }

        // try to load from resource pack
        return File::OpenRead(path.TruncateHead(7), m_CommonResources);
    }

    if (!path.IcmpN("/FS/", 4))
    {
        path = path.TruncateHead(4);

        return File::OpenRead(path);
    }

    if (!path.IcmpN("/Embedded/", 10))
    {
        path = path.TruncateHead(10);

        return File::OpenRead(path, Runtime::GetEmbeddedResources());
    }

    LOG("Invalid path \"{}\"\n", path);
    return {};
}

HK_NAMESPACE_END
