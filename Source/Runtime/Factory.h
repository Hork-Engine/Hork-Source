/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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
class AAttributeMeta;
class ADummy;

class AObjectFactory
{
    AN_FORBID_COPY(AObjectFactory)

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
    AN_FORBID_COPY(AClassMeta)

    friend class AObjectFactory;
    friend class AAttributeMeta;

public:
    const uint64_t ClassId;

    const char*           GetName() const { return ClassName; }
    uint64_t              GetId() const { return ClassId; }
    AClassMeta const*     SuperClass() const { return pSuperClass; }
    AClassMeta const*     Next() const { return pNext; }
    AObjectFactory const* Factory() const { return pFactory; }
    AAttributeMeta const* GetAttribList() const { return AttributesHead; }

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

    // TODO: class flags?

    virtual ADummy* CreateInstance() const = 0;

    static void CloneAttributes(ADummy const* _Template, ADummy* _Destination);

    static AObjectFactory& DummyFactory()
    {
        static AObjectFactory ObjectFactory("Dummy factory");
        return ObjectFactory;
    }

    // Utilites
    AAttributeMeta const* FindAttribute(AStringView _Name, bool _Recursive) const;
    void                  GetAttributes(TPodVector<AAttributeMeta const*>& _Attributes, bool _Recursive = true) const;

protected:
    AClassMeta(AObjectFactory& _Factory, const char* _ClassName, AClassMeta const* _SuperClassMeta) :
        ClassId(_Factory.NumClasses + 1), ClassName(_ClassName)
    {
        AN_ASSERT_(_Factory.FindClass(_ClassName) == NULL, "Class already defined");
        pNext            = _Factory.Classes;
        pSuperClass      = _SuperClassMeta;
        AttributesHead   = nullptr;
        AttributesTail   = nullptr;
        _Factory.Classes = this;
        _Factory.NumClasses++;
        pFactory = &_Factory;
    }

private:
    static void CloneAttributes_r(AClassMeta const* Meta, ADummy const* _Template, ADummy* _Destination);

    const char*           ClassName;
    AClassMeta*           pNext;
    AClassMeta const*     pSuperClass;
    AObjectFactory const* pFactory;
    AAttributeMeta const* AttributesHead;
    AAttributeMeta const* AttributesTail;
};

