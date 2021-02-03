/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <World/Public/Components/SoundEmitter.h>
#include <World/Public/Components/MeshComponent.h>
#include <World/Public/Actors/PlayerController.h>
#include <World/Public/World.h>
#include <Audio/AudioDevice.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>

ASoundEmitter * ASoundEmitter::SoundEmitters;
ASoundEmitter * ASoundEmitter::SoundEmittersTail;

ASoundOneShot * ASoundEmitter::OneShots = nullptr;
ASoundOneShot * ASoundEmitter::OneShotsTail = nullptr;

static void AddChannel( SAudioChannel * Channel )
{
    INTRUSIVE_ADD_UNIQUE( Channel, Next, Prev, SAudioChannel::Channels, SAudioChannel::ChannelsTail );
}

static void RemoveChannel( SAudioChannel * Channel )
{
    INTRUSIVE_REMOVE( Channel, Next, Prev, SAudioChannel::Channels, SAudioChannel::ChannelsTail );
}

AN_CLASS_META( ASoundGroup )

AN_CLASS_META( ASoundEmitter )

ASoundEmitter::ASoundEmitter()
{
    ListenerMask = ~0u;
    EmitterType = SOUND_EMITTER_POINT;
    Volume = 1.0f;
    ReferenceDistance = SOUND_REF_DISTANCE_DEFAULT;
    MaxDistance = SOUND_DISTANCE_DEFAULT;
    RolloffRate = SOUND_ROLLOFF_RATE_DEFAULT;
    ConeInnerAngle = 360;
    ConeOuterAngle = 360;
    bEmitterPaused = false;
    bVirtualizeWhenSilent = false;
    ResourceRevision = 0;

    Core::ZeroMem( &Channel, sizeof( Channel ) );
    Channel.LoopStart = -1;

    bCanEverTick = true;
}

void ASoundEmitter::InitializeComponent()
{
    Super::InitializeComponent();
}

void ASoundEmitter::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    ClearSound();
}

void ASoundEmitter::OnTransformDirty()
{
    Super::OnTransformDirty();
}

void ASoundEmitter::OnCreateAvatar()
{
    Super::OnCreateAvatar();

    // TODO: Create mesh or sprite for avatar
    static TStaticResourceFinder< AIndexedMesh > Mesh( _CTS( "/Default/Meshes/Sphere" ) );
    static TStaticResourceFinder< AMaterialInstance > MaterialInstance( _CTS( "AvatarMaterialInstance" ) );
    AMeshComponent * meshComponent = GetOwnerActor()->CreateComponent< AMeshComponent >( "SoundEmitterAvatar" );
    meshComponent->SetMotionBehavior( MB_KINEMATIC );
    meshComponent->SetCollisionGroup( CM_NOCOLLISION );
    meshComponent->SetMesh( Mesh.GetObject() );
    meshComponent->SetMaterialInstance( MaterialInstance.GetObject() );
    meshComponent->SetCastShadow( false );
    meshComponent->SetAbsoluteScale( true );
    meshComponent->SetAbsoluteRotation( true );
    meshComponent->SetScale( 0.1f );
    meshComponent->AttachTo( this );
    meshComponent->SetHideInEditor( true );
}

void ASoundEmitter::BeginPlay()
{
    INTRUSIVE_ADD( this, Next, Prev, SoundEmitters, SoundEmittersTail );

    if ( IsSilent() ) {
        SelectNextSound();
    }
}

void ASoundEmitter::EndPlay()
{
    INTRUSIVE_REMOVE( this, Next, Prev, SoundEmitters, SoundEmittersTail );
}

AN_FORCEINLINE float FalloffDistance( float MaxDistance )
{
    return MaxDistance * 1.3f;
}

void ASoundEmitter::PlaySound( ASoundResource * SoundResource, int StartFrame, int _LoopStart )
{
    if ( !IsInitialized() ) {
        GLogger.Printf( "ASoundEmitter::PlaySound: not initialized\n" );
        return;
    }

    ClearSound();

    if ( Client && Client->IsPendingKill() ) {
        // Client is dead
        return;
    }

    bool bLooped = _LoopStart >= 0;
    bool bShouldVirtualizeWhenSilent = bVirtualizeWhenSilent || bLooped;

    if ( EmitterType != SOUND_EMITTER_BACKGROUND && !bShouldVirtualizeWhenSilent ) {
        float maxDist = Math::Clamp( MaxDistance, ReferenceDistance, SOUND_DISTANCE_MAX );
        float falloff = FalloffDistance( maxDist );
        float cullDist = maxDist + falloff;

        SAudioListener const & listener = GAudioSystem.GetListener();

        if ( listener.Position.DistSqr( GetWorldPosition() ) >= cullDist * cullDist ) {
            // Sound is too far from listener
            return;
        }
    }

    Spatialize();

    if ( Channel.NewVol[0] == 0 && Channel.NewVol[1] == 0 ) {
        if ( !bShouldVirtualizeWhenSilent ) {
            // Don't even start
            return;
        }

        // Start virtualized
        Virtualize();
    }

    StartPlay( SoundResource, StartFrame, _LoopStart );
}

