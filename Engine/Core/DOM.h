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

#include "TypeRegistry.h"
#include "SmallString.h"

HK_NAMESPACE_BEGIN

// Document Object Model
// Future optimization: smmalloc or pools can be used for Object/Member allocation.

namespace DOM
{

class Object;
class Member;

namespace Traits
{

template <typename T>
struct Convert
{
    static Object Encode(T const& rhs);
    static bool Decode(Object const& object, T& rhs);
};

} // namespace Traits

class Object final
{
public:
    Object() = default;
    Object(StringView str);
    Object(Object&& rhs) noexcept = default;
    ~Object();

    Object& operator=(Object&& rhs) noexcept = default;

    Object Copy() const;

    void CopyFrom(Object const& source);

    void Clear();

    bool IsStructure() const;

    bool IsArray() const;

    bool IsString() const;

    // Structure

    bool HasMember(StringID memberName) const;

    Member* Find(StringID memberName) const;

    Object& operator[](StringView memberName);

    Object& Insert(StringID memberName);

    Object& Insert(StringID memberName, Object&& object);

    void Remove(StringID memberName);

    using MemberIterator = TVector<Member*>::Iterator;
    using MemberConstIterator = TVector<Member*>::ConstIterator;

    MemberIterator MemberBegin() { return m_Members.begin(); }
    MemberIterator MemberEnd() { return m_Members.end(); }

    MemberConstIterator MemberBegin() const { return m_Members.begin(); }
    MemberConstIterator MemberEnd() const { return m_Members.end(); }

    // Array

    size_t GetArraySize() const
    {
        return m_Array.Size();
    }

    Object& At(int index)
    {
        HK_ASSERT(IsArray());
        return *m_Array[index];
    }

    Object const& At(int index) const
    {
        HK_ASSERT(IsArray());
        return *m_Array[index];
    }

    void PreallocateArray(size_t size)
    {
        m_Array.Reserve(size);
    }

    void Add(StringView string)
    {
        ClearStructure();
        ClearString();

        m_Array.Add(new Object(string));
    }

    void Add(Object&& object)
    {
        ClearStructure();
        ClearString();

        m_Array.Add(new Object(std::forward<Object>(object)));
    }

    Object& EmplaceBack()
    {
        ClearStructure();
        ClearString();

        m_Array.Add(new Object);
        return *m_Array.Last();
    }

    // String

    Object& operator=(StringView str);

    StringView AsString() const { return m_String; }

    const char* AsRawString() const { return m_String.GetRawString(); }

    // Decoding

    template <typename T>
    T As() const
    {
        T v;
        Traits::Convert<T>::Decode(*this, v);
        return v;
    }

    // Encoding

    template <typename T>
    static Object From(T const& rhs)
    {
        return Traits::Convert<T>::Encode(rhs);
    }

private:
    Object(Object const& rhs)
    {
        CopyFrom(rhs);
    }

    void ClearStructure();
    void ClearArray();
    void ClearString();

    // Structure data
    TVector<Member*> m_Members;
    // Array data
    TVector<Object*> m_Array;
    // String data
    SmallString m_String;
};

class Member final
{
public:
    explicit Member(StringID name);
    explicit Member(StringID name, Object&& object);
    Member(Member const&) = delete;
    Member(Member&&) = delete;

    StringID GetName() const
    {
        return m_Name;
    }

    Object& GetObject()
    {
        return m_Object;
    }

    Object const& GetObject() const
    {
        return m_Object;
    }

private:
    StringID m_Name;
    Object m_Object;
};

class ObjectView final
{
public:
    ObjectView() :
        m_ObjectPtr(nullptr)
    {}
    ObjectView(Object const& object) :
        m_ObjectPtr(&object)
    {}

    bool IsStructure() const
    {
        return m_ObjectPtr ? m_ObjectPtr->IsStructure() : false;
    }

    bool IsArray() const
    {
        return m_ObjectPtr ? m_ObjectPtr->IsArray() : false;
    }

    bool IsString() const
    {
        return m_ObjectPtr ? m_ObjectPtr->IsString() : false;
    }

    Object const* GetObjectPtr() const
    {
        return m_ObjectPtr;
    }

    /// Возвращает копию объекта или пустой объект, если оригинал не существует.
    Object ToObject() const
    {
        return m_ObjectPtr ? m_ObjectPtr->Copy() : Object{};
    }

    // Structure

    bool HasMember(StringID memberName) const
    {
        return m_ObjectPtr ? m_ObjectPtr->HasMember(memberName) : false;
    }

