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

#include "BaseObject.h"
#include <Core/IntrusiveLinkedListMacro.h>

HK_CLASS_META(ABaseObject)

uint64_t ABaseObject::TotalObjects = 0;

ABaseObject* ABaseObject::Objects     = nullptr;
ABaseObject* ABaseObject::ObjectsTail = nullptr;

static uint64_t GUniqueIdGenerator = 0;

ABaseObject::ABaseObject() :
    Id(++GUniqueIdGenerator)
{
    INTRUSIVE_ADD(this, NextObject, PrevObject, Objects, ObjectsTail);
    TotalObjects++;
    AGarbageCollector::AddObject(this);
}

ABaseObject::~ABaseObject()
{
    INTRUSIVE_REMOVE(this, NextObject, PrevObject, Objects, ObjectsTail);

    TotalObjects--;

    if (WeakRefCounter)
    {
        WeakRefCounter->Object = nullptr;
    }
}

void ABaseObject::AddRef()
{
    HK_ASSERT_(RefCount != -666, "Calling AddRef() in destructor");
    ++RefCount;
    if (RefCount == 1)
    {
        AGarbageCollector::RemoveObject(this);
    }
}

void ABaseObject::RemoveRef()
{
    HK_ASSERT_(RefCount != -666, "Calling RemoveRef() in destructor");
    if (--RefCount == 0)
    {
        AGarbageCollector::AddObject(this);
        return;
    }
    HK_ASSERT(RefCount > 0);
}

ABaseObject* ABaseObject::FindObject(uint64_t _Id)
{
    if (!_Id)
    {
        return nullptr;
    }

    // FIXME: use hash/map?
    for (ABaseObject* object = Objects; object; object = object->NextObject)
    {
        if (object->Id == _Id)
        {
            return object;
        }
    }
    return nullptr;
}

void ABaseObject::SetProperties_r(AClassMeta const* Meta, THashContainer<AString, AString> const& Properties)
{
    if (Meta)
    {
        SetProperties_r(Meta->SuperClass(), Properties);

        for (AProperty const* prop = Meta->GetPropertyList(); prop; prop = prop->Next())
        {
            int hash = prop->GetNameHash();

            for (int i = Properties.First(hash); i != -1; i = Properties.Next(i))
            {
                if (!Properties[i].first.Icmp(prop->GetName()))
                {
                    // Property found
                    prop->SetValue(this, AVariant(prop->GetType(), prop->GetEnum(), Properties[i].second));
                    break;
                }
            }
        }
    }
}

void ABaseObject::SetProperties(THashContainer<AString, AString> const& Properties)
{
    if (Properties.IsEmpty())
    {
        return;
    }
    SetProperties_r(&FinalClassMeta(), Properties);
}

bool ABaseObject::SetProperty(AStringView PropertyName, AStringView PropertyValue)
{
    AProperty const* prop = FinalClassMeta().FindProperty(PropertyName, true);
    if (!prop)
        return false;

    prop->SetValue(this, AVariant(prop->GetType(), prop->GetEnum(), PropertyValue));
    return true;
}

#if 0
bool ABaseObject::SetProperty(AStringView PropertyName, AVariant const& PropertyValue)
{
    AProperty const* prop = FinalClassMeta().FindProperty(PropertyName, true);
    if (!prop)
        return false;

    prop->SetValue(this, PropertyValue);
    return true;
}
#endif

AProperty const* ABaseObject::FindProperty(AStringView PropertyName, bool bRecursive) const
{
    return FinalClassMeta().FindProperty(PropertyName, bRecursive);
}

void ABaseObject::GetProperties(TPodVector<AProperty const*>& Properties, bool bRecursive) const
{
    FinalClassMeta().GetProperties(Properties, bRecursive);
}

ABaseObject* AGarbageCollector::GarbageObjects     = nullptr;
ABaseObject* AGarbageCollector::GarbageObjectsTail = nullptr;

void AGarbageCollector::AddObject(ABaseObject* _Object)
{
    INTRUSIVE_ADD(_Object, NextGarbageObject, PrevGarbageObject, GarbageObjects, GarbageObjectsTail)
}

void AGarbageCollector::RemoveObject(ABaseObject* _Object)
{
    INTRUSIVE_REMOVE(_Object, NextGarbageObject, PrevGarbageObject, GarbageObjects, GarbageObjectsTail)
}

void AGarbageCollector::DeallocateObjects()
{
    while (GarbageObjects)
    {
        ABaseObject* object = GarbageObjects;

        // Mark RefCount to prevent using of AddRef/RemoveRef in the object destructor
        object->RefCount = -666;

        RemoveObject(object);

        delete object;
    }

    GarbageObjectsTail = nullptr;
}
