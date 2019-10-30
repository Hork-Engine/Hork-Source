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

#include <Engine/World/Public/Components/ActorComponent.h>
#include <Engine/World/Public/Actors/Actor.h>
#include <Engine/World/Public/World.h>

AN_BEGIN_CLASS_META( FActorComponent )
AN_ATTRIBUTE_( bCanEverTick, AF_DEFAULT )
AN_END_CLASS_META()

FActorComponent::FActorComponent() {
    GUID.Generate();
}

FWorld * FActorComponent::GetWorld() const {
    AN_Assert( ParentActor != nullptr );
    return ParentActor->GetWorld();
}

FLevel * FActorComponent::GetLevel() const {
    AN_Assert( ParentActor != nullptr );
    return ParentActor->GetLevel();
}

void FActorComponent::SetName( FString const & _Name ) {
    if ( !ParentActor ) {
        // In constructor
        Name = _Name;
        return;
    }

    FString newName = _Name;

    // Clear name for GenerateComponentUniqueName
    Name.Clear();

    // Generate new name
    Name = ParentActor->GenerateComponentUniqueName( newName.ToConstChar() );
}

void FActorComponent::RegisterComponent() {
    InitializeComponent();

    // FIXME: Call BeginPlay() from here?
}

void FActorComponent::Destroy() {

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
}

int FActorComponent::Serialize( FDocument & _Doc ) {
    int object = Super::Serialize( _Doc );

    _Doc.AddStringField( object, "Name", _Doc.ProxyBuffer.NewString( Name ).ToConstChar() );
    //_Doc.AddStringField( object, "bCreatedDuringConstruction", bCreatedDuringConstruction ? "1" : "0" );

    return object;
}

void FActorComponent::Clone( FActorComponent const * _TemplateComponent ) {
    FClassMeta::CloneAttributes( _TemplateComponent, this );
}
