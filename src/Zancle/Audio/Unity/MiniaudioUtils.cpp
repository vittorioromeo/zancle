// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Priv/MiniaudioUtils.hpp"

#include "Zancle/Audio/AudioSettings.hpp"
#include "Zancle/Audio/ChannelMap.hpp"
#include "Zancle/Audio/EffectProcessor.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/Priv/SoundBase.hpp"
#include "Zancle/Audio/SoundChannel.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Base/IntTypes.hpp"

#ifdef ZA_ENABLE_LIFETIME_TRACKING
    #include "Zancle/Lifetime/LifetimeDependee.hpp"
#endif

#include "Zancle/Container/InPlaceVector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/OffsetOf.hpp"

#include <miniaudio.h>


namespace za::priv
{
////////////////////////////////////////////////////////////
void MiniaudioUtils::SoundBase::nodeOnProcess(
    ma_node* const      node,
    const float** const framesIn,
    ma_uint32* const    frameCountIn,
    float** const       framesOut,
    ma_uint32* const    frameCountOut)
{
    SoundBase& impl = *(reinterpret_cast<SoundBase*>(static_cast<char*>(node) - ZA_OFFSETOF(SoundBase, effectNode)));

    // Assuming that `onProcess` is never called after the destructor of `SoundBase` is finished
    ZA_ASSERT(!impl.effectNodeUninitialized);
    impl.processEffect(framesIn, *frameCountIn, framesOut, *frameCountOut);
}


////////////////////////////////////////////////////////////
void MiniaudioUtils::SoundBase::processEffect(const float** const framesIn,
                                              za::U32&            frameCountIn,
                                              float** const       framesOut,
                                              za::U32&            frameCountOut)
{
    // If a processor is set, call it
    if (effectProcessor)
    {
        if (!framesIn)
            frameCountIn = 0;

        effectProcessor(framesIn ? framesIn[0] : nullptr, frameCountIn, framesOut[0], frameCountOut, effectNode.channelCount);
        return;
    }

    // Otherwise just pass the data through 1:1
    if (framesIn == nullptr)
    {
        frameCountIn  = 0u;
        frameCountOut = 0u;
        return;
    }

    const auto toProcess = za::min(frameCountIn, frameCountOut);
    ZA_MEMCPY(framesOut[0], framesIn[0], toProcess * effectNode.channelCount * sizeof(float));
    frameCountIn  = toProcess;
    frameCountOut = toProcess;
}


////////////////////////////////////////////////////////////
void MiniaudioUtils::SoundBase::setChannelMap(const ChannelMap& channelMap)
{
    static_assert(SoundChannelCount <= static_cast<unsigned int>(MA_CHANNEL_POSITION_COUNT),
                  "`soundChannelMap` must be able to hold every channel of a `ChannelMap`");

    soundChannelMap.clear();

    for (const SoundChannel channel : channelMap)
        soundChannelMap.unsafeEmplaceBack(priv::MiniaudioUtils::soundChannelToMiniaudioChannel(channel));

    // We don't need to set the pointer again as it's stable
}


////////////////////////////////////////////////////////////
MiniaudioUtils::SoundBase::SoundBase(PlaybackDevice&   thePlaybackDevice,
                                     const void* const dataSourceVTable,
                                     const ChannelMap& channelMap) :
    dataSourceBase{}, // must be first member!
    playbackDevice(&thePlaybackDevice)
{
    ZA_UPDATE_LIFETIME_DEPENDANT(PlaybackDevice, SoundBase, this, playbackDevice);

    setChannelMap(channelMap);

    // Set this object up as a miniaudio data source
    ma_data_source_config config = ma_data_source_config_init();
    config.vtable                = static_cast<const ma_data_source_vtable*>(dataSourceVTable);

    if (const ma_result result = ma_data_source_init(&config, &dataSourceBase); result != MA_SUCCESS)
        fail("initialize audio data source", result);
}


////////////////////////////////////////////////////////////
void MiniaudioUtils::SoundBase::uninitSound()
{
    // `ma_sound_uninit` detaches the sound from the engine graph and
    // synchronizes with the audio thread, ensuring the read callback is
    // not in flight after this call returns. Derived `Impl` types should
    // call this explicitly before any data their read callback depends
    // on is destroyed (member destruction order would otherwise free
    // those buffers before this destructor runs).
    if (soundUninitialized)
        return;

    ma_sound_uninit(&sound);
    soundUninitialized = true;
}


////////////////////////////////////////////////////////////
MiniaudioUtils::SoundBase::~SoundBase()
{
#ifdef ZA_ENABLE_LIFETIME_TRACKING
    // When the owning sound object fails a lifetime tracking test, its dependee
    // has already been destroyed. We must still detach from the engine graph
    // (otherwise the engine will access freed node memory during its own
    // destruction), but skip `ma_data_source_uninit` as the data source may
    // reference the freed dependee's memory.

    if (priv::LifetimeDependee::TestingModeGuard::fatalErrorTriggered("SoundBuffer") ||
        priv::LifetimeDependee::TestingModeGuard::fatalErrorTriggered("MusicReader"))
    {
        uninitSound();

        ma_node_uninit(&effectNode, nullptr);
        effectNodeUninitialized = true;

        return;
    }
#endif

    uninitSound();

    ma_node_uninit(&effectNode, nullptr);
    effectNodeUninitialized = true; // Only for debugging

    ma_data_source_uninit(&dataSourceBase);
}


////////////////////////////////////////////////////////////
bool MiniaudioUtils::SoundBase::initialize(ma_sound_end_proc endCallback)
{
    // Get the engine
    auto* engine = static_cast<ma_engine*>(playbackDevice->getMAEngine());

    // Initialize the sound
    ma_sound_config soundConfig      = ma_sound_config_init();
    soundConfig.pDataSource          = this;
    soundConfig.pEndCallbackUserData = this;
    soundConfig.endCallback          = endCallback;

    if (const ma_result result = ma_sound_init_ex(engine, &soundConfig, &sound); result != MA_SUCCESS)
        return fail("initialize sound", result);

    sound.engineNode.spatializer.pChannelMapIn = soundChannelMap.data();

    // Initialize the custom effect node
    const auto nodeChannelCount = ma_engine_get_channels(engine);

    ma_node_config nodeConfig  = ma_node_config_init();
    nodeConfig.vtable          = &SoundBase::effectNodeVTable;
    nodeConfig.pInputChannels  = &nodeChannelCount;
    nodeConfig.pOutputChannels = &nodeChannelCount;

    if (const ma_result result = ma_node_init(ma_engine_get_node_graph(engine), &nodeConfig, nullptr, &effectNode);
        result != MA_SUCCESS)
        return fail("initialize effect node", result);

    effectNode.channelCount = nodeChannelCount;

    // Route the sound through the effect node depending on whether an effect processor is set
    if (!connectEffect(bool{effectProcessor}))
    {
        priv::errMsg("Failed to connect effect node");
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////
bool MiniaudioUtils::SoundBase::connectEffect(const bool connect)
{
    auto* engine = static_cast<ma_engine*>(playbackDevice->getMAEngine());

    if (connect)
    {
        // Attach the custom effect node output to our engine endpoint
        if (const ma_result result = ma_node_attach_output_bus(&effectNode, 0, ma_engine_get_endpoint(engine), 0);
            result != MA_SUCCESS)
            return fail("attach effect node output to endpoint", result);
    }
    else
    {
        // Detach the custom effect node output from our engine endpoint
        if (const ma_result result = ma_node_detach_output_bus(&effectNode, 0); result != MA_SUCCESS)
            return fail("detach effect node output from endpoint", result);
    }

    // Attach the sound output to the custom effect node or the engine endpoint
    if (const ma_result result = ma_node_attach_output_bus(&sound, 0, connect ? &effectNode : ma_engine_get_endpoint(engine), 0);
        result != MA_SUCCESS)
        return fail("attach sound node output to effect node", result);

    return true;
}


////////////////////////////////////////////////////////////
ma_sound& MiniaudioUtils::SoundBase::getSound()
{
    return sound;
}


////////////////////////////////////////////////////////////
bool MiniaudioUtils::SoundBase::setAndConnectEffectProcessor(const EffectProcessor& theEffectProcessor)
{
    effectProcessor = theEffectProcessor;
    return connectEffect(bool{effectProcessor});
}


////////////////////////////////////////////////////////////
void MiniaudioUtils::SoundBase::applyAudioSettings(const AudioSettings& audioSettings)
{
    const auto& s = audioSettings;

    ma_sound_set_cone(&sound, s.cone.innerAngle.asRadians(), s.cone.outerAngle.asRadians(), s.cone.outerGain);
    ma_sound_set_position(&sound, s.position.x, s.position.y, s.position.z);
    ma_sound_set_direction(&sound, s.direction.x, s.direction.y, s.direction.z);
    ma_sound_set_velocity(&sound, s.velocity.x, s.velocity.y, s.velocity.z);
    ma_sound_set_pitch(&sound, s.pitch);
    ma_sound_set_pan(&sound, s.pan);
    ma_sound_set_volume(&sound, s.volume);
    ma_sound_set_directional_attenuation_factor(&sound, s.directionalAttenuationFactor);
    ma_sound_set_doppler_factor(&sound, s.dopplerFactor);
    ma_sound_set_min_distance(&sound, s.minDistance);
    ma_sound_set_max_distance(&sound, s.maxDistance);
    ma_sound_set_min_gain(&sound, s.minGain);
    ma_sound_set_max_gain(&sound, s.maxGain);
    ma_sound_set_rolloff(&sound, s.attenuation);
    ma_sound_set_positioning(&sound, static_cast<ma_positioning>(s.positioning));
    ma_sound_set_looping(&sound, s.looping);
    ma_sound_set_spatialization_enabled(&sound, s.spatializationEnabled);
}


////////////////////////////////////////////////////////////
za::U8 MiniaudioUtils::soundChannelToMiniaudioChannel(const SoundChannel soundChannel)
{
    switch (soundChannel)
    {
            // clang-format off
        case SoundChannel::Unspecified:         return za::U8{MA_CHANNEL_NONE};
        case SoundChannel::Mono:                return za::U8{MA_CHANNEL_MONO};
        case SoundChannel::FrontLeft:           return za::U8{MA_CHANNEL_FRONT_LEFT};
        case SoundChannel::FrontRight:          return za::U8{MA_CHANNEL_FRONT_RIGHT};
        case SoundChannel::FrontCenter:         return za::U8{MA_CHANNEL_FRONT_CENTER};
        case SoundChannel::FrontLeftOfCenter:   return za::U8{MA_CHANNEL_FRONT_LEFT_CENTER};
        case SoundChannel::FrontRightOfCenter:  return za::U8{MA_CHANNEL_FRONT_RIGHT_CENTER};
        case SoundChannel::LowFrequencyEffects: return za::U8{MA_CHANNEL_LFE};
        case SoundChannel::BackLeft:            return za::U8{MA_CHANNEL_BACK_LEFT};
        case SoundChannel::BackRight:           return za::U8{MA_CHANNEL_BACK_RIGHT};
        case SoundChannel::BackCenter:          return za::U8{MA_CHANNEL_BACK_CENTER};
        case SoundChannel::SideLeft:            return za::U8{MA_CHANNEL_SIDE_LEFT};
        case SoundChannel::SideRight:           return za::U8{MA_CHANNEL_SIDE_RIGHT};
        case SoundChannel::TopCenter:           return za::U8{MA_CHANNEL_TOP_CENTER};
        case SoundChannel::TopFrontLeft:        return za::U8{MA_CHANNEL_TOP_FRONT_LEFT};
        case SoundChannel::TopFrontRight:       return za::U8{MA_CHANNEL_TOP_FRONT_RIGHT};
        case SoundChannel::TopFrontCenter:      return za::U8{MA_CHANNEL_TOP_FRONT_CENTER};
        case SoundChannel::TopBackLeft:         return za::U8{MA_CHANNEL_TOP_BACK_LEFT};
        case SoundChannel::TopBackRight:        return za::U8{MA_CHANNEL_TOP_BACK_RIGHT};
            // clang-format on

        default:
            ZA_ASSERT(soundChannel == SoundChannel::TopBackCenter);
            return za::U8{MA_CHANNEL_TOP_BACK_CENTER};
    }
}


////////////////////////////////////////////////////////////
SoundChannel MiniaudioUtils::miniaudioChannelToSoundChannel(const za::U8 soundChannel)
{
    const ma_channel maChannel{soundChannel};

    switch (maChannel)
    {
            // clang-format off
        case MA_CHANNEL_NONE:               return SoundChannel::Unspecified;
        case MA_CHANNEL_MONO:               return SoundChannel::Mono;
        case MA_CHANNEL_FRONT_LEFT:         return SoundChannel::FrontLeft;
        case MA_CHANNEL_FRONT_RIGHT:        return SoundChannel::FrontRight;
        case MA_CHANNEL_FRONT_CENTER:       return SoundChannel::FrontCenter;
        case MA_CHANNEL_FRONT_LEFT_CENTER:  return SoundChannel::FrontLeftOfCenter;
        case MA_CHANNEL_FRONT_RIGHT_CENTER: return SoundChannel::FrontRightOfCenter;
        case MA_CHANNEL_LFE:                return SoundChannel::LowFrequencyEffects;
        case MA_CHANNEL_BACK_LEFT:          return SoundChannel::BackLeft;
        case MA_CHANNEL_BACK_RIGHT:         return SoundChannel::BackRight;
        case MA_CHANNEL_BACK_CENTER:        return SoundChannel::BackCenter;
        case MA_CHANNEL_SIDE_LEFT:          return SoundChannel::SideLeft;
        case MA_CHANNEL_SIDE_RIGHT:         return SoundChannel::SideRight;
        case MA_CHANNEL_TOP_CENTER:         return SoundChannel::TopCenter;
        case MA_CHANNEL_TOP_FRONT_LEFT:     return SoundChannel::TopFrontLeft;
        case MA_CHANNEL_TOP_FRONT_RIGHT:    return SoundChannel::TopFrontRight;
        case MA_CHANNEL_TOP_FRONT_CENTER:   return SoundChannel::TopFrontCenter;
        case MA_CHANNEL_TOP_BACK_LEFT:      return SoundChannel::TopBackLeft;
        case MA_CHANNEL_TOP_BACK_RIGHT:     return SoundChannel::TopBackRight;
            // clang-format on

        default:
            ZA_ASSERT(maChannel == MA_CHANNEL_TOP_BACK_CENTER);
            return SoundChannel::TopBackCenter;
    }
}


////////////////////////////////////////////////////////////
za::Optional<Time> MiniaudioUtils::getPlayingOffset(ma_sound& sound)
{
    float cursor = 0.f;

    if (const ma_result result = ma_sound_get_cursor_in_seconds(&sound, &cursor); result != MA_SUCCESS)
    {
        fail("get sound cursor", result);
        return za::nullOpt;
    }

    return za::makeOptional<Time>(seconds(cursor));
}


////////////////////////////////////////////////////////////
za::Optional<za::U64> MiniaudioUtils::getFrameIndex(ma_sound& sound, const Time timeOffset)
{
    ma_uint32 sampleRate{};
    if (const ma_result result = ma_sound_get_data_format(&sound, nullptr, nullptr, &sampleRate, nullptr, 0);
        result != MA_SUCCESS)
    {
        fail("get sound data format", result);
        return za::nullOpt;
    }

    const auto frameIndex = static_cast<za::U64>(timeOffset.asSeconds() * static_cast<float>(sampleRate));
    if (const ma_result result = ma_sound_seek_to_pcm_frame(&sound, frameIndex); result != MA_SUCCESS)
    {
        fail("seek sound to pcm frame", result);
        return za::nullOpt;
    }

    return za::makeOptional<za::U64>(frameIndex);
}


////////////////////////////////////////////////////////////
bool MiniaudioUtils::fail(const char* const what, const int maResult)
{
    errMsg("Failed to {}: {}", what, ma_result_description(static_cast<ma_result>(maResult)));
    return false;
}

} // namespace za::priv
