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

class AResource : public ABaseObject
{
    HK_CLASS(AResource, ABaseObject)

    friend class AResourceManager;

public:
    template <typename ResourceType>
    static ResourceType* CreateDefault()
    {
        ResourceType* resource = CreateInstanceOf<ResourceType>();
        resource->InitializeDefaultObject();
        return resource;
    }

    template <typename ResourceType>
    static ResourceType* CreateFromFile(AStringView Path)
    {
        ResourceType* resource = CreateInstanceOf<ResourceType>();
        resource->InitializeFromFile(Path);
        return resource;
    }

    /** Get physical resource path */
    AString const& GetResourcePath() const { return m_ResourcePath; }

    static bool IsResourceExists(AStringView Path);

protected:
    AResource() {}

    /** Initialize default object representation */
    void InitializeDefaultObject();

    /** Initialize object from file */
    void InitializeFromFile(AStringView Path);

    /** Load resource from file */
    virtual bool LoadResource(IBinaryStreamReadInterface& _Stream) { return false; }

    /** Create internal resource */
    virtual void LoadInternalResource(AStringView Path) {}

    virtual const char* GetDefaultResourcePath() const { return "/Default/UnknownResource"; }

private:
    bool LoadFromPath(AStringView Path);

    /** Set physical resource path */
    void SetResourcePath(AStringView _ResourcePath) { m_ResourcePath = _ResourcePath; }

    AString m_ResourcePath;
};

class ABinaryResource : public AResource
{
    HK_CLASS(ABinaryResource, AResource)

public:
    ABinaryResource();
    ~ABinaryResource();

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
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(AStringView Path) override;

private:
    void*  m_pBinaryData = nullptr;
    size_t m_SizeInBytes = 0;
};
