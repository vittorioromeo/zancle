// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Music.hpp"

#include "Zancle/Audio/AudioSettings.hpp"
#include "Zancle/Audio/MusicReader.hpp"
#include "Zancle/Audio/Priv/MiniaudioSoundSource.hpp"
#include "Zancle/Audio/Priv/MiniaudioUtils.hpp"
#include "Zancle/Audio/Priv/SoundBase.hpp"
#include "Zancle/Audio/SoundStreamState.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Concurrency/AtomicMutex.hpp"
#include "Zancle/Concurrency/LockGuard.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr za::Time samplesToTime(
    const unsigned int sampleRate,
    const unsigned int channelCount,
    const za::U64      samples)
{
    ZA_ASSERT(sampleRate > 0u);
    ZA_ASSERT(channelCount > 0u);

    return za::microseconds(static_cast<za::I64>((samples * 1'000'000u) / (channelCount * sampleRate)));
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] inline constexpr za::U64 timeToSamples(
    const unsigned int sampleRate,
    const unsigned int channelCount,
    const za::Time     position)
{
    // Always ROUND, no unchecked truncation, hence the addition in the numerator.
    // This avoids most precision errors arising from "samples => Time => samples" conversions
    // Original rounding calculation is ((Micros * Freq * Channels) / 1'000'000) + 0.5
    // We refactor it to keep za::I64 as the data type throughout the whole operation.
    return ((static_cast<za::U64>(position.asMicroseconds()) * sampleRate * channelCount) + 500'000u) / 1'000'000u;
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
struct Music::Impl
{
    ////////////////////////////////////////////////////////////
    /// \brief State object owned by `SoundStreamState<MusicState>`
    ///
    /// Holds all the audio-thread-touched data. Its methods
    /// (`onGetData`/`onSeek`/`onLoop`) are invoked by the audio
    /// callback; the `SoundStream` template guarantees the state
    /// outlives any in-flight callback.
    ///
    ////////////////////////////////////////////////////////////
    struct MusicState
    {
        ////////////////////////////////////////////////////////////
        mutable AtomicMutex               loopMutex;      //!< Protects `loopSpan` and `sampleOffset`
        Music::Span<za::U64>              loopSpan;       //!< Loop range Specifier
        MusicReader&                      musicReader;    //!< The music reader
        const priv::MiniaudioSoundSource& source;         //!< Back-ref to the owning `Music` for `isLooping()`
        za::U64                           sampleOffset{}; //!< Current offset in the stream

        ////////////////////////////////////////////////////////////
        explicit MusicState(MusicReader& theMusicReader, const priv::MiniaudioSoundSource& theSource, za::U64 sampleCount) :
            loopSpan{0u, sampleCount},
            musicReader(theMusicReader),
            source(theSource)
        {
        }

        ////////////////////////////////////////////////////////////
        bool onGetData(za::Vector<za::I16>& outBuffer)
        {
            const LockGuard lock(loopMutex);

            // Size the output buffer to hold up to 1 second of audio samples
            outBuffer.resize(musicReader.getSampleRate() * musicReader.getChannelCount());

            za::SizeT     toFill  = outBuffer.size();
            const za::U64 loopEnd = loopSpan.offset + loopSpan.length;

            // If the loop end is enabled and imminent, request less data so we trip an `onLoop()`.
            if (source.isLooping() && (loopSpan.length != 0) && (sampleOffset <= loopEnd) &&
                (sampleOffset + toFill > loopEnd))
                toFill = static_cast<za::SizeT>(loopEnd - sampleOffset);

            // `seekAndRead` is thread-safe
            const auto [sampleOffsetAfter, samplesRead] = musicReader.seekAndRead(sampleOffset, outBuffer.data(), toFill);

            outBuffer.resize(static_cast<za::SizeT>(samplesRead));
            sampleOffset = sampleOffsetAfter + samplesRead;

            return (samplesRead != 0) && (sampleOffset < musicReader.getSampleCount()) &&
                   (sampleOffset != loopEnd || loopSpan.length == 0);
        }

        ////////////////////////////////////////////////////////////
        void onSeek(const Time timeOffset)
        {
            const LockGuard lock(loopMutex);
            sampleOffset = timeToSamples(musicReader.getSampleRate(), musicReader.getChannelCount(), timeOffset);
        }

        ////////////////////////////////////////////////////////////
        za::Optional<za::U64> onLoop()
        {
            const LockGuard lock(loopMutex);

            if (!source.isLooping())
                return za::nullOpt;

            if ((loopSpan.length != 0) && (sampleOffset == loopSpan.offset + loopSpan.length))
            {
                sampleOffset = loopSpan.offset;
                return za::makeOptional(sampleOffset);
            }

            if (sampleOffset >= musicReader.getSampleCount())
            {
                sampleOffset = 0u;
                return za::makeOptional(sampleOffset);
            }

            return za::nullOpt;
        }
    };

    SoundStreamState<MusicState> stream;

    explicit Impl(PlaybackDevice& playbackDevice, MusicReader& musicReader, const priv::MiniaudioSoundSource& source) :
        stream(playbackDevice,
               musicReader.getChannelMap(),
               musicReader.getSampleRate(),
               musicReader,
               source,
               musicReader.getSampleCount())
    {
    }

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDS_ON(MusicReader); // `Music` depends on `MusicReader`
};


////////////////////////////////////////////////////////////
Music::Music(PlaybackDevice& playbackDevice, MusicReader& musicReader, const AudioSettings& audioSettings) :
    m_impl(playbackDevice, musicReader, *this)
{
    ZA_LIFETIME_UPDATE_DEPENDENCY(&*m_impl, MusicReader, &m_impl->stream.state().musicReader);
    applyAudioSettings(audioSettings);
}


////////////////////////////////////////////////////////////
Music::Music(PlaybackDevice& playbackDevice, MusicReader& musicReader) :
    Music{playbackDevice, musicReader, AudioSettings{}}
{
}


////////////////////////////////////////////////////////////
Music::~Music() = default;


////////////////////////////////////////////////////////////
void Music::setPlayingOffset(const Time playingOffset)
{
    m_impl->stream.setPlayingOffset(playingOffset);
}


////////////////////////////////////////////////////////////
priv::MiniaudioUtils::SoundBase& Music::getSoundBase() const
{
    return const_cast<Music*>(this)->m_impl->stream.getSoundBase();
}


////////////////////////////////////////////////////////////
Music::TimeSpan Music::getLoopPoints() const
{
    const auto& state = m_impl->stream.state();

    const auto sampleRate   = state.musicReader.getSampleRate();
    const auto channelCount = state.musicReader.getChannelCount();

    const LockGuard lock(state.loopMutex);

    return TimeSpan{samplesToTime(sampleRate, channelCount, state.loopSpan.offset),
                    samplesToTime(sampleRate, channelCount, state.loopSpan.length)};
}


////////////////////////////////////////////////////////////
void Music::setLoopPoints(const TimeSpan timePoints)
{
    auto& state = m_impl->stream.state();

    const auto sampleRate   = state.musicReader.getSampleRate();
    const auto channelCount = state.musicReader.getChannelCount();

    ZA_ASSERT(channelCount > 0u);
    ZA_ASSERT(sampleRate > 0u);

    const auto fileSampleCount = state.musicReader.getSampleCount();

    Span<za::U64> samplePoints{timeToSamples(sampleRate, channelCount, timePoints.offset),
                               timeToSamples(sampleRate, channelCount, timePoints.length)};

    if (fileSampleCount == 0u)
    {
        priv::errMsg("Music is not in a valid state to assign Loop Points.");
        return;
    }

    // Round up to the next even sample if needed
    samplePoints.offset += (channelCount - 1u);
    samplePoints.offset -= (samplePoints.offset % channelCount);
    samplePoints.length += (channelCount - 1u);
    samplePoints.length -= (samplePoints.length % channelCount);

    if (samplePoints.offset >= fileSampleCount)
    {
        priv::errMsg("LoopPoints offset val must be in range [0, Duration).");
        state.loopSpan = {0u, state.musicReader.getSampleCount()};
        return;
    }

    if (samplePoints.length == 0u)
    {
        priv::errMsg("LoopPoints length val must be nonzero.");
        state.loopSpan = {0u, state.musicReader.getSampleCount()};
        return;
    }

    samplePoints.length = za::min(samplePoints.length, fileSampleCount - samplePoints.offset);

    if (samplePoints.offset == state.loopSpan.offset && samplePoints.length == state.loopSpan.length)
        return;

    const bool oldIsPlaying = isPlaying();
    const Time oldPos       = getPlayingOffset();

    stop();
    state.loopSpan = samplePoints;

    if (oldPos != Time{})
        setPlayingOffset(oldPos);

    if (oldIsPlaying)
        resume();
}


////////////////////////////////////////////////////////////
const MusicReader& Music::getMusicReader() const
{
    return m_impl->stream.state().musicReader;
}


////////////////////////////////////////////////////////////
PlaybackDevice& Music::getPlaybackDevice() const
{
    return m_impl->stream.getPlaybackDevice();
}

} // namespace za
