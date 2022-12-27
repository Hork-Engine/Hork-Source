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

HK_NAMESPACE_BEGIN

HK_CLASS_META(ActorComponent)

ActorComponent::ActorComponent()
{
    m_bInitialized = false;
    m_bPendingKill = false;
    m_bTicking = false;
    m_bIsDefault = false;
}

World* ActorComponent::GetWorld() const
{
    HK_ASSERT(m_OwnerActor);
    return m_OwnerActor ? m_OwnerActor->GetWorld() : nullptr;
}

Level* ActorComponent::GetLevel() const
{
    HK_ASSERT(m_OwnerActor);
    return m_OwnerActor ? m_OwnerActor->GetLevel() : nullptr;
}

bool ActorComponent::IsInEditor() const
{
    HK_ASSERT(m_OwnerActor);
    return m_OwnerActor ? m_OwnerActor->IsInEditor() : false;
}

void ActorComponent::RegisterComponent()
{
    if (m_bPendingKill)
    {
        return;
    }

    if (m_bInitialized)
    {
        return;
    }

    if (m_OwnerActor->IsSpawning())
    {
        return;
    }

    InitializeComponent();
    m_bInitialized = true;

    // FIXME: Call BeginPlay() from here?
    BeginPlay();
}

void ActorComponent::Destroy()
{
    World::DestroyComponent(this);
}

HK_NAMESPACE_END