void ASoundEmitter::PlayOneShot( ASoundResource * SoundResource, float VolumeScale, bool bFixedPosition, int StartFrame )
{
    if ( !IsInitialized() ) {
        GLogger.Printf( "ASoundEmitter::PlayOneShot: not initialized\n" );
        return;
    }

    SSoundSpawnInfo spawnInfo;
    spawnInfo.EmitterType = EmitterType;
    spawnInfo.Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;
    spawnInfo.bVirtualizeWhenSilent = bVirtualizeWhenSilent;
    spawnInfo.bFollowInstigator = !bFixedPosition;
    spawnInfo.AudioClient = Client;
    spawnInfo.ListenerMask = ListenerMask;
    spawnInfo.Group = Group;
    spawnInfo.Attenuation.ReferenceDistance = ReferenceDistance;
    spawnInfo.Attenuation.Distance = MaxDistance;
    spawnInfo.Attenuation.RolloffRate = RolloffRate;
    spawnInfo.Volume = Volume * VolumeScale;
    spawnInfo.StartFrame = StartFrame;
    spawnInfo.bStopWhenInstigatorDead = false;//true;
    spawnInfo.ConeInnerAngle = ConeInnerAngle;
    spawnInfo.ConeOuterAngle = ConeOuterAngle;
    spawnInfo.Direction = GetWorldForwardVector();

    SpawnSound( SoundResource, GetWorldPosition(), GetWorld(), this, &spawnInfo );
}

void ASoundEmitter::PlaySoundAt( AWorld * World, ASoundResource * SoundResource, ASoundGroup * SoundGroup, Float3 const & Position, float Volume, int StartFrame )
{
    SSoundSpawnInfo spawnInfo;

    spawnInfo.EmitterType = SOUND_EMITTER_POINT;
    spawnInfo.Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;
//    spawnInfo.bVirtualizeWhenSilent = bVirtualizeWhenSilent;
//    spawnInfo.AudioClient = Client;
//    spawnInfo.ListenerMask = ListenerMask;
    spawnInfo.Group = SoundGroup;
//    spawnInfo.Attenuation.ReferenceDistance = ReferenceDistance;
//    spawnInfo.Attenuation.Distance = MaxDistance;
//    spawnInfo.Attenuation.RolloffRate = RolloffRate;
    spawnInfo.Volume = Volume;
    spawnInfo.StartFrame = StartFrame;

    SpawnSound( SoundResource, Position, World, nullptr, &spawnInfo );
}

void ASoundEmitter::PlaySoundBackground( AWorld * World, ASoundResource * SoundResource, ASoundGroup * SoundGroup, float Volume, int StartFrame )
{
    SSoundSpawnInfo spawnInfo;

    spawnInfo.EmitterType = SOUND_EMITTER_BACKGROUND;
    spawnInfo.Priority = AUDIO_CHANNEL_PRIORITY_ONESHOT;
//    spawnInfo.bVirtualizeWhenSilent = bVirtualizeWhenSilent;
//    spawnInfo.AudioClient = Client;
//    spawnInfo.ListenerMask = ListenerMask;
    spawnInfo.Group = SoundGroup;
    spawnInfo.Volume = Volume;
    spawnInfo.StartFrame = StartFrame;

    SpawnSound( SoundResource, Float3( 0.0f ), World, nullptr, &spawnInfo );
}

