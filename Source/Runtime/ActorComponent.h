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

HK_NAMESPACE_BEGIN

class World;
class Level;
class Actor;
class DebugRenderer;

#define HK_COMPONENT(_Class, _SuperClass) \
    HK_FACTORY_CLASS(ActorComponent::Factory(), _Class, _SuperClass)

/**

ActorComponent

Base class for all actor components

*/
class ActorComponent : public BaseObject
{
    HK_COMPONENT(ActorComponent, BaseObject)

    friend class Actor;
    friend class World;

public:
    /** Actor Component factory */
    static ObjectFactory& Factory()
    {
        static ObjectFactory ObjectFactory("Actor Component factory");
        return ObjectFactory;
    }

    /** Component owner */
    Actor* GetOwnerActor() const { return m_OwnerActor; }

    /** Component parent level */
    Level* GetLevel() const;

    /** Get world */
    World* GetWorld() const;

    /** Destroy this component */
    void Destroy();

    /** Is component initialized */
    bool IsInitialized() const { return m_bInitialized; }

    /** Is component marked as pending kill */
    bool IsPendingKill() const { return m_bPendingKill; }

    /** Spawned for editing */
    bool IsInEditor() const;

    /** Register component to initialize it at runtime */
    void RegisterComponent();

    /** Set object debug/editor or ingame name */
    void SetObjectName(StringView Name) { m_Name = Name; }

    /** Get object debug/editor or ingame name */
    String const& GetObjectName() const { return m_Name; }

    int GetLocalId() const { return m_LocalId; }

protected:
    bool m_bCanEverTick = false;

    ActorComponent();

    /** Called from Actor's InitializeComponents() */
    virtual void InitializeComponent() {}

    /** Called from Actor's DeinitializeComponents() */
    virtual void DeinitializeComponent() {}

    virtual void BeginPlay() {}

    virtual void TickComponent(float _TimeStep) {}

    virtual void DrawDebug(DebugRenderer* InRenderer) {}

private:
    Actor* m_OwnerActor = nullptr;

    ActorComponent* m_NextPendingKillComponent = nullptr;

    String m_Name;

    int m_LocalId{};
    int m_ComponentIndex = -1;

    bool m_bInitialized : 1;
    bool m_bPendingKill : 1;
    bool m_bTicking : 1;
    bool m_bIsDefault : 1;
};

HK_NAMESPACE_END
