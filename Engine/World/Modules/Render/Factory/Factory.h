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

#include <Engine/Core/Containers/Hash.h>
#include "Variant.h"

HK_NAMESPACE_BEGIN

class ClassMeta;
class Property;
class BaseObject;

class ObjectFactory final : public Noncopyable
{
    friend class ClassMeta;

public:
    ObjectFactory(const char* Tag);
    ~ObjectFactory() = default;

    const char* GetTag() const { return m_Tag; }

    BaseObject* CreateInstance(StringView ClassName) const;
    BaseObject* CreateInstance(uint64_t ClassId) const;

    ClassMeta const* GetClassList() const;

    ClassMeta const* FindClass(StringView ClassName) const;

    ClassMeta const* LookupClass(StringView ClassName) const;
    ClassMeta const* LookupClass(uint64_t ClassId) const;

    uint64_t FactoryClassCount() const { return m_NumClasses; }

    static ObjectFactory const* Factories() { return m_FactoryList; }
    ObjectFactory const* Next() const { return m_NextFactory; }

private:
    const char*           m_Tag;
    ClassMeta*            m_Classes;
    mutable Vector<ClassMeta*> m_IdTable;
    mutable HashMap<StringView, ClassMeta const*> m_LookupTable;
    uint64_t              m_NumClasses;
    ObjectFactory*        m_NextFactory;
    static ObjectFactory* m_FactoryList;
};

using PropertyList = SmallVector<Property const*, 32>;

class ClassMeta : public Noncopyable
{
    friend class ObjectFactory;
    friend class Property;

public:
    const uint64_t ClassId;

    const char*             GetName() const { return m_ClassName.CStr(); }
    GlobalStringView const& GetName2() const { return m_ClassName; }
    uint64_t                GetId() const { return ClassId; }
    ClassMeta const*        SuperClass() const { return m_pSuperClass; }
    ClassMeta const*        Next() const { return m_pNext; }
    ObjectFactory const*    Factory() const { return m_pFactory; }
    Property const*         GetPropertyList() const { return m_PropertyList; }

    bool IsSubclassOf(ClassMeta const& Superclass) const
    {
        for (ClassMeta const* meta = this; meta; meta = meta->SuperClass())
        {
            if (meta->GetId() == Superclass.GetId())
            {
                return true;
            }
        }
        return false;
    }

    template <typename Superclass>
    bool IsSubclassOf() const
    {
        return IsSubclassOf(Superclass::GetClassMeta());
    }

    virtual BaseObject* CreateInstance() const = 0;

    static void CloneProperties(BaseObject const* Template, BaseObject* Destination);

    static ObjectFactory& DummyFactory()
    {
        static ObjectFactory ObjectFactory("Dummy factory");
        return ObjectFactory;
    }

    // Utilites
    Property const* FindProperty(StringView PropertyName, bool bRecursive) const;
    void            GetProperties(PropertyList& Properties, bool bRecursive = true) const;

protected:
    ClassMeta(ObjectFactory& Factory, GlobalStringView ClassName, ClassMeta const* SuperClassMeta) :
        ClassId(Factory.m_NumClasses + 1), m_ClassName(ClassName)
    {
        HK_ASSERT_(Factory.FindClass(m_ClassName) == NULL, "Class already defined");
        m_pNext            = Factory.m_Classes;
        m_pSuperClass      = SuperClassMeta;
        m_PropertyList     = nullptr;
        m_PropertyListTail = nullptr;
        Factory.m_Classes = this;
        Factory.m_NumClasses++;
        m_pFactory = &Factory;
    }

private:
    static void CloneProperties_r(ClassMeta const* Meta, BaseObject const* Template, BaseObject* Destination);

    GlobalStringView     m_ClassName;
    ClassMeta*           m_pNext;
    ClassMeta const*     m_pSuperClass;
    ObjectFactory const* m_pFactory;
    Property const*      m_PropertyList;
    Property const*      m_PropertyListTail;
};

