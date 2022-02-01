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

#include "ActorComponent.h"
#include "Actor.h"
#include "World.h"

AN_CLASS_META(AActorComponent)

AActorComponent::AActorComponent()
{
    bInitialized  = false;
    bPendingKill  = false;
    bHideInEditor = false;
    bTicking      = false;
}

AWorld* AActorComponent::GetWorld() const
{
    AN_ASSERT(OwnerActor);
    return OwnerActor ? OwnerActor->GetWorld() : nullptr;
}

ALevel* AActorComponent::GetLevel() const
{
    AN_ASSERT(OwnerActor);
    return OwnerActor ? OwnerActor->GetLevel() : nullptr;
}

bool AActorComponent::IsInEditor() const
{
    AN_ASSERT(OwnerActor);
    return OwnerActor ? OwnerActor->IsInEditor() : false;
}

void AActorComponent::RegisterComponent()
{
    if (bPendingKill)
    {
        return;
    }

    if (bInitialized)
    {
        return;
    }

    if (OwnerActor->IsSpawning())
    {
        return;
    }

    InitializeComponent();
    bInitialized = true;

    // FIXME: Call BeginPlay() from here?
    BeginPlay();
}

void AActorComponent::Destroy()
{
    if (bPendingKill)
    {
        return;
    }

    // Mark component pending kill
    bPendingKill = true;

    // Add component to pending kill list
    NextPendingKillComponent          = GetWorld()->PendingKillComponents;
    GetWorld()->PendingKillComponents = this;
}
