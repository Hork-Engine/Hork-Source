/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
#include "GameModuleInterface.h"
#include "Engine.h"
#include <Platform/Logger.h>

AResourceManager* GResourceManager = nullptr;

AResourceManager::AResourceManager()
{
    Core::TraverseDirectory(GEngine->GetRootPath(), false,
                            [this](AStringView FileName, bool bIsDirectory)
                            {
                                if (bIsDirectory)
                                {
                                    return;
                                }

                                if (FileName.CompareExt(".resources"))
                                {
                                    AddResourcePack(FileName);
                                }
                            });

    CommonResources = MakeUnique<AArchive>();
    CommonResources->Open("common.resources", true);
}

AResourceManager::~AResourceManager()
{
    for (int i = ResourceCache.Size() - 1; i >= 0; i--)
    {
        ResourceCache[i]->RemoveRef();
    }
    ResourceCache.Free();
    ResourceHash.Free();

    ResourcePacks.Clear();

    CommonResources.Reset();
}

void AResourceManager::AddResourcePack(AStringView FileName)
{
    ResourcePacks.emplace_back<TUniqueRef<AArchive>>(MakeUnique<AArchive>(FileName, true));
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

AResource* AResourceManager::FindResource(AClassMeta const& _ClassMeta, const char* _Alias, bool& _bMetadataMismatch, int& _Hash)
{
    _Hash = Core::HashCase(_Alias, Platform::Strlen(_Alias));

    _bMetadataMismatch = false;

    for (int i = ResourceHash.First(_Hash); i != -1; i = ResourceHash.Next(i))
    {
        if (!ResourceCache[i]->GetResourcePath().Icmp(_Alias))
        {
            if (&ResourceCache[i]->FinalClassMeta() != &_ClassMeta)
            {
                GLogger.Printf("FindResource: %s class doesn't match meta data (%s vs %s)\n", _Alias, ResourceCache[i]->FinalClassName(), _ClassMeta.GetName());
                _bMetadataMismatch = true;
                return nullptr;
            }
            return ResourceCache[i];
        }
    }
    return nullptr;
}

AResource* AResourceManager::FindResourceByAlias(const char* _Alias)
{
    int hash = Core::HashCase(_Alias, Platform::Strlen(_Alias));

    for (int i = ResourceHash.First(hash); i != -1; i = ResourceHash.Next(i))
    {
        if (!ResourceCache[i]->GetResourcePath().Icmp(_Alias))
        {
            return ResourceCache[i];
        }
    }

    return nullptr;
}

AResource* AResourceManager::GetResource(AClassMeta const& _ClassMeta, const char* _Alias, bool* _bResourceFoundResult, bool* _bMetadataMismatch)
{
    int hash = Core::HashCase(_Alias, Platform::Strlen(_Alias));

    if (_bResourceFoundResult)
    {
        *_bResourceFoundResult = false;
    }

    if (_bMetadataMismatch)
    {
        *_bMetadataMismatch = false;
    }

    for (int i = ResourceHash.First(hash); i != -1; i = ResourceHash.Next(i))
    {
        if (!ResourceCache[i]->GetResourcePath().Icmp(_Alias))
        {
            if (&ResourceCache[i]->FinalClassMeta() != &_ClassMeta)
            {
                GLogger.Printf("GetResource: %s class doesn't match meta data (%s vs %s)\n", _Alias, ResourceCache[i]->FinalClassName(), _ClassMeta.GetName());

                if (_bMetadataMismatch)
                {
                    *_bMetadataMismatch = true;
                }
                break;
            }

            if (_bResourceFoundResult)
            {
                *_bResourceFoundResult = true;
            }
            return ResourceCache[i];
        }
    }

    // Never return nullptr, always create default object

    AResource* resource = static_cast<AResource*>(_ClassMeta.CreateInstance());

    resource->InitializeDefaultObject();

    return resource;
}

AClassMeta const* AResourceManager::GetResourceInfo(const char* _Alias)
{
    int hash = Core::HashCase(_Alias, Platform::Strlen(_Alias));

    for (int i = ResourceHash.First(hash); i != -1; i = ResourceHash.Next(i))
    {
        if (!ResourceCache[i]->GetResourcePath().Icmp(_Alias))
        {
            return &ResourceCache[i]->FinalClassMeta();
        }
    }

    return nullptr;
}

AResource* AResourceManager::GetOrCreateResource(AClassMeta const& _ClassMeta, const char* _Path)
{
    int  hash;
    bool bMetadataMismatch;

    AResource* resource = FindResource(_ClassMeta, _Path, bMetadataMismatch, hash);
    if (bMetadataMismatch)
    {

        // Never return null

        resource = static_cast<AResource*>(_ClassMeta.CreateInstance());
        resource->InitializeDefaultObject();

        return resource;
    }

    if (resource)
    {
        //GLogger.Printf( "CACHING %s: Name %s, Path: \"%s\"\n", resource->GetObjectNameCStr(), resource->FinalClassName(), resource->GetResourcePath().CStr() );

        return resource;
    }

    resource = static_cast<AResource*>(_ClassMeta.CreateInstance());
    resource->AddRef();
    resource->SetResourcePath(_Path);
    resource->SetObjectName(_Path);
    resource->InitializeFromFile(_Path);

    ResourceHash.Insert(hash, ResourceCache.Size());
    ResourceCache.Append(resource);

    return resource;
}

bool AResourceManager::RegisterResource(AResource* _Resource, const char* _Alias)
{
    int  hash;
    bool bMetadataMismatch;

    if (!_Resource->GetResourcePath().IsEmpty())
    {
        GLogger.Printf("RegisterResource: Resource already registered (%s)\n", _Resource->GetResourcePath().CStr());
        return false;
    }

    AResource* resource = FindResource(_Resource->FinalClassMeta(), _Alias, bMetadataMismatch, hash);
    if (resource || bMetadataMismatch)
    {
        GLogger.Printf("RegisterResource: Resource with same alias already exists (%s)\n", _Alias);
        return false;
    }

    _Resource->AddRef();
    _Resource->SetResourcePath(_Alias);
    ResourceHash.Insert(hash, ResourceCache.Size());
    ResourceCache.Append(_Resource);

    return true;
}

bool AResourceManager::UnregisterResource(AResource* _Resource)
{
    int hash = _Resource->GetResourcePath().HashCase();
    int i;

    for (i = ResourceHash.First(hash); i != -1; i = ResourceHash.Next(i))
    {
        if (!ResourceCache[i]->GetResourcePath().Icmp(_Resource->GetResourcePath()))
        {
            if (&ResourceCache[i]->FinalClassMeta() != &_Resource->FinalClassMeta())
            {
                GLogger.Printf("UnregisterResource: %s class doesn't match meta data (%s vs %s)\n", _Resource->GetResourcePath().CStr(), ResourceCache[i]->FinalClassName(), _Resource->FinalClassMeta().GetName());
                return false;
            }
            break;
        }
    }

    if (i == -1)
    {
        GLogger.Printf("UnregisterResource: resource %s is not found\n", _Resource->GetResourcePath().CStr());
        return false;
    }

    _Resource->SetResourcePath("");
    _Resource->RemoveRef();
    ResourceHash.RemoveIndex(hash, i);
    ResourceCache.Remove(i);
    return true;
}

void AResourceManager::UnregisterResources(AClassMeta const& _ClassMeta)
{
    for (int i = ResourceCache.Size() - 1; i >= 0; i--)
    {
        if (ResourceCache[i]->FinalClassId() == _ClassMeta.GetId())
        {
            AResource* resource = ResourceCache[i];

            ResourceHash.RemoveIndex(resource->GetResourcePath().HashCase(), i);
            ResourceCache.Remove(i);

            resource->RemoveRef();
        }
    }
}

void AResourceManager::UnregisterResources()
{
    for (int i = ResourceCache.Size() - 1; i >= 0; i--)
    {
        ResourceCache[i]->RemoveRef();
    }
    ResourceHash.Clear();
    ResourceCache.Clear();
}
