/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Runtime/InputComponent.h>
#include <Runtime/MeshComponent.h>
#include <Runtime/Timer.h>

class ATrigger : public AActor
{
    AN_ACTOR(ATrigger, AActor)

public:
    std::function<void()> SpawnFunction;

protected:
    APhysicalBody* TriggerBody{};
    TRef<ATimer>   Timer;

    ATrigger()
    {}

    void Initialize(SActorInitializer& Initializer) override
    {
        TriggerBody = CreateComponent<APhysicalBody>("TriggerBody");
        TriggerBody->SetDispatchOverlapEvents(true);
        TriggerBody->SetTrigger(true);
        TriggerBody->SetMotionBehavior(MB_STATIC);
        TriggerBody->SetCollisionGroup(CM_TRIGGER);
        TriggerBody->SetCollisionMask(CM_PAWN);
        ACollisionModel* collisionModel = CreateInstanceOf<ACollisionModel>();
        collisionModel->CreateBody<ACollisionBox>();
        TriggerBody->SetCollisionModel(collisionModel);

        RootComponent = TriggerBody;

        Timer = CreateInstanceOf<ATimer>();
    }

    void BeginPlay()
    {
        Super::BeginPlay();

        E_OnBeginOverlap.Add(this, &ATrigger::OnBeginOverlap);
        E_OnEndOverlap.Add(this, &ATrigger::OnEndOverlap);
        E_OnUpdateOverlap.Add(this, &ATrigger::OnUpdateOverlap);

        Timer->SleepDelay = 0.5f;
        Timer->Callback.Set(this, &ATrigger::OnTimer);

        //Timer = AddTimer(this, &ATrigger::OnTimer);
        //Timer->SleepDelay = 0.5f;

        //RemoveTimer(Timer);
    }

    void OnBeginOverlap(SOverlapEvent const& _Event)
    {
        GLogger.Printf("OnBeginOverlap: self %s other %s\n",
                       _Event.SelfBody->GetObjectName().CStr(),
                       _Event.OtherBody->GetObjectName().CStr());

        Timer->Register(GetWorld());
    }

    void OnEndOverlap(SOverlapEvent const& _Event)
    {
        GLogger.Printf("OnEndOverlap: self %s other %s\n",
                       _Event.SelfBody->GetObjectName().CStr(),
                       _Event.OtherBody->GetObjectName().CStr());

        Timer->Unregister();
    }

    void OnUpdateOverlap(SOverlapEvent const& _Event)
    {
        //    GLogger.Printf( "OnUpdateOverlap: self %s other %s\n",
        //                    _Event.SelfBody->GetObjectName().CStr(),
        //                    _Event.OtherBody->GetObjectName().CStr() );
    }

    void OnTimer()
    {
        SpawnFunction();
    }
};
