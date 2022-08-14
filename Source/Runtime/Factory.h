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

#pragma once

#include <Platform/Memory/Memory.h>
#include <Core/HashFunc.h>
#include <Core/Ref.h>
#include <Containers/Hash.h>
#include <Containers/Vector.h>
#include "Variant.h"

class AClassMeta;
class AProperty;
class ADummy;

class AObjectFactory final
{
    HK_FORBID_COPY(AObjectFactory)

    friend class AClassMeta;

public:
    AObjectFactory(const char* Tag);
    ~AObjectFactory() = default;

    const char* GetTag() const { return Tag; }

    ADummy* CreateInstance(AStringView ClassName) const;
    ADummy* CreateInstance(uint64_t ClassId) const;

    AClassMeta const* GetClassList() const;

    AClassMeta const* FindClass(AStringView ClassName) const;

    AClassMeta const* LookupClass(AStringView ClassName) const;
    AClassMeta const* LookupClass(uint64_t ClassId) const;

    uint64_t FactoryClassCount() const { return NumClasses; }

    static AObjectFactory const* Factories() { return FactoryList; }
    AObjectFactory const*        Next() const { return NextFactory; }

private:
    const char*            Tag;
    AClassMeta*            Classes;
    mutable TVector<AClassMeta*> IdTable;
    mutable THashMap<AStringView, AClassMeta const*> LookupTable;
    uint64_t               NumClasses;
    AObjectFactory*        NextFactory;
    static AObjectFactory* FactoryList;
};

using APropertyList = TSmallVector<AProperty const*, 32>;

class AClassMeta
{
    HK_FORBID_COPY(AClassMeta)

    friend class AObjectFactory;
    friend class AProperty;

public:
    const uint64_t ClassId;

    const char*           GetName() const { return ClassName; }
    uint64_t              GetId() const { return ClassId; }
    AClassMeta const*     SuperClass() const { return pSuperClass; }
    AClassMeta const*     Next() const { return pNext; }
    AObjectFactory const* Factory() const { return pFactory; }
    AProperty const*      GetPropertyList() const { return PropertyList; }

    bool IsSubclassOf(AClassMeta const& Superclass) const
    {
        for (AClassMeta const* meta = this; meta; meta = meta->SuperClass())
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
        return IsSubclassOf(Superclass::ClassMeta());
    }

    virtual ADummy* CreateInstance() const = 0;

    static void CloneProperties(ADummy const* Template, ADummy* Destination);

    static AObjectFactory& DummyFactory()
    {
        static AObjectFactory ObjectFactory("Dummy factory");
        return ObjectFactory;
    }

    // Utilites
    AProperty const* FindProperty(AStringView PropertyName, bool bRecursive) const;
    void             GetProperties(APropertyList& Properties, bool bRecursive = true) const;

protected:
    AClassMeta(AObjectFactory& Factory, const char* ClassName, AClassMeta const* SuperClassMeta) :
        ClassId(Factory.NumClasses + 1), ClassName(ClassName)
    {
        HK_ASSERT_(Factory.FindClass(ClassName) == NULL, "Class already defined");
        pNext            = Factory.Classes;
        pSuperClass      = SuperClassMeta;
        PropertyList     = nullptr;
        PropertyListTail = nullptr;
        Factory.Classes  = this;
        Factory.NumClasses++;
        pFactory = &Factory;
    }

private:
    static void CloneProperties_r(AClassMeta const* Meta, ADummy const* Template, ADummy* Destination);

    const char*           ClassName;
    AClassMeta*           pNext;
    AClassMeta const*     pSuperClass;
    AObjectFactory const* pFactory;
    AProperty const*      PropertyList;
    AProperty const*      PropertyListTail;
};

