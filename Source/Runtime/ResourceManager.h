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

    void AddResourcePack(AStringView fileName);

    /** Find file in resource packs */
    bool FindFile(AStringView fileName, int* pResourcePackIndex, AFileHandle* pFileHandle) const;

    /** Get or create resource. Return default object if fails. */
    template <typename T>
    HK_FORCEINLINE T* GetOrCreateResource(AStringView path, RESOURCE_FLAGS flags = RESOURCE_FLAG_DEFAULT)
    {
        return static_cast<T*>(GetOrCreateResource(T::ClassMeta(), path, flags));
    }

    /** Get resource. Return default object if fails. */
    template <typename T>
    HK_FORCEINLINE T* GetResource(AStringView path, bool* bResourceFoundResult = nullptr, bool* bMetadataMismatch = nullptr)
    {
        return static_cast<T*>(GetResource(T::ClassMeta(), path, bResourceFoundResult, bMetadataMismatch));
    }

    /** Get or create resource. Return default object if fails. */
    AResource* GetOrCreateResource(AClassMeta const& classMeta, AStringView path, RESOURCE_FLAGS flags = RESOURCE_FLAG_DEFAULT);

    /** Get resource. Return default object if fails. */
    AResource* GetResource(AClassMeta const& classMeta, AStringView path, bool* bResourceFoundResult = nullptr, bool* bMetadataMismatch = nullptr);

    /** Get resource meta. Return null if fails. */
    AClassMeta const* GetResourceInfo(AStringView path);

    /** Find resource in cache. Return null if fails. */
    template <typename T>
    T* FindResource(AStringView path, bool& bMetadataMismatch)
    {
        return static_cast<T*>(FindResource(T::ClassMeta(), path, bMetadataMismatch));
    }

    /** Find resource in cache. Return null if fails. */
    AResource* FindResource(AClassMeta const& classMeta, AStringView path, bool& bMetadataMismatch);

    /** Find resource in cache. Return null if fails. */
    AResource* FindResource(AStringView path);

    /** Register object as resource. */
    bool RegisterResource(AResource* resource, AStringView path);

    /** Unregister object as resource. */
    bool UnregisterResource(AResource* resource);

    /** Unregister all resources with same meta. */
    void UnregisterResources(AClassMeta const& classMeta);

    /** Unregister all resources by type. */
    template <typename T>
    HK_FORCEINLINE void UnregisterResources()
    {
        UnregisterResources(T::ClassMeta());
    }

    /** Unregister all resources. */
    void UnregisterResources();

    void RemoveUnreferencedResources();

    TVector<AArchive> const& GetResourcePacks() const { return m_ResourcePacks; }
    AArchive const& GetCommonResources() const { return m_CommonResources; }

    bool IsResourceExists(AStringView path);
    AFile OpenResource(AStringView path);

private:
    TNameHash<AResource*> m_ResourceCache;
    TVector<AArchive>     m_ResourcePacks;
    AArchive              m_CommonResources;
};


/*

Helpers

*/

/** Get or create resource. Return default object if fails. */
template <typename T>
HK_FORCEINLINE T* GetOrCreateResource(AStringView path, RESOURCE_FLAGS flags = RESOURCE_FLAG_DEFAULT)
{
    return GEngine->GetResourceManager()->GetOrCreateResource<T>(path, flags);
}

/** Get resource. Return default object if fails. */
template <typename T>
HK_FORCEINLINE T* GetResource(AStringView path, bool* bResourceFoundResult = nullptr, bool* bMetadataMismatch = nullptr)
{
    return GEngine->GetResourceManager()->GetResource<T>(path, bResourceFoundResult, bMetadataMismatch);
}

/** Get resource meta. Return null if fails. */
HK_FORCEINLINE AClassMeta const* GetResourceInfo(AStringView path)
{
    return GEngine->GetResourceManager()->GetResourceInfo(path);
}

/** Find resource in cache. Return null if fails. */
HK_FORCEINLINE AResource* FindResource(AClassMeta const& classMeta, AStringView path, bool& bMetadataMismatch)
{
    return GEngine->GetResourceManager()->FindResource(classMeta, path, bMetadataMismatch);
}

/** Find resource in cache. Return null if fails. */
template <typename T>
HK_FORCEINLINE T* FindResource(AStringView path, bool& bMetadataMismatch)
{
    return static_cast<T*>(FindResource(T::ClassMeta(), path, bMetadataMismatch));
}

/** Find resource in cache. Return null if fails. */
template <typename T>
HK_FORCEINLINE T* FindResource(AStringView path)
{
    bool bMetadataMismatch;
    return static_cast<T*>(FindResource(T::ClassMeta(), path, bMetadataMismatch));
}

/** Find resource in cache. Return null if fails. */
HK_FORCEINLINE AResource* FindResource(AStringView path)
{
    return GEngine->GetResourceManager()->FindResource(path);
}

/** Register object as resource. */
HK_FORCEINLINE bool RegisterResource(AResource* resource, AStringView path)
{
    return GEngine->GetResourceManager()->RegisterResource(resource, path);
}

/** Unregister object as resource. */
HK_FORCEINLINE bool UnregisterResource(AResource* resource)
{
    return GEngine->GetResourceManager()->UnregisterResource(resource);
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
static TStaticResourceFinder<AIndexedMesh> Resource("/Root/Meshes/MyMesh.asset"s);
AIndexedMesh * mesh = Resource.GetObject();

*/
template <typename T>
struct TStaticResourceFinder
{
    TStaticResourceFinder(AGlobalStringView path) :
        m_ResourcePath(path)
    {
        m_Object = GetOrCreateResource<T>(m_ResourcePath);
    }

    T* GetObject()
    {
        if (m_Object.IsExpired())
        {
            m_Object = GetOrCreateResource<T>(m_ResourcePath);
        }
        return m_Object;
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
    AStringView m_ResourcePath;
    TWeakRef<T> m_Object;
};
