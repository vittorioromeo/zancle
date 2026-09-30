// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Audio/SoundRecorder.hpp"

#include "Zancle/Audio/CaptureDevice.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Lifetime/LifetimeDependant.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <miniaudio.h>


namespace za
{
////////////////////////////////////////////////////////////
SoundRecorder::SoundRecorder() = default;


////////////////////////////////////////////////////////////
SoundRecorder::~SoundRecorder()
{
    ZA_ASSERT(m_lastCaptureDevice == nullptr &&
              "The sound recorder must be stopped before destroying the capture device");
}


////////////////////////////////////////////////////////////
bool SoundRecorder::start(CaptureDevice& captureDevice, unsigned int sampleRate)
{
    // Store the sample rate and re-initialize if necessary
    if (!captureDevice.setSampleRate(sampleRate))
        return false;

    // Ensure we have a capture device
    if (!captureDevice.isDeviceInitialized())
    {
        priv::errMsg("Trying to start sound recorder, but no device available");
        return false;
    }

    // Check that another capture is not already running
    if (captureDevice.isDeviceStarted())
    {
        priv::errMsg("Trying to start sound recorder, but another capture is already running");
        return false;
    }

    // Notify derived class
    if (!onStart(captureDevice))
        return false;

    // Register callback before starting the device to prevent a race where
    // the audio thread invokes the callback before it has been set.
    captureDevice.setProcessSamplesFunc(this, [](void* userData, const za::I16* samples, za::SizeT sampleCount) {
        return static_cast<SoundRecorder*>(userData)->onProcessSamples(samples, sampleCount);
    });

    // Start the capture
    if (!captureDevice.startDevice())
    {
        captureDevice.setProcessSamplesFunc(nullptr, nullptr);
        priv::errMsg("Failed to start sound recorder");
        return false;
    }

    m_lastCaptureDevice = &captureDevice;
    ZA_LIFETIME_UPDATE_DEPENDENCY(this, CaptureDevice, m_lastCaptureDevice);

    return true;
}


////////////////////////////////////////////////////////////
bool SoundRecorder::stop()
{
    if (m_lastCaptureDevice == nullptr) // Already stopped
        return true;

    auto* const savedCaptureDevice = m_lastCaptureDevice;
    m_lastCaptureDevice            = nullptr;

    if (!savedCaptureDevice->isDeviceInitialized() || !savedCaptureDevice->isDeviceStarted())
    {
        savedCaptureDevice->setProcessSamplesFunc(nullptr, nullptr);
        return false;
    }

    // Stop the device before clearing the callback to prevent a race where
    // the audio thread invokes a null callback. ma_device_stop is synchronous,
    // so no more callbacks will fire after it returns.
    if (!savedCaptureDevice->stopDevice())
    {
        savedCaptureDevice->setProcessSamplesFunc(nullptr, nullptr);
        priv::errMsg("Failed to stop sound recorder");
        return false;
    }

    savedCaptureDevice->setProcessSamplesFunc(nullptr, nullptr);

    // Notify derived class
    return onStop(*savedCaptureDevice);
}


////////////////////////////////////////////////////////////
bool SoundRecorder::onStart(CaptureDevice&)
{
    return true;
}


////////////////////////////////////////////////////////////
bool SoundRecorder::onStop(CaptureDevice&)
{
    return true;
}

} // namespace za