    ObjectView operator[](StringView memberName) const
    {
        if (!m_ObjectPtr)
            return {};

        auto* member = m_ObjectPtr->Find(StringID(memberName));
        if (!member)
            return {};

        return member->GetObject();
    }

    // Array

    size_t GetArraySize() const
    {
        return m_ObjectPtr ? m_ObjectPtr->GetArraySize() : 0;
    }

    ObjectView At(int index) const
    {
        return m_ObjectPtr && m_ObjectPtr->IsArray() ? m_ObjectPtr->At(index) : ObjectView{};
    }

    // String

    StringView AsString() const
    {
        return m_ObjectPtr ? m_ObjectPtr->AsString() : StringView{};
    }

    const char* AsRawString() const
    {
        return m_ObjectPtr ? m_ObjectPtr->AsRawString() : "";
    }

    // Decoding

    template <typename T>
    T As() const
    {
        if (m_ObjectPtr)
        {
            T v;
            Traits::Convert<T>::Decode(*m_ObjectPtr, v);
            return v;
        }

        return {};
    }

private:
    Object const* m_ObjectPtr;
};

class MemberIterator
{
public:
    using Iterator = Object::MemberIterator;

private:
    Iterator m_Begin;
    Iterator m_End;

public:
    explicit MemberIterator(Object& object) :
        m_Begin(object.MemberBegin()), m_End(object.MemberEnd())
    {}

    Iterator begin()
    {
        return m_Begin;
    }

    Iterator end()
    {
        return m_End;
    }
};

class MemberConstIterator
{
public:
    using ConstIterator = Object::MemberConstIterator;

private:
    ConstIterator m_Begin;
    ConstIterator m_End;

public:
    explicit MemberConstIterator(Object const& object) :
        m_Begin(object.MemberBegin()), m_End(object.MemberEnd())
    {}

    explicit MemberConstIterator(ObjectView view)
    {
        Object const* objectPtr = view.GetObjectPtr();
        if (objectPtr)
        {
            m_Begin = objectPtr->MemberBegin();
            m_End = objectPtr->MemberEnd();
        }
        else
        {
            m_Begin = m_End = {};
        }
    }

    ConstIterator begin() const
    {
        return m_Begin;
    }

    ConstIterator end() const
    {
        return m_End;
    }
};

namespace Traits
{
#define HK_DOM_TRAITS_CONVERT(type) \
template <> \
struct Convert<type> \
{ \
    static Object Encode(type const& rhs) \
    { \
        return Core::ToString(rhs); \
    } \
    static bool Decode(Object const& object, type& rhs) \
    { \
        rhs = Core::Parse<type>(object.AsString()); \
        return true; \
    } \
};
HK_DOM_TRAITS_CONVERT(bool)
HK_DOM_TRAITS_CONVERT(int8_t)
HK_DOM_TRAITS_CONVERT(int16_t)
HK_DOM_TRAITS_CONVERT(int32_t)
HK_DOM_TRAITS_CONVERT(int64_t)
HK_DOM_TRAITS_CONVERT(uint8_t)
HK_DOM_TRAITS_CONVERT(uint16_t)
HK_DOM_TRAITS_CONVERT(uint32_t)
HK_DOM_TRAITS_CONVERT(uint64_t)
HK_DOM_TRAITS_CONVERT(float)
HK_DOM_TRAITS_CONVERT(double)
}

template <typename Impl>
class Visitor
{
public:
    void Visit(ObjectView dobjectView)
    {
        Object const* objectPtr = dobjectView.GetObjectPtr();
        if (objectPtr)
            Visit(*objectPtr);
    }

    void Visit(Object const& dobject)
    {
        VisitObject({}, dobject, 0);
    }

protected:
    int Stack() const
    {
        return m_Stack;
    }

private:
    Impl& GetImpl()
    {
        return *static_cast<Impl*>(this);
    }

    void VisitObject(StringID name, Object const& dobject, int index)
    {
        if (dobject.IsStructure())
        {
            if (!name.IsEmpty())
                GetImpl().OnBeginStructure(name, dobject);
            else
                GetImpl().OnBeginStructure(dobject, index);

            m_Stack++;
            for (auto member : MemberConstIterator(dobject))
            {
                VisitObject(member->GetName(), member->GetObject(), 0);
            }
            m_Stack--;
            GetImpl().OnEndStructure();
        }
        else if (dobject.IsArray())
        {
            if (!name.IsEmpty())
                GetImpl().OnBeginArray(name, dobject);
            else
                GetImpl().OnBeginArray(dobject, index);

            m_Stack++;
            for (int i = 0, count = dobject.GetArraySize(); i < count; i++)
            {
                VisitObject({}, dobject.At(i), i);
            }
            m_Stack--;
            GetImpl().OnEndArray();
        }
        else
        {
            if (!name.IsEmpty())
                GetImpl().OnVisitString(name, dobject);
            else
                GetImpl().OnVisitString(dobject, index);
        }
    }