AN_FORCEINLINE ADummy* AObjectFactory::CreateInstance(const char* _ClassName) const
{
    AClassMeta const* classMeta = LookupClass(_ClassName);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

AN_FORCEINLINE ADummy* AObjectFactory::CreateInstance(uint64_t _ClassId) const
{
    AClassMeta const* classMeta = LookupClass(_ClassId);
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

AN_FORCEINLINE AClassMeta const* AObjectFactory::GetClassList() const
{
    return Classes;
}

class AAttributeMeta
{
    AN_FORBID_COPY(AAttributeMeta)

public:
    const char* GetName() const { return Name.c_str(); }
    int         GetNameHash() const { return NameHash; }
    uint32_t    GetFlags() const { return Flags; }

    AAttributeMeta const* Next() const { return pNext; }
    AAttributeMeta const* Prev() const { return pPrev; }

    AAttributeMeta(AClassMeta const& _ClassMeta, const char* _Name, uint32_t _Flags) :
        Name(std::string(_ClassMeta.GetName()) + "." + _Name), NameHash(Core::Hash(Name.c_str(), Name.length())), Flags(_Flags)
    {
        AClassMeta& classMeta = const_cast<AClassMeta&>(_ClassMeta);
        pNext                 = nullptr;
        pPrev                 = classMeta.AttributesTail;
        if (pPrev)
        {
            const_cast<AAttributeMeta*>(pPrev)->pNext = this;
        }
        else
        {
            classMeta.AttributesHead = this;
        }
        classMeta.AttributesTail = this;
    }

    // TODO: Min/Max range for integer or float attributes, support for enums

    void SetValue(ADummy* _Object, AString const& _Value) const
    {
        FromString(_Object, _Value);
    }

    void GetValue(ADummy* _Object, AString& _Value) const
    {
        ToString(_Object, _Value);
    }

    void CopyValue(ADummy const* _Src, ADummy* _Dst) const
    {
        Copy(_Src, _Dst);
    }

private:
    std::string           Name;
    int                   NameHash;
    AAttributeMeta const* pNext;
    AAttributeMeta const* pPrev;
    uint32_t              Flags;

protected:
    std::function<void(ADummy*, AString const&)> FromString;
    std::function<void(ADummy*, AString&)>       ToString;
    std::function<void(ADummy const*, ADummy*)>  Copy;
};


template <typename AttributeType>
void SetAttributeFromString(AttributeType& Attribute, AString const& String);


AN_FORCEINLINE void SetAttributeFromString(uint8_t& Attribute, AString const& String)
{
    Attribute = Math::ToInt<uint8_t>(String);
}

AN_FORCEINLINE void SetAttributeFromString(bool& Attribute, AString const& String)
{
    Attribute = !!Math::ToInt<uint8_t>(String);
}

AN_FORCEINLINE void SetAttributeFromString(int32_t& Attribute, AString const& String)
{
    Attribute = Math::ToInt<int32_t>(String);
}

AN_FORCEINLINE void SetAttributeFromString(float& Attribute, AString const& String)
{
    uint32_t i = Math::ToInt<uint32_t>(String);
    Attribute  = *(float*)&i;
}

AN_FORCEINLINE void SetAttributeFromString(Float2& Attribute, AString const& String)
{
    uint32_t tmp[2];
    sscanf(String.CStr(), "%d %d", &tmp[0], &tmp[1]);
    for (int i = 0; i < 2; i++)
    {
        Attribute[i] = *(float*)&tmp[i];
    }
}

AN_FORCEINLINE void SetAttributeFromString(Float3& Attribute, AString const& String)
{
    uint32_t tmp[3];
    sscanf(String.CStr(), "%d %d %d", &tmp[0], &tmp[1], &tmp[2]);
    for (int i = 0; i < 3; i++)
    {
        Attribute[i] = *(float*)&tmp[i];
    }
}

AN_FORCEINLINE void SetAttributeFromString(Float4& Attribute, AString const& String)
{
    uint32_t tmp[4];
    sscanf(String.CStr(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
    for (int i = 0; i < 4; i++)
    {
        Attribute[i] = *(float*)&tmp[i];
    }
}

AN_FORCEINLINE void SetAttributeFromString(Quat& Attribute, AString const& String)
{
    uint32_t tmp[4];
    sscanf(String.CStr(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
    for (int i = 0; i < 4; i++)
    {
        Attribute[i] = *(float*)&tmp[i];
    }
}

AN_FORCEINLINE void SetAttributeFromString(AString& Attribute, AString const& String)
{
    Attribute = String;
}


template <typename AttributeType>
void SetAttributeToString(AttributeType const& Attribute, AString& String);

AN_FORCEINLINE void SetAttributeToString(uint8_t const& Attribute, AString& String)
{
    String = Math::ToString(Attribute);
}

AN_FORCEINLINE void SetAttributeToString(bool const& Attribute, AString& String)
{
    String = Math::ToString(Attribute);
}

AN_FORCEINLINE void SetAttributeToString(int32_t const& Attribute, AString& String)
{
    String = Math::ToString(Attribute);
}

AN_FORCEINLINE void SetAttributeToString(float const& Attribute, AString& String)
{
    String = Math::ToString(*((uint32_t*)&Attribute));
}

AN_FORCEINLINE void SetAttributeToString(Float2 const& Attribute, AString& String)
{
    String = Math::ToString(*((uint32_t*)&Attribute.X)) + " " + Math::ToString(*((uint32_t*)&Attribute.Y));
    ;
}

AN_FORCEINLINE void SetAttributeToString(Float3 const& Attribute, AString& String)
{
    String = Math::ToString(*((uint32_t*)&Attribute.X)) + " " + Math::ToString(*((uint32_t*)&Attribute.Y)) + " " + Math::ToString(*((uint32_t*)&Attribute.Z));
}

AN_FORCEINLINE void SetAttributeToString(Float4 const& Attribute, AString& String)
{
    String = Math::ToString(*((uint32_t*)&Attribute.X)) + " " + Math::ToString(*((uint32_t*)&Attribute.Y)) + " " + Math::ToString(*((uint32_t*)&Attribute.Z)) + " " + Math::ToString(*((uint32_t*)&Attribute.W));
}

AN_FORCEINLINE void SetAttributeToString(Quat const& Attribute, AString& String)
{
    String = Math::ToString(*((uint32_t*)&Attribute.X)) + " " + Math::ToString(*((uint32_t*)&Attribute.Y)) + " " + Math::ToString(*((uint32_t*)&Attribute.Z)) + " " + Math::ToString(*((uint32_t*)&Attribute.W));
}

AN_FORCEINLINE void SetAttributeToString(AString const& Attribute, AString& String)
{
    String = Attribute;
}

template <typename ObjectType, typename AttributeType>
class TAttributeMeta : public AAttributeMeta
{
    AN_FORBID_COPY(TAttributeMeta)

public:
    template <typename AttributeSetterType, typename AttributeGetterType>
    TAttributeMeta(AClassMeta const& _ClassMeta, const char* _Name, void (ObjectType::*_Setter)(AttributeSetterType), AttributeGetterType (ObjectType::*_Getter)() const, int _Flags) :
        AAttributeMeta(_ClassMeta, _Name, _Flags)
    {
        FromString = [_Setter](ADummy* _Object, AString const& _Value)
        {
            AttributeType Attribute;

            SetAttributeFromString(Attribute, _Value);

            (*static_cast<ObjectType*>(_Object).*_Setter)(Attribute);
        };

        ToString = [_Getter](ADummy* _Object, AString& _Value)
        {
            SetAttributeToString((*static_cast<ObjectType*>(_Object).*_Getter)(), _Value);
        };

        Copy = [_Setter, _Getter](ADummy const* _Src, ADummy* _Dst)
        {
            (*static_cast<ObjectType*>(_Dst).*_Setter)((*static_cast<ObjectType const*>(_Src).*_Getter)());
        };
    }

    TAttributeMeta(AClassMeta const& _ClassMeta, const char* _Name, AttributeType* _AttribPointer, int _Flags) :
        AAttributeMeta(_ClassMeta, _Name, _Flags)
    {
        FromString = [_AttribPointer](ADummy* _Object, AString const& _Value)
        {
            SetAttributeFromString(*(AttributeType*)((byte*)static_cast<ObjectType*>(_Object) + (size_t)_AttribPointer), _Value);
        };

        ToString = [_AttribPointer](ADummy* _Object, AString& _Value)
        {
            SetAttributeToString(*(AttributeType*)((byte*)static_cast<ObjectType*>(_Object) + (size_t)_AttribPointer), _Value);
        };

        Copy = [_AttribPointer](ADummy const* _Src, ADummy* _Dst)
        {
            *(AttributeType*)((byte*)static_cast<ObjectType*>(_Dst) + (size_t)_AttribPointer) = *(AttributeType const*)((byte const*)static_cast<ObjectType const*>(_Src) + (size_t)_AttribPointer);
        };
    }
};

#define _AN_GENERATED_CLASS_BODY()                    \
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


//template <typename T>
//using EnableIfDerivedFromDummy = typename std::enable_if<std::is_base_of<class ADummy, T>::value, T>::type;
//
//template <typename T, EnableIfDerivedFromDummy<T>* = nullptr> class TObjectCreator
//{
//public:
//    template <typename... TArgs>
//    static T* CreateInstance(TArgs&&... _Args)
//    {
//        return new FinalClass(std::forward<TArgs>(_Args)...);
//    }
//
//private:
//    class FinalClass : private T
//    {
//        template <typename... TArgs>
//        FinalClass(TArgs&&... _Args) :
//            T(std::forward<TArgs>(_Args)...)
//        {
//        }
//        ~FinalClass()
//        {
//        }
//        virtual void FactoryImpl(typename T::SFactoryImplKey&&) override {}
//        friend class TObjectCreator;
//    };
//};

template <typename T, typename... TArgs> T* CreateInstanceOf(TArgs&&... _Args)
{
    //return TObjectCreator<T>::CreateInstance(std::forward<TArgs>(_Args)...);
    return new T(std::forward<TArgs>(_Args)...);
}

#define AN_CLASS(_Class, _SuperClass) \
    AN_FACTORY_CLASS(AClassMeta::DummyFactory(), _Class, _SuperClass)

#define AN_FACTORY_CLASS(_Factory, _Class, _SuperClass) \
    AN_FACTORY_CLASS_A(_Factory, _Class, _SuperClass, ADummy::Allocator)

#define AN_FACTORY_CLASS_A(_Factory, _Class, _SuperClass, _Allocator)                     \
    AN_FORBID_COPY(_Class)                                                                \
    friend class ADummy;                                                                  \
                                                                                          \
public:                                                                                   \
    typedef _SuperClass Super;                                                            \
    typedef _Class      ThisClass;                                                        \
    typedef _Allocator  Allocator;                                                        \
    class ThisClassMeta : public AClassMeta                                               \
    {                                                                                     \
    public:                                                                               \
        ThisClassMeta() : AClassMeta(_Factory, AN_STRINGIFY(_Class), &Super::ClassMeta()) \
        {                                                                                 \
            RegisterAttributes();                                                         \
        }                                                                                 \
        ADummy* CreateInstance() const override                                           \
        {                                                                                 \
            return new ThisClass;                                                         \
        }                                                                                 \
                                                                                          \
    private:                                                                              \
        void RegisterAttributes();                                                        \
    };                                                                                    \
    _AN_GENERATED_CLASS_BODY()                                                            \
private:



#define AN_BEGIN_CLASS_META(_Class)                               \
    AClassMeta const& _Class##__Meta = _Class::ClassMeta();       \
    void              _Class::ThisClassMeta::RegisterAttributes() \
    {

#define AN_END_CLASS_META() \
    }

#define AN_CLASS_META(_Class) AN_BEGIN_CLASS_META(_Class) \
AN_END_CLASS_META()

#define AN_ATTRIBUTE(_Name, _Type, _Setter, _Getter, _Flags) \
    static TAttributeMeta<ThisClass, _Type> const _Name##Meta(*this, AN_STRINGIFY(_Name), &ThisClass::_Setter, &ThisClass::_Getter, _Flags);

#define AN_ATTRIBUTE_(_Name, _Flags) \
    static TAttributeMeta<ThisClass, decltype(((ThisClass*)0)->_Name)> const _Name##Meta(*this, AN_STRINGIFY(_Name), (&((ThisClass*)0)->_Name), _Flags);

/* Attribute flags */
#define AF_DEFAULT          0
#define AF_NON_SERIALIZABLE 1


/**

ADummy

Base factory object class.
Needs to resolve class meta data.

*/
class ADummy
{
    AN_FORBID_COPY(ADummy)

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
        void RegisterAttributes();
    };

    ADummy() {}
    virtual ~ADummy() {}

    _AN_GENERATED_CLASS_BODY()

private:
    //struct SFactoryImplKey;
    //virtual void FactoryImpl(SFactoryImplKey&&) = 0;
    //template <typename T, EnableIfDerivedFromDummy<T>*> friend class TObjectCreator; // Allow TObjectCreator to access to SFactoryImplKey
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
