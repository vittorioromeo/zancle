#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/Export.hpp"

#include "Zancle/Window/Joystick.hpp"
#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/Mouse.hpp"
#include "Zancle/Window/Sensor.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Vocabulary/Variant.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Defines a system event and its parameters
///
////////////////////////////////////////////////////////////
class ZA_WINDOW_API Event
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Closed event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct Closed
    {
    };

    ////////////////////////////////////////////////////////////
    /// \brief Resized event subtype
    ///
    /// Resize events are sent when a window is resized, or when
    /// the orientation is changed on a mobile platform even if
    /// a resizable window was not requested
    ///
    ////////////////////////////////////////////////////////////
    struct Resized
    {
        Vec2u size; //!< New size, in pixels
    };

    ////////////////////////////////////////////////////////////
    /// \brief Lost focus event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct FocusLost
    {
    };

    ////////////////////////////////////////////////////////////
    /// \brief Gained focus event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct FocusGained
    {
    };

    ////////////////////////////////////////////////////////////
    /// \brief Text event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct TextEntered
    {
        char32_t unicode{}; //!< UTF-32 Unicode value of the character
    };

    ////////////////////////////////////////////////////////////
    /// \brief Key pressed event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct KeyPressed
    {
        Keyboard::Key      code{};       //!< Code of the key that has been pressed
        Keyboard::Scancode scancode{};   //!< Physical code of the key that has been pressed
        bool               alt{};        //!< Is the Alt key pressed?
        bool               control{};    //!< Is the Control key pressed?
        bool               shift{};      //!< Is the Shift key pressed?
        bool               system{};     //!< Is the System key pressed?
        bool               capsLock{};   //!< Is the CapsLock key toggled?
        bool               numLock{};    //!< Is the NumLock key toggled?
        bool               scrollLock{}; //!< Is the ScrollLock key toggled?
    };

    ////////////////////////////////////////////////////////////
    /// \brief Key released event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct KeyReleased
    {
        Keyboard::Key      code{};       //!< Code of the key that has been released
        Keyboard::Scancode scancode{};   //!< Physical code of the key that has been released
        bool               alt{};        //!< Is the Alt key pressed?
        bool               control{};    //!< Is the Control key pressed?
        bool               shift{};      //!< Is the Shift key pressed?
        bool               system{};     //!< Is the System key pressed?
        bool               capsLock{};   //!< Is the CapsLock key toggled?
        bool               numLock{};    //!< Is the NumLock key toggled?
        bool               scrollLock{}; //!< Is the ScrollLock key toggled?
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse wheel scrolled event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct MouseWheelScrolled
    {
        Mouse::Wheel wheel{}; //!< Which wheel (for mice with multiple ones)
        float delta{}; //!< Wheel offset (positive is up/left, negative is down/right). High-precision mice may use non-integral offsets.
        Vec2i position; //!< Position of the mouse pointer, relative to the top left of the owner window
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse button pressed event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct MouseButtonPressed
    {
        Mouse::Button button{}; //!< Code of the button that has been pressed
        Vec2i         position; //!< Position of the mouse pointer, relative to the top left of the owner window
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse button released event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct MouseButtonReleased
    {
        Mouse::Button button{}; //!< Code of the button that has been released
        Vec2i         position; //!< Position of the mouse pointer, relative to the top left of the owner window
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse move event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct MouseMoved
    {
        Vec2i position; //!< Position of the mouse pointer, relative to the top left of the owner window
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse move raw event subtype
    ///
    /// Raw mouse input data comes unprocessed from the
    /// operating system hence "raw". While the MouseMoved
    /// position value is dependent on the screen resolution,
    /// raw data is not. If the physical mouse is moved too
    /// little to cause the screen cursor to move at least a
    /// single pixel, no MouseMoved event will be generated. In
    /// contrast, any movement information generated by the
    /// mouse independent of its sensor resolution will always
    /// generate a `MouseMovedRaw` event.
    ///
    /// In addition to screen resolution independence, raw
    /// mouse data also does not have mouse acceleration or
    /// smoothing applied to it as MouseMoved does.
    ///
    /// Raw mouse movement data is intended for controlling
    /// non-cursor movement, e.g. controlling the camera
    /// orientation in a first person view, whereas MouseMoved
    /// is intended primarily for controlling things related to
    /// the screen cursor hence the additional processing
    /// applied to it.
    ///
    /// Currently, raw mouse input events will only be generated
    /// on Windows and Linux.
    ///
    ////////////////////////////////////////////////////////////
    struct MouseMovedRaw
    {
        Vec2i delta; ///< Delta movement of the mouse since the last event
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse entered event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct MouseEntered
    {
    };

    ////////////////////////////////////////////////////////////
    /// \brief Mouse left event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct MouseLeft
    {
    };

    ////////////////////////////////////////////////////////////
    /// \brief Joystick button pressed event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct JoystickButtonPressed
    {
        unsigned int joystickId{}; //!< Index of the joystick (in range [0 .. Joystick::MaxCount - 1])
        unsigned int button{}; //!< Index of the button that has been pressed (in range [0 .. Joystick::ButtonCount - 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Joystick button released event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct JoystickButtonReleased
    {
        unsigned int joystickId{}; //!< Index of the joystick (in range [0 .. Joystick::MaxCount - 1])
        unsigned int button{}; //!< Index of the button that has been released (in range [0 .. Joystick::ButtonCount - 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Joystick axis move event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct JoystickMoved
    {
        unsigned int   joystickId{}; //!< Index of the joystick (in range [0 .. Joystick::MaxCount - 1])
        Joystick::Axis axis{};       //!< Axis on which the joystick moved
        float          position{};   //!< New position on the axis (in range [-100 .. 100])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Joystick connected event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct JoystickConnected
    {
        unsigned int joystickId{}; //!< Index of the joystick (in range [0 .. Joystick::MaxCount - 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Joystick disconnected event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct JoystickDisconnected
    {
        unsigned int joystickId{}; //!< Index of the joystick (in range [0 .. Joystick::MaxCount - 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Touch began event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct TouchBegan
    {
        unsigned int finger{};   //!< Index of the finger in case of multi-touch events
        Vec2i        position;   //!< Start position of the touch, relative to the top left of the owner window
        float        pressure{}; //!< Pressure of the touch (in range [0, 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Touch moved event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct TouchMoved
    {
        unsigned int finger{};   //!< Index of the finger in case of multi-touch events
        Vec2i        position;   //!< Current position of the touch, relative to the top left of the owner window
        float        pressure{}; //!< Pressure of the touch (in range [0, 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Touch ended event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct TouchEnded
    {
        unsigned int finger{};   //!< Index of the finger in case of multi-touch events
        Vec2i        position;   //!< Final position of the touch, relative to the top left of the owner window
        float        pressure{}; //!< Pressure of the touch (in range [0, 1])
    };

    ////////////////////////////////////////////////////////////
    /// \brief Sensor event subtype
    ///
    ////////////////////////////////////////////////////////////
    struct SensorChanged
    {
        Sensor::Type type{}; //!< Type of the sensor
        Vec3f        value;  //!< Current value of the sensor on the X, Y, and Z axes
    };

    ////////////////////////////////////////////////////////////
    /// \brief Deleted default constructor
    ///
    ////////////////////////////////////////////////////////////
    Event() = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Construct from a given `za::Event` subtype
    ///
    /// \tparam `TEventSubtype` Type of event subtype used to construct the event
    ///
    /// \param eventSubtype Event subtype instance used to construct the event
    ///
    ////////////////////////////////////////////////////////////
    template <typename TEventSubtype>
    [[nodiscard]] /* implicit */ Event(const TEventSubtype& eventSubtype);

    ////////////////////////////////////////////////////////////
    /// \brief Check current event subtype
    ///
    /// \tparam `TEventSubtype` Type of the event subtype to check against
    ///
    /// \return `true` if the current event subtype matches given template parameter
    ///
    ////////////////////////////////////////////////////////////
    template <typename TEventSubtype>
    [[nodiscard]] bool is() const;

    ////////////////////////////////////////////////////////////
    /// \brief Attempt to get specified event subtype
    ///
    /// \tparam `TEventSubtype` Type of the desired event subtype
    ///
    /// \return Address of current event subtype on success, `nullptr` otherwise
    ///
    ////////////////////////////////////////////////////////////
    template <typename TEventSubtype>
    [[nodiscard]] TEventSubtype* getIf();

    ////////////////////////////////////////////////////////////
    /// \brief Attempt to get specified event subtype
    ///
    /// \tparam `TEventSubtype` Type of the desired event subtype
    ///
    /// \return Address of current event subtype on success, `nullptr` otherwise
    ///
    ////////////////////////////////////////////////////////////
    template <typename TEventSubtype>
    [[nodiscard]] const TEventSubtype* getIf() const;

    ////////////////////////////////////////////////////////////
    /// \brief Applies the specified `visitor` to the event
    ///
    /// \return Transparently forwards whatever `visitor` returns
    ///
    ////////////////////////////////////////////////////////////
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor)
    {
        return m_data.linearVisit(static_cast<Visitor&&>(visitor));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Applies the specified `visitor` to the event
    ///
    /// \return Transparently forwards whatever `visitor` returns
    ///
    ////////////////////////////////////////////////////////////
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const
    {
        return m_data.linearVisit(static_cast<Visitor&&>(visitor));
    }


    ////////////////////////////////////////////////////////////
    /// \brief Invokes `visit` with an overload created from `handlers...`
    ///
    /// \return Transparently forwards whatever `visit` returns
    ///
    ////////////////////////////////////////////////////////////
    template <typename... Handlers>
    decltype(auto) match(Handlers&&... handlers)
    {
        return m_data.linearMatch(static_cast<Handlers&&>(handlers)...);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Invokes `visit` with an overload created from `handlers...`
    ///
    /// \return Transparently forwards whatever `visit` returns
    ///
    ////////////////////////////////////////////////////////////
    template <typename... Handlers>
    decltype(auto) match(Handlers&&... handlers) const
    {
        return m_data.linearMatch(static_cast<Handlers&&>(handlers)...);
    }

private:
    // clang-format off

    #define ZA_PRIV_EVENTS_X_MACRO(x, xSep)         \
        x(::za::Event::Closed)                 xSep() \
        x(::za::Event::Resized)                xSep() \
        x(::za::Event::FocusLost)              xSep() \
        x(::za::Event::FocusGained)            xSep() \
        x(::za::Event::TextEntered)            xSep() \
        x(::za::Event::KeyPressed)             xSep() \
        x(::za::Event::KeyReleased)            xSep() \
        x(::za::Event::MouseWheelScrolled)     xSep() \
        x(::za::Event::MouseButtonPressed)     xSep() \
        x(::za::Event::MouseButtonReleased)    xSep() \
        x(::za::Event::MouseMoved)             xSep() \
        x(::za::Event::MouseMovedRaw)          xSep() \
        x(::za::Event::MouseEntered)           xSep() \
        x(::za::Event::MouseLeft)              xSep() \
        x(::za::Event::JoystickButtonPressed)  xSep() \
        x(::za::Event::JoystickButtonReleased) xSep() \
        x(::za::Event::JoystickMoved)          xSep() \
        x(::za::Event::JoystickConnected)      xSep() \
        x(::za::Event::JoystickDisconnected)   xSep() \
        x(::za::Event::TouchBegan)             xSep() \
        x(::za::Event::TouchMoved)             xSep() \
        x(::za::Event::TouchEnded)             xSep() \
        x(::za::Event::SensorChanged)

    // clang-format on

#define ZA_PRIV_EVENT_X_EXPAND(x) x
#define ZA_PRIV_EVENT_X_COMMA()   ,

#define ZA_PRIV_EVENT_VARIANT_TYPE ::za::Variant<ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_EXPAND, ZA_PRIV_EVENT_X_COMMA)>

    using VariantType = ZA_PRIV_EVENT_VARIANT_TYPE;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    VariantType m_data; //!< Event data
};

} // namespace za


////////////////////////////////////////////////////////////
// Explicit instantiation declarations
////////////////////////////////////////////////////////////
extern template class ZA_PRIV_EVENT_VARIANT_TYPE;

#define ZA_PRIV_EVENT_X_EXTERN_TEMPLATE_CTOR(x)  extern template za::Event::Event(const x&);
#define ZA_PRIV_EVENT_X_EXTERN_TEMPLATE_IS(x)    extern template bool za::Event::is<x>() const;
#define ZA_PRIV_EVENT_X_EXTERN_TEMPLATE_GETIF(x) extern template const x* za::Event::getIf<x>() const;

#define ZA_PRIV_EVENT_X_SEMICOLON() ;

ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_EXTERN_TEMPLATE_GETIF, ZA_PRIV_EVENT_X_SEMICOLON);


////////////////////////////////////////////////////////////
/// \class za::Event
/// \ingroup window
///
/// `za::Event` holds all the information about a system event
/// that just happened. Events are obtained via
/// `za::WindowBase::pollEvent`, `za::WindowBase::waitEvent`,
/// or dispatched in bulk through
/// `za::WindowBase::pollAndHandleEvents`.
///
/// A `za::Event` instance contains the subtype of the event
/// (mouse moved, key pressed, window closed, etc.) as well
/// as the details about this particular event. Each event
/// corresponds to a different subtype struct which contains
/// the data required to process that event.
///
/// Event subtypes are nested types of `za::Event`, such as
/// `za::Event::Closed` or `za::Event::MouseMoved`.
///
/// The simplest way to inspect an event is to use
/// `za::Event::is<T>` (just check the subtype) and
/// `za::Event::getIf<T>` (return a pointer to the subtype's
/// data, or `nullptr` if the event is not of that type).
/// More advanced patterns are available through
/// `za::Event::visit` and `za::Event::match`, which dispatch
/// to a set of handlers based on the active subtype.
///
/// \code
/// while (const za::Optional event = window.pollEvent())
/// {
///     // Window closed or escape key pressed: exit
///     if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
///         return 0; // break out of both event and main loops
///
///     // The window was resized
///     if (const auto* resized = event->getIf<za::Event::Resized>())
///         doSomethingWithTheNewSize(resized->size);
///
///     // etc ...
/// }
/// \endcode
///
////////////////////////////////////////////////////////////