bool ASoundEmitter::StartPlay( ASoundResource * SoundResource, int StartFrame, int LoopStart )
{
    if ( !SoundResource ) {
        GLogger.Printf( "ASoundEmitter::StartPlay: No sound specified\n" );
        return false;
    }

    if ( SoundResource->GetFrameCount() == 0 ) {
        GLogger.Printf( "ASoundEmitter::StartPlay: Sound has no frames\n" );
        return false;
    }

    if ( LoopStart >= SoundResource->GetFrameCount() ) {
        LoopStart = 0;
    }

    if ( StartFrame < 0 ) {
        StartFrame = 0;
    }

    int loopsCount = 0;

    if ( StartFrame >= SoundResource->GetFrameCount() ) {
        if ( LoopStart < 0 ) {
            return false;
        }

        StartFrame = LoopStart;
        loopsCount++;
    }

    TRef< IAudioStream > streamInterface;

    // Initialize audio stream instance
    if ( SoundResource->GetStreamType() != SOUND_STREAM_DISABLED ) {
        if ( !SoundResource->CreateAudioStreamInstance( &streamInterface ) ) {
            GLogger.Printf( "ASoundEmitter::StartPlay: Couldn't create audio stream instance\n" );
            return false;
        }
    }

    Resource = SoundResource;
    ResourceRevision = Resource->GetRevision();
    StreamInterface = streamInterface;

    Channel.LoopStart = LoopStart;
    Channel.PlaybackPos = StartFrame;
    Channel.PlaybackEnd = 0; // Will be calculated by mixer
    Channel.LoopsCount = loopsCount;
    Channel.StreamInterface = StreamInterface.GetObject();    
    Channel.pRawSamples = SoundResource->GetRawSamples();
    Channel.FrameCount = SoundResource->GetFrameCount();
    Channel.Ch = SoundResource->GetChannels();
    Channel.SampleBits = SoundResource->GetSampleBits();
    Channel.SampleStride = SoundResource->GetSampleStride();

    if ( StreamInterface && !Channel.bVirtual ) {
        StreamInterface->SeekToFrame( StartFrame );
    }

    AddChannel( &Channel );

    return true;
}

bool ASoundEmitter::RestartSound()
{
    TRef< ASoundResource > newSound = Resource;

    RemoveChannel( &Channel );

    Resource.Reset();
    StreamInterface.Reset();

    return StartPlay( newSound.GetObject(), 0, Channel.LoopStart );
}

void ASoundEmitter::ClearSound()
{
    RemoveChannel( &Channel );
    Core::ZeroMem( &Channel, sizeof( Channel ) );

    Resource.Reset();
    StreamInterface.Reset();

    ClearQueue();
}

void ASoundEmitter::AddToQueue( ASoundResource * SoundResource )
{
    if ( !SoundResource ) {
        GLogger.Printf( "ASoundEmitter::AddToQueue: No sound specified\n" );
        return;
    }

    if ( SoundResource->GetFrameCount() == 0 ) {
        GLogger.Printf( "ASoundEmitter::AddToQueue: Sound has no frames\n" );
        return;
    }

    bool bPlayNow = IsInitialized() && IsSilent();

    if ( bPlayNow && AudioQueue.IsEmpty() ) {
        StartPlay( SoundResource, 0, -1 );
        return;
    }

    *AudioQueue.Push() = SoundResource;
    SoundResource->AddRef();

    if ( bPlayNow ) {
        SelectNextSound();
    }
}

bool ASoundEmitter::SelectNextSound()
{
    bool bSelected = false;

    RemoveChannel( &Channel );

    Resource.Reset();
    StreamInterface.Reset();

    while ( !AudioQueue.IsEmpty() && !bSelected ) {
        ASoundResource * playSound = *AudioQueue.Pop();

        bSelected = StartPlay( playSound, 0, -1 );

        playSound->RemoveRef();
    }

    return bSelected;
}

void ASoundEmitter::ClearQueue()
{
    while ( !AudioQueue.IsEmpty() ) {
        ASoundResource * sound = *AudioQueue.Pop();
        sound->RemoveRef();
    }
}

void ASoundEmitter::Virtualize()
{
    if ( Channel.bVirtual ) {
        return;
    }

    // ... do something here ...

    Channel.bVirtual = true;
}

void ASoundEmitter::Devirtualize()
{
    if ( !Channel.bVirtual ) {
        return;
    }

    if ( StreamInterface ) {
        StreamInterface->SeekToFrame( Channel.PlaybackPos );
    }

    Channel.bVirtual = false;
}

