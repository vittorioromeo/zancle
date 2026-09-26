#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp"

#include "Zancle/Window/EGL/EGLContext.hpp"
#include "Zancle/Window/Event.hpp"

#include "Zancle/Container/EnumArray.hpp"

#include "Zancle/Base/SizeT.hpp"

#include <android/configuration.h>
#include <android/native_activity.h>

#include <mutex>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>

class ZA_SYSTEM_API LogcatStream : public std::streambuf
{
public:
    LogcatStream() = default;

    std::streambuf::int_type overflow(std::streambuf::int_type c) override;

private:
    std::string m_message;
};

namespace za::priv
{
struct ActivityStates
{
    ANativeActivity* activity{};
    ANativeWindow*   window{};

    ALooper*        looper{};
    AInputQueue*    inputQueue{};
    AConfiguration* config{};

    EGLDisplay  display{};
    EglContext* context{};

    std::vector<std::byte> savedState;

    std::recursive_mutex mutex;

    void (*forwardEvent)(const Event& event){};
    int (*processEvent)(int fd, int events, void* data){};

    std::unordered_map<int, Vec2i>                     touchEvents;
    Vec2i                                              mousePosition;
    EnumArray<Mouse::Button, bool, Mouse::ButtonCount> isButtonPressed{};

    bool mainOver{};

    Vec2i screenSize;
    Vec2i fullScreenSize;

    bool initialized{};
    bool terminated{};

    bool fullscreen{};

    bool updated{};

    LogcatStream logcat;
};

ZA_SYSTEM_API ActivityStates*& getActivityStatesPtr();

ZA_SYSTEM_API void resetActivity(ActivityStates* initializedStates);

ZA_SYSTEM_API ActivityStates& getActivity();

} // namespace za::priv
