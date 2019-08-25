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

#include <Engine/Base/Public/Factory.h>
#include <Engine/Core/Public/Alloc.h>
#include <Engine/Core/Public/HashFunc.h>
#include <Engine/Core/Public/Logger.h>

AN_CLASS_META( FDummy )

const char * FAttributeMeta::TypeNames[ (int)EAttributeType::T_Max ] =
{
    "Byte", "Bool", "Int", "Float", "Float2", "Float3", "Float4", "Quat", "String"
};

FObjectFactory * FObjectFactory::FactoryList = nullptr;

void InitializeFactories() {
    //for ( FObjectFactory * factory = FObjectFactory::FactoryList ; factory = factory->NextFactory ) {
    //    ... do something
    //}
}

void DeinitializeFactories() {
    for ( FObjectFactory * factory = FObjectFactory::FactoryList ; factory ; factory = factory->NextFactory ) {
        GMainMemoryZone.Dealloc( factory->IdTable );
        factory->IdTable = nullptr;
        factory->NameTable.Free();
    }
}

FObjectFactory::FObjectFactory( const char * _Tag )
    : Tag( _Tag ), Classes( nullptr ), IdTable( nullptr ), NumClasses( 0 )
{
    NextFactory = FactoryList;
    FactoryList = this;
}

FObjectFactory::~FObjectFactory() {
    AN_Assert( IdTable == nullptr );
}

const FClassMeta * FObjectFactory::FindClass( const char * _ClassName ) const {
    if ( !*_ClassName ) {
        return NULL;
    }
    for ( FClassMeta const * n = Classes ; n ; n = n->pNext ) {
        if ( !FString::Cmp( n->GetName(), _ClassName ) ) {
            return n;
        }
    }
    return NULL;
}

const FClassMeta * FObjectFactory::LookupClass( const char * _ClassName ) const {
    if ( !NameTable.IsAllocated() ) {
        // init name table
        for ( FClassMeta * n = Classes ; n ; n = n->pNext ) {
            NameTable.Insert( FCore::Hash( n->GetName(), FString::Length( n->GetName() ) ), n->GetId() );
        }
    }

    int i = NameTable.First( FCore::Hash( _ClassName, FString::Length( _ClassName ) ) );
    for ( ; i != -1 ; i = NameTable.Next( i ) ) {
        FClassMeta const * classMeta = LookupClass( i );
        if ( classMeta && !FString::Cmp( classMeta->GetName(), _ClassName ) ) {
             return classMeta;
        }
    }

    return nullptr;
}

const FClassMeta * FObjectFactory::LookupClass( uint64_t _ClassId ) const {
    if ( _ClassId > NumClasses ) {
        // invalid class id
        return nullptr;
    }

    if ( !IdTable ) {
        // init lookup table
        IdTable = ( FClassMeta ** )GMainMemoryZone.Alloc( ( NumClasses + 1 ) * sizeof( *IdTable ), 1 );
        IdTable[ 0 ] = nullptr;
        for ( FClassMeta * n = Classes ; n ; n = n->pNext ) {
            IdTable[ n->GetId() ] = n;
        }
    }

    return IdTable[ _ClassId ];
}

FAttributeMeta const * FClassMeta::FindAttribute( const char * _Name, bool _Recursive ) const {
    for ( FAttributeMeta const * attrib = AttributesHead ; attrib ; attrib = attrib->Next() ) {
        if ( !FString::Cmp( attrib->GetName(), _Name ) ) {
            return attrib;
        }
    }
    if ( _Recursive && pSuperClass ) {
        return pSuperClass->FindAttribute( _Name, true );
    }
    return nullptr;
}

void FClassMeta::GetAttributes( TPodArray< FAttributeMeta const * > & _Attributes, bool _Recursive ) const {
    for ( FAttributeMeta const * attrib = AttributesHead ; attrib ; attrib = attrib->Next() ) {
        _Attributes.Append( attrib );
    }
    if ( _Recursive && pSuperClass ) {
        pSuperClass->GetAttributes( _Attributes, true );
    }
}

void FClassMeta::CloneAttributes( FDummy const * _Template, FDummy * _Destination ) {
    if ( &_Template->FinalClassMeta() != &_Destination->FinalClassMeta() ) {
        GLogger.Printf( "FClassMeta::CloneAttributes: Template is not an %s class\n", _Destination->FinalClassName() );
        return;
    }
    for ( FClassMeta const * Meta = &_Template->FinalClassMeta() ; Meta ; Meta = Meta->SuperClass() ) {
        for ( FAttributeMeta const * attr = Meta->GetAttribList() ; attr ; attr = attr->Next() ) {
            attr->CopyValue( _Template, _Destination );
        }
    }
}
