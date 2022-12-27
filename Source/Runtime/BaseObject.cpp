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

HK_NAMESPACE_BEGIN

uint64_t BaseObject::m_TotalObjects = 0;

BaseObject* BaseObject::m_Objects   = nullptr;
BaseObject* BaseObject::m_ObjectsTail = nullptr;

static uint64_t GUniqueIdGenerator = 0;

BaseObject::BaseObject() :
    Id(++GUniqueIdGenerator)
{
    INTRUSIVE_ADD(this, m_NextObject, m_PrevObject, m_Objects, m_ObjectsTail);
    m_TotalObjects++;    
}

BaseObject::~BaseObject()
{
    INTRUSIVE_REMOVE(this, m_NextObject, m_PrevObject, m_Objects, m_ObjectsTail);
    m_TotalObjects--;    
}

BaseObject* BaseObject::FindObject(uint64_t _Id)
{
    if (!_Id)
    {
        return nullptr;
    }

    // FIXME: use hash/map?
    for (BaseObject* object = m_Objects; object; object = object->m_NextObject)
    {
        if (object->Id == _Id)
        {
            return object;
        }
    }
    return nullptr;
}

void BaseObject::SetProperties_r(ClassMeta const* Meta, TStringHashMap<String> const& Properties)
{
    if (Meta)
    {
        SetProperties_r(Meta->SuperClass(), Properties);

        for (Property const* prop = Meta->GetPropertyList(); prop; prop = prop->Next())
        {
            auto it = Properties.Find(prop->GetName());
            if (it != Properties.End())
            {
                // Property found
                prop->SetValue(this, Variant(prop->GetType(), prop->GetEnum(), it->second));
            }
        }
    }
}

void BaseObject::SetProperties(TStringHashMap<String> const& Properties)
{
    if (Properties.IsEmpty())
    {
        return;
    }
    SetProperties_r(&FinalClassMeta(), Properties);
}

bool BaseObject::SetProperty(StringView PropertyName, StringView PropertyValue)
{
    Property const* prop = FinalClassMeta().FindProperty(PropertyName, true);
    if (!prop)
        return false;

    prop->SetValue(this, PropertyValue);
    return true;
}

#if 0
bool BaseObject::SetProperty(StringView PropertyName, Variant const& PropertyValue)
{
    Property const* prop = FinalClassMeta().FindProperty(PropertyName, true);
    if (!prop)
        return false;

    prop->SetValue(this, PropertyValue);
    return true;
}
#endif

Property const* BaseObject::FindProperty(StringView PropertyName, bool bRecursive) const
{
    return FinalClassMeta().FindProperty(PropertyName, bRecursive);
}

void BaseObject::GetProperties(TPodVector<Property const*>& Properties, bool bRecursive) const
{
    FinalClassMeta().GetProperties(Properties, bRecursive);
}

HK_NAMESPACE_END
