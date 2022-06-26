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

#include "ResourceManager.h"
#include "GameModule.h"
#include <Platform/Logger.h>

AResourceManager::AResourceManager()
{
    Core::TraverseDirectory(GEngine->GetRootPath(), false,
                            [this](AStringView FileName, bool bIsDirectory)
                            {
                                if (bIsDirectory)
                                {
                                    return;
                                }

                                if (PathUtils::CompareExt(FileName, ".resources"))
                                {
                                    AddResourcePack(FileName);
                                }
                            });

    CommonResources = MakeUnique<AArchive>();
    CommonResources->Open("common.resources", true);
}

AResourceManager::~AResourceManager()
{
    for (auto it : ResourceCache)
    {
        AResource* resource = it.second;
        resource->RemoveRef();
    }
}

void AResourceManager::AddResourcePack(AStringView FileName)
{
    ResourcePacks.EmplaceBack<TUniqueRef<AArchive>>(MakeUnique<AArchive>(FileName, true));
}

bool AResourceManager::FindFile(AStringView FileName, AArchive** ppResourcePack, int* pFileIndex)
{
    *ppResourcePack = nullptr;
    *pFileIndex     = -1;

    for (int i = ResourcePacks.Size() - 1; i >= 0; i--)
    {
        TUniqueRef<AArchive>& pack = ResourcePacks[i];

        int index = pack->LocateFile(FileName);
        if (index != -1)
        {
            *ppResourcePack = pack.GetObject();
            *pFileIndex     = index;
            return true;
        }
    }
    return false;
}

AResource* AResourceManager::FindResource(AClassMeta const& _ClassMeta, AStringView _Alias, bool& _bMetadataMismatch)
{
    _bMetadataMismatch = false;

    auto it = ResourceCache.Find(_Alias);
    if (it == ResourceCache.End())
        return nullptr;

    AResource* cachedResource = it->second;

    if (&cachedResource->FinalClassMeta() != &_ClassMeta)
    {
        LOG("FindResource: {} class doesn't match meta data ({} vs {})\n", _Alias, cachedResource->FinalClassName(), _ClassMeta.GetName());
        _bMetadataMismatch = true;
        return nullptr;
    }

    return cachedResource;
}

AResource* AResourceManager::FindResourceByAlias(AStringView _Alias)
{
    auto it = ResourceCache.Find(_Alias);
    if (it == ResourceCache.End())
        return nullptr;

    return it->second;
}

AResource* AResourceManager::GetResource(AClassMeta const& _ClassMeta, AStringView _Alias, bool* _bResourceFoundResult, bool* _bMetadataMismatch)
{
    if (_bResourceFoundResult)
    {
        *_bResourceFoundResult = false;
    }

    if (_bMetadataMismatch)
    {
        *_bMetadataMismatch = false;
    }

    auto it = ResourceCache.Find(_Alias);
    if (it != ResourceCache.End())
    {
        AResource* cachedResource = it->second;
        if (&cachedResource->FinalClassMeta() != &_ClassMeta)
        {
            LOG("GetResource: {} class doesn't match meta data ({} vs {})\n", _Alias, cachedResource->FinalClassName(), _ClassMeta.GetName());

            if (_bMetadataMismatch)
            {
                *_bMetadataMismatch = true;
            }
        }

        if (_bResourceFoundResult)
        {
            *_bResourceFoundResult = true;
        }
        return cachedResource;
    }

    // Never return nullptr, always create default object

    AResource* resource = static_cast<AResource*>(_ClassMeta.CreateInstance());

    resource->InitializeDefaultObject();

    return resource;
}

AClassMeta const* AResourceManager::GetResourceInfo(AStringView _Alias)
{
    auto it = ResourceCache.Find(_Alias);
    if (it == ResourceCache.End())
        return nullptr;

    return &it->second->FinalClassMeta();
}

AResource* AResourceManager::GetOrCreateResource(AClassMeta const& _ClassMeta, AStringView _Path)
{
    bool bMetadataMismatch;

    AResource* resource = FindResource(_ClassMeta, _Path, bMetadataMismatch);
    if (bMetadataMismatch)
    {
        // Never return null

        resource = static_cast<AResource*>(_ClassMeta.CreateInstance());
        resource->InitializeDefaultObject();

        return resource;
    }

    if (resource)
    {
        //DEBUG( "CACHING {}: Name {}, Path: \"{}\"\n", resource->GetObjectName(), resource->FinalClassName(), resource->GetResourcePath().CStr() );

        return resource;
    }

    resource = static_cast<AResource*>(_ClassMeta.CreateInstance());
    resource->AddRef();
    resource->SetResourcePath(_Path);
    resource->SetObjectName(_Path);
    resource->InitializeFromFile(_Path);

    ResourceCache[_Path] = resource;

    return resource;
}

bool AResourceManager::RegisterResource(AResource* _Resource, AStringView _Alias)
{
    bool bMetadataMismatch;

    if (!_Resource->GetResourcePath().IsEmpty())
    {
        LOG("RegisterResource: Resource already registered ({})\n", _Resource->GetResourcePath());
        return false;
    }

    AResource* resource = FindResource(_Resource->FinalClassMeta(), _Alias, bMetadataMismatch);
    if (resource || bMetadataMismatch)
    {
        LOG("RegisterResource: Resource with same alias already exists ({})\n", _Alias);
        return false;
    }

    _Resource->AddRef();
    _Resource->SetResourcePath(_Alias);

    ResourceCache[_Alias] = _Resource;

    return true;
}

bool AResourceManager::UnregisterResource(AResource* _Resource)
{
    auto it = ResourceCache.Find(_Resource->GetResourcePath());
    if (it == ResourceCache.End())
    {
        LOG("UnregisterResource: resource {} is not found\n", _Resource->GetResourcePath());
        return false;
    }

    AResource* cachedResource = it->second;
    if (&cachedResource->FinalClassMeta() != &_Resource->FinalClassMeta())
    {
        LOG("UnregisterResource: {} class doesn't match meta data ({} vs {})\n", _Resource->GetResourcePath(), cachedResource->FinalClassName(), _Resource->FinalClassMeta().GetName());
        return false;
    }

    // FIXME: Match resource pointers/ids?

    _Resource->SetResourcePath("");
    _Resource->RemoveRef();

    ResourceCache.Erase(it);

    return true;
}

void AResourceManager::UnregisterResources(AClassMeta const& _ClassMeta)
{
    for (auto it = ResourceCache.Begin(); it != ResourceCache.End() ; )
    {
        AResource* resource = it->second;
        if (resource->FinalClassId() == _ClassMeta.GetId())
        {
            it = ResourceCache.Erase(it);
            resource->RemoveRef();
        }
        else
        {
            ++it;
        }
    }
}

void AResourceManager::UnregisterResources()
{
    for (auto it : ResourceCache)
    {
        AResource* resource = it.second;
        resource->RemoveRef();
    }
    ResourceCache.Clear();
}
