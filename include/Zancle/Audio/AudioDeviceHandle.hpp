#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class AudioContext;
class CaptureDevice;
class CaptureDeviceHandle;
class PlaybackDevice;
class PlaybackDeviceHandle;
} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Lightweight, copyable description of an audio device
///
/// `AudioDeviceHandle` wraps an opaque pointer to a miniaudio
/// device descriptor (`ma_device_info`) along with a copy of the
/// device's name and "is default" flag. It is the type-erased
/// base for the user-facing `za::PlaybackDeviceHandle` and
/// `za::CaptureDeviceHandle`.
///
/// Handles are obtained from `za::AudioContext` (e.g.
/// `getAvailablePlaybackDeviceHandles`) and consumed by
/// `za::PlaybackDevice` / `za::CaptureDevice` constructors. They
/// can be freely copied and stored without keeping the originating
/// `AudioContext` alive.
///
/// This class is implementation detail and not meant to be used
/// directly by client code; use the strongly-typed wrappers
/// `za::PlaybackDeviceHandle` and `za::CaptureDeviceHandle`
/// instead.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_AUDIO_API AudioDeviceHandle
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~AudioDeviceHandle();

    ////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    ////////////////////////////////////////////////////////////
    AudioDeviceHandle(const AudioDeviceHandle& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    AudioDeviceHandle(AudioDeviceHandle&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Copy assignment
    ///
    ////////////////////////////////////////////////////////////
    AudioDeviceHandle& operator=(const AudioDeviceHandle& rhs);

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    AudioDeviceHandle& operator=(AudioDeviceHandle&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Get the human-readable name of the device
    ///
    /// The returned string is owned by this handle and remains
    /// valid for as long as the handle (or any copy of it) is alive.
    ///
    /// \return Null-terminated UTF-8 device name
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* getName() const;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether this handle refers to the system default device
    ///
    /// \return `true` if this is the default device for its kind
    ///         (playback or capture), `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDefault() const;

    ////////////////////////////////////////////////////////////
    /// \brief Compare two device handles for equality
    ///
    /// Two handles compare equal if they refer to the same
    /// underlying device (matching driver-level identifier).
    ///
    ////////////////////////////////////////////////////////////
    friend ZA_AUDIO_API bool operator==(const AudioDeviceHandle& lhs, const AudioDeviceHandle& rhs);

protected:
    friend PlaybackDevice;
    friend PlaybackDeviceHandle;
    friend CaptureDevice;
    friend CaptureDeviceHandle;

    ////////////////////////////////////////////////////////////
    /// \brief Create a device handle from miniaudio device info
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit AudioDeviceHandle(const void* maDeviceInfo);

    ////////////////////////////////////////////////////////////
    /// \brief Return a pointer to the stored miniaudio device info
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const void* getMADeviceInfo() const;

public:
    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Create a device handle from miniaudio device info (passkey)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit AudioDeviceHandle(za::PassKey<AudioContext>&&, const void* maDeviceInfo);

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Create a device handle from miniaudio device info (passkey)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit AudioDeviceHandle(za::PassKey<PlaybackDevice>&&, const void* maDeviceInfo);

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Create a device handle from miniaudio device info (passkey)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit AudioDeviceHandle(za::PassKey<CaptureDevice>&&, const void* maDeviceInfo);

private:
    struct Impl;
    za::InPlacePImpl<Impl, 1792> m_impl; //!< Implementation details
};

////////////////////////////////////////////////////////////
/// \brief Compare equality between device handles
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_AUDIO_API bool operator==(const AudioDeviceHandle& lhs, const AudioDeviceHandle& rhs);

////////////////////////////////////////////////////////////
/// \brief CRTP-style strong typedef around `AudioDeviceHandle`
///
/// This template makes it impossible to accidentally pass a
/// playback device handle where a capture device handle is
/// expected (or vice versa) by tagging the handle type with the
/// device class it is associated with.
///
/// Used internally to define `za::PlaybackDeviceHandle` and
/// `za::CaptureDeviceHandle`.
///
////////////////////////////////////////////////////////////
template <typename T>
class StronglyTypedDeviceHandle : private AudioDeviceHandle
{
    friend T;

    using AudioDeviceHandle::AudioDeviceHandle;

public:
    using AudioDeviceHandle::getName;
    using AudioDeviceHandle::isDefault;
    using AudioDeviceHandle::operator=;

    [[nodiscard, gnu::always_inline, gnu::pure]] bool operator==(const StronglyTypedDeviceHandle& rhs) const = default;

private:
    using AudioDeviceHandle::getMADeviceInfo;
};

} // namespace za::priv
