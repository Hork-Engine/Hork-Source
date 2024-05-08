#pragma once

#include <Engine/World/Common/EngineSystem.h>
#include <Engine/Audio/AudioMixer.h>

#include "../AudioInterface.h"
#include "../SoundSource.h"

HK_NAMESPACE_BEGIN

class SoundSystem : public EngineSystemECS
{
public:
    SoundSystem(class World* world);

    void Update(struct GameFrame const& frame);

private:
    World* m_World;
    AudioInterface& m_AudioInterface;
    AudioListener m_Listener;
    AudioMixerSubmitQueue m_SubmitQueue;
};

HK_NAMESPACE_END
