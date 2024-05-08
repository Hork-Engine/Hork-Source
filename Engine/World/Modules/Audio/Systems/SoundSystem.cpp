#include "SoundSystem.h"
#include "../AudioModule.h"
#include "../Components/AudioListenerComponent.h"
#include "../Components/SoundComponent.h"

#include <Engine/Audio/AudioMixer.h>

#include <Engine/World/Common/GameFrame.h>
#include <Engine/World/World.h>
#include <Engine/World/Modules/Transform/Components/WorldTransformComponent.h>

HK_NAMESPACE_BEGIN

//ConsoleVar Snd_RefreshRate("Snd_RefreshRate"s, "16"s); // TODO
ConsoleVar Snd_MasterVolume("Snd_MasterVolume"s, "1"s);

SoundSystem::SoundSystem(World* world) :
    m_World(world),
    m_AudioInterface(world->GetAudioInterface())
{
}

void SoundSystem::Update(GameFrame const& frame)
{
    // TODO: Update sound system with fixed frame rate (use Snd_RefreshRate)

    auto state_index = frame.StateIndex;

    m_Listener.Entity = m_AudioInterface.GetListener();
    auto listener_view = m_World->GetEntityView(m_Listener.Entity);
    if (auto listener_component = listener_view.GetComponent<AudioListenerComponent>())
    {
        m_Listener.VolumeScale = Math::Saturate(listener_component->Volume * m_AudioInterface.MasterVolume * Snd_MasterVolume.GetFloat());
        m_Listener.Mask = listener_component->ListenerMask;
    }
    else
    {
        m_Listener.VolumeScale = Math::Saturate(m_AudioInterface.MasterVolume * Snd_MasterVolume.GetFloat());
        m_Listener.Mask = ~0u;
    }

    if (auto world_transform = listener_view.GetComponent<WorldTransformComponent>())
    {
        m_Listener.TransformInv.Compose(world_transform->Position[state_index], world_transform->Rotation[state_index].ToMatrix3x3());
        m_Listener.TransformInv.InverseSelf();
        m_Listener.Position = world_transform->Position[state_index];
        m_Listener.RightVec = world_transform->Rotation[state_index].XAxis();
    }
    else
    {
        m_Listener.TransformInv.SetIdentity();
        m_Listener.Position.Clear();
        m_Listener.RightVec = Float3(1, 0, 0);
    }

    using Query2 = ECS::Query<>
        ::Required<SoundComponent>
        ::Required<WorldTransformComponent>;

    for (Query2::Iterator it(*m_World); it; it++)
    {
        auto sound_components = it.Get<SoundComponent>();
        auto world_transforms = it.Get<WorldTransformComponent>();

        for (int i = 0; i < it.Count(); i++)
        {
            SoundSource* source = sound_components[i].Source.RawPtr();

            source->SetPositionAndRotation(world_transforms[i].Position[state_index],
                                           world_transforms[i].Rotation[state_index]);
            source->Spatialize(m_Listener);
            source->UpdateTrack(m_SubmitQueue, m_AudioInterface.bPaused);
        }
    }

    m_AudioInterface.UpdateOneShotSound(m_SubmitQueue, m_Listener);

    AudioModule::Get().GetMixer()->SubmitTracks(m_SubmitQueue);
}

HK_NAMESPACE_END