void ASoundEmitter::Update()
{
    if ( !Resource ) {
        // silent
        return;
    }

    // Check if the audio clip has been modified
    if ( ResourceRevision != Resource->GetRevision() ) {
        if ( !RestartSound() ) {
            // couldn't restart
            return;
        }
    }

    // Select next sound from queue if playback position is reached the end
    if ( Channel.PlaybackPos >= Resource->GetFrameCount() ) {
        if ( !SelectNextSound() ) {
            return;
        }
    }

    bool paused = bEmitterPaused;

    bool bPlayEvenWhenPaused = Group ? Group->ShouldPlayEvenWhenPaused() : false;

    if ( !bPlayEvenWhenPaused ) {
        paused = paused || GetWorld()->IsPaused();
    }

    if ( Group ) {
        paused = paused || Group->IsPaused();
    }

    // Check unpaused
    if ( !paused && Channel.bPaused ) {
        // fade out
        Channel.CurVol[0] = 0;
        Channel.CurVol[1] = 0;
    }

    Channel.bPaused = paused;

    if ( Channel.bPaused ) {
        // fade in
        Channel.NewVol[0] = 0;
        Channel.NewVol[1] = 0;

        // Channel is really paused if current volume is zero
        if ( Channel.CurVol[0] == 0 && Channel.CurVol[1] == 0 ) {
            Virtualize();
            return;
        }
    }

    if ( !Channel.bPaused ) {
        Spatialize();
    }

    if ( Channel.NewVol[0] == 0 && Channel.NewVol[1] == 0
         && Channel.CurVol[0] == 0 && Channel.CurVol[1] == 0 ) {
        if ( !Channel.bVirtual ) {
            bool bLooped = Channel.LoopStart >= 0;
            if ( bVirtualizeWhenSilent || bLooped ) {
                Virtualize();
            }
            else {
                ClearSound();
                return;
            }
        }
    }
    else {
        Devirtualize();
    }

#if 0
    if ( IsVirtual() ) {
        // Update playback position
        VirtualTime += TimeStep;

        if ( VirtualTime >= Clip->GetDurationInSecounds() ) {
            if ( Channel.LoopStart >= 0 ) {
                VirtualTime = Channel.LoopStart / GAudioSystem.pPlaybackDevice->GetSampleRate(); //Clip->GetFrequency();
                Channel.LoopsCount++;
            }
            else {
                // Stopped
                FreeChannel( Chan );
                return;
            }
        }
    }
#endif
}

static void CalcAttenuation( ESoundEmitterType EmitterType,
                             Float3 const & SoundPosition, Float3 const & SoundDirection,
                             Float3 const & ListenerPosition, Float3 const & ListenerRightVec,
                             float ReferenceDistance, float MaxDistance, float RolloffRate,
                             float ConeInnerAngle, float ConeOuterAngle,
                             float * pLeftVol, float * pRightVol )
{
    Float3 dir = SoundPosition - ListenerPosition;
    float distance = dir.NormalizeSelf();
    float attenuation = 1;

    // Cone attenuation
    if ( EmitterType == SOUND_EMITTER_DIRECTIONAL && ConeInnerAngle < 360.0f ) {
        float angle = -2.0f * Math::Degrees( std::acos( Math::Dot( SoundDirection, dir ) ) );
        float angleInterval = ConeOuterAngle - ConeInnerAngle;

        if ( angle > ConeInnerAngle ) {
            if ( angleInterval > 0.0f ) {
                //attenuation = std::powf( 1.0f - Math::Clamp( angle - ConeInnerAngle, 0.0f, angleInterval ) / angleInterval, RolloffFactor );
                attenuation = 1.0f - (angle - ConeInnerAngle) / angleInterval;
            }
            else {
                attenuation = 0;
            }
        }
    }

    // Calc clamped distance
    float d = Math::Clamp( distance, ReferenceDistance, MaxDistance );

    // Linear distance clamped model
    //attenuation *= ( 1.0f - RolloffRate * (d - ReferenceDistance) / (MaxDistance - ReferenceDistance) );

    // Inverse distance clamped model
    attenuation *= ReferenceDistance / ( ReferenceDistance + RolloffRate * ( d - ReferenceDistance ) );

    // Falloff
    distance -= MaxDistance;
    if ( distance > 0 ) {
        const float falloff = FalloffDistance( MaxDistance );
        if ( distance >= falloff ) {
            attenuation = 0;
        }
        else {
            attenuation *= 1.0f - distance / falloff;
        }
    }

    // Panning
    if ( Snd_HRTF || GAudioSystem.IsMono() ) {
        *pLeftVol = *pRightVol = attenuation;
    }
    else {
        float panning = Math::Dot( ListenerRightVec, dir );

        float leftPan = 1.0f - panning;
        float rightPan = 1.0f + panning;

        *pLeftVol = attenuation * leftPan;
        *pRightVol = attenuation * rightPan;
    }
}

