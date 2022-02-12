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
#include <Core/String.h>
#include <Core/Ref.h>
#include <Containers/Hash.h>
#include <Containers/PodVector.h>
#include <Geometry/VectorMath.h>
#include <Geometry/Quat.h>

class AClassMeta;
class AProperty;
class ADummy;

class AObjectFactory
{
    HK_FORBID_COPY(AObjectFactory)

    friend class AClassMeta;
    friend void InitializeFactories();
    friend void DeinitializeFactories();

public:
    AObjectFactory(const char* _Tag);
    ~AObjectFactory();

    const char* GetTag() const { return Tag; }

    ADummy* CreateInstance(const char* _ClassName) const;
    ADummy* CreateInstance(uint64_t _ClassId) const;

    AClassMeta const* GetClassList() const;

    AClassMeta const* FindClass(const char* _ClassName) const;

    AClassMeta const* LookupClass(const char* _ClassName) const;
    AClassMeta const* LookupClass(uint64_t _ClassId) const;

    uint64_t FactoryClassCount() const { return NumClasses; }

    static AObjectFactory const* Factories() { return FactoryList; }
    AObjectFactory const*        Next() const { return NextFactory; }

private:
    const char*            Tag;
    AClassMeta*            Classes;
    mutable AClassMeta**   IdTable;
    mutable THash<>        NameTable;
    uint64_t               NumClasses;
    AObjectFactory*        NextFactory;
    static AObjectFactory* FactoryList;
};

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

    bool IsSubclassOf(AClassMeta const& _Superclass) const
    {
        for (AClassMeta const* meta = this; meta; meta = meta->SuperClass())
        {
            if (meta->GetId() == _Superclass.GetId())
            {
                return true;
            }
        }
        return false;
    }

    template <typename _Superclass>
    bool IsSubclassOf() const
    {
        return IsSubclassOf(_Superclass::ClassMeta());
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
    void             GetProperties(TPodVector<AProperty const*>& Properties, bool bRecursive = true) const;

protected:
    AClassMeta(AObjectFactory& _Factory, const char* _ClassName, AClassMeta const* _SuperClassMeta) :
        ClassId(_Factory.NumClasses + 1), ClassName(_ClassName)
    {
        HK_ASSERT_(_Factory.FindClass(_ClassName) == NULL, "Class already defined");
        pNext            = _Factory.Classes;
        pSuperClass      = _SuperClassMeta;
        PropertyList     = nullptr;
        PropertyListTail = nullptr;
        _Factory.Classes = this;
        _Factory.NumClasses++;
        pFactory = &_Factory;
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

HK_FORCEINLINE ADummy* AObjectFactory::CreateInstance(const char* _ClassName) const
{
    AClassMeta const* classMeta = LookupClass(_ClassName);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

HK_FORCEINLINE ADummy* AObjectFactory::CreateInstance(uint64_t _ClassId) const
{
    AClassMeta const* classMeta = LookupClass(_ClassId);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

HK_FORCEINLINE AClassMeta const* AObjectFactory::GetClassList() const
{
    return Classes;
}

using APackedValue = AString;

struct SEnumDef
{
    int64_t     Value;
    const char* HumanReadableName;
};

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
#define HK_PROPERTY_DEFAULT          0
#define HK_PROPERTY_NON_SERIALIZABLE 1
#define HK_PROPERTY_BITMASK          2

enum PROPERTY_TYPE
{
    PROPERTY_TYPE_BOOLEAN,
    PROPERTY_TYPE_SIGNED_INT,
    PROPERTY_TYPE_UNSIGNED_INT,
    PROPERTY_TYPE_FLOAT32,
    PROPERTY_TYPE_FLOAT64,
    PROPERTY_TYPE_ENUM,
    PROPERTY_TYPE_STRING,
    PROPERTY_TYPE_NON_TRIVIAL
};

class AProperty
{
    HK_FORBID_COPY(AProperty)

public:
    using SetterFun = void (*)(ADummy*, APackedValue const&);
    using GetterFun = APackedValue (*)(ADummy const*);
    using CopyFun   = void (*)(ADummy*, ADummy const*);

    AProperty(AClassMeta const& _ClassMeta, PROPERTY_TYPE Type, SEnumDef const* EnumDef, const char* Name, SetterFun Setter, GetterFun Getter, CopyFun Copy, SPropertyRange const& Range, uint32_t Flags) :
        Type(Type),
        Name(Name),
        NameHash(Core::Hash(Name, Platform::Strlen(Name))),
        EnumDef(EnumDef),
        Range(Range),
        Flags(Flags),
        Setter(std::move(Setter)),
        Getter(std::move(Getter)),
        Copy(std::move(Copy))

    {
        AClassMeta& classMeta = const_cast<AClassMeta&>(_ClassMeta);
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

    void SetValue(ADummy* pObject, APackedValue const& PackedVal) const
    {
        Setter(pObject, PackedVal);
    }

    APackedValue GetValue(ADummy const* pObject) const
    {
        return Getter(pObject);
    }

    void CopyValue(ADummy* Dst, ADummy const* Src) const
    {
        return Copy(Dst, Src);
    }

    PROPERTY_TYPE         GetType() const { return Type; }
    const char*           GetName() const { return Name; }
    int                   GetNameHash() const { return NameHash; }
    SEnumDef const*       GetEnumDef() const { return EnumDef; }
    SPropertyRange const& GetRange() const { return Range; }
    uint32_t              GetFlags() const { return Flags; }
    AProperty const*      Next() const { return pNext; }
    AProperty const*      Prev() const { return pPrev; }

private:
    PROPERTY_TYPE    Type;
    const char*      Name;
    int              NameHash;
    SEnumDef const*  EnumDef;
    SPropertyRange   Range;
    uint32_t         Flags;
    SetterFun        Setter;
    GetterFun        Getter;
    CopyFun          Copy;
    AProperty const* pNext;
    AProperty const* pPrev;
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

template <typename T>
SEnumDef const* GetEnumDef();

HK_INLINE APackedValue PackEnum(SEnumDef const* EnumDef, int64_t Value)
{
    if (EnumDef)
    {
        for (int i = 0; EnumDef[i].HumanReadableName; i++)
        {
            if (EnumDef[i].Value == Value)
                return EnumDef[i].HumanReadableName;
        }
        HK_ASSERT(0);
    }
    return "[Undefined]";
}

HK_INLINE int64_t UnpackEnum(SEnumDef const* EnumDef, APackedValue const& PackedVal)
{
    if (EnumDef)
    {
        for (int i = 0; EnumDef[i].HumanReadableName; i++)
        {
            if (EnumDef[i].HumanReadableName == PackedVal)
                return EnumDef[i].Value;
        }
        HK_ASSERT(0);
    }
    return 0;
}

template <typename T>
APackedValue PackObject(T const&);

template <typename T>
T UnpackObject(APackedValue const&);

template <typename T>
APackedValue PackType(T const& Val, typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
{
    return PackObject(Val);
}

template <typename T>
APackedValue PackType(T const& Val, typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    return PackEnum(GetEnumDef<T>(), static_cast<int64_t>(Val));
}

template <typename T>
T UnpackType(APackedValue const& PackedVal, typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
{
    return UnpackObject<T>(PackedVal);
}

template <typename T>
T UnpackType(APackedValue const& PackedVal, typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    return static_cast<T>(UnpackEnum(GetEnumDef<T>(), PackedVal));
}

template <typename T>
SEnumDef const* _GetEnumDef(typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
{
    return nullptr;
}

template <typename T>
SEnumDef const* _GetEnumDef(typename std::enable_if<std::is_enum<T>::value>::type* = 0)
{
    return GetEnumDef<T>();
}

template <typename T>
constexpr PROPERTY_TYPE GetPropertyType()
{
    // clang-format off
    constexpr bool IsBoolean     = std::is_same<T, bool>::value;
    constexpr bool IsEnum        = std::is_enum<T>::value;
    constexpr bool IsString      = std::is_same<T, AString>::value;
    constexpr bool IsSignedInt   = std::is_integral<T>::value && std::is_signed<T>::value;
    constexpr bool IsUnsignedInt = std::is_integral<T>::value && std::is_unsigned<T>::value;
    constexpr bool IsFloat32     = std::is_same<T, float>::value;
    constexpr bool IsFloat64     = std::is_same<T, double>::value;

    return IsBoolean ?
            PROPERTY_TYPE_BOOLEAN :
            (
                IsEnum ?
                PROPERTY_TYPE_ENUM :
                (
                    IsSignedInt ?
                    PROPERTY_TYPE_SIGNED_INT :
                    (
                        IsUnsignedInt ?
                        PROPERTY_TYPE_UNSIGNED_INT :
                        (
                            IsFloat32 ?
                            PROPERTY_TYPE_FLOAT32 :
                            (
                                IsFloat64 ?
                                PROPERTY_TYPE_FLOAT64 :
                                (
                                    IsString ?
                                    PROPERTY_TYPE_STRING :
                                    (
                                        PROPERTY_TYPE_NON_TRIVIAL
                                    )
                                )
                            )
                        )
                    )
                )
            );
    // clang-format on
}

template <> HK_FORCEINLINE APackedValue PackObject(bool const& Val) { return Val ? "1" : "0"; }
template <> HK_FORCEINLINE APackedValue PackObject(uint8_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(uint16_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(uint32_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(uint64_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(int8_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(int16_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(int32_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(int64_t const& Val) { return Math::ToString(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(float const& Val) { return Math::ToString(*((uint32_t*)&Val)); }
template <> HK_FORCEINLINE APackedValue PackObject(double const& Val) { return Math::ToString(*((uint64_t*)&Val)); }

template <typename VectorType>
APackedValue PackVector(VectorType const& Vector)
{
    APackedValue packedVal;
    for (auto i = 0; i < Vector.NumComponents(); i++)
    {
        packedVal += Math::ToString(*((uint32_t*)&Vector[i]));
        if (i + 1 < Vector.NumComponents())
            packedVal += " ";
    }
    return packedVal;
}

template <> HK_FORCEINLINE APackedValue PackObject(Float2 const& Val) { return PackVector(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(Float3 const& Val) { return PackVector(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(Float4 const& Val) { return PackVector(Val); }
template <> HK_FORCEINLINE APackedValue PackObject(Quat const& Val) { return PackVector(Val); }

template <> HK_FORCEINLINE bool     UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<uint64_t>(PackedVal) != 0; }
template <> HK_FORCEINLINE uint8_t  UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<uint8_t>(PackedVal); }
template <> HK_FORCEINLINE uint16_t UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<uint16_t>(PackedVal); }
template <> HK_FORCEINLINE uint32_t UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<uint32_t>(PackedVal); }
template <> HK_FORCEINLINE uint64_t UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<uint64_t>(PackedVal); }
template <> HK_FORCEINLINE int8_t   UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<int8_t>(PackedVal); }
template <> HK_FORCEINLINE int16_t  UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<int16_t>(PackedVal); }
template <> HK_FORCEINLINE int32_t  UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<int32_t>(PackedVal); }
template <> HK_FORCEINLINE int64_t  UnpackObject(APackedValue const& PackedVal) { return Math::ToInt<int64_t>(PackedVal); }

template <>
HK_FORCEINLINE float UnpackObject(APackedValue const& PackedVal)
{
    uint32_t i = Math::ToInt<uint32_t>(PackedVal);
    return *(float*)&i;
}

template <>
HK_FORCEINLINE double UnpackObject(APackedValue const& PackedVal)
{
    uint64_t i = Math::ToInt<uint64_t>(PackedVal);
    return *(double*)&i;
}

template <typename VectorType>
VectorType UnpackVector(APackedValue const& PackedVal)
{
    uint32_t tmp[VectorType::NumComponents()] = {};

    switch (VectorType::NumComponents())
    {
        case 2:
            sscanf(PackedVal.CStr(), "%d %d", &tmp[0], &tmp[1]);
            break;
        case 3:
            sscanf(PackedVal.CStr(), "%d %d %d", &tmp[0], &tmp[1], &tmp[2]);
            break;
        case 4:
            sscanf(PackedVal.CStr(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
            break;
    }

    VectorType v;
    for (auto i = 0; i < v.NumComponents(); i++)
    {
        v[i] = *(float*)&tmp[i];
    }
    return v;
}

template <> HK_FORCEINLINE Float2 UnpackObject(APackedValue const& PackedVal) { return UnpackVector<Float2>(PackedVal); }
template <> HK_FORCEINLINE Float3 UnpackObject(APackedValue const& PackedVal) { return UnpackVector<Float3>(PackedVal); }
template <> HK_FORCEINLINE Float4 UnpackObject(APackedValue const& PackedVal) { return UnpackVector<Float4>(PackedVal); }
template <> HK_FORCEINLINE Quat   UnpackObject(APackedValue const& PackedVal) { return UnpackVector<Quat>(PackedVal); }

template <>
HK_FORCEINLINE APackedValue PackObject(AString const& Val)
{
    return Val;
}

template <>
HK_FORCEINLINE AString UnpackObject(APackedValue const& PackedVal)
{
    return PackedVal;
}

template <typename T>
APackedValue PackObject(T const& Val)
{
    return Val.Pack();
}

template <typename T>
T UnpackObject(APackedValue const& PackedVal)
{
    T val;
    val.Unpack(PackedVal);
    return val;
}


#define _HK_GENERATED_CLASS_BODY()                    \
public:                                               \
    static ThisClassMeta const& ClassMeta()           \
    {                                                 \
        static const ThisClassMeta __Meta;            \
        return __Meta;                                \
    }                                                 \
    static AClassMeta const* SuperClass()             \
    {                                                 \
        return ClassMeta().SuperClass();              \
    }                                                 \
    static const char* ClassName()                    \
    {                                                 \
        return ClassMeta().GetName();                 \
    }                                                 \
    static uint64_t ClassId()                         \
    {                                                 \
        return ClassMeta().GetId();                   \
    }                                                 \
    virtual AClassMeta const& FinalClassMeta() const  \
    {                                                 \
        return ClassMeta();                           \
    }                                                 \
    virtual const char* FinalClassName() const        \
    {                                                 \
        return ClassName();                           \
    }                                                 \
    virtual uint64_t FinalClassId() const             \
    {                                                 \
        return ClassId();                             \
    }                                                 \
    void* operator new(size_t _SizeInBytes)           \
    {                                                 \
        return Allocator::Inst().Alloc(_SizeInBytes); \
    }                                                 \
    void operator delete(void* _Ptr)                  \
    {                                                 \
        Allocator::Inst().Free(_Ptr);                 \
    }

#define HK_CLASS(_Class, _SuperClass) \
    HK_FACTORY_CLASS(AClassMeta::DummyFactory(), _Class, _SuperClass)

#define HK_FACTORY_CLASS(_Factory, _Class, _SuperClass) \
    HK_FACTORY_CLASS_A(_Factory, _Class, _SuperClass, ADummy::Allocator)

#define HK_FACTORY_CLASS_A(_Factory, _Class, _SuperClass, _Allocator)                     \
    HK_FORBID_COPY(_Class)                                                                \
    friend class ADummy;                                                                  \
                                                                                          \
public:                                                                                   \
    typedef _SuperClass Super;                                                            \
    typedef _Class      ThisClass;                                                        \
    typedef _Allocator  Allocator;                                                        \
    class ThisClassMeta : public AClassMeta                                               \
    {                                                                                     \
    public:                                                                               \
        ThisClassMeta() : AClassMeta(_Factory, HK_STRINGIFY(_Class), &Super::ClassMeta()) \
        {                                                                                 \
            RegisterProperties();                                                         \
        }                                                                                 \
        ADummy* CreateInstance() const override                                           \
        {                                                                                 \
            return new ThisClass;                                                         \
        }                                                                                 \
                                                                                          \
    private:                                                                              \
        void RegisterProperties();                                                        \
    };                                                                                    \
    _HK_GENERATED_CLASS_BODY()                                                            \
private:



#define HK_BEGIN_CLASS_META(_Class)                               \
    AClassMeta const& _Class##__Meta = _Class::ClassMeta();       \
    void              _Class::ThisClassMeta::RegisterProperties() \
    {

#define HK_END_CLASS_META() \
    }

#define HK_CLASS_META(_Class)   \
    HK_BEGIN_CLASS_META(_Class) \
    HK_END_CLASS_META()

/** Provides direct access to a class member. */
#define HK_PROPERTY_DIRECT_RANGE(Member, Flags, Range)                                                                                                                                   \
    static AProperty const Property##Member(                                                                                                                                             \
        *this,                                                                                                                                                                           \
        GetPropertyType<decltype(ThisClass::Member)>(),                                                                                                                                  \
        _GetEnumDef<decltype(ThisClass::Member)>(),                                                                                                                                      \
        #Member,                                                                                                                                                                         \
        [](ADummy* pObject, APackedValue const& PackedVal)                                                                                                                               \
        {                                                                                                                                                                                \
            using T        = decltype(ThisClass::Member);                                                                                                                                \
            static_cast<ThisClass*>(pObject)->Member = UnpackType<T>(PackedVal);                                                                                                       \
        },                                                                                                                                                                               \
        [](ADummy const* pObject) -> APackedValue                                                                                                                                        \
        {                                                                                                                                                                                \
            return PackType(static_cast<ThisClass const*>(pObject)->Member);                                                                                                      \
        },                                                                                                                                                                               \
        [](ADummy* Dst, ADummy const* Src)                                                                                                                                               \
        {                                                                                                                                                                                \
            static_cast<ThisClass*>(Dst)->Member = static_cast<ThisClass const*>(Src)->Member;                                                                                          \
        },                                                                                                                                                                               \
        Range,                                                                                                                                                                           \
        Flags);

/** Provides direct access to a class member. */
#define HK_PROPERTY_DIRECT(Member, Flags) HK_PROPERTY_DIRECT_RANGE(Member, Flags, RangeUnbound())

/** Provides access to a class member via a setter/getter. */
#define HK_PROPERTY_RANGE(Member, Setter, Getter, Flags, Range)                                                                                                                                                                                                          \
    static AProperty const Property##Member(                                                                                                                                                                                                                             \
        *this,                                                                                                                                                                                                                                                           \
        GetPropertyType<decltype(ThisClass::Member)>(),                                                                                                                                                                                                                  \
        _GetEnumDef<decltype(ThisClass::Member)>(),                                                                                                                                                                                                                      \
        #Member,                                                                                                                                                                                                                                                         \
        [](ADummy* pObject, APackedValue const& PackedVal)                                                                                                                                                                                                               \
        {                                                                                                                                                                                                                                                                \
            using T = decltype(ThisClass::Member);                                                                                                                                                                                                                       \
            static_assert(std::is_same<typename TArgumentType<0U, decltype(&ThisClass::Setter)>::Type, T>::value || std::is_same<typename TArgumentType<0U, decltype(&ThisClass::Setter)>::Type, T const&>::value, "Setter argument type does not match property type"); \
            static_cast<ThisClass*>(pObject)->Setter(UnpackType<T>(PackedVal));                                                                                                                                                                                        \
        },                                                                                                                                                                                                                                                               \
        [](ADummy const* pObject) -> APackedValue                                                                                                                                                                                                                        \
        {                                                                                                                                                                                                                                                                \
            using T = decltype(ThisClass::Member);                                                                                                                                                                                                                       \
            static_assert(std::is_same<TReturnType<decltype(&ThisClass::Getter)>::Type, T>::value || std::is_same<TReturnType<decltype(&ThisClass::Getter)>::Type, T const&>::value, "Getter return type does not match property type");                                 \
            return PackType(static_cast<ThisClass const*>(pObject)->Getter());                                                                                                                                                                                         \
        },                                                                                                                                                                                                                                                               \
        [](ADummy* Dst, ADummy const* Src)                                                                                                                                                                                                                               \
        {                                                                                                                                                                                                                                                                \
            static_cast<ThisClass*>(Dst)->Setter(static_cast<ThisClass const*>(Src)->Getter());                                                                                                                                                                          \
        },                                                                                                                                                                                                                                                               \
        Range,                                                                                                                                                                                                                                                           \
        Flags);

/** Provides access to a class member via a setter/getter. */
#define HK_PROPERTY(Member, Setter, Getter, Flags) HK_PROPERTY_RANGE(Member, Setter, Getter, Flags, RangeUnbound())

/** Provides access to a class property via a setter/getter without a class member. */
#define HK_PROPERTY2_RANGE(MemberType, Name, Setter, Getter, Flags, Range)                                                                                                                                                                                               \
    static AProperty const Property##Setter(                                                                                                                                                                                                                             \
        *this,                                                                                                                                                                                                                                                           \
        GetPropertyType<MemberType>(),                                                                                                                                                                                                                                   \
        _GetEnumDef<MemberType>(),                                                                                                                                                                                                                                       \
        Name,                                                                                                                                                                                                                                                            \
        [](ADummy* pObject, APackedValue const& PackedVal)                                                                                                                                                                                                               \
        {                                                                                                                                                                                                                                                                \
            using T = MemberType;                                                                                                                                                                                                                                        \
            static_assert(std::is_same<typename TArgumentType<0U, decltype(&ThisClass::Setter)>::Type, T>::value || std::is_same<typename TArgumentType<0U, decltype(&ThisClass::Setter)>::Type, T const&>::value, "Setter argument type does not match property type"); \
            static_cast<ThisClass*>(pObject)->Setter(UnpackType<T>(PackedVal));                                                                                                                                                                                        \
        },                                                                                                                                                                                                                                                               \
        [](ADummy const* pObject) -> APackedValue                                                                                                                                                                                                                        \
        {                                                                                                                                                                                                                                                                \
            using T = MemberType;                                                                                                                                                                                                                                        \
            static_assert(std::is_same<TReturnType<decltype(&ThisClass::Getter)>::Type, T>::value || std::is_same<TReturnType<decltype(&ThisClass::Getter)>::Type, T const&>::value, "Getter return type does not match property type");                                 \
            return PackType(static_cast<ThisClass const*>(pObject)->Getter());                                                                                                                                                                                         \
        },                                                                                                                                                                                                                                                               \
        [](ADummy* Dst, ADummy const* Src)                                                                                                                                                                                                                               \
        {                                                                                                                                                                                                                                                                \
            static_cast<ThisClass*>(Dst)->Setter(static_cast<ThisClass const*>(Src)->Getter());                                                                                                                                                                          \
        },                                                                                                                                                                                                                                                               \
        Range,                                                                                                                                                                                                                                                           \
        Flags);

/** Provides access to a class property via a setter/getter without a class member. */
#define HK_PROPERTY2(MemberType, Name, Setter, Getter, Flags) HK_PROPERTY2_RANGE(MemberType, Name, Setter, Getter, Flags, RangeUnbound())

template <typename T, typename... TArgs>
T* CreateInstanceOf(TArgs&&... _Args)
{
    return new T(std::forward<TArgs>(_Args)...);
}

/**

ADummy

Base factory object class.
Needs to resolve class meta data.

*/
class ADummy
{
    HK_FORBID_COPY(ADummy)

public:
    typedef ADummy         ThisClass;
    typedef AZoneAllocator Allocator;
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
T* Upcast(ADummy* _Object)
{
    if (_Object && _Object->FinalClassMeta().IsSubclassOf<T>())
    {
        return static_cast<T*>(_Object);
    }
    return nullptr;
}

void InitializeFactories();
void DeinitializeFactories();
