#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/Export.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class CaptureDevice;
class Time;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Abstract base class for capturing sound data
///
////////////////////////////////////////////////////////////
class ZA_AUDIO_API SoundRecorder
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Derived classes must call `stop()` from their own
    /// destructor before any state used by `onProcessSamples`
    /// is torn down. Failing to do so leaves the audio engine's
    /// internal capture thread referencing a half-destroyed
    /// derived object.
    ///
    ////////////////////////////////////////////////////////////
    virtual ~SoundRecorder();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    SoundRecorder(const SoundRecorder&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    SoundRecorder& operator=(const SoundRecorder&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move constructor
    ///
    ////////////////////////////////////////////////////////////
    SoundRecorder(SoundRecorder&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted move assignment
    ///
    ////////////////////////////////////////////////////////////
    SoundRecorder& operator=(SoundRecorder&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Start the capture on a given capture device
    ///
    /// The `sampleRate` parameter defines the number of audio
    /// samples captured per second. The higher, the better the
    /// quality (for example, 44100 samples/sec is CD quality).
    /// The capture device's sample rate is updated accordingly.
    ///
    /// The capture happens in the audio engine's internal
    /// thread, so this function returns immediately. Captured
    /// chunks are delivered through the virtual `onProcessSamples`
    /// callback (which is therefore also invoked from that
    /// thread).
    ///
    /// Only one `SoundRecorder` may be active on a given
    /// `CaptureDevice` at a time. The capture device must
    /// outlive the recorder.
    ///
    /// \param captureDevice Capture device to record from
    /// \param sampleRate    Desired capture rate, in number of samples per second
    ///
    /// \return `true` if the capture was successfully started, `false` otherwise
    ///
    /// \see `stop`, `za::CaptureDevice`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool start(CaptureDevice& captureDevice, unsigned int sampleRate = 44'100);

    ////////////////////////////////////////////////////////////
    /// \brief Stop the capture
    ///
    /// Has no effect if no capture is currently running. After
    /// this call, no more `onProcessSamples` invocations will
    /// happen for the previously bound capture device.
    ///
    /// \return `true` on success, `false` on failure
    ///
    /// \see `start`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool stop();

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// This constructor is only meant to be called by derived classes.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit SoundRecorder();

    ////////////////////////////////////////////////////////////
    /// \brief Hook called once when a new capture starts
    ///
    /// This virtual function may be overridden by a derived
    /// class if something has to be done every time a new
    /// capture starts. The default implementation does nothing
    /// and returns `true`. Returning `false` aborts the capture.
    ///
    /// Called from the thread that invoked `start()` (i.e. the
    /// user thread), not from the audio thread.
    ///
    /// \param captureDevice Capture device the recording is starting on
    ///
    /// \return `true` to start the capture, or `false` to abort it
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual bool onStart(CaptureDevice& captureDevice);

    ////////////////////////////////////////////////////////////
    /// \brief Process a new chunk of recorded samples
    ///
    /// This virtual function is called every time a new chunk
    /// of recorded data is available. The derived class can
    /// then do whatever it wants with it (storing it, playing
    /// it, sending it over the network, etc.).
    ///
    /// Samples are interleaved 16-bit signed PCM. This function
    /// is called from the audio engine's internal capture
    /// thread; any data shared with other threads must be
    /// synchronized appropriately.
    ///
    /// \param samples     Pointer to the new chunk of recorded samples
    /// \param sampleCount Number of samples pointed to by `samples`
    ///
    /// \return `true` to continue the capture, or `false` to stop it
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual bool onProcessSamples(const za::I16* samples, za::SizeT sampleCount) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Hook called once when the capture stops
    ///
    /// This virtual function may be overridden by a derived
    /// class if something has to be done every time the capture
    /// ends. The default implementation does nothing and
    /// returns `true`.
    ///
    /// Called from the thread that invoked `stop()` (i.e. the
    /// user thread), not from the audio thread.
    ///
    /// \param captureDevice Capture device the recording is stopping on
    ///
    /// \return `true` on success, `false` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual bool onStop(CaptureDevice& captureDevice);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    CaptureDevice* m_lastCaptureDevice{};

    ////////////////////////////////////////////////////////////
    // Lifetime tracking
    ////////////////////////////////////////////////////////////
    ZA_LIFETIME_DEPENDS_ON(CaptureDevice); // `SoundRecorder` depends on `CaptureDevice`
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::SoundRecorder
/// \ingroup audio
///
/// `za::SoundRecorder` provides a simple interface to access
/// the audio recording capabilities of the computer (the
/// microphone or any other capture device). As an abstract
/// base class, it only takes care of capturing sound samples;
/// the task of doing something useful with them is left to the
/// derived class. Zancle provides a built-in specialization that
/// stores the captured data into a `za::SoundBuffer`: see
/// `za::SoundBufferRecorder`.
///
/// A derived class has only one virtual function to override:
/// \li `onProcessSamples` receives the new chunks of audio samples while the capture happens
///
/// Two additional virtual functions can be overridden if
/// custom setup or teardown is needed:
/// \li `onStart` is called from the user thread before the audio thread starts pulling samples
/// \li `onStop`  is called from the user thread after the audio thread has been stopped
///
/// The list of available capture devices is enumerated via
/// `za::AudioContext::getAvailableCaptureDeviceHandles`. A
/// `za::CaptureDevice` is then opened and passed to `start()`.
/// The capture device's sample rate, channel count, and
/// channel map can all be configured before recording starts.
///
/// It is important to note that the audio capture happens in a
/// separate thread (the audio engine's internal capture
/// thread), so that it doesn't block the rest of the program.
/// In particular, the `onProcessSamples` virtual function
/// (but not `onStart` and not `onStop`) is called from this
/// separate thread. Any data shared with other threads must
/// therefore be synchronized appropriately. Another thing to
/// bear in mind is that you must call `stop()` in the destructor
/// of your derived class, so that the recording thread finishes
/// before your object is destroyed.
///
/// Usage example:
/// \code
/// class CustomRecorder : public za::SoundRecorder
/// {
/// public:
///     ~CustomRecorder() override
///     {
///         // Make sure the recording thread is stopped before our state is destroyed
///         (void)stop();
///     }
///
/// private:
///     bool onStart(za::CaptureDevice& /*captureDevice*/) override // optional
///     {
///         // Initialize whatever has to be done before the capture starts
///         return true;
///     }
///
///     [[nodiscard]] bool onProcessSamples(const za::I16* samples, za::SizeT sampleCount) override
///     {
///         // Do something with the new chunk of samples (store them, send them, etc.)
///         return true; // return false to stop the capture
///     }
///
///     bool onStop(za::CaptureDevice& /*captureDevice*/) override // optional
///     {
///         // Clean up whatever has to be done after the capture ends
///         return true;
///     }
/// };
///
/// // Usage
/// auto audioContext  = za::AudioContext::create().value();
/// auto deviceHandle  = za::AudioContext::getDefaultCaptureDeviceHandle().value();
/// za::CaptureDevice  captureDevice{deviceHandle};
///
/// CustomRecorder recorder;
/// if (!recorder.start(captureDevice))
///     return -1;
///
/// // ... record for a while ...
///
/// (void)recorder.stop();
/// \endcode
///
/// \see `za::SoundBufferRecorder`, `za::CaptureDevice`, `za::AudioContext`
///
////////////////////////////////////////////////////////////