void ASoundEmitter::Spatialize()
{
    SAudioListener const & listener = GAudioSystem.GetListener();

    Channel.NewVol[0] = 0;
    Channel.NewVol[1] = 0;

    // Cull if muted
    if ( bMuted ) {
        return;
    }

    // Cull by client
    if ( Client && listener.Id != Client->Id ) {
        return;
    }

    // Cull by mask
    if ( ( ListenerMask & listener.Mask ) == 0 ) {
        return;
    }

    float volume = Volume;
    volume *= listener.VolumeScale;
    volume *= GetWorld()->GetAudioVolume();
    if ( Group ) {
        volume *= Group->GetVolume();
    }

    // Cull by volume
    if ( volume < 0.0001f ) {
        return;
    }

    // Don't be too lound
    if ( volume > 1.0f ) {
        volume = 1.0f;
    }

    const float VolumeFtoI = 65536;

    volume *= VolumeFtoI;

    // If the sound is played from the listener, consider it as background
    if ( EmitterType == SOUND_EMITTER_BACKGROUND || GetOwnerActor()->Id == listener.Id ) {
        // Use full volume without attenuation
        Channel.NewVol[0] = Channel.NewVol[1] = (int)volume;

        // Don't spatialize stereo sounds
        Channel.bSpatializedStereo = false;
        return;
    }

    Float3 soundPosition = GetWorldPosition();

    float leftVol, rightVol;

    CalcAttenuation( EmitterType, soundPosition, GetWorldForwardVector(), listener.Position, listener.RightVec, ReferenceDistance, MaxDistance, RolloffRate, ConeInnerAngle, ConeOuterAngle, &leftVol, &rightVol );

    Channel.NewVol[0] = (int)(volume * leftVol);
    Channel.NewVol[1] = (int)(volume * rightVol);

    // Should never happen, but just in case
    if ( Channel.NewVol[0] < 0 )
        Channel.NewVol[0] = 0;
    if ( Channel.NewVol[1] < 0 )
        Channel.NewVol[1] = 0;
    if ( Channel.NewVol[0] > 65536 )
        Channel.NewVol[0] = 65536;
    if ( Channel.NewVol[1] > 65536 )
        Channel.NewVol[1] = 65536;

    Channel.bSpatializedStereo = !GAudioSystem.IsMono();

    if ( Snd_HRTF ) {
        Channel.NewDir = (listener.TransformInv * soundPosition).Normalized();
    }
}

void ASoundEmitter::SetSoundGroup( ASoundGroup * SoundGroup )
{
    Group = SoundGroup;
}

void ASoundEmitter::SetAudioClient( APawn * AudioClient )
{
    Client = AudioClient;
}

void ASoundEmitter::SetListenerMask( uint32_t Mask )
{
    ListenerMask = Mask;
}

void ASoundEmitter::SetEmitterType( ESoundEmitterType _EmitterType )
{
    EmitterType = _EmitterType;
}

void ASoundEmitter::SetVirtualizeWhenSilent( bool _bVirtualizeWhenSilent )
{
    bVirtualizeWhenSilent = _bVirtualizeWhenSilent;
}

void ASoundEmitter::SetVolume( float _Volume )
{
    Volume = Math::Saturate( _Volume );
}

void ASoundEmitter::SetReferenceDistance( float Dist )
{
    ReferenceDistance = Math::Clamp( Dist, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX );
}

void ASoundEmitter::SetMaxDistance( float Dist )
{
    MaxDistance = Math::Clamp( Dist, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX );
}

void ASoundEmitter::SetRolloffRate( float Rolloff )
{
    RolloffRate = Math::Clamp( Rolloff, 0.0f, 1.0f );
}

void ASoundEmitter::SetConeInnerAngle( float Angle )
{
    ConeInnerAngle = Math::Clamp( Angle, 0.0f, 360.0f );
}

void ASoundEmitter::SetConeOuterAngle( float Angle )
{
    ConeOuterAngle = Math::Clamp( Angle, 0.0f, 360.0f );
}

void ASoundEmitter::SetPaused( bool _bPaused )
{
    bEmitterPaused = _bPaused;
}

