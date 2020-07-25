/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <Core/Public/Alloc.h>
#include <Core/Public/Hash.h>
#include <Core/Public/HashFunc.h>
#include <Core/Public/String.h>
#include <Core/Public/CoreMath.h>
#include <Core/Public/PodArray.h>

class AClassMeta;
class AAttributeMeta;
class APrecacheMeta;
class ADummy;

class ANGIE_API AObjectFactory {
    AN_FORBID_COPY( AObjectFactory )

    friend class AClassMeta;
    friend void InitializeFactories();
    friend void DeinitializeFactories();

public:
    AObjectFactory( const char * _Tag );
    ~AObjectFactory();

    const char * GetTag() const { return Tag; }

    ADummy * CreateInstance( const char * _ClassName ) const;
    ADummy * CreateInstance( uint64_t _ClassId ) const;

    template< typename T >
    T * CreateInstance() const { return static_cast< T * >( CreateInstance( T::ClassId() ) ); }

    AClassMeta const * GetClassList() const;

    AClassMeta const * FindClass( const char * _ClassName ) const;

    AClassMeta const * LookupClass( const char * _ClassName ) const;
    AClassMeta const * LookupClass( uint64_t _ClassId ) const;

    uint64_t FactoryClassCount() const { return NumClasses; }

    static AObjectFactory const * Factories() { return FactoryList; }
    AObjectFactory const * Next() const { return NextFactory; }

private:
    const char * Tag;
    AClassMeta * Classes;
    mutable AClassMeta ** IdTable;
    mutable THash<> NameTable;
    uint64_t NumClasses;
    AObjectFactory * NextFactory;
    static AObjectFactory * FactoryList;
};

class ANGIE_API AClassMeta {
    AN_FORBID_COPY( AClassMeta )

    friend class AObjectFactory;
    friend class AAttributeMeta;
    friend class APrecacheMeta;

public:
    const uint64_t ClassId;

    const char * GetName() const { return ClassName; }
    uint64_t GetId() const { return ClassId; }
    AClassMeta const * SuperClass() const { return pSuperClass; }
    AClassMeta const * Next() const { return pNext; }
    AObjectFactory const * Factory() const { return pFactory; }
    AAttributeMeta const * GetAttribList() const { return AttributesHead; }
    APrecacheMeta const * GetPrecacheList() const { return PrecacheHead; }

    bool IsSubclassOf( AClassMeta const & _Superclass ) const {
        for ( AClassMeta const * meta = this ; meta ; meta = meta->SuperClass() ) {
            if ( meta->GetId() == _Superclass.GetId() ) {
                return true;
            }
        }
        return false;
    }

    template< typename _Superclass >
    bool IsSubclassOf() const {
        return IsSubclassOf( _Superclass::ClassMeta() );
    }

    // TODO: class flags?

    virtual ADummy * CreateInstance() const = 0;
    virtual void DestroyInstance( ADummy * _Object ) const = 0;

    static void CloneAttributes( ADummy const * _Template, ADummy * _Destination );

    static AObjectFactory & DummyFactory() { static AObjectFactory ObjectFactory( "Dummy factory" ); return ObjectFactory; }

    // Utilites
    AAttributeMeta const * FindAttribute( const char * _Name, bool _Recursive ) const;
    void GetAttributes( TPodArray< AAttributeMeta const * > & _Attributes, bool _Recursive = true ) const;

protected:
    AClassMeta( AObjectFactory & _Factory, const char * _ClassName, AClassMeta const * _SuperClassMeta )
        : ClassId( _Factory.NumClasses + 1 ), ClassName( _ClassName )
    {
        AN_ASSERT_( _Factory.FindClass( _ClassName ) == NULL, "Class already defined" );
        pNext = _Factory.Classes;
        pSuperClass = _SuperClassMeta;
        AttributesHead = nullptr;
        AttributesTail = nullptr;
        PrecacheHead = nullptr;
        PrecacheTail = nullptr;
        _Factory.Classes = this;
        _Factory.NumClasses++;
        pFactory = &_Factory;
    }

private:
    const char * ClassName;
    AClassMeta * pNext;
    AClassMeta const * pSuperClass;
    AObjectFactory const * pFactory;
    AAttributeMeta const * AttributesHead;
    AAttributeMeta const * AttributesTail;
    APrecacheMeta const * PrecacheHead;
    APrecacheMeta const * PrecacheTail;
};

