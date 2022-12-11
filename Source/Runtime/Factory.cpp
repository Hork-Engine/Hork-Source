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

#include "Factory.h"
#include "BaseObject.h"

ObjectFactory* ObjectFactory::FactoryList = nullptr;

ObjectFactory::ObjectFactory(const char* Tag) :
    Tag(Tag), Classes(nullptr), NumClasses(0)
{
    NextFactory = FactoryList;
    FactoryList = this;
}

const ClassMeta* ObjectFactory::FindClass(StringView ClassName) const
{
    for (ClassMeta const* n = Classes; n; n = n->pNext)
    {
        if (!ClassName.Cmp(n->GetName()))
        {
            return n;
        }
    }
    return nullptr;
}

const ClassMeta* ObjectFactory::LookupClass(StringView ClassName) const
{
    if (LookupTable.IsEmpty())
    {
        // init name table
        for (ClassMeta* n = Classes; n; n = n->pNext)
        {
            LookupTable[n->GetName()] = n;
        }
    }

    auto it = LookupTable.Find(ClassName);
    return it != LookupTable.End() ? it->second : nullptr;
}

const ClassMeta* ObjectFactory::LookupClass(uint64_t ClassId) const
{
    if (ClassId > NumClasses)
    {
        // invalid class id
        return nullptr;
    }

    if (IdTable.IsEmpty())
    {
        // init lookup table
        IdTable.Resize(NumClasses + 1);
        IdTable[0] = nullptr;
        for (ClassMeta* n = Classes; n; n = n->pNext)
        {
            IdTable[n->GetId()] = n;
        }
    }

    return IdTable[ClassId];
}

Property const* ClassMeta::FindProperty(StringView PropertyName, bool bRecursive) const
{
    for (Property const* prop = PropertyList; prop; prop = prop->Next())
    {
        if (PropertyName == prop->GetName())
        {
            return prop;
        }
    }
    if (bRecursive && pSuperClass)
    {
        return pSuperClass->FindProperty(PropertyName, true);
    }
    return nullptr;
}

void ClassMeta::GetProperties(APropertyList& Properties, bool bRecursive) const
{
    if (bRecursive && pSuperClass)
    {
        pSuperClass->GetProperties(Properties, true);
    }
    for (Property const* prop = PropertyList; prop; prop = prop->Next())
    {
        Properties.Add(prop);
    }
}

void ClassMeta::CloneProperties_r(ClassMeta const* Meta, BaseObject const* Template, BaseObject* Destination)
{
    if (Meta)
    {
        CloneProperties_r(Meta->SuperClass(), Template, Destination);

        for (Property const* prop = Meta->GetPropertyList(); prop; prop = prop->Next())
        {
            prop->CopyValue(Destination, Template);
        }
    }
}

void ClassMeta::CloneProperties(BaseObject const* Template, BaseObject* Destination)
{
    if (&Template->FinalClassMeta() != &Destination->FinalClassMeta())
    {
        LOG("ClassMeta::CloneProperties: Template is not an {} class\n", Destination->FinalClassName());
        return;
    }

    CloneProperties_r(&Template->FinalClassMeta(), Template, Destination);
}
