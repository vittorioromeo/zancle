// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Sound.hpp"

#include "Zancle/Audio/AudioSettings.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/Priv/MiniaudioUtils.hpp"
#include "Zancle/Audio/Priv/SoundBase.hpp"
#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <miniaudio.h>


namespace za
{
////////////////////////////////////////////////////////////
struct Sound::Impl
{
    ////////////////////////////////////////////////////////////
    explicit Impl(PlaybackDevice& thePlaybackDevice, Sound& theOwner, const SoundBuffer& theBuffer) :
        soundBase(thePlaybackDevice, &Impl::vtable, theBuffer.getChannelMap()),
        owner(theOwner),
        buffer(theBuffer)
    {
        if (!soundBase.initialize(&onEnd))
        {
            priv::errMsg("Failed to initialize sound base");
            return;
        }
    }

    ////////////////////////////////////////////////////////////
    static void onEnd(void* const /* userData */, ma_sound* const soundPtr)
    {
        // Seek back to the start of the sound when it finishes playing
        if (const ma_result result = ma_sound_seek_to_pcm_frame(soundPtr, 0u); result != MA_SUCCESS)
            priv::MiniaudioUtils::fail("seek sound to frame 0", result);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ma_result read(ma_data_source* const dataSource,
                                        void* const           framesOut,
                                        const ma_uint64       frameCount,
                                        ma_uint64* const      framesRead)
    {
        auto& impl = *static_cast<Impl*>(dataSource);

        *framesRead = 0u;

        const ma_uint32 channelCount = impl.buffer.getChannelCount();
        ZA_ASSERT(channelCount > 0u);

        const ma_uint64 totalBufferSamples = impl.buffer.getSampleCount();
        const ma_uint64 totalBufferFrames  = totalBufferSamples / channelCount;

        // If cursor is already at or beyond the end of the buffer, either loop or exit
        if (impl.cursor >= totalBufferFrames)
        {
            if (impl.owner.isLooping())
                impl.cursor = 0u;
            else
                return MA_SUCCESS;
        }

        // Determine how many frames we can read
        *framesRead = za::min(frameCount, static_cast<ma_uint64>(totalBufferFrames - impl.cursor));

        // Copy the samples to the output
        const auto sampleCount = *framesRead * impl.buffer.getChannelCount();

        ZA_MEMCPY(framesOut,
                  impl.buffer.getSamples() + impl.cursor * impl.buffer.getChannelCount(),
                  static_cast<za::SizeT>(sampleCount) * sizeof(impl.buffer.getSamples()[0]));

        impl.cursor += *framesRead;

        // If we are looping and at the end of the sound, set the cursor back to the start
        if (impl.owner.isLooping() && impl.cursor >= totalBufferFrames)
            impl.cursor = 0u;

        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ma_result seek(ma_data_source* const dataSource, const ma_uint64 frameIndex)
    {
        static_cast<Impl*>(dataSource)->cursor = static_cast<za::SizeT>(frameIndex);
        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ma_result getFormat(
        ma_data_source* const dataSource,
        ma_format* const      format,
        ma_uint32* const      channels,
        ma_uint32* const      sampleRate,
        ma_channel* const,
        const za::SizeT)
    {
        const auto& impl = *static_cast<const Impl*>(dataSource);

        // If we don't have valid values yet, initialize with defaults so sound creation doesn't fail
        *format     = ma_format_s16;
        *channels   = impl.buffer.getChannelCount();
        *sampleRate = impl.buffer.getSampleRate();

        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ma_result getCursor(ma_data_source* const dataSource, ma_uint64* const cursor)
    {
        *cursor = static_cast<const Impl*>(dataSource)->cursor;
        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ma_result getLength(ma_data_source* const dataSource, ma_uint64* const length)
    {
        const auto& impl = *static_cast<const Impl*>(dataSource);

        *length = impl.buffer.getSampleCount() / impl.buffer.getChannelCount();
        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ma_result setLooping(ma_data_source* const /* dataSource */, const ma_bool32 /* looping */)
    {
        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    static inline constexpr ma_data_source_vtable vtable{read, seek, getFormat, getCursor, getLength, setLooping, /* flags */ 0u};

    priv::MiniaudioUtils::SoundBase soundBase; //!< Sound base, needs to be first member

    Sound&             owner;    //!< Owning `Sound` object
    za::U64            cursor{}; //!< The current playing position (in frames)
    const SoundBuffer& buffer;   //!< Sound buffer bound to the source
};


////////////////////////////////////////////////////////////
Sound::Sound(PlaybackDevice& playbackDevice, const SoundBuffer& buffer, const AudioSettings& audioSettings) :
    m_impl(playbackDevice, *this, buffer)
{
    ZA_LIFETIME_UPDATE_DEPENDENCY(this, SoundBuffer, &m_impl->buffer);
    applyAudioSettings(audioSettings);
}


////////////////////////////////////////////////////////////
Sound::Sound(PlaybackDevice& playbackDevice, const SoundBuffer& buffer) : Sound{playbackDevice, buffer, AudioSettings{}}
{
}


////////////////////////////////////////////////////////////
Sound::~Sound()
{
    ZA_LIFETIME_RETURN_IF_TESTING_ERROR(SoundBuffer);

    // TODO P1: revisit?
    // Stop the sound before `Impl` begins tearing down, otherwise the audio
    // callback can still race with `read()` during destruction.
    pause();
}


////////////////////////////////////////////////////////////
void Sound::setPlayingOffset(const Time playingOffset)
{
    auto& sound = m_impl->soundBase.getSound();

    ZA_ASSERT(sound.pDataSource != nullptr);
    ZA_ASSERT(sound.engineNode.pEngine != nullptr);

    m_impl->cursor = priv::MiniaudioUtils::getFrameIndex(sound, playingOffset).value();
}


////////////////////////////////////////////////////////////
priv::MiniaudioUtils::SoundBase& Sound::getSoundBase() const
{
    // TODO P0: const bs
    return const_cast<Sound*>(this)->m_impl->soundBase;
}


////////////////////////////////////////////////////////////
const SoundBuffer& Sound::getBuffer() const
{
    return m_impl->buffer;
}


////////////////////////////////////////////////////////////
PlaybackDevice& Sound::getPlaybackDevice() const
{
    return *m_impl->soundBase.playbackDevice;
}

} // namespace za
