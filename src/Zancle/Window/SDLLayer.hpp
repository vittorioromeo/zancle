#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/Cursor.hpp"
#include "Zancle/Window/DisplayOrientation.hpp"
#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/Mouse.hpp"
#include "Zancle/Window/WindowHandle.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/TrivialAbi.hpp"

#include "Zancle/Trait/IsSame.hpp"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_touch.h>
#include <SDL3/SDL_video.h>


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
struct SDL_Joystick;
struct SDL_GUID;

namespace za
{
class Utf8String;
struct ContextSettings;
struct VideoMode;
struct WindowSettings;
} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] Keyboard::Scan mapSDLScancodeToZancle(SDL_Scancode sdlCode) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] SDL_Scancode mapZancleScancodeToSDL(Keyboard::Scan scanCode) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] Keyboard::Key mapSDLKeycodeToZancle(SDL_Keycode sdlKey) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] SDL_Keycode mapZancleKeycodeToSDL(Keyboard::Key key) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] SDL_SystemCursor cursorTypeToSDLCursor(za::Cursor::Type type) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard]] SDL_PropertiesID makeSDLWindowPropertiesFromHandle(const char* currentVideoDriver, WindowHandle handle);

////////////////////////////////////////////////////////////
[[nodiscard]] SDL_PropertiesID makeSDLWindowPropertiesFromWindowSettings(const WindowSettings& windowSettings);

////////////////////////////////////////////////////////////
[[nodiscard, gnu::pure]] SDL_WindowFlags makeSDLWindowFlagsFromWindowSettings(const WindowSettings& windowSettings) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] Mouse::Button getZancleButtonFromSDLButton(za::U8 sdlButton) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] za::U8 getSDLButtonFromZancleButton(Mouse::Button button) noexcept;

////////////////////////////////////////////////////////////
[[nodiscard, gnu::const]] DisplayOrientation mapSDLDisplayOrientationToZancle(SDL_DisplayOrientation displayOrientation);

////////////////////////////////////////////////////////////
struct ZA_TRIVIAL_ABI UniquePtrSDLDeleter
{
    template <typename T>
    [[gnu::always_inline]] void operator()(T* const ptr) const noexcept
    {
        static_assert(!ZA_IS_SAME(T, void), "can't delete pointer to incomplete type");

        // NOLINTNEXTLINE(bugprone-sizeof-expression)
        static_assert(sizeof(T) > 0u, "can't delete pointer to incomplete type");

        SDL_free(static_cast<void*>(ptr));
    }
};

////////////////////////////////////////////////////////////
template <typename T>
using SDLUPtr = za::UniquePtr<T, UniquePtrSDLDeleter>;

////////////////////////////////////////////////////////////
struct ZA_TRIVIAL_ABI UniquePtrSDLSurfaceDeleter
{
    [[gnu::always_inline]] void operator()(SDL_Surface* const ptr) const noexcept
    {
        SDL_DestroySurface(ptr);
    }
};

////////////////////////////////////////////////////////////
using SDLSurfaceUPtr = za::UniquePtr<SDL_Surface, UniquePtrSDLSurfaceDeleter>;

////////////////////////////////////////////////////////////
template <typename T>
struct SDLAllocatedArray
{
    ////////////////////////////////////////////////////////////
    SDLUPtr<T> ptr;
    za::SizeT  count;


    ////////////////////////////////////////////////////////////
    explicit SDLAllocatedArray(SDLUPtr<T>&& thePtr, const za::SizeT theCount) : ptr{ZA_MOVE(thePtr)}, count{theCount}
    {
    }


