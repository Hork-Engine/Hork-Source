#pragma once

#include <Engine/ECSRuntime/BehaviorTree.h>

HK_NAMESPACE_BEGIN

/*
Usage:

Setup:
BehaviorTreeComponent& btComponent = commandBuffer.AddComponent<BehaviorTreeComponent>(entity);
btComponent.BT_Tree.Attach(new BT::BehaviorTree(new BT::Repeater(new BT::ECSNode<WalkStateComponent>(entity), 3)));
btComponent.BT_Tree->Start(context);

WalkingSystem:
for (ECS::Query<>::Required<WalkStateComponent>::Iterator it(*m_World); it; it++)
{
    WalkStateComponent* walkState = it.Get<WalkStateComponent>();

    for (int i = 0; i < it.Count(); i++)
    {
        ...Update walk...
             
        if (done walk)
            walkState[i].BT_Leaf->SetSuccess();
    }
}

WalkStateComponent:
struct WalkingStateComponent
{
    BT::ECSNode<WalkingStateComponent>* BT_Leaf;
};
*/
struct BehaviorTreeComponent
{
    TRef<BT::BehaviorTree> BT_Tree;
};

HK_NAMESPACE_END