    int m_Stack{};
};

class Writer : public Visitor<Writer>
{
public:
    virtual void Write(StringView text);

    void OnBeginStructure(StringID name, Object const& dobject);
    void OnBeginStructure(Object const& dobject, int index);
    void OnEndStructure();
    void OnBeginArray(StringID name, Object const& dobject);
    void OnBeginArray(Object const& dobject, int index);
    void OnEndArray();
    void OnVisitString(StringID name, Object const& dobject);
    void OnVisitString(Object const& dobject, int index);

private:
    void Indent();
};

class WriterCompact : public Visitor<WriterCompact>
{
public:
    virtual void Write(StringView text);

    void OnBeginStructure(StringID name, Object const& dobject);
    void OnBeginStructure(Object const& dobject, int index);
    void OnEndStructure();
    void OnBeginArray(StringID name, Object const& dobject);
    void OnBeginArray(Object const& dobject, int index);
    void OnEndArray();
    void OnVisitString(StringID name, Object const& dobject);
    void OnVisitString(Object const& dobject, int index);
};

template <typename StringBuffer>
class StringWriter : public Writer
{
    StringBuffer& m_Buffer;

public:
    explicit StringWriter(StringBuffer& ref) :
        m_Buffer(ref)
    {
        m_Buffer.Clear();
    }

    void Write(StringView str) override
    {
        m_Buffer += str;
    }
};

template <typename StringBuffer>
class StringWriterCompact : public WriterCompact
{
    StringBuffer& m_Buffer;

public:
    explicit StringWriterCompact(StringBuffer& ref) :
        m_Buffer(ref)
    {
        m_Buffer.Clear();
    }

    void Write(StringView str) override
    {
        m_Buffer += str;
    }
};

Object Serialize(TR::TypeRegistry const& inTypeRegistry, void const* inObjectPtr, TR::TypeInfo const* inTypeInfo);
void Deserialize(Object const& dobject, TR::TypeRegistry const& inTypeRegistry, void* inObjectPtr, TR::TypeInfo const* inTypeInfo);

template <typename T>
HK_FORCEINLINE Object Serialize(T const& inObject, TR::TypeRegistry const& inTypeRegistry)
{
    auto type_id = TR::TypeID<T>::GetID();
    auto type_info = inTypeRegistry.FindType(type_id);

    return Serialize(inTypeRegistry, &inObject, type_info);
}

template <typename T>
HK_FORCEINLINE T Deserialize(Object const& dobject, TR::TypeRegistry const& inTypeRegistry)
{
    T object;

    auto type_id = TR::TypeID<T>::GetID();
    auto type_info = inTypeRegistry.FindType(type_id);

    Deserialize(dobject, inTypeRegistry, &object, type_info);

    return object;
}

template <typename T>
HK_FORCEINLINE T Deserialize(ObjectView dobjectView, TR::TypeRegistry const& inTypeRegistry)
{
    DOM::Object const* dobject = dobjectView.GetObjectPtr();
    if (!dobject)
        return {};
    return Deserialize<T>(*dobject, inTypeRegistry);
}

class Parser
{
public:
    Object Parse(const char* inStr);
    Object Parse(String const& inStr);

private:
    enum TOKEN_TYPE : uint8_t
    {
        TOKEN_UNKNOWN,
        TOKEN_EOF,
        TOKEN_BRACKET,
        TOKEN_MEMBER,
        TOKEN_STRING
    };

    struct Token
    {
        const char* Begin = "";
        const char* End = "";
        TOKEN_TYPE Type = TOKEN_UNKNOWN;
    };

    class Tokenizer
    {
    public:
        Tokenizer();

        void Reset(const char* pDocumentData);
        void NextToken();
        Token GetToken() const { return m_Token; }

    private:
        void SkipWhitespaces();

        const char* m_Cur;
        //int m_LineNumber;
        Token m_Token;
    };

    Object ParseStructure(bool bExpectClosedBracket);
    Object ParseArray();

    Tokenizer m_Tokenizer;
};

} // namespace DOM

HK_NAMESPACE_END
