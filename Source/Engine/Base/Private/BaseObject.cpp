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

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>
#include <Engine/Resource/Public/ResourceManager.h>

AN_BEGIN_CLASS_META( ABaseObject )
//AN_ATTRIBUTE( Name, SetName, GetName, AF_DEFAULT )
AN_END_CLASS_META()

uint64_t ABaseObject::TotalObjects = 0;

ABaseObject::ABaseObject() {
    TotalObjects++;
    AGarbageCollector::AddObject( this );
}

ABaseObject::~ABaseObject() {
    TotalObjects--;

    if ( WeakRefCounter ) {
        WeakRefCounter->Object = nullptr;
    }
}

void ABaseObject::AddRef() {
    AN_ASSERT( RefCount != -666, "Calling AddRef() in destructor" );
    ++RefCount;
    if ( RefCount == 1 ) {
        AGarbageCollector::RemoveObject( this );
    }
}

void ABaseObject::RemoveRef() {
    AN_ASSERT( RefCount != -666, "Calling RemoveRef() in destructor" );
    if ( RefCount == 1 ) {
        RefCount = 0;
        AGarbageCollector::AddObject( this );
    } else if ( RefCount > 0 ) {
        --RefCount;
    }
}

int ABaseObject::Serialize( ADocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "ClassName", FinalClassName() );

//    int precacheArray = -1;

    for ( AClassMeta const * Meta = &FinalClassMeta()
          ; Meta ; Meta = Meta->SuperClass() ) {

#if 0
        APrecacheMeta const * precache = Meta->GetPrecacheList();
        if ( precache ) {

            for ( ; precache ; precache = precache->Next() ) {

                precacheArray = ( precacheArray == -1 ) ? _Doc.AddArray( object, "Precache" ) : precacheArray;

                _Doc.AddValueToField( precacheArray, _Doc.CreateStringValue( precache->GetResourcePath() ) );
            }
        }
#endif

        AAttributeMeta const * attribs = Meta->GetAttribList();
        if ( attribs ) {
            int attribArray = -1;

            for ( AAttributeMeta const * attr = attribs ; attr ; attr = attr->Next() ) {

                if ( attr->GetFlags() & AF_NON_SERIALIZABLE ) {
                    continue;
                }

                attribArray = ( attribArray == -1 ) ? _Doc.AddArray( object, Meta->GetName() ) : attribArray;

                int attribObject = _Doc.CreateObjectValue();

                AString & s = _Doc.ProxyBuffer.NewString();
                attr->GetValue( this, s );

                _Doc.AddStringField( attribObject, attr->GetName(), s.CStr() );

                _Doc.AddValueToField( attribArray, attribObject );
            }
        }
    }

    return object;
}

void ABaseObject::LoadAttributes( ADocument const & _Document, int _FieldsHead ) {
    for ( AClassMeta const * meta = &FinalClassMeta() ; meta ; meta = meta->SuperClass() ) {
        SDocumentField const * field = _Document.FindField( _FieldsHead, meta->GetName() );
        if ( field ) {
            for ( int i = field->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
                SDocumentValue * attributeObject = &_Document.Values[ i ];
                if ( attributeObject->Type == SDocumentValue::T_Object ) {
                    for ( int j = attributeObject->FieldsHead ; j != -1 ; j = _Document.Fields[ j ].Next ) {
                        SDocumentField const * attributeName = &_Document.Fields[ j ];
                        AAttributeMeta const * attrMeta = meta->FindAttribute( attributeName->Name.ToString().CStr(), false );
                        if ( attrMeta ) {
                            SDocumentValue const * attributeValue = &_Document.Values[ attributeName->ValuesHead ];
                            attrMeta->SetValue( this, attributeValue->Token.ToString() );
                        }
                    }
                }
            }
        }
    }
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


void AWeakReference::ResetWeakRef( ABaseObject * _Object ) {
    ABaseObject * Cur = WeakRefCounter ? WeakRefCounter->Object : nullptr;

    if ( Cur == _Object ) {
        return;
    }

    RemoveWeakRef();

    if ( !_Object ) {
        return;
    }

    WeakRefCounter = _Object->GetWeakRefCounter();
    if ( !WeakRefCounter ) {
        WeakRefCounter = AllocateWeakRefCounter();
        WeakRefCounter->Object = _Object;
        WeakRefCounter->RefCount = 1;
        _Object->SetWeakRefCounter( WeakRefCounter );
    } else {
        WeakRefCounter->RefCount++;
    }
}

void AWeakReference::RemoveWeakRef() {
    if ( WeakRefCounter ) {
        if ( --WeakRefCounter->RefCount == 0 ) {
            if ( WeakRefCounter->Object ) {
                WeakRefCounter->Object->SetWeakRefCounter( nullptr );
            }
            DeallocateWeakRefCounter( WeakRefCounter );
        }
        WeakRefCounter = nullptr;
    }
}

SWeakRefCounter * AWeakReference::AllocateWeakRefCounter() {
    // Own allocator couldn't handle destruction of static objects :(
    return new SWeakRefCounter;
    //return ( SWeakRefCounter * )GZoneMemory.Alloc( sizeof( SWeakRefCounter ), 1 );
}

void AWeakReference::DeallocateWeakRefCounter( SWeakRefCounter * _RefCounter ) {
    // Own allocator couldn't handle destruction of static objects :(
    delete _RefCounter;
    //GZoneMemory.Dealloc( _RefCounter );
}




AN_CLASS_META( AResourceBase )

void AResourceBase::InitializeDefaultObject() {
    InitializeFromFile( GetDefaultResourcePath() );
}

void AResourceBase::InitializeFromFile( const char * _Path ) {
    if ( !AString::IcmpN( _Path, "/Default/", 9 ) ) {
        LoadInternalResource( _Path );
        return;
    }

    if ( !AString::IcmpN( _Path, "/Root/", 6 ) ) {
        _Path += 6;

        AString fileSystemPath = GResourceManager.GetRootPath() + _Path;

        if ( !LoadResource( fileSystemPath ) ) {
            InitializeDefaultObject();
        }

        return;

    } else if ( !AString::IcmpN( _Path, "/Common/", 6 ) ) {
        _Path += 1;

        if ( !LoadResource( _Path ) ) {
            InitializeDefaultObject();
        }

        return;

    }

    // Invalid path
    GLogger.Printf( "Invalid path \"%s\"\n", _Path );
    InitializeDefaultObject();
}
