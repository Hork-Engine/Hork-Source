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

#include "Trigger.h"

HK_NAMESPACE_BEGIN

HK_CLASS_META(ATrigger)

ATrigger::ATrigger()
{
    m_TriggerBody = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = m_TriggerBody;

    m_TriggerBody->SetDispatchOverlapEvents(true);
    m_TriggerBody->SetTrigger(true);
    m_TriggerBody->SetMotionBehavior(MB_STATIC);
    m_TriggerBody->SetCollisionGroup(CM_TRIGGER);
    m_TriggerBody->SetCollisionMask(CM_PAWN);
}

void ATrigger::SetCollisionModel(CollisionModel* model)
{
    m_TriggerBody->SetCollisionModel(model);
}

void ATrigger::SetBoxCollider()
{
    CollisionBoxDef box;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&box));
}

void ATrigger::SetSphereCollider()
{
    CollisionSphereDef sphere;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&sphere));
}

void ATrigger::SetCylinderCollider()
{
    CollisionCylinderDef cylinder;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&cylinder));
}

void ATrigger::SetConeCollider()
{
    CollisionConeDef cone;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&cone));
}

void ATrigger::SetCapsuleCollider()
{
    CollisionCapsuleDef capsule;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&capsule));
}

void ATrigger::SetLevelGeometry(LevelGeometry const& geometry)
{
    // TODO:
    //m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(geometry));
}

HK_NAMESPACE_END
