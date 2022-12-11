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

#include "Triggers.h"

HK_CLASS_META(ATriggerBox)

ATriggerBox::ATriggerBox()
{
    TriggerBody   = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = TriggerBody;

    TriggerBody->SetDispatchOverlapEvents(true);
    TriggerBody->SetTrigger(true);
    TriggerBody->SetMotionBehavior(MB_STATIC);
    TriggerBody->SetCollisionGroup(CM_TRIGGER);
    TriggerBody->SetCollisionMask(CM_PAWN);

    CollisionBoxDef box;
    CollisionModel* collisionModel = NewObj<CollisionModel>(&box);

    TriggerBody->SetCollisionModel(collisionModel);
}

HK_CLASS_META(ATriggerSphere)

ATriggerSphere::ATriggerSphere()
{
    TriggerBody   = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = TriggerBody;

    TriggerBody->SetDispatchOverlapEvents(true);
    TriggerBody->SetTrigger(true);
    TriggerBody->SetMotionBehavior(MB_STATIC);
    TriggerBody->SetCollisionGroup(CM_TRIGGER);
    TriggerBody->SetCollisionMask(CM_PAWN);

    CollisionSphereDef sphere;
    CollisionModel*    collisionModel = NewObj<CollisionModel>(&sphere);

    TriggerBody->SetCollisionModel(collisionModel);
}

HK_CLASS_META(ATriggerCylinder)

ATriggerCylinder::ATriggerCylinder()
{
    TriggerBody   = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = TriggerBody;

    TriggerBody->SetDispatchOverlapEvents(true);
    TriggerBody->SetTrigger(true);
    TriggerBody->SetMotionBehavior(MB_STATIC);
    TriggerBody->SetCollisionGroup(CM_TRIGGER);
    TriggerBody->SetCollisionMask(CM_PAWN);

    CollisionCylinderDef cylinder;
    CollisionModel*      collisionModel = NewObj<CollisionModel>(&cylinder);

    TriggerBody->SetCollisionModel(collisionModel);
}

HK_CLASS_META(ATriggerCone)

ATriggerCone::ATriggerCone()
{
    TriggerBody   = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = TriggerBody;

    TriggerBody->SetDispatchOverlapEvents(true);
    TriggerBody->SetTrigger(true);
    TriggerBody->SetMotionBehavior(MB_STATIC);
    TriggerBody->SetCollisionGroup(CM_TRIGGER);
    TriggerBody->SetCollisionMask(CM_PAWN);

    CollisionConeDef cone;
    CollisionModel*  collisionModel = NewObj<CollisionModel>(&cone);

    TriggerBody->SetCollisionModel(collisionModel);
}

HK_CLASS_META(ATriggerCapsule)

ATriggerCapsule::ATriggerCapsule()
{
    TriggerBody   = CreateComponent<PhysicalBody>("TriggerBody");
    m_RootComponent = TriggerBody;

    TriggerBody->SetDispatchOverlapEvents(true);
    TriggerBody->SetTrigger(true);
    TriggerBody->SetMotionBehavior(MB_STATIC);
    TriggerBody->SetCollisionGroup(CM_TRIGGER);
    TriggerBody->SetCollisionMask(CM_PAWN);

    CollisionCapsuleDef capsule;
    CollisionModel*     collisionModel = NewObj<CollisionModel>(&capsule);

    TriggerBody->SetCollisionModel(collisionModel);
}
