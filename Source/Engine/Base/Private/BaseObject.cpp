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

AN_BEGIN_CLASS_META( FBaseObject )
//AN_ATTRIBUTE( Name, SetName, GetName, AF_DEFAULT )
AN_END_CLASS_META()

uint64_t FBaseObject::TotalObjects = 0;

FBaseObject::FBaseObject() {
    TotalObjects++;
    FGarbageCollector::AddObject( this );
}

FBaseObject::~FBaseObject() {
    TotalObjects--;

    if ( WeakRefCounter ) {
        WeakRefCounter->Object = nullptr;
    }
}

void FBaseObject::AddRef() {
    AN_ASSERT( RefCount != -666, "Calling AddRef() in destructor" );
    ++RefCount;
    if ( RefCount == 1 ) {
        FGarbageCollector::RemoveObject( this );
    }
}

void FBaseObject::RemoveRef() {
    AN_ASSERT( RefCount != -666, "Calling RemoveRef() in destructor" );
    if ( RefCount == 1 ) {
        RefCount = 0;
        FGarbageCollector::AddObject( this );
    } else if ( RefCount > 0 ) {
        --RefCount;
    }
}

void FBaseObject::InitializeDefaultObject() {
    InitializeInternalResource( ( FString( FinalClassName() ) + ".Default" ).ToConstChar() );
}

bool FBaseObject::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    if ( _CreateDefultObjectIfFails ) {
        InitializeDefaultObject();
        return true;
    }
    return false;
}

void FBaseObject::InitializeInternalResource( const char * _InternalResourceName ) {
    //SetName( _InternalResourceName );
}

int FBaseObject::Serialize( FDocument & _Doc ) {
    int object = _Doc.CreateObjectValue();

    _Doc.AddStringField( object, "ClassName", FinalClassName() );

//    int precacheArray = -1;

    for ( FClassMeta const * Meta = &FinalClassMeta()
          ; Meta ; Meta = Meta->SuperClass() ) {

#if 0
        FPrecacheMeta const * precache = Meta->GetPrecacheList();
        if ( precache ) {

            for ( ; precache ; precache = precache->Next() ) {

                precacheArray = ( precacheArray == -1 ) ? _Doc.AddArray( object, "Precache" ) : precacheArray;

                _Doc.AddValueToField( precacheArray, _Doc.CreateStringValue( precache->GetResourcePath() ) );
            }
        }
#endif

        FAttributeMeta const * attribs = Meta->GetAttribList();
        if ( attribs ) {
            int attribArray = -1;

            for ( FAttributeMeta const * attr = attribs ; attr ; attr = attr->Next() ) {

                if ( attr->GetFlags() & AF_NON_SERIALIZABLE ) {
                    continue;
                }

                attribArray = ( attribArray == -1 ) ? _Doc.AddArray( object, Meta->GetName() ) : attribArray;

                int attribObject = _Doc.CreateObjectValue();

                FString & s = _Doc.ProxyBuffer.NewString();
                attr->GetValue( this, s );

                _Doc.AddStringField( attribObject, attr->GetName(), s.ToConstChar() );

                _Doc.AddValueToField( attribArray, attribObject );
            }
        }
    }

    return object;
}

void FBaseObject::LoadAttributes( FDocument const & _Document, int _FieldsHead ) {
    for ( FClassMeta const * meta = &FinalClassMeta() ; meta ; meta = meta->SuperClass() ) {
        FDocumentField const * field = _Document.FindField( _FieldsHead, meta->GetName() );
        if ( field ) {
            for ( int i = field->ValuesHead ; i != -1 ; i = _Document.Values[ i ].Next ) {
                FDocumentValue * attributeObject = &_Document.Values[ i ];
                if ( attributeObject->Type == FDocumentValue::T_Object ) {
                    for ( int j = attributeObject->FieldsHead ; j != -1 ; j = _Document.Fields[ j ].Next ) {
                        FDocumentField const * attributeName = &_Document.Fields[ j ];
                        FAttributeMeta const * attrMeta = meta->FindAttribute( attributeName->Name.ToString().ToConstChar(), false );
                        if ( attrMeta ) {
                            FDocumentValue const * attributeValue = &_Document.Values[ attributeName->ValuesHead ];
                            attrMeta->SetValue( this, attributeValue->Token.ToString() );
                        }
                    }
                }
            }
        }
    }
}

FBaseObject * FGarbageCollector::GarbageObjects = nullptr;
FBaseObject * FGarbageCollector::GarbageObjectsTail = nullptr;

void FGarbageCollector::Initialize() {

}

void FGarbageCollector::Deinitialize() {
    DeallocateObjects();
}

void FGarbageCollector::AddObject( FBaseObject * _Object ) {
    IntrusiveAddToList( _Object, NextGarbageObject, PrevGarbageObject, GarbageObjects, GarbageObjectsTail )
}

void FGarbageCollector::RemoveObject( FBaseObject * _Object ) {
    IntrusiveRemoveFromList( _Object, NextGarbageObject, PrevGarbageObject, GarbageObjects, GarbageObjectsTail )
}

void FGarbageCollector::DeallocateObjects() {
#if 0
    while ( GarbageObjects ) {
        FBaseObject * object = GarbageObjects;
        FBaseObject * nextObject;

        GarbageObjects = nullptr;

        while ( object ) {
            nextObject = object->NextGarbageObject;
            const FClassMeta & classMeta = object->FinalClassMeta();
            classMeta.DestroyInstance( object );
            object = nextObject;
        }
    }
#endif
    while ( GarbageObjects ) {
        FBaseObject * object = GarbageObjects;

        // Mark RefCount to prevent using of AddRef/RemoveRef in the object destructor
        object->RefCount = -666;

        RemoveObject( object );

        const FClassMeta & classMeta = object->FinalClassMeta();
        classMeta.DestroyInstance( object );
    }

    GarbageObjectsTail = nullptr;

    //GPrintf( "TotalObjects: %d\n", FBaseObject::GetTotalObjects() );
}


void FWeakReference::ResetWeakRef( FBaseObject * _Object ) {
    FBaseObject * Cur = WeakRefCounter ? WeakRefCounter->Object : nullptr;

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

void FWeakReference::RemoveWeakRef() {
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

FWeakRefCounter * FWeakReference::AllocateWeakRefCounter() {
    // Own allocator couldn't handle destruction of static objects :(
    return new FWeakRefCounter;
    //return ( FWeakRefCounter * )GZoneMemory.Alloc( sizeof( FWeakRefCounter ), 1 );
}

void FWeakReference::DeallocateWeakRefCounter( FWeakRefCounter * _RefCounter ) {
    // Own allocator couldn't handle destruction of static objects :(
    delete _RefCounter;
    //GZoneMemory.Dealloc( _RefCounter );
}
