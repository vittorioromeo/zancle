#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/AudioSettings.hpp"
#include "Zancle/Audio/EffectProcessor.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Container/InPlaceVector.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include <miniaudio.h>


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class ChannelMap;
class EffectProcessor;
} // namespace za


namespace za::priv::MiniaudioUtils
{
////////////////////////////////////////////////////////////
struct SoundBase
{
    ////////////////////////////////////////////////////////////
    explicit SoundBase(PlaybackDevice& thePlaybackDevice, const void* dataSourceVTable, const ChannelMap& channelMap);

    ////////////////////////////////////////////////////////////
    ~SoundBase();

    ////////////////////////////////////////////////////////////
    SoundBase(const SoundBase&) = delete;
    SoundBase(SoundBase&&)      = delete;

    ////////////////////////////////////////////////////////////
    SoundBase& operator=(const SoundBase&) = delete;
    SoundBase& operator=(SoundBase&&)      = delete;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool initialize(ma_sound_end_proc endCallback);

    ////////////////////////////////////////////////////////////
    /// Detach the sound from the engine graph and stop the audio
    /// thread from invoking its data source callbacks. Idempotent.
    /// Must be called before any data the read callback depends on
    /// is freed.
    ////////////////////////////////////////////////////////////
    void uninitSound();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool connectEffect(bool connect);

    ////////////////////////////////////////////////////////////
    ma_sound& getSound();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setAndConnectEffectProcessor(const EffectProcessor& effectProcessor);

    ////////////////////////////////////////////////////////////
    void applyAudioSettings(const AudioSettings& audioSettings);

    ////////////////////////////////////////////////////////////
    static void nodeOnProcess(ma_node*      node,
                              const float** framesIn,
                              ma_uint32*    frameCountIn,
                              float**       framesOut,
                              ma_uint32*    frameCountOut);

    ////////////////////////////////////////////////////////////
    void processEffect(const float** framesIn, za::U32& frameCountIn, float** framesOut, za::U32& frameCountOut);

    ////////////////////////////////////////////////////////////
    void setChannelMap(const ChannelMap& channelMap);

    ////////////////////////////////////////////////////////////
    static inline constexpr ma_node_vtable effectNodeVTable{
        .onProcess                    = &nodeOnProcess,
        .onGetRequiredInputFrameCount = nullptr,
        .inputBusCount                = 1,
        .outputBusCount               = 1,
        .flags                        = MA_NODE_FLAG_CONTINUOUS_PROCESSING | MA_NODE_FLAG_ALLOW_NULL_INPUT,
    };

    ////////////////////////////////////////////////////////////
    struct EffectNode
    {
        ma_node_base base{}; // must be first member
        ma_uint32    channelCount{};
    };

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    ma_data_source_base dataSourceBase{}; //!< The struct that makes this object a miniaudio data source (must be first member)

    PlaybackDevice* playbackDevice;

    EffectNode effectNode; //!< The engine node that performs effect processing

    za::InPlaceVector<ma_channel, MA_CHANNEL_POSITION_COUNT> soundChannelMap; //!< The map of position in sample frame to sound channel

    ma_sound        sound{};         //!< The sound
    EffectProcessor effectProcessor; //!< The effect processor

    bool soundUninitialized{}; //!< `true` once `ma_sound_uninit` has been called (prevents double-uninit)
    [[maybe_unused]] bool effectNodeUninitialized{}; //!< Failsafe debug boolean to check if `onProcess` is called after destruction

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDS_ON(PlaybackDevice); // `SoundBase` depends on `PlaybackDevice`
};

} // namespace za::priv::MiniaudioUtils
