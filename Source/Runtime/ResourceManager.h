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

class ResourceFactory : public RefCounted
{
public:
    virtual bool IsResourceExists(StringView Path)
    {
        return false;
    }

    virtual File OpenResource(StringView Path)
    {
        return {};
    }
};

class ResourceManager
{
public:
    ResourceManager();
    virtual ~ResourceManager();

    /** Add custom resource factory */
    void AddResourceFactory(ResourceFactory* Factory);

    void AddResourcePack(StringView fileName);

    /** Find file in resource packs */
    bool FindFile(StringView fileName, int* pResourcePackIndex, FileHandle* pFileHandle) const;

    /** Get or create resource. Return default object if fails. */
    template <typename T>
    HK_FORCEINLINE T* GetOrCreateResource(StringView path, RESOURCE_FLAGS flags = RESOURCE_FLAG_DEFAULT)
    {
        return static_cast<T*>(GetOrCreateResource(T::GetClassMeta(), path, flags));
    }

    /** Get resource. Return default object if fails. */
    template <typename T>
    HK_FORCEINLINE T* GetResource(StringView path, bool* bResourceFoundResult = nullptr, bool* bMetadataMismatch = nullptr)
    {
        return static_cast<T*>(GetResource(T::GetClassMeta(), path, bResourceFoundResult, bMetadataMismatch));
    }

    /** Get or create resource. Return default object if fails. */
    Resource* GetOrCreateResource(ClassMeta const& classMeta, StringView path, RESOURCE_FLAGS flags = RESOURCE_FLAG_DEFAULT);

    /** Get resource. Return default object if fails. */
    Resource* GetResource(ClassMeta const& classMeta, StringView path, bool* bResourceFoundResult = nullptr, bool* bMetadataMismatch = nullptr);

    /** Get resource meta. Return null if fails. */
    ClassMeta const* GetResourceInfo(StringView path);

    /** Find resource in cache. Return null if fails. */
    template <typename T>
    T* FindResource(StringView path, bool& bMetadataMismatch)
    {
        return static_cast<T*>(FindResource(T::ClassMeta(), path, bMetadataMismatch));
    }

    /** Find resource in cache. Return null if fails. */
    Resource* FindResource(ClassMeta const& classMeta, StringView path, bool& bMetadataMismatch);

    /** Find resource in cache. Return null if fails. */
    Resource* FindResource(StringView path);

    /** Register object as resource. */
    bool RegisterResource(Resource* resource, StringView path);

    /** Unregister object as resource. */
    bool UnregisterResource(Resource* resource);

    /** Unregister all resources with same meta. */
    void UnregisterResources(ClassMeta const& classMeta);

    /** Unregister all resources by type. */
    template <typename T>
    HK_FORCEINLINE void UnregisterResources()
    {
        UnregisterResources(T::ClassMeta());
    }

    /** Unregister all resources. */
    void UnregisterResources();

    void RemoveUnreferencedResources();

    TVector<Archive> const& GetResourcePacks() const { return m_ResourcePacks; }
    Archive const& GetCommonResources() const { return m_CommonResources; }

    bool IsResourceExists(StringView path);
    File OpenResource(StringView path);

private:
    TVector<TRef<ResourceFactory>> m_ResourceFactories;
    TNameHash<Resource*> m_ResourceCache;
    TVector<Archive>     m_ResourcePacks;
    Archive              m_CommonResources;
};


/*

Helpers

*/

/** Get or create resource. Return default object if fails. */
template <typename T>
HK_FORCEINLINE T* GetOrCreateResource(StringView path, RESOURCE_FLAGS flags = RESOURCE_FLAG_DEFAULT)
{
    return GEngine->GetResourceManager()->GetOrCreateResource<T>(path, flags);
}

/** Get resource. Return default object if fails. */
template <typename T>
HK_FORCEINLINE T* GetResource(StringView path, bool* bResourceFoundResult = nullptr, bool* bMetadataMismatch = nullptr)
{
    return GEngine->GetResourceManager()->GetResource<T>(path, bResourceFoundResult, bMetadataMismatch);
}

/** Get resource meta. Return null if fails. */
HK_FORCEINLINE ClassMeta const* GetResourceInfo(StringView path)
{
    return GEngine->GetResourceManager()->GetResourceInfo(path);
}

/** Find resource in cache. Return null if fails. */
HK_FORCEINLINE Resource* FindResource(ClassMeta const& classMeta, StringView path, bool& bMetadataMismatch)
{
    return GEngine->GetResourceManager()->FindResource(classMeta, path, bMetadataMismatch);
}

/** Find resource in cache. Return null if fails. */
template <typename T>
HK_FORCEINLINE T* FindResource(StringView path, bool& bMetadataMismatch)
{
    return static_cast<T*>(FindResource(T::GetClassMeta(), path, bMetadataMismatch));
}

/** Find resource in cache. Return null if fails. */
template <typename T>
HK_FORCEINLINE T* FindResource(StringView path)
{
    bool bMetadataMismatch;
    return static_cast<T*>(FindResource(T::GetClassMeta(), path, bMetadataMismatch));
}

/** Find resource in cache. Return null if fails. */
HK_FORCEINLINE Resource* FindResource(StringView path)
{
    return GEngine->GetResourceManager()->FindResource(path);
}

/** Register object as resource. */
HK_FORCEINLINE bool RegisterResource(Resource* resource, StringView path)
{
    return GEngine->GetResourceManager()->RegisterResource(resource, path);
}

/** Unregister object as resource. */
HK_FORCEINLINE bool UnregisterResource(Resource* resource)
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
static TStaticResourceFinder<IndexedMesh> Resource("/Root/Meshes/MyMesh.asset"s);
IndexedMesh * mesh = Resource.GetObject();

*/
template <typename T>
struct TStaticResourceFinder
{
    TStaticResourceFinder(GlobalStringView path) :
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
    StringView m_ResourcePath;
    TWeakRef<T> m_Object;
};
