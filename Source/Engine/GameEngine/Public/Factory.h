/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/Hash.h>
#include <Engine/Core/Public/String.h>
#include <Engine/Core/Public/Math.h>
#include <Engine/Core/Public/PodArray.h>

class FClassMeta;
class FAttributeMeta;
class FPrecacheMeta;
class FDummy;

class ANGIE_API FObjectFactory {
    AN_FORBID_COPY( FObjectFactory )

    friend class FClassMeta;
    friend void InitializeFactories();
    friend void DeinitializeFactories();

public:
    FObjectFactory( const char * _Tag );
    ~FObjectFactory();

    const char * GetTag() const { return Tag; }

    FDummy * CreateInstance( const char * _ClassName ) const;
    FDummy * CreateInstance( uint64_t _ClassId ) const;

    template< typename T >
    T * CreateInstance() const { return static_cast< T * >( CreateInstance( T::ClassId() ) ); }

    FClassMeta const * GetClassList() const;

    FClassMeta const * FindClass( const char * _ClassName ) const;

    FClassMeta const * LookupClass( const char * _ClassName ) const;
    FClassMeta const * LookupClass( uint64_t _ClassId ) const;

    uint64_t FactoryClassCount() const { return NumClasses; }

    static FObjectFactory const * Factories() { return FactoryList; }
    FObjectFactory const * Next() const { return NextFactory; }

private:
    const char * Tag;
    FClassMeta * Classes;
    mutable FClassMeta ** IdTable;
    mutable THash<> NameTable;
    uint64_t NumClasses;
    FObjectFactory * NextFactory;
    static FObjectFactory * FactoryList;
};

class ANGIE_API FClassMeta {
    AN_FORBID_COPY( FClassMeta )

    friend class FObjectFactory;
    friend class FAttributeMeta;
    friend class FPrecacheMeta;

public:
    const char * GetName() const { return ClassName; }
    uint64_t GetId() const { return ClassId; }
    FClassMeta const * SuperClass() const { return pSuperClass; }
    FClassMeta const * Next() const { return pNext; }
    FObjectFactory const * Factory() const { return pFactory; }
    FAttributeMeta const * GetAttribList() const { return AttributesHead; }
    FPrecacheMeta const * GetPrecacheList() const { return PrecacheHead; }

