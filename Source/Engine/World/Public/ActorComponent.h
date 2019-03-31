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

#include "BaseObject.h"
#include <Engine/Core/Public/Guid.h>

#define AN_COMPONENT( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( FActorComponent::Factory(), _Class, _SuperClass ) \
protected: \
    ~_Class() {} \
private:

class FWorld;
class FActor;
class FDebugDraw;

/*

FActorComponent

Base class for all actor components

*/
class ANGIE_API FActorComponent : public FBaseObject {
    AN_COMPONENT( FActorComponent, FBaseObject )

    friend class FActor;
    friend class FWorld;

public:
    // Actor Component factory
    static FObjectFactory & Factory() { static FObjectFactory ObjectFactory( "Actor Component factory" ); return ObjectFactory; }

    // Get component GUID
    FGUID const & GetGUID() const { return GUID; }

    // Set component unique name
    void SetName( FString const & _Name ) override;

    // Component parent actor
    FActor * GetParentActor() const { return ParentActor; }

    // Get world
    FWorld * GetWorld() const;

    // Serialize component to document data
    int Serialize( FDocument & _Doc ) override;

    // Destroy this component
    void Destroy();

    // Is component marked as pending kill
    bool IsPendingKill() const { return bPendingKill; }

    // Register component to initialize it at runtime
    void RegisterComponent();

//    void AddTickPrerequisiteActor( Actor * _Actor );
//    void AddTickPrerequisiteComponent( FActorComponent * _Component );
//    void SetComponentTickEnabled( bool _Enabled );
//    void RegisterComponentTickFunction( FTickFunction & _TickFunction );
//    PrimaryTick PrimaryComponentTick;

    bool bCanEverTick;

protected:

    FActorComponent();

    // [called from Actor's InitializeComponents(), overridable]
    virtual void InitializeComponent() {}

    // [Actor friend, overridable]
    virtual void BeginPlay() {}

    // [Actor friend, overridable]
    // Called only from Destroy() method
    virtual void EndPlay() {}

    // [Actor friend, overridable]
    virtual void TickComponent( float _TimeStep ) {}

    virtual void DebugDraw( FDebugDraw * _DebugDraw ) {}

private:

    void Clone( FActorComponent const * _TemplateComponent );

    FGUID GUID;

    // [Actor friend]
    FActor * ParentActor;
//    ActorRenderState * RenderState;
//    ActorPhysicsState * PhysicsState;

    // [World friend]
    bool bPendingKill;
    bool bCreatedDuringConstruction;
    FActorComponent * NextPendingKillComponent;

    int ComponentIndex = -1;
};
