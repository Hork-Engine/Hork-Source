/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

class AWorld;
class ALevel;
class AActor;
class ADebugRenderer;

#define AN_COMPONENT( _Class, _SuperClass ) \
    AN_FACTORY_CLASS( AActorComponent::Factory(), _Class, _SuperClass )

/**

AActorComponent

Base class for all actor components

*/
class AActorComponent : public ABaseObject {
    AN_COMPONENT( AActorComponent, ABaseObject )

    friend class AActor;
    friend class AWorld;

public:
    /** Actor Component factory */
    static AObjectFactory & Factory() { static AObjectFactory ObjectFactory( "Actor Component factory" ); return ObjectFactory; }

    /** Component owner */
    AActor * GetOwnerActor() const { return OwnerActor; }

    /** Component parent level */
    ALevel * GetLevel() const;

    /** Get world */
    AWorld * GetWorld() const;

    /** Destroy this component */
    void Destroy();

    /** Is component initialized */
    bool IsInitialized() const { return bInitialized; }

    /** Is component marked as pending kill */
    bool IsPendingKill() const { return bPendingKill; }

    /** Spawned for editing */
    bool IsInEditor() const;

    /** Attributes of this component will not be visible in editor */
    void SetHideInEditor( bool _HideInEditor ) { bHideInEditor = _HideInEditor; }

    bool HiddenInEditor() const { return bHideInEditor; }

    /** Register component to initialize it at runtime */
    void RegisterComponent();

protected:
    bool bCanEverTick = false;

    AActorComponent();

    virtual void OnCreateAvatar() {}

    /** Called from Actor's InitializeComponents() */
    virtual void InitializeComponent() {}

    /** Called from Actor's DeinitializeComponents() */
    virtual void DeinitializeComponent() {}

    virtual void BeginPlay() {}

    virtual void TickComponent( float _TimeStep ) {}

    virtual void DrawDebug( ADebugRenderer * InRenderer ) {}

private:
    AActor * OwnerActor = nullptr;

    AActorComponent * NextPendingKillComponent = nullptr;

    int LocalId{};
    int ComponentIndex = -1;

    bool bInitialized : 1;
    bool bPendingKill : 1;
    bool bHideInEditor : 1;
    bool bTicking : 1;
};