HK_FORCEINLINE ADummy* AObjectFactory::CreateInstance(AStringView ClassName) const
{
    AClassMeta const* classMeta = LookupClass(ClassName);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

HK_FORCEINLINE ADummy* AObjectFactory::CreateInstance(uint64_t ClassId) const
{
    AClassMeta const* classMeta = LookupClass(ClassId);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

HK_FORCEINLINE AClassMeta const* AObjectFactory::GetClassList() const
{
    return Classes;
}

struct SPropertyRange
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

constexpr SPropertyRange RangeUnbound()
{
    return SPropertyRange();
}

constexpr SPropertyRange RangeInt(int64_t MinIntegral, int64_t MaxIntegral)
{
    return {MinIntegral, MaxIntegral, static_cast<double>(MinIntegral), static_cast<double>(MaxIntegral)};
}

constexpr SPropertyRange RangeFloat(double MinFloat, double MaxFloat)
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


class AProperty
{
    HK_FORBID_COPY(AProperty)

public:
    using SetterFun = void (*)(ADummy*, AVariant const&);
    using GetterFun = AVariant (*)(ADummy const*);
    using CopyFun   = void (*)(ADummy*, ADummy const*);

    AProperty(AClassMeta const& ClassMeta, VARIANT_TYPE Type, SEnumDef const* EnumDef, const char* Name, SetterFun Setter, GetterFun Getter, CopyFun Copy, SPropertyRange const& Range, HK_PROPERTY_FLAGS Flags) :
        Type(Type),
        Name(Name),
        pEnum(EnumDef),
        Range(Range),
        Flags(Flags),
        Setter(Setter),
        Getter(Getter),
        Copy(Copy)
    {
        AClassMeta& classMeta = const_cast<AClassMeta&>(ClassMeta);
        pNext                 = nullptr;
        pPrev                 = classMeta.PropertyListTail;
        if (pPrev)
        {
            const_cast<AProperty*>(pPrev)->pNext = this;
        }
        else
        {
            classMeta.PropertyList = this;
        }
        classMeta.PropertyListTail = this;
    }

    void SetValue(ADummy* pObject, AVariant const& Value) const
    {
        Setter(pObject, Value);
    }

    void SetValue(ADummy* pObject, AStringView Value) const
    {
        SetValue(pObject, AVariant(GetType(), GetEnum(), Value));
    }

    AVariant GetValue(ADummy const* pObject) const
    {
        return Getter(pObject);
    }

    void CopyValue(ADummy* Dst, ADummy const* Src) const
    {
        return Copy(Dst, Src);
    }

    VARIANT_TYPE          GetType() const { return Type; }
    const char*           GetName() const { return Name; }
    SEnumDef const*       GetEnum() const { return pEnum; }
    SPropertyRange const& GetRange() const { return Range; }
    HK_PROPERTY_FLAGS     GetFlags() const { return Flags; }
    AProperty const*      Next() const { return pNext; }
    AProperty const*      Prev() const { return pPrev; }

private:
    VARIANT_TYPE      Type;
    const char*       Name;
    SEnumDef const*   pEnum;
    SPropertyRange    Range;
    HK_PROPERTY_FLAGS Flags;
    SetterFun         Setter;
    GetterFun         Getter;
    CopyFun           Copy;
    AProperty const*  pNext;
    AProperty const*  pPrev;
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

#define _HK_GENERATED_CLASS_BODY()                   \
public:                                              \
    static ThisClassMeta const& ClassMeta()          \
    {                                                \
        static const ThisClassMeta __Meta;           \
        return __Meta;                               \
    }                                                \
    static AClassMeta const* SuperClass()            \
    {                                                \
        return ClassMeta().SuperClass();             \
    }                                                \
    static const char* ClassName()                   \
    {                                                \
        return ClassMeta().GetName();                \
    }                                                \
    static uint64_t ClassId()                        \
    {                                                \
        return ClassMeta().GetId();                  \
    }                                                \
    virtual AClassMeta const& FinalClassMeta() const \
    {                                                \
        return ClassMeta();                          \
    }                                                \
    virtual const char* FinalClassName() const       \
    {                                                \
        return ClassName();                          \
    }                                                \
    virtual uint64_t FinalClassId() const            \
    {                                                \
        return ClassId();                            \
    }                                                \
    void* operator new(size_t SizeInBytes)           \
    {                                                \
        return Allocator().allocate(SizeInBytes);    \
    }                                                \
    void operator delete(void* Ptr)                  \
    {                                                \
        Allocator().deallocate(Ptr);                 \
    }

#define HK_CLASS(Class, SuperClass) \
    HK_FACTORY_CLASS(AClassMeta::DummyFactory(), Class, SuperClass)

#define HK_FACTORY_CLASS(Factory, Class, SuperClass) \
    HK_FACTORY_CLASS_A(Factory, Class, SuperClass, ADummy::Allocator)

#define HK_FACTORY_CLASS_A(Factory, Class, SuperClass, _Allocator)                      \
    HK_FORBID_COPY(Class)                                                               \
    friend class ADummy;                                                                \
                                                                                        \
public:                                                                                 \
    typedef SuperClass Super;                                                           \
    typedef Class      ThisClass;                                                       \
    typedef _Allocator Allocator;                                                       \
    class ThisClassMeta : public AClassMeta                                             \
    {                                                                                   \
    public:                                                                             \
        ThisClassMeta() : AClassMeta(Factory, HK_STRINGIFY(Class), &Super::ClassMeta()) \
        {                                                                               \
            RegisterProperties();                                                       \
        }                                                                               \
        ADummy* CreateInstance() const override                                         \
        {                                                                               \
            return new ThisClass;                                                       \
        }                                                                               \
                                                                                        \
    private:                                                                            \
        void RegisterProperties();                                                      \
    };                                                                                  \
    _HK_GENERATED_CLASS_BODY()                                                          \
private:



#define HK_BEGIN_CLASS_META(Class)                               \
    AClassMeta const& Class##__Meta = Class::ClassMeta();        \
    void              Class::ThisClassMeta::RegisterProperties() \
    {

#define HK_END_CLASS_META() \
    }

#define HK_CLASS_META(Class)   \
    HK_BEGIN_CLASS_META(Class) \
    HK_END_CLASS_META()

/** Provides direct access to a class member. */
#define HK_PROPERTY_DIRECT_RANGE(Property, Flags, Range)                                               \
    {                                                                                                  \
        using PropertyType = decltype(ThisClass::Property);                                            \
        static AProperty const decl_##Property(                                                        \
            *this,                                                                                     \
            VariantTraits::GetVariantType<PropertyType>(),                                             \
            VariantTraits::GetVariantEnum<PropertyType>(),                                             \
            #Property,                                                                                 \
            [](ADummy* pObject, AVariant const& Value) {                                               \
                auto* pValue = Value.Get<PropertyType>();                                              \
                if (pValue)                                                                            \
                {                                                                                      \
                    static_cast<ThisClass*>(pObject)->Property = *pValue;                              \
                }                                                                                      \
            },                                                                                         \
            [](ADummy const* pObject) -> AVariant {                                                    \
                return static_cast<ThisClass const*>(pObject)->Property;                               \
            },                                                                                         \
            [](ADummy* Dst, ADummy const* Src) {                                                       \
                static_cast<ThisClass*>(Dst)->Property = static_cast<ThisClass const*>(Src)->Property; \
            },                                                                                         \
            Range,                                                                                     \
            Flags);                                                                                    \
    }

/** Provides direct access to a class member. */
#define HK_PROPERTY_DIRECT(Property, Flags) HK_PROPERTY_DIRECT_RANGE(Property, Flags, RangeUnbound())

template <class T>
struct RemoveCVRef
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

/** Provides access to a property via a setter/getter. */
#define HK_PROPERTY_RANGE(Property, Setter, Getter, Flags, Range)                                                   \
    {                                                                                                               \
        using PropertyType     = RemoveCVRef<typename TArgumentType<0U, decltype(&ThisClass::Setter)>::Type>::type; \
        using GetterReturnType = RemoveCVRef<TReturnType<decltype(&ThisClass::Getter)>::Type>::type;                \
        static_assert(std::is_same<PropertyType, GetterReturnType>::value, "Setter and getter type mismatch");      \
        static AProperty const decl_##Property(                                                                     \
            *this,                                                                                                  \
            VariantTraits::GetVariantType<PropertyType>(),                                                          \
            VariantTraits::GetVariantEnum<PropertyType>(),                                                          \
            #Property,                                                                                              \
            [](ADummy* pObject, AVariant const& Value) {                                                            \
                auto* pValue = Value.Get<PropertyType>();                                                           \
                if (pValue)                                                                                         \
                {                                                                                                   \
                    static_cast<ThisClass*>(pObject)->Setter(*pValue);                                              \
                }                                                                                                   \
            },                                                                                                      \
            [](ADummy const* pObject) -> AVariant {                                                                 \
                return static_cast<ThisClass const*>(pObject)->Getter();                                            \
            },                                                                                                      \
            [](ADummy* Dst, ADummy const* Src) {                                                                    \
                static_cast<ThisClass*>(Dst)->Setter(static_cast<ThisClass const*>(Src)->Getter());                 \
            },                                                                                                      \
            Range,                                                                                                  \
            Flags);                                                                                                 \
    }

/** Provides access to a property via a setter/getter. */
#define HK_PROPERTY(Property, Setter, Getter, Flags) HK_PROPERTY_RANGE(Property, Setter, Getter, Flags, RangeUnbound())


template <typename T, typename... TArgs>
T* CreateInstanceOf(TArgs&&... Args)
{
    return new T(std::forward<TArgs>(Args)...);
}

/**

ADummy

Base factory object class.

*/
class ADummy
{
    HK_FORBID_COPY(ADummy)

public:
    typedef ADummy         ThisClass;
    typedef Allocators::HeapMemoryAllocator<HEAP_WORLD_OBJECTS> Allocator;
    class ThisClassMeta : public AClassMeta
    {
    public:
        ThisClassMeta() :
            AClassMeta(AClassMeta::DummyFactory(), "ADummy", nullptr)
        {
        }

        ADummy* CreateInstance() const override
        {
            return CreateInstanceOf<ThisClass>();
        }

    private:
        void RegisterProperties();
    };

    ADummy() {}
    virtual ~ADummy() {}

    _HK_GENERATED_CLASS_BODY()
};

template <typename T>
T* Upcast(ADummy* Object)
{
    if (Object && Object->FinalClassMeta().IsSubclassOf<T>())
    {
        return static_cast<T*>(Object);
    }
    return nullptr;
}

template <typename T>
T const* Upcast(ADummy const* Object)
{
    if (Object && Object->FinalClassMeta().IsSubclassOf<T>())
    {
        return static_cast<T const*>(Object);
    }
    return nullptr;
}