void ASoundEmitter::SetPlaybackPosition( int FrameNum )
{
    if ( !Resource ) {
        return;
    }

    if ( Channel.PlaybackPos == FrameNum ) {
        return;
    }

    Channel.PlaybackPos = Math::Clamp( FrameNum, 0, Resource->GetFrameCount() );

    if ( StreamInterface && !Channel.bVirtual ) {
        StreamInterface->SeekToFrame( Channel.PlaybackPos );
    }
}

int ASoundEmitter::GetPlaybackPosition() const
{
    return Channel.PlaybackPos;
}

void ASoundEmitter::SetPlaybackTime( float Time )
{
    AAudioDevice * device = GAudioSystem.GetPlaybackDevice();

    int frameNum = Math::Round( Time * device->GetSampleRate() );

    SetPlaybackPosition( frameNum );
}

float ASoundEmitter::GetPlaybackTime() const
{
    AAudioDevice * device = GAudioSystem.GetPlaybackDevice();

    return (float)Channel.PlaybackPos / device->GetSampleRate();
}

void ASoundEmitter::SetMuted( bool _bMuted )
{
    bMuted = _bMuted;
}

bool ASoundEmitter::IsMuted() const
{
    return bMuted;
}

bool ASoundEmitter::IsVirtual() const
{
    return Channel.bVirtual;
}

bool ASoundEmitter::IsSilent() const
{
    return !Resource.GetObject();
}

void ASoundEmitter::SpawnSound( ASoundResource * SoundResource, Float3 const & SpawnPosition, AWorld * World, ASceneComponent * Instigator, SSoundSpawnInfo const * SpawnInfo )
{
    static SSoundSpawnInfo DefaultSoundSpawnInfo;

    if ( !SpawnInfo ) {
        SpawnInfo = &DefaultSoundSpawnInfo;
    }

    if ( !SoundResource ) {
        GLogger.Printf( "ASoundEmitter::SpawnSound: No sound specified\n" );
        return;
    }

    if ( SoundResource->GetFrameCount() == 0 ) {
        GLogger.Printf( "ASoundEmitter::SpawnSound: Sound has no frames\n" );
        return;
    }

    int startFrame = SpawnInfo->StartFrame;

    if ( startFrame < 0 ) {
        startFrame = 0;
    }

    if ( startFrame >= SoundResource->GetFrameCount() ) {
        return;
    }

    if ( SpawnInfo->AudioClient && SpawnInfo->AudioClient->IsPendingKill() ) {
        return;
    }

    SSoundAttenuationParameters const & atten = SpawnInfo->Attenuation;

    float refDist = Math::Clamp( atten.ReferenceDistance, SOUND_DISTANCE_MIN, SOUND_DISTANCE_MAX );
    float maxDist = Math::Clamp( atten.Distance, refDist, SOUND_DISTANCE_MAX );
    float falloff = FalloffDistance( maxDist );

    if ( SpawnInfo->EmitterType != SOUND_EMITTER_BACKGROUND && !SpawnInfo->bVirtualizeWhenSilent ) {
        SAudioListener const & listener = GAudioSystem.GetListener();
        float cullDist = maxDist + falloff;

        if ( listener.Position.DistSqr( SpawnPosition ) >= cullDist * cullDist ) {
            // Sound is too far from listener
            return;
        }
    }

    TRef< IAudioStream > streamInterface;

    // Initialize audio stream instance
    if ( SoundResource->GetStreamType() != SOUND_STREAM_DISABLED ) {
        if ( !SoundResource->CreateAudioStreamInstance( &streamInterface ) ) {
            GLogger.Printf( "Couldn't create audio stream instance\n" );
            return;
        }
    }

    auto & pool = GAudioSystem.GetChannelPool();

    ASoundOneShot * sound = new ( pool.Allocate() ) ASoundOneShot;
    sound->Volume = Math::Saturate( SpawnInfo->Volume );
    sound->ReferenceDistance = refDist;
    sound->MaxDistance = maxDist;
    sound->RolloffRate = Math::Saturate( atten.RolloffRate );
    sound->bStopWhenInstigatorDead = Instigator && SpawnInfo->bStopWhenInstigatorDead;
    sound->EmitterType = SpawnInfo->EmitterType;
    sound->Resource = SoundResource;
    sound->ResourceRevision = SoundResource->GetRevision();
    sound->StreamInterface = streamInterface;
    sound->Priority = SpawnInfo->Priority;
    sound->bFollowInstigator = SpawnInfo->bFollowInstigator;
    if ( SpawnInfo->EmitterType == SOUND_EMITTER_DIRECTIONAL ) {
        sound->ConeInnerAngle = Math::Clamp( SpawnInfo->ConeInnerAngle, 0.0f, 360.0f );
        sound->ConeOuterAngle = Math::Clamp( SpawnInfo->ConeOuterAngle, SpawnInfo->ConeInnerAngle, 360.0f );

        if ( SpawnInfo->bFollowInstigator ) {
            sound->SoundDirection = Instigator ? Instigator->GetWorldForwardVector() : SpawnInfo->Direction;
        } else {
            sound->SoundDirection = SpawnInfo->Direction;
        }
    }
    sound->AudioClient = SpawnInfo->AudioClient ? SpawnInfo->AudioClient->Id : 0;
    sound->ListenerMask = SpawnInfo->ListenerMask;
    sound->Group = SpawnInfo->Group;
    sound->Instigator = Instigator;
    sound->InstigatorId = Instigator ? Instigator->GetOwnerActor()->Id : 0;
    sound->World = World;
    sound->SoundPosition = SpawnPosition;
    sound->bVirtualizeWhenSilent = SpawnInfo->bVirtualizeWhenSilent;
    sound->Channel.PlaybackPos = startFrame;
    sound->Channel.LoopStart = -1;
    sound->Channel.StreamInterface = streamInterface.GetObject();
    sound->Channel.pRawSamples = SoundResource->GetRawSamples();
    sound->Channel.FrameCount = SoundResource->GetFrameCount();
    sound->Channel.Ch = SoundResource->GetChannels();
    sound->Channel.SampleBits = SoundResource->GetSampleBits();
    sound->Channel.SampleStride = SoundResource->GetSampleStride();
    sound->Channel.bPaused = false;

    sound->Spatialize();

    if ( sound->Channel.NewVol[0] == 0 && sound->Channel.NewVol[1] == 0 ) {
        Virtualize( sound );
    }

    if ( sound->StreamInterface && !sound->Channel.bVirtual ) {
        sound->StreamInterface->SeekToFrame( sound->Channel.PlaybackPos );
    }

    INTRUSIVE_ADD( sound, Next, Prev, OneShots, OneShotsTail );

    AddChannel( &sound->Channel );
}

