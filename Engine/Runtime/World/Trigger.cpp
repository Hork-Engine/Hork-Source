/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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
#include "PhysicalBody.h"

HK_NAMESPACE_BEGIN

HK_CLASS_META(Actor_Trigger)

Actor_Trigger::Actor_Trigger()
{
    m_TriggerBody = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = m_TriggerBody;

    m_TriggerBody->SetDispatchOverlapEvents(true);
    m_TriggerBody->SetTrigger(true);
    m_TriggerBody->SetMotionBehavior(MB_STATIC);
    m_TriggerBody->SetCollisionGroup(CM_TRIGGER);
    m_TriggerBody->SetCollisionMask(CM_PAWN);
}

void Actor_Trigger::SetCollisionModel(CollisionModel* model)
{
    m_TriggerBody->SetCollisionModel(model);
}

void Actor_Trigger::SetBoxCollider()
{
    CollisionBoxDef box;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&box));
}

void Actor_Trigger::SetSphereCollider()
{
    CollisionSphereDef sphere;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&sphere));
}

void Actor_Trigger::SetCylinderCollider()
{
    CollisionCylinderDef cylinder;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&cylinder));
}

void Actor_Trigger::SetConeCollider()
{
    CollisionConeDef cone;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&cone));
}

void Actor_Trigger::SetCapsuleCollider()
{
    CollisionCapsuleDef capsule;
    m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(&capsule));
}

void Actor_Trigger::SetLevelGeometry(LevelGeometry const& geometry)
{
    // TODO:
    //m_TriggerBody->SetCollisionModel(NewObj<CollisionModel>(geometry));
}

HK_NAMESPACE_END
