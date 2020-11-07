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

#include <World/Public/Base/BaseObject.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

AN_BEGIN_CLASS_META( ABaseObject )
//AN_ATTRIBUTE( Name, SetName, GetName, AF_DEFAULT )
AN_END_CLASS_META()

uint64_t ABaseObject::TotalObjects = 0;

ABaseObject * ABaseObject::Objects = nullptr;
ABaseObject * ABaseObject::ObjectsTail = nullptr;

static uint64_t GUniqueIdGenerator = 0;

ABaseObject::ABaseObject() : Id(++GUniqueIdGenerator)
{
    INTRUSIVE_ADD( this, NextObject, PrevObject, Objects, ObjectsTail );
    TotalObjects++;
    AGarbageCollector::AddObject( this );
}

ABaseObject::~ABaseObject() {
    INTRUSIVE_REMOVE( this, NextObject, PrevObject, Objects, ObjectsTail );

    TotalObjects--;

    if ( WeakRefCounter ) {
        WeakRefCounter->Object = nullptr;
    }
}

void ABaseObject::AddRef() {
    AN_ASSERT_( RefCount != -666, "Calling AddRef() in destructor" );
    ++RefCount;
    if ( RefCount == 1 ) {
        AGarbageCollector::RemoveObject( this );
    }
}

void ABaseObject::RemoveRef() {
    AN_ASSERT_( RefCount != -666, "Calling RemoveRef() in destructor" );
    if ( --RefCount == 0 ) {
        AGarbageCollector::AddObject( this );
        return;
    }
    AN_ASSERT( RefCount > 0 );
}

ABaseObject * ABaseObject::FindObject( uint64_t _Id ) {
    if ( !_Id ) {
        return nullptr;
    }

    // FIXME: use hash/map?
    for ( ABaseObject * object = Objects ; object ; object = object->NextObject ) {
        if ( object->Id == _Id ) {
            return object;
        }
    }
    return nullptr;
}

TRef< ADocObject > ABaseObject::Serialize() {
    TRef< ADocObject > object = MakeRef< ADocObject >();

    object->AddString( "ClassName", FinalClassName() );
    object->AddString( "ObjectName", GetObjectName() );

    for ( AClassMeta const * Meta = &FinalClassMeta()
          ; Meta ; Meta = Meta->SuperClass() ) {

        AAttributeMeta const * attribs = Meta->GetAttribList();
        if ( attribs ) {
            ADocMember * attribArray = nullptr;

            for ( AAttributeMeta const * attr = attribs ; attr ; attr = attr->Next() ) {

                if ( attr->GetFlags() & AF_NON_SERIALIZABLE ) {
                    continue;
                }

                attribArray = ( attribArray == nullptr ) ? object->AddArray( Meta->GetName() ) : attribArray;

                TRef< ADocObject > attribObject = MakeRef< ADocObject >();

                AString s;
                attr->GetValue( this, s );

                attribObject->AddString( attr->GetName(), s );

                attribArray->AddValue( attribObject );
            }
        }
    }

    return object;
}

void ABaseObject::LoadAttributes_r( AClassMeta const * Meta, ADocValue const * pObject ) {
    if ( Meta ) {
        LoadAttributes_r( Meta->SuperClass(), pObject );

        for ( AAttributeMeta const * attrib = Meta->GetAttribList() ; attrib ; attrib = attrib->Next() ) {
            ADocMember const * attributeField = pObject->FindMember( attrib->GetName() );
            if ( attributeField ) {
                attrib->SetValue( this, attributeField->GetString() );
            }
        }
    }
}

void ABaseObject::LoadAttributes( ADocValue const * pObject ) {
    LoadAttributes_r( &FinalClassMeta(), pObject );
}

void ABaseObject::SetAttributes_r( AClassMeta const * Meta, THash<> const & AttributeHash, TStdVector< std::pair< AString, AString > > const & Attributes ) {
    if ( Meta ) {
        SetAttributes_r( Meta->SuperClass(), AttributeHash, Attributes );

        for ( AAttributeMeta const * attrib = Meta->GetAttribList() ; attrib ; attrib = attrib->Next() ) {

            int hash = attrib->GetNameHash();

            for ( int i = AttributeHash.First( hash ) ; i != -1 ; i = AttributeHash.Next( i ) ) {
                if ( !Attributes[i].first.Icmp( attrib->GetName() ) ) {
                    // Attribute found
                    attrib->SetValue( this, Attributes[i].second );

                    //attrib->SetSubmember( this, AttributeHash, Attributes );
                    break;
                }
            }
        }
    }
}

void ABaseObject::SetAttributes( THash<> const & AttributeHash, TStdVector< std::pair< AString, AString > > const & Attributes ) {
    if ( Attributes.IsEmpty() ) {
        return;
    }
    SetAttributes_r( &FinalClassMeta(), AttributeHash, Attributes );
}

ABaseObject * AGarbageCollector::GarbageObjects = nullptr;
ABaseObject * AGarbageCollector::GarbageObjectsTail = nullptr;

void AGarbageCollector::Initialize() {

}

void AGarbageCollector::Deinitialize() {
    DeallocateObjects();
}

void AGarbageCollector::AddObject( ABaseObject * _Object ) {
    INTRUSIVE_ADD( _Object, NextGarbageObject, PrevGarbageObject, GarbageObjects, GarbageObjectsTail )
}

void AGarbageCollector::RemoveObject( ABaseObject * _Object ) {
    INTRUSIVE_REMOVE( _Object, NextGarbageObject, PrevGarbageObject, GarbageObjects, GarbageObjectsTail )
}

void AGarbageCollector::DeallocateObjects() {
#if 0
    while ( GarbageObjects ) {
        ABaseObject * object = GarbageObjects;
        ABaseObject * nextObject;

        GarbageObjects = nullptr;

        while ( object ) {
            nextObject = object->NextGarbageObject;
            const AClassMeta & classMeta = object->FinalClassMeta();
            classMeta.DestroyInstance( object );
            object = nextObject;
        }
    }
#endif
    while ( GarbageObjects ) {
        ABaseObject * object = GarbageObjects;

        // Mark RefCount to prevent using of AddRef/RemoveRef in the object destructor
        object->RefCount = -666;

        RemoveObject( object );

        const AClassMeta & classMeta = object->FinalClassMeta();
        classMeta.DestroyInstance( object );
    }

    GarbageObjectsTail = nullptr;

    //GPrintf( "TotalObjects: %d\n", ABaseObject::GetTotalObjects() );
}
