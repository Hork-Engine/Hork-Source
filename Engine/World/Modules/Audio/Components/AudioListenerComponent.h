#pragma once

HK_NAMESPACE_BEGIN

struct AudioListenerComponent
{
    float Volume = 1.0f;
    uint32_t ListenerMask = ~0u;

    // TODO
    //Float3 Velocity;
    //float DopplerFactor = 1;
    //float DopplerVelocity = 1;
    //float SpeedOfSound = 343.3f;
    //AUDIO_DISTANCE_MODEL DistanceModel = AUDIO_DIST_LINEAR_CLAMPED;//AUDIO_DIST_INVERSE_CLAMPED;
};

HK_NAMESPACE_END
