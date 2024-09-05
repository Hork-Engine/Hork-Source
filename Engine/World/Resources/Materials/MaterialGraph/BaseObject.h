/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "Factory.h"

HK_NAMESPACE_BEGIN

/**

BaseObject

Base object class.

*/
class BaseObject : public RefCounted
{
public:
    typedef BaseObject                                 ThisClass;
    typedef Allocators::HeapMemoryAllocator<HEAP_MISC> Allocator;
    class ThisClassMeta : public ClassMeta
    {
    public:
        ThisClassMeta() :
            ClassMeta(ClassMeta::sDummyFactory(), "BaseObject"_s, nullptr)
        {}

        Ref<BaseObject> CreateInstance() const override
        {
            return MakeRef<ThisClass>();
        }
    };

    _HK_GENERATED_CLASS_BODY()

    void SetProperties(StringHashMap<String> const& Properties);

    bool SetProperty(StringView PropertyName, StringView PropertyValue);

    Property const* FindProperty(StringView PropertyName, bool bRecursive) const;

    void GetProperties(PropertyList& Properties, bool bRecursive = true) const;

private:
    void SetProperties_r(ClassMeta const* Meta, StringHashMap<String> const& Properties);
};

HK_NAMESPACE_END
