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

AN_CLASS_META(ABaseObject)

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
    AN_ASSERT_(RefCount != -666, "Calling AddRef() in destructor");
    ++RefCount;
    if (RefCount == 1)
    {
        AGarbageCollector::RemoveObject(this);
    }
}

void ABaseObject::RemoveRef()
{
    AN_ASSERT_(RefCount != -666, "Calling RemoveRef() in destructor");
    if (--RefCount == 0)
    {
        AGarbageCollector::AddObject(this);
        return;
    }
    AN_ASSERT(RefCount > 0);
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

void ABaseObject::SetAttributes_r(AClassMeta const* Meta, THashContainer<AString, AString> const& Attributes)
{
    if (Meta)
    {
        SetAttributes_r(Meta->SuperClass(), Attributes);

        for (AAttributeMeta const* attrib = Meta->GetAttribList(); attrib; attrib = attrib->Next())
        {
            int hash = attrib->GetNameHash();

            for (int i = Attributes.First(hash); i != -1; i = Attributes.Next(i))
            {
                if (!Attributes[i].first.Icmp(attrib->GetName()))
                {
                    // Attribute found
                    attrib->SetValue(this, Attributes[i].second);
                    break;
                }
            }
        }
    }
}

void ABaseObject::SetAttributes(THashContainer<AString, AString> const& Attributes)
{
    if (Attributes.IsEmpty())
    {
        return;
    }
    SetAttributes_r(&FinalClassMeta(), Attributes);
}

bool ABaseObject::SetAttribute(AStringView AttributeName, AStringView AttributeValue)
{
    AAttributeMeta const* attrib = FinalClassMeta().FindAttribute(AttributeName, true);
    if (!attrib)
        return false;

    attrib->SetValue(this, AttributeValue);
    return true;
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