    ////////////////////////////////////////////////////////////
    SDLAllocatedArray(decltype(nullptr)) : ptr{nullptr}, count{0u}
    {
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] T* get() noexcept
    {
        return ptr.get();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const T* get() const noexcept
    {
        return ptr.get();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] T& operator[](const za::SizeT index) noexcept
    {
        ZA_ASSERT(ptr != nullptr);
        ZA_ASSERT(index < count);

        return ptr.get()[index];
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const T& operator[](const za::SizeT index) const noexcept
    {
        ZA_ASSERT(ptr != nullptr);
        ZA_ASSERT(index < count);

        return ptr.get()[index];
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool valid() const noexcept
    {
        return ptr != nullptr;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT size() const noexcept
    {
        return count;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool empty() const noexcept
    {
        return count == 0u;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] T* begin() noexcept
    {
        return ptr.get();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] T* end() noexcept
    {
        return ptr.get() + count;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const T* begin() const noexcept
    {
        return ptr.get();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const T* end() const noexcept
    {
        return ptr.get() + count;
    }
};


////////////////////////////////////////////////////////////
class SDLLayer
{
public:
    ////////////////////////////////////////////////////////////
    explicit SDLLayer();

    ////////////////////////////////////////////////////////////
    ~SDLLayer();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDLAllocatedArray<SDL_DisplayID> getDisplays() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDLAllocatedArray<SDL_DisplayMode*> getFullscreenDisplayModesForDisplay(SDL_DisplayID displayId);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const SDL_PixelFormatDetails* getPixelFormatDetails(SDL_PixelFormat format) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const SDL_DisplayMode* getDesktopDisplayMode(SDL_DisplayID displayId) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const SDL_DisplayMode* getPrimaryDisplayDesktopDisplayMode() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDLAllocatedArray<SDL_TouchID> getTouchDevices();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDLAllocatedArray<SDL_Finger*> getTouchFingers(SDL_TouchID touchDeviceId);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDL_TouchDeviceType getTouchDeviceType(SDL_TouchID touchDeviceId);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* getTouchDeviceName(SDL_TouchID touchDeviceId);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Keyboard::Key localizeScancode(Keyboard::Scancode code) const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Keyboard::Scancode delocalizeScancode(Keyboard::Key key) const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isKeyPressedByScancode(Keyboard::Scancode code) const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* getScancodeDescription(Keyboard::Scancode code) const noexcept;

    ////////////////////////////////////////////////////////////
    void setVirtualKeyboardVisible(bool visible) const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2i getGlobalMousePosition() const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setGlobalMousePosition(Vec2i position) const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Utf8String getClipboardString() const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setClipboardString(const Utf8String& string) const noexcept;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getDisplayContentScale(SDL_DisplayID displayID) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Utf8String getDisplayName(SDL_DisplayID displayID) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Rect2i getDisplayBounds(SDL_DisplayID displayID) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Rect2i getDisplayUsableBounds(SDL_DisplayID displayID) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] DisplayOrientation getNaturalDisplayOrientation(SDL_DisplayID displayID) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] DisplayOrientation getCurrentDisplayOrientation(SDL_DisplayID displayID) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getPrimaryDisplayContentScale() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getDisplayScale(SDL_Window& window) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SDLSurfaceUPtr createSurfaceFromPixels(const za::U8* pixels, Vec2u size) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getJoystickButtonCount(SDL_Joystick& handle);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getJoystickAxisCount(SDL_Joystick& handle);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getJoystickHatCount(SDL_Joystick& handle);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* getJoystickName(SDL_Joystick& handle);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getJoystickVendor(SDL_Joystick& handle);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getJoystickProduct(SDL_Joystick& handle);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool areGUIDsEqual(const SDL_GUID& a, const SDL_GUID& b);

    ////////////////////////////////////////////////////////////
    void setWindowSize(SDL_Window& window, Vec2u size) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2u getWindowSize(SDL_Window& window) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] VideoMode getVideoModeFromSDLDisplayMode(const SDL_DisplayMode& mode) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setGLAttribute(int attribute, int value) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool applyGLContextSettings(const ContextSettings& settings) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] const char* getCurrentVideoDriver() const;
};

} // namespace za::priv
