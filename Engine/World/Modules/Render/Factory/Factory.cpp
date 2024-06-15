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

#include "Factory.h"
#include "BaseObject.h"

HK_NAMESPACE_BEGIN

ObjectFactory* ObjectFactory::m_FactoryList = nullptr;

ObjectFactory::ObjectFactory(const char* Tag) :
    m_Tag(Tag), m_Classes(nullptr), m_NumClasses(0)
{
    m_NextFactory = m_FactoryList;
    m_FactoryList = this;
}

const ClassMeta* ObjectFactory::FindClass(StringView ClassName) const
{
    for (ClassMeta const* n = m_Classes; n; n = n->m_pNext)
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
    if (m_LookupTable.IsEmpty())
    {
        // init name table
        for (ClassMeta* n = m_Classes; n; n = n->m_pNext)
        {
            m_LookupTable[n->GetName()] = n;
        }
    }

    auto it = m_LookupTable.Find(ClassName);
    return it != m_LookupTable.End() ? it->second : nullptr;
}

const ClassMeta* ObjectFactory::LookupClass(uint64_t ClassId) const
{
    if (ClassId > m_NumClasses)
    {
        // invalid class id
        return nullptr;
    }

    if (m_IdTable.IsEmpty())
    {
        // init lookup table
        m_IdTable.Resize(m_NumClasses + 1);
        m_IdTable[0] = nullptr;
        for (ClassMeta* n = m_Classes; n; n = n->m_pNext)
        {
            m_IdTable[n->GetId()] = n;
        }
    }

    return m_IdTable[ClassId];
}

Property const* ClassMeta::FindProperty(StringView PropertyName, bool bRecursive) const
{
    for (Property const* prop = m_PropertyList; prop; prop = prop->Next())
    {
        if (PropertyName == prop->GetName())
        {
            return prop;
        }
    }
    if (bRecursive && m_pSuperClass)
    {
        return m_pSuperClass->FindProperty(PropertyName, true);
    }
    return nullptr;
}

void ClassMeta::GetProperties(PropertyList& Properties, bool bRecursive) const
{
    if (bRecursive && m_pSuperClass)
    {
        m_pSuperClass->GetProperties(Properties, true);
    }
    for (Property const* prop = m_PropertyList; prop; prop = prop->Next())
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

HK_NAMESPACE_END
