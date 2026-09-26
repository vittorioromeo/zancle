#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/Export.hpp"

#include "Zancle/Vocabulary/Optional.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Utf8String;
} // namespace za


namespace za::priv
{
class JoystickManager;
struct JoystickIdentification;
} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Give access to the real-time state of the joysticks
///
////////////////////////////////////////////////////////////
struct ZA_WINDOW_API Joystick
{
    ////////////////////////////////////////////////////////////
    /// \brief Axes supported by Zancle joysticks
    ///
    ////////////////////////////////////////////////////////////
    enum class Axis
    {
        X,    //!< The X axis
        Y,    //!< The Y axis
        Z,    //!< The Z axis
        R,    //!< The R axis
        U,    //!< The U axis
        V,    //!< The V axis
        PovX, //!< The X axis of the point-of-view hat
        PovY  //!< The Y axis of the point-of-view hat
    };

    ////////////////////////////////////////////////////////////
    /// \brief Constants related to joysticks capabilities
    ///
    ////////////////////////////////////////////////////////////
    enum : unsigned int
    {
        MaxCount    = 8u,  //!< Maximum number of supported joysticks
        ButtonCount = 32u, //!< Maximum number of supported buttons
        AxisCount   = 8u   //!< Maximum number of supported axes
    };

    ////////////////////////////////////////////////////////////
    /// \brief Refresh the cached state of every joystick
    ///
    /// This function is used internally by Zancle's event loop,
    /// so you normally don't have to call it explicitly.
    /// However, you may need to call it if you want to query
    /// joystick state without having created a window yet (or
    /// without having a window at all): in that case the
    /// joystick states are not refreshed automatically.
    ///
    ////////////////////////////////////////////////////////////
    static void update();

    // Forward declaration for friendship
    class Query;

    ////////////////////////////////////////////////////////////
    /// \brief Open a query handle on a specific joystick slot
    ///
    /// Returns a `Query` object bound to the joystick at the
    /// given index, or `za::nullOpt` if no joystick is
    /// currently connected at that slot.
    ///
    /// The returned `Query` only stays valid as long as the
    /// joystick remains connected. If the joystick is
    /// disconnected, you should drop the query and call
    /// `query()` again.
    ///
    /// \param joystickId Index of the joystick to query, in `[0, MaxCount - 1]`
    ///
    /// \return `Query` for the joystick on success, `za::nullOpt` if it is not connected
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Query> query(unsigned int joystickId);
};

////////////////////////////////////////////////////////////
/// \brief Cheap, read-only handle to a connected joystick
///
/// `Query` is a thin wrapper that bundles a reference to the
/// joystick manager with the joystick index, exposing the
/// real-time state of one joystick (axes, buttons, name,
/// vendor / product IDs).
///
/// Instances are obtained from `za::Joystick::query`.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_WINDOW_API Joystick::Query
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Get the index of the joystick this query is bound to
    ///
    /// \return Joystick index, in `[0, MaxCount - 1]`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getIndex() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the human-readable name of the joystick
    ///
    /// \return Joystick name as reported by the OS
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Utf8String& getName() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the USB vendor identifier of the joystick
    ///
    /// \return Vendor identifier reported by the OS (`0` if unknown)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getVendorId() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the USB product identifier of the joystick
    ///
    /// \return Product identifier reported by the OS (`0` if unknown)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getProductId() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the number of buttons supported by a joystick
    ///
    /// \return Number of buttons supported by the joystick
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getButtonCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Check if a joystick supports a given axis
    ///
    /// \param axis Axis to check
    ///
    /// \return `true` if the joystick supports the axis, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasAxis(Axis axis) const;

    ////////////////////////////////////////////////////////////
    /// \brief Check if a joystick button is pressed
    ///
    /// \param button Button to check
    ///
    /// \return `true` if the button is pressed, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isButtonPressed(unsigned int button) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the current position of a joystick axis
    ///
    /// \param axis Axis to check
    ///
    /// \return Current position of the axis, in range [-100 .. 100]
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getAxisPosition(Axis axis) const;

private:
    friend za::Optional<Query> Joystick::query(unsigned int joystickId);

    ////////////////////////////////////////////////////////////
    /// \brief Create a joystick state query
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Query(const priv::JoystickManager& joystickManager, unsigned int joystickId);

    ////////////////////////////////////////////////////////////
    /// \brief Check if a joystick is connected
    ///
    /// \return `true` if the joystick is connected, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isConnected() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the identification of the joystick
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const priv::JoystickIdentification& getIdentification() const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    const priv::JoystickManager& m_joystickManager;
    unsigned int                 m_joystickId;
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::Joystick
/// \ingroup window
///
/// `za::Joystick` provides an interface to the state of the
/// joysticks. Each joystick is identified by an index that is
/// passed to `za::Joystick::query`, which returns a
/// `za::Joystick::Query` object exposing the joystick's name,
/// vendor / product IDs, supported axes, and real-time button
/// / axis state.
///
/// This struct allows users to query the state of joysticks at any
/// time and directly, without having to deal with a window and
/// its events. Compared to the `JoystickMoved`, `JoystickButtonPressed`
/// and `JoystickButtonReleased` events, `za::Joystick` can retrieve the
/// state of axes and buttons of joysticks at any time
/// (you don't need to store and update a boolean on your side
/// in order to know if a button is pressed or released), and you
/// always get the real state of joysticks, even if they are
/// moved, pressed or released when your window is out of focus
/// and no event is triggered.
///
/// Zancle supports:
/// \li 8 joysticks (`za::Joystick::MaxCount`)
/// \li 32 buttons per joystick (`za::Joystick::ButtonCount`)
/// \li 8 axes per joystick (`za::Joystick::AxisCount`)
///
/// Unlike the keyboard or mouse, the state of joysticks is sometimes
/// not directly available (depending on the OS), therefore an `update()`
/// function must be called in order to update the current state of
/// joysticks. When you have a window with event handling, this is done
/// automatically, you don't need to call anything. But if you have no
/// window, or if you want to check joysticks state before creating one,
/// you must call `za::Joystick::update` explicitly.
///
/// Usage example:
/// \code
/// const auto query = za::Joystick::query(0);
///
/// if (!query.hasValue()) // Is joystick #0 disconnected?
///     return;
///
/// // How many buttons does joystick #0 support?
/// const unsigned int buttonCount = query->getButtonCount();
///
/// // Does joystick #0 define an X axis?
/// const bool hasX = query->hasAxis(za::Joystick::Axis::X);
///
/// // Is button #2 pressed on joystick #0?
/// const bool pressed = query->isButtonPressed(2);
///
/// // What's the current position of the Y axis on joystick #0?
/// const float position = query->getAxisPosition(za::Joystick::Axis::Y);
/// \endcode
///
/// \see `za::Keyboard`, `za::Mouse`
///
////////////////////////////////////////////////////////////