    bool IsSubclassOf( FClassMeta const & _Superclass ) const {
        for ( FClassMeta const * meta = this ; meta ; meta = meta->SuperClass() ) {
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

    virtual FDummy * CreateInstance() const = 0;
    virtual void DestroyInstance( FDummy * _Object ) const = 0;

    static void CloneAttributes( FDummy const * _Template, FDummy * _Destination );

    static FObjectFactory & DummyFactory() { static FObjectFactory ObjectFactory( "Dummy factory" ); return ObjectFactory; }

    // Utilites
    FAttributeMeta const * FindAttribute( const char * _Name, bool _Recursive ) const;
    void GetAttributes( TPodArray< FAttributeMeta const * > & _Attributes, bool _Recursive = true ) const;

protected:
    FClassMeta( FObjectFactory & _Factory, const char * _ClassName, FClassMeta const * _SuperClassMeta )
        : ClassName( _ClassName )
    {
        AN_ASSERT( _Factory.FindClass( _ClassName ) == NULL, "Class already defined" );
        ClassId = _Factory.NumClasses + 1;
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
    uint64_t ClassId;
    FClassMeta * pNext;
    FClassMeta const * pSuperClass;
    FObjectFactory const * pFactory;
    FAttributeMeta const * AttributesHead;
    FAttributeMeta const * AttributesTail;
    FPrecacheMeta const * PrecacheHead;
    FPrecacheMeta const * PrecacheTail;
};

AN_FORCEINLINE FDummy * FObjectFactory::CreateInstance( const char * _ClassName ) const {
    FClassMeta const * classMeta = LookupClass( _ClassName );
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

AN_FORCEINLINE FDummy * FObjectFactory::CreateInstance( uint64_t _ClassId ) const {
    FClassMeta const * classMeta = LookupClass( _ClassId );
    return classMeta ? classMeta->CreateInstance() : nullptr;
}

AN_FORCEINLINE FClassMeta const * FObjectFactory::GetClassList() const {
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
AN_FORCEINLINE EAttributeType GetAttributeType< FString const & >() { return EAttributeType::T_String; }

template<>
AN_FORCEINLINE EAttributeType GetAttributeType< FString >() { return EAttributeType::T_String; }

AN_FORCEINLINE FString AttrToString( byte const & v ) { return Byte( v ).ToString(); }
AN_FORCEINLINE FString AttrToString( bool const & v ) { return Byte( v ).ToString(); }
AN_FORCEINLINE FString AttrToString( int const & v ) { return Int( v ).ToString(); }
AN_FORCEINLINE FString AttrToString( float const & v ) { return Int( *( (int *)&v ) ).ToString(); }
AN_FORCEINLINE FString AttrToString( Float2 const & v ) { return Int( *( (int *)&v.X ) ).ToString() + " " + Int( *( (int *)&v.Y ) ).ToString(); }
AN_FORCEINLINE FString AttrToString( Float3 const & v ) { return Int( *( (int *)&v.X ) ).ToString() + " " + Int( *( (int *)&v.Y ) ).ToString() + " " + Int( *( (int *)&v.Z ) ).ToString(); }
AN_FORCEINLINE FString AttrToString( Float4 const & v ) { return Int( *( (int *)&v.X ) ).ToString() + " " + Int( *( (int *)&v.Y ) ).ToString() + " " + Int( *( (int *)&v.Z ) ).ToString() + " " + Int( *( (int *)&v.W ) ).ToString(); }
AN_FORCEINLINE FString AttrToString( Quat const & v ) { return Int( *( (int *)&v.X ) ).ToString() + " " + Int( *( (int *)&v.Y ) ).ToString() + " " + Int( *( (int *)&v.Z ) ).ToString() + " " + Int( *( (int *)&v.W ) ).ToString(); }
AN_FORCEINLINE FString AttrToString( FString const & v ) { return v; }

template< typename T >
/*AN_FORCEINLINE*/ T AttrFromString( FString const & v );

template<>
AN_FORCEINLINE byte AttrFromString< byte >( FString const & v ) {
    return Byte().FromString( v );
}

template<>
AN_FORCEINLINE bool AttrFromString< bool >( FString const & v ) {
    return Bool().FromString( v );
}

template<>
AN_FORCEINLINE int AttrFromString< int >( FString const & v ) {
    return Int().FromString( v );
}

template<>
AN_FORCEINLINE float AttrFromString< float >( FString const & v ) {
    int i = Int().FromString( v );
    return *( float * )&i;
}

template<>
AN_FORCEINLINE Float2 AttrFromString< Float2 >( FString const & v ) {
    Float2 val;
    int tmp[2];
    sscanf( v.ToConstChar(), "%d %d", &tmp[0], &tmp[1] );
    for ( int i = 0 ; i < 2 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE Float3 AttrFromString< Float3 >( FString const & v ) {
    Float3 val;
    int tmp[3];
    sscanf( v.ToConstChar(), "%d %d %d", &tmp[0], &tmp[1], &tmp[2] );
    for ( int i = 0 ; i < 3 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE Float4 AttrFromString< Float4 >( FString const & v ) {
    Float4 val;
    int tmp[4];
    sscanf( v.ToConstChar(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3] );
    for ( int i = 0 ; i < 4 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE Quat AttrFromString< Quat >( FString const & v ) {
    Quat val;
    int tmp[4];
    sscanf( v.ToConstChar(), "%d %d %d %d", &tmp[0], &tmp[1], &tmp[2], &tmp[3] );
    for ( int i = 0 ; i < 4 ; i++ ) {
        val[i] = *( float * )&tmp[i];
    }
    return val;
}

template<>
AN_FORCEINLINE FString AttrFromString< FString >( FString const & v ) {
    return v;
}

ANGIE_TEMPLATE template class ANGIE_API TFunction< void( FDummy *, FString const & ) >;
ANGIE_TEMPLATE template class ANGIE_API TFunction< void( FDummy *, FString & ) >;
ANGIE_TEMPLATE template class ANGIE_API TFunction< void( FDummy const *, FDummy * ) >;

class ANGIE_API FAttributeMeta {
    AN_FORBID_COPY( FAttributeMeta )

public:
    const char * GetName() const { return Name; }
    EAttributeType GetType() const { return Type; }
    const char * GetTypeName() const { return TypeNames[ (int)Type ]; }
    uint32_t GetFlags() const { return Flags; }

    FAttributeMeta const * Next() const { return pNext; }
    FAttributeMeta const * Prev() const { return pPrev; }

    FAttributeMeta( FClassMeta const & _ClassMeta, const char * _Name, EAttributeType _Type, uint32_t _Flags )
        : Name( _Name )
        , Type( _Type )
        , Flags( _Flags )
    {
        FClassMeta & classMeta = const_cast< FClassMeta & >( _ClassMeta );
        pNext = nullptr;
        pPrev = classMeta.AttributesTail;
        if ( pPrev ) {
            const_cast< FAttributeMeta * >( pPrev )->pNext = this;
        } else {
            classMeta.AttributesHead = this;
        }
        classMeta.AttributesTail = this;
    }

    // TODO: Min/Max range for integer or float attributes, support for enums

//    void CallSetter( FDummy * _Object, const void * _Value ) const {
//        if ( Setter ) {
//            Setter( _Object, _Value );
//        }
//    }

//    void CallGetter( FDummy * _Object, void * _Value ) const {
//        if ( Getter ) {
//            Getter( _Object, _Value );
//        }
//    }

    void SetValue( FDummy * _Object, FString const & _Value ) const {
        FromString( _Object, _Value );
    }

    void GetValue( FDummy * _Object, FString & _Value ) const {
        ToString( _Object, _Value );
    }

    void CopyValue( FDummy const * _Src, FDummy * _Dst ) const {
        Copy( _Src, _Dst );
    }

    byte GetByteValue( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< byte >( s );
    }

    bool GetBoolValue( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< bool >( s );
    }

    int GetIntValue( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< int >( s );
    }

    float GetFloatValue( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< float >( s );
    }

    Float2 GetFloat2Value( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< Float2 >( s );
    }

    Float3 GetFloat3Value( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< Float3 >( s );
    }

    Float4 GetFloat4Value( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< Float4 >( s );
    }

    Quat GetQuatValue( FDummy * _Object ) const {
        FString s;
        ToString( _Object, s );
        return AttrFromString< Quat >( s );
    }

private:
    const char * Name;
    EAttributeType Type;
    FAttributeMeta const * pNext;
    FAttributeMeta const * pPrev;
    uint32_t Flags;

protected:
//    TFunction< void( FDummy *, const void * ) > Setter;
//    TFunction< void( FDummy *, void * ) > Getter;
    TFunction< void( FDummy *, FString const & ) > FromString;
    TFunction< void( FDummy *, FString & ) > ToString;
    TFunction< void( FDummy const *, FDummy * ) > Copy;

    static const char * TypeNames[ (int)EAttributeType::T_Max ];
};

#include <Engine/Core/Public/HashFunc.h>
class ANGIE_API FPrecacheMeta {
    AN_FORBID_COPY( FPrecacheMeta )

public:
    FClassMeta const & GetResourceClassMeta() const { return ResourceClassMeta; }
    const char * GetResourcePath() const { return Path; }
    int GetResourceHash() const { return Hash; }

    FPrecacheMeta const * Next() const { return pNext; }
    FPrecacheMeta const * Prev() const { return pPrev; }

    FPrecacheMeta( FClassMeta const & _ClassMeta, FClassMeta const & _ResourceClassMeta, const char * _Path )
        : ResourceClassMeta( _ResourceClassMeta )
        , Path( _Path )
        , Hash( FCore::HashCase( _Path, FString::Length( _Path ) ) )
    {
        FClassMeta & classMeta = const_cast< FClassMeta & >( _ClassMeta );
        pNext = nullptr;
        pPrev = classMeta.PrecacheTail;
        if ( pPrev ) {
            const_cast< FPrecacheMeta * >( pPrev )->pNext = this;
        } else {
            classMeta.PrecacheHead = this;
        }
        classMeta.PrecacheTail = this;
    }

private:
    FClassMeta const & ResourceClassMeta;
    const char * Path;
    int Hash;
    FPrecacheMeta const * pNext;
    FPrecacheMeta const * pPrev;
};

template< typename ObjectType >
class TAttributeMeta : public FAttributeMeta {
    AN_FORBID_COPY( TAttributeMeta )

public:

    template< typename AttributeType >
    TAttributeMeta( FClassMeta const & _ClassMeta, const char * _Name,
                    void(ObjectType::*_Setter)( AttributeType ),
                    AttributeType(ObjectType::*_Getter)() const,
                    int _Flags )
        : FAttributeMeta( _ClassMeta, _Name, GetAttributeType< AttributeType >(), _Flags )
    {
//        Setter = [_Setter]( FDummy * _Object, const void * _DataPtr ) {
//            ObjectType * object = static_cast< ObjectType * >( _Object );
//            (*object.*_Setter)( *(( AttributeType const * )_DataPtr) );
//        };
//        Getter = [_Getter]( FDummy * _Object, void * _DataPtr ) {
//            ObjectType * object = static_cast< ObjectType * >( _Object );
//            AttributeType Value = (*object.*_Getter)();
//            memcpy( ( AttributeType * )_DataPtr, &Value, sizeof( Value ) );
//        };

        FromString = [_Setter]( FDummy * _Object, FString const & _Value ) {
            (*static_cast< ObjectType * >( _Object ).*_Setter)( ::AttrFromString< FString >( _Value ) );
        };
        ToString = [_Getter]( FDummy * _Object, FString & _Value ) {
            _Value = ::AttrToString( (*static_cast< ObjectType * >( _Object ).*_Getter)() );
        };
        Copy = [_Setter,_Getter]( FDummy const * _Src, FDummy * _Dst ) {
            (*static_cast< ObjectType * >( _Dst ).*_Setter)( (*static_cast< ObjectType const * >( _Src ).*_Getter)() );
        };
    }

    template< typename AttributeType >
    TAttributeMeta( FClassMeta const & _ClassMeta, const char * _Name, AttributeType * _AttribPointer, int _Flags )
        : FAttributeMeta( _ClassMeta, _Name, GetAttributeType< AttributeType >(), _Flags )
    {
        FromString = [_AttribPointer]( FDummy * _Object, FString const & _Value ) {
            *( AttributeType * )( (byte *)static_cast< ObjectType * >( _Object ) + (size_t)_AttribPointer ) = ::AttrFromString< AttributeType >( _Value );
        };
        ToString = [_AttribPointer]( FDummy * _Object, FString & _Value ) {
            _Value = ::AttrToString( *( AttributeType * )( (byte *)static_cast< ObjectType * >( _Object ) + (size_t)_AttribPointer ) );
        };
        Copy = [_AttribPointer]( FDummy const * _Src, FDummy * _Dst ) {
            *( AttributeType * )( (byte *)static_cast< ObjectType * >( _Dst ) + (size_t)_AttribPointer ) = *( AttributeType const * )( (byte const *)static_cast< ObjectType const * >( _Src ) + (size_t)_AttribPointer );
        };
    }
};

#define _AN_GENERATED_CLASS_BODY() \
public: \
    static FClassMeta const & ClassMeta() { \
        static ThisClassMeta __Meta; \
        return __Meta; \
    }\
    static FClassMeta const * SuperClass() { \
        return ClassMeta().SuperClass(); \
    } \
    static const char * ClassName() { \
        return ClassMeta().GetName(); \
    } \
    static uint64_t ClassId() { \
        return ClassMeta().GetId(); \
    } \
    virtual FClassMeta const & FinalClassMeta() const { return ClassMeta(); } \
    virtual const char * FinalClassName() const { return ClassName(); } \
    virtual uint64_t FinalClassId() const { return ClassId(); }


#define AN_CLASS( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( FClassMeta::DummyFactory(), _Class, _SuperClass )

#define AN_FACTORY_CLASS( _Factory, _Class, _SuperClass ) \
    AN_FACTORY_CLASS_A( _Factory, _Class, _SuperClass, FDummy::Allocator )

#define AN_FACTORY_CLASS_A( _Factory, _Class, _SuperClass, _Allocator ) \
    AN_FORBID_COPY( _Class ) \
    friend class FDummy; \
public:\
    typedef _SuperClass Super; \
    typedef _Class ThisClass; \
    typedef _Allocator Allocator; \
    class ThisClassMeta : public FClassMeta { \
    public: \
        ThisClassMeta() : FClassMeta( _Factory, AN_STRINGIFY( _Class ), &Super::ClassMeta() ) { \
            RegisterAttributes(); \
        } \
        FDummy * CreateInstance() const override { \
            return NewObject< ThisClass >(); \
        } \
        void DestroyInstance( FDummy * _Object ) const override { \
            _Object->~FDummy(); \
            _Allocator::Dealloc( _Object ); \
        } \
    private: \
        void RegisterAttributes(); \
    }; \
    _AN_GENERATED_CLASS_BODY() \
private:



#define AN_BEGIN_CLASS_META( _Class ) \
FClassMeta const & _Class##__Meta = _Class::ClassMeta(); \
void _Class::ThisClassMeta::RegisterAttributes() {

#define AN_END_CLASS_META() \
    }

#define AN_CLASS_META_NO_ATTRIBS( _Class ) AN_BEGIN_CLASS_META( _Class ) AN_END_CLASS_META()

#define AN_ATTRIBUTE( _Name, _Setter, _Getter, _Flags ) \
    static TAttributeMeta< ThisClass > const _Name##Meta( *this, AN_STRINGIFY( _Name ), &ThisClass::_Setter, &ThisClass::_Getter, _Flags );

#define AN_ATTRIBUTE_( _Name, _Flags ) \
    static TAttributeMeta< ThisClass > const _Name##Meta( *this, AN_STRINGIFY( _Name ), (&(( ThisClass * )0)->_Name), _Flags );

#define AN_PRECACHE( _ResourceClass, _ResourceName, _Path ) \
    static FPrecacheMeta const _ResourceName##Precache( *this, _ResourceClass::ClassMeta(), _Path );

/* Attribute flags */
#define AF_DEFAULT              0
#define AF_NON_SERIALIZABLE     1

/*

FDummy

Base factory object class.
Needs to resolve class meta data.

*/
class ANGIE_API FDummy {
    AN_FORBID_COPY( FDummy )

public:
    typedef FDummy ThisClass;
    typedef FAllocator Allocator;
    class ThisClassMeta : public FClassMeta {
    public:
        ThisClassMeta() : FClassMeta( FClassMeta::DummyFactory(), "FDummy", nullptr ) {
        }
        FDummy * CreateInstance() const override {
            return NewObject< ThisClass >();
        }
        void DestroyInstance( FDummy * _Object ) const override {
            _Object->~FDummy();
            Allocator::Dealloc( _Object );
        }
    private:
        void RegisterAttributes();
    };
    template< typename T >
    static T * NewObject() {
        void * data = T::Allocator::AllocCleared1( sizeof( T ) );
        FDummy * object = new (data) T;
        return static_cast< T * >( object );
    }
    virtual ~FDummy() {}
protected:
    FDummy() {}
    _AN_GENERATED_CLASS_BODY()
};

template< typename T > T * CreateInstanceOf() {
    return static_cast< T * >( T::ClassMeta().CreateInstance() );
}

template< typename T >
T * Upcast( FDummy * _Object ) {
    AN_Assert( _Object );
    if ( _Object->FinalClassMeta().IsSubclassOf< T >() ) {
        return static_cast< T * >( _Object );
    }
    return nullptr;
}
