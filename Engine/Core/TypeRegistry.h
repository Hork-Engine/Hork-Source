/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "StringId.h"
#include "Parse.h"
#include "Ref.h"

HK_NAMESPACE_BEGIN

namespace TR {

struct MakeID
{
    static uint32_t Generate()
    {
        static uint32_t gen = 0;
        return ++gen;
    }
};

template <typename T>
struct TypeID
{
    static uint32_t GetID()
    {
        return ID;
    }

private:
    static uint32_t ID;
};

template <typename T>
uint32_t TypeID<T>::ID = MakeID::Generate();

class StructureBase;

struct TypeInfo
{
    TUniqueRef<StructureBase> Struct;
    uint32_t ArrayElementTypeId;
    StringId Name; // just for debug

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
    BasePointerDefinition(uint32_t inTypeId, StringView inName) :
        m_TypeId(inTypeId), m_Name(inName)
    {}
    virtual ~BasePointerDefinition() {}

    virtual void *DereferencePtr(void* inObjectPtr) = 0;

    uint32_t GetTypeId() const { return m_TypeId; }
    StringId GetName() const { return m_Name; }

private:
    uint32_t m_TypeId;
    StringId m_Name;
};

template<typename Object, typename MemberType>
class MemberObjectDefinition final : public BasePointerDefinition {
public:
    MemberObjectDefinition(StringView inName, MemberType Object::* inMemberPtr)
        : BasePointerDefinition(TypeID<MemberType>::GetID(), inName), m_MemberPtr(inMemberPtr) {}

protected:
    void* DereferencePtr(void* inObjectPtr) override
    {
        return &(static_cast<Object*>(inObjectPtr)->*m_MemberPtr);
    }

private:
    MemberType Object::* m_MemberPtr;
};

class StructureBase
{
public:
    virtual ~StructureBase() {}

    TVector<TUniqueRef<BasePointerDefinition>> const& GetMembers() const { return m_Members; }

protected:
    TVector<TUniqueRef<BasePointerDefinition>> m_Members;
};

template <typename Object>
class Structure : public StructureBase
{
public:
    template <typename MemberType>
    void RegisterMember(StringView inName, MemberType Object::* inMemberObjectPtr)
    {
        TUniqueRef<MemberObjectDefinition<Object, MemberType>> member =
            MakeUnique<MemberObjectDefinition<Object, MemberType>>(inName, inMemberObjectPtr);

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

template <typename T, typename = int>
struct HasResize : std::false_type { };

template <typename T>
struct HasResize <T, decltype(&T::Resize, 0)> : std::true_type { };

template<typename T>
typename std::enable_if<HasResize<T>::value>::type
ArrayResize(T& container, size_t s)
{
    container.Resize(s);
}

template<typename T>
typename std::enable_if<!HasResize<T>::value>::type
ArrayResize(T&, size_t)
{
}

}

class TypeRegistry
{
public:
    template <typename T>
    void RegisterType(StringView inName);

    template <typename T>
    Structure<T>* RegisterStruct(StringView inName);

    template <typename ArrayType>
    void RegisterArray(StringView inName);

    template <typename T>
    StringId FindType() const;

    TypeInfo const* FindType(uint32_t inTypeId) const;

private:
    THashMap<uint32_t, TypeInfo> m_TypeInfos;
};

template <typename T>
HK_INLINE void TypeRegistry::RegisterType(StringView inName)
{
    uint32_t id = TypeID<T>::GetID();

    TypeInfo& type_info = m_TypeInfos[id];
    type_info.Name = StringId(inName);

    type_info.Value.ToString = [](void const* inObjectPtr)
    {
        return Traits::ToString(*static_cast<T const*>(inObjectPtr));
    };
    type_info.Value.FromString = [](void* inObjectPtr, StringView inStr)
    {
        *static_cast<T*>(inObjectPtr) = Traits::FromString<T>(inStr);
    };
}

template <typename T>
HK_INLINE Structure<T>* TypeRegistry::RegisterStruct(StringView inName)
{
    TUniqueRef<Structure<T>> structure = MakeUnique<Structure<T>>();

    auto ptr = structure.GetObject();

    uint32_t id = TypeID<T>::GetID();

    TypeInfo& type_info = m_TypeInfos[id];
    type_info.Name = StringId(inName);
    type_info.Struct = std::move(structure);

    type_info.Value.ToString = [](void const*) -> String
    {
        return {};
    };
    type_info.Value.FromString = [](void*, StringView)
    {
    };

    return ptr;
}

template <typename ArrayType>
HK_INLINE void TypeRegistry::RegisterArray(StringView inName)
{
    using T = typename ArrayType::value_type;

    uint32_t id = TypeID<ArrayType>::GetID();

    TypeInfo& type_info = m_TypeInfos[id];
    type_info.Name = StringId(inName);
    type_info.ArrayElementTypeId = TypeID<T>::GetID();
    type_info.Array.GetArraySize = [](void const* inObjectPtr)
    {
        return static_cast<ArrayType const*>(inObjectPtr)->Size();
    };
    type_info.Array.GetArrayAt = [](int inIndex, void* inObjectPtr) -> void*
    {
        T& element = (*static_cast<ArrayType*>(inObjectPtr))[inIndex];
        return &element;
    };
    type_info.Array.TryResize = [](size_t inSize, void* inObjectPtr) -> bool
    {
        auto& arr = *static_cast<ArrayType*>(inObjectPtr);
        Internal::ArrayResize(arr, inSize);
        return arr.Size() == inSize;
    };
}

template <typename T>
HK_INLINE StringId TypeRegistry::FindType() const
{
    TypeInfo* type_info = FindType(TypeID<T>::GetID());
    if (type_info)
        return type_info->Name;
    return {};
}

HK_INLINE TypeInfo const* TypeRegistry::FindType(uint32_t inTypeId) const
{
    auto it = m_TypeInfos.Find(inTypeId);
    if (it == m_TypeInfos.End())
        return nullptr;
    return &it->second;
}

}

using TypeRegistry = TR::TypeRegistry;

HK_NAMESPACE_END
