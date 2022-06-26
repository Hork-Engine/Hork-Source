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

#include <Platform/Memory/Memory.h>
#include <Platform/Logger.h>
#include <Core/HashFunc.h>

HK_CLASS_META(ADummy)

AObjectFactory* AObjectFactory::FactoryList = nullptr;

void InitializeFactories()
{
}

void DeinitializeFactories()
{
    for (AObjectFactory* factory = AObjectFactory::FactoryList; factory; factory = factory->NextFactory)
    {
        Platform::GetHeapAllocator<HEAP_MISC>().Free(factory->IdTable);
        factory->IdTable = nullptr;
        factory->LookupTable.Clear();
    }
}

AObjectFactory::AObjectFactory(const char* Tag) :
    Tag(Tag), Classes(nullptr), IdTable(nullptr), NumClasses(0)
{
    NextFactory = FactoryList;
    FactoryList = this;
}

AObjectFactory::~AObjectFactory()
{
    HK_ASSERT(IdTable == nullptr);
}

const AClassMeta* AObjectFactory::FindClass(AStringView ClassName) const
{
    for (AClassMeta const* n = Classes; n; n = n->pNext)
    {
        if (!ClassName.Cmp(n->GetName()))
        {
            return n;
        }
    }
    return nullptr;
}

const AClassMeta* AObjectFactory::LookupClass(AStringView ClassName) const
{
    if (LookupTable.IsEmpty())
    {
        // init name table
        for (AClassMeta* n = Classes; n; n = n->pNext)
        {
            LookupTable[n->GetName()] = n;
        }
    }

    auto it = LookupTable.Find(ClassName);
    return it != LookupTable.End() ? it->second : nullptr;
}

const AClassMeta* AObjectFactory::LookupClass(uint64_t ClassId) const
{
    if (ClassId > NumClasses)
    {
        // invalid class id
        return nullptr;
    }

    if (!IdTable)
    {
        // init lookup table
        IdTable    = (AClassMeta**)Platform::GetHeapAllocator<HEAP_MISC>().Alloc((NumClasses + 1) * sizeof(*IdTable));
        IdTable[0] = nullptr;
        for (AClassMeta* n = Classes; n; n = n->pNext)
        {
            IdTable[n->GetId()] = n;
        }
    }

    return IdTable[ClassId];
}

AProperty const* AClassMeta::FindProperty(AStringView PropertyName, bool bRecursive) const
{
    for (AProperty const* prop = PropertyList; prop; prop = prop->Next())
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

void AClassMeta::GetProperties(TPodVector<AProperty const*>& Properties, bool bRecursive) const
{
    if (bRecursive && pSuperClass)
    {
        pSuperClass->GetProperties(Properties, true);
    }
    for (AProperty const* prop = PropertyList; prop; prop = prop->Next())
    {
        Properties.Add(prop);
    }
}

void AClassMeta::CloneProperties_r(AClassMeta const* Meta, ADummy const* Template, ADummy* Destination)
{
    if (Meta)
    {
        CloneProperties_r(Meta->SuperClass(), Template, Destination);

        for (AProperty const* prop = Meta->GetPropertyList(); prop; prop = prop->Next())
        {
            prop->CopyValue(Destination, Template);
        }
    }
}

void AClassMeta::CloneProperties(ADummy const* Template, ADummy* Destination)
{
    if (&Template->FinalClassMeta() != &Destination->FinalClassMeta())
    {
        LOG("AClassMeta::CloneProperties: Template is not an {} class\n", Destination->FinalClassName());
        return;
    }

    CloneProperties_r(&Template->FinalClassMeta(), Template, Destination);
}