HK_FORCEINLINE BaseObject* ObjectFactory::CreateInstance(StringView ClassName) const
{
    ClassMeta const* classMeta = LookupClass(ClassName);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

HK_FORCEINLINE BaseObject* ObjectFactory::CreateInstance(uint64_t ClassId) const
{
    ClassMeta const* classMeta = LookupClass(ClassId);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

HK_FORCEINLINE ClassMeta const* ObjectFactory::GetClassList() const
{
    return m_Classes;
}

struct PropertyRange
{
    int64_t MinIntegral = 0;
    int64_t MaxIntegral = 0;
    double  MinFloat    = 0;
    double  MaxFloat    = 0;

    bool IsUnbound() const
    {
        return MinIntegral == MaxIntegral && MinFloat == MaxFloat;
    }
};

constexpr PropertyRange RangeUnbound()
{
    return PropertyRange();
}

constexpr PropertyRange RangeInt(int64_t MinIntegral, int64_t MaxIntegral)
{
    return {MinIntegral, MaxIntegral, static_cast<double>(MinIntegral), static_cast<double>(MaxIntegral)};
}

constexpr PropertyRange RangeFloat(double MinFloat, double MaxFloat)
{
    return {static_cast<int64_t>(MinFloat), static_cast<int64_t>(MaxFloat), MinFloat, MaxFloat};
}

/*

Property flags

*/
enum HK_PROPERTY_FLAGS : uint32_t
{
    HK_PROPERTY_DEFAULT          = 0,
    HK_PROPERTY_NON_SERIALIZABLE = HK_BIT(0),
    HK_PROPERTY_BITMASK          = HK_BIT(1)
};

HK_FLAG_ENUM_OPERATORS(HK_PROPERTY_FLAGS)


class Property final : public Noncopyable
{
public:
    using SetterFun = void (*)(BaseObject*, Variant const&);
    using GetterFun = Variant (*)(BaseObject const*);
    using CopyFun   = void (*)(BaseObject*, BaseObject const*);

    Property(ClassMeta const& _ClassMeta, VARIANT_TYPE Type, EnumDef const* EnumDef, GlobalStringView Name, SetterFun Setter, GetterFun Getter, CopyFun Copy, PropertyRange const& Range, HK_PROPERTY_FLAGS Flags) :
        m_Type(Type),
        m_Name(Name),
        m_pEnum(EnumDef),
        m_Range(Range),
        m_Flags(Flags),
        m_Setter(Setter),
        m_Getter(Getter),
        m_Copy(Copy)
    {
        ClassMeta& classMeta = const_cast<ClassMeta&>(_ClassMeta);
        m_pNext = nullptr;
        m_pPrev = classMeta.m_PropertyListTail;
        if (m_pPrev)
        {
            const_cast<Property*>(m_pPrev)->m_pNext = this;
        }
        else
        {
            classMeta.m_PropertyList = this;
        }
        classMeta.m_PropertyListTail = this;
    }

    void SetValue(BaseObject* pObject, Variant const& Value) const
    {
        m_Setter(pObject, Value);
    }

    void SetValue(BaseObject* pObject, StringView Value) const
    {
        SetValue(pObject, Variant(GetType(), GetEnum(), Value));
    }

    Variant GetValue(BaseObject const* pObject) const
    {
        return m_Getter(pObject);
    }

    void CopyValue(BaseObject* Dst, BaseObject const* Src) const
    {
        return m_Copy(Dst, Src);
    }

    VARIANT_TYPE GetType() const { return m_Type; }
    const char* GetName() const { return m_Name.CStr(); }
    GlobalStringView const& GetName2() const { return m_Name; }    
    EnumDef const* GetEnum() const { return m_pEnum; }
    PropertyRange const& GetRange() const { return m_Range; }
    HK_PROPERTY_FLAGS GetFlags() const { return m_Flags; }
    Property const*      Next() const { return m_pNext; }
    Property const* Prev() const { return m_pPrev; }

private:
    VARIANT_TYPE      m_Type;
    GlobalStringView  m_Name;
    EnumDef const*    m_pEnum;
    PropertyRange     m_Range;
    HK_PROPERTY_FLAGS m_Flags;
    SetterFun         m_Setter;
    GetterFun         m_Getter;
    CopyFun           m_Copy;
    Property const*   m_pNext;
    Property const*   m_pPrev;
};

template <std::size_t N, typename T0, typename... Ts>
struct _ArgumentTypeN
{
    using Type = typename _ArgumentTypeN<N - 1U, Ts...>::Type;
};

template <typename T0, typename... Ts>
struct _ArgumentTypeN<0U, T0, Ts...>
{
    using Type = T0;
};

template <std::size_t, typename>
struct TArgumentType;

template <std::size_t N, typename Class, typename R, typename... As>
struct TArgumentType<N, R (Class::*)(As...)>
{
    using Type = typename _ArgumentTypeN<N, As...>::Type;
};

template <typename>
struct TReturnType;

template <class Class, class R, class... As>
struct TReturnType<R (Class::*)(As...) const>
{
    using Type = R;
};

template <class Class, class R, class... As>
struct TReturnType<R (Class::*)(As...)>
{
    using Type = R;
};

#define _HK_GENERATED_CLASS_BODY()                      \
public:                                                 \
    static ThisClassMeta const& GetClassMeta()          \
    {                                                   \
        static const ThisClassMeta __Meta;              \
        return __Meta;                                  \
    }                                                   \
    static Hk::ClassMeta const* SuperClass()            \
    {                                                   \
        return GetClassMeta().SuperClass();             \
    }                                                   \
    static const char* ClassName()                      \
    {                                                   \
        return GetClassMeta().GetName();                \
    }                                                   \
    static uint64_t ClassId()                           \
    {                                                   \
        return GetClassMeta().GetId();                  \
    }                                                   \
    virtual Hk::ClassMeta const& FinalClassMeta() const \
    {                                                   \
        return GetClassMeta();                          \
    }                                                   \
    virtual const char* FinalClassName() const          \
    {                                                   \
        return ClassName();                             \
    }                                                   \
    virtual uint64_t FinalClassId() const               \
    {                                                   \
        return ClassId();                               \
    }                                                   \
    void* operator new(size_t SizeInBytes)              \
    {                                                   \
        return Allocator().allocate(SizeInBytes);       \
    }                                                   \
    void operator delete(void* Ptr)                     \
    {                                                   \
        Allocator().deallocate(Ptr);                    \
    }

#define HK_CLASS(Class, SuperClass) \
    HK_FACTORY_CLASS(Hk::ClassMeta::DummyFactory(), Class, SuperClass)

#define HK_FACTORY_CLASS(Factory, Class, SuperClass) \
    HK_FACTORY_CLASS_A(Factory, Class, SuperClass, BaseObject::Allocator)

#define HK_FACTORY_CLASS_A(Factory, Class, SuperClass, _Allocator)                                          \
    Class(Class const&) = delete;                                                                           \
    Class& operator=(Class const&) = delete;                                                                \
    friend class BaseObject;                                                                                \
                                                                                                            \
public:                                                                                                     \
    typedef SuperClass Super;                                                                               \
    typedef Class ThisClass;                                                                                \
    typedef _Allocator Allocator;                                                                           \
    class ThisClassMeta : public Hk::ClassMeta                                                              \
    {                                                                                                       \
    public:                                                                                                 \
        ThisClassMeta() : Hk::ClassMeta(Factory, HK_CONCAT(HK_STRINGIFY(Class), s), &Super::GetClassMeta()) \
        {                                                                                                   \
            RegisterProperties();                                                                           \
        }                                                                                                   \
        BaseObject* CreateInstance() const override                                                         \
        {                                                                                                   \
            return new ThisClass;                                                                           \
        }                                                                                                   \
                                                                                                            \
    private:                                                                                                \
        void RegisterProperties();                                                                          \
    };                                                                                                      \
    _HK_GENERATED_CLASS_BODY()                                                                              \
private:



#define HK_BEGIN_CLASS_META(Class)                              \
    Hk::ClassMeta const& Class##__Meta = Class::GetClassMeta(); \
    void Class::ThisClassMeta::RegisterProperties()             \
    {

#define HK_END_CLASS_META() \
    }

#define HK_CLASS_META(Class)   \
    HK_BEGIN_CLASS_META(Class) \
    HK_END_CLASS_META()

/** Provides direct access to a class member. */
#define HK_PROPERTY_DIRECT_RANGE(TheProperty, Flags, Range)                                                  \
    {                                                                                                        \
        using PropertyType = decltype(ThisClass::TheProperty);                                               \
        static Property const decl_##TheProperty(                                                            \
            *this,                                                                                           \
            VariantTraits::GetVariantType<PropertyType>(),                                                   \
            VariantTraits::GetVariantEnum<PropertyType>(),                                                   \
            #TheProperty##s,                                                                                 \
            [](BaseObject* pObject, Variant const& Value) {                                                  \
                auto* pValue = Value.Get<PropertyType>();                                                    \
                if (pValue)                                                                                  \
                {                                                                                            \
                    static_cast<ThisClass*>(pObject)->TheProperty = *pValue;                                 \
                }                                                                                            \
            },                                                                                               \
            [](BaseObject const* pObject) -> Variant {                                                       \
                return static_cast<ThisClass const*>(pObject)->TheProperty;                                  \
            },                                                                                               \
            [](BaseObject* Dst, BaseObject const* Src) {                                                     \
                static_cast<ThisClass*>(Dst)->TheProperty = static_cast<ThisClass const*>(Src)->TheProperty; \
            },                                                                                               \
            Range,                                                                                           \
            Flags);                                                                                          \
    }

/** Provides direct access to a class member. */
#define HK_PROPERTY_DIRECT(TheProperty, Flags) HK_PROPERTY_DIRECT_RANGE(TheProperty, Flags, RangeUnbound())

template <class T>
struct RemoveCVRef
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

/** Provides access to a property via a setter/getter. */
#define HK_PROPERTY_RANGE(TheProperty, Setter, Getter, Flags, Range)                                            \
    {                                                                                                           \
        using PropertyType = RemoveCVRef<typename TArgumentType<0U, decltype(&ThisClass::Setter)>::Type>::type; \
        using GetterReturnType = RemoveCVRef<TReturnType<decltype(&ThisClass::Getter)>::Type>::type;            \
        static_assert(std::is_same<PropertyType, GetterReturnType>::value, "Setter and getter type mismatch");  \
        static Property const decl_##TheProperty(                                                               \
            *this,                                                                                              \
            VariantTraits::GetVariantType<PropertyType>(),                                                      \
            VariantTraits::GetVariantEnum<PropertyType>(),                                                      \
            #TheProperty##s,                                                                                    \
            [](BaseObject* pObject, Variant const& Value) {                                                     \
                auto* pValue = Value.Get<PropertyType>();                                                       \
                if (pValue)                                                                                     \
                {                                                                                               \
                    static_cast<ThisClass*>(pObject)->Setter(*pValue);                                          \
                }                                                                                               \
            },                                                                                                  \
            [](BaseObject const* pObject) -> Variant {                                                          \
                return static_cast<ThisClass const*>(pObject)->Getter();                                        \
            },                                                                                                  \
            [](BaseObject* Dst, BaseObject const* Src) {                                                        \
                static_cast<ThisClass*>(Dst)->Setter(static_cast<ThisClass const*>(Src)->Getter());             \
            },                                                                                                  \
            Range,                                                                                              \
            Flags);                                                                                             \
    }

/** Provides access to a property via a setter/getter. */
#define HK_PROPERTY(TheProperty, Setter, Getter, Flags) HK_PROPERTY_RANGE(TheProperty, Setter, Getter, Flags, RangeUnbound())

HK_NAMESPACE_END