void ASoundEmitter::ClearOneShotSounds()
{
    ASoundOneShot * next;

    for ( ASoundOneShot * sound = OneShots ; sound ; sound = next ) {
        next = sound->Next;

        FreeSound( sound );
    }

    AN_ASSERT( OneShots == nullptr );
}

void ASoundEmitter::FreeSound( ASoundOneShot * Sound )
{
    RemoveChannel( &Sound->Channel );

    INTRUSIVE_REMOVE( Sound, Next, Prev, OneShots, OneShotsTail );

    Sound->~ASoundOneShot();

    auto & pool = GAudioSystem.GetChannelPool();
    pool.Deallocate( Sound );
}

void ASoundEmitter::Virtualize( ASoundOneShot * Sound )
{
    if ( Sound->Channel.bVirtual ) {
        return;
    }

    Sound->Channel.bVirtual = true;
}

void ASoundEmitter::Devirtualize( ASoundOneShot * Sound )
{
    if ( !Sound->Channel.bVirtual ) {
        return;
    }

    Sound->Channel.bVirtual = false;

    if ( Sound->StreamInterface ) {
        Sound->StreamInterface->SeekToFrame( Sound->Channel.PlaybackPos );
    }
}

void ASoundEmitter::Update( ASoundOneShot * Sound )
{
    SAudioChannel * chan = &Sound->Channel;

    // Check if the instigator is still alive
    if ( Sound->bStopWhenInstigatorDead && Sound->Instigator && Sound->Instigator->IsPendingKill() ) {
        FreeSound( Sound );
        return;
    }

    // Check if the audio clip has been modified
    if ( Sound->ResourceRevision != Sound->Resource->GetRevision() ) {
        FreeSound( Sound );
        return;
    }

    // Free the channel if the sound stops
    if ( chan->PlaybackPos >= chan->FrameCount ) {
        FreeSound( Sound );
        return;
    }

    // Update position, direction, velocity
    if ( Sound->bFollowInstigator && Sound->Instigator && !Sound->Instigator->IsPendingKill() ) {
        Sound->SoundPosition = Sound->Instigator->GetWorldPosition();

        if ( Sound->EmitterType == SOUND_EMITTER_DIRECTIONAL ) {
            Sound->SoundDirection = Sound->Instigator->GetWorldForwardVector();
        }
    }


    AWorld * world = Sound->World;

    bool bPlayEvenWhenPaused = Sound->Group ? Sound->Group->ShouldPlayEvenWhenPaused() : false;

    bool paused = false;
    if ( world && !bPlayEvenWhenPaused ) {
        paused = world->IsPaused();
    }
    if ( Sound->Group ) {
        paused = paused || Sound->Group->IsPaused();
    }

    // Check unpaused
    if ( !paused && chan->bPaused ) {
        // fade out
        chan->CurVol[0] = 0;
        chan->CurVol[1] = 0;
    }

    chan->bPaused = paused;

    if ( chan->bPaused ) {
        // fade in
        chan->NewVol[0] = 0;
        chan->NewVol[1] = 0;

        // Channel is really paused if current volume is zero
        if ( chan->CurVol[0] == 0 && chan->CurVol[1] == 0 ) {
            Virtualize( Sound );
            return;
        }
    }
    
    if ( !chan->bPaused ) {
        Sound->Spatialize();
    }

    if ( chan->NewVol[0] == 0 && chan->NewVol[1] == 0
         && chan->CurVol[0] == 0 && chan->CurVol[1] == 0 ) {
        if ( !chan->bVirtual ) {
            if ( Sound->bVirtualizeWhenSilent ) {
                Virtualize( Sound );
            }
            else {
                FreeSound( Sound );
                return;
            }
        }
    }
    else {
        Devirtualize( Sound );
    }

#if 0
    if ( Sound->IsVirtual() ) {
        // Update playback position
        Sound->VirtualTime += TimeStep;

        if ( Sound->VirtualTime >= chan->Clip->GetDurationInSecounds() ) {
            // Stopped
            FreeSound( Sound );
            return;
        }
    }
#endif
}

