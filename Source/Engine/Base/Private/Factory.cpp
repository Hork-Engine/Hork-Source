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

AN_CLASS_META( ADummy )

const char * AAttributeMeta::TypeNames[ (int)EAttributeType::T_Max ] =
{
    "Byte", "Bool", "Int", "Float", "Float2", "Float3", "Float4", "Quat", "String"
};

AObjectFactory * AObjectFactory::FactoryList = nullptr;

void InitializeFactories() {
    //for ( AObjectFactory * factory = AObjectFactory::FactoryList ; factory = factory->NextFactory ) {
    //    ... do something
    //}
}

void DeinitializeFactories() {
    for ( AObjectFactory * factory = AObjectFactory::FactoryList ; factory ; factory = factory->NextFactory ) {
        GZoneMemory.Dealloc( factory->IdTable );
        factory->IdTable = nullptr;
        factory->NameTable.Free();
    }
}

AObjectFactory::AObjectFactory( const char * _Tag )
    : Tag( _Tag ), Classes( nullptr ), IdTable( nullptr ), NumClasses( 0 )
{
    NextFactory = FactoryList;
    FactoryList = this;
}

AObjectFactory::~AObjectFactory() {
    AN_Assert( IdTable == nullptr );
}

const AClassMeta * AObjectFactory::FindClass( const char * _ClassName ) const {
    if ( !*_ClassName ) {
        return NULL;
    }
    for ( AClassMeta const * n = Classes ; n ; n = n->pNext ) {
        if ( !AString::Cmp( n->GetName(), _ClassName ) ) {
            return n;
        }
    }
    return NULL;
}

const AClassMeta * AObjectFactory::LookupClass( const char * _ClassName ) const {
    if ( !NameTable.IsAllocated() ) {
        // init name table
        for ( AClassMeta * n = Classes ; n ; n = n->pNext ) {
            NameTable.Insert( Core::Hash( n->GetName(), AString::Length( n->GetName() ) ), n->GetId() );
        }
    }

    int i = NameTable.First( Core::Hash( _ClassName, AString::Length( _ClassName ) ) );
    for ( ; i != -1 ; i = NameTable.Next( i ) ) {
        AClassMeta const * classMeta = LookupClass( i );
        if ( classMeta && !AString::Cmp( classMeta->GetName(), _ClassName ) ) {
             return classMeta;
        }
    }

    return nullptr;
}

const AClassMeta * AObjectFactory::LookupClass( uint64_t _ClassId ) const {
    if ( _ClassId > NumClasses ) {
        // invalid class id
        return nullptr;
    }

    if ( !IdTable ) {
        // init lookup table
        IdTable = ( AClassMeta ** )GZoneMemory.Alloc( ( NumClasses + 1 ) * sizeof( *IdTable ), 1 );
        IdTable[ 0 ] = nullptr;
        for ( AClassMeta * n = Classes ; n ; n = n->pNext ) {
            IdTable[ n->GetId() ] = n;
        }
    }

    return IdTable[ _ClassId ];
}

AAttributeMeta const * AClassMeta::FindAttribute( const char * _Name, bool _Recursive ) const {
    for ( AAttributeMeta const * attrib = AttributesHead ; attrib ; attrib = attrib->Next() ) {
        if ( !AString::Cmp( attrib->GetName(), _Name ) ) {
            return attrib;
        }
    }
    if ( _Recursive && pSuperClass ) {
        return pSuperClass->FindAttribute( _Name, true );
    }
    return nullptr;
}

void AClassMeta::GetAttributes( TPodArray< AAttributeMeta const * > & _Attributes, bool _Recursive ) const {
    for ( AAttributeMeta const * attrib = AttributesHead ; attrib ; attrib = attrib->Next() ) {
        _Attributes.Append( attrib );
    }
    if ( _Recursive && pSuperClass ) {
        pSuperClass->GetAttributes( _Attributes, true );
    }
}

void AClassMeta::CloneAttributes( ADummy const * _Template, ADummy * _Destination ) {
    if ( &_Template->FinalClassMeta() != &_Destination->FinalClassMeta() ) {
        GLogger.Printf( "AClassMeta::CloneAttributes: Template is not an %s class\n", _Destination->FinalClassName() );
        return;
    }
    for ( AClassMeta const * Meta = &_Template->FinalClassMeta() ; Meta ; Meta = Meta->SuperClass() ) {
        for ( AAttributeMeta const * attr = Meta->GetAttribList() ; attr ; attr = attr->Next() ) {
            attr->CopyValue( _Template, _Destination );
        }
    }
}
