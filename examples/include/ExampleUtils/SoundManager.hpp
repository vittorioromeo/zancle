#pragma once


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "ExampleUtils/LoadedSound.hpp"

#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/Sound.hpp"
#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/Algorithm/Find.hpp"

#include "Zancle/Container/InPlaceVector.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
struct [[nodiscard]] SoundManager
{
    ////////////////////////////////////////////////////////////
    enum : za::SizeT
    {
        maxSounds = 256u
    };


    ////////////////////////////////////////////////////////////
    za::InPlaceVector<za::Sound, maxSounds> soundsBeingPlayed;


    ////////////////////////////////////////////////////////////
    explicit SoundManager() = default;


    ////////////////////////////////////////////////////////////
    SoundManager(const SoundManager&) = delete;
    SoundManager(SoundManager&&)      = delete;


    ////////////////////////////////////////////////////////////
    void stopPlayingAll(const LoadedSound& ls)
    {
        for (za::Sound& sound : soundsBeingPlayed)
            if (sound.isPlaying() && &sound.getBuffer() == &ls.buffer)
                sound.stop();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] za::SizeT countPlayingPooled(const LoadedSound& ls) const
    {
        za::SizeT acc = 0u;

        for (const za::Sound& sound : soundsBeingPlayed)
            if (sound.isPlaying() && &sound.getBuffer() == &ls.buffer)
                ++acc;

        return acc;
    }


    ////////////////////////////////////////////////////////////
    bool playPooled(za::PlaybackDevice& playbackDevice, const LoadedSound& ls, const za::SizeT maxOverlap)
    {
        // TODO P2 (lib): improve in library

        if (countPlayingPooled(ls) >= maxOverlap)
            return false;

        auto* const it = za::findIf( //
            soundsBeingPlayed.begin(),
            soundsBeingPlayed.end(),
            [](const za::Sound& sound) { return !sound.isPlaying(); });

        if (it != soundsBeingPlayed.end())
        {
            ZA_ASSERT(&it->getPlaybackDevice() == &playbackDevice);

            if (&it->getBuffer() == &ls.buffer)
            {
                it->applyAudioSettings(ls.settings);
                it->play();
                return true;
            }

            soundsBeingPlayed.reEmplaceByIterator(it, playbackDevice, ls.buffer, ls.settings).play();
            return true;
        }

        if (soundsBeingPlayed.size() >= maxSounds)
            return false;

        soundsBeingPlayed.unsafeEmplaceBack(playbackDevice, ls.buffer, ls.settings).play(); // capacity checked above
        return true;
    }
};
