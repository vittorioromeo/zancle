// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/SDLWindowImpl.hpp"

#include "Zancle/Config.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/Joystick.hpp"
#include "Zancle/Window/JoystickCapabilities.hpp"
#include "Zancle/Window/JoystickManager.hpp"
#include "Zancle/Window/JoystickState.hpp"
#include "Zancle/Window/Mouse.hpp"
#include "Zancle/Window/SDLGlContext.hpp"
#include "Zancle/Window/SDLLayer.hpp"
#include "Zancle/Window/Sensor.hpp"
#include "Zancle/Window/SensorManager.hpp"
#include "Zancle/Window/VideoMode.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"
#include "Zancle/Window/Vulkan.hpp"
#include "Zancle/Window/WindowContext.hpp"
#include "Zancle/Window/WindowHandle.hpp"
#include "Zancle/Window/WindowSettings.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"
#include "Zancle/String/Utf.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"
#include "Zancle/Container/EnumArray.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Math/Fabs.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Strlen.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_touch.h>
#include <SDL3/SDL_video.h>


////////////////////////////////////////////////////////////
#if defined(ZA_SYSTEM_WINDOWS)
    #include "Zancle/Window/Win32/Utils.hpp"
#endif


namespace
{
// A nested named namespace is used here to allow unity builds of Zancle.
// Yes, this is a rather weird namespace.
namespace SDLWindowImplImpl
{
////////////////////////////////////////////////////////////
struct TouchInfo
{
    unsigned int     normalizedIndex;
    za::Vec2i        position;
    za::WindowHandle handle;
};


////////////////////////////////////////////////////////////
bool touchIndexPool[32]{}; // Keeps track of which finger indices are in use


////////////////////////////////////////////////////////////
[[nodiscard]] int findFirstNormalizedTouchIndex()
{
    for (za::SizeT i = 0u; i < 32u; ++i)
        if (!touchIndexPool[i])
            return static_cast<int>(i);

    za::priv::errMsg("No available touch index\n");
    return -1;
}


////////////////////////////////////////////////////////////
bool setWindowNonExclusiveFullscreenIfNeeded([[maybe_unused]] const bool      hasTitlebar,
                                             [[maybe_unused]] const za::Vec2u size,
                                             [[maybe_unused]] SDL_Window*     sdlWindowPtr)
{
#ifdef ZA_SYSTEM_WINDOWS
    {
        // See https://github.com/libsdl-org/SDL/issues/12791

        const auto desktopModeSize = za::VideoModeUtils::getDesktopMode().size;
        if (!hasTitlebar && size == desktopModeSize)
        {
            void* hwnd = SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindowPtr),
                                                SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                                nullptr);

            za::priv::setWindowBorderless(hwnd, size.x, size.y);
            return true;
        }
    }
#endif

    return false;
}


////////////////////////////////////////////////////////////
[[nodiscard]] za::String windowSettingsToString(const za::WindowSettings& settings)
{
    return "size={" + za::toString(settings.size.x) + ", " + za::toString(settings.size.y) + "}, " +                //
           "bitsPerPixel=" + za::toString(settings.bitsPerPixel) + ", " +                                           //
           "title=\"" + settings.title.asBytes() + "\", " +                                                         //
           "fullscreen=" + za::toString(settings.fullscreen) + ", " +                                               //
           "resizable=" + za::toString(settings.resizable) + ", " +                                                 //
           "closable=" + za::toString(settings.closable) + ", " +                                                   //
           "hasTitlebar=" + za::toString(settings.hasTitlebar) + ", " +                                             //
           "vsync=" + za::toString(settings.vsync) + ", " +                                                         //
           "frametimeLimit=" + za::toString(settings.frametimeLimit) + ", " +                                       //
           "contextSettings={depthBits=" + za::toString(settings.contextSettings.depthBits) +                       //
           ", stencilBits=" + za::toString(settings.contextSettings.stencilBits) +                                  //
           ", majorVersion=" + za::toString(settings.contextSettings.majorVersion) +                                //
           ", minorVersion=" + za::toString(settings.contextSettings.minorVersion) +                                //
           ", attributeFlags=" + za::toString(static_cast<unsigned int>(settings.contextSettings.attributeFlags)) + //
           "}";
}


////////////////////////////////////////////////////////////
ankerl::unordered_dense::map<SDL_FingerID, TouchInfo> touchMap;


////////////////////////////////////////////////////////////
const za::priv::SDLWindowImpl* fullscreenWindow = nullptr; // TODO P1: not sure why we're tracking this


////////////////////////////////////////////////////////////
ankerl::unordered_dense::map<SDL_WindowID, za::priv::SDLWindowImpl*> windowImplMap;

} // namespace SDLWindowImplImpl
} // namespace


