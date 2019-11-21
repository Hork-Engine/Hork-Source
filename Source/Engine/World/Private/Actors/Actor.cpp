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

#include <World/Public/Actors/Actor.h>
#include <World/Public/Actors/Pawn.h>
#include <World/Public/Components/SceneComponent.h>
#include <World/Public/World.h>
#include <World/Public/Timer.h>
#include <Core/Public/Logger.h>

AN_CLASS_META( AActor )

ARuntimeVariable RVDrawRootComponentAxis( _CTS( "DrawRootComponentAxis" ), _CTS( "0" ), VAR_CHEAT );

static UInt UniqueName = 0;

AActor::AActor() {
    //GUID.Generate();
    SetObjectName( "Actor" + UniqueName.ToString() );
    UniqueName++;
}

//void AActor::SetName( AString const & _Name ) {
//    if ( !ParentWorld ) {
//        // In constructor
//        Name = _Name;
//        return;
//    }
//
//    AString newName = _Name;
//
//    // Clear name for GenerateActorUniqueName
//    Name.Clear();
//
//    // Generate new name
//    Name = ParentWorld->GenerateActorUniqueName( newName.CStr() );
//}

void AActor::Destroy() {
    if ( bPendingKill ) {
        return;
    }

    // Mark actor to remove it from the world
    bPendingKill = true;
    NextPendingKillActor = ParentWorld->PendingKillActors;
    ParentWorld->PendingKillActors = this;

    // Unregister timers
    for ( ATimer * timer = Timers ; timer ; timer = timer->P ) {
        ParentWorld->UnregisterTimer( timer );
    }
    Timers = nullptr;

    DestroyComponents();

    EndPlay();

    if ( Instigator ) {
        Instigator->RemoveRef();
        Instigator = nullptr;
    }
}

void AActor::DestroyComponents() {
    for ( AActorComponent * component : Components ) {
        component->Destroy();
    }
}

void AActor::AddComponent( AActorComponent * _Component ) {
    Components.Append( _Component );
    _Component->ComponentIndex = Components.Size() - 1;
    _Component->ParentActor = this;
    _Component->bCreatedDuringConstruction = bDuringConstruction;
}

//AString AActor::GenerateComponentUniqueName( const char * _Name ) {
//    if ( !FindComponent( _Name ) ) {
//        return _Name;
//    }
//    int uniqueNumber = 0;
//    AString uniqueName;
//    do {
//        uniqueName.Resize( 0 );
//        uniqueName.Concat( _Name );
//        uniqueName.Concat( Int( ++uniqueNumber ).CStr() );
//    } while ( FindComponent( uniqueName.CStr() ) != nullptr );
//    return uniqueName;
//}

AActorComponent * AActor::CreateComponent( uint64_t _ClassId, const char * _Name ) {
    AActorComponent * component = static_cast< AActorComponent * >( AActorComponent::Factory().CreateInstance( _ClassId ) );
    if ( !component ) {
        return nullptr;
    }
    component->AddRef();
    component->SetObjectName( _Name );//GenerateComponentUniqueName( _Name );
    AddComponent( component );
    return component;
}

AActorComponent * AActor::CreateComponent( const char * _ClassName, const char * _Name ) {
    AActorComponent * component = static_cast< AActorComponent * >( AActorComponent::Factory().CreateInstance( _ClassName ) );
    if ( !component ) {
        return nullptr;
    }
    component->AddRef();
    component->SetObjectName( _Name );//GenerateComponentUniqueName( _Name );
    AddComponent( component );
    return component;
}

AActorComponent * AActor::CreateComponent( AClassMeta const * _ClassMeta, const char * _Name ) {
    AN_ASSERT( _ClassMeta->Factory() == &AActorComponent::Factory() );
    AActorComponent * component = static_cast< AActorComponent * >( _ClassMeta->CreateInstance() );
    if ( !component ) {
        return nullptr;
    }
    component->AddRef();
    component->SetObjectName( _Name );//GenerateComponentUniqueName( _Name );
    AddComponent( component );
    return component;
}

AActorComponent * AActor::GetComponent( uint64_t _ClassId ) {
    for ( AActorComponent * component : Components ) {
        if ( component->FinalClassId() == _ClassId ) {
            return component;
        }
    }
    return nullptr;
}

AActorComponent * AActor::GetComponent( const char * _ClassName ) {
    for ( AActorComponent * component : Components ) {
        if ( !AString::Cmp( component->FinalClassName(), _ClassName ) ) {
            return component;
        }
    }
    return nullptr;
}

AActorComponent * AActor::GetComponent( AClassMeta const * _ClassMeta ) {
    AN_ASSERT( _ClassMeta->Factory() == &AActorComponent::Factory() );
    for ( AActorComponent * component : Components ) {
        if ( &component->FinalClassMeta() == _ClassMeta ) {
            return component;
        }
    }
    return nullptr;
}

//AActorComponent * AActor::FindComponent( const char * _UniqueName ) {
//    for ( AActorComponent * component : Components ) {
//        if ( !component->GetName().Icmp( _UniqueName ) ) {
//            return component;
//        }
//    }
//    return nullptr;
//}

//AActorComponent * AActor::FindComponentGUID( AGUID const & _GUID ) {
//    for ( AActorComponent * component : Components ) {
//        if ( component->GUID == _GUID ) {
//            return component;
//        }
//    }
//    return nullptr;
//}

