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

#include <Engine/GameEngine/Public/Actor.h>
#include <Engine/GameEngine/Public/Pawn.h>
#include <Engine/GameEngine/Public/World.h>
#include <Engine/GameEngine/Public/SceneComponent.h>
#include <Engine/GameEngine/Public/Timer.h>
#include <Engine/Core/Public/Logger.h>

AN_BEGIN_CLASS_META( FActor )
AN_ATTRIBUTE_( LifeSpan, AF_DEFAULT )
AN_ATTRIBUTE_( bCanEverTick, AF_DEFAULT )
AN_ATTRIBUTE_( bTickEvenWhenPaused, AF_DEFAULT )
AN_END_CLASS_META()

AN_CLASS_META_NO_ATTRIBS( FViewActor )

static UInt UniqueName = 0;

FActor::FActor() {
    GUID.Generate();
    Name = "Actor" + UniqueName.ToString();
    UniqueName++;
}

void FActor::SetName( FString const & _Name ) {
    if ( !ParentWorld ) {
        // In constructor
        Name = _Name;
        return;
    }

    FString newName = _Name;

    // Clear name for GenerateActorUniqueName
    Name.Clear();

    // Generate new name
    Name = ParentWorld->GenerateActorUniqueName( newName.ToConstChar() );
}