AN_FORCEINLINE ADummy * AObjectFactory::CreateInstance( const char * _ClassName ) const {
    AClassMeta const * classMeta = LookupClass( _ClassName );
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

AN_FORCEINLINE ADummy * AObjectFactory::CreateInstance( uint64_t _ClassId ) const {
    AClassMeta const * classMeta = LookupClass( _ClassId );
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

AN_FORCEINLINE AClassMeta const * AObjectFactory::GetClassList() const {
    return Classes;
}

enum class EAttributeType {
    T_Byte,
    T_Bool,
    T_Int,
    T_Float,
    T_Float2,
    T_Float3,
    T_Float4,
    T_Quat,
    T_String,

    T_Max
};

template< typename T >
/*AN_FORCEINLINE*/ EAttributeType GetAttributeType();

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< byte >() { return EAttributeType::T_Byte; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< bool >() { return EAttributeType::T_Bool; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< int >() { return EAttributeType::T_Int; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< float >() { return EAttributeType::T_Float; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< Float2 >() { return EAttributeType::T_Float2; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< Float3 >() { return EAttributeType::T_Float3; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< Float4 >() { return EAttributeType::T_Float4; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< Quat >() { return EAttributeType::T_Quat; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< AString const & >() { return EAttributeType::T_String; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< AString >() { return EAttributeType::T_String; }

AN_FORCEINLINE AString AttrToString( byte const & v ) { return Math::ToString( v ); }
AN_FORCEINLINE AString AttrToString( bool const & v ) { return Math::ToString( v ); }
AN_FORCEINLINE AString AttrToString( int const & v ) { return Math::ToString( v ); }
AN_FORCEINLINE AString AttrToString( float const & v ) { return Math::ToString( *( (int *)&v ) ); }
AN_FORCEINLINE AString AttrToString( Float2 const & v ) { return Math::ToString( *( (int *)&v.X ) ) + " " + Math::ToString( *( (int *)&v.Y ) ); }
AN_FORCEINLINE AString AttrToString( Float3 const & v ) { return Math::ToString( *( (int *)&v.X ) ) + " " + Math::ToString( *( (int *)&v.Y ) ) + " " + Math::ToString( *( (int *)&v.Z ) ); }
AN_FORCEINLINE AString AttrToString( Float4 const & v ) { return Math::ToString( *( (int *)&v.X ) ) + " " + Math::ToString( *( (int *)&v.Y ) ) + " " + Math::ToString( *( (int *)&v.Z ) ) + " " + Math::ToString( *( (int *)&v.W ) ); }
AN_FORCEINLINE AString AttrToString( Quat const & v ) { return Math::ToString( *( (int *)&v.X ) ) + " " + Math::ToString( *( (int *)&v.Y ) ) + " " + Math::ToString( *( (int *)&v.Z ) ) + " " + Math::ToString( *( (int *)&v.W ) ); }
AN_FORCEINLINE AString AttrToString( AString const & v ) { return v; }

template< typename T >
/*AN_FORCEINLINE*/ T AttrFromString( AString const & v );

template<>
AN_FORCEINLINE byte AttrFromString< byte >( AString const & v ) {
    return Math::ToInt< uint8_t >( v );
}

template<>
AN_FORCEINLINE bool AttrFromString< bool >( AString const & v ) {
    return !!Math::ToInt< uint8_t >( v );
}

template<>
AN_FORCEINLINE int AttrFromString< int >( AString const & v ) {
    return Math::ToInt< int32_t >( v );
}

template<>
AN_FORCEINLINE float AttrFromString< float >( AString const & v ) {
    int i = Math::ToInt< int32_t >( v );
    return *( float * )&i;
}

template<>
AN_FORCEINLINE Float2 AttrFromString< Float2 >( AString const & v ) {
    Float2 val;
    int tmp[2];
    sscanf( v.CStr(), "%d %d", &tmp[0], &tmp[1] );
    for ( int i = 0 ; i < 2 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE Float3 AttrFromString< Float3 >( AString const & v ) {
    Float3 val;
    int tmp[3];
    sscanf( v.CStr(), "%d %d %d", &tmp[0], &tmp[1], &tmp[2] );
    for ( int i = 0 ; i < 3 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE Float4 AttrFromString< Float4 >( AString const & v ) {
    Float4 val;
    int tmp[4];
    sscanf( v.CStr(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3] );
    for ( int i = 0 ; i < 4 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE Quat AttrFromString< Quat >( AString const & v ) {
    Quat val;
    int tmp[4];
    sscanf( v.CStr(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3] );
    for ( int i = 0 ; i < 4 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE AString AttrFromString< AString >( AString const & v ) {
    return v;
}

class ANGIE_API AAttributeMeta {
    AN_FORBID_COPY( AAttributeMeta )

public:
    const char * GetName() const { return Name; }
    EAttributeType GetType() const { return Type; }
    const char * GetTypeName() const { return TypeNames[ (int)Type ]; }
    uint32_t GetFlags() const { return Flags; }

    AAttributeMeta const * Next() const { return pNext; }
    AAttributeMeta const * Prev() const { return pPrev; }

    AAttributeMeta( AClassMeta const & _ClassMeta, const char * _Name, EAttributeType _Type, uint32_t _Flags )
        : Name( _Name )
        , Type( _Type )
        , Flags( _Flags )
    {
        AClassMeta & classMeta = const_cast< AClassMeta & >( _ClassMeta );
        pNext = nullptr;
        pPrev = classMeta.AttributesTail;
        if ( pPrev ) {
            const_cast< AAttributeMeta * >( pPrev )->pNext = this;
        } else {
            classMeta.AttributesHead = this;
        }
        classMeta.AttributesTail = this;
    }

    // TODO: Min/Max range for integer or float attributes, support for enums

//    void CallSetter( ADummy * _Object, const void * _Value ) const {
//        if ( Setter ) {
//            Setter( _Object, _Value );
//        }
//    }

//    void CallGetter( ADummy * _Object, void * _Value ) const {
//        if ( Getter ) {
//            Getter( _Object, _Value );
//        }
//    }

    void SetValue( ADummy * _Object, AString const & _Value ) const {
        FromString( _Object, _Value );
    }

    void GetValue( ADummy * _Object, AString & _Value ) const {
        ToString( _Object, _Value );
    }

    void CopyValue( ADummy const * _Src, ADummy * _Dst ) const {
        Copy( _Src, _Dst );
    }

    byte GetByteValue( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< byte >( s );
    }

    bool GetBoolValue( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< bool >( s );
    }

    int GetIntValue( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< int >( s );
    }

    float GetFloatValue( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< float >( s );
    }

    Float2 GetFloat2Value( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< Float2 >( s );
    }

    Float3 GetFloat3Value( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< Float3 >( s );
    }

    Float4 GetFloat4Value( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< Float4 >( s );
    }

    Quat GetQuatValue( ADummy * _Object ) const {
        AString s;
        ToString( _Object, s );
        return AttrFromString< Quat >( s );
    }

private:
    const char * Name;
    EAttributeType Type;
    AAttributeMeta const * pNext;
    AAttributeMeta const * pPrev;
    uint32_t Flags;

protected:
//    TStdFunction< void( ADummy *, const void * ) > Setter;
//    TStdFunction< void( ADummy *, void * ) > Getter;
    TStdFunction< void( ADummy *, AString const & ) > FromString;
    TStdFunction< void( ADummy *, AString & ) > ToString;
    TStdFunction< void( ADummy const *, ADummy * ) > Copy;

    static const char * TypeNames[ (int)EAttributeType::T_Max ];
};

class ANGIE_API APrecacheMeta {
    AN_FORBID_COPY( APrecacheMeta )

public:
    AClassMeta const & GetResourceClassMeta() const { return ResourceClassMeta; }
    const char * GetResourcePath() const { return Path; }
    int GetResourceHash() const { return Hash; }

    APrecacheMeta const * Next() const { return pNext; }
    APrecacheMeta const * Prev() const { return pPrev; }

    APrecacheMeta( AClassMeta const & _ClassMeta, AClassMeta const & _ResourceClassMeta, const char * _Path )
        : ResourceClassMeta( _ResourceClassMeta )
        , Path( _Path )
        , Hash( Core::HashCase( _Path, Core::Strlen( _Path ) ) )
    {
        AClassMeta & classMeta = const_cast< AClassMeta & >( _ClassMeta );
        pNext = nullptr;
        pPrev = classMeta.PrecacheTail;
        if ( pPrev ) {
            const_cast< APrecacheMeta * >( pPrev )->pNext = this;
        } else {
            classMeta.PrecacheHead = this;
        }
        classMeta.PrecacheTail = this;
    }

private:
    AClassMeta const & ResourceClassMeta;
    const char * Path;
    int Hash;
    APrecacheMeta const * pNext;
    APrecacheMeta const * pPrev;
};

template< typename ObjectType >
class TAttributeMeta : public AAttributeMeta {
    AN_FORBID_COPY( TAttributeMeta )

public:

    template< typename AttributeType >
    TAttributeMeta( AClassMeta const & _ClassMeta, const char * _Name,
                    void(ObjectType::*_Setter)( AttributeType ),
                    AttributeType(ObjectType::*_Getter)() const,
                    int _Flags )
        : AAttributeMeta( _ClassMeta, _Name, GetAttributeType< AttributeType >(), _Flags )
    {
//        Setter = [_Setter]( ADummy * _Object, const void * _DataPtr ) {
//            ObjectType * object = static_cast< ObjectType * >( _Object );
//            (*object.*_Setter)( *(( AttributeType const * )_DataPtr) );
//        };
//        Getter = [_Getter]( ADummy * _Object, void * _DataPtr ) {
//            ObjectType * object = static_cast< ObjectType * >( _Object );
//            AttributeType Value = (*object.*_Getter)();
//            Core::Memcpy( ( AttributeType * )_DataPtr, &Value, sizeof( Value ) );
//        };

        FromString = [_Setter]( ADummy * _Object, AString const & _Value ) {
            (*static_cast< ObjectType * >( _Object ).*_Setter)( ::AttrFromString< AString >( _Value ) );
        };
        ToString = [_Getter]( ADummy * _Object, AString & _Value ) {
            _Value = ::AttrToString( (*static_cast< ObjectType * >( _Object ).*_Getter)() );
        };
        Copy = [_Setter,_Getter]( ADummy const * _Src, ADummy * _Dst ) {
            (*static_cast< ObjectType * >( _Dst ).*_Setter)( (*static_cast< ObjectType const * >( _Src ).*_Getter)() );
        };
    }

    template< typename AttributeType >
    TAttributeMeta( AClassMeta const & _ClassMeta, const char * _Name, AttributeType * _AttribPointer, int _Flags )
        : AAttributeMeta( _ClassMeta, _Name, GetAttributeType< AttributeType >(), _Flags )
    {
        FromString = [_AttribPointer]( ADummy * _Object, AString const & _Value ) {
            *( AttributeType * )( (byte *)static_cast< ObjectType * >( _Object ) + (size_t)_AttribPointer ) = ::AttrFromString< AttributeType >( _Value );
        };
        ToString = [_AttribPointer]( ADummy * _Object, AString & _Value ) {
            _Value = ::AttrToString( *( AttributeType * )( (byte *)static_cast< ObjectType * >( _Object ) + (size_t)_AttribPointer ) );
        };
        Copy = [_AttribPointer]( ADummy const * _Src, ADummy * _Dst ) {
            *( AttributeType * )( (byte *)static_cast< ObjectType * >( _Dst ) + (size_t)_AttribPointer ) = *( AttributeType const * )( (byte const *)static_cast< ObjectType const * >( _Src ) + (size_t)_AttribPointer );
        };
    }
};

#define _AN_GENERATED_CLASS_BODY() \
public: \
    static AClassMeta const & ClassMeta() { \
        static ThisClassMeta __Meta; \
        return __Meta; \
    }\
    static AClassMeta const * SuperClass() { \
        return ClassMeta().SuperClass(); \
    } \
    static const char * ClassName() { \
        return ClassMeta().GetName(); \
    } \
    static uint64_t ClassId() { \
        return ClassMeta().GetId(); \
    } \
    virtual AClassMeta const & FinalClassMeta() const { return ClassMeta(); } \
    virtual const char * FinalClassName() const { return ClassName(); } \
    virtual uint64_t FinalClassId() const { return ClassId(); }


#define AN_CLASS( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( AClassMeta::DummyFactory(), _Class, _SuperClass )

#define AN_FACTORY_CLASS( _Factory, _Class, _SuperClass ) \
    AN_FACTORY_CLASS_A( _Factory, _Class, _SuperClass, ADummy::Allocator )

#define AN_FACTORY_CLASS_A( _Factory, _Class, _SuperClass, _Allocator ) \
    AN_FORBID_COPY( _Class ) \
    friend class ADummy; \
public:\
    typedef _SuperClass Super; \
    typedef _Class ThisClass; \
    typedef _Allocator Allocator; \
    class ThisClassMeta : public AClassMeta { \
    public: \
        ThisClassMeta() : AClassMeta( _Factory, AN_STRINGIFY( _Class ), &Super::ClassMeta() ) { \
            RegisterAttributes(); \
        } \
        ADummy * CreateInstance() const override { \
            return NewObject< ThisClass >(); \
        } \
        void DestroyInstance( ADummy * _Object ) const override { \
            _Object->~ADummy(); \
            _Allocator::Inst().Free( _Object ); \
        } \
    private: \
        void RegisterAttributes(); \
    }; \
    _AN_GENERATED_CLASS_BODY() \
private:



#define AN_BEGIN_CLASS_META( _Class ) \
AClassMeta const & _Class##__Meta = _Class::ClassMeta(); \
void _Class::ThisClassMeta::RegisterAttributes() {

#define AN_END_CLASS_META() \
    }

#define AN_CLASS_META( _Class ) AN_BEGIN_CLASS_META( _Class ) AN_END_CLASS_META()

#define AN_ATTRIBUTE( _Name, _Setter, _Getter, _Flags ) \
    static TAttributeMeta< ThisClass > const _Name##Meta( *this, AN_STRINGIFY( _Name ), &ThisClass::_Setter, &ThisClass::_Getter, _Flags );

#define AN_ATTRIBUTE_( _Name, _Flags ) \
    static TAttributeMeta< ThisClass > const _Name##Meta( *this, AN_STRINGIFY( _Name ), (&(( ThisClass * )0)->_Name), _Flags );

#define AN_PRECACHE( _ResourceClass, _ResourceName, _Path ) \
    static APrecacheMeta const _ResourceName##Precache( *this, _ResourceClass::ClassMeta(), _Path );

/* Attribute flags */
#define AF_DEFAULT              0
#define AF_NON_SERIALIZABLE     1

/*

ADummy

Base factory object class.
Needs to resolve class meta data.

*/
class ANGIE_API ADummy {
    AN_FORBID_COPY( ADummy )

public:
    typedef ADummy ThisClass;
    typedef AZoneAllocator Allocator;
    class ThisClassMeta : public AClassMeta {
    public:
        ThisClassMeta() : AClassMeta( AClassMeta::DummyFactory(), "ADummy", nullptr ) {
        }
        ADummy * CreateInstance() const override {
            return NewObject< ThisClass >();
        }
        void DestroyInstance( ADummy * _Object ) const override {
            _Object->~ADummy();
            Allocator::Inst().Free( _Object );
        }
    private:
        void RegisterAttributes();
    };
    template< typename T >
    static T * NewObject() {
        void * data = T::Allocator::Inst().ClearedAlloc( sizeof( T ) );
        ADummy * object = new (data) T;
        return static_cast< T * >( object );
    }
    virtual ~ADummy() {}
protected:
    ADummy() {}
    _AN_GENERATED_CLASS_BODY()
};

template< typename T > T * CreateInstanceOf() {
    return static_cast< T * >( T::ClassMeta().CreateInstance() );
}

template< typename T >
T * Upcast( ADummy * _Object ) {
    if ( _Object && _Object->FinalClassMeta().IsSubclassOf< T >() ) {
        return static_cast< T * >( _Object );
    }
    return nullptr;
}

void InitializeFactories();
void DeinitializeFactories();