void AActor::Initialize( ATransform const & _SpawnTransform ) {
    if ( RootComponent ) {
        RootComponent->SetTransform( _SpawnTransform );
    }

    PreInitializeComponents();
    InitializeComponents();
    PostInitializeComponents();

    BeginPlayComponents();
    BeginPlay();
}

void AActor::InitializeComponents() {
    for ( AActorComponent * component : Components ) {
        component->InitializeComponent();
        component->bInitialized = true;
    }
}

void AActor::BeginPlayComponents() {
    for ( AActorComponent * component : Components ) {
        component->BeginPlay();
    }
}

//void AActor::EndPlayComponents() {
//    for ( AActorComponent * component : Components ) {
//        component->EndPlay();
//    }
//}

void AActor::TickComponents( float _TimeStep ) {
    for ( AActorComponent * component : Components ) {
        if ( component->bCanEverTick && !component->IsPendingKill() ) {
            component->TickComponent( _TimeStep );
        }
    }
}

int AActor::Serialize( ADocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    //_Doc.AddStringField( object, "GUID", _Doc.ProxyBuffer.NewString( GUID.ToString() ).CStr() );

    //if ( RootComponent ) {
    //    _Doc.AddStringField( object, "Root", _Doc.ProxyBuffer.NewString( RootComponent->GetName() ).CStr() );
    //}

    //int components = _Doc.AddArray( object, "Components" );

    //for ( AActorComponent * component : Components ) {
    //    if ( component->IsPendingKill() ) {
    //        continue;
    //    }
    //    int componentObject = component->Serialize( _Doc );
    //    _Doc.AddValueToField( components, componentObject );
    //}

    return object;
}

void AActor::Clone( AActor const * _TemplateActor ) {
    // Clone attributes
    AClassMeta::CloneAttributes( _TemplateActor, this );

    // TODO: Clone components

//    for ( AActorComponent const * templateComponent : _TemplateActor->Components ) {
//        if ( templateComponent->IsPendingKill() ) {
//            continue;
//        }

//        AActorComponent * component;
//        if ( templateComponent->bCreatedDuringConstruction ) {
//            component = FindComponentGUID( templateComponent->GetGUID() );
//        } else {
//            component = AddComponent( &templateComponent->FinalClassMeta(), templateComponent->GetObjectName().CStr() );
//        }

//        if ( component ) {

//            if ( templateComponent == _TemplateActor->RootComponent ) {
//                RootComponent = dynamic_cast< ASceneComponent * >( component );
//            }

//            AClassMeta::CloneAttributes( templateComponent, component );
//        }
//    }

    // TODO: Clone components hierarchy, etc
}

#if 0
AActorComponent * AActor::LoadComponent( ADocument const & _Document, int _FieldsHead ) {
    SDocumentField const * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "AActor::LoadComponent: invalid component class\n" );
        return nullptr;
    }

    SDocumentValue const * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    AClassMeta const * classMeta = AActorComponent::Factory().LookupClass( classNameValue->Token.ToString().CStr() );
    if ( !classMeta ) {
        GLogger.Printf( "AActor::LoadComponent: invalid component class \"%s\"\n", classNameValue->Token.ToString().CStr() );
        return nullptr;
    }

    AString name, guid;

    SDocumentField * field = _Document.FindField( _FieldsHead, "Name" );
    if ( field ) {
        name = _Document.Values[ field->ValuesHead ].Token.ToString();
    }

    field = _Document.FindField( _FieldsHead, "GUID" );
    if ( field ) {
        guid = _Document.Values[field->ValuesHead].Token.ToString();
    }

    bool bCreatedDuringConstruction = false;

    AActorComponent * component = nullptr;

    if ( !guid.IsEmpty() ) {
        component = FindComponentGUID( AGUID().FromString( guid ) );
        if ( component && &component->FinalClassMeta() == classMeta ) {
            bCreatedDuringConstruction = component->bCreatedDuringConstruction;
        }
    }

    if ( !bCreatedDuringConstruction ) {
        component = AddComponent( classMeta, name.IsEmpty() ? "Unnamed" : name.CStr() );
    }

    if ( component ) {
        component->LoadAttributes( _Document, _FieldsHead );
    }

    return component;
}
#endif

void AActor::RegisterTimer( ATimer * _Timer ) {
    if ( bDuringConstruction ) {
        GLogger.Printf( "Use AActor::RegisterTimer() in BeginPlay()\n" );
        return;
    }
    AN_ASSERT( ParentWorld != nullptr );
    _Timer->P = Timers;
    Timers = _Timer;
    ParentWorld->RegisterTimer( _Timer );
}

void AActor::DrawDebug( ADebugDraw * _DebugDraw ) {
    for ( AActorComponent * component : Components ) {
        component->DrawDebug( _DebugDraw );
    }

    if ( RVDrawRootComponentAxis ) {
        if ( RootComponent ) {
            _DebugDraw->SetDepthTest( false );
            _DebugDraw->DrawAxis( RootComponent->GetWorldTransformMatrix(), false );
        }
    }
}

void AActor::EndPlay() {
}

void AActor::ApplyDamage( float _DamageAmount, Float3 const & _Position, AActor * _DamageCauser ) {

}
