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

#include "StringID.h"
#include "Parse.h"
#include "Ref.h"
#include "Containers/Vector.h"

HK_NAMESPACE_BEGIN

namespace TR {

struct MakeID
{
    static uint32_t sGenerate()
    {
        static uint32_t gen = 0;
        return ++gen;
    }
};

template <typename T>
struct TypeID
{
    static uint32_t sGetID()
    {
        return ID;
    }

private:
    static uint32_t ID;
};

template <typename T>
uint32_t TypeID<T>::ID = MakeID::sGenerate();

class StructureBase;

struct TypeInfo
{
    UniqueRef<StructureBase> Struct;
    uint32_t ArrayElementTypeId;
    StringID Name; // just for debug

    union
    {
        struct
        {
            size_t (*GetArraySize)(void const*);
            void* (*GetArrayAt)(int, void*);
            bool (*TryResize)(size_t, void*);
        } Array;

        struct
        {
            String (*ToString)(void const*);
            void (*FromString)(void*, StringView);
        } Value;
    };
};

class BasePointerDefinition
{
public:
    BasePointerDefinition(uint32_t typeId, StringView name) :
        m_TypeId(typeId), m_Name(name)
    {}
    virtual ~BasePointerDefinition() {}

    virtual void *DereferencePtr(void* objectPtr) = 0;

    uint32_t GetTypeId() const { return m_TypeId; }
    StringID GetName() const { return m_Name; }

private:
    uint32_t m_TypeId;
    StringID m_Name;
};

template<typename Object, typename MemberType>
class MemberObjectDefinition final : public BasePointerDefinition {
public:
    MemberObjectDefinition(StringView name, MemberType Object::* memberPtr)
        : BasePointerDefinition(TypeID<MemberType>::GetID(), name), m_MemberPtr(memberPtr) {}

protected:
    void* DereferencePtr(void* objectPtr) override
    {
        return &(static_cast<Object*>(objectPtr)->*m_MemberPtr);
    }

private:
    MemberType Object::* m_MemberPtr;
};

class StructureBase
{
public:
    virtual ~StructureBase() {}

    Vector<UniqueRef<BasePointerDefinition>> const& GetMembers() const { return m_Members; }

protected:
    Vector<UniqueRef<BasePointerDefinition>> m_Members;
};

template <typename Object>
class Structure : public StructureBase
{
public:
    template <typename MemberType>
    void RegisterMember(StringView name, MemberType Object::* memberObjectPtr)
    {
        UniqueRef<MemberObjectDefinition<Object, MemberType>> member =
            MakeUnique<MemberObjectDefinition<Object, MemberType>>(name, memberObjectPtr);

        m_Members.Add(std::move(member));
    }
};

namespace Traits
{

template <typename T>
HK_INLINE String ToString(T const& v)
{
    return Core::ToString(v);
}

template <typename T>
HK_INLINE T FromString(StringView s)
{
    return Core::Parse<T>(s);
}

}

namespace Internal
{

HK_FIND_METHOD(Resize)

template<typename T>
typename std::enable_if<HK_HAS_METHOD(T, Resize)>::type
ArrayResize(T& container, size_t s)
{
    container.Resize(s);
}

template<typename T>
typename std::enable_if<!HK_HAS_METHOD(T, Resize)>::type
ArrayResize(T&, size_t)
{
}

}

class TypeRegistry
{
public:
    template <typename T>
    void RegisterType(StringView name);

    template <typename T>
    Structure<T>* RegisterStruct(StringView name);

    template <typename ArrayType>
    void RegisterArray(StringView name);

    template <typename T>
    StringID FindType() const;

    TypeInfo const* FindType(uint32_t typeId) const;

private:
    HashMap<uint32_t, TypeInfo> m_TypeInfos;
};

template <typename T>
HK_INLINE void TypeRegistry::RegisterType(StringView name)
{
    uint32_t id = TypeID<T>::GetID();

    TypeInfo& typeInfo = m_TypeInfos[id];
    typeInfo.Name = StringID(name);

    typeInfo.Value.ToString = [](void const* objectPtr)
    {
        return Traits::ToString(*static_cast<T const*>(objectPtr));
    };
    typeInfo.Value.FromString = [](void* objectPtr, StringView inStr)
    {
        *static_cast<T*>(objectPtr) = Traits::FromString<T>(inStr);
    };
}

template <typename T>
HK_INLINE Structure<T>* TypeRegistry::RegisterStruct(StringView name)
{
    UniqueRef<Structure<T>> structure = MakeUnique<Structure<T>>();

    auto ptr = structure.RawPtr();

    uint32_t id = TypeID<T>::GetID();

    TypeInfo& typeInfo = m_TypeInfos[id];
    typeInfo.Name = StringID(name);
    typeInfo.Struct = std::move(structure);

    typeInfo.Value.ToString = [](void const*) -> String
    {
        return {};
    };
    typeInfo.Value.FromString = [](void*, StringView)
    {
    };

    return ptr;
}

template <typename ArrayType>
HK_INLINE void TypeRegistry::RegisterArray(StringView name)
{
    using T = typename ArrayType::value_type;

    uint32_t id = TypeID<ArrayType>::GetID();

    TypeInfo& typeInfo = m_TypeInfos[id];
    typeInfo.Name = StringID(name);
    typeInfo.ArrayElementTypeId = TypeID<T>::GetID();
    typeInfo.Array.GetArraySize = [](void const* objectPtr)
    {
        return static_cast<ArrayType const*>(objectPtr)->Size();
    };
    typeInfo.Array.GetArrayAt = [](int index, void* objectPtr) -> void*
    {
        T& element = (*static_cast<ArrayType*>(objectPtr))[index];
        return &element;
    };
    typeInfo.Array.TryResize = [](size_t size, void* objectPtr) -> bool
    {
        auto& arr = *static_cast<ArrayType*>(objectPtr);
        Internal::ArrayResize(arr, size);
        return arr.Size() == size;
    };
}

template <typename T>
HK_INLINE StringID TypeRegistry::FindType() const
{
    TypeInfo* typeInfo = FindType(TypeID<T>::GetID());
    if (typeInfo)
        return typeInfo->Name;
    return {};
}

HK_INLINE TypeInfo const* TypeRegistry::FindType(uint32_t typeId) const
{
    auto it = m_TypeInfos.Find(typeId);
    if (it == m_TypeInfos.End())
        return nullptr;
    return &it->second;
}

}

using TypeRegistry = TR::TypeRegistry;

HK_NAMESPACE_END
