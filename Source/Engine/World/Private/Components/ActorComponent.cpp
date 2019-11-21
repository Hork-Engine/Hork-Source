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

#include <World/Public/Components/ActorComponent.h>
#include <World/Public/Actors/Actor.h>
#include <World/Public/World.h>

AN_BEGIN_CLASS_META( AActorComponent )
AN_ATTRIBUTE_( bCanEverTick, AF_DEFAULT )
AN_END_CLASS_META()

AActorComponent::AActorComponent() {
    //GUID.Generate();
}

AWorld * AActorComponent::GetWorld() const {
    AN_ASSERT( ParentActor != nullptr );
    return ParentActor->GetWorld();
}

ALevel * AActorComponent::GetLevel() const {
    AN_ASSERT( ParentActor != nullptr );
    return ParentActor->GetLevel();
}

//void AActorComponent::SetName( AString const & _Name ) {
//    if ( !ParentActor ) {
//        // In constructor
//        Name = _Name;
//        return;
//    }
//
//    AString newName = _Name;
//
//    // Clear name for GenerateComponentUniqueName
//    Name.Clear();
//
//    // Generate new name
//    Name = ParentActor->GenerateComponentUniqueName( newName.CStr() );
//}

void AActorComponent::RegisterComponent() {
    InitializeComponent();

    // FIXME: Call BeginPlay() from here?
}

void AActorComponent::Destroy() {

    if ( bPendingKill ) {
        return;
    }

    // Mark component pending kill
    bPendingKill = true;

    // Add component to pending kill list
    NextPendingKillComponent = GetWorld()->PendingKillComponents;
    GetWorld()->PendingKillComponents = this;

    EndPlay();

    DeinitializeComponent();
    bInitialized = false;
}

int AActorComponent::Serialize( ADocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( GetObjectName() ).CStr() );
    //_Doc.AddStringField( object, "bCreatedDuringConstruction", bCreatedDuringConstruction ? "1" : "0" );

    return object;
}

void AActorComponent::Clone( AActorComponent const * _TemplateComponent ) {
    AClassMeta::CloneAttributes( _TemplateComponent, this );
}