void ASoundOneShot::Spatialize()
{
    SAudioListener const & listener = GAudioSystem.GetListener();

    Channel.NewVol[0] = 0;
    Channel.NewVol[1] = 0;

    // Cull by client
    if ( AudioClient && listener.Id != AudioClient ) {
        return;
    }

    // Cull by mask
    if ( ( ListenerMask & listener.Mask ) == 0 ) {
        return;
    }

    float volume = Volume;
    volume *= listener.VolumeScale;
    if ( World ) {
        volume *= World->GetAudioVolume();
    }
    if ( Group ) {
        volume *= Group->GetVolume();
    }

    // Cull by volume
    if ( volume < 0.0001f ) {
        return;
    }

    // Don't be too lound
    if ( volume > 1.0f ) {
        volume = 1.0f;
    }

    const float VolumeFtoI = 65536;

    volume *= VolumeFtoI;

    // If the sound is played from the listener, consider it as background
    if ( EmitterType == SOUND_EMITTER_BACKGROUND || ( bFollowInstigator && InstigatorId == listener.Id ) ) {
        // Use full volume without attenuation
        Channel.NewVol[0] = Channel.NewVol[1] = (int)volume;

        // Don't spatialize stereo sounds
        Channel.bSpatializedStereo = false;
        return;
    }

    float leftVol, rightVol;

    CalcAttenuation( EmitterType, SoundPosition, SoundDirection, listener.Position, listener.RightVec, ReferenceDistance, MaxDistance, RolloffRate, ConeInnerAngle, ConeOuterAngle, &leftVol, &rightVol );

    Channel.NewVol[0] = (int)(volume * leftVol);
    Channel.NewVol[1] = (int)(volume * rightVol);

    // Should never happen, but just in case
    if ( Channel.NewVol[0] < 0 )
        Channel.NewVol[0] = 0;
    if ( Channel.NewVol[1] < 0 )
        Channel.NewVol[1] = 0;
    if ( Channel.NewVol[0] > 65536 )
        Channel.NewVol[0] = 65536;
    if ( Channel.NewVol[1] > 65536 )
        Channel.NewVol[1] = 65536;

    Channel.bSpatializedStereo = !GAudioSystem.IsMono();

    if ( Snd_HRTF ) {
        Channel.NewDir = (listener.TransformInv * SoundPosition).Normalized();
    }
}

void ASoundEmitter::UpdateSounds()
{
    ASoundOneShot * next;

    for ( ASoundOneShot * sound = OneShots ; sound ; sound = next ) {
        next = sound->Next;
        Update( sound );
    }

    for ( ASoundEmitter * e = SoundEmitters ; e ; e = e->GetNext() ) {
        e->Update();
    }
}