namespace za::priv
{
////////////////////////////////////////////////////////////
struct SDLWindowImpl::Impl
{
    za::Vector<Event> events; //!< Queue of available events (FIFO; popped from the front)

    JoystickState joystickStates[Joystick::MaxCount]{};    //!< Previous state of the joysticks
    bool          joystickConnected[Joystick::MaxCount]{}; //!< Previous connection state of the joysticks

    za::EnumArray<Sensor::Type, Vec3f, Sensor::Count> sensorValue; //!< Previous value of the sensors

    float joystickThreshold{0.1f}; //!< Joystick threshold (minimum motion for "move" event to be generated)

    za::EnumArray<Joystick::Axis, float, Joystick::AxisCount>
        previousAxes[Joystick::MaxCount]{}; //!< Position of each axis last time a move event triggered, in range [-100, 100]

    za::Optional<Vec2u> minimumSize; //!< Minimum window size
    za::Optional<Vec2u> maximumSize; //!< Maximum window size

    SDL_Window* sdlWindow; //!< SDL window handle

    bool keyRepeatEnabled = false; //!< Is the key repeat feature enabled?

    bool isExternal = false; //!< Is the window created externally?

    explicit Impl(const char* context, SDL_Window* theSDLWindow, const bool theIsExternal) :
        sdlWindow{theSDLWindow},
        isExternal{theIsExternal}
    {
        if (!sdlWindow)
        {
            errMsg("Failed to create window created from {}: {}", context, SDL_GetError());
            return;
        }

        // On platforms with a screen keyboard (Android, iOS, Steam Deck) `SDL_StartTextInput` shows
        // the on-screen keyboard immediately; defer that to `Keyboard::setVirtualKeyboardVisible`.
        // On desktop it just enables `TEXT_INPUT` events without any visual side-effect.
        if (!SDL_HasScreenKeyboardSupport() && !SDL_StartTextInput(sdlWindow))
            errMsg("Failed to start text input for window created from {}: {}", context, SDL_GetError());
    }

    Impl(const Impl&)            = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&& rhs)            = delete;
    Impl& operator=(Impl&& rhs) = delete;

