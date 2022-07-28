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

#pragma once

#include "Resource.h"
#include "Engine.h"

class AResourceManager
{
public:
    AResourceManager();
    virtual ~AResourceManager();

    void AddResourcePack(AStringView FileName);

    /** Find file in resource packs */
    bool FindFile(AStringView FileName, int* pResourcePackIndex, int* pFileIndex) const;

    /** Get or create resource. Return default object if fails. */
    template <typename T>
    HK_FORCEINLINE T* GetOrCreateResource(AStringView _Path)
    {
        return static_cast<T*>(GetOrCreateResource(T::ClassMeta(), _Path));
    }

    /** Get resource. Return default object if fails. */
    template <typename T>
    HK_FORCEINLINE T* GetResource(AStringView _Alias, bool* _bResourceFoundResult = nullptr, bool* _bMetadataMismatch = nullptr)
    {
        return static_cast<T*>(GetResource(T::ClassMeta(), _Alias, _bResourceFoundResult, _bMetadataMismatch));
    }

    /** Get or create resource. Return default object if fails. */
    AResource* GetOrCreateResource(AClassMeta const& _ClassMeta, AStringView _Path);

    /** Get resource. Return default object if fails. */
    AResource* GetResource(AClassMeta const& _ClassMeta, AStringView _Alias, bool* _bResourceFoundResult = nullptr, bool* _bMetadataMismatch = nullptr);

    /** Get resource meta. Return null if fails. */
    AClassMeta const* GetResourceInfo(AStringView _Alias);

    /** Find resource in cache. Return null if fails. */
    template <typename T>
    T* FindResource(AStringView _Alias, bool& _bMetadataMismatch)
    {
        return static_cast<T*>(FindResource(T::ClassMeta(), _Alias, _bMetadataMismatch));
    }

    /** Find resource in cache. Return null if fails. */
    AResource* FindResource(AClassMeta const& _ClassMeta, AStringView _Alias, bool& _bMetadataMismatch);

    /** Find resource in cache. Return null if fails. */
    AResource* FindResourceByAlias(AStringView _Alias);

    /** Register object as resource. */
    bool RegisterResource(AResource* _Resource, AStringView _Alias);

    /** Unregister object as resource. */
    bool UnregisterResource(AResource* _Resource);

    /** Unregister all resources with same meta. */
    void UnregisterResources(AClassMeta const& _ClassMeta);

    /** Unregister all resources by type. */
    template <typename T>
    HK_FORCEINLINE void UnregisterResources()
    {
        UnregisterResources(T::ClassMeta());
    }

    /** Unregister all resources. */
    void UnregisterResources();

    TVector<AArchive> const& GetResourcePacks() const { return ResourcePacks; }
    AArchive const& GetCommonResources() const { return CommonResources; }

    bool  IsResourceExists(AStringView Path);
    AFile OpenResource(AStringView Path);

private:
    TNameHash<AResource*> ResourceCache;
    TVector<AArchive>     ResourcePacks;
    AArchive              CommonResources;
};


/*

Helpers

*/

/** Get or create resource. Return default object if fails. */
template <typename T>
HK_FORCEINLINE T* GetOrCreateResource(AStringView _Path)
{
    return GEngine->GetResourceManager()->GetOrCreateResource<T>(_Path);
}

/** Get resource. Return default object if fails. */
template <typename T>
HK_FORCEINLINE T* GetResource(AStringView _Alias, bool* _bResourceFoundResult = nullptr, bool* _bMetadataMismatch = nullptr)
{
    return GEngine->GetResourceManager()->GetResource<T>(_Alias, _bResourceFoundResult, _bMetadataMismatch);
}

/** Get resource meta. Return null if fails. */
HK_FORCEINLINE AClassMeta const* GetResourceInfo(AStringView _Alias)
{
    return GEngine->GetResourceManager()->GetResourceInfo(_Alias);
}

/** Find resource in cache. Return null if fails. */
HK_FORCEINLINE AResource* FindResource(AClassMeta const& _ClassMeta, AStringView _Alias, bool& _bMetadataMismatch)
{
    return GEngine->GetResourceManager()->FindResource(_ClassMeta, _Alias, _bMetadataMismatch);
}

/** Find resource in cache. Return null if fails. */
template <typename T>
HK_FORCEINLINE T* FindResource(AStringView _Alias, bool& _bMetadataMismatch)
{
    return static_cast<T*>(FindResource(T::ClassMeta(), _Alias, _bMetadataMismatch));
}

/** Find resource in cache. Return null if fails. */
HK_FORCEINLINE AResource* FindResourceByAlias(AStringView _Alias)
{
    return GEngine->GetResourceManager()->FindResourceByAlias(_Alias);
}

/** Register object as resource. */
HK_FORCEINLINE bool RegisterResource(AResource* _Resource, AStringView _Alias)
{
    return GEngine->GetResourceManager()->RegisterResource(_Resource, _Alias);
}

/** Unregister object as resource. */
HK_FORCEINLINE bool UnregisterResource(AResource* _Resource)
{
    return GEngine->GetResourceManager()->UnregisterResource(_Resource);
}

/** Unregister all resources by type. */
template <typename T>
HK_FORCEINLINE void UnregisterResources()
{
    GEngine->GetResourceManager()->UnregisterResources<T>();
}

/** Unregister all resources. */
HK_FORCEINLINE void UnregisterResources()
{
    GEngine->GetResourceManager()->UnregisterResources();
}

/**

Static resource finder

Usage:
static TStaticResourceFinder< AIndexedMesh > Resource("/Root/Meshes/MyMesh.asset"s);
AIndexedMesh * mesh = Resource.GetObject();

*/
template <typename T>
struct TStaticResourceFinder
{
    TStaticResourceFinder(AGlobalStringView _Path) :
        ResourcePath(_Path)
    {
        Object = GetOrCreateResource<T>(ResourcePath);
    }

    T* GetObject()
    {
        if (Object.IsExpired())
        {
            Object = GetOrCreateResource<T>(ResourcePath);
        }
        return Object;
    }

    operator T* ()
    {
        return GetObject();
    }

    T* operator->()
    {
        return GetObject();
    }

private:
    AStringView ResourcePath;
    TWeakRef<T> Object;
};
