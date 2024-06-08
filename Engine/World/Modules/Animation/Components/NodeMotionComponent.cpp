#include "NodeMotionComponent.h"

#include <Engine/World/Modules/Animation/NodeMotion.h>
#include <Engine/World/World.h>

HK_NAMESPACE_BEGIN

void NodeMotionComponent::FixedUpdate()
{
    if (!Animation)
        return;

    auto owner = GetOwner();

    float time = Timer.Time;

    for (auto& channel : Animation->m_Channels)
    {
        if (channel.TargetNode == NodeID)
        {
            switch (channel.TargetPath)
            {
            case NODE_ANIMATION_PATH_TRANSLATION:
                owner->SetPosition(Animation->SampleVector(channel.Smp, time));
                break;
            case NODE_ANIMATION_PATH_ROTATION:
                owner->SetRotation(Animation->SampleQuaternion(channel.Smp, time));
                break;
            case NODE_ANIMATION_PATH_SCALE:
                owner->SetScale(Animation->SampleVector(channel.Smp, time));
                break;
            }
        }
    }

    if (!Timer.IsPaused)
    {
        Timer.Time += GetWorld()->GetTick().FixedTimeStep;

        if (Timer.Time > Timer.LoopTime)
            Timer.Time = Math::FMod(Timer.Time, Timer.LoopTime);
    }
}

HK_NAMESPACE_END