void FActor::Destroy() {
    if ( bPendingKill ) {
        return;
    }

    // Mark actor to remove it from the world
    bPendingKill = true;
    NextPendingKillActor = ParentWorld->PendingKillActors;
    ParentWorld->PendingKillActors = this;

    // Unregister timers
    for ( FTimer * timer = Timers ; timer ; timer = timer->P ) {
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

void FActor::DestroyComponents() {
    for ( FActorComponent * component : Components ) {
        component->Destroy();
    }
}

void FActor::AddComponent( FActorComponent * _Component ) {
    Components.Append( _Component );
    _Component->ComponentIndex = Components.Size() - 1;
    _Component->ParentActor = this;
    _Component->bCreatedDuringConstruction = bDuringConstruction;
}

FString FActor::GenerateComponentUniqueName( const char * _Name ) {
    if ( !FindComponent( _Name ) ) {
        return _Name;
    }
    int uniqueNumber = 0;
    FString uniqueName;
    do {
        uniqueName.Resize( 0 );
        uniqueName.Concat( _Name );
        uniqueName.Concat( Int( ++uniqueNumber ).ToConstChar() );
    } while ( FindComponent( uniqueName.ToConstChar() ) != nullptr );
    return uniqueName;
}

FActorComponent * FActor::CreateComponent( uint64_t _ClassId, const char * _Name ) {
    FActorComponent * component = static_cast< FActorComponent * >( FActorComponent::Factory().CreateInstance( _ClassId ) );
    if ( !component ) {
        return nullptr;
    }
    component->AddRef();
    component->Name = GenerateComponentUniqueName( _Name );
    AddComponent( component );
    return component;
}

FActorComponent * FActor::CreateComponent( const char * _ClassName, const char * _Name ) {
    FActorComponent * component = static_cast< FActorComponent * >( FActorComponent::Factory().CreateInstance( _ClassName ) );
    if ( !component ) {
        return nullptr;
    }
    component->AddRef();
    component->Name = GenerateComponentUniqueName( _Name );
    AddComponent( component );
    return component;
}

FActorComponent * FActor::CreateComponent( FClassMeta const * _ClassMeta, const char * _Name ) {
    AN_Assert( _ClassMeta->Factory() == &FActorComponent::Factory() );
    FActorComponent * component = static_cast< FActorComponent * >( _ClassMeta->CreateInstance() );
    if ( !component ) {
        return nullptr;
    }
    component->AddRef();
    component->Name = GenerateComponentUniqueName( _Name );
    AddComponent( component );
    return component;
}

FActorComponent * FActor::GetComponent( uint64_t _ClassId ) {
    for ( FActorComponent * component : Components ) {
        if ( component->FinalClassId() == _ClassId ) {
            return component;
        }
    }
    return nullptr;
}

FActorComponent * FActor::GetComponent( const char * _ClassName ) {
    for ( FActorComponent * component : Components ) {
        if ( !FString::Cmp( component->FinalClassName(), _ClassName ) ) {
            return component;
        }
    }
    return nullptr;
}

FActorComponent * FActor::GetComponent( FClassMeta const * _ClassMeta ) {
    AN_Assert( _ClassMeta->Factory() == &FActorComponent::Factory() );
    for ( FActorComponent * component : Components ) {
        if ( &component->FinalClassMeta() == _ClassMeta ) {
            return component;
        }
    }
    return nullptr;
}

FActorComponent * FActor::FindComponent( const char * _UniqueName ) {
    for ( FActorComponent * component : Components ) {
        if ( !component->GetName().Icmp( _UniqueName ) ) {
            return component;
        }
    }
    return nullptr;
}

FActorComponent * FActor::FindComponentGUID( FGUID const & _GUID ) {
    for ( FActorComponent * component : Components ) {
        if ( component->GUID == _GUID ) {
            return component;
        }
    }
    return nullptr;
}

void FActor::PostSpawnInitialize( FTransform const & _SpawnTransform ) {
    //GLogger.Printf( "FActor::PostSpawnInitialize()\n" );
    if ( RootComponent ) {
        //GLogger.Printf( "FActor::PostSpawnInitialize: setting transform for %s\n", RootComponent->FinalClassName() );
        RootComponent->SetTransform( _SpawnTransform );
    }
}

//bool FActor::ExecuteContruction( FTransform const & _Transform ) {

//    OnContruction( _Transform );

//    return true;
//}

void FActor::PostActorConstruction() {
    PreInitializeComponents();
    InitializeComponents();
    PostInitializeComponents();
}

void FActor::InitializeComponents() {
    for ( FActorComponent * component : Components ) {
        component->InitializeComponent();
    }
}

void FActor::BeginPlayComponents() {
    for ( FActorComponent * component : Components ) {
        component->BeginPlay();
    }
}

//void FActor::EndPlayComponents() {
//    for ( FActorComponent * component : Components ) {
//        component->EndPlay();
//    }
//}

void FActor::TickComponents( float _TimeStep ) {
    for ( FActorComponent * component : Components ) {
        if ( component->bCanEverTick && !component->IsPendingKill() ) {
            component->TickComponent( _TimeStep );
        }
    }
}

int FActor::Serialize( FDocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    _Doc.AddStringField( object, "GUID", _Doc.ProxyBuffer.NewString( GUID.ToString() ).ToConstChar() );

    if ( RootComponent ) {
        _Doc.AddStringField( object, "Root", _Doc.ProxyBuffer.NewString( RootComponent->GetName() ).ToConstChar() );
    }

    int components = _Doc.AddArray( object, "Components" );

    for ( FActorComponent * component : Components ) {
        if ( component->IsPendingKill() ) {
            continue;
        }
        int componentObject = component->Serialize( _Doc );
        _Doc.AddValueToField( components, componentObject );
    }

    return object;
}

void FActor::Clone( FActor const * _TemplateActor ) {
    // Clone attributes
    FClassMeta::CloneAttributes( _TemplateActor, this );

    // Clone components
    for ( FActorComponent const * templateComponent : _TemplateActor->Components ) {
        if ( templateComponent->IsPendingKill() ) {
            continue;
        }

        FActorComponent * component;
        if ( templateComponent->bCreatedDuringConstruction ) {
            component = FindComponent( templateComponent->GetName().ToConstChar() );
        } else {
            component = CreateComponent( &templateComponent->FinalClassMeta(), templateComponent->GetName().ToConstChar() );
        }

        if ( component ) {
            FClassMeta::CloneAttributes( templateComponent, component );
        }
    }

    if ( _TemplateActor->RootComponent ) {
        FActorComponent * component = FindComponent( _TemplateActor->RootComponent->GetName().ToConstChar() );

        FSceneComponent * root = dynamic_cast< FSceneComponent * >( component );
        if ( root ) {
            RootComponent = root;
        }
    }

    // TODO: Clone components hierarchy, etc
}

FActorComponent * FActor::LoadComponent( FDocument const & _Document, int _FieldsHead ) {
    FDocumentField const * classNameField = _Document.FindField( _FieldsHead, "ClassName" );
    if ( !classNameField ) {
        GLogger.Printf( "FActor::LoadComponent: invalid component class\n" );
        return nullptr;
    }

    FDocumentValue const * classNameValue = &_Document.Values[ classNameField->ValuesHead ];

    FClassMeta const * classMeta = FActorComponent::Factory().LookupClass( classNameValue->Token.ToString().ToConstChar() );
    if ( !classMeta ) {
        GLogger.Printf( "FActor::LoadComponent: invalid component class \"%s\"\n", classNameValue->Token.ToString().ToConstChar() );
        return nullptr;
    }

    FString name;

    FDocumentField * field = _Document.FindField( _FieldsHead, "Name" );
    if ( field ) {
        name = _Document.Values[ field->ValuesHead ].Token.ToString();
    }

    bool bCreatedDuringConstruction = false;

    FActorComponent * component = nullptr;

    if ( !name.IsEmpty() ) {
        component = FindComponent( name.ToConstChar() );
        if ( component && &component->FinalClassMeta() == classMeta ) {
            bCreatedDuringConstruction = component->bCreatedDuringConstruction;
        }
    }

    if ( !bCreatedDuringConstruction ) {
        component = CreateComponent( classMeta, name.IsEmpty() ? "Unnamed" : name.ToConstChar() );
    }

    if ( component ) {
        component->LoadAttributes( _Document, _FieldsHead );
    }

    return component;
}

void FActor::RegisterTimer( FTimer * _Timer ) {
    if ( bDuringConstruction ) {
        GLogger.Printf( "Use FActor::RegisterTimer() in BeginPlay()\n" );
        return;
    }
    AN_Assert( ParentWorld != nullptr );
    _Timer->P = Timers;
    Timers = _Timer;
    ParentWorld->RegisterTimer( _Timer );
}

void FActor::DrawDebug( FDebugDraw * _DebugDraw ) {
    for ( FActorComponent * component : Components ) {
        component->DrawDebug( _DebugDraw );
    }
}

void FActor::EndPlay() {
    E_OnBeginContact.UnsubscribeAll();
    E_OnEndContact.UnsubscribeAll();
    E_OnUpdateContact.UnsubscribeAll();
    E_OnBeginOverlap.UnsubscribeAll();
    E_OnEndOverlap.UnsubscribeAll();
    E_OnUpdateOverlap.UnsubscribeAll();
}

void FActor::ApplyDamage( float _DamageAmount, Float3 const & _Position, FActor * _DamageCauser ) {

}
