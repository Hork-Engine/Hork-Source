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

#include "BaseObject.h"

HK_NAMESPACE_BEGIN

enum RESOURCE_FLAGS : uint32_t
{
    RESOURCE_FLAG_DEFAULT = 0,
    RESOURCE_FLAG_PERSISTENT = 1
};

HK_FLAG_ENUM_OPERATORS(RESOURCE_FLAGS)

class Resource : public BaseObject
{
    HK_CLASS(Resource, BaseObject)

    friend class ResourceManager;

public:
    template <typename ResourceType>
    static ResourceType* CreateDefault()
    {
        ResourceType* resource = NewObj<ResourceType>();
        resource->InitializeDefaultObject();
        return resource;
    }

    template <typename ResourceType>
    static ResourceType* CreateFromFile(StringView path)
    {
        ResourceType* resource = NewObj<ResourceType>();
        resource->InitializeFromFile(path);
        return resource;
    }

    /** Get physical resource path */
    String const& GetResourcePath() const { return m_ResourcePath; }

    bool IsManualResource() const { return m_bManualResource; }

    bool IsPersistent() const { return !!(m_Flags & RESOURCE_FLAG_PERSISTENT); }

protected:
    Resource() = default;

    /** Initialize default object representation */
    void InitializeDefaultObject();

    /** Initialize object from file */
    void InitializeFromFile(StringView path);

    /** Load resource from file */
    virtual bool LoadResource(IBinaryStreamReadInterface& stream) { return false; }

    /** Create internal resource */
    virtual void LoadInternalResource(StringView path) {}

    virtual const char* GetDefaultResourcePath() const { return "/Default/UnknownResource"; }

private:
    bool LoadFromPath(StringView path);

    /** Set resource path */
    void SetResourcePath(StringView path) { m_ResourcePath = path; }

    void SetManualResource(bool bManualResource) { m_bManualResource = bManualResource; }

    void SetResourceFlags(RESOURCE_FLAGS Flags) { m_Flags = Flags; }

    String m_ResourcePath;
    bool m_bManualResource = false;
    RESOURCE_FLAGS m_Flags = RESOURCE_FLAG_DEFAULT;
};

class BinaryResource : public Resource
{
    HK_CLASS(BinaryResource, Resource)

public:
    BinaryResource() = default;
    ~BinaryResource();

    void* GetBinaryData()
    {
        return m_pBinaryData;
    }

    size_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }

    const char* GetAsString() const
    {
        return m_pBinaryData ? (const char*)m_pBinaryData : "";
    }
protected:

    void Purge();

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView path) override;

private:
    void*  m_pBinaryData = nullptr;
    size_t m_SizeInBytes = 0;
};

HK_NAMESPACE_END