    ~Impl()
    {
        // Only stop text input if it was actually started; on platforms with a screen keyboard
        // we defer `SDL_StartTextInput` to `Keyboard::setVirtualKeyboardVisible`, so it may
        // never have been enabled for this window.
        if (SDL_TextInputActive(sdlWindow) && !SDL_StopTextInput(sdlWindow))
            errMsg("Failed to stop text input for window: {}", SDL_GetError());

        if (!isExternal)
            SDL_DestroyWindow(sdlWindow);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDL_WindowID getWindowID() const
    {
        const auto result = SDL_GetWindowID(sdlWindow);

        if (result == 0)
            errMsg("Failed to get window ID: {}", SDL_GetError());

        return result;
    }
};


////////////////////////////////////////////////////////////
void SDLWindowImpl::processSDLEvent(const SDL_Event& e)
{
    const auto et = static_cast<SDL_EventType>(e.type);

    switch (et)
    {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            pushEvent(Event::Closed{});
            break;
        }

        case SDL_EVENT_WINDOW_RESIZED:
        {
            pushEvent(Event::Resized{Vec2i{e.window.data1, e.window.data2}.toVec2u()});
            break;
        }

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        {
            pushEvent(Event::MouseEntered{});
            break;
        }

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        {
            pushEvent(Event::MouseLeft{});
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        {
            pushEvent(Event::FocusGained{});
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_LOST:
        {
            pushEvent(Event::FocusLost{});
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            if (!m_impl->keyRepeatEnabled && e.key.repeat)
                return;

            pushEvent(Event::KeyPressed{.code       = mapSDLKeycodeToZancle(e.key.key),
                                        .scancode   = mapSDLScancodeToZancle(e.key.scancode),
                                        .alt        = static_cast<bool>(e.key.mod & SDL_KMOD_ALT),
                                        .control    = static_cast<bool>(e.key.mod & SDL_KMOD_CTRL),
                                        .shift      = static_cast<bool>(e.key.mod & SDL_KMOD_SHIFT),
                                        .system     = static_cast<bool>(e.key.mod & SDL_KMOD_GUI),
                                        .capsLock   = static_cast<bool>(e.key.mod & SDL_KMOD_CAPS),
                                        .numLock    = static_cast<bool>(e.key.mod & SDL_KMOD_NUM),
                                        .scrollLock = static_cast<bool>(e.key.mod & SDL_KMOD_SCROLL)});
            break;
        }

        case SDL_EVENT_KEY_UP:
        {
            pushEvent(Event::KeyReleased{.code       = mapSDLKeycodeToZancle(e.key.key),
                                         .scancode   = mapSDLScancodeToZancle(e.key.scancode),
                                         .alt        = static_cast<bool>(e.key.mod & SDL_KMOD_ALT),
                                         .control    = static_cast<bool>(e.key.mod & SDL_KMOD_CTRL),
                                         .shift      = static_cast<bool>(e.key.mod & SDL_KMOD_SHIFT),
                                         .system     = static_cast<bool>(e.key.mod & SDL_KMOD_GUI),
                                         .capsLock   = static_cast<bool>(e.key.mod & SDL_KMOD_CAPS),
                                         .numLock    = static_cast<bool>(e.key.mod & SDL_KMOD_NUM),
                                         .scrollLock = static_cast<bool>(e.key.mod & SDL_KMOD_SCROLL)});
            break;
        }

        case SDL_EVENT_TEXT_INPUT:
        {
            char32_t    unicode   = 0;
            const char* keyBuffer = e.text.text;
            const auto  length    = ZA_STRLEN(keyBuffer);
            const auto* iter      = keyBuffer;

            while (iter < keyBuffer + length)
            {
                iter = Utf8::decode(iter, keyBuffer + length, unicode, 0);
                if (unicode != 0)
                    pushEvent(Event::TextEntered{unicode});
            }

            break;
        }

        case SDL_EVENT_MOUSE_MOTION:
        {
            pushEvent(Event::MouseMoved{
                .position = {static_cast<int>(e.motion.x), static_cast<int>(e.motion.y)},
            });
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            pushEvent(Event::MouseButtonPressed{
                .button   = getZancleButtonFromSDLButton(e.button.button),
                .position = {static_cast<int>(e.button.x), static_cast<int>(e.button.y)},
            });
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            pushEvent(Event::MouseButtonReleased{
                .button   = getZancleButtonFromSDLButton(e.button.button),
                .position = {static_cast<int>(e.button.x), static_cast<int>(e.button.y)},
            });
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            pushEvent(Event::MouseWheelScrolled{
                .wheel    = Mouse::Wheel::Vertical, // TODO P0: horizontal wheel support?
                .delta    = static_cast<float>(e.wheel.y),
                .position = {static_cast<int>(e.wheel.x), static_cast<int>(e.wheel.y)}, // TODO P0: this is wrong
            });
            break;
        }

        case SDL_EVENT_FINGER_DOWN:
        {
            const SDL_TouchFingerEvent& fingerEvent = e.tfinger; // TODO P0: add touch device?
            const auto touchPos = Vec2f{fingerEvent.x, fingerEvent.y}.componentWiseMul(getSize().toVec2f()).toVec2i();

            ZA_ASSERT(!SDLWindowImplImpl::touchMap.contains(fingerEvent.fingerID));

            const int normalizedIndex = SDLWindowImplImpl::findFirstNormalizedTouchIndex();
            if (normalizedIndex == -1)
                break;

            const auto fingerIdx                         = static_cast<unsigned int>(normalizedIndex);
            SDLWindowImplImpl::touchIndexPool[fingerIdx] = true;
            SDLWindowImplImpl::touchMap.emplace(fingerEvent.fingerID,
                                                SDLWindowImplImpl::TouchInfo{fingerIdx, touchPos, getNativeHandle()});

            pushEvent(za::Event::TouchBegan{fingerIdx, touchPos, fingerEvent.pressure});
            break;
        }

        case SDL_EVENT_FINGER_UP:
        {
            const SDL_TouchFingerEvent& fingerEvent = e.tfinger;
            const auto touchPos = Vec2f{fingerEvent.x, fingerEvent.y}.componentWiseMul(getSize().toVec2f()).toVec2i();

            ZA_ASSERT(SDLWindowImplImpl::touchMap.contains(fingerEvent.fingerID));
            const auto [fingerIdx, pos, handle] = SDLWindowImplImpl::touchMap[fingerEvent.fingerID];

            SDLWindowImplImpl::touchIndexPool[fingerIdx] = false;
            SDLWindowImplImpl::touchMap.erase(fingerEvent.fingerID);

            pushEvent(za::Event::TouchEnded{fingerIdx, touchPos, fingerEvent.pressure});
            break;
        }

        case SDL_EVENT_FINGER_MOTION:
        {
            const SDL_TouchFingerEvent& fingerEvent = e.tfinger;
            const auto touchPos = Vec2f{fingerEvent.x, fingerEvent.y}.componentWiseMul(getSize().toVec2f()).toVec2i();

            ZA_ASSERT(SDLWindowImplImpl::touchMap.contains(fingerEvent.fingerID));
            const auto [fingerIdx, pos, handle] = SDLWindowImplImpl::touchMap[fingerEvent.fingerID];

            pushEvent(za::Event::TouchMoved{fingerIdx, touchPos, fingerEvent.pressure});
            break;
        }

        case SDL_EVENT_FINGER_CANCELED: // TODO
        {
            break;
        }

            // unused (TODO P1: revisit)
        case SDL_EVENT_AUDIO_DEVICE_ADDED:
        case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED:
        case SDL_EVENT_AUDIO_DEVICE_REMOVED:
        case SDL_EVENT_CAMERA_DEVICE_ADDED:
        case SDL_EVENT_CAMERA_DEVICE_APPROVED:
        case SDL_EVENT_CAMERA_DEVICE_DENIED:
        case SDL_EVENT_CAMERA_DEVICE_REMOVED:
        case SDL_EVENT_CLIPBOARD_UPDATE:
        case SDL_EVENT_DID_ENTER_BACKGROUND:
        case SDL_EVENT_DID_ENTER_FOREGROUND:
        case SDL_EVENT_DISPLAY_ADDED:
        case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
        case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
        case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
        case SDL_EVENT_DISPLAY_MOVED:
        case SDL_EVENT_DISPLAY_ORIENTATION:
        case SDL_EVENT_DISPLAY_REMOVED:
        case SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED:
        case SDL_EVENT_DROP_BEGIN:
        case SDL_EVENT_DROP_COMPLETE:
        case SDL_EVENT_DROP_FILE:
        case SDL_EVENT_DROP_POSITION:
        case SDL_EVENT_DROP_TEXT:
        case SDL_EVENT_ENUM_PADDING:
        case SDL_EVENT_FIRST:
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_GAMEPAD_CAPSENSE_RELEASE:
        case SDL_EVENT_GAMEPAD_CAPSENSE_TOUCH:
        case SDL_EVENT_GAMEPAD_REMAPPED:
        case SDL_EVENT_GAMEPAD_REMOVED:
        case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
        case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
        case SDL_EVENT_GAMEPAD_UPDATE_COMPLETE:
        case SDL_EVENT_JOYSTICK_ADDED:
        case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        case SDL_EVENT_JOYSTICK_BALL_MOTION:
        case SDL_EVENT_JOYSTICK_BATTERY_UPDATED:
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
        case SDL_EVENT_JOYSTICK_HAT_MOTION:
        case SDL_EVENT_JOYSTICK_REMOVED:
        case SDL_EVENT_JOYSTICK_UPDATE_COMPLETE:
        case SDL_EVENT_KEYBOARD_ADDED:
        case SDL_EVENT_KEYBOARD_REMOVED:
        case SDL_EVENT_KEYMAP_CHANGED:
        case SDL_EVENT_LAST:
        case SDL_EVENT_LOCALE_CHANGED:
        case SDL_EVENT_LOW_MEMORY:
        case SDL_EVENT_MOUSE_ADDED:
        case SDL_EVENT_MOUSE_REMOVED:
        case SDL_EVENT_PEN_AXIS:
        case SDL_EVENT_PEN_BUTTON_DOWN:
        case SDL_EVENT_PEN_BUTTON_UP:
        case SDL_EVENT_PEN_DOWN:
        case SDL_EVENT_PEN_MOTION:
        case SDL_EVENT_PEN_PROXIMITY_IN:
        case SDL_EVENT_PEN_PROXIMITY_OUT:
        case SDL_EVENT_PEN_UP:
        case SDL_EVENT_PINCH_BEGIN:
        case SDL_EVENT_PINCH_END:
        case SDL_EVENT_PINCH_UPDATE:
        case SDL_EVENT_POLL_SENTINEL:
        case SDL_EVENT_PRIVATE0:
        case SDL_EVENT_PRIVATE1:
        case SDL_EVENT_PRIVATE2:
        case SDL_EVENT_PRIVATE3:
        case SDL_EVENT_QUIT:
        case SDL_EVENT_RENDER_DEVICE_LOST:
        case SDL_EVENT_RENDER_DEVICE_RESET:
        case SDL_EVENT_RENDER_TARGETS_RESET:
        case SDL_EVENT_SCREEN_KEYBOARD_HIDDEN:
        case SDL_EVENT_SCREEN_KEYBOARD_SHOWN:
        case SDL_EVENT_SENSOR_UPDATE:
        case SDL_EVENT_SYSTEM_THEME_CHANGED:
        case SDL_EVENT_TERMINATING:
        case SDL_EVENT_TEXT_EDITING_CANDIDATES:
        case SDL_EVENT_TEXT_EDITING:
        case SDL_EVENT_USER:
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
        case SDL_EVENT_WILL_ENTER_FOREGROUND:
        case SDL_EVENT_WINDOW_DESTROYED:
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_HIT_TEST:
        case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_OCCLUDED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        case SDL_EVENT_WINDOW_SETTINGS_CHANGED:
        case SDL_EVENT_WINDOW_SHOWN:
            break;
    }
}


////////////////////////////////////////////////////////////
za::UniquePtr<SDLWindowImpl> SDLWindowImpl::create(WindowSettings windowSettings)
{
    if (!WindowContext::getSDLLayer().applyGLContextSettings(windowSettings.contextSettings))
        errMsg("Failed to apply SDL GL context settings for SDL window");

    // Fullscreen style requires some tests
    if (windowSettings.fullscreen)
    {
        // Make sure there's not already a fullscreen window (only one is allowed)
        if (SDLWindowImplImpl::fullscreenWindow != nullptr)
        {
            errMsg("Creating two fullscreen windows is not allowed, switching to windowed mode");
            windowSettings.fullscreen = false;
        }
        else
        {
            // TODO P0: get refresh rate from user

            ZA_ASSERT(!VideoModeUtils::getFullscreenModes().empty() && "No video modes available");
            const auto bestFullscreenMode = VideoModeUtils::getFullscreenModes()[0];

            VideoMode videoMode{.size         = windowSettings.size,
                                .bitsPerPixel = windowSettings.bitsPerPixel,
                                .pixelDensity = bestFullscreenMode.pixelDensity,
                                .refreshRate  = bestFullscreenMode.refreshRate};

            // Make sure that the chosen video mode is compatible
            if (!videoMode.isValid())
            {
                const auto formatVideoMode = [](ErrMsgScope& s, const VideoMode& mode)
                {
                    s.fmt("{{ size: {{ {}, {} }}, bitsPerPixel: {}, pixelDensity: {}, refreshRate: {} }}",
                          mode.size.x,
                          mode.size.y,
                          mode.bitsPerPixel,
                          mode.pixelDensity,
                          mode.refreshRate);
                };

                {
                    ErrMsgScope scope;
                    scope.append("The requested video mode (");
                    formatVideoMode(scope, videoMode);
                    scope.append(") is not available, switching to a valid mode");
                }

                {
                    ErrMsgScope scope;
                    scope.append("Selected video mode (");
                    formatVideoMode(scope, bestFullscreenMode);
                    scope.append(")");
                }

                // TODO P1: actually switch?
            }
        }
    }

// Check validity of style according to the underlying platform
#if defined(ZA_SYSTEM_IOS) || defined(ZA_SYSTEM_ANDROID)
    if (windowSettings.fullscreen)
        windowSettings.hasTitlebar = false;
    else
        windowSettings.hasTitlebar = true;
#else
    if (windowSettings.closable || windowSettings.resizable)
        windowSettings.hasTitlebar = true;
#endif

    SDL_Window* sdlWindowPtr = SDL_CreateWindowWithProperties(makeSDLWindowPropertiesFromWindowSettings(windowSettings));

    if (sdlWindowPtr == nullptr)
    {
        errMsg("Failed to create window: {} (Window settings: {})",
               SDL_GetError(),
               SDLWindowImplImpl::windowSettingsToString(windowSettings));

        return nullptr;
    }

    auto* windowImplPtr = new SDLWindowImpl{"window settings",
                                            static_cast<void*>(sdlWindowPtr),
                                            /* isExternal */ false};

#ifdef ZA_SYSTEM_EMSCRIPTEN
    // This seems necessary on Emscripten to set the initial canvas size
    SDL_SetWindowSize(sdlWindowPtr, static_cast<int>(windowSettings.size.x), static_cast<int>(windowSettings.size.y));

    // The shared `SDLGlContext` creates a hidden 1x1 window during
    // `GraphicsContext::create()`, which on Emscripten sets the canvas to
    // `display: none` (via `Emscripten_HideWindow`). Subsequent regular
    // window creations don't reset that style, leaving the user's canvas
    // invisible. Force-show here to restore `display: block`, unless the
    // caller explicitly requested a hidden window.
    if (windowSettings.visible)
        SDL_ShowWindow(sdlWindowPtr);
#endif

    if (windowSettings.fullscreen)
        SDLWindowImplImpl::fullscreenWindow = windowImplPtr;
    else
        SDLWindowImplImpl::setWindowNonExclusiveFullscreenIfNeeded(windowSettings.hasTitlebar, windowSettings.size, sdlWindowPtr);

    windowImplPtr->setMinimumSize(windowSettings.minimumSize);
    windowImplPtr->setMaximumSize(windowSettings.maximumSize);
    windowImplPtr->setMouseCursorVisible(windowSettings.mouseCursorVisible);
    windowImplPtr->setKeyRepeatEnabled(windowSettings.keyRepeatEnabled);
    windowImplPtr->setJoystickThreshold(windowSettings.joystickThreshold);

    return za::UniquePtr<SDLWindowImpl>{windowImplPtr};
}


////////////////////////////////////////////////////////////
za::UniquePtr<SDLWindowImpl> SDLWindowImpl::create(const WindowHandle handle)
{
    const SDL_PropertiesID props = makeSDLWindowPropertiesFromHandle(WindowContext::getSDLLayer().getCurrentVideoDriver(),
                                                                     handle);

#if defined(ZA_SYSTEM_WINDOWS)
    // Copy the pixel format from the shared GL context's hidden window instead of calling
    // WIN_GL_SetupWindow, which fails for externally-created HWNDs (SetPixelFormat restrictions).
    const auto& sharedCtx = static_cast<const priv::SDLGlContext&>(WindowContext::getSharedGlContext());
    if (SDL_Window* const sharedSDLWindow = sharedCtx.getSDLWindow())
    {
        if (auto* const sharedHWND = SDL_GetPointerProperty(SDL_GetWindowProperties(sharedSDLWindow),
                                                            SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                                            nullptr))
            SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_PIXEL_FORMAT_HWND_POINTER, sharedHWND);
    }
#endif

    SDL_Window* sdlWindowPtr = SDL_CreateWindowWithProperties(props);

    if (sdlWindowPtr == nullptr)
    {
        errMsg("Failed to create window from handle: {}", SDL_GetError());
        return nullptr;
    }

    return za::UniquePtr<SDLWindowImpl>{new SDLWindowImpl{"handle",
                                                          static_cast<void*>(sdlWindowPtr),
                                                          /* isExternal */ true}};
}


////////////////////////////////////////////////////////////
SDLWindowImpl::~SDLWindowImpl()
{
    if (SDLWindowImplImpl::fullscreenWindow == this)
        SDLWindowImplImpl::fullscreenWindow = nullptr;

    // Unregister the window from the global map
    const auto windowId = m_impl->getWindowID();
    ZA_ASSERT(SDLWindowImplImpl::windowImplMap.contains(windowId));
    SDLWindowImplImpl::windowImplMap.erase(windowId);
}


////////////////////////////////////////////////////////////
za::Optional<Vec2u> SDLWindowImpl::getMinimumSize() const
{
    return m_impl->minimumSize;
}


////////////////////////////////////////////////////////////
za::Optional<Vec2u> SDLWindowImpl::getMaximumSize() const
{
    return m_impl->maximumSize;
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setJoystickThreshold(const float threshold)
{
    m_impl->joystickThreshold = threshold;
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setMinimumSize(const za::Optional<Vec2u>& minimumSize)
{
    m_impl->minimumSize = minimumSize;
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setMaximumSize(const za::Optional<Vec2u>& maximumSize)
{
    m_impl->maximumSize = maximumSize;
}


////////////////////////////////////////////////////////////
za::Optional<Event> SDLWindowImpl::waitEvent(const Time timeout)
{
    za::Clock clock;

    const auto timedOut = [&clock, timeout, startTime = clock.getElapsedTime()]
    {
        const bool infiniteTimeout = timeout == Time{};
        return !infiniteTimeout && (clock.getElapsedTime() - startTime) >= timeout;
    };

    // If the event queue is empty, let's first check if new events are available from the OS
    if (m_impl->events.empty())
        populateEventQueue();

    // Here we use a manual wait loop instead of the optimized wait-event provided by the OS,
    // so that we don't skip joystick events (which require polling)
    while (m_impl->events.empty() && !timedOut())
    {
        ThisThread::sleepFor(milliseconds(10));
        populateEventQueue();
    }

    return popEvent();
}


////////////////////////////////////////////////////////////
za::Optional<Event> SDLWindowImpl::pollEvent()
{
    // If the event queue is empty, let's first check if new events are available from the OS
    if (m_impl->events.empty())
        populateEventQueue();

    return popEvent();
}


////////////////////////////////////////////////////////////
za::Optional<Event> SDLWindowImpl::popEvent()
{
    za::Optional<Event> event; // Use a single local variable for NRVO

    if (!m_impl->events.empty())
    {
        event.emplace(m_impl->events.front());
        m_impl->events.erase(m_impl->events.begin());
    }

    return event;
}


////////////////////////////////////////////////////////////
SDLWindowImpl::SDLWindowImpl(const char* const context, void* const sdlWindow, const bool isExternal) :
    m_impl{context, static_cast<SDL_Window*>(sdlWindow), isExternal}
{
    auto& joystickManager = WindowContext::getJoystickManager();

    // Get the initial joystick states
    joystickManager.update();

    for (unsigned int i = 0; i < Joystick::MaxCount; ++i)
    {
        m_impl->joystickStates[i]    = joystickManager.getState(i);
        m_impl->joystickConnected[i] = joystickManager.isConnected(i);
        m_impl->previousAxes[i].fill(0.f);
    }

    // Get the initial sensor states
    for (Vec3f& vec : m_impl->sensorValue.elements)
        vec = {0.f, 0.f, 0.f};

    // Register the window in the global map
    const auto windowId = m_impl->getWindowID();
    ZA_ASSERT(!SDLWindowImplImpl::windowImplMap.contains(windowId));
    SDLWindowImplImpl::windowImplMap.emplace(windowId, this); // Needs address stability
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::pushEvent(const Event& event)
{
    m_impl->events.pushBack(event);
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::processJoystickEvents()
{
    auto& joystickManager = WindowContext::getJoystickManager();

    joystickManager.update();

    for (unsigned int i = 0; i < Joystick::MaxCount; ++i)
    {
        // Copy the previous state of the joystick and get the new one
        const JoystickState previousState = m_impl->joystickStates[i];
        m_impl->joystickStates[i]         = joystickManager.getState(i);

        // Copy the previous connection state of the joystick and get the new one
        const bool previousConnected = m_impl->joystickConnected[i];
        m_impl->joystickConnected[i] = joystickManager.isConnected(i);

        const bool connected = m_impl->joystickConnected[i];
        if (previousConnected != connected)
        {
            if (connected)
                pushEvent(Event::JoystickConnected{i});
            else
                pushEvent(Event::JoystickDisconnected{i});

            // Clear previous axes positions
            if (connected)
                m_impl->previousAxes[i].fill(0.f);
        }

        if (!connected)
            continue;

        const JoystickCapabilities caps = joystickManager.getCapabilities(i);

        // Axes
        for (unsigned int j = 0; j < Joystick::AxisCount; ++j)
        {
            const auto axis = static_cast<Joystick::Axis>(j);
            if (!caps.axes[axis])
                continue;

            const float prevPos = m_impl->previousAxes[i][axis];
            const float currPos = m_impl->joystickStates[i].axes[axis];
            if (za::fabs(currPos - prevPos) >= m_impl->joystickThreshold)
            {
                pushEvent(Event::JoystickMoved{i, axis, currPos});
                m_impl->previousAxes[i][axis] = currPos;
            }
        }

        // Buttons
        for (unsigned int j = 0; j < caps.buttonCount; ++j)
        {
            const bool prevPressed = previousState.buttons[j];
            const bool currPressed = m_impl->joystickStates[i].buttons[j];

            if (prevPressed != currPressed)
            {
                if (currPressed)
                    pushEvent(Event::JoystickButtonPressed{i, j});
                else
                    pushEvent(Event::JoystickButtonReleased{i, j});
            }
        }
    }
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::processSensorEvents()
{
    auto& sensorManager = WindowContext::getSensorManager();
    sensorManager.update();

    for (unsigned int i = 0; i < Sensor::Count; ++i)
    {
        const auto sensor = static_cast<Sensor::Type>(i);

        // Only process enabled sensors
        if (!sensorManager.isEnabled(sensor))
            continue;

        // Copy the previous value of the sensor and get the new one
        const Vec3f previousValue   = m_impl->sensorValue[sensor];
        m_impl->sensorValue[sensor] = sensorManager.getValue(sensor);

        // If the value has changed, trigger an event
        if (m_impl->sensorValue[sensor] != previousValue)
            pushEvent(Event::SensorChanged{sensor, m_impl->sensorValue[sensor]});
    }
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::populateEventQueue()
{
    processJoystickEvents();
    processSensorEvents();
    SDLWindowImpl::processEvents();

#ifdef ZA_SYSTEM_EMSCRIPTEN
    SDL_SyncWindow(m_impl->sdlWindow);
#endif
}


////////////////////////////////////////////////////////////
bool SDLWindowImpl::createVulkanSurface([[maybe_unused]] const Vulkan::VulkanSurfaceData& vulkanSurfaceData) const
{
    return Vulkan::createVulkanSurface(vulkanSurfaceData.instance,
                                       m_impl->sdlWindow,
                                       vulkanSurfaceData.surface,
                                       vulkanSurfaceData.allocator);
}


////////////////////////////////////////////////////////////
Vec2i SDLWindowImpl::getPosition() const
{
    Vec2i result;

    if (!SDL_GetWindowPosition(m_impl->sdlWindow, &result.x, &result.y))
        errMsg("Failed to get window position: {}", SDL_GetError());

    return result;
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setPosition(const Vec2i position)
{
    if (!SDL_SetWindowPosition(m_impl->sdlWindow, position.x, position.y))
        errMsg("Failed to set window position: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
Vec2u SDLWindowImpl::getSize() const
{
    ZA_ASSERT(m_impl->sdlWindow);
    return WindowContext::getSDLLayer().getWindowSize(*m_impl->sdlWindow);
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setSize(const Vec2u size)
{
    ZA_ASSERT(m_impl->sdlWindow);
    WindowContext::getSDLLayer().setWindowSize(*m_impl->sdlWindow, size);

    if (!isFullscreen())
        SDLWindowImplImpl::setWindowNonExclusiveFullscreenIfNeeded(hasTitlebar(), getSize(), m_impl->sdlWindow);
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setTitle(const Utf8String& title)
{
    if (!SDL_SetWindowTitle(m_impl->sdlWindow, title.cStr()))
        errMsg("Failed to set window title: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setIcon(const za::U8* pixels, const Vec2u size)
{
    auto surface = WindowContext::getSDLLayer().createSurfaceFromPixels(pixels, size);
    if (surface == nullptr)
    {
        errMsg("Failed to set icon");
        return;
    }

    if (!SDL_SetWindowIcon(m_impl->sdlWindow, surface.get()))
        errMsg("Failed to set window icon: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setVisible(const bool visible)
{
    if (visible)
    {
        if (!SDL_ShowWindow(m_impl->sdlWindow))
            errMsg("Failed to show window: {}", SDL_GetError());
    }
    else
    {
        if (!SDL_HideWindow(m_impl->sdlWindow))
            errMsg("Failed to hide window: {}", SDL_GetError());
    }
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setMouseCursorVisible(const bool visible)
{
    if (visible)
    {
        if (!SDL_ShowCursor())
            errMsg("Failed to show cursor: {}", SDL_GetError());
    }
    else
    {
        if (!SDL_HideCursor())
            errMsg("Failed to hide cursor: {}", SDL_GetError());
    }
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setMouseCursorGrabbed(const bool grabbed)
{
    if (!SDL_SetWindowMouseGrab(m_impl->sdlWindow, grabbed))
        errMsg("Failed to set window mouse grab: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setMouseCursor(void* const cursor)
{
    SDL_SetCursor(static_cast<SDL_Cursor*>(cursor));
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setKeyRepeatEnabled(const bool enabled)
{
    m_impl->keyRepeatEnabled = enabled;
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::requestFocus()
{
    if (!SDL_RaiseWindow(m_impl->sdlWindow))
        errMsg("Failed to raise window: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
bool SDLWindowImpl::hasFocus() const
{
    return SDL_GetWindowFlags(m_impl->sdlWindow) & SDL_WINDOW_INPUT_FOCUS;
}


////////////////////////////////////////////////////////////
float SDLWindowImpl::getDisplayScale() const
{
    return WindowContext::getSDLLayer().getDisplayScale(*m_impl->sdlWindow);
}


////////////////////////////////////////////////////////////
bool SDLWindowImpl::isFullscreen() const
{
    return SDL_GetWindowFlags(m_impl->sdlWindow) & SDL_WINDOW_FULLSCREEN;
}


////////////////////////////////////////////////////////////
bool SDLWindowImpl::isResizable() const
{
    return SDL_GetWindowFlags(m_impl->sdlWindow) & SDL_WINDOW_RESIZABLE;
}


////////////////////////////////////////////////////////////
bool SDLWindowImpl::hasTitlebar() const
{
    return !(SDL_GetWindowFlags(m_impl->sdlWindow) & SDL_WINDOW_BORDERLESS);
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setResizable(const bool resizable)
{
    if (!SDL_SetWindowResizable(m_impl->sdlWindow, resizable))
        errMsg("Failed to set window resizable: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::setHasTitlebar(const bool hasTitleBar)
{
    if (!SDL_SetWindowBordered(m_impl->sdlWindow, hasTitleBar))
        errMsg("Failed to set window titlebar: {}", SDL_GetError());
}


////////////////////////////////////////////////////////////
WindowHandle SDLWindowImpl::getNativeHandle() const
{
    const auto props = SDL_GetWindowProperties(m_impl->sdlWindow);

    return static_cast<WindowHandle>(
#if defined(ZA_SYSTEM_WINDOWS)
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr)
#elif defined(ZA_SYSTEM_LINUX_OR_BSD)
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr)
            ? reinterpret_cast<WindowHandle>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr))
            : static_cast<WindowHandle>(SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0))
#elif defined(ZA_SYSTEM_MACOS)
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr)
#elif defined(ZA_SYSTEM_IOS)
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr)
#elif defined(ZA_SYSTEM_ANDROID)
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr)
#elif defined(ZA_SYSTEM_EMSCRIPTEN)
        SDL_GetStringProperty(props, SDL_PROP_WINDOW_EMSCRIPTEN_CANVAS_ID_STRING, nullptr)
#endif
    );
}


////////////////////////////////////////////////////////////
SDL_Window* SDLWindowImpl::getSDLHandle() const
{
    return m_impl->sdlWindow;
}


////////////////////////////////////////////////////////////
void SDLWindowImpl::processEvents()
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
        if (SDL_Window* window = SDL_GetWindowFromEvent(&e))
        {
            const SDL_WindowID windowID = SDL_GetWindowID(window);
            const auto*        it       = SDLWindowImplImpl::windowImplMap.find(windowID);

            if (it != SDLWindowImplImpl::windowImplMap.end())
                it->second->processSDLEvent(e);
        }
}

} // namespace za::priv
